#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>

char apSSID[64] = "HospitalAlarm";
char apPass[64] = "";
const char* ADMIN_USER = "admin";
const char* ADMIN_PASS = "admin1234";

// ===== ONLINE CONFIG =====
// Set your hosted server URL (Replit app URL) and device API key here
char serverURL[128] = "https://YOUR-APP.replit.app";
char deviceKey[64] = "YOUR_DEVICE_API_KEY";
// ==========================

#define MAX_SLAVES 20
#define BUZZER_PIN 4

struct Slave {
  char slaveId[32];
  char patientName[64];
  char bed[16];
  char room[16];
  bool alertActive;
  bool registered;
  bool approved;
  unsigned long lastAlertTime;
  unsigned long lastSeen;
  bool used;
};

Slave slaves[MAX_SLAVES];
int slaveCount = 0;
int wifiMode = 0;
bool setupDone = false;
char staSSID[64] = "";
char staPass[64] = "";
String masterIP = "192.168.4.1";
bool hasInternet = false;

AsyncWebServer server(80);
WiFiUDP beacon;
#define BEACON_PORT 5555
#define BEACON_INTERVAL 3000
unsigned long lastBeacon = 0;

#define SYNC_INTERVAL 30000
unsigned long lastSync = 0;

WiFiClient httpClient;

bool checkAdminAuth(AsyncWebServerRequest *request) {
  if (!request->hasHeader("Authorization")) return false;
  String authHeader = request->header("Authorization");
  if (!authHeader.startsWith("Basic ")) return false;
  String expected = "YWRtaW46YWRtaW4xMjM0";
  String provided = authHeader.substring(6);
  provided.trim();
  return (provided == expected);
}

void sendUnauthorized(AsyncWebServerRequest *request) {
  request->send(401, "application/json", "{\"message\":\"Unauthorized\"}");
}

int findSlaveIndex(const char* slaveId) {
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (slaves[i].used && strcmp(slaves[i].slaveId, slaveId) == 0) return i;
  }
  return -1;
}

int findFreeSlot() {
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (!slaves[i].used) return i;
  }
  return -1;
}

String getSlavesJson(bool onlyApproved) {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (!slaves[i].used) continue;
    if (onlyApproved && !slaves[i].approved) continue;
    JsonObject obj = arr.add<JsonObject>();
    obj["slaveId"] = slaves[i].slaveId;
    obj["patientName"] = slaves[i].patientName;
    obj["bed"] = slaves[i].bed;
    obj["room"] = slaves[i].room;
    obj["alertActive"] = slaves[i].alertActive;
    obj["registered"] = slaves[i].registered;
    obj["approved"] = slaves[i].approved;
    if (slaves[i].lastAlertTime > 0) obj["lastAlertTime"] = slaves[i].lastAlertTime;
    else obj["lastAlertTime"] = (char*)NULL;
    obj["lastSeen"] = slaves[i].lastSeen;
  }
  String output;
  serializeJson(doc, output);
  return output;
}

// ===== FORWARD TO HOSTED SERVER =====

void forwardRegister(const char* slaveId) {
  if (!hasInternet) return;
  HTTPClient http;
  String url = String(serverURL) + "/api/register";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Key", deviceKey);
  http.setTimeout(5000);
  JsonDocument doc;
  doc["slaveId"] = slaveId;
  String body;
  serializeJson(doc, body);
  int code = http.POST(body);
  if (code == 200) {
    Serial.printf("[ONLINE] Register forwarded: %s\n", slaveId);
  } else {
    Serial.printf("[ONLINE] Register forward failed: %d\n", code);
  }
  http.end();
}

void forwardAlert(const char* slaveId) {
  if (!hasInternet) return;
  HTTPClient http;
  String url = String(serverURL) + "/api/alert";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Key", deviceKey);
  http.setTimeout(5000);
  JsonDocument doc;
  doc["slaveId"] = slaveId;
  String body;
  serializeJson(doc, body);
  int code = http.POST(body);
  if (code == 200) {
    String resp = http.getString();
    Serial.printf("[ONLINE] Alert forwarded: %s -> %s\n", slaveId, resp.c_str());
  } else {
    Serial.printf("[ONLINE] Alert forward failed: %d\n", code);
  }
  http.end();
}

void forwardHeartbeat(const char* slaveId) {
  if (!hasInternet) return;
  HTTPClient http;
  String url = String(serverURL) + "/api/heartbeat";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Key", deviceKey);
  http.setTimeout(5000);
  JsonDocument doc;
  doc["slaveId"] = slaveId;
  String body;
  serializeJson(doc, body);
  int code = http.POST(body);
  if (code == 200) {
    String resp = http.getString();
    Serial.printf("[ONLINE] Heartbeat: %s -> %s\n", slaveId, resp.c_str());
  } else {
    Serial.printf("[ONLINE] Heartbeat failed: %d\n", code);
  }
  http.end();
}

