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
const authHeader = 'admin1234';
const authType = 'X-Auth-Token';
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
    headers: { 'Content-Type': 'application/json', [authType]: authHeader },
    body: JSON.stringify({ mode: selMode })
  }).then(() => {
    document.getElementById('finish-info').textContent = 'Redirecting to Dashboard...';
    showStep('stepFinish');
    setTimeout(() => { window.location.href = '/'; }, 1500);
  });
}
</script>
</body>
</html>
)rawliteral";

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Unified Dashboard - Hospital Alarm</title>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;500;600;700&display=swap" rel="stylesheet">
<style>
:root { --bg: #020617; --card-bg: rgba(15, 23, 42, 0.6); --primary: #10b981; --crimson: #ef4444; --slate-400: #94a3b8; --slate-200: #e2e8f0; --glass: rgba(255,255,255,0.05); }
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:'Outfit', sans-serif; background:var(--bg); color:var(--slate-200); min-height:100vh; display:flex; flex-direction:column; }
header { background:rgba(2, 6, 23, 0.8); backdrop-filter:blur(12px); border-bottom:1px solid var(--glass); padding:20px 40px; display:flex; justify-content:space-between; align-items:center; position:sticky; top:0; z-index:100; }
.logo-area { display:flex; align-items:center; gap:12px; }
.logo-icon { width:40px; height:40px; background:linear-gradient(135deg, #3b82f6, #8b5cf6); border-radius:12px; display:flex; align-items:center; justify-content:center; font-size:20px; box-shadow:0 10px 20px rgba(59,130,246,0.3); }
.logo-text h1 { font-size:18px; font-weight:700; color:white; }
.logo-text p { font-size:12px; color:var(--slate-400); text-transform:uppercase; letter-spacing:1px; }

main { padding:40px; flex:1; max-width:1400px; margin:0 auto; width:100%; display:flex; flex-direction:column; gap:40px; }
.section-title { font-size:14px; font-weight:600; color:var(--slate-400); display:flex; align-items:center; gap:10px; margin-bottom:20px; text-transform:uppercase; letter-spacing:1px; }

.grid { display:grid; grid-template-columns:repeat(auto-fill, minmax(320px, 1fr)); gap:24px; }

/* Dashboard Card matching React */
.card { background:var(--card-bg); border:1px solid var(--glass); border-radius:24px; padding:24px; position:relative; overflow:hidden; transition:all 0.3s; backdrop-filter:blur(12px); }
.card:hover { border-color:rgba(16,185,129,0.3); transform:translateY(-4px); }
.card.alerting { border-color:var(--crimson); background:rgba(239, 68, 68, 0.05); }
.card.alerting::before { content:''; position:absolute; inset:0; background:rgba(239,68,68,0.1); animation:pulse 2s infinite; pointer-events:none; }
@keyframes pulse { 0%, 100% { opacity:0.3; } 50% { opacity:1; } }

.c-head { display:flex; justify-content:space-between; margin-bottom:24px; }
.c-title { font-size:20px; font-weight:700; color:white; line-height:1.2; }
.c-id { font-size:12px; color:var(--slate-400); margin-top:4px; display:flex; align-items:center; gap:4px; }
.c-icon { width:56px; height:56px; border-radius:16px; display:flex; align-items:center; justify-content:center; font-size:24px; transition:0.4s; }
.card.online .c-icon { background:rgba(16,185,129,0.15); border:1px solid rgba(16,185,129,0.2); }
.card.offline .c-icon { background:var(--glass); border:1px solid var(--glass); opacity:0.5; }
.card.alerting .c-icon { background:var(--crimson); box-shadow:0 10px 20px rgba(239,68,68,0.4); transform:scale(1.1); animation:bounce 2s infinite; }
@keyframes bounce { 0%, 100% { transform:translateY(0) scale(1.1); } 50% { transform:translateY(-5px) scale(1.1); } }

.c-stats { display:grid; grid-template-columns:1fr 1fr; gap:12px; }
.c-box { background:rgba(255,255,255,0.03); border:1px solid var(--glass); border-radius:12px; padding:12px; }
.c-box-label { font-size:12px; color:var(--slate-400); margin-bottom:4px; display:flex; align-items:center; gap:6px; }
.c-box-val { font-size:16px; font-weight:700; color:white; }

.c-foot { margin-top:20px; padding-top:16px; border-top:1px solid var(--glass); display:flex; justify-content:space-between; font-size:11px; font-weight:600; text-transform:uppercase; letter-spacing:1px; color:var(--slate-400); }
.c-foot-left { display:flex; align-items:center; gap:8px; }
.dot { width:8px; height:8px; border-radius:50%; background:var(--slate-400); }
.card.online .dot { background:var(--primary); box-shadow:0 0 10px var(--primary); }
.card.alerting .dot { background:var(--crimson); box-shadow:0 0 10px var(--crimson); }

/* Admin Pending Card */
.pending-card { background:rgba(245, 158, 11, 0.05); border:1px dashed rgba(245, 158, 11, 0.3); border-radius:16px; padding:20px; display:flex; justify-content:space-between; align-items:center; }
.p-id { font-family:monospace; font-size:16px; font-weight:600; color:#f59e0b; }
.p-desc { font-size:13px; color:var(--slate-400); margin-top:4px; }
.btn { padding:10px 20px; border-radius:10px; border:none; font-weight:600; cursor:pointer; color:white; transition:0.2s; }
.btn:hover { transform:translateY(-2px); }
.btn-approve { background:var(--primary); box-shadow:0 5px 15px rgba(16,185,129,0.3); }

/* Modal */
.modal-overlay { position:fixed; inset:0; background:rgba(0,0,0,0.8); backdrop-filter:blur(8px); z-index:999; display:none; align-items:center; justify-content:center; }
.modal { background:var(--card-bg); border:1px solid var(--glass); border-radius:24px; padding:32px; width:100%; max-width:400px; box-shadow:0 25px 50px rgba(0,0,0,0.5); }
.input { width:100%; padding:14px; background:rgba(0,0,0,0.3); border:1px solid var(--glass); border-radius:12px; color:white; margin-bottom:16px; outline:none; font-family:inherit; }
.input:focus { border-color:#3b82f6; }
label { display:block; font-size:12px; color:var(--slate-400); margin-bottom:6px; font-weight:500; }
</style>
</head>
<body>

<header>
  <div class="logo-area">
    <div class="logo-icon">&#128276;</div>
    <div class="logo-text">
      <h1>Station Dashboard</h1>
      <p>Local Operating Mode</p>
    </div>
  </div>
  <div>
    <button class="btn" style="background:rgba(255,255,255,0.1);" onclick="location.href='/setup'">Configure AP</button>
  </div>
</header>

<main>
  <div id="admin-panel" style="display:none;">
    <div class="section-title"><span style="color:#f59e0b;">&#9888;</span> Action Required</div>
    <div id="pending-grid" style="display:flex; flex-direction:column; gap:12px;"></div>
  </div>

  <div>
    <div class="section-title">&#128187; Active Monitoring Units</div>
    <div class="grid" id="slave-grid"></div>
  </div>
</main>

<div class="modal-overlay" id="approval-modal">
  <div class="modal">
    <h2 style="margin-bottom:24px; font-weight:700;">Approve Device</h2>
    <label>Patient/Device Name</label>
    <input type="text" id="m-name" class="input" placeholder="e.g. John Doe">
    <div style="display:flex; gap:15px;">
      <div style="flex:1;"><label>Bed</label><input type="text" id="m-bed" class="input" placeholder="e.g. B-12"></div>
      <div style="flex:1;"><label>Room</label><input type="text" id="m-room" class="input" placeholder="e.g. ICU"></div>
    </div>
    <div style="display:flex; gap:12px; margin-top:10px;">
      <button class="btn btn-approve" style="flex:1;" onclick="submitApproval()">Approve & Join Mesh</button>
      <button class="btn" style="background:rgba(255,255,255,0.1);" onclick="closeModal()">Cancel</button>
    </div>
  </div>
</div>

<script>
let pendingApprovalId = null;
const authHeader = 'admin1234';

function refresh() {
  fetch('/api/slaves?all=1').then(r => r.json()).then(slaves => {
    const grid = document.getElementById('slave-grid');
    const pendGrid = document.getElementById('pending-grid');
    const adminPanel = document.getElementById('admin-panel');
    
    let hActive = '';
    let hPending = '';
    let pendingCount = 0;

    slaves.forEach(s => {
      if (!s.approved) {
        pendingCount++;
        hPending += `<div class="pending-card">
                       <div>
                         <div class="p-id">${s.slaveId}</div>
                         <div class="p-desc">Unknown device requesting mesh access</div>
                       </div>
                       <button class="btn btn-approve" onclick="openApproval('${s.slaveId}')">Approve Unit</button>
                     </div>`;
      } else {
        const isOnline = s.registered;
        const isAlert = s.alertActive;
        const stateClass = isAlert ? 'alerting' : (isOnline ? 'online' : 'offline');
        const icon = isAlert ? '&#128276;' : (isOnline ? '&#10084;' : '&#9888;');
        const timeStr = s.lastAlertTime ? new Date(s.lastAlertTime).toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'}) : 'No Events';

        hActive += `<div class="card ${stateClass}">
                      <div class="c-head">
                        <div>
                          <div class="c-title">${s.patientName || 'Unnamed Unit'}</div>
                          <div class="c-id">&#128274; ${s.slaveId}</div>
                        </div>
                        <div class="c-icon">${icon}</div>
                      </div>
                      <div class="c-stats">
                        <div class="c-box"><div class="c-box-label">&#128716; Bed</div><div class="c-box-val">${s.bed || '-'}</div></div>
                        <div class="c-box"><div class="c-box-label">&#128682; Room</div><div class="c-box-val">${s.room || '-'}</div></div>
                      </div>
                      <div class="c-foot">
                        <div class="c-foot-left">
                          <div class="dot"></div>
                          ${isOnline ? 'SYSTEM STABLE' : 'LINK DOWN'}
                        </div>
                        <div>&#128336; ${timeStr}</div>
                      </div>
                    </div>`;
      }
    });

    if (pendingCount > 0) {
      adminPanel.style.display = 'block';
      pendGrid.innerHTML = hPending;
    } else {
      adminPanel.style.display = 'none';
      pendGrid.innerHTML = '';
    }

    grid.innerHTML = hActive || '<div style="grid-column:1/-1; text-align:center; padding:100px; color:var(--slate-400);">No active units found. Connect a slave module to begin.</div>';
  });
}

function openApproval(id) {
  pendingApprovalId = id;
  document.getElementById('approval-modal').style.display = 'flex';
}
function closeModal() {
  document.getElementById('approval-modal').style.display = 'none';
}
function submitApproval() {
  const n = document.getElementById('m-name').value;
  const b = document.getElementById('m-bed').value;
  const r = document.getElementById('m-room').value;
  
  fetch('/api/approve/' + pendingApprovalId, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', 'X-Auth-Token': authHeader },
    body: JSON.stringify({ patientName: n, bed: b, room: r })
  }).then(res => {
    closeModal();
    refresh();
  });
}

refresh();
setInterval(refresh, 2000);
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
const authHeader = 'Basic YWRtaW46YWRtaW4xMjM0';
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
    headers: { 'Content-Type': 'application/json', [authType]: authHeader },
    body: JSON.stringify({ patientName: n, bed: b, room: r })
  }).then(r => {
    if(!r.ok) alert('Auth Failed');
    closeModal();
    setTimeout(refresh, 500);
  });
}
function delDev(id) { 
  if(confirm('Delete ' + id + '?')) {
    fetch('/api/slaves/' + id, {
      method:'DELETE',
      headers: { [authType]: authHeader }
    }).then(() => setTimeout(refresh, 500)); 
  }
}
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
