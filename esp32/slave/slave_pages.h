#ifndef SLAVE_PAGES_H
#define SLAVE_PAGES_H

#include <Arduino.h>

const char SLAVE_SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Slave Setup - Hospital Alarm</title>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700&display=swap" rel="stylesheet">
<style>
:root { --bg: #0f172a; --card-bg: rgba(30, 41, 59, 0.7); --primary: #10b981; --primary-glow: rgba(16, 185, 129, 0.4); --slate-400: #94a3b8; --slate-200: #e2e8f0; --glass-border: rgba(255, 255, 255, 0.1); }
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Outfit', sans-serif; background:#0f172a; color:var(--slate-200); min-height:100vh; display:flex; align-items:center; justify-content:center; padding:20px; }
.card { background:var(--card-bg); backdrop-filter:blur(16px); border:1px solid var(--glass-border); border-radius:24px; padding:40px; width:100%; max-width:480px; box-shadow:0 20px 50px rgba(0,0,0,0.5); }
.logo { text-align:center; margin-bottom:30px; }
.logo-icon { width:64px; height:64px; background:linear-gradient(135deg, var(--primary), #34d399); border-radius:18px; display:inline-flex; align-items:center; justify-content:center; font-size:32px; margin-bottom:12px; box-shadow:0 8px 20px var(--primary-glow); }
.logo h1 { font-size:22px; font-weight:700; background:linear-gradient(135deg, #6ee7b7, #34d399); -webkit-background-clip:text; -webkit-text-fill-color:transparent; }
.step { display:none; } .step.active { display:block; animation: fadeIn 0.4s ease; }
@keyframes fadeIn { from { opacity:0; transform:translateY(10px); } to { opacity:1; transform:translateY(0); } }
.chip-id { display:inline-block; background:rgba(16, 185, 129, 0.1); color:var(--primary); padding:6px 16px; border-radius:10px; font-family:monospace; font-size:13px; margin-bottom:20px; }
.form-group { margin-bottom:16px; }
.form-group label { display:block; font-size:13px; color:var(--slate-400); margin-bottom:8px; }
.form-group input { width:100%; padding:12px 16px; background:rgba(0,0,0,0.2); border:1px solid var(--glass-border); border-radius:12px; color:white; outline:none; }
.form-group input:focus { border-color:var(--primary); }
.wifi-list { max-height:160px; overflow-y:auto; border-radius:12px; border:1px solid var(--glass-border); margin-bottom:16px; }
.wifi-item { padding:12px 16px; border-bottom:1px solid var(--glass-border); cursor:pointer; display:flex; justify-content:space-between; }
.wifi-item:hover { background:rgba(255,255,255,0.05); }
.wifi-item.selected { background:rgba(16, 185, 129, 0.1); border-left:3px solid var(--primary); }
.btn { width:100%; padding:14px; border:none; border-radius:14px; font-size:15px; font-weight:600; cursor:pointer; transition:all 0.3s; margin-top:10px; background:linear-gradient(135deg, var(--primary), #059669); color:white; }
.btn:disabled { opacity:0.5; cursor:not-allowed; }
.btn-secondary { background:rgba(148,163,184,0.1); color:var(--slate-400); margin-top:8px; border:1px solid var(--glass-border); }
.status { padding:12px; border-radius:12px; text-align:center; font-size:13px; margin-top:12px; }
.status.info { background:rgba(16, 185, 129, 0.05); color:var(--primary); border:1px solid rgba(16, 185, 129, 0.2); }
</style>
</head>
<body>
<div class="card">
  <div class="logo">
    <div class="logo-icon">&#128225;</div>
    <h1>Slave Device Setup</h1>
    <p>Patient Call Station</p>
  </div>
  
  <div id="step1" class="step active">
    <div style="text-align:center"><span class="chip-id" id="chip-id">Loading ID...</span></div>
    <div class="form-group">
      <label>Device ID</label>
      <input type="text" id="dev-id" readonly>
    </div>
    <div id="scan-list" class="wifi-list">
      <div style="text-align:center; padding:20px; color:var(--slate-400);">Scanning...</div>
    </div>
    <div class="form-group">
      <label>Password</label>
      <input type="password" id="wifi-pass" placeholder="Network password">
    </div>
    <div id="setup-status"></div>
    <button class="btn" id="connectBtn" onclick="doConnect()" disabled>Configure & Connect</button>
    <button class="btn btn-secondary" onclick="scanWiFi()">Rescan Networks</button>
  </div>

  <div id="stepFinish" class="step">
    <div style="text-align:center; padding:20px 0;">
      <div style="font-size:48px; margin-bottom:15px;">🚀</div>
      <h2 style="margin-bottom:10px;">Connected!</h2>
      <p style="color:var(--slate-400); font-size:14px;">Master connection pending admin approval.</p>
      <button class="btn" style="margin-top:20px;" onclick="location.href='/'">View Status</button>
    </div>
  </div>
</div>

<script>
let selSSID = '';
function load() {
  fetch('/api/status').then(r => r.json()).then(d => {
    document.getElementById('dev-id').value = d.id;
    document.getElementById('chip-id').textContent = 'Chip ID: ' + d.id;
  });
  scanWiFi();
}
function scanWiFi() {
  const list = document.getElementById('scan-list');
  list.innerHTML = '<div style="text-align:center; padding:20px;">Scanning...</div>';
  fetch('/api/scan').then(r => r.json()).then(nets => {
    let h = '';
    nets.forEach(n => {
      h += `<div class="wifi-item" onclick="pickWiFi(this, '${n.ssid}')">
              <span>${n.ssid}</span>
              <span style="font-size:11px; opacity:0.6;">${n.rssi}dBm</span>
            </div>`;
    });
    list.innerHTML = h;
  });
}
function pickWiFi(el, ssid) {
  selSSID = ssid;
  document.querySelectorAll('.wifi-item').forEach(i => i.classList.remove('selected'));
  el.classList.add('selected');
  document.getElementById('connectBtn').disabled = false;
}
function doConnect() {
  const pass = document.getElementById('wifi-pass').value;
  const status = document.getElementById('setup-status');
  status.innerHTML = '<div class="status info">Connecting...</div>';
  fetch('/api/connect', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ ssid: selSSID, password: pass })
  }).then(r => r.json()).then(d => {
    if(d.success) document.getElementById('stepFinish').classList.add('active'), document.getElementById('step1').classList.remove('active');
    else status.innerHTML = '<div class="status" style="color:#ef4444">Failed to connect</div>';
  });
}
load();
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
<title>Slave Status - Hospital Alarm</title>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700&display=swap" rel="stylesheet">
<style>
:root { --bg: #0f172a; --card-bg: rgba(30, 41, 59, 0.7); --primary: #10b981; --slate-400: #94a3b8; --slate-200: #e2e8f0; --glass-border: rgba(255, 255, 255, 0.1); }
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Outfit', sans-serif; background:#0f172a; color:var(--slate-200); min-height:100vh; display:flex; align-items:center; justify-content:center; padding:20px; }
.card { background:var(--card-bg); backdrop-filter:blur(16px); border:1px solid var(--glass-border); border-radius:24px; padding:32px; width:100%; max-width:400px; text-align:center; }
.status-icon { width:80px; height:80px; background:rgba(255,255,255,0.05); border-radius:50%; display:inline-flex; align-items:center; justify-content:center; font-size:32px; margin-bottom:20px; }
.status-icon.active { background:rgba(16, 185, 129, 0.2); color:var(--primary); box-shadow:0 0 20px rgba(16, 185, 129, 0.2); }
.status-icon.alert { background:rgba(239, 68, 68, 0.2); color:#ef4444; animation: pulse 1s infinite; }
@keyframes pulse { 0%, 100% { transform: scale(1); } 50% { transform: scale(1.1); } }
h1 { font-size:20px; margin-bottom:24px; }
.info-row { display:flex; justify-content:space-between; padding:12px 0; border-bottom:1px solid var(--glass-border); font-size:13px; }
.info-label { color:var(--slate-400); }
.info-value { font-weight:600; }
.btn-alert { width:100%; padding:16px; border:none; border-radius:16px; background:linear-gradient(135deg, #ef4444, #b91c1c); color:white; font-weight:700; margin-top:24px; cursor:pointer; }
</style>
</head>
<body>
<div class="card">
  <div id="icon" class="status-icon">📡</div>
  <h1 id="status-title">Monitoring...</h1>
  <div class="info-row"><span class="info-label">ID</span><span class="info-value" id="d-id">-</span></div>
  <div class="info-row"><span class="info-label">IP</span><span class="info-value" id="d-ip">-</span></div>
  <div class="info-row"><span class="info-label">Master</span><span class="info-value" id="d-master">-</span></div>
  <button class="btn-alert" onclick="trigger()">🚨 TRIGGER ALERT</button>
</div>
<script>
function update() {
  fetch('/api/status').then(r => r.json()).then(d => {
    document.getElementById('d-id').textContent = d.id;
    document.getElementById('d-ip').textContent = d.ip;
    document.getElementById('d-master').textContent = d.master;
    const icon = document.getElementById('icon');
    const title = document.getElementById('status-title');
    if(d.alertPending) { icon.className='status-icon alert'; icon.textContent='🚨'; title.textContent='Alert Active!'; }
    else if(d.connected) { icon.className='status-icon active'; icon.textContent='🛡️'; title.textContent='Secured'; }
    else { icon.className='status-icon'; icon.textContent='📡'; title.textContent='Connecting...'; }
  });
}
function trigger() { fetch('/api/trigger-alert', {method:'POST'}).then(update); }
setInterval(update, 3000); update();
</script>
</body>
</html>
)rawliteral";

#endif