void forwardClearAlert(const char* slaveId) {
  if (!hasInternet) return;
  HTTPClient http;
  String url = String(serverURL) + "/api/clearAlert/" + String(slaveId);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Key", deviceKey);
  http.setTimeout(5000);
  int code = http.POST("");
  if (code == 200) {
    Serial.printf("[ONLINE] Clear forwarded: %s\n", slaveId);
  } else {
    Serial.printf("[ONLINE] Clear forward failed: %d\n", code);
  }
  http.end();
}

void syncAllSlaves() {
  if (!hasInternet) return;
  Serial.println("[ONLINE] Syncing all slaves...");
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (!slaves[i].used) continue;
    forwardHeartbeat(slaves[i].slaveId);
    delay(100);
  }
}

// ===== SETUP HTML (same as offline, but with server config step) =====

const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Setup - Hospital Alarm (Online)</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:linear-gradient(135deg,#0f172a 0%,#1e293b 50%,#0f172a 100%);color:#e2e8f0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.container{width:100%;max-width:520px}
.card{background:rgba(30,41,59,.9);border:1px solid rgba(148,163,184,.15);border-radius:20px;padding:36px;backdrop-filter:blur(20px);box-shadow:0 20px 60px rgba(0,0,0,.4)}
.logo{text-align:center;margin-bottom:28px}
.logo-icon{width:72px;height:72px;background:linear-gradient(135deg,#10b981,#059669);border-radius:18px;display:inline-flex;align-items:center;justify-content:center;font-size:36px;margin-bottom:12px;box-shadow:0 8px 24px rgba(16,185,129,.3)}
.logo h1{font-size:24px;font-weight:700;background:linear-gradient(135deg,#34d399,#6ee7b7);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.logo p{color:#94a3b8;font-size:14px;margin-top:4px}
.online-badge{display:inline-block;padding:4px 12px;background:rgba(16,185,129,.15);color:#34d399;border-radius:20px;font-size:11px;font-weight:600;margin-top:6px;border:1px solid rgba(16,185,129,.3)}
.step{display:none}.step.active{display:block}
.modes{display:flex;flex-direction:column;gap:12px;margin-top:20px}
.mode-card{background:rgba(51,65,85,.5);border:2px solid rgba(148,163,184,.1);border-radius:14px;padding:18px;cursor:pointer;transition:all .3s}
.mode-card:hover{border-color:rgba(59,130,246,.4);transform:translateY(-2px);box-shadow:0 8px 20px rgba(0,0,0,.2)}
.mode-card.selected{border-color:#3b82f6;background:rgba(59,130,246,.1)}
.mode-card h3{font-size:16px;font-weight:600;margin-bottom:4px;display:flex;align-items:center;gap:8px}
.mode-card p{font-size:13px;color:#94a3b8;line-height:1.4}
.btn{width:100%;padding:12px;border:none;border-radius:10px;font-size:15px;font-weight:600;cursor:pointer;transition:all .3s;margin-top:16px}
.btn-primary{background:linear-gradient(135deg,#10b981,#059669);color:#fff}.btn-primary:hover{transform:translateY(-1px);box-shadow:0 6px 16px rgba(5,150,105,.4)}
.btn-secondary{background:rgba(51,65,85,.5);color:#94a3b8;border:1px solid rgba(148,163,184,.2)}.btn-secondary:hover{background:rgba(51,65,85,.8)}
.btn:disabled{opacity:.5;cursor:not-allowed;transform:none}
.form-group{margin-bottom:14px}
.form-group label{display:block;font-size:13px;font-weight:500;color:#94a3b8;margin-bottom:6px}
.form-group input{width:100%;padding:10px 14px;background:rgba(15,23,42,.6);border:1px solid rgba(148,163,184,.2);border-radius:8px;color:#e2e8f0;font-size:14px;outline:none;transition:border .3s}
.form-group input:focus{border-color:#10b981;box-shadow:0 0 0 3px rgba(16,185,129,.15)}
.wifi-list{max-height:200px;overflow-y:auto;margin:12px 0;border-radius:8px;border:1px solid rgba(148,163,184,.1)}
.wifi-item{padding:10px 14px;display:flex;justify-content:space-between;align-items:center;cursor:pointer;transition:background .2s;border-bottom:1px solid rgba(148,163,184,.05)}
.wifi-item:last-child{border-bottom:none}
.wifi-item:hover{background:rgba(16,185,129,.1)}
.wifi-item.selected{background:rgba(16,185,129,.15);border-left:3px solid #10b981}
.wifi-name{font-weight:500;font-size:14px}
.wifi-signal{font-size:12px;color:#94a3b8}
.scanning{text-align:center;padding:24px;color:#94a3b8}
.scanning .spinner{display:inline-block;width:24px;height:24px;border:3px solid rgba(148,163,184,.2);border-top-color:#10b981;border-radius:50%;animation:spin 1s linear infinite;margin-bottom:8px}
@keyframes spin{to{transform:rotate(360deg)}}
.status-msg{padding:10px 14px;border-radius:8px;font-size:13px;margin-top:12px;text-align:center}
.status-msg.success{background:rgba(34,197,94,.1);color:#4ade80;border:1px solid rgba(34,197,94,.2)}
.status-msg.error{background:rgba(239,68,68,.1);color:#f87171;border:1px solid rgba(239,68,68,.2)}
.status-msg.info{background:rgba(16,185,129,.1);color:#34d399;border:1px solid rgba(16,185,129,.2)}
.progress{display:flex;justify-content:center;gap:8px;margin-bottom:24px}
.progress .dot{width:10px;height:10px;border-radius:50%;background:rgba(148,163,184,.3);transition:all .3s}
.progress .dot.active{background:#10b981;box-shadow:0 0 8px rgba(16,185,129,.4)}
.progress .dot.done{background:#22c55e}
.note{font-size:12px;color:#64748b;margin-top:8px;line-height:1.4;text-align:center}
</style>
</head>
<body>
<div class="container">
<div class="card">
<div class="progress"><div class="dot active" id="d1"></div><div class="dot" id="d2"></div><div class="dot" id="d3"></div><div class="dot" id="d4"></div></div>
<div class="logo">
<div class="logo-icon">&#127760;</div>
<h1>Hospital Alarm System</h1>
<p>Online Mode Setup</p>
<div class="online-badge">&#9679; ONLINE MODE</div>
</div>
<div class="step active" id="stepServer">
<h2 style="font-size:16px;text-align:center;margin-bottom:4px">Server Configuration</h2>
<p style="text-align:center;color:#94a3b8;font-size:13px;margin-bottom:16px">Connect this device to your hosted dashboard</p>
<div class="form-group">
<label>Server URL</label>
<input type="text" id="srv-url" placeholder="https://your-app.replit.app">
</div>
<div class="form-group">
<label>Device API Key</label>
<input type="text" id="srv-key" placeholder="Enter your device API key">
</div>
<div id="srv-status"></div>
<button class="btn btn-primary" onclick="testServer()">Test Connection</button>
<p class="note">Enter the URL of your hosted dashboard and the device API key from your server settings.</p>
</div>
<div class="step" id="step1">
<h2 style="font-size:16px;text-align:center;margin-bottom:4px">Select Network Mode</h2>
<p style="text-align:center;color:#94a3b8;font-size:13px;margin-bottom:16px">Choose how slave devices will communicate with this master</p>
<div class="modes">
<div class="mode-card" onclick="selectMode(2)" id="mode2">
<h3><span class="mode-icon">&#127968;</span> STA Mode</h3>
<p>All devices connect to hospital Wi-Fi. Master forwards data online. <strong>Recommended.</strong></p>
</div>
<div class="mode-card" onclick="selectMode(3)" id="mode3">
<h3><span class="mode-icon">&#128260;</span> AP + STA Mode</h3>
<p>Master creates AP for slaves AND connects to hospital Wi-Fi for online sync.</p>
</div>
</div>
<button class="btn btn-primary" id="nextBtn" onclick="goStep2()" disabled>Continue</button>
<button class="btn btn-secondary" onclick="showStep('stepServer')">Back</button>
</div>
<div class="step" id="step2">
<h2 style="font-size:16px;text-align:center;margin-bottom:4px">Connect to Hospital Wi-Fi</h2>
<p style="text-align:center;color:#94a3b8;font-size:13px;margin-bottom:12px">Master needs internet access to sync with the server</p>
<div id="scan-area"><div class="scanning"><div class="spinner"></div><br>Scanning...</div></div>
<div class="form-group" style="margin-top:12px">
<label>Wi-Fi Password</label>
<input type="password" id="wifi-pass" placeholder="Enter network password">
</div>
<button class="btn btn-primary" id="connectBtn" onclick="connectWifi()" disabled>Connect</button>
<button class="btn btn-secondary" onclick="scanWifi()">Rescan</button>
<div id="wifi-status"></div>
</div>
<div class="step" id="stepAPSTA">
<h2 style="font-size:16px;text-align:center;margin-bottom:4px">AP + STA Settings</h2>
<p style="text-align:center;color:#94a3b8;font-size:13px;margin-bottom:16px">Configure AP for slaves, then connect to hospital Wi-Fi for internet</p>
<div class="form-group">
<label>AP Network Name (SSID)</label>
<input type="text" id="apsta-ssid" placeholder="e.g. HospitalAlarm" value="HospitalAlarm">
</div>
<div class="form-group">
<label>AP Password (leave empty for open)</label>
<input type="password" id="apsta-pass" placeholder="Enter password (min 8 chars or empty)">
</div>
<h3 style="font-size:14px;margin:16px 0 8px;color:#94a3b8">Hospital Wi-Fi (for internet)</h3>
<div id="apsta-scan-area"><div class="scanning"><div class="spinner"></div><br>Scanning...</div></div>
<div class="form-group" style="margin-top:12px">
<label>Hospital Wi-Fi Password</label>
<input type="password" id="apsta-wifi-pass" placeholder="Enter hospital network password">
</div>
<button class="btn btn-primary" id="apstaBtn" onclick="saveApStaSettings()" disabled>Connect & Apply</button>
<button class="btn btn-secondary" onclick="scanApSta()">Rescan</button>
<div id="apsta-status"></div>
</div>
<div class="step" id="step3">
<div style="text-align:center;padding:20px 0">
<div style="font-size:48px;margin-bottom:16px">&#9989;</div>
<h2 style="font-size:20px;margin-bottom:8px">Online Setup Complete!</h2>
<p style="color:#94a3b8;font-size:14px;margin-bottom:8px" id="setup-info"></p>
<p style="color:#34d399;font-size:14px;font-weight:600" id="setup-ip"></p>
<p style="color:#64748b;font-size:13px;margin-top:12px">Data will be forwarded to your hosted dashboard. Slave devices can now connect to this master.</p>
<button class="btn btn-primary" onclick="finishSetup()">Done</button>
</div>
</div>
</div>
</div>
<script>
var selectedMode=0,selectedSSID='',apstaSSID='';
function selectMode(m){selectedMode=m;document.querySelectorAll('.mode-card').forEach(function(c){c.classList.remove('selected')});document.getElementById('mode'+m).classList.add('selected');document.getElementById('nextBtn').disabled=false;}
function showStep(id){document.querySelectorAll('.step').forEach(function(s){s.classList.remove('active')});document.getElementById(id).classList.add('active');}
function testServer(){var url=document.getElementById('srv-url').value.trim();var key=document.getElementById('srv-key').value.trim();if(!url){document.getElementById('srv-status').innerHTML='<div class="status-msg error">Enter server URL</div>';return;}if(!key){document.getElementById('srv-status').innerHTML='<div class="status-msg error">Enter device API key</div>';return;}document.getElementById('srv-status').innerHTML='<div class="status-msg info">Saving server config...</div>';fetch('/api/server-config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({url:url,key:key})}).then(function(r){return r.json()}).then(function(d){if(d.success){document.getElementById('srv-status').innerHTML='<div class="status-msg success">Server config saved!</div>';document.getElementById('d1').classList.remove('active');document.getElementById('d1').classList.add('done');document.getElementById('d2').classList.add('active');setTimeout(function(){showStep('step1');},800);}else{document.getElementById('srv-status').innerHTML='<div class="status-msg error">Error: '+(d.message||'Failed')+'</div>';}}).catch(function(){document.getElementById('srv-status').innerHTML='<div class="status-msg error">Could not save config</div>';});}
function goStep2(){document.getElementById('d2').classList.remove('active');document.getElementById('d2').classList.add('done');document.getElementById('d3').classList.add('active');if(selectedMode===3){showStep('stepAPSTA');scanApSta();return;}showStep('step2');scanWifi();}
function scanWifi(){document.getElementById('scan-area').innerHTML='<div class="scanning"><div class="spinner"></div><br>Scanning...</div>';fetch('/api/scan').then(function(r){return r.json()}).then(function(nets){if(nets.length===0){document.getElementById('scan-area').innerHTML='<div class="scanning">No networks found.</div>';return;}var html='<div class="wifi-list">';nets.forEach(function(n){var bars=n.rssi>-50?'&#9679;&#9679;&#9679;&#9679;':n.rssi>-65?'&#9679;&#9679;&#9679;&#9675;':n.rssi>-75?'&#9679;&#9679;&#9675;&#9675;':'&#9679;&#9675;&#9675;&#9675;';html+='<div class="wifi-item" onclick="pickWifi(this,\''+n.ssid+'\')"><span class="wifi-name">'+n.ssid+'</span><span class="wifi-signal">'+bars+'</span></div>';});html+='</div>';document.getElementById('scan-area').innerHTML=html;}).catch(function(){document.getElementById('scan-area').innerHTML='<div class="scanning">Scan failed.</div>';});}
function pickWifi(el,ssid){selectedSSID=ssid;document.querySelectorAll('#step2 .wifi-item').forEach(function(i){i.classList.remove('selected')});el.classList.add('selected');document.getElementById('connectBtn').disabled=false;}
function connectWifi(){var pass=document.getElementById('wifi-pass').value;document.getElementById('connectBtn').disabled=true;document.getElementById('connectBtn').textContent='Connecting...';document.getElementById('wifi-status').innerHTML='<div class="status-msg info">Connecting to '+selectedSSID+'...</div>';fetch('/api/connect-wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:selectedSSID,password:pass})}).then(function(r){return r.json()}).then(function(d){if(d.success){document.getElementById('wifi-status').innerHTML='<div class="status-msg success">Connected! IP: '+d.ip+'</div>';setTimeout(function(){applyMode(d.ip)},1500);}else{document.getElementById('wifi-status').innerHTML='<div class="status-msg error">Failed: '+d.message+'</div>';document.getElementById('connectBtn').disabled=false;document.getElementById('connectBtn').textContent='Connect';}}).catch(function(){document.getElementById('wifi-status').innerHTML='<div class="status-msg error">Connection error</div>';document.getElementById('connectBtn').disabled=false;document.getElementById('connectBtn').textContent='Connect';});}
function applyMode(staIP){fetch('/api/setup',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:selectedMode})}).then(function(r){return r.json()}).then(function(d){showStep('step3');document.getElementById('d3').classList.remove('active');document.getElementById('d3').classList.add('done');document.getElementById('d4').classList.add('active');var modes=['','AP Mode','STA Mode','AP + STA Mode'];document.getElementById('setup-info').textContent='Mode: '+modes[selectedMode];document.getElementById('setup-ip').textContent='Network IP: '+staIP;}).catch(function(){});}
function scanApSta(){document.getElementById('apsta-scan-area').innerHTML='<div class="scanning"><div class="spinner"></div><br>Scanning...</div>';fetch('/api/scan').then(function(r){return r.json()}).then(function(nets){if(nets.length===0){document.getElementById('apsta-scan-area').innerHTML='<div class="scanning">No networks found.</div>';return;}var html='<div class="wifi-list">';nets.forEach(function(n){var bars=n.rssi>-50?'&#9679;&#9679;&#9679;&#9679;':n.rssi>-65?'&#9679;&#9679;&#9679;&#9675;':n.rssi>-75?'&#9679;&#9679;&#9675;&#9675;':'&#9679;&#9675;&#9675;&#9675;';html+='<div class="wifi-item" onclick="pickApStaWifi(this,\''+n.ssid+'\')"><span class="wifi-name">'+n.ssid+'</span><span class="wifi-signal">'+bars+'</span></div>';});html+='</div>';document.getElementById('apsta-scan-area').innerHTML=html;}).catch(function(){document.getElementById('apsta-scan-area').innerHTML='<div class="scanning">Scan failed.</div>';});}
function pickApStaWifi(el,ssid){apstaSSID=ssid;document.querySelectorAll('#stepAPSTA .wifi-item').forEach(function(i){i.classList.remove('selected')});el.classList.add('selected');document.getElementById('apstaBtn').disabled=false;}
function saveApStaSettings(){var ssid=document.getElementById('apsta-ssid').value.trim();var pass=document.getElementById('apsta-pass').value;var wifiPass=document.getElementById('apsta-wifi-pass').value;if(!ssid){document.getElementById('apsta-status').innerHTML='<div class="status-msg error">Enter AP network name</div>';return;}if(pass.length>0&&pass.length<8){document.getElementById('apsta-status').innerHTML='<div class="status-msg error">AP password must be 8+ chars or empty</div>';return;}if(!apstaSSID){document.getElementById('apsta-status').innerHTML='<div class="status-msg error">Select a hospital network</div>';return;}document.getElementById('apstaBtn').disabled=true;document.getElementById('apstaBtn').textContent='Connecting...';document.getElementById('apsta-status').innerHTML='<div class="status-msg info">Connecting to '+apstaSSID+'...</div>';fetch('/api/connect-wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:apstaSSID,password:wifiPass})}).then(function(r){return r.json()}).then(function(d){if(d.success){document.getElementById('apsta-status').innerHTML='<div class="status-msg success">Connected! IP: '+d.ip+'</div>';setTimeout(function(){fetch('/api/setup',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:3,apSSID:ssid,apPass:pass})}).then(function(r){return r.json()}).then(function(){showStep('step3');document.getElementById('d3').classList.remove('active');document.getElementById('d3').classList.add('done');document.getElementById('d4').classList.add('active');document.getElementById('setup-info').textContent='Mode: AP+STA | AP: '+ssid;document.getElementById('setup-ip').textContent='AP: 192.168.4.1 | Network: '+d.ip;});},1500);}else{document.getElementById('apsta-status').innerHTML='<div class="status-msg error">Failed: '+(d.message||'Error')+'</div>';document.getElementById('apstaBtn').disabled=false;document.getElementById('apstaBtn').textContent='Connect & Apply';}}).catch(function(){document.getElementById('apsta-status').innerHTML='<div class="status-msg error">Connection error</div>';document.getElementById('apstaBtn').disabled=false;document.getElementById('apstaBtn').textContent='Connect & Apply';});}
function finishSetup(){window.location.href='/status';}
</script>
</body>
</html>
)rawliteral";

// ===== SIMPLE STATUS PAGE (since dashboard is online) =====

const char STATUS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Master Status - Hospital Alarm</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:linear-gradient(135deg,#0f172a 0%,#1e293b 50%,#0f172a 100%);color:#e2e8f0;min-height:100vh;padding:20px}
.container{max-width:600px;margin:0 auto}
h1{font-size:22px;text-align:center;margin-bottom:8px;background:linear-gradient(135deg,#34d399,#6ee7b7);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.subtitle{text-align:center;color:#94a3b8;font-size:13px;margin-bottom:24px}
.online-badge{display:inline-block;padding:4px 12px;background:rgba(16,185,129,.15);color:#34d399;border-radius:20px;font-size:11px;font-weight:600;border:1px solid rgba(16,185,129,.3)}
.card{background:rgba(30,41,59,.9);border:1px solid rgba(148,163,184,.15);border-radius:16px;padding:20px;margin-bottom:16px;backdrop-filter:blur(20px)}
.card h2{font-size:16px;margin-bottom:12px;display:flex;align-items:center;gap:8px}
.row{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid rgba(148,163,184,.1)}
.row:last-child{border:none}
.row .label{color:#94a3b8;font-size:13px}
.row .value{font-weight:600;font-size:14px}
.slave-item{padding:10px;background:rgba(51,65,85,.5);border-radius:8px;margin-bottom:8px;display:flex;justify-content:space-between;align-items:center}
.slave-item .id{font-family:monospace;font-size:13px;font-weight:600}
.slave-item .status{font-size:12px;padding:3px 10px;border-radius:12px}
.alert-status{background:rgba(239,68,68,.2);color:#f87171}
.approved-status{background:rgba(34,197,94,.2);color:#4ade80}
.pending-status{background:rgba(245,158,11,.2);color:#fbbf24}
.empty-msg{text-align:center;color:#64748b;font-size:13px;padding:16px}
.refresh-note{text-align:center;color:#64748b;font-size:12px;margin-top:16px}
</style>
</head>
<body>
<div class="container">
<h1>&#127760; Master Device Status</h1>
<p class="subtitle"><span class="online-badge">&#9679; ONLINE MODE</span> Data synced to hosted dashboard</p>
<div class="card">
<h2>&#128225; System Info</h2>
<div id="sys-info"><div class="empty-msg">Loading...</div></div>
</div>
<div class="card">
<h2>&#128268; Connected Slaves</h2>
<div id="slave-list"><div class="empty-msg">Loading...</div></div>
</div>
<p class="refresh-note">Auto-refreshes every 3 seconds</p>
</div>
<script>
function refresh(){
  fetch('/api/status').then(function(r){return r.json()}).then(function(s){
    var modes=['Not set','AP','STA','AP+STA'];
    var html='<div class="row"><span class="label">Mode</span><span class="value">'+modes[s.mode]+'</span></div>';
    html+='<div class="row"><span class="label">Master IP</span><span class="value">'+s.masterIP+'</span></div>';
    html+='<div class="row"><span class="label">Total Slaves</span><span class="value">'+s.slaves+'</span></div>';
    html+='<div class="row"><span class="label">Internet</span><span class="value">'+(s.internet?'&#9989; Connected':'&#10060; No connection')+'</span></div>';
    html+='<div class="row"><span class="label">Server</span><span class="value" style="font-size:12px;word-break:break-all">'+s.serverURL+'</span></div>';
    document.getElementById('sys-info').innerHTML=html;
  });
  fetch('/api/slaves?all=1').then(function(r){return r.json()}).then(function(devs){
    if(devs.length===0){document.getElementById('slave-list').innerHTML='<div class="empty-msg">No slaves connected yet</div>';return;}
    var html='';
    for(var i=0;i<devs.length;i++){
      var d=devs[i];
      var st=d.alertActive?'alert':d.approved?'approved':'pending';
      var stT=d.alertActive?'ALERT':d.approved?'Online':'Pending';
      html+='<div class="slave-item"><div><div class="id">'+d.slaveId+'</div>';
      if(d.patientName)html+='<div style="font-size:12px;color:#94a3b8;margin-top:2px">'+d.patientName+' - Bed '+d.bed+'</div>';
      html+='</div><span class="status '+st+'-status">'+stT+'</span></div>';
    }
    document.getElementById('slave-list').innerHTML=html;
  });
}
refresh();setInterval(refresh,3000);
</script>
</body>
</html>
)rawliteral";

void setupRoutes() {
  server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", SETUP_HTML);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!setupDone) {
      request->send_P(200, "text/html", SETUP_HTML);
      return;
    }
    request->send_P(200, "text/html", STATUS_HTML);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", STATUS_HTML);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["mode"] = wifiMode;
    doc["setup"] = setupDone;
    doc["masterIP"] = masterIP;
    doc["slaves"] = slaveCount;
    doc["internet"] = hasInternet;
    doc["serverURL"] = serverURL;
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
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

  server.on("/api/slaves", HTTP_GET, [](AsyncWebServerRequest *request) {
    bool onlyApproved = request->hasParam("approved");
    bool showAll = request->hasParam("all");
    String json;
    if (showAll) json = getSlavesJson(false);
    else json = getSlavesJson(onlyApproved || !showAll);
    request->send(200, "application/json", json);
  });

  // Server config endpoint for setup wizard
  AsyncCallbackJsonWebHandler* serverConfigHandler = new AsyncCallbackJsonWebHandler("/api/server-config",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* url = obj["url"] | "";
      const char* key = obj["key"] | "";
      if (strlen(url) == 0 || strlen(key) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"URL and key required\"}");
        return;
      }
      strncpy(serverURL, url, sizeof(serverURL) - 1);
      serverURL[sizeof(serverURL) - 1] = '\0';
      strncpy(deviceKey, key, sizeof(deviceKey) - 1);
      deviceKey[sizeof(deviceKey) - 1] = '\0';
      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("Server config: %s\n", serverURL);
    }
  );
  server.addHandler(serverConfigHandler);

  AsyncCallbackJsonWebHandler* connectHandler = new AsyncCallbackJsonWebHandler("/api/connect-wifi",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* ssid = obj["ssid"] | "";
      const char* pass = obj["password"] | "";
      if (strlen(ssid) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing SSID\"}");
        return;
      }
      strncpy(staSSID, ssid, sizeof(staSSID));
      strncpy(staPass, pass, sizeof(staPass));
      WiFi.begin(staSSID, staPass);
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        attempts++;
      }
      if (WiFi.status() == WL_CONNECTED) {
        masterIP = WiFi.localIP().toString();
        hasInternet = true;
        String resp = "{\"success\":true,\"ip\":\"" + masterIP + "\"}";
        request->send(200, "application/json", resp);
        Serial.print("Connected to WiFi. IP: ");
        Serial.println(masterIP);
      } else {
        WiFi.disconnect();
        request->send(200, "application/json", "{\"success\":false,\"message\":\"Could not connect\"}");
      }
    }
  );
  server.addHandler(connectHandler);

  AsyncCallbackJsonWebHandler* setupHandler = new AsyncCallbackJsonWebHandler("/api/setup",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      wifiMode = obj["mode"] | 2;
      const char* newSSID = obj["apSSID"] | "";
      const char* newPass = obj["apPass"] | "";
      if (strlen(newSSID) > 0) {
        strncpy(apSSID, newSSID, sizeof(apSSID) - 1);
        apSSID[sizeof(apSSID) - 1] = '\0';
      }
      strncpy(apPass, newPass, sizeof(apPass) - 1);
      apPass[sizeof(apPass) - 1] = '\0';
      setupDone = true;
      if (wifiMode == 2) {
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
      } else if (wifiMode == 3) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
        delay(100);
        if (strlen(apPass) >= 8) {
          WiFi.softAP(apSSID, apPass, 1, 0, 8);
        } else {
          WiFi.softAP(apSSID, NULL, 1, 0, 8);
        }
      }
      String resp = "{\"success\":true,\"mode\":" + String(wifiMode) + ",\"apSSID\":\"" + String(apSSID) + "\",\"staIP\":\"" + masterIP + "\"}";
      request->send(200, "application/json", resp);
      Serial.printf("Setup complete. Mode: %d, AP SSID: %s\n", wifiMode, apSSID);
    }
  );
  server.addHandler(setupHandler);

  // Slave registration - forward to server
  AsyncCallbackJsonWebHandler* registerHandler = new AsyncCallbackJsonWebHandler("/api/register",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* slaveId = obj["slaveId"] | "";
      if (strlen(slaveId) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing slaveId\"}");
        return;
      }
      int idx = findSlaveIndex(slaveId);
      if (idx >= 0) {
        slaves[idx].registered = true;
        slaves[idx].lastSeen = millis();
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Registered\"}");
        Serial.printf("Slave %s re-registered\n", slaveId);
        forwardRegister(slaveId);
        return;
      }
      int slot = findFreeSlot();
      if (slot < 0) {
        request->send(507, "application/json", "{\"success\":false,\"message\":\"Max devices reached\"}");
        return;
      }
      strncpy(slaves[slot].slaveId, slaveId, sizeof(slaves[slot].slaveId));
      slaves[slot].slaveId[sizeof(slaves[slot].slaveId) - 1] = '\0';
      slaves[slot].patientName[0] = '\0';
      slaves[slot].bed[0] = '\0';
      slaves[slot].room[0] = '\0';
      slaves[slot].alertActive = false;
      slaves[slot].registered = true;
      slaves[slot].approved = false;
      slaves[slot].lastAlertTime = 0;
      slaves[slot].lastSeen = millis();
      slaves[slot].used = true;
      slaveCount++;
      request->send(200, "application/json", "{\"success\":true,\"message\":\"Pending approval\"}");
      Serial.printf("New slave %s detected - pending approval\n", slaveId);
      forwardRegister(slaveId);
    }
  );
  server.addHandler(registerHandler);

  // Alert - forward to server
  AsyncCallbackJsonWebHandler* alertHandler = new AsyncCallbackJsonWebHandler("/api/alert",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* slaveId = obj["slaveId"] | "";
      if (strlen(slaveId) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing slaveId\"}");
        return;
      }
      int idx = findSlaveIndex(slaveId);
      if (idx < 0) {
        request->send(200, "application/json", "{\"success\":false,\"reason\":\"unknown slave\"}");
        return;
      }
      if (!slaves[idx].approved) {
        request->send(200, "application/json", "{\"success\":false,\"reason\":\"not approved\"}");
        return;
      }
      if (slaves[idx].alertActive) {
        request->send(200, "application/json", "{\"success\":false,\"reason\":\"alert already active\"}");
        return;
      }
      slaves[idx].alertActive = true;
      slaves[idx].lastAlertTime = millis();
      slaves[idx].lastSeen = millis();
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("ALERT from %s! Forwarding online...\n", slaveId);
      forwardAlert(slaveId);
    }
  );
  server.addHandler(alertHandler);

  // Admin login (local)
  AsyncCallbackJsonWebHandler* loginHandler = new AsyncCallbackJsonWebHandler("/api/admin/login",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* user = obj["username"] | "";
      const char* pass = obj["password"] | "";
      if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(401, "application/json", "{\"message\":\"Invalid credentials\"}");
      }
    }
  );
  server.addHandler(loginHandler);

  server.on("/api/clearAlert/*", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }
    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    String slaveId = url.substring(lastSlash + 1);
    int idx = findSlaveIndex(slaveId.c_str());
    if (idx < 0) { request->send(404, "application/json", "{\"message\":\"Not found\"}"); return; }
    slaves[idx].alertActive = false;
    request->send(200, "application/json", "{\"success\":true}");
    Serial.printf("Alert cleared: %s\n", slaveId.c_str());
    forwardClearAlert(slaveId.c_str());
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    String url = request->url();
    if (request->method() == HTTP_DELETE && url.startsWith("/api/slaves/")) {
      if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }
      String slaveId = url.substring(12);
      int idx = findSlaveIndex(slaveId.c_str());
      if (idx < 0) { request->send(404, "application/json", "{\"message\":\"Not found\"}"); return; }
      slaves[idx].used = false;
      memset(&slaves[idx], 0, sizeof(Slave));
      slaveCount--;
      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("Deleted: %s\n", slaveId.c_str());
      return;
    }
    if (request->method() == HTTP_POST && url.startsWith("/api/approve/")) {
      request->send(200, "application/json", "{\"success\":true}");
      return;
    }
    if (request->method() == HTTP_OPTIONS) { request->send(200); return; }
    request->send(404, "application/json", "{\"message\":\"Not found\"}");
  });

  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String url = request->url();
    if (request->method() == HTTP_PUT && url.startsWith("/api/slaves/")) {
      if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }
      String slaveId = url.substring(12);
      int idx = findSlaveIndex(slaveId.c_str());
      if (idx < 0) { request->send(404, "application/json", "{\"message\":\"Not found\"}"); return; }
      JsonDocument doc;
      if (deserializeJson(doc, (const char*)data, len)) { request->send(400, "application/json", "{\"message\":\"Bad JSON\"}"); return; }
      if (doc.containsKey("patientName")) strncpy(slaves[idx].patientName, doc["patientName"] | "", sizeof(slaves[idx].patientName));
      if (doc.containsKey("bed")) strncpy(slaves[idx].bed, doc["bed"] | "", sizeof(slaves[idx].bed));
      if (doc.containsKey("room")) strncpy(slaves[idx].room, doc["room"] | "", sizeof(slaves[idx].room));
      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("Updated: %s\n", slaveId.c_str());
    }
    if (request->method() == HTTP_POST && url.startsWith("/api/approve/")) {
      if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }
      String slaveId = url.substring(13);
      int idx = findSlaveIndex(slaveId.c_str());
      if (idx < 0) { request->send(404, "application/json", "{\"message\":\"Not found\"}"); return; }
      JsonDocument doc;
      if (deserializeJson(doc, (const char*)data, len)) { request->send(400, "application/json", "{\"message\":\"Bad JSON\"}"); return; }
      strncpy(slaves[idx].patientName, doc["patientName"] | "", sizeof(slaves[idx].patientName));
      strncpy(slaves[idx].bed, doc["bed"] | "", sizeof(slaves[idx].bed));
      strncpy(slaves[idx].room, doc["room"] | "", sizeof(slaves[idx].room));
      slaves[idx].approved = true;
      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("Approved: %s -> %s\n", slaveId.c_str(), slaves[idx].patientName);
    }
  });
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Hospital Patient Alarm System (ONLINE) ===");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  memset(slaves, 0, sizeof(slaves));

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  delay(100);
  WiFi.softAP(apSSID, NULL, 1, 0, 8);
  delay(500);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  WiFi.scanNetworks(true);

  setupRoutes();

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  server.begin();

  beacon.begin(BEACON_PORT);
  Serial.println("Web server started. Open http://192.168.4.1");
  Serial.println("Mode: ONLINE - data will forward to hosted server");
}

void loop() {
  // Check internet connectivity
  if (WiFi.status() == WL_CONNECTED) {
    hasInternet = true;
  } else {
    hasInternet = false;
  }

  // UDP beacon for slave auto-discovery
  if (setupDone && millis() - lastBeacon > BEACON_INTERVAL) {
    String msg = "HOSPITAL_ALARM:" + masterIP;
    IPAddress broadcastIP;
    if (wifiMode == 3) {
      broadcastIP = IPAddress(192,168,4,255);
      beacon.beginPacket(broadcastIP, BEACON_PORT);
      beacon.write((uint8_t*)msg.c_str(), msg.length());
      beacon.endPacket();
      broadcastIP = IPAddress(255,255,255,255);
    } else {
      broadcastIP = IPAddress(255,255,255,255);
    }
    beacon.beginPacket(broadcastIP, BEACON_PORT);
    beacon.write((uint8_t*)msg.c_str(), msg.length());
    beacon.endPacket();
    lastBeacon = millis();
  }

  // Periodic sync of all slaves to server
  if (setupDone && hasInternet && millis() - lastSync > SYNC_INTERVAL) {
    syncAllSlaves();
    lastSync = millis();
  }

  // Buzzer for active alerts
  bool anyAlert = false;
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (slaves[i].used && slaves[i].alertActive) {
      anyAlert = true;
      break;
    }
  }
  if (anyAlert) {
    static unsigned long lastBeep = 0;
    if (millis() - lastBeep > 5000) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      lastBeep = millis();
    }
  }
  delay(10);
}
