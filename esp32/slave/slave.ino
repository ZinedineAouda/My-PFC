#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#define BUTTON_PIN  0
#define LED_PIN     2
#define DEBOUNCE_MS 300
#define REGISTER_RETRY_MS 10000

char slaveId[32] = "slave-001";
char targetSSID[64] = "";
char targetPass[64] = "";
char masterURL[128] = "http://192.168.4.1";

bool wifiConnected = false;
bool isRegistered = false;
bool setupDone = false;
volatile bool buttonPressed = false;
unsigned long lastButtonPress = 0;
unsigned long lastRegisterAttempt = 0;
unsigned long lastReconnect = 0;

AsyncWebServer setupServer(80);

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
.progress{display:flex;justify-content:center;gap:8px;margin-bottom:20px}
.progress .dot{width:10px;height:10px;border-radius:50%;background:rgba(148,163,184,.3);transition:all .3s}
.progress .dot.active{background:#10b981;box-shadow:0 0 8px rgba(16,185,129,.4)}
.progress .dot.done{background:#22c55e}
.status-page{text-align:center;padding:20px 0}
.status-page .big-icon{font-size:48px;margin-bottom:12px}
.status-page h2{font-size:18px;margin-bottom:6px}
.status-page p{color:#94a3b8;font-size:13px;margin-bottom:4px}
.status-page .ip{color:#6ee7b7;font-weight:600;font-size:14px}
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
<div class="form-group">
<label>Device ID</label>
<input type="text" id="dev-id" placeholder="e.g. slave-001" value="slave-001">
</div>
<div class="form-group">
<label>Master IP Address</label>
<input type="text" id="master-ip" placeholder="e.g. 192.168.4.1" value="192.168.4.1">
</div>
<h3 style="font-size:14px;margin:16px 0 8px;color:#94a3b8">Select Wi-Fi Network</h3>
<div id="scan-area"><div class="scanning"><div class="spinner"></div><br>Scanning...</div></div>
<div class="form-group" style="margin-top:12px">
<label>Wi-Fi Password (leave empty if open)</label>
<input type="password" id="wifi-pass" placeholder="Network password">
</div>
<button class="btn btn-primary" id="connectBtn" onclick="doConnect()" disabled>Connect & Register</button>
<button class="btn btn-secondary" onclick="scanWifi()">Rescan Networks</button>
<div id="status-area"></div>
</div>

<div class="step" id="step2">
<div class="status-page">
<div class="big-icon">&#9989;</div>
<h2>Device Connected!</h2>
<p id="conn-info"></p>
<p class="ip" id="conn-ip"></p>
<p style="color:#94a3b8;font-size:12px;margin-top:16px">The button is now active. Press it to send an alert to the master.</p>
<p style="color:#fbbf24;font-size:12px;margin-top:8px">Note: This device needs to be approved in the Admin Panel before alerts will work.</p>
</div>
</div>
</div>
</div>

<script>
let selectedSSID='';

async function scanWifi(){
  document.getElementById('scan-area').innerHTML='<div class="scanning"><div class="spinner"></div><br>Scanning...</div>';
  try{
    const r=await fetch('/api/scan');
    const nets=await r.json();
    if(nets.length===0){
      document.getElementById('scan-area').innerHTML='<div class="scanning">No networks found. Try again.</div>';
      return;
    }
    let html='<div class="wifi-list">';
    nets.forEach(n=>{
      const bars=n.rssi>-50?'&#9679;&#9679;&#9679;&#9679;':n.rssi>-65?'&#9679;&#9679;&#9679;&#9675;':n.rssi>-75?'&#9679;&#9679;&#9675;&#9675;':'&#9679;&#9675;&#9675;&#9675;';
      html+='<div class="wifi-item" onclick="pickWifi(this,\''+n.ssid+'\')"><span class="wifi-name">'+n.ssid+'</span><span class="wifi-signal">'+bars+'</span></div>';
    });
    html+='</div>';
    document.getElementById('scan-area').innerHTML=html;
  }catch(e){
    document.getElementById('scan-area').innerHTML='<div class="scanning">Scan failed.</div>';
  }
}

function pickWifi(el,ssid){
  selectedSSID=ssid;
  document.querySelectorAll('.wifi-item').forEach(i=>i.classList.remove('selected'));
  el.classList.add('selected');
  document.getElementById('connectBtn').disabled=false;
}

async function doConnect(){
  const devId=document.getElementById('dev-id').value.trim();
  const masterIp=document.getElementById('master-ip').value.trim();
  const pass=document.getElementById('wifi-pass').value;
  if(!devId){alert('Enter a Device ID');return;}
  if(!masterIp){alert('Enter Master IP');return;}
  if(!selectedSSID){alert('Select a network');return;}
  document.getElementById('connectBtn').disabled=true;
  document.getElementById('connectBtn').textContent='Connecting...';
  document.getElementById('status-area').innerHTML='<div class="status info">Connecting to '+selectedSSID+'...</div>';
  try{
    const r=await fetch('/api/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:selectedSSID,password:pass,masterIP:'http://'+masterIp,slaveId:devId})});
    const d=await r.json();
    if(d.success){
      document.getElementById('status-area').innerHTML='<div class="status ok">Connected! Registering with master...</div>';
      setTimeout(async()=>{
        try{
          const r2=await fetch('/api/status');
          const s=await r2.json();
          document.querySelectorAll('.step').forEach(s=>s.classList.remove('active'));
          document.getElementById('step2').classList.add('active');
          document.getElementById('d1').classList.remove('active');
          document.getElementById('d1').classList.add('done');
          document.getElementById('d2').classList.add('active');
          document.getElementById('d2').classList.add('done');
          document.getElementById('d3').classList.add('active');
          document.getElementById('d3').classList.add('done');
          document.getElementById('conn-info').textContent='Device: '+devId+' | Network: '+selectedSSID;
          document.getElementById('conn-ip').textContent='IP: '+s.ip+' | Master: '+masterIp;
        }catch(e){
          document.getElementById('status-area').innerHTML='<div class="status ok">Connected! Setup page will close.</div>';
        }
      },2000);
    }else{
      document.getElementById('status-area').innerHTML='<div class="status err">Failed: '+(d.message||'Unknown error')+'</div>';
      document.getElementById('connectBtn').disabled=false;
      document.getElementById('connectBtn').textContent='Connect & Register';
    }
  }catch(e){
    document.getElementById('status-area').innerHTML='<div class="status err">Connection error</div>';
    document.getElementById('connectBtn').disabled=false;
    document.getElementById('connectBtn').textContent='Connect & Register';
  }
}

scanWifi();
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
<title>Slave Status</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:linear-gradient(135deg,#0f172a,#1e293b);color:#e2e8f0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.card{background:rgba(30,41,59,.9);border:1px solid rgba(148,163,184,.15);border-radius:20px;padding:32px;width:100%;max-width:400px;text-align:center}
.icon{font-size:48px;margin-bottom:12px}
h1{font-size:20px;margin-bottom:16px;background:linear-gradient(135deg,#6ee7b7,#34d399);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.info{font-size:14px;color:#94a3b8;margin-bottom:8px}
.val{color:#e2e8f0;font-weight:600}
.status-dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:6px}
.online{background:#22c55e;box-shadow:0 0 8px rgba(34,197,94,.4)}
.offline{background:#ef4444}
</style>
</head>
<body>
<div class="card">
<div class="icon">&#128276;</div>
<h1>Slave Device</h1>
<div id="info"></div>
<script>
async function load(){
  try{
    const r=await fetch('/api/status');
    const d=await r.json();
    document.getElementById('info').innerHTML=
      '<p class="info">Device ID: <span class="val">'+d.id+'</span></p>'+
      '<p class="info">Network: <span class="val">'+(d.ssid||'Not connected')+'</span></p>'+
      '<p class="info">IP: <span class="val">'+(d.ip||'-')+'</span></p>'+
      '<p class="info">Master: <span class="val">'+d.master+'</span></p>'+
      '<p class="info">Status: <span class="status-dot '+(d.connected?'online':'offline')+'"></span><span class="val">'+(d.registered?'Registered':(d.connected?'Connected':'Offline'))+'</span></p>';
  }catch(e){}
}
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
    doc["setup"] = setupDone;
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  AsyncCallbackJsonWebHandler* connectHandler = new AsyncCallbackJsonWebHandler("/api/connect",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* ssid = obj["ssid"] | "";
      const char* pass = obj["password"] | "";
      const char* master = obj["masterIP"] | "http://192.168.4.1";
      const char* devId = obj["slaveId"] | "slave-001";

      if (strlen(ssid) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing SSID\"}");
        return;
      }

      strncpy(targetSSID, ssid, sizeof(targetSSID));
      strncpy(targetPass, pass, sizeof(targetPass));
      strncpy(masterURL, master, sizeof(masterURL));
      strncpy(slaveId, devId, sizeof(slaveId));

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

  JsonDocument doc;
  doc["slaveId"] = slaveId;
  String body;
  serializeJson(doc, body);

  Serial.print("Registering as: ");
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
      Serial.println("Registered!");
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH); delay(100);
        digitalWrite(LED_PIN, LOW); delay(100);
      }
      http.end();
      return true;
    }
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
  }
  http.end();
  return false;
}

void sendAlert() {
  if (WiFi.status() != WL_CONNECTED) {
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_PIN, HIGH); delay(50);
      digitalWrite(LED_PIN, LOW); delay(50);
    }
    return;
  }

  HTTPClient http;
  String url = String(masterURL) + "/api/alert";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

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
      digitalWrite(LED_PIN, HIGH);
    } else {
      for (int i = 0; i < 2; i++) {
        digitalWrite(LED_PIN, HIGH); delay(200);
        digitalWrite(LED_PIN, LOW); delay(200);
      }
    }
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
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
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  String apName = "SlaveSetup-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFF), HEX);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  delay(100);
  WiFi.softAP(apName.c_str(), NULL, 6, 0, 2);
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
}

void loop() {
  if (setupDone && WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnect > 5000) {
      Serial.println("Reconnecting...");
      if (strlen(targetPass) > 0) WiFi.begin(targetSSID, targetPass);
      else WiFi.begin(targetSSID);
      lastReconnect = millis();
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(500); attempts++;
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      }
      digitalWrite(LED_PIN, LOW);
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("Reconnected!");
      }
    }
  }

  if (setupDone && !isRegistered && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastRegisterAttempt > REGISTER_RETRY_MS) {
      isRegistered = registerWithMaster();
      lastRegisterAttempt = millis();
    }
  }

  if (buttonPressed) {
    buttonPressed = false;
    Serial.println("Button pressed!");
    if (setupDone && WiFi.status() == WL_CONNECTED) {
      if (!isRegistered) {
        isRegistered = registerWithMaster();
      }
      sendAlert();
    } else {
      Serial.println("Not connected yet");
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH); delay(100);
        digitalWrite(LED_PIN, LOW); delay(100);
      }
    }
  }

  delay(10);
}
