/*
 * Hospital Alarm System - Slave Device
 * Target: ESP32 V1 (ESP32-WROOM-32 / DevKit V1)
 * 
 * Board: "ESP32 Dev Module" in Arduino IDE
 * Pins:  GPIO0  = BOOT button (built-in, used as call button)
 *        GPIO2  = Onboard LED (active HIGH)
 *        GPIO4  = Buzzer (optional, active HIGH)
 *        GPIO15 = External call button (optional, wire to GND)
 *
 * Libraries required:
 *   - ArduinoJson (v7.x)
 *   - ESPAsyncWebServer
 *   - AsyncTCP
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#define BUTTON_PIN    0
#define EXT_BUTTON_PIN 15
#define LED_PIN       2
#define BUZZER_PIN    4
#define DEBOUNCE_MS   300
#define REGISTER_INTERVAL_MS 10000
#define HEARTBEAT_INTERVAL_MS 30000
#define RECONNECT_INTERVAL_MS 5000

char slaveId[32] = "";
char targetSSID[64] = "";
char targetPass[64] = "";
char masterURL[128] = "http://192.168.4.1";

bool wifiConnected = false;
bool isRegistered = false;
bool setupDone = false;
bool isApproved = false;
volatile bool buttonPressed = false;
unsigned long lastButtonPress = 0;
unsigned long lastRegisterAttempt = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastReconnect = 0;
unsigned long alertSentTime = 0;
bool alertPending = false;

AsyncWebServer setupServer(80);

void generateSlaveId() {
  uint64_t mac = ESP.getEfuseMac();
  uint16_t chip = (uint16_t)(mac & 0xFFFF);
  snprintf(slaveId, sizeof(slaveId), "slave-%04X", chip);
}

String getApName() {
  uint64_t mac = ESP.getEfuseMac();
  uint16_t chip = (uint16_t)(mac & 0xFFFF);
  char buf[32];
  snprintf(buf, sizeof(buf), "SlaveSetup-%04X", chip);
  return String(buf);
}

void ledBlink(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(onMs);
    digitalWrite(LED_PIN, LOW);
    if (i < times - 1) delay(offMs);
  }
}

void beep(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(onMs);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < times - 1) delay(offMs);
  }
}

const char SLAVE_SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Slave Device Setup</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:linear-gradient(135deg,#0f172a 0%,#1e293b 50%,#0f172a 100%);color:#e2e8f0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.container{width:100%;max-width:480px}
.card{background:rgba(30,41,59,.9);border:1px solid rgba(148,163,184,.15);border-radius:20px;padding:32px;backdrop-filter:blur(20px);box-shadow:0 20px 60px rgba(0,0,0,.4)}
.logo{text-align:center;margin-bottom:24px}
.logo-icon{width:64px;height:64px;background:linear-gradient(135deg,#10b981,#059669);border-radius:16px;display:inline-flex;align-items:center;justify-content:center;font-size:32px;margin-bottom:10px;box-shadow:0 8px 24px rgba(16,185,129,.3)}
.logo h1{font-size:20px;font-weight:700;background:linear-gradient(135deg,#6ee7b7,#34d399);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.logo p{color:#94a3b8;font-size:13px;margin-top:4px}
.step{display:none}.step.active{display:block}
.form-group{margin-bottom:14px}
.form-group label{display:block;font-size:13px;font-weight:500;color:#94a3b8;margin-bottom:6px}
.form-group input{width:100%;padding:10px 14px;background:rgba(15,23,42,.6);border:1px solid rgba(148,163,184,.2);border-radius:8px;color:#e2e8f0;font-size:14px;outline:none;transition:border .3s}
.form-group input:focus{border-color:#10b981;box-shadow:0 0 0 3px rgba(16,185,129,.15)}
.btn{width:100%;padding:12px;border:none;border-radius:10px;font-size:15px;font-weight:600;cursor:pointer;transition:all .3s;margin-top:12px}
.btn-primary{background:linear-gradient(135deg,#10b981,#059669);color:#fff}.btn-primary:hover{transform:translateY(-1px);box-shadow:0 6px 16px rgba(16,185,129,.4)}
.btn-secondary{background:rgba(51,65,85,.5);color:#94a3b8;border:1px solid rgba(148,163,184,.2)}.btn-secondary:hover{background:rgba(51,65,85,.8)}
.btn:disabled{opacity:.5;cursor:not-allowed;transform:none}
.wifi-list{max-height:200px;overflow-y:auto;margin:12px 0;border-radius:8px;border:1px solid rgba(148,163,184,.1)}
.wifi-item{padding:10px 14px;display:flex;justify-content:space-between;align-items:center;cursor:pointer;transition:background .2s;border-bottom:1px solid rgba(148,163,184,.05)}
.wifi-item:last-child{border-bottom:none}
.wifi-item:hover{background:rgba(16,185,129,.1)}
.wifi-item.selected{background:rgba(16,185,129,.15);border-left:3px solid #10b981}
.wifi-name{font-weight:500;font-size:14px}
.wifi-signal{font-size:12px;color:#94a3b8}
.scanning{text-align:center;padding:24px;color:#94a3b8}
.spinner{display:inline-block;width:24px;height:24px;border:3px solid rgba(148,163,184,.2);border-top-color:#10b981;border-radius:50%;animation:spin 1s linear infinite;margin-bottom:8px}
@keyframes spin{to{transform:rotate(360deg)}}
.status{padding:10px 14px;border-radius:8px;font-size:13px;margin-top:12px;text-align:center}
.status.ok{background:rgba(34,197,94,.1);color:#4ade80;border:1px solid rgba(34,197,94,.2)}
.status.err{background:rgba(239,68,68,.1);color:#f87171;border:1px solid rgba(239,68,68,.2)}
.status.info{background:rgba(59,130,246,.1);color:#60a5fa;border:1px solid rgba(59,130,246,.2)}
.status.warn{background:rgba(251,191,36,.1);color:#fbbf24;border:1px solid rgba(251,191,36,.2)}
.progress{display:flex;justify-content:center;gap:8px;margin-bottom:20px}
.progress .dot{width:10px;height:10px;border-radius:50%;background:rgba(148,163,184,.3);transition:all .3s}
.progress .dot.active{background:#10b981;box-shadow:0 0 8px rgba(16,185,129,.4)}
.progress .dot.done{background:#22c55e}
.chip-id{display:inline-block;background:rgba(16,185,129,.15);color:#6ee7b7;padding:4px 12px;border-radius:6px;font-family:monospace;font-size:13px;font-weight:600;margin-bottom:16px}
</style>
</head>
<body>
<div class="container">
<div class="card">
<div class="progress"><div class="dot active" id="d1"></div><div class="dot" id="d2"></div><div class="dot" id="d3"></div></div>
<div class="logo">
<div class="logo-icon">&#128276;</div>
<h1>Slave Device Setup</h1>
<p>Patient Call Button</p>
</div>

<div class="step active" id="step1">
<div style="text-align:center"><span class="chip-id" id="chip-id"></span></div>
<div class="form-group">
<label>Device ID (auto-generated from chip)</label>
<input type="text" id="dev-id" placeholder="e.g. slave-A1B2">
</div>
<div class="form-group">
<label>Master IP Address</label>
<input type="text" id="master-ip" placeholder="e.g. 192.168.4.1" value="192.168.4.1">
</div>
<h3 style="font-size:14px;margin:16px 0 8px;color:#94a3b8">Select Wi-Fi Network</h3>
<p style="font-size:12px;color:#64748b;margin-bottom:8px">Choose the same network the master is on, or the master's AP (HospitalAlarm)</p>
<div id="scan-area"><div class="scanning"><div class="spinner"></div><br>Scanning nearby networks...</div></div>
<div class="form-group" style="margin-top:12px">
<label>Wi-Fi Password (leave empty for open networks)</label>
<input type="password" id="wifi-pass" placeholder="Network password">
</div>
<button class="btn btn-primary" id="connectBtn" onclick="doConnect()" disabled>Connect & Register</button>
<button class="btn btn-secondary" onclick="scanWifi()">Rescan Networks</button>
<div id="status-area"></div>
</div>

<div class="step" id="step2">
<div style="text-align:center;padding:20px 0">
<div style="font-size:48px;margin-bottom:12px">&#9989;</div>
<h2 style="font-size:18px;margin-bottom:6px">Device Connected!</h2>
<p id="conn-info" style="color:#94a3b8;font-size:13px"></p>
<p id="conn-ip" style="color:#6ee7b7;font-weight:600;font-size:14px;margin-top:4px"></p>
<div class="status warn" style="margin-top:16px">&#9888; This device needs admin approval before alerts will work. Ask the admin to approve it in the Admin Panel.</div>
<p style="color:#94a3b8;font-size:12px;margin-top:16px">The call button is now active. Press it to send alerts to the master.</p>
</div>
</div>
</div>
</div>

<script>
var selectedSSID='';
function loadId(){fetch('/api/status').then(function(r){return r.json()}).then(function(d){document.getElementById('dev-id').value=d.id;document.getElementById('chip-id').textContent='Chip: '+d.id;}).catch(function(){});}
function scanWifi(){document.getElementById('scan-area').innerHTML='<div class="scanning"><div class="spinner"></div><br>Scanning nearby networks...</div>';fetch('/api/scan').then(function(r){return r.json()}).then(function(nets){if(nets.length===0){document.getElementById('scan-area').innerHTML='<div class="scanning">No networks found. Try again.</div>';return;}var html='<div class="wifi-list">';for(var i=0;i<nets.length;i++){var n=nets[i];var bars=n.rssi>-50?'&#9679;&#9679;&#9679;&#9679;':n.rssi>-65?'&#9679;&#9679;&#9679;&#9675;':n.rssi>-75?'&#9679;&#9679;&#9675;&#9675;':'&#9679;&#9675;&#9675;&#9675;';html+='<div class="wifi-item" onclick="pickWifi(this,\''+n.ssid+'\')"><span class="wifi-name">'+n.ssid+(n.ssid==='HospitalAlarm'?' (Master)':'')+'</span><span class="wifi-signal">'+bars+'</span></div>';}html+='</div>';document.getElementById('scan-area').innerHTML=html;}).catch(function(){document.getElementById('scan-area').innerHTML='<div class="scanning">Scan failed.</div>';});}
function pickWifi(el,ssid){selectedSSID=ssid;document.querySelectorAll('.wifi-item').forEach(function(i){i.classList.remove('selected')});el.classList.add('selected');document.getElementById('connectBtn').disabled=false;}
function doConnect(){var devId=document.getElementById('dev-id').value.trim();var masterIp=document.getElementById('master-ip').value.trim();var pass=document.getElementById('wifi-pass').value;if(!devId){alert('Enter a Device ID');return;}if(!masterIp){alert('Enter Master IP');return;}if(!selectedSSID){alert('Select a network');return;}document.getElementById('connectBtn').disabled=true;document.getElementById('connectBtn').textContent='Connecting...';document.getElementById('status-area').innerHTML='<div class="status info">Connecting to '+selectedSSID+'...</div>';fetch('/api/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:selectedSSID,password:pass,masterIP:'http://'+masterIp,slaveId:devId})}).then(function(r){return r.json()}).then(function(d){if(d.success){document.getElementById('status-area').innerHTML='<div class="status ok">Connected! Registering with master...</div>';setTimeout(function(){fetch('/api/status').then(function(r){return r.json()}).then(function(s){document.querySelectorAll('.step').forEach(function(st){st.classList.remove('active')});document.getElementById('step2').classList.add('active');document.getElementById('d1').classList.remove('active');document.getElementById('d1').classList.add('done');document.getElementById('d2').classList.add('active');document.getElementById('d2').classList.add('done');document.getElementById('d3').classList.add('active');document.getElementById('d3').classList.add('done');document.getElementById('conn-info').textContent='Device: '+devId+' | Network: '+selectedSSID;document.getElementById('conn-ip').textContent='IP: '+s.ip+' | Master: '+masterIp;}).catch(function(){document.getElementById('status-area').innerHTML='<div class="status ok">Connected! Setup complete.</div>';});},2000);}else{document.getElementById('status-area').innerHTML='<div class="status err">Failed: '+(d.message||'Unknown error')+'</div>';document.getElementById('connectBtn').disabled=false;document.getElementById('connectBtn').textContent='Connect & Register';}}).catch(function(){document.getElementById('status-area').innerHTML='<div class="status err">Connection error</div>';document.getElementById('connectBtn').disabled=false;document.getElementById('connectBtn').textContent='Connect & Register';});}
loadId();scanWifi();
</script>
</body>
</html>
)rawliteral";

const char SLAVE_STATUS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Slave Device Status</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:linear-gradient(135deg,#0f172a,#1e293b);color:#e2e8f0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.card{background:rgba(30,41,59,.9);border:1px solid rgba(148,163,184,.15);border-radius:20px;padding:32px;width:100%;max-width:420px;backdrop-filter:blur(20px);box-shadow:0 20px 60px rgba(0,0,0,.4)}
.header{text-align:center;margin-bottom:24px}
.icon{width:64px;height:64px;border-radius:16px;display:inline-flex;align-items:center;justify-content:center;font-size:32px;margin-bottom:12px}
.icon.ok{background:linear-gradient(135deg,#10b981,#059669);box-shadow:0 8px 24px rgba(16,185,129,.3)}
.icon.off{background:rgba(100,116,139,.3)}
.icon.alert{background:linear-gradient(135deg,#ef4444,#dc2626);animation:pulse 2s infinite;box-shadow:0 8px 24px rgba(239,68,68,.3)}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.7}}
h1{font-size:20px;font-weight:700;background:linear-gradient(135deg,#6ee7b7,#34d399);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.info-grid{margin-top:20px;display:flex;flex-direction:column;gap:10px}
.info-row{display:flex;justify-content:space-between;align-items:center;padding:10px 14px;background:rgba(15,23,42,.4);border-radius:10px;border:1px solid rgba(148,163,184,.08)}
.info-label{font-size:13px;color:#94a3b8}
.info-value{font-size:14px;font-weight:600;color:#e2e8f0;font-family:monospace}
.status-badge{display:inline-flex;align-items:center;gap:6px;padding:4px 12px;border-radius:20px;font-size:12px;font-weight:600}
.status-badge.online{background:rgba(34,197,94,.15);color:#4ade80}
.status-badge.offline{background:rgba(239,68,68,.15);color:#f87171}
.status-badge.pending{background:rgba(251,191,36,.15);color:#fbbf24}
.status-badge.alerting{background:rgba(239,68,68,.2);color:#f87171;animation:pulse 2s infinite}
.dot{width:8px;height:8px;border-radius:50%;display:inline-block}
.dot.on{background:#22c55e;box-shadow:0 0 6px rgba(34,197,94,.5)}
.dot.off{background:#ef4444}
.dot.warn{background:#fbbf24}
.btn-alert{width:100%;padding:16px;border:none;border-radius:12px;font-size:16px;font-weight:700;cursor:pointer;margin-top:20px;transition:all .3s;background:linear-gradient(135deg,#ef4444,#dc2626);color:#fff;box-shadow:0 8px 24px rgba(239,68,68,.3)}
.btn-alert:hover{transform:translateY(-2px);box-shadow:0 12px 32px rgba(239,68,68,.4)}
.btn-alert:active{transform:scale(.97)}
.btn-alert:disabled{opacity:.5;cursor:not-allowed;transform:none}
.footer{text-align:center;margin-top:16px;font-size:11px;color:#64748b}
</style>
</head>
<body>
<div class="card">
<div class="header">
<div class="icon" id="main-icon">&#128276;</div>
<h1>Slave Device</h1>
</div>
<div class="info-grid" id="info"></div>
<button class="btn-alert" id="alertBtn" onclick="sendAlert()">&#128680; Send Alert</button>
<div id="alert-status"></div>
<div class="footer">Auto-refreshes every 5s</div>
</div>
<script>
function load(){fetch('/api/status').then(function(r){return r.json()}).then(function(d){var icon=document.getElementById('main-icon');if(d.alertPending){icon.className='icon alert';icon.innerHTML='&#128680;';}else if(d.connected){icon.className='icon ok';icon.innerHTML='&#128276;';}else{icon.className='icon off';icon.innerHTML='&#128263;';}var connClass=d.connected?'online':'offline';var connText=d.connected?'Connected':'Disconnected';var regClass=d.registered?(d.approved?'online':'pending'):'offline';var regText=d.registered?(d.approved?'Approved':'Pending Approval'):'Not Registered';document.getElementById('info').innerHTML='<div class="info-row"><span class="info-label">Device ID</span><span class="info-value">'+d.id+'</span></div>'+'<div class="info-row"><span class="info-label">Network</span><span class="info-value">'+(d.ssid||'None')+'</span></div>'+'<div class="info-row"><span class="info-label">IP Address</span><span class="info-value">'+(d.ip||'-')+'</span></div>'+'<div class="info-row"><span class="info-label">Master</span><span class="info-value">'+d.master+'</span></div>'+'<div class="info-row"><span class="info-label">Wi-Fi</span><span class="status-badge '+connClass+'"><span class="dot '+(d.connected?'on':'off')+'"></span>'+connText+'</span></div>'+'<div class="info-row"><span class="info-label">Registration</span><span class="status-badge '+regClass+'"><span class="dot '+(d.registered?(d.approved?'on':'warn'):'off')+'"></span>'+regText+'</span></div>';document.getElementById('alertBtn').disabled=!d.connected;}).catch(function(){});}
function sendAlert(){document.getElementById('alertBtn').disabled=true;document.getElementById('alertBtn').textContent='Sending...';fetch('/api/trigger-alert',{method:'POST'}).then(function(r){return r.json()}).then(function(d){if(d.success){document.getElementById('alert-status').innerHTML='<div style="padding:10px;border-radius:8px;margin-top:10px;background:rgba(34,197,94,.1);color:#4ade80;text-align:center;font-size:13px">Alert sent to master!</div>';}else{document.getElementById('alert-status').innerHTML='<div style="padding:10px;border-radius:8px;margin-top:10px;background:rgba(239,68,68,.1);color:#f87171;text-align:center;font-size:13px">Failed: '+(d.message||'Error')+'</div>';}document.getElementById('alertBtn').textContent='&#128680; Send Alert';document.getElementById('alertBtn').disabled=false;setTimeout(function(){document.getElementById('alert-status').innerHTML='';},4000);}).catch(function(){document.getElementById('alertBtn').textContent='&#128680; Send Alert';document.getElementById('alertBtn').disabled=false;});}
load();setInterval(load,5000);
</script>
</div>
</body>
</html>
)rawliteral";

void setupSlaveRoutes() {
  setupServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (setupDone) {
      request->send_P(200, "text/html", SLAVE_STATUS_HTML);
    } else {
      request->send_P(200, "text/html", SLAVE_SETUP_HTML);
    }
  });

  setupServer.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
      WiFi.scanNetworks(true);
      request->send(200, "application/json", "[]");
      return;
    }
    if (n == WIFI_SCAN_RUNNING) {
      request->send(200, "application/json", "[]");
      return;
    }
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < n && i < 20; i++) {
      JsonObject obj = arr.add<JsonObject>();
      obj["ssid"] = WiFi.SSID(i);
      obj["rssi"] = WiFi.RSSI(i);
      obj["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  setupServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["id"] = slaveId;
    doc["ssid"] = targetSSID;
    doc["ip"] = WiFi.localIP().toString();
    doc["master"] = masterURL;
    doc["connected"] = (WiFi.status() == WL_CONNECTED);
    doc["registered"] = isRegistered;
    doc["approved"] = isApproved;
    doc["setup"] = setupDone;
    doc["alertPending"] = alertPending;
    doc["uptime"] = millis() / 1000;
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  setupServer.on("/api/trigger-alert", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!setupDone || WiFi.status() != WL_CONNECTED) {
      request->send(200, "application/json", "{\"success\":false,\"message\":\"Not connected\"}");
      return;
    }
    buttonPressed = true;
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Alert queued\"}");
  });

  AsyncCallbackJsonWebHandler* connectHandler = new AsyncCallbackJsonWebHandler("/api/connect",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* ssid = obj["ssid"] | "";
      const char* pass = obj["password"] | "";
      const char* master = obj["masterIP"] | "http://192.168.4.1";
      const char* devId = obj["slaveId"] | "";

      if (strlen(ssid) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing SSID\"}");
        return;
      }

      strncpy(targetSSID, ssid, sizeof(targetSSID));
      strncpy(targetPass, pass, sizeof(targetPass));
      strncpy(masterURL, master, sizeof(masterURL));
      if (strlen(devId) > 0) strncpy(slaveId, devId, sizeof(slaveId));

      WiFi.mode(WIFI_AP_STA);
      if (strlen(pass) > 0) {
        WiFi.begin(targetSSID, targetPass);
      } else {
        WiFi.begin(targetSSID);
      }

      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        attempts++;
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      }
      digitalWrite(LED_PIN, LOW);

      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        setupDone = true;
        Serial.print("Connected to: ");
        Serial.println(targetSSID);
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        String resp = "{\"success\":true,\"ip\":\"" + WiFi.localIP().toString() + "\"}";
        request->send(200, "application/json", resp);
      } else {
        WiFi.disconnect();
        WiFi.mode(WIFI_AP);
        request->send(200, "application/json", "{\"success\":false,\"message\":\"Could not connect to network\"}");
      }
    }
  );
  setupServer.addHandler(connectHandler);
}

bool registerWithMaster() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = String(masterURL) + "/api/register";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);

  JsonDocument doc;
  doc["slaveId"] = slaveId;
  String body;
  serializeJson(doc, body);

  Serial.print("Registering: ");
  Serial.println(slaveId);

  int httpCode = http.POST(body);
  if (httpCode == 200) {
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
    JsonDocument respDoc;
    deserializeJson(respDoc, response);
    bool success = respDoc["success"] | false;
    if (success) {
      Serial.println("Registered with master!");
      ledBlink(3, 100, 100);
      beep(1, 100, 0);
      http.end();
      return true;
    }
  } else {
    Serial.printf("Register failed, HTTP: %d\n", httpCode);
  }
  http.end();
  return false;
}

void sendAlert() {
  if (WiFi.status() != WL_CONNECTED) {
    ledBlink(5, 50, 50);
    return;
  }

  HTTPClient http;
  String url = String(masterURL) + "/api/alert";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);

  JsonDocument doc;
  doc["slaveId"] = slaveId;
  String body;
  serializeJson(doc, body);

  Serial.println("Sending alert...");
  int httpCode = http.POST(body);
  if (httpCode == 200) {
    String response = http.getString();
    Serial.println(response);
    JsonDocument respDoc;
    deserializeJson(respDoc, response);
    bool success = respDoc["success"] | false;
    if (success) {
      Serial.println("Alert sent!");
      alertPending = true;
      alertSentTime = millis();
      digitalWrite(LED_PIN, HIGH);
      beep(2, 150, 100);
    } else {
      String reason = respDoc["reason"] | "unknown";
      Serial.print("Alert rejected: ");
      Serial.println(reason);
      if (reason == "alert already active") {
        ledBlink(2, 300, 200);
      } else if (reason == "not approved") {
        ledBlink(4, 100, 100);
      } else {
        ledBlink(2, 200, 200);
      }
    }
  } else {
    Serial.printf("Alert HTTP error: %d\n", httpCode);
    ledBlink(5, 50, 50);
  }
  http.end();
}

void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_MS) {
    buttonPressed = true;
    lastButtonPress = now;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Hospital Alarm - Slave Device ===");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(EXT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  generateSlaveId();
  Serial.print("Device ID: ");
  Serial.println(slaveId);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(EXT_BUTTON_PIN), buttonISR, FALLING);

  String apName = getApName();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  delay(100);
  WiFi.softAP(apName.c_str());
  delay(500);

  Serial.print("Setup AP: ");
  Serial.println(apName);
  Serial.println("Open http://192.168.4.1 to configure");

  WiFi.scanNetworks(true);

  setupSlaveRoutes();

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  setupServer.begin();

  Serial.println("Setup server started");
  ledBlink(2, 200, 200);
}

void loop() {
  // Wi-Fi reconnection
  if (setupDone && WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    if (millis() - lastReconnect > RECONNECT_INTERVAL_MS) {
      Serial.println("Reconnecting to Wi-Fi...");
      if (strlen(targetPass) > 0) WiFi.begin(targetSSID, targetPass);
      else WiFi.begin(targetSSID);
      lastReconnect = millis();
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(500);
        attempts++;
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      }
      digitalWrite(LED_PIN, LOW);
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("Reconnected!");
        ledBlink(2, 100, 100);
      }
    }
  }

  // Registration retry
  if (setupDone && !isRegistered && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastRegisterAttempt > REGISTER_INTERVAL_MS) {
      isRegistered = registerWithMaster();
      lastRegisterAttempt = millis();
    }
  }

  // Heartbeat re-registration (keep-alive)
  if (setupDone && isRegistered && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS) {
      registerWithMaster();
      lastHeartbeat = millis();
    }
  }

  // Alert LED timeout (turn off LED after 30s)
  if (alertPending && millis() - alertSentTime > 30000) {
    alertPending = false;
    digitalWrite(LED_PIN, LOW);
  }

  // Button press handler
  if (buttonPressed) {
    buttonPressed = false;
    Serial.println("Button pressed!");
    if (setupDone && WiFi.status() == WL_CONNECTED) {
      if (!isRegistered) {
        isRegistered = registerWithMaster();
      }
      sendAlert();
    } else if (!setupDone) {
      Serial.println("Setup not complete");
      ledBlink(3, 100, 100);
    } else {
      Serial.println("Wi-Fi not connected");
      ledBlink(5, 50, 50);
    }
  }

  delay(10);
}
