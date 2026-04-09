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
:root { --bg: #070b14; --card-bg: rgba(15, 23, 42, 0.8); --primary: #10b981; --primary-glow: rgba(16, 185, 129, 0.2); --slate-400: #64748b; --slate-200: #f1f5f9; --glass-border: rgba(255, 255, 255, 0.05); }
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Outfit', sans-serif; background:radial-gradient(circle at 0% 0%, #0f172a, #070b14); color:var(--slate-200); min-height:100vh; display:flex; align-items:center; justify-content:center; padding:20px; }
.card { background:var(--card-bg); backdrop-filter:blur(20px); -webkit-backdrop-filter:blur(20px); border:1px solid var(--glass-border); border-radius:32px; padding:40px; width:100%; max-width:440px; box-shadow:0 30px 60px rgba(0,0,0,0.4); }
.logo { text-align:center; margin-bottom:30px; }
.logo-icon { width:64px; height:64px; background:rgba(16, 185, 129, 0.1); border:1px solid var(--primary); border-radius:20px; display:inline-flex; align-items:center; justify-content:center; font-size:32px; margin-bottom:12px; }
.logo h1 { font-size:22px; font-weight:700; color:white; }
.step { display:none; } .step.active { display:block; animation: fadeIn 0.3s ease; }
@keyframes fadeIn { from { opacity:0; transform:translateY(10px); } to { opacity:1; transform:translateY(0); } }
.chip-id { display:inline-block; background:rgba(255,255,255,0.05); color:var(--primary); padding:6px 16px; border-radius:12px; font-family:monospace; font-size:12px; margin-bottom:20px; font-weight:700; opacity:0.8; }
.form-group { margin-bottom:20px; text-align:left; }
.form-group label { display:block; font-size:11px; font-weight:800; text-transform:uppercase; color:var(--slate-400); margin-bottom:8px; letter-spacing:1px; }
.form-group input { width:100%; padding:14px 18px; background:rgba(255,255,255,0.03); border:1px solid var(--glass-border); border-radius:16px; color:white; outline:none; transition:0.2s; }
.form-group input:focus { border-color:var(--primary); background:rgba(16, 185, 129, 0.05); }
.wifi-list { max-height:140px; overflow-y:auto; border-radius:16px; border:1px solid var(--glass-border); margin-bottom:20px; background:rgba(0,0,0,0.2); }
.wifi-item { padding:14px 18px; border-bottom:1px solid var(--glass-border); cursor:pointer; display:flex; justify-content:space-between; align-items:center; transition:0.2s; }
.wifi-item:hover { background:rgba(255,255,255,0.05); }
.wifi-item.selected { background:rgba(16, 185, 129, 0.15); border-left:4px solid var(--primary); }
.btn { width:100%; padding:16px; border:none; border-radius:18px; font-size:15px; font-weight:700; cursor:pointer; transition:all 0.3s; background:var(--primary); color:#070b14; }
.btn:disabled { opacity:0.3; filter:grayscale(1); }
.btn-secondary { background:rgba(255,255,255,0.05); color:white; margin-top:10px; border:1px solid var(--glass-border); }
.status { padding:14px; border-radius:16px; text-align:center; font-size:12px; margin-top:12px; background:rgba(16, 185, 129, 0.1); color:var(--primary); font-weight:700; }
</style>
</head>
<body>
<div class="card">
  <div class="logo">
    <div class="logo-icon">&#128225;</div>
    <h1>Setup Call Station</h1>
    <p style="color:var(--slate-400); font-size:13px; margin-top:4px;">Node Connection Interface</p>
  </div>
  
  <div id="step1" class="step active">
    <div style="text-align:center"><span class="chip-id" id="chip-id">SYSCFG_INIT...</span></div>
    <div id="scan-list" class="wifi-list"><div style="text-align:center; padding:30px; color:var(--slate-400); font-size:12px; font-weight:700; letter-spacing:1px;">SCANNING_RF_BAND...</div></div>
    <div class="form-group">
      <label>Passphrase</label>
      <input type="password" id="wifi-pass" placeholder="Network password">
    </div>
    <div id="setup-status"></div>
    <button class="btn" id="connectBtn" onclick="doConnect()" disabled>Authorize Link</button>
    <button class="btn btn-secondary" onclick="scanWiFi()">Rescan Frequencies</button>
  </div>

  <div id="stepFinish" class="step">
    <div style="text-align:center; padding:30px 0;">
      <div style="font-size:60px; margin-bottom:20px;">🛡️</div>
      <h2 style="margin-bottom:12px;">Link Establised</h2>
      <p style="color:var(--slate-400); font-size:14px; line-height:1.6;">Node is now broadcasting. Wait for admin approval on the Hospital Hub.</p>
      <button class="btn" style="margin-top:30px;" onclick="location.href='/'">Monitor Node</button>
    </div>
  </div>
</div>
<script>
let selSSID = '';
function load() {
  fetch('/api/status').then(r => r.json()).then(d => {
    document.getElementById('chip-id').textContent = 'ID: ' + d.id;
  });
  scanWiFi();
}
function scanWiFi() {
  const list = document.getElementById('scan-list');
  list.innerHTML = '<div style="text-align:center; padding:30px; color:var(--slate-400);">SCANNING...</div>';
  fetch('/api/scan').then(r => r.json()).then(nets => {
    let h = '';
    nets.forEach(n => {
      h += `<div class="wifi-item ${selSSID===n.ssid?'selected':''}" onclick="pickWiFi(this, '${n.ssid}')">
              <span style="font-weight:700; font-size:14px;">${n.ssid}</span>
              <span style="font-family:monospace; font-size:11px; opacity:0.6;">${n.rssi} dBm</span>
            </div>`;
    });
    list.innerHTML = h || '<div style="padding:30px; text-align:center">NONE_FOUND</div>';
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
  status.innerHTML = '<div class="status">ENCRYPTING_LINK...</div>';
  fetch('/api/connect', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ ssid: selSSID, password: pass })
  }).then(r => r.json()).then(d => {
    if(d.success) {
       document.getElementById('stepFinish').classList.add('active');
       document.getElementById('step1').classList.remove('active');
    } else status.innerHTML = '<div class="status" style="color:#ef4444">LINK_FAIL</div>';
  });
}
window.onload = load;
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
<title>Node Status - Hospital Alarm</title>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700&display=swap" rel="stylesheet">
<style>
:root { --bg: #070b14; --card-bg: rgba(15, 23, 42, 0.8); --primary: #10b981; --slate-400: #64748b; --slate-200: #f1f5f9; --glass-border: rgba(255, 255, 255, 0.05); }
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Outfit', sans-serif; background:radial-gradient(circle at 100% 100%, #1e293b, #070b14); color:var(--slate-200); min-height:100vh; display:flex; align-items:center; justify-content:center; padding:20px; }
.card { background:var(--card-bg); backdrop-filter:blur(24px); border:1px solid var(--glass-border); border-radius:32px; padding:40px; width:100%; max-width:400px; text-align:center; box-shadow:0 30px 60px rgba(0,0,0,0.4); }
.status-icon { width:100px; height:100px; background:rgba(255,255,255,0.03); border:1px solid var(--glass-border); border-radius:50%; display:inline-flex; align-items:center; justify-content:center; font-size:40px; margin-bottom:24px; transition:0.3s; }
.status-icon.active { background:rgba(16, 185, 129, 0.1); color:var(--primary); border-color:var(--primary); box-shadow:0 0 30px rgba(16, 185, 129, 0.2); }
.status-icon.alert { background:rgba(239, 68, 68, 0.15); color:#ef4444; border-color:#ef4444; animation: pulse 0.8s infinite; }
@keyframes pulse { 0%, 100% { transform: scale(1); box-shadow:0 0 20px rgba(239, 68, 68, 0.2); } 50% { transform: scale(1.05); box-shadow:0 0 40px rgba(239, 68, 68, 0.4); } }
h1 { font-size:22px; font-weight:800; margin-bottom:30px; letter-spacing:-0.5px; }
.info-row { display:flex; justify-content:space-between; padding:16px 0; border-bottom:1px solid var(--glass-border); font-size:12px; }
.info-label { color:var(--slate-400); font-weight:800; text-transform:uppercase; letter-spacing:1px; }
.info-value { font-weight:700; color:white; font-family:monospace; }
.btn-alert { width:100%; padding:20px; border:none; border-radius:20px; background:linear-gradient(135deg, #ef4444, #991b1b); color:white; font-weight:800; margin-top:30px; cursor:pointer; font-size:16px; box-shadow:0 10px 20px rgba(239, 68, 68, 0.2); transition:0.2s; }
.btn-alert:active { transform: scale(0.98); }
</style>
</head>
<body>
<div class="card">
  <div id="icon" class="status-icon">📡</div>
  <h1 id="status-title">SYS_INIT...</h1>
  <div class="info-row"><span class="info-label">IDENT</span><span class="info-value" id="d-id">-</span></div>
  <div class="info-row"><span class="info-label">UP_LINK</span><span class="info-value" id="d-master">-</span></div>
  <button class="btn-alert" onclick="trigger()">🚨 TRIGGER SIGNAL</button>
</div>
<script>
function update() {
  fetch('/api/status').then(r => r.json()).then(d => {
    document.getElementById('d-id').textContent = d.id;
    document.getElementById('d-master').textContent = d.master ? d.master.replace('http://','') : 'SEARCHING...';
    const icon = document.getElementById('icon');
    const title = document.getElementById('status-title');
    if(d.alertPending) { icon.className='status-icon alert'; icon.textContent='🚨'; title.textContent='SIGNAL_SENT'; }
    else if(d.connected) { icon.className='status-icon active'; icon.textContent='🛡️'; title.textContent='LINKED'; }
    else { icon.className='status-icon'; icon.textContent='📡'; title.textContent='CONNECTING...'; }
  });
}
function trigger() { fetch('/api/trigger-alert', {method:'POST'}).then(update); }
setInterval(update, 2000); update();
</script>
</body>
</html>
)rawliteral";

#endif
