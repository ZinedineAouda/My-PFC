#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// --- ONLINE MODE SETTINGS ------------------------
// Pre-configure these before flashing the ESP32!
const String serverURL = "https://my-pfc-production.up.railway.app/";
const String deviceKey = "esp32";
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 10000;
// -------------------------------------------------

char apSSID[64] = "HospitalAlarm";
char apPass[64] = "";
const char* ADMIN_USER = "admin";
const char* ADMIN_PASS = "admin1234";

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

AsyncWebServer server(80);
WiFiUDP beacon;
#define BEACON_PORT 5555
#define BEACON_INTERVAL 3000
unsigned long lastBeacon = 0;

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

const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Setup - Hospital Alarm</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:linear-gradient(135deg,#0f172a 0%,#1e293b 50%,#0f172a 100%);color:#e2e8f0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.container{width:100%;max-width:520px}
.card{background:rgba(30,41,59,.9);border:1px solid rgba(148,163,184,.15);border-radius:20px;padding:36px;backdrop-filter:blur(20px);box-shadow:0 20px 60px rgba(0,0,0,.4)}
.logo{text-align:center;margin-bottom:28px}
.logo-icon{width:72px;height:72px;background:linear-gradient(135deg,#3b82f6,#8b5cf6);border-radius:18px;display:inline-flex;align-items:center;justify-content:center;font-size:36px;margin-bottom:12px;box-shadow:0 8px 24px rgba(59,130,246,.3)}
.logo h1{font-size:24px;font-weight:700;background:linear-gradient(135deg,#60a5fa,#a78bfa);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.logo p{color:#94a3b8;font-size:14px;margin-top:4px}
.step{display:none}.step.active{display:block}
.modes{display:flex;flex-direction:column;gap:12px;margin-top:20px}
.mode-card.online-card:hover{border-color:rgba(16,185,129,.4)}
.mode-card.online-card.selected{border-color:#10b981;background:rgba(16,185,129,.1)}
.online-badge{display:inline-block;padding:2px 7px;background:rgba(16,185,129,.2);color:#34d399;border-radius:8px;font-size:10px;font-weight:700;margin-left:6px;border:1px solid rgba(16,185,129,.3)}
.mode-card{background:rgba(51,65,85,.5);border:2px solid rgba(148,163,184,.1);border-radius:14px;padding:18px;cursor:pointer;transition:all .3s}
.mode-card:hover{border-color:rgba(59,130,246,.4);transform:translateY(-2px);box-shadow:0 8px 20px rgba(0,0,0,.2)}
.mode-card.selected{border-color:#3b82f6;background:rgba(59,130,246,.1)}
.mode-card h3{font-size:16px;font-weight:600;margin-bottom:4px;display:flex;align-items:center;gap:8px}
.mode-card p{font-size:13px;color:#94a3b8;line-height:1.4}
.btn{width:100%;padding:12px;border:none;border-radius:10px;font-size:15px;font-weight:600;cursor:pointer;transition:all .3s;margin-top:16px}
.btn-primary{background:linear-gradient(135deg,#3b82f6,#2563eb);color:#fff}.btn-primary:hover{transform:translateY(-1px);box-shadow:0 6px 16px rgba(37,99,235,.4)}
.btn-secondary{background:rgba(51,65,85,.5);color:#94a3b8;border:1px solid rgba(148,163,184,.2)}.btn-secondary:hover{background:rgba(51,65,85,.8)}
.btn:disabled{opacity:.5;cursor:not-allowed;transform:none}
.form-group{margin-bottom:14px}
.form-group label{display:block;font-size:13px;font-weight:500;color:#94a3b8;margin-bottom:6px}
.form-group input{width:100%;padding:10px 14px;background:rgba(15,23,42,.6);border:1px solid rgba(148,163,184,.2);border-radius:8px;color:#e2e8f0;font-size:14px;outline:none;transition:border .3s}
.form-group input:focus{border-color:#3b82f6;box-shadow:0 0 0 3px rgba(59,130,246,.15)}
.wifi-list{max-height:200px;overflow-y:auto;margin:12px 0;border-radius:8px;border:1px solid rgba(148,163,184,.1)}
.wifi-item{padding:10px 14px;display:flex;justify-content:space-between;align-items:center;cursor:pointer;transition:background .2s;border-bottom:1px solid rgba(148,163,184,.05)}
.wifi-item:last-child{border-bottom:none}
.wifi-item:hover{background:rgba(59,130,246,.1)}
.wifi-item.selected{background:rgba(59,130,246,.15);border-left:3px solid #3b82f6}
.wifi-name{font-weight:500;font-size:14px}
.wifi-signal{font-size:12px;color:#94a3b8}
.scanning{text-align:center;padding:24px;color:#94a3b8}
.scanning .spinner{display:inline-block;width:24px;height:24px;border:3px solid rgba(148,163,184,.2);border-top-color:#3b82f6;border-radius:50%;animation:spin 1s linear infinite;margin-bottom:8px}
@keyframes spin{to{transform:rotate(360deg)}}
.status-msg{padding:10px 14px;border-radius:8px;font-size:13px;margin-top:12px;text-align:center}
.status-msg.success{background:rgba(34,197,94,.1);color:#4ade80;border:1px solid rgba(34,197,94,.2)}
.status-msg.error{background:rgba(239,68,68,.1);color:#f87171;border:1px solid rgba(239,68,68,.2)}
.status-msg.info{background:rgba(59,130,246,.1);color:#60a5fa;border:1px solid rgba(59,130,246,.2)}
.progress{display:flex;justify-content:center;gap:8px;margin-bottom:24px}
.progress .dot{width:10px;height:10px;border-radius:50%;background:rgba(148,163,184,.3);transition:all .3s}
.progress .dot.active{background:#3b82f6;box-shadow:0 0 8px rgba(59,130,246,.4)}
.progress .dot.done{background:#22c55e}
</style>
</head>
<body>
<div class="container">
<div class="card">
<div class="progress"><div class="dot active" id="d1"></div><div class="dot" id="d2"></div><div class="dot" id="d3"></div></div>
<div class="logo">
<div class="logo-icon">&#9829;</div>
<h1>Hospital Alarm System</h1>
<p>Initial Setup</p>
</div>
<div class="step active" id="step1">
<h2 style="font-size:16px;text-align:center;margin-bottom:4px">Select Network Mode</h2>
<p style="text-align:center;color:#94a3b8;font-size:13px;margin-bottom:16px">Choose how devices will communicate</p>
<div class="modes">
<div class="mode-card" onclick="selectMode(1)" id="mode1">
<h3><span class="mode-icon">&#128225;</span> AP Mode</h3>
<p>Master creates its own Wi-Fi network. Best for small areas.</p>
</div>
<div class="mode-card" onclick="selectMode(2)" id="mode2">
<h3><span class="mode-icon">&#127968;</span> STA Mode</h3>
<p>Connect to hospital Wi-Fi. Best for large areas.</p>
</div>
<div class="mode-card" onclick="selectMode(3)" id="mode3">
<h3><span class="mode-icon">&#128260;</span> AP + STA Mode</h3>
<p>Both modes combined. Hybrid connectivity.</p>
</div>
<div class="mode-card online-card" onclick="selectMode(4)" id="mode4">
<h3><span class="mode-icon">&#127760;</span> Online Mode<span class="online-badge">CLOUD</span></h3>
<p>Forward alerts to a hosted cloud dashboard. Requires Railway server &amp; internet connection.</p>
</div>
</div>
<button class="btn btn-primary" id="nextBtn" onclick="goStep2()" disabled>Continue</button>
</div>
<div class="step" id="stepAP">
<h2 style="font-size:16px;text-align:center;margin-bottom:4px">AP Network Settings</h2>
<p style="text-align:center;color:#94a3b8;font-size:13px;margin-bottom:16px">Configure the Wi-Fi network this master will create</p>
<div class="form-group">
<label>Network Name (SSID)</label>
<input type="text" id="ap-ssid" placeholder="e.g. HospitalAlarm" value="HospitalAlarm">
</div>
<div class="form-group">
<label>Network Password (leave empty for open network)</label>
<input type="password" id="ap-pass" placeholder="Enter password (min 8 chars or empty)">
</div>
<div id="ap-status"></div>
<button class="btn btn-primary" id="apSaveBtn" onclick="saveApSettings()">Save & Apply</button>
<button class="btn btn-secondary" onclick="goBack()">Back</button>
</div>
<div class="step" id="step2">
<h2 style="font-size:16px;text-align:center;margin-bottom:4px">Connect to Wi-Fi</h2>
<p style="text-align:center;color:#94a3b8;font-size:13px;margin-bottom:12px">Select the hospital network</p>
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
<p style="text-align:center;color:#94a3b8;font-size:13px;margin-bottom:16px">Configure the AP network, then connect to hospital Wi-Fi</p>
<div class="form-group">
<label>AP Network Name (SSID)</label>
<input type="text" id="apsta-ssid" placeholder="e.g. HospitalAlarm" value="HospitalAlarm">
</div>
<div class="form-group">
<label>AP Password (leave empty for open)</label>
<input type="password" id="apsta-pass" placeholder="Enter password (min 8 chars or empty)">
</div>
<h3 style="font-size:14px;margin:16px 0 8px;color:#94a3b8">Hospital Wi-Fi Network</h3>
<div id="apsta-scan-area"><div class="scanning"><div class="spinner"></div><br>Scanning...</div></div>
<div class="form-group" style="margin-top:12px">
<label>Hospital Wi-Fi Password</label>
<input type="password" id="apsta-wifi-pass" placeholder="Enter hospital network password">
</div>
<button class="btn btn-primary" id="apstaBtn" onclick="saveApStaSettings()" disabled>Connect & Apply</button>
<button class="btn btn-secondary" onclick="scanApSta()">Rescan</button>
<div id="apsta-status"></div>
</div>
<div class="step" id="stepOnline">
<h2 style="font-size:16px;text-align:center;margin-bottom:4px">&#127760; Online Mode Setup</h2>
<p style="text-align:center;color:#94a3b8;font-size:13px;margin-bottom:16px">Connect your master device to the Internet</p>

<div class="form-group">
<label>AP Network Name (SSID)</label>
<input type="text" id="ol-ap-ssid" placeholder="e.g. HospitalAlarm" value="HospitalAlarm">
</div>
<div class="form-group">
<label>AP Password (leave empty for open)</label>
<input type="password" id="ol-ap-pass" placeholder="Min 8 chars or empty">
</div>

<h3 style="font-size:13px;margin:12px 0 8px;color:#94a3b8">&#128246; Select Internet Wi-Fi</h3>
<div id="ol-scan-area"><div class="scanning"><div class="spinner"></div><br>Scanning...</div></div>
<div class="form-group" style="margin-top:12px">
<label>Wi-Fi Password</label>
<input type="password" id="ol-wifi-pass" placeholder="Enter network password">
</div>
<button class="btn btn-primary" id="ol-connectBtn" onclick="connectOnlineWifi()" disabled>Connect &amp; Finish</button>
<button class="btn btn-secondary" onclick="scanOnlineWifi()">Rescan</button>
<div id="ol-status"></div>
<div id="ol-result" style="display:none;margin-top:16px;padding:16px;background:rgba(16,185,129,.08);border:1px solid rgba(16,185,129,.25);border-radius:12px;text-align:center">
<div style="font-size:32px;margin-bottom:8px">&#9989;</div>
<p style="font-size:13px;color:#94a3b8;margin-bottom:10px">Connected! Your device is now online.</p>
</div>
<button class="btn btn-secondary" style="margin-top:8px" onclick="goBack()">&#8592; Back</button>
</div>
<div class="step" id="step3">
<div style="text-align:center;padding:20px 0">
<div style="font-size:48px;margin-bottom:16px">&#9989;</div>
<h2 style="font-size:20px;margin-bottom:8px">Setup Complete!</h2>
<p style="color:#94a3b8;font-size:14px;margin-bottom:8px" id="setup-info"></p>
<p style="color:#60a5fa;font-size:14px;font-weight:600" id="setup-ip"></p>
<button class="btn btn-primary" onclick="finishSetup()">Open Dashboard</button>
</div>
</div>
</div>
</div>
<script>
var selectedMode=0,selectedSSID='',apstaSSID='';
 const selectMode = (m) => {selectedMode=m;document.querySelectorAll('.mode-card').forEach((c) => {c.classList.remove('selected')});document.getElementById('mode'+m).classList.add('selected');document.getElementById('nextBtn').disabled=false;}
 const showStep = (id) => {document.querySelectorAll('.step').forEach((s) => {s.classList.remove('active')});document.getElementById(id).classList.add('active');}
 const goBack = () => {showStep('step1');}
 const goStep2 = () => {document.getElementById('d1').classList.remove('active');document.getElementById('d1').classList.add('done');document.getElementById('d2').classList.add('active');if(selectedMode===1){showStep('stepAP');return;}if(selectedMode===3){showStep('stepAPSTA');scanApSta();return;}if(selectedMode===4){showStep('stepOnline');scanOnlineWifi();return;}showStep('step2');scanWifi();}
 const saveApSettings = () => {var ssid=document.getElementById('ap-ssid').value.trim();var pass=document.getElementById('ap-pass').value;if(!ssid){document.getElementById('ap-status').innerHTML='<div class="status-msg error">Enter a network name</div>';return;}if(pass.length>0&&pass.length<8){document.getElementById('ap-status').innerHTML='<div class="status-msg error">Password must be at least 8 characters or empty</div>';return;}document.getElementById('apSaveBtn').disabled=true;document.getElementById('apSaveBtn').textContent='Applying...';fetch('/api/setup',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:1,apSSID:ssid,apPass:pass})}).then((r) => {return r.json()}).then((d) => {showStep('step3');document.getElementById('d2').classList.remove('active');document.getElementById('d2').classList.add('done');document.getElementById('d3').classList.add('active');document.getElementById('setup-info').textContent='Mode: AP Mode | SSID: '+ssid;document.getElementById('setup-ip').textContent='AP: 192.168.4.1';}).catch(() => {document.getElementById('ap-status').innerHTML='<div class="status-msg error">Error applying settings</div>';document.getElementById('apSaveBtn').disabled=false;document.getElementById('apSaveBtn').textContent='Save & Apply';});}
 const scanWifi = () => {document.getElementById('scan-area').innerHTML='<div class="scanning"><div class="spinner"></div><br>Scanning...</div>';fetch('/api/scan').then((r) => {return r.json()}).then((nets) => {if(nets.length===0){document.getElementById('scan-area').innerHTML='<div class="scanning">No networks found.</div>';return;}var html='<div class="wifi-list">';nets.forEach((n) => {var bars=n.rssi>-50?'&#9679;&#9679;&#9679;&#9679;':n.rssi>-65?'&#9679;&#9679;&#9679;&#9675;':n.rssi>-75?'&#9679;&#9679;&#9675;&#9675;':'&#9679;&#9675;&#9675;&#9675;';html+='<div class="wifi-item" onclick="pickWifi(this,\''+n.ssid+'\')"><span class="wifi-name">'+n.ssid+'</span><span class="wifi-signal">'+bars+'</span></div>';});html+='</div>';document.getElementById('scan-area').innerHTML=html;}).catch(() => {document.getElementById('scan-area').innerHTML='<div class="scanning">Scan failed.</div>';});}
 const pickWifi = (el,ssid) => {selectedSSID=ssid;document.querySelectorAll('#step2 .wifi-item').forEach((i) => {i.classList.remove('selected')});el.classList.add('selected');document.getElementById('connectBtn').disabled=false;}
 const connectWifi = () => {var pass=document.getElementById('wifi-pass').value;document.getElementById('connectBtn').disabled=true;document.getElementById('connectBtn').textContent='Connecting...';document.getElementById('wifi-status').innerHTML='<div class="status-msg info">Connecting to '+selectedSSID+'...</div>';fetch('/api/connect-wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:selectedSSID,password:pass})}).then((r) => {return r.json()}).then((d) => {if(d.success){document.getElementById('wifi-status').innerHTML='<div class="status-msg success">Connected! IP: '+d.ip+'</div>';setTimeout(() => {applyMode(d.ip)},1500);}else{document.getElementById('wifi-status').innerHTML='<div class="status-msg error">Failed: '+d.message+'</div>';document.getElementById('connectBtn').disabled=false;document.getElementById('connectBtn').textContent='Connect';}}).catch(() => {document.getElementById('wifi-status').innerHTML='<div class="status-msg error">Connection error</div>';document.getElementById('connectBtn').disabled=false;document.getElementById('connectBtn').textContent='Connect';});}
 const applyMode = (staIP) => {fetch('/api/setup',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:selectedMode})}).then((r) => {return r.json()}).then((d) => {showStep('step3');document.getElementById('d2').classList.remove('active');document.getElementById('d2').classList.add('done');document.getElementById('d3').classList.add('active');var modes=['','AP Mode','STA Mode','AP + STA Mode'];document.getElementById('setup-info').textContent='Mode: '+modes[selectedMode];var ipText='AP: 192.168.4.1';if(staIP)ipText+=' | Network: '+staIP;document.getElementById('setup-ip').textContent=ipText;}).catch(() => {});}
 const scanApSta = () => {document.getElementById('apsta-scan-area').innerHTML='<div class="scanning"><div class="spinner"></div><br>Scanning...</div>';fetch('/api/scan').then((r) => {return r.json()}).then((nets) => {if(nets.length===0){document.getElementById('apsta-scan-area').innerHTML='<div class="scanning">No networks found.</div>';return;}var html='<div class="wifi-list">';nets.forEach((n) => {var bars=n.rssi>-50?'&#9679;&#9679;&#9679;&#9679;':n.rssi>-65?'&#9679;&#9679;&#9679;&#9675;':n.rssi>-75?'&#9679;&#9679;&#9675;&#9675;':'&#9679;&#9675;&#9675;&#9675;';html+='<div class="wifi-item" onclick="pickApStaWifi(this,\''+n.ssid+'\')"><span class="wifi-name">'+n.ssid+'</span><span class="wifi-signal">'+bars+'</span></div>';});html+='</div>';document.getElementById('apsta-scan-area').innerHTML=html;}).catch(() => {document.getElementById('apsta-scan-area').innerHTML='<div class="scanning">Scan failed.</div>';});}
 const pickApStaWifi = (el,ssid) => {apstaSSID=ssid;document.querySelectorAll('#stepAPSTA .wifi-item').forEach((i) => {i.classList.remove('selected')});el.classList.add('selected');document.getElementById('apstaBtn').disabled=false;}
var olSSID='';
 const scanOnlineWifi = () => {document.getElementById('ol-scan-area').innerHTML='<div class="scanning"><div class="spinner"></div><br>Scanning...</div>';fetch('/api/scan').then((r) => {return r.json()}).then((nets) => {if(nets.length===0){document.getElementById('ol-scan-area').innerHTML='<div class="scanning">No networks found.</div>';return;}var html='<div class="wifi-list">';nets.forEach((n) => {var bars=n.rssi>-50?'&#9679;&#9679;&#9679;&#9679;':n.rssi>-65?'&#9679;&#9679;&#9679;&#9675;':n.rssi>-75?'&#9679;&#9679;&#9675;&#9675;':'&#9679;&#9675;&#9675;&#9675;';html+='<div class="wifi-item" onclick="pickOnlineWifi(this,\''+n.ssid+'\')">'+'<span class="wifi-name">'+n.ssid+'</span><span class="wifi-signal">'+bars+'</span></div>';});html+='</div>';document.getElementById('ol-scan-area').innerHTML=html;}).catch(() => {document.getElementById('ol-scan-area').innerHTML='<div class="scanning">Scan failed.</div>';});}
 const pickOnlineWifi = (el,ssid) => {olSSID=ssid;document.querySelectorAll('#stepOnline .wifi-item').forEach((i) => {i.classList.remove('selected')});el.classList.add('selected');document.getElementById('ol-connectBtn').disabled=false;}
 const connectOnlineWifi = () => {var apSSID=document.getElementById('ol-ap-ssid').value.trim();var apPass=document.getElementById('ol-ap-pass').value;var pass=document.getElementById('ol-wifi-pass').value;if(!apSSID){document.getElementById('ol-status').innerHTML='<div class="status-msg error">Enter AP network name</div>';return;}if(apPass.length>0&&apPass.length<8){document.getElementById('ol-status').innerHTML='<div class="status-msg error">AP password must be 8+ chars or empty</div>';return;}if(!olSSID){document.getElementById('ol-status').innerHTML='<div class="status-msg error">Select a Wi-Fi network first</div>';return;}document.getElementById('ol-connectBtn').disabled=true;document.getElementById('ol-connectBtn').textContent='Connecting...';document.getElementById('ol-status').innerHTML='<div class="status-msg info">Connecting to '+olSSID+'...</div>';fetch('/api/connect-wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:olSSID,password:pass})}).then((r) => {return r.json()}).then((d) => {if(d.success){document.getElementById('ol-status').innerHTML='<div class="status-msg success">Connected! IP: '+d.ip+'</div>';setTimeout(() => {fetch('/api/setup',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:4,apSSID:apSSID,apPass:apPass})}).then((r) => {return r.json()}).then(() => {document.getElementById('ol-status').innerHTML='';document.getElementById('ol-result').style.display='block';document.getElementById('ol-connectBtn').textContent='Connected &#10003;';});},1500);}else{document.getElementById('ol-status').innerHTML='<div class="status-msg error">Failed: '+d.message+'</div>';document.getElementById('ol-connectBtn').disabled=false;document.getElementById('ol-connectBtn').textContent='Connect & Finish';}}).catch(() => {document.getElementById('ol-status').innerHTML='<div class="status-msg error">Connection error</div>';document.getElementById('ol-connectBtn').disabled=false;document.getElementById('ol-connectBtn').textContent='Connect & Finish';});}
 const saveApStaSettings = () => {var ssid=document.getElementById('apsta-ssid').value.trim();var pass=document.getElementById('apsta-pass').value;var wifiPass=document.getElementById('apsta-wifi-pass').value;if(!ssid){document.getElementById('apsta-status').innerHTML='<div class="status-msg error">Enter AP network name</div>';return;}if(pass.length>0&&pass.length<8){document.getElementById('apsta-status').innerHTML='<div class="status-msg error">AP password must be 8+ chars or empty</div>';return;}if(!apstaSSID){document.getElementById('apsta-status').innerHTML='<div class="status-msg error">Select a hospital network</div>';return;}document.getElementById('apstaBtn').disabled=true;document.getElementById('apstaBtn').textContent='Connecting...';document.getElementById('apsta-status').innerHTML='<div class="status-msg info">Connecting to '+apstaSSID+'...</div>';fetch('/api/connect-wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:apstaSSID,password:wifiPass})}).then((r) => {return r.json()}).then((d) => {if(d.success){document.getElementById('apsta-status').innerHTML='<div class="status-msg success">Connected! IP: '+d.ip+'</div>';setTimeout(() => {fetch('/api/setup',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:3,apSSID:ssid,apPass:pass})}).then((r) => {return r.json()}).then(() => {showStep('step3');document.getElementById('d2').classList.remove('active');document.getElementById('d2').classList.add('done');document.getElementById('d3').classList.add('active');document.getElementById('setup-info').textContent='Mode: AP+STA | AP: '+ssid;document.getElementById('setup-ip').textContent='AP: 192.168.4.1 | Network: '+d.ip;});},1500);}else{document.getElementById('apsta-status').innerHTML='<div class="status-msg error">Failed: '+(d.message||'Error')+'</div>';document.getElementById('apstaBtn').disabled=false;document.getElementById('apstaBtn').textContent='Connect & Apply';}}).catch(() => {document.getElementById('apsta-status').innerHTML='<div class="status-msg error">Connection error</div>';document.getElementById('apstaBtn').disabled=false;document.getElementById('apstaBtn').textContent='Connect & Apply';});}
 const finishSetup = () => {window.location.href='/';}
</script>
</body>
</html>
)rawliteral";

const char ONLINE_INFO_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Online Mode - Hospital Alarm</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:linear-gradient(135deg,#0f172a,#1e293b);color:#e2e8f0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.card{background:rgba(30,41,59,.9);border:1px solid rgba(16,185,129,.2);border-radius:20px;padding:36px;max-width:500px;width:100%;backdrop-filter:blur(20px);box-shadow:0 20px 60px rgba(0,0,0,.4)}
h1{font-size:22px;font-weight:700;background:linear-gradient(135deg,#34d399,#6ee7b7);-webkit-background-clip:text;-webkit-text-fill-color:transparent;margin-bottom:6px;text-align:center}
.sub{text-align:center;color:#94a3b8;font-size:13px;margin-bottom:24px}
.step-list{list-style:none;padding:0}
.step-list li{display:flex;gap:14px;align-items:flex-start;padding:12px 0;border-bottom:1px solid rgba(148,163,184,.1)}
.step-list li:last-child{border:none}
.step-num{width:28px;height:28px;min-width:28px;background:linear-gradient(135deg,#10b981,#059669);border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:13px;font-weight:700;margin-top:2px}
.step-text{font-size:14px;color:#cbd5e1;line-height:1.5}
.step-text code{background:rgba(15,23,42,.7);padding:2px 6px;border-radius:4px;font-size:12px;color:#34d399}
.back-btn{display:block;text-align:center;margin-top:24px;padding:12px;background:rgba(51,65,85,.5);border:1px solid rgba(148,163,184,.2);border-radius:10px;color:#94a3b8;text-decoration:none;font-size:14px;font-weight:600;transition:all .3s}
.back-btn:hover{background:rgba(51,65,85,.8)}
</style>
</head>
<body>
<div class="card">
<h1>&#127760; Online Mode</h1>
<p class="sub">This firmware is <strong>Offline only</strong>. To use Online Mode, follow these steps:</p>
<ul class="step-list">
<li><div class="step-num">1</div><div class="step-text">In Arduino IDE, open <code>esp32/online/master/master.ino</code></div></li>
<li><div class="step-num">2</div><div class="step-text">Set your server URL at the top of the file: <code>serverURL = "https://your-app.up.railway.app"</code></div></li>
<li><div class="step-num">3</div><div class="step-text">Set your <code>deviceKey</code> from the Railway dashboard settings</div></li>
<li><div class="step-num">4</div><div class="step-text">Flash the Online firmware to this ESP32 master device</div></li>
<li><div class="step-num">5</div><div class="step-text">On first boot connect to <code>HospitalAlarm</code> Wi-Fi &rarr; open <code>http://192.168.4.1</code> to complete setup</div></li>
</ul>
<a href="/setup" class="back-btn">&#8592; Back to Setup</a>
</div>
</body>
</html>
)rawliteral";

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Patient Alarm System</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:#f0f4f8;color:#1a202c;min-height:100vh}
header{background:linear-gradient(135deg,#1e293b,#334155);padding:16px 24px;position:sticky;top:0;z-index:50;box-shadow:0 4px 12px rgba(0,0,0,.1)}
.header-inner{max-width:1200px;margin:0 auto;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:12px}
.logo{display:flex;align-items:center;gap:12px}
.logo-icon{width:42px;height:42px;background:linear-gradient(135deg,#3b82f6,#8b5cf6);border-radius:10px;display:flex;align-items:center;justify-content:center;color:#fff;font-size:22px}
.logo h1{font-size:20px;font-weight:700;color:#f1f5f9;letter-spacing:-0.5px}
.logo p{font-size:12px;color:#94a3b8}
.stats{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
.stat{padding:5px 14px;border-radius:20px;font-size:12px;font-weight:600;background:rgba(255,255,255,.1);color:#cbd5e1}
.stat.alert{background:rgba(239,68,68,.2);color:#fca5a5;animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.6}}
@keyframes fadeIn{from{opacity:0;transform:translateY(12px)}to{opacity:1;transform:translateY(0)}}
@keyframes alertGlow{0%,100%{box-shadow:0 0 0 0 rgba(239,68,68,.3)}50%{box-shadow:0 0 20px 4px rgba(239,68,68,.2)}}
.nav-link{font-size:13px;color:#93c5fd;text-decoration:none;font-weight:500;padding:6px 14px;background:rgba(59,130,246,.15);border-radius:6px}
main{max-width:1200px;margin:0 auto;padding:24px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(290px,1fr));gap:16px}
.card{background:#fff;border-radius:14px;padding:22px;border:1px solid #e2e8f0;transition:all .4s;animation:fadeIn .5s ease;position:relative;overflow:hidden}
.card:hover{transform:translateY(-3px);box-shadow:0 8px 24px rgba(0,0,0,.08)}
.card.alerting{border-color:#ef4444;background:linear-gradient(135deg,#fef2f2,#fff1f2);animation:fadeIn .5s ease,alertGlow 2s infinite}
.card.alerting .alert-bar{position:absolute;top:0;left:0;right:0;height:3px;background:linear-gradient(90deg,#ef4444,#f97316,#ef4444);background-size:200% 100%;animation:shimmer 2s linear infinite}
@keyframes shimmer{0%{background-position:200% 0}100%{background-position:-200% 0}}
.card-top{display:flex;justify-content:space-between;align-items:flex-start;margin-bottom:14px}
.patient-name{font-size:18px;font-weight:700;color:#0f172a}
.patient-id{font-size:12px;color:#94a3b8;font-family:monospace}
.status-icon{width:50px;height:50px;border-radius:14px;display:flex;align-items:center;justify-content:center;font-size:24px;flex-shrink:0}
.status-icon.alert{background:linear-gradient(135deg,#ef4444,#dc2626);color:#fff;animation:pulse 2s infinite}
.status-icon.connected{background:linear-gradient(135deg,#dcfce7,#bbf7d0);color:#16a34a}
.status-icon.offline{background:#f1f5f9;color:#94a3b8}
.alert-badge{display:inline-flex;align-items:center;gap:4px;padding:4px 12px;border-radius:6px;font-size:11px;font-weight:700;text-transform:uppercase;background:#fef2f2;color:#dc2626;margin-bottom:12px;border:1px solid rgba(239,68,68,.2)}
.info-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px}
.info-cell{background:#f8fafc;border-radius:8px;padding:8px 12px}
.info-cell .label{font-size:11px;color:#94a3b8;text-transform:uppercase}
.info-cell .value{font-size:15px;font-weight:600;color:#1e293b;margin-top:2px}
.card-footer{margin-top:14px;padding-top:12px;border-top:1px solid #f1f5f9;display:flex;align-items:center;gap:8px;font-size:12px;color:#94a3b8}
.conn-dot{width:8px;height:8px;border-radius:50%}
.conn-dot.on{background:#22c55e;box-shadow:0 0 6px rgba(34,197,94,.4)}
.conn-dot.off{background:#94a3b8}
.empty{text-align:center;padding:80px 20px;background:#fff;border-radius:16px;border:1px solid #e2e8f0}
.empty h2{font-size:20px;font-weight:600;margin-bottom:8px;color:#475569}
.empty p{color:#94a3b8;font-size:14px}
.toast-container{position:fixed;top:80px;right:20px;z-index:100;display:flex;flex-direction:column;gap:8px}
.toast{background:linear-gradient(135deg,#dc2626,#b91c1c);color:#fff;padding:14px 22px;border-radius:10px;font-size:14px;font-weight:500;box-shadow:0 8px 24px rgba(220,38,38,.3);animation:slideIn .4s}
@keyframes slideIn{from{transform:translateX(120%);opacity:0}to{transform:translateX(0);opacity:1}}
</style>
</head>
<body>
<header>
<div class="header-inner">
<div class="logo">
<div class="logo-icon">&#9829;</div>
<div><h1>Patient Alarm System</h1><p>Real-time Monitoring</p></div>
</div>
<div class="stats" id="stats"></div>
</div>
</header>
<div class="toast-container" id="toasts"></div>
<main><div class="grid" id="grid"></div></main>
<script>
var prevAlerts={};
 const showToast = (msg) => {var c=document.getElementById('toasts');var t=document.createElement('div');t.className='toast';t.textContent=msg;c.appendChild(t);setTimeout(() => {t.remove()},5000);}
 const refresh = () => {fetch('/api/slaves?approved=1').then((r) => {return r.json()}).then((slaves) => {var grid=document.getElementById('grid');var stats=document.getElementById('stats');var alerts=slaves.filter((s) => {return s.alertActive}).length;var conn=slaves.filter((s) => {return s.registered}).length;stats.innerHTML='<span class="stat">'+slaves.length+' Patient'+(slaves.length!==1?'s':'')+'</span><span class="stat">'+conn+' Online</span>'+(alerts>0?'<span class="stat alert">&#9888; '+alerts+' Alert'+(alerts!==1?'s':'')+'</span>':'')+'<a href="/admin" class="nav-link">Admin Panel</a>';slaves.forEach((s) => {if(s.alertActive&&!prevAlerts[s.slaveId])showToast('ALERT: '+s.patientName+' - Bed '+s.bed+', Room '+s.room);});prevAlerts={};slaves.forEach((s) => {if(s.alertActive)prevAlerts[s.slaveId]=true;});slaves.sort((a,b) => {if(a.alertActive&&!b.alertActive)return -1;if(!a.alertActive&&b.alertActive)return 1;return a.slaveId.localeCompare(b.slaveId);});if(slaves.length===0){grid.innerHTML='<div class="empty"><h2>No Patients Yet</h2><p>Approve devices in the Admin Panel.</p></div>';return;}var html='';for(var i=0;i<slaves.length;i++){var s=slaves[i];html+='<div class="card '+(s.alertActive?'alerting':'')+'">';if(s.alertActive)html+='<div class="alert-bar"></div>';html+='<div class="card-top"><div><div class="patient-name">'+(s.patientName||'Unnamed')+'</div><div class="patient-id">'+s.slaveId+'</div></div><div class="status-icon '+(s.alertActive?'alert':s.registered?'connected':'offline')+'">'+(s.alertActive?'&#128680;':s.registered?'&#9829;':'&#128263;')+'</div></div>';if(s.alertActive)html+='<div class="alert-badge">&#9888; Alert Active</div>';html+='<div class="info-grid"><div class="info-cell"><div class="label">Bed</div><div class="value">'+(s.bed||'-')+'</div></div><div class="info-cell"><div class="label">Room</div><div class="value">'+(s.room||'-')+'</div></div></div>';html+='<div class="card-footer"><div class="conn-dot '+(s.registered?'on':'off')+'"></div>'+(s.registered?'Connected':'Offline')+'</div></div>';}grid.innerHTML=html;}).catch((e) => {console.error(e);});}
refresh();setInterval(refresh,2000);
</script>
</body>
</html>
)rawliteral";

const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Admin - Patient Alarm System</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:#f0f4f8;color:#1a202c;min-height:100vh}
header{background:linear-gradient(135deg,#1e293b,#334155);padding:16px 24px;position:sticky;top:0;z-index:50;box-shadow:0 4px 12px rgba(0,0,0,.1)}
.header-inner{max-width:1200px;margin:0 auto;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:12px}
.logo{display:flex;align-items:center;gap:12px}
.logo-icon{width:42px;height:42px;background:linear-gradient(135deg,#f59e0b,#d97706);border-radius:10px;display:flex;align-items:center;justify-content:center;color:#fff;font-size:22px}
.logo h1{font-size:20px;font-weight:700;color:#f1f5f9}
.logo p{font-size:12px;color:#94a3b8}
.nav-links{display:flex;gap:8px}
.nav-link{font-size:13px;color:#93c5fd;text-decoration:none;font-weight:500;padding:6px 14px;background:rgba(59,130,246,.15);border-radius:6px}
main{max-width:1200px;margin:0 auto;padding:24px}
.section{margin-bottom:24px}
.section-title{font-size:16px;font-weight:700;margin-bottom:12px;display:flex;align-items:center;gap:8px;color:#334155}
.section-title .count{background:#e2e8f0;color:#64748b;font-size:12px;padding:2px 8px;border-radius:10px}
.pending-title .count{background:#fef3c7;color:#d97706}
.btn{padding:8px 16px;border:none;border-radius:8px;font-size:13px;font-weight:600;cursor:pointer;transition:all .2s;display:inline-flex;align-items:center;gap:6px}
.btn-primary{background:#2563eb;color:#fff}
.btn-success{background:#16a34a;color:#fff}
.btn-danger{background:#ef4444;color:#fff}
.btn-ghost{background:transparent;color:#64748b;border:1px solid #e2e8f0}
.btn-sm{padding:5px 10px;font-size:12px}
.device-card{background:#fff;border-radius:12px;border:1px solid #e2e8f0;padding:16px;margin-bottom:10px;display:flex;align-items:center;justify-content:space-between;gap:12px;animation:fadeIn .4s ease}
.device-card.pending{border-left:4px solid #f59e0b;background:#fffbeb}
.device-card.alerting{border-left:4px solid #ef4444;background:#fef2f2}
.device-card.approved{border-left:4px solid #22c55e}
@keyframes fadeIn{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:translateY(0)}}
.device-info{flex:1;min-width:0}
.device-id{font-family:monospace;font-size:14px;font-weight:700;color:#1e293b}
.device-meta{font-size:12px;color:#94a3b8;margin-top:2px}
.device-patient{font-size:14px;font-weight:600;color:#334155;margin-top:4px}
.badge{padding:3px 10px;border-radius:20px;font-size:11px;font-weight:600}
.badge-pending{background:#fef3c7;color:#d97706}
.badge-alert{background:#fef2f2;color:#dc2626}
.badge-ok{background:#f0fdf4;color:#16a34a}
.badge-off{background:#f1f5f9;color:#94a3b8}
.actions{display:flex;gap:6px;flex-wrap:wrap;flex-shrink:0}
.modal-overlay{position:fixed;inset:0;background:rgba(0,0,0,.5);z-index:100;display:none;align-items:center;justify-content:center;padding:20px}
.modal-overlay.show{display:flex}
.modal{background:#fff;border-radius:16px;padding:28px;width:100%;max-width:420px;box-shadow:0 20px 60px rgba(0,0,0,.2);animation:modalIn .3s ease}
@keyframes modalIn{from{opacity:0;transform:scale(.95)}to{opacity:1;transform:scale(1)}}
.modal h3{font-size:18px;font-weight:700;margin-bottom:20px;color:#0f172a}
.form-group{margin-bottom:14px}
.form-group label{display:block;font-size:13px;font-weight:500;color:#64748b;margin-bottom:6px}
.form-group input{width:100%;padding:10px 14px;border:1px solid #e2e8f0;border-radius:8px;font-size:14px;outline:none}
.form-group input:focus{border-color:#2563eb;box-shadow:0 0 0 3px rgba(37,99,235,.1)}
.form-group input:disabled{background:#f8fafc;color:#94a3b8}
.modal-actions{display:flex;justify-content:flex-end;gap:8px;margin-top:20px}
.empty{text-align:center;padding:40px 20px;background:#fff;border-radius:12px;border:1px solid #e2e8f0}
.empty p{color:#94a3b8;margin-top:8px;font-size:14px}
.toolbar{display:flex;justify-content:space-between;align-items:center;margin-bottom:16px}
.mode-badge{display:inline-flex;align-items:center;gap:6px;padding:6px 14px;background:#eff6ff;color:#2563eb;border-radius:8px;font-size:13px;font-weight:600}
.login-wrap{display:flex;align-items:center;justify-content:center;min-height:100vh;padding:20px;background:linear-gradient(135deg,#0f172a,#1e293b)}
.login-card{background:#fff;border-radius:16px;padding:32px;width:100%;max-width:380px;box-shadow:0 20px 60px rgba(0,0,0,.2)}
.login-card h2{text-align:center;margin-bottom:4px;font-size:22px;color:#0f172a}
.login-card p{text-align:center;color:#94a3b8;font-size:13px;margin-bottom:24px}
.login-icon{width:56px;height:56px;background:#eff6ff;border-radius:50%;display:flex;align-items:center;justify-content:center;margin:0 auto 16px;font-size:28px;color:#2563eb}
.error-msg{background:#fef2f2;color:#dc2626;padding:8px 12px;border-radius:6px;font-size:13px;margin-bottom:12px;display:none}
@media(max-width:640px){.device-card{flex-direction:column;align-items:flex-start}.actions{width:100%}}
</style>
</head>
<body>
<div id="login-page" class="login-wrap">
<div class="login-card">
<div class="login-icon">&#128274;</div>
<h2>Admin Login</h2>
<p>Hospital Alarm System</p>
<div class="error-msg" id="login-error"></div>
<div class="form-group"><label>Username</label><input type="text" id="username" placeholder="Enter username"></div>
<div class="form-group"><label>Password</label><input type="password" id="password" placeholder="Enter password"></div>
<button class="btn btn-primary" style="width:100%" onclick="doLogin()">Sign In &#8594;</button>
</div>
</div>
<div id="admin-page" style="display:none">
<header>
<div class="header-inner">
<div class="logo">
<div class="logo-icon">&#128737;</div>
<div><h1>Admin Panel</h1><p>Device Management</p></div>
</div>
<div class="nav-links">
<a href="/" class="nav-link">&#9829; Dashboard</a>
<a href="/setup" class="nav-link">&#9881; Setup</a>
<a href="#" class="nav-link" onclick="doLogout()">Logout</a>
</div>
</div>
</header>
<main>
<div class="toolbar"><div id="mode-info"></div></div>
<div class="section">
<div class="section-title pending-title">&#128308; Pending Approval <span class="count" id="pending-count">0</span></div>
<div id="pending-area"></div>
</div>
<div class="section">
<div class="section-title">&#9989; Approved Devices <span class="count" id="approved-count">0</span></div>
<div id="approved-area"></div>
</div>
</main>
</div>
<div class="modal-overlay" id="modal">
<div class="modal">
<h3 id="modal-title">Approve Device</h3>
<div class="form-group"><label>Device ID</label><input type="text" id="m-id" disabled></div>
<div class="form-group"><label>Patient Name</label><input type="text" id="m-name" placeholder="Enter patient name"></div>
<div class="form-group"><label>Bed Number</label><input type="text" id="m-bed" placeholder="e.g. 12A"></div>
<div class="form-group"><label>Room</label><input type="text" id="m-room" placeholder="e.g. 301"></div>
<div class="modal-actions">
<button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
<button class="btn btn-primary" id="modal-submit" onclick="submitModal()">Approve</button>
</div>
</div>
</div>
<script>
var editingId=null,isApproving=false,authToken='';
 const $ = (id) => {return document.getElementById(id)}
 const doLogin = () => {var u=$('username').value,p=$('password').value;fetch('/api/admin/login',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({username:u,password:p})}).then((r) => {if(!r.ok){$('login-error').style.display='block';$('login-error').textContent='Invalid credentials';return;}authToken=btoa(u+':'+p);$('login-page').style.display='none';$('admin-page').style.display='block';loadDevices();}).catch(() => {$('login-error').style.display='block';$('login-error').textContent='Connection error';});}
 const doLogout = () => {authToken='';$('admin-page').style.display='none';$('login-page').style.display='flex';$('username').value='';$('password').value='';}
 const authHeaders = () => {return{'Content-Type':'application/json','Authorization':'Basic '+authToken}}
 const loadDevices = () => {fetch('/api/slaves?all=1').then((r) => {return r.json()}).then((devs) => {var pending=devs.filter((d) => {return !d.approved});var approved=devs.filter((d) => {return d.approved});$('pending-count').textContent=pending.length;$('approved-count').textContent=approved.length;if(pending.length===0){$('pending-area').innerHTML='<div class="empty"><p>No pending devices. Waiting for slaves to connect...</p></div>';}else{var html='';for(var i=0;i<pending.length;i++){var d=pending[i];html+='<div class="device-card pending"><div class="device-info"><div class="device-id">'+d.slaveId+'</div><div class="device-meta">Detected - Awaiting approval</div></div><div class="actions"><span class="badge badge-pending">Pending</span><button class="btn btn-success btn-sm" onclick="openApproveModal(\''+d.slaveId+'\')">Approve</button><button class="btn btn-ghost btn-sm" onclick="rejectDevice(\''+d.slaveId+'\')">Reject</button></div></div>';}$('pending-area').innerHTML=html;}if(approved.length===0){$('approved-area').innerHTML='<div class="empty"><p>No approved devices yet.</p></div>';}else{var html2='';for(var j=0;j<approved.length;j++){var d2=approved[j];var st=d2.alertActive?'badge-alert':d2.registered?'badge-ok':'badge-off';var stT=d2.alertActive?'Alert':d2.registered?'Online':'Offline';html2+='<div class="device-card '+(d2.alertActive?'alerting':'approved')+'"><div class="device-info"><div class="device-id">'+d2.slaveId+'</div><div class="device-patient">'+(d2.patientName||'No name')+' - Bed '+(d2.bed||'-')+', Room '+(d2.room||'-')+'</div></div><div class="actions"><span class="badge '+st+'">'+stT+'</span>'+(d2.alertActive?'<button class="btn btn-danger btn-sm" onclick="clearAlert(\''+d2.slaveId+'\')">Clear</button>':'')+'<button class="btn btn-ghost btn-sm" onclick="openEditModal(\''+d2.slaveId+'\',\''+(d2.patientName||'')+'\',\''+(d2.bed||'')+'\',\''+(d2.room||'')+'\')">Edit</button><button class="btn btn-ghost btn-sm" onclick="deleteDevice(\''+d2.slaveId+'\')">Del</button></div></div>';}$('approved-area').innerHTML=html2;}fetch('/api/status').then((r) => {return r.json()}).then((ms) => {var modes={'1':'AP Mode','2':'STA Mode','3':'AP+STA Mode'};$('mode-info').innerHTML='<span class="mode-badge">&#128225; '+(modes[ms.mode]||'Not configured')+'</span>';}).catch(() => {});}).catch((e) => {console.error(e);});}
 const openApproveModal = (id) => {editingId=id;isApproving=true;$('modal-title').textContent='Approve: '+id;$('modal-submit').textContent='Approve & Save';$('m-id').value=id;$('m-name').value='';$('m-bed').value='';$('m-room').value='';$('modal').classList.add('show');}
 const openEditModal = (id,name,bed,room) => {editingId=id;isApproving=false;$('modal-title').textContent='Edit: '+id;$('modal-submit').textContent='Save';$('m-id').value=id;$('m-name').value=name;$('m-bed').value=bed;$('m-room').value=room;$('modal').classList.add('show');}
 const closeModal = () => {$('modal').classList.remove('show');}
 const submitModal = () => {var name=$('m-name').value.trim(),bed=$('m-bed').value.trim(),room=$('m-room').value.trim();if(!name||!bed||!room){alert('Fill all fields');return;}if(isApproving){fetch('/api/approve/'+editingId,{method:'POST',headers:authHeaders(),body:JSON.stringify({patientName:name,bed:bed,room:room})}).then(() => {closeModal();loadDevices();});}else{fetch('/api/slaves/'+editingId,{method:'PUT',headers:authHeaders(),body:JSON.stringify({patientName:name,bed:bed,room:room})}).then(() => {closeModal();loadDevices();});}}
 const clearAlert = (id) => {fetch('/api/clearAlert/'+id,{method:'POST',headers:authHeaders()}).then(() => {loadDevices();});}
 const deleteDevice = (id) => {if(!confirm('Delete '+id+'?'))return;fetch('/api/slaves/'+id,{method:'DELETE',headers:authHeaders()}).then(() => {loadDevices();});}
 const rejectDevice = (id) => {if(!confirm('Reject '+id+'?'))return;fetch('/api/slaves/'+id,{method:'DELETE',headers:authHeaders()}).then(() => {loadDevices();});}
$('password').addEventListener('keypress',(e) => {if(e.key==='Enter')doLogin()});
setInterval(() => {if($('admin-page').style.display!=='none')loadDevices()},3000);
</script>
</body>
</html>
)rawliteral";

void forwardToCloud(String endpoint, String payload) {
  if (wifiMode != 4 || WiFi.status() != WL_CONNECTED) return;
  
  String url = serverURL;
  if (url.endsWith("/") && endpoint.startsWith("/")) {
    url = url.substring(0, url.length() - 1) + endpoint;
  } else if (!url.endsWith("/") && !endpoint.startsWith("/")) {
    url = url + "/" + endpoint;
  } else {
    url = url + endpoint;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Disable SSL certificate verification

  HTTPClient http;
  http.setReuse(false); // Disable keep-alive to avoid connection refused on reused sockets
  http.setTimeout(10000); // Set 10s timeout to prevent hanging connections
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-device-key", deviceKey);
  http.addHeader("Connection", "close"); // Force close HTTP connection from server side
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    Serial.printf("[Cloud] %s -> HTTP %d\n", endpoint.c_str(), httpCode);
  } else {
    Serial.printf("[Cloud] %s -> Error: %s\n", endpoint.c_str(), http.errorToString(httpCode).c_str());
  }
  http.end();
  client.stop(); // Force close the client connection after use
}

void setupRoutes() {
  server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", SETUP_HTML);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!setupDone) {
      request->send_P(200, "text/html", SETUP_HTML);
      return;
    }
    request->send_P(200, "text/html", DASHBOARD_HTML);
  });

  server.on("/online-setup", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", ONLINE_INFO_HTML);
  });

  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", ADMIN_HTML);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"mode\":" + String(wifiMode) + ",\"setup\":" + (setupDone ? "true" : "false") + ",\"masterIP\":\"" + masterIP + "\",\"slaves\":" + String(slaveCount) + "}";
    request->send(200, "application/json", json);
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
      wifiMode = obj["mode"] | 1;
      const char* newSSID = obj["apSSID"] | "";
      const char* newPass = obj["apPass"] | "";
      if (strlen(newSSID) > 0) {
        strncpy(apSSID, newSSID, sizeof(apSSID) - 1);
        apSSID[sizeof(apSSID) - 1] = '\0';
      }
      strncpy(apPass, newPass, sizeof(apPass) - 1);
      apPass[sizeof(apPass) - 1] = '\0';
      setupDone = true;
      if (wifiMode == 1) {
        WiFi.disconnect();
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
        delay(100);
        if (strlen(apPass) >= 8) {
          WiFi.softAP(apSSID, apPass, 1, 0, 8);
        } else {
          WiFi.softAP(apSSID, NULL, 1, 0, 8);
        }
        masterIP = "192.168.4.1";
      } else if (wifiMode == 2) {
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
      } else if (wifiMode == 4) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
        WiFi.softAP(apSSID, apPass, 1, 0, 8);
        Serial.println("[MODE 4] Online Mode Active. AP+STA configured.");
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
        if (wifiMode == 4) forwardToCloud("/api/register", "{\"slaveId\":\"" + String(slaveId) + "\"}");
        return;
      }
      int slot = findFreeSlot();
      if (slot < 0) {
        request->send(507, "application/json", "{\"success\":false,\"message\":\"Max devices reached\"}");
        return;
      }
      strncpy(slaves[slot].slaveId, slaveId, sizeof(slaves[slot].slaveId));
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
      if (wifiMode == 4) forwardToCloud("/api/register", "{\"slaveId\":\"" + String(slaveId) + "\"}");
    }
  );
  server.addHandler(registerHandler);

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
      Serial.printf("ALERT from %s!\n", slaveId);
      if (wifiMode == 4) forwardToCloud("/api/alert", "{\"slaveId\":\"" + String(slaveId) + "\"}");
    }
  );
  server.addHandler(alertHandler);

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
  Serial.println("\n=== Hospital Patient Alarm System ===");

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
}

void loop() {
  if (wifiMode == 4 && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
      forwardToCloud("/api/master-ping", "{}");
      lastHeartbeat = millis();
    }
  }

  if (setupDone && millis() - lastBeacon > BEACON_INTERVAL) {
    String msg = "HOSPITAL_ALARM:" + masterIP;
    IPAddress broadcastIP;
    if (wifiMode == 1) {
      broadcastIP = IPAddress(192,168,4,255);
    } else {
      broadcastIP = IPAddress(255,255,255,255);
    }
    beacon.beginPacket(broadcastIP, BEACON_PORT);
    beacon.write((uint8_t*)msg.c_str(), msg.length());
    beacon.endPacket();
    lastBeacon = millis();
  }

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
