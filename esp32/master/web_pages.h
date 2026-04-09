#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <Arduino.h>

// --- SHARED STYLES -----------------------------------------
const char SHARED_CSS[] PROGMEM = R"rawliteral(
:root {
  --bg: #0f172a;
  --card-bg: rgba(30, 41, 59, 0.7);
  --primary: #3b82f6;
  --primary-glow: rgba(59, 130, 246, 0.5);
  --emerald: #10b981;
  --emerald-glow: rgba(16, 185, 129, 0.4);
  --crimson: #ef4444;
  --crimson-glow: rgba(239, 68, 68, 0.4);
  --slate-400: #94a3b8;
  --slate-200: #e2e8f0;
  --glass-border: rgba(255, 255, 255, 0.1);
}

* { margin: 0; padding: 0; box-sizing: border-box; }

body {
  font-family: 'Outfit', 'Inter', system-ui, sans-serif;
  background: radial-gradient(circle at top right, #1e293b, #0f172a);
  color: var(--slate-200);
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.glass {
  background: var(--card-bg);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  border: 1px solid var(--glass-border);
  box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
}

.btn {
  padding: 12px 24px;
  border-radius: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  border: none;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
}

.btn-primary {
  background: linear-gradient(135deg, var(--primary), #2563eb);
  color: white;
  box-shadow: 0 4px 15px var(--primary-glow);
}

.btn-primary:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px var(--primary-glow);
}

.btn-secondary {
  background: rgba(148, 163, 184, 0.1);
  color: var(--slate-200);
  border: 1px solid var(--glass-border);
}

.btn-secondary:hover {
  background: rgba(148, 163, 184, 0.2);
}

input {
  width: 100%;
  padding: 12px 16px;
  background: rgba(15, 23, 42, 0.6);
  border: 1px solid var(--glass-border);
  border-radius: 10px;
  color: white;
  outline: none;
  transition: all 0.3s;
}

input:focus {
  border-color: var(--primary);
  box-shadow: 0 0 0 3px var(--primary-glow);
}
)rawliteral";

// --- SETUP PAGE ---
const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Setup - Hospital Alarm</title>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700&display=swap" rel="stylesheet">
<style>
/* CSS will be injected here during build or used as is */
:root { --bg: #0f172a; --card-bg: rgba(30, 41, 59, 0.7); --primary: #3b82f6; --primary-glow: rgba(59, 130, 246, 0.5); --emerald: #10b981; --crimson: #ef4444; --slate-400: #94a3b8; --slate-200: #e2e8f0; --glass-border: rgba(255, 255, 255, 0.1); }
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Outfit', sans-serif; background:#0f172a; color:var(--slate-200); min-height:100vh; display:flex; align-items:center; justify-content:center; padding:20px; }
.card { background:var(--card-bg); backdrop-filter:blur(16px); border:1px solid var(--glass-border); border-radius:24px; padding:40px; width:100%; max-width:500px; box-shadow:0 20px 50px rgba(0,0,0,0.5); }
.logo { text-align:center; margin-bottom:30px; }
.logo-icon { width:64px; height:64px; background:linear-gradient(135deg, var(--primary), #8b5cf6); border-radius:18px; display:inline-flex; align-items:center; justify-content:center; font-size:32px; margin-bottom:12px; box-shadow:0 8px 20px var(--primary-glow); }
.logo h1 { font-size:24px; font-weight:700; background:linear-gradient(135deg, #60a5fa, #a78bfa); -webkit-background-clip:text; -webkit-text-fill-color:transparent; }
.step { display:none; } .step.active { display:block; animation: fadeIn 0.4s ease; }
@keyframes fadeIn { from { opacity:0; transform:translateY(10px); } to { opacity:1; transform:translateY(0); } }
.modes { display:flex; flex-direction:column; gap:12px; margin:20px 0; }
.mode-card { background:rgba(255,255,255,0.03); border:1px solid var(--glass-border); border-radius:16px; padding:20px; cursor:pointer; transition:all 0.3s; }
.mode-card:hover { border-color:var(--primary); background:rgba(59,130,246,0.05); transform:translateY(-2px); }
.mode-card.selected { border-color:var(--primary); background:rgba(59,130,246,0.1); box-shadow:0 0 20px rgba(59,130,246,0.15); }
.mode-card h3 { font-size:16px; display:flex; align-items:center; gap:10px; margin-bottom:4px; }
.mode-card p { font-size:13px; color:var(--slate-400); line-height:1.4; }
.btn { width:100%; padding:14px; border:none; border-radius:14px; font-size:15px; font-weight:600; cursor:pointer; transition:all 0.3s; margin-top:10px; background:linear-gradient(135deg, var(--primary), #2563eb); color:white; }
.btn:disabled { opacity:0.5; cursor:not-allowed; }
.btn-secondary { background:rgba(148,163,184,0.1); color:var(--slate-400); margin-top:8px; border:1px solid var(--glass-border); }
.form-group { margin-bottom:16px; }
.form-group label { display:block; font-size:13px; color:var(--slate-400); margin-bottom:8px; }
.form-group input { width:100%; padding:12px 16px; background:rgba(0,0,0,0.2); border:1px solid var(--glass-border); border-radius:12px; color:white; outline:none; }
.form-group input:focus { border-color:var(--primary); }
.wifi-list { max-height:180px; overflow-y:auto; border-radius:12px; border:1px solid var(--glass-border); margin-bottom:16px; }
.wifi-item { padding:12px 16px; border-bottom:1px solid var(--glass-border); cursor:pointer; display:flex; justify-content:space-between; }
.wifi-item:hover { background:rgba(255,255,255,0.05); }
.wifi-item.selected { background:rgba(59,130,246,0.1); border-left:3px solid var(--primary); }
.scanning { text-align:center; padding:20px; color:var(--slate-400); }
.spinner { width:20px; height:20px; border:2px solid rgba(255,255,255,0.1); border-top-color:var(--primary); border-radius:50%; animation:spin 1s linear infinite; display:inline-block; vertical-align:middle; margin-right:8px; }
@keyframes spin { to { transform:rotate(360deg); } }
.status { padding:12px; border-radius:12px; text-align:center; font-size:13px; margin-top:12px; }
.status.info { background:rgba(59,130,246,0.1); color:var(--primary); border:1px solid rgba(59,130,246,0.2); }
.status.error { background:rgba(239,68,68,0.1); color:var(--crimson); border:1px solid rgba(239,68,68,0.2); }
</style>
</head>
<body>
<div class="card">
  <div class="logo">
    <div class="logo-icon">&#128276;</div>
    <h1>Hospital Alarm</h1>
    <p>System Configuration</p>
  </div>
  
  <div id="step1" class="step active">
    <h2 style="font-size:17px; text-align:center; margin-bottom:20px;">Select Network Mode</h2>
    <div class="modes">
      <div class="mode-card" onclick="selectMode(1)" id="mode1">
        <h3>AP Mode</h3>
        <p>Standalone local network. No hospital Wi-Fi required.</p>
      </div>
      <div class="mode-card" onclick="selectMode(2)" id="mode2">
        <h3>STA Mode</h3>
        <p>Connects to hospital network for wider coverage.</p>
      </div>
      <div class="mode-card" onclick="selectMode(3)" id="mode3">
        <h3>AP + STA Mode</h3>
        <p>Hybrid mode. Local AP + Hospital network connection.</p>
      </div>
      <div class="mode-card" onclick="selectMode(4)" id="mode4">
        <h3>Cloud Mode</h3>
        <p>Full internet sync. View alerts from anywhere in the world.</p>
      </div>
    </div>
    <button class="btn" id="nextBtn" onclick="goStep2()" disabled>Continue</button>
  </div>

  <div id="stepWiFi" class="step">
    <h2 style="font-size:17px; text-align:center; margin-bottom:15px;">Connect to Network</h2>
    <div id="scan-list" class="wifi-list">
      <div class="scanning"><div class="spinner"></div>Scanning...</div>
    </div>
    <div class="form-group">
      <label>Password</label>
      <input type="password" id="wifi-pass" placeholder="Network password">
    </div>
    <div id="setup-status"></div>
    <button class="btn" id="connectBtn" onclick="doConnect()" disabled>Connect & Continue</button>
    <button class="btn btn-secondary" onclick="scanWiFi()">Rescan</button>
  </div>
  
  <div id="stepFinish" class="step">
    <div style="text-align:center; padding:20px 0;">
      <div style="font-size:48px; margin-bottom:15px;">&#9989;</div>
      <h2 style="margin-bottom:10px;">Setup Complete!</h2>
      <p id="finish-info" style="color:var(--slate-400); font-size:14px;"></p>
      <button class="btn" style="margin-top:20px;" onclick="location.href='/'">Go to Dashboard</button>
    </div>
  </div>
</div>

<script>
let selMode=0, selSSID='';
function selectMode(m) {
  selMode=m;
  document.querySelectorAll('.mode-card').forEach(c => c.classList.remove('selected'));
  document.getElementById('mode'+m).classList.add('selected');
  document.getElementById('nextBtn').disabled=false;
}
function showStep(s) {
  document.querySelectorAll('.step').forEach(el => el.classList.remove('active'));
  document.getElementById(s).classList.add('active');
}
function goStep2() {
  if(selMode === 1) { finishSetup(); return; }
  showStep('stepWiFi');
  scanWiFi();
}
function scanWiFi() {
  const list = document.getElementById('scan-list');
  list.innerHTML = '<div class="scanning"><div class="spinner"></div>Scanning...</div>';
  fetch('/api/scan').then(r => r.json()).then(nets => {
    let h = '';
    nets.forEach(n => {
      h += `<div class="wifi-item" onclick="pickWiFi(this, '${n.ssid}')">
              <span>${n.ssid}</span>
              <span style="font-size:11px; opacity:0.6;">${n.rssi}dBm</span>
            </div>`;
    });
    list.innerHTML = h || '<div class="scanning">No networks found</div>';
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
  status.innerHTML = '<div class="status info">Connecting to ' + selSSID + '...</div>';
  fetch('/api/connect-wifi', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ ssid: selSSID, password: pass })
  }).then(r => r.json()).then(d => {
    if(d.success) {
      status.innerHTML = '<div class="status info">Connected! Applying settings...</div>';
      finishSetup();
    } else {
      status.innerHTML = '<div class="status error">Failed: ' + d.message + '</div>';
    }
  });
}
function finishSetup() {
  fetch('/api/setup', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ mode: selMode })
  }).then(() => {
    document.getElementById('finish-info').textContent = 'System is ready in mode ' + selMode;
    showStep('stepFinish');
  });
}
</script>
</body>
</html>
)rawliteral";

// --- DASHBOARD PAGE ---
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Dashboard - Hospital Alarm</title>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700&display=swap" rel="stylesheet">
<style>
:root { --bg: #0f172a; --card-bg: rgba(30, 41, 59, 0.6); --primary: #3b82f6; --crimson: #ef4444; --emerald: #10b981; --slate-400: #94a3b8; --slate-200: #e2e8f0; --glass-border: rgba(255, 255, 255, 0.1); }
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Outfit', sans-serif; background:#0f172a; color:var(--slate-200); min-height:100vh; overflow-x:hidden; }
header { padding: 20px 40px; background: rgba(15, 23, 42, 0.8); backdrop-filter: blur(10px); border-bottom: 1px solid var(--glass-border); display: flex; justify-content: space-between; align-items: center; position: sticky; top: 0; z-index: 100; }
.logo { display:flex; align-items:center; gap:12px; }
.logo-icon { width:36px; height:36px; background:linear-gradient(135deg, var(--primary), #8b5cf6); border-radius:10px; display:flex; align-items:center; justify-content:center; font-size:18px; }
.logo h1 { font-size:18px; font-weight:700; }
.stats { display:flex; gap:15px; }
.stat-item { background:rgba(255,255,255,0.05); padding:6px 14px; border-radius:20px; font-size:12px; font-weight:600; border:1px solid var(--glass-border); }
.stat-item.alert { background:rgba(239,68,68,0.1); color:var(--crimson); border-color:rgba(239,68,68,0.2); animation: pulse 2s infinite; }
@keyframes pulse { 0%, 100% { opacity:1; } 50% { opacity:0.6; } }
main { padding: 40px; max-width: 1400px; margin: 0 auto; }
.grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(320px, 1fr)); gap: 24px; }
.card { background: var(--card-bg); backdrop-filter: blur(12px); border: 1px solid var(--glass-border); border-radius: 24px; padding: 24px; transition: all 0.3s; position: relative; overflow: hidden; }
.card:hover { transform: translateY(-5px); border-color: var(--primary); }
.card.alerting { border-color: var(--crimson); background: rgba(239, 68, 68, 0.05); animation: alertGlow 2s infinite; }
@keyframes alertGlow { 0%, 100% { box-shadow: 0 0 0 0 rgba(239, 68, 68, 0); } 50% { box-shadow: 0 0 20px rgba(239, 68, 68, 0.2); } }
.card-header { display: flex; justify-content: space-between; margin-bottom: 20px; }
.p-name { font-size: 20px; font-weight: 700; color: white; }
.p-id { font-size: 12px; color: var(--slate-400); font-family: monospace; }
.status-badge { padding: 4px 12px; border-radius: 20px; font-size: 11px; font-weight: 700; text-transform: uppercase; }
.badge-alert { background: var(--crimson); color: white; box-shadow: 0 4px 10px var(--crimson); }
.badge-online { background: rgba(16, 185, 129, 0.1); color: var(--emerald); border: 1px solid rgba(16, 185, 129, 0.2); }
.info-row { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-top: 15px; }
.info-box { background: rgba(0,0,0,0.2); padding: 10px; border-radius: 12px; }
.info-label { font-size: 10px; color: var(--slate-400); text-transform: uppercase; letter-spacing: 0.5px; }
.info-value { font-size: 16px; font-weight: 600; margin-top: 2px; }
.card-footer { margin-top: 20px; padding-top: 15px; border-top: 1px solid var(--glass-border); font-size: 11px; color: var(--slate-400); display: flex; align-items: center; gap: 8px; }
.dot { width: 8px; height: 8px; border-radius: 50%; background: var(--slate-400); }
.dot.online { background: var(--emerald); box-shadow: 0 0 8px var(--emerald); }
.nav-btn { color: var(--slate-200); text-decoration: none; font-size: 13px; padding: 8px 16px; border-radius: 10px; background: rgba(255,255,255,0.05); }
.nav-btn:hover { background: rgba(255,255,255,0.1); }
</style>
</head>
<body>
<header>
  <div class="logo">
    <div class="logo-icon">&#128276;</div>
    <h1>Hospital Alarm</h1>
  </div>
  <div class="stats" id="head-stats"></div>
  <div style="display:flex; gap:10px;">
    <a href="/admin" class="nav-btn">Admin Panel</a>
    <a href="/setup" class="nav-btn">Setup</a>
  </div>
</header>
<main>
  <div class="grid" id="slave-grid"></div>
</main>
<script>
function refresh() {
  fetch('/api/slaves?approved=1').then(r => r.json()).then(slaves => {
    const grid = document.getElementById('slave-grid');
    const stats = document.getElementById('head-stats');
    let alerts = slaves.filter(s => s.alertActive).length;
    stats.innerHTML = `<div class="stat-item">${slaves.length} Units</div>
                       <div class="stat-item ${alerts?'alert':''}">${alerts} Alerts</div>`;
    
    let h = '';
    slaves.forEach(s => {
      h += `<div class="card ${s.alertActive?'alerting':''}">
              <div class="card-header">
                <div>
                  <div class="p-name">${s.patientName || 'Emergency Unit'}</div>
                  <div class="p-id">${s.slaveId}</div>
                </div>
                <div>
                  ${s.alertActive ? '<span class="status-badge badge-alert">Alert</span>' : '<span class="status-badge badge-online">Monitoring</span>'}
                </div>
              </div>
              <div class="info-row">
                <div class="info-box"><div class="info-label">Bed</div><div class="info-value">${s.bed || '-'}</div></div>
                <div class="info-box"><div class="info-label">Room</div><div class="info-value">${s.room || '-'}</div></div>
              </div>
              <div class="card-footer">
                <div class="dot ${s.registered?'online':''}"></div>
                ${s.registered ? 'Device Connected' : 'Last seen: ' + new Date(s.lastSeen).toLocaleTimeString()}
              </div>
            </div>`;
    });
    grid.innerHTML = h || '<div style="grid-column: 1/-1; text-align:center; padding:100px; opacity:0.5;">No active units</div>';
  });
}
refresh(); setInterval(refresh, 2000);
</script>
</body>
</html>
)rawliteral";

// --- ADMIN PAGE ---
const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Admin - Hospital Alarm</title>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700&display=swap" rel="stylesheet">
<style>
:root { --bg: #0f172a; --card-bg: rgba(30, 41, 59, 0.7); --primary: #3b82f6; --crimson: #ef4444; --emerald: #10b981; --slate-400: #94a3b8; --slate-200: #e2e8f0; --glass-border: rgba(255, 255, 255, 0.1); }
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Outfit', sans-serif; background:#0f172a; color:var(--slate-200); min-height:100vh; padding: 20px; }
.container { max-width: 900px; margin: 0 auto; }
header { margin-bottom: 30px; display: flex; justify-content: space-between; align-items: center; }
.card { background: var(--card-bg); backdrop-filter: blur(10px); border: 1px solid var(--glass-border); border-radius: 20px; padding: 20px; margin-bottom: 15px; display: flex; align-items: center; justify-content: space-between; }
.card.pending { border-left: 4px solid #f59e0b; }
.info { flex: 1; }
.id { font-family: monospace; font-weight: 700; color: var(--primary); }
.meta { font-size: 13px; color: var(--slate-400); }
.actions { display: flex; gap: 10px; }
.btn { padding: 8px 16px; border-radius: 10px; border: none; font-weight: 600; cursor: pointer; transition: 0.2s; font-size: 13px; }
.btn-approve { background: var(--emerald); color: white; }
.btn-delete { background: rgba(239, 68, 68, 0.1); color: var(--crimson); }
.btn-edit { background: rgba(59, 130, 246, 0.1); color: var(--primary); }
.section-title { font-size: 14px; text-transform: uppercase; letter-spacing: 1px; color: var(--slate-400); margin-bottom: 15px; margin-top: 30px; border-bottom: 1px solid var(--glass-border); padding-bottom: 8px; }

/* Modal */
.modal-wrap { position: fixed; inset: 0; background: rgba(0,0,0,0.8); backdrop-filter: blur(5px); display: none; align-items: center; justify-content: center; z-index: 1000; }
.modal { background: #1e293b; border: 1px solid var(--glass-border); border-radius: 24px; padding: 30px; width: 100%; max-width: 400px; }
.form-group { margin-bottom: 15px; }
.form-group label { display: block; font-size: 12px; color: var(--slate-400); margin-bottom: 5px; }
.form-group input { width: 100%; padding: 12px; background: rgba(0,0,0,0.2); border: 1px solid var(--glass-border); border-radius: 10px; color: white; }
</style>
</head>
<body>
<div class="container">
  <header>
    <h1>Admin Panel</h1>
    <a href="/" style="color: var(--primary); text-decoration: none;">&larr; Dashboard</a>
  </header>
  
  <div class="section-title">New Devices (Awaiting Approval)</div>
  <div id="pending-list"></div>
  
  <div class="section-title">Approved Units</div>
  <div id="approved-list"></div>
</div>

<div class="modal-wrap" id="modal">
  <div class="modal">
    <h2 id="m-title" style="margin-bottom:20px;">Approve Device</h2>
    <div class="form-group"><label>Patient Name</label><input type="text" id="m-name"></div>
    <div class="form-group"><label>Bed</label><input type="text" id="m-bed"></div>
    <div class="form-group"><label>Room</label><input type="text" id="m-room"></div>
    <div style="display:flex; gap:10px; margin-top:20px;">
      <button class="btn btn-approve" style="flex:1" onclick="submitModal()">Save</button>
      <button class="btn" style="background:#334155; color:white; flex:1" onclick="closeModal()">Cancel</button>
    </div>
  </div>
</div>

<script>
let editId = null;
function refresh() {
  fetch('/api/slaves?all=1').then(r => r.json()).then(devs => {
    const pend = document.getElementById('pending-list');
    const appr = document.getElementById('approved-list');
    let hP = '', hA = '';
    devs.forEach(d => {
      const card = `<div class="card ${d.approved?'':'pending'}">
                      <div class="info">
                        <div class="id">${d.slaveId}</div>
                        <div class="meta">${d.approved ? (d.patientName + ' | Bed ' + d.bed) : 'New request detected'}</div>
                      </div>
                      <div class="actions">
                        ${d.approved ? `<button class="btn btn-edit" onclick="openEdit('${d.slaveId}', '${d.patientName}', '${d.bed}', '${d.room}')">Edit</button>` 
                                     : `<button class="btn btn-approve" onclick="openApprove('${d.slaveId}')">Approve</button>`}
                        <button class="btn btn-delete" onclick="delDev('${d.slaveId}')">Delete</button>
                      </div>
                    </div>`;
      if(d.approved) hA += card; else hP += card;
    });
    pend.innerHTML = hP || '<p style="opacity:0.5; padding:20px;">No pending devices</p>';
    appr.innerHTML = hA || '<p style="opacity:0.5; padding:20px;">No approved units</p>';
  });
}
function openApprove(id) { editId=id; document.getElementById('modal').style.display='flex'; }
function closeModal() { document.getElementById('modal').style.display='none'; }
function openEdit(id, n, b, r) { editId=id; document.getElementById('m-name').value=n; document.getElementById('m-bed').value=b; document.getElementById('m-room').value=r; document.getElementById('modal').style.display='flex'; }
function submitModal() {
  const n = document.getElementById('m-name').value;
  const b = document.getElementById('m-bed').value;
  const r = document.getElementById('m-room').value;
  fetch('/api/approve/' + editId, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ patientName: n, bed: b, room: r })
  }).then(() => { closeModal(); refresh(); });
}
function delDev(id) { if(confirm('Delete ' + id + '?')) fetch('/api/slaves/' + id, {method:'DELETE'}).then(refresh); }
refresh();
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
<title>Cloud Info - Hospital Alarm</title>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700&display=swap" rel="stylesheet">
<style>
:root { --bg: #0f172a; --card-bg: rgba(30, 41, 59, 0.7); --primary: #3b82f6; --slate-400: #94a3b8; --slate-200: #e2e8f0; --glass-border: rgba(255, 255, 255, 0.1); }
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Outfit', sans-serif; background:#0f172a; color:var(--slate-200); min-height:100vh; display:flex; align-items:center; justify-content:center; padding:20px; }
.card { background:var(--card-bg); backdrop-filter:blur(16px); border:1px solid var(--glass-border); border-radius:24px; padding:40px; width:100%; max-width:400px; text-align:center; }
.icon { font-size:48px; margin-bottom:20px; }
h1 { font-size:24px; margin-bottom:10px; }
p { color:var(--slate-400); font-size:14px; margin-bottom:20px; }
.btn { display:block; width:100%; padding:14px; background:var(--primary); color:white; border-radius:12px; text-decoration:none; font-weight:600; }
</style>
</head>
<body>
<div class="card">
  <div class="icon">☁️</div>
  <h1>Online Mode Active</h1>
  <p>The system is currently syncing with the Railway cloud. You can use the link below to access your dashboard from anywhere.</p>
  <a href="/setup" class="btn">Configure Network</a>
  <a href="/" style="display:block; margin-top:15px; color:var(--primary); text-decoration:none; font-size:13px;">Go to Local Dashboard</a>
</div>
</body>
</html>
)rawliteral";

#endif
