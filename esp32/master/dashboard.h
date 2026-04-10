/*
 * ══════════════════════════════════════════════════════════════
 *  Embedded Dashboard — Single Page Application
 *  Served from ESP32 flash via PROGMEM
 *  Features: Setup wizard, Live dashboard, Admin panel
 *  Transport: WebSocket (ws://IP/ws) — zero polling
 * ══════════════════════════════════════════════════════════════
 */
#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <Arduino.h>

// ─── SETUP PAGE ──────────────────────────────────────────────
const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Setup — Hospital Alarm</title>
<style>
:root{--bg:#0a0e1a;--card:rgba(15,23,42,.85);--primary:#6366f1;--primary-g:rgba(99,102,241,.4);--emerald:#10b981;--crimson:#ef4444;--amber:#f59e0b;--s200:#e2e8f0;--s400:#94a3b8;--s600:#475569;--glass:rgba(255,255,255,.06);--glow:0 0 40px rgba(99,102,241,.15)}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:var(--bg);color:var(--s200);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px;background-image:radial-gradient(ellipse at 20% 50%,rgba(99,102,241,.08) 0%,transparent 50%),radial-gradient(ellipse at 80% 20%,rgba(16,185,129,.06) 0%,transparent 50%)}
.card{background:var(--card);backdrop-filter:blur(20px);border:1px solid var(--glass);border-radius:28px;padding:44px 36px;width:100%;max-width:480px;box-shadow:var(--glow)}
.logo{text-align:center;margin-bottom:36px}
.logo-icon{width:72px;height:72px;background:linear-gradient(135deg,var(--primary),#8b5cf6);border-radius:20px;display:inline-flex;align-items:center;justify-content:center;font-size:36px;margin-bottom:14px;box-shadow:0 12px 30px var(--primary-g);animation:float 3s ease-in-out infinite}
@keyframes float{0%,100%{transform:translateY(0)}50%{transform:translateY(-6px)}}
.logo h1{font-size:26px;font-weight:800;background:linear-gradient(135deg,#818cf8,#c084fc);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.logo p{font-size:13px;color:var(--s400);margin-top:4px;letter-spacing:.5px}
.step{display:none;animation:fadeUp .4s ease}.step.active{display:block}
@keyframes fadeUp{from{opacity:0;transform:translateY(12px)}to{opacity:1;transform:translateY(0)}}
.modes{display:flex;flex-direction:column;gap:10px;margin:20px 0}
.mode-card{background:rgba(255,255,255,.02);border:1px solid var(--glass);border-radius:16px;padding:18px 20px;cursor:pointer;transition:all .3s}
.mode-card:hover{border-color:rgba(99,102,241,.3);background:rgba(99,102,241,.04);transform:translateY(-2px)}
.mode-card.selected{border-color:var(--primary);background:rgba(99,102,241,.08);box-shadow:0 0 20px rgba(99,102,241,.1)}
.mode-card h3{font-size:15px;display:flex;align-items:center;gap:10px;margin-bottom:3px;font-weight:700}
.mode-card p{font-size:12px;color:var(--s400);line-height:1.5}
.badge{font-size:10px;padding:2px 8px;border-radius:20px;font-weight:600;text-transform:uppercase;letter-spacing:.5px}
.badge-blue{background:rgba(99,102,241,.15);color:#818cf8}
.badge-green{background:rgba(16,185,129,.15);color:#34d399}
.badge-purple{background:rgba(168,85,247,.15);color:#c084fc}
.btn{width:100%;padding:14px;border:none;border-radius:14px;font-size:14px;font-weight:700;cursor:pointer;transition:all .3s;margin-top:10px;background:linear-gradient(135deg,var(--primary),#7c3aed);color:#fff;letter-spacing:.3px}
.btn:hover{transform:translateY(-2px);box-shadow:0 8px 24px var(--primary-g)}
.btn:disabled{opacity:.4;cursor:not-allowed;transform:none;box-shadow:none}
.btn-ghost{background:rgba(148,163,184,.08);color:var(--s400);border:1px solid var(--glass);margin-top:8px}
.btn-ghost:hover{background:rgba(148,163,184,.12)}
.fg{margin-bottom:14px}
.fg label{display:block;font-size:12px;color:var(--s400);margin-bottom:6px;font-weight:600;letter-spacing:.3px}
.fg input{width:100%;padding:12px 16px;background:rgba(0,0,0,.25);border:1px solid var(--glass);border-radius:12px;color:#fff;outline:none;font-size:14px;transition:border .3s}
.fg input:focus{border-color:var(--primary);box-shadow:0 0 0 3px rgba(99,102,241,.15)}
.wl{max-height:200px;overflow-y:auto;border-radius:14px;border:1px solid var(--glass);margin-bottom:14px}
.wl::-webkit-scrollbar{width:4px}.wl::-webkit-scrollbar-thumb{background:var(--s600);border-radius:4px}
.wi{padding:14px 16px;border-bottom:1px solid var(--glass);cursor:pointer;display:flex;justify-content:space-between;align-items:center;transition:all .2s}
.wi:last-child{border-bottom:none}
.wi:hover{background:rgba(255,255,255,.03)}
.wi.selected{background:rgba(99,102,241,.08);border-left:3px solid var(--primary)}
.wi-rssi{font-size:11px;color:var(--s600);font-family:monospace}
.scanning{text-align:center;padding:24px;color:var(--s400);font-size:13px}
.spinner{width:18px;height:18px;border:2px solid rgba(255,255,255,.08);border-top-color:var(--primary);border-radius:50%;animation:spin .8s linear infinite;display:inline-block;vertical-align:middle;margin-right:8px}
@keyframes spin{to{transform:rotate(360deg)}}
.status-msg{padding:12px 16px;border-radius:12px;text-align:center;font-size:13px;margin-top:12px;font-weight:500}
.status-msg.info{background:rgba(99,102,241,.08);color:#818cf8;border:1px solid rgba(99,102,241,.15)}
.status-msg.error{background:rgba(239,68,68,.08);color:#f87171;border:1px solid rgba(239,68,68,.15)}
.status-msg.ok{background:rgba(16,185,129,.08);color:#34d399;border:1px solid rgba(16,185,129,.15)}
.fg-row{display:flex;gap:12px}.fg-row .fg{flex:1}
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
    <h2 style="font-size:16px;text-align:center;margin-bottom:18px;font-weight:700">Select Operating Mode</h2>
    <div class="modes">
      <div class="mode-card" onclick="pickMode(1)" id="m1">
        <h3>&#128225; AP Mode <span class="badge badge-blue">Local</span></h3>
        <p>Standalone network. No router needed. Slaves connect directly.</p>
      </div>
      <div class="mode-card" onclick="pickMode(2)" id="m2">
        <h3>&#128246; STA Mode <span class="badge badge-blue">Router</span></h3>
        <p>Connects to hospital WiFi. Broader network coverage.</p>
      </div>
      <div class="mode-card" onclick="pickMode(3)" id="m3">
        <h3>&#128260; AP + STA <span class="badge badge-green">Hybrid</span></h3>
        <p>Local AP for slaves + router connection. Recommended setup.</p>
      </div>
      <div class="mode-card" onclick="pickMode(4)" id="m4">
        <h3>&#9729;&#65039; Online Mode <span class="badge badge-purple">Cloud</span></h3>
        <p>AP + STA + Cloud sync. Monitor from anywhere in the world.</p>
      </div>
    </div>
    <button class="btn" id="nextBtn" onclick="goStep2()" disabled>Continue</button>
  </div>

  <div id="stepAP" class="step">
    <h2 style="font-size:16px;text-align:center;margin-bottom:18px;font-weight:700">Configure Access Point</h2>
    <div class="fg"><label>AP Network Name (SSID)</label><input id="ap-ssid" value="HospitalAlarm" placeholder="e.g. HospitalAlarm"></div>
    <div class="fg"><label>AP Password (optional, 8+ chars for WPA2)</label><input id="ap-pass" type="password" placeholder="Leave empty for open network"></div>
    <button class="btn" onclick="goStepWiFi()">Continue</button>
    <button class="btn btn-ghost" onclick="showStep('step1')">Back</button>
  </div>

  <div id="stepWiFi" class="step">
    <h2 style="font-size:16px;text-align:center;margin-bottom:14px;font-weight:700">Connect to Network</h2>
    <div id="scan-list" class="wl"><div class="scanning"><div class="spinner"></div>Scanning...</div></div>
    <div class="fg"><label>Password</label><input type="password" id="wifi-pass" placeholder="Network password"></div>
    <div id="setup-status"></div>
    <button class="btn" id="connectBtn" onclick="doConnect()" disabled>Connect &amp; Finish</button>
    <button class="btn btn-ghost" onclick="scanWiFi()">Rescan Networks</button>
    <button class="btn btn-ghost" onclick="showStep('stepAP')">Back</button>
  </div>

  <div id="stepDone" class="step">
    <div style="text-align:center;padding:24px 0">
      <div style="font-size:56px;margin-bottom:16px">&#9989;</div>
      <h2 style="font-size:22px;font-weight:800;margin-bottom:8px">Setup Complete</h2>
      <p id="done-info" style="color:var(--s400);font-size:14px;line-height:1.6"></p>
      <button class="btn" style="margin-top:24px" onclick="location.href='/'">Open Dashboard</button>
    </div>
  </div>
</div>

<script>
let selMode=0,selSSID='';
function pickMode(m){selMode=m;document.querySelectorAll('.mode-card').forEach(c=>c.classList.remove('selected'));document.getElementById('m'+m).classList.add('selected');document.getElementById('nextBtn').disabled=false}
function showStep(s){document.querySelectorAll('.step').forEach(e=>e.classList.remove('active'));document.getElementById(s).classList.add('active')}
function goStep2(){showStep('stepAP')}
function goStepWiFi(){if(selMode===1){finishSetup();return}showStep('stepWiFi');scanWiFi()}
function scanWiFi(){const l=document.getElementById('scan-list');l.innerHTML='<div class="scanning"><div class="spinner"></div>Scanning...</div>';fetch('/api/scan').then(r=>r.json()).then(nets=>{let h='';nets.forEach(n=>{h+=`<div class="wi" onclick="pickWiFi(this,'${n.ssid}')"><span>${n.ssid}</span><span class="wi-rssi">${n.rssi}dBm</span></div>`});l.innerHTML=h||'<div class="scanning">No networks found</div>'})}
function pickWiFi(el,ssid){selSSID=ssid;document.querySelectorAll('.wi').forEach(i=>i.classList.remove('selected'));el.classList.add('selected');document.getElementById('connectBtn').disabled=false}
function doConnect(){
  const p=document.getElementById('wifi-pass').value, s=document.getElementById('setup-status');
  s.innerHTML='<div class="status-msg info"><div class="spinner"></div> Connecting to '+selSSID+'...</div>';
  fetch('/api/connect-wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:selSSID,password:p})})
    .then(r=>r.json())
    .then(d=>{
      if(d.success){
        s.innerHTML='<div class="status-msg ok">Connected! Applying offline settings...</div>';
        setTimeout(finishSetup, 1000);
      } else {
        s.innerHTML='<div class="status-msg error">Failed: '+d.message+'</div>';
      }
    });
}
function finishSetup(){
  const as=document.getElementById('ap-ssid').value||'HospitalAlarm', ap=document.getElementById('ap-pass').value;
  document.getElementById('setup-status').innerHTML='<div class="status-msg info"><div class="spinner"></div> Starting Dashboard... Please wait.</div>';
  
  // Send the request, but don't wait for the response because the ESP32 resets its AP interface!
  fetch('/api/setup',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:selMode,apSSID:as,apPass:ap})}).catch(()=>console.log("Expected disconnect"));
  
  // Force a redirect after 4 seconds to allow the AP to restart and the phone to reconnect
  setTimeout(() => {
    window.location.href = '/';
  }, 4000);
}
</script>
</body>
</html>
)rawliteral";

// ─── MAIN DASHBOARD (SPA with WebSocket) ─────────────────────
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Dashboard — Hospital Alarm System</title>
<style>
:root{--bg:#020617;--surface:rgba(15,23,42,.7);--primary:#6366f1;--primary-g:rgba(99,102,241,.35);--emerald:#10b981;--emerald-g:rgba(16,185,129,.3);--crimson:#ef4444;--crimson-g:rgba(239,68,68,.35);--amber:#f59e0b;--amber-g:rgba(245,158,11,.25);--s100:#f1f5f9;--s200:#e2e8f0;--s400:#94a3b8;--s500:#64748b;--s600:#475569;--glass:rgba(255,255,255,.05);--glass2:rgba(255,255,255,.08)}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:var(--bg);color:var(--s200);min-height:100vh;display:flex;flex-direction:column}
body::before{content:'';position:fixed;top:0;left:0;right:0;bottom:0;background:radial-gradient(ellipse at 15% 5%,rgba(99,102,241,.06) 0%,transparent 50%),radial-gradient(ellipse at 85% 90%,rgba(16,185,129,.04) 0%,transparent 50%);pointer-events:none;z-index:0}

/* ── Header ── */
header{background:rgba(2,6,23,.85);backdrop-filter:blur(16px);border-bottom:1px solid var(--glass);padding:16px 32px;display:flex;justify-content:space-between;align-items:center;position:sticky;top:0;z-index:100}
.logo{display:flex;align-items:center;gap:14px}
.logo-icon{width:44px;height:44px;background:linear-gradient(135deg,var(--primary),#8b5cf6);border-radius:13px;display:flex;align-items:center;justify-content:center;font-size:22px;box-shadow:0 8px 24px var(--primary-g)}
.logo h1{font-size:17px;font-weight:800;color:#fff}
.logo p{font-size:11px;color:var(--s500);text-transform:uppercase;letter-spacing:1.2px;font-weight:600}
.hdr-right{display:flex;align-items:center;gap:12px}
.ws-dot{width:8px;height:8px;border-radius:50%;background:var(--s600)}
.ws-dot.on{background:var(--emerald);box-shadow:0 0 8px var(--emerald-g)}
.hdr-btn{padding:8px 16px;border-radius:10px;border:1px solid var(--glass2);background:rgba(255,255,255,.03);color:var(--s400);font-size:12px;font-weight:600;cursor:pointer;transition:all .2s;text-decoration:none}
.hdr-btn:hover{background:rgba(255,255,255,.06);color:var(--s200)}

/* ── Stats Bar ── */
.stats{display:flex;gap:16px;padding:24px 32px;position:relative;z-index:1}
.stat{flex:1;background:var(--surface);backdrop-filter:blur(12px);border:1px solid var(--glass);border-radius:16px;padding:18px 20px;display:flex;align-items:center;gap:14px}
.stat-icon{width:44px;height:44px;border-radius:12px;display:flex;align-items:center;justify-content:center;font-size:20px}
.stat-icon.blue{background:rgba(99,102,241,.12);color:#818cf8}
.stat-icon.green{background:rgba(16,185,129,.12);color:#34d399}
.stat-icon.red{background:rgba(239,68,68,.12);color:#f87171}
.stat-icon.amber{background:rgba(245,158,11,.12);color:#fbbf24}
.stat-val{font-size:26px;font-weight:800;color:#fff}
.stat-label{font-size:11px;color:var(--s500);text-transform:uppercase;letter-spacing:.8px;font-weight:600}

/* ── Main Content ── */
main{padding:0 32px 40px;flex:1;position:relative;z-index:1}
.section{margin-bottom:32px}
.section-hdr{display:flex;align-items:center;gap:10px;margin-bottom:16px;font-size:13px;font-weight:700;color:var(--s400);text-transform:uppercase;letter-spacing:1px}

/* ── Pending Cards ── */
.pending{background:rgba(245,158,11,.04);border:1px dashed rgba(245,158,11,.2);border-radius:16px;padding:18px 22px;display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;transition:all .3s}
.pending:hover{border-color:rgba(245,158,11,.4);background:rgba(245,158,11,.06)}
.p-id{font-family:'Cascadia Code','Fira Code',monospace;font-size:15px;font-weight:700;color:var(--amber)}
.p-sub{font-size:12px;color:var(--s500);margin-top:3px}

/* ── Device Grid ── */
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(320px,1fr));gap:20px}

/* ── Device Card ── */
.card{background:var(--surface);border:1px solid var(--glass);border-radius:22px;padding:22px;position:relative;overflow:hidden;transition:all .4s;backdrop-filter:blur(12px)}
.card:hover{border-color:rgba(99,102,241,.2);transform:translateY(-3px)}
.card.alerting{border-color:var(--crimson);background:rgba(239,68,68,.04)}
.card.alerting::before{content:'';position:absolute;inset:0;background:rgba(239,68,68,.06);animation:pulse 2s infinite;pointer-events:none}
@keyframes pulse{0%,100%{opacity:.2}50%{opacity:1}}
.card.offline{opacity:.6}
.card.offline:hover{opacity:.8}

.c-top{display:flex;justify-content:space-between;margin-bottom:20px}
.c-name{font-size:18px;font-weight:800;color:#fff}
.c-id{font-size:11px;color:var(--s500);margin-top:3px;font-family:monospace;display:flex;align-items:center;gap:5px}
.c-icon{width:52px;height:52px;border-radius:15px;display:flex;align-items:center;justify-content:center;font-size:22px;transition:all .4s;flex-shrink:0}
.card.online .c-icon{background:rgba(16,185,129,.1);border:1px solid rgba(16,185,129,.15)}
.card.offline .c-icon{background:var(--glass);border:1px solid var(--glass);opacity:.5}
.card.alerting .c-icon{background:var(--crimson);box-shadow:0 8px 24px var(--crimson-g);animation:bounce 1.5s infinite}
@keyframes bounce{0%,100%{transform:scale(1)}50%{transform:scale(1.08) translateY(-3px)}}

.c-fields{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.c-field{background:rgba(255,255,255,.02);border:1px solid var(--glass);border-radius:11px;padding:11px 14px}
.c-field-label{font-size:10px;color:var(--s500);margin-bottom:3px;text-transform:uppercase;letter-spacing:.6px;font-weight:600;display:flex;align-items:center;gap:5px}
.c-field-val{font-size:15px;font-weight:700;color:#fff}

.c-footer{margin-top:16px;padding-top:14px;border-top:1px solid var(--glass);display:flex;justify-content:space-between;align-items:center;font-size:10px;font-weight:700;text-transform:uppercase;letter-spacing:1px;color:var(--s500)}
.dot{width:7px;height:7px;border-radius:50%;display:inline-block;margin-right:6px}
.card.online .dot{background:var(--emerald);box-shadow:0 0 8px var(--emerald-g)}
.card.alerting .dot{background:var(--crimson);box-shadow:0 0 8px var(--crimson-g)}
.card.offline .dot{background:var(--s600)}

.c-actions{display:flex;gap:6px;margin-top:12px}
.c-btn{padding:7px 14px;border-radius:8px;border:none;font-size:11px;font-weight:700;cursor:pointer;transition:all .2s;text-transform:uppercase;letter-spacing:.5px}
.c-btn:hover{transform:translateY(-1px)}
.c-btn-clear{background:rgba(239,68,68,.1);color:#f87171;border:1px solid rgba(239,68,68,.15)}
.c-btn-clear:hover{background:rgba(239,68,68,.2)}

/* ── Buttons ── */
.btn{padding:12px 22px;border-radius:12px;border:none;font-weight:700;cursor:pointer;color:#fff;transition:all .2s;font-size:13px}
.btn:hover{transform:translateY(-2px)}
.btn-approve{background:linear-gradient(135deg,var(--emerald),#059669);box-shadow:0 6px 18px var(--emerald-g)}
.btn-delete{background:rgba(239,68,68,.1);color:var(--crimson);border:1px solid rgba(239,68,68,.15)}

/* ── Modal ── */
.modal-bg{position:fixed;inset:0;background:rgba(0,0,0,.75);backdrop-filter:blur(10px);z-index:999;display:none;align-items:center;justify-content:center}
.modal{background:rgba(15,23,42,.95);border:1px solid var(--glass2);border-radius:24px;padding:36px;width:100%;max-width:420px;box-shadow:0 30px 60px rgba(0,0,0,.5)}
.modal h2{font-size:20px;font-weight:800;margin-bottom:24px}
.m-input{width:100%;padding:13px 16px;background:rgba(0,0,0,.3);border:1px solid var(--glass2);border-radius:12px;color:#fff;margin-bottom:14px;outline:none;font-size:14px;font-family:inherit;transition:border .3s}
.m-input:focus{border-color:var(--primary)}
.m-label{display:block;font-size:11px;color:var(--s400);margin-bottom:6px;font-weight:600;text-transform:uppercase;letter-spacing:.5px}
.m-row{display:flex;gap:12px}.m-row>div{flex:1}
.m-actions{display:flex;gap:10px;margin-top:8px}
.m-actions .btn{flex:1}

/* ── Empty State ── */
.empty{grid-column:1/-1;text-align:center;padding:80px 20px;color:var(--s500)}
.empty-icon{font-size:56px;margin-bottom:16px;opacity:.4}
.empty-text{font-size:14px;line-height:1.6}

/* ── Responsive ── */
@media(max-width:768px){
  header{padding:14px 18px}
  .stats{padding:16px 18px;flex-wrap:wrap;gap:10px}
  .stat{min-width:calc(50% - 5px);flex:unset}
  main{padding:0 18px 30px}
  .grid{grid-template-columns:1fr}
}
</style>
</head>
<body>
<header>
  <div class="logo">
    <div class="logo-icon">&#128276;</div>
    <div>
      <h1>Hospital Alarm</h1>
      <p id="mode-label">Initializing...</p>
    </div>
  </div>
  <div class="hdr-right">
    <div class="ws-dot" id="ws-indicator" title="WebSocket"></div>
    <a href="/setup" class="hdr-btn">&#9881; Setup</a>
    <a href="/admin" class="hdr-btn">&#128274; Admin</a>
  </div>
</header>

<div class="stats">
  <div class="stat"><div class="stat-icon blue">&#128225;</div><div><div class="stat-val" id="s-total">0</div><div class="stat-label">Total Devices</div></div></div>
  <div class="stat"><div class="stat-icon green">&#128994;</div><div><div class="stat-val" id="s-online">0</div><div class="stat-label">Online</div></div></div>
  <div class="stat"><div class="stat-icon red">&#128680;</div><div><div class="stat-val" id="s-alerts">0</div><div class="stat-label">Active Alerts</div></div></div>
  <div class="stat"><div class="stat-icon amber">&#9203;</div><div><div class="stat-val" id="s-pending">0</div><div class="stat-label">Pending</div></div></div>
</div>

<main>
  <div class="section" id="pending-section" style="display:none">
    <div class="section-hdr">&#9888;&#65039; Awaiting Approval</div>
    <div id="pending-list"></div>
  </div>

  <div class="section">
    <div class="section-hdr">&#128421; Monitoring Units</div>
    <div class="grid" id="device-grid"></div>
  </div>
</main>

<!-- Approval Modal -->
<div class="modal-bg" id="modal">
  <div class="modal">
    <h2 id="modal-title">Approve Device</h2>
    <div><div class="m-label">Patient / Device Name</div><input class="m-input" id="m-name" placeholder="e.g. John Doe"></div>
    <div class="m-row">
      <div><div class="m-label">Bed</div><input class="m-input" id="m-bed" placeholder="e.g. B-12"></div>
      <div><div class="m-label">Room</div><input class="m-input" id="m-room" placeholder="e.g. ICU-3"></div>
    </div>
    <div class="m-actions">
      <button class="btn btn-approve" onclick="submitModal()">Approve</button>
      <button class="btn" style="background:var(--glass2);color:var(--s400)" onclick="closeModal()">Cancel</button>
    </div>
  </div>
</div>

<script>
/* ── State ── */
let devices={},ws=null,modalId=null;
const AUTH='admin1234';

/* ── WebSocket ── */
function connectWS(){
  const proto=location.protocol==='https:'?'wss:':'ws:';
  ws=new WebSocket(proto+'//'+location.host+'/ws');
  ws.onopen=()=>{document.getElementById('ws-indicator').classList.add('on');console.log('[WS] Connected')};
  ws.onclose=()=>{document.getElementById('ws-indicator').classList.remove('on');setTimeout(connectWS,2000)};
  ws.onerror=()=>ws.close();
  ws.onmessage=(e)=>{
    try{
      const msg=JSON.parse(e.data);
      if(msg.type==='FULL_STATE'){
        devices={};
        if(Array.isArray(msg.devices))msg.devices.forEach(d=>devices[d.slaveId]=d);
        render();
      }else if(msg.type==='UPDATE'&&msg.device){
        devices[msg.device.slaveId]=msg.device;
        render();
      }else if(msg.type==='DELETE'&&msg.slaveId){
        delete devices[msg.slaveId];
        render();
      }
    }catch(err){console.error('[WS] Parse error:',err)}
  };
}
connectWS();

/* ── Fallback: initial fetch ── */
fetch('/api/slaves?all=1').then(r=>r.json()).then(list=>{
  if(Array.isArray(list))list.forEach(d=>devices[d.slaveId]=d);
  render();
});
fetch('/api/status').then(r=>r.json()).then(s=>{
  const modes=['','AP Mode','STA Mode','AP+STA Hybrid','Online (Cloud)'];
  document.getElementById('mode-label').textContent=modes[s.mode]||'Setup Required';
});

/* ── Render UI ── */
function render(){
  const all=Object.values(devices);
  const approved=all.filter(d=>d.approved);
  const pending=all.filter(d=>!d.approved);
  const online=all.filter(d=>d.online);
  const alerts=all.filter(d=>d.alertActive);

  document.getElementById('s-total').textContent=approved.length;
  document.getElementById('s-online').textContent=online.length;
  document.getElementById('s-alerts').textContent=alerts.length;
  document.getElementById('s-pending').textContent=pending.length;

  // Pending section
  const ps=document.getElementById('pending-section');
  const pl=document.getElementById('pending-list');
  if(pending.length>0){
    ps.style.display='block';
    pl.innerHTML=pending.map(d=>`
      <div class="pending">
        <div><div class="p-id">${d.slaveId}</div><div class="p-sub">New device requesting access</div></div>
        <button class="btn btn-approve" onclick="openModal('${d.slaveId}')">Approve</button>
      </div>`).join('');
  }else{ps.style.display='none';pl.innerHTML=''}

  // Device grid
  const grid=document.getElementById('device-grid');
  if(approved.length===0){
    grid.innerHTML='<div class="empty"><div class="empty-icon">&#128225;</div><div class="empty-text">No active units.<br>Connect a slave device to begin monitoring.</div></div>';
    return;
  }
  grid.innerHTML=approved.map(d=>{
    const cls=d.alertActive?'alerting':(d.online?'online':'offline');
    const icon=d.alertActive?'&#128680;':(d.online?'&#128154;':'&#9888;&#65039;');
    const status=d.alertActive?'ALERT ACTIVE':(d.online?'SYSTEM STABLE':'LINK DOWN');
    const time=d.lastAlertTime?formatTime(d.lastAlertTime):'No Events';
    return `
      <div class="card ${cls}">
        <div class="c-top">
          <div><div class="c-name">${d.patientName||'Unnamed'}</div><div class="c-id">&#128274; ${d.slaveId}</div></div>
          <div class="c-icon">${icon}</div>
        </div>
        <div class="c-fields">
          <div class="c-field"><div class="c-field-label">&#128716; Bed</div><div class="c-field-val">${d.bed||'-'}</div></div>
          <div class="c-field"><div class="c-field-label">&#128682; Room</div><div class="c-field-val">${d.room||'-'}</div></div>
        </div>
        ${d.alertActive?`<div class="c-actions"><button class="c-btn c-btn-clear" onclick="clearAlert('${d.slaveId}')">&#10006; Clear Alert</button></div>`:''}
        <div class="c-footer"><div><span class="dot"></span>${status}</div><div>&#128336; ${time}</div></div>
      </div>`;
  }).join('');
}

function formatTime(ms){
  if(!ms||ms===0)return'Never';
  const s=Math.floor((Date.now()-ms)/1000);
  if(s<5)return'Just now';
  if(s<60)return s+'s ago';
  if(s<3600)return Math.floor(s/60)+'m ago';
  return Math.floor(s/3600)+'h ago';
}

/* ── Actions ── */
function openModal(id){modalId=id;document.getElementById('modal').style.display='flex';document.getElementById('m-name').value='';document.getElementById('m-bed').value='';document.getElementById('m-room').value='';document.getElementById('m-name').focus()}
function closeModal(){document.getElementById('modal').style.display='none'}
function submitModal(){
  const n=document.getElementById('m-name').value,b=document.getElementById('m-bed').value,r=document.getElementById('m-room').value;
  if(!n||!b||!r){alert('All fields required');return}
  fetch('/api/approve/'+modalId,{method:'POST',headers:{'Content-Type':'application/json','X-Auth-Token':AUTH},body:JSON.stringify({patientName:n,bed:b,room:r})}).then(()=>{closeModal();fetch('/api/slaves?all=1').then(r=>r.json()).then(list=>{devices={};list.forEach(d=>devices[d.slaveId]=d);render()})});
}
function clearAlert(id){
  fetch('/api/clearAlert/'+id,{method:'POST',headers:{'Content-Type':'application/json','X-Auth-Token':AUTH},body:'{}'}).then(()=>{if(devices[id])devices[id].alertActive=false;render()});
}

/* ── Keep time displays updated ── */
setInterval(render,15000);
</script>
</body>
</html>
)rawliteral";

// ─── ADMIN PAGE ──────────────────────────────────────────────
const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Admin — Hospital Alarm</title>
<style>
:root{--bg:#0a0e1a;--surface:rgba(15,23,42,.85);--primary:#6366f1;--emerald:#10b981;--crimson:#ef4444;--amber:#f59e0b;--s200:#e2e8f0;--s400:#94a3b8;--s600:#475569;--glass:rgba(255,255,255,.06)}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:var(--bg);color:var(--s200);min-height:100vh;padding:24px;background-image:radial-gradient(ellipse at 20% 50%,rgba(99,102,241,.06) 0%,transparent 50%)}
.container{max-width:860px;margin:0 auto}
header{display:flex;justify-content:space-between;align-items:center;margin-bottom:32px}
header h1{font-size:22px;font-weight:800;color:#fff}
header a{color:var(--primary);text-decoration:none;font-size:14px;font-weight:600}
.section-title{font-size:12px;text-transform:uppercase;letter-spacing:1.2px;color:var(--s400);font-weight:700;margin-bottom:14px;margin-top:28px;padding-bottom:8px;border-bottom:1px solid var(--glass)}
.dev{background:var(--surface);border:1px solid var(--glass);border-radius:16px;padding:18px 22px;margin-bottom:10px;display:flex;align-items:center;justify-content:space-between;backdrop-filter:blur(12px);transition:all .2s}
.dev:hover{border-color:rgba(99,102,241,.2);transform:translateY(-1px)}
.dev.pending{border-left:3px solid var(--amber)}
.dev-id{font-family:'Cascadia Code','Fira Code',monospace;font-weight:700;color:var(--primary);font-size:14px}
.dev-meta{font-size:12px;color:var(--s400);margin-top:2px}
.dev-actions{display:flex;gap:8px}
.btn{padding:8px 16px;border-radius:10px;border:none;font-weight:700;cursor:pointer;font-size:12px;transition:all .2s}
.btn:hover{transform:translateY(-1px)}
.btn-approve{background:var(--emerald);color:#fff}
.btn-edit{background:rgba(99,102,241,.1);color:var(--primary);border:1px solid rgba(99,102,241,.15)}
.btn-del{background:rgba(239,68,68,.08);color:var(--crimson);border:1px solid rgba(239,68,68,.12)}
.modal-bg{position:fixed;inset:0;background:rgba(0,0,0,.75);backdrop-filter:blur(8px);display:none;align-items:center;justify-content:center;z-index:999}
.modal{background:rgba(15,23,42,.95);border:1px solid var(--glass);border-radius:24px;padding:32px;width:100%;max-width:400px}
.modal h2{margin-bottom:20px;font-size:18px;font-weight:800}
.m-input{width:100%;padding:12px;background:rgba(0,0,0,.25);border:1px solid var(--glass);border-radius:10px;color:#fff;margin-bottom:12px;outline:none;font-size:14px;font-family:inherit}
.m-input:focus{border-color:var(--primary)}
.m-label{display:block;font-size:11px;color:var(--s400);margin-bottom:5px;font-weight:600}
.empty{padding:24px;text-align:center;color:var(--s600);font-size:13px}
</style>
</head>
<body>
<div class="container">
  <header>
    <h1>&#128736; Admin Panel</h1>
    <a href="/">&larr; Dashboard</a>
  </header>
  <div class="section-title">&#9888;&#65039; Pending Approval</div>
  <div id="pending"></div>
  <div class="section-title">&#9989; Approved Devices</div>
  <div id="approved"></div>
</div>

<div class="modal-bg" id="modal">
  <div class="modal">
    <h2 id="modal-title">Device Details</h2>
    <div class="m-label">Patient Name</div><input class="m-input" id="m-name">
    <div class="m-label">Bed</div><input class="m-input" id="m-bed">
    <div class="m-label">Room</div><input class="m-input" id="m-room">
    <div style="display:flex;gap:10px;margin-top:10px">
      <button class="btn btn-approve" style="flex:1" onclick="submitModal()">Save</button>
      <button class="btn" style="flex:1;background:var(--glass);color:var(--s400)" onclick="closeModal()">Cancel</button>
    </div>
  </div>
</div>

<script>
let editId=null,editMode='approve';
const AUTH='admin1234';
function refresh(){
  fetch('/api/slaves?all=1').then(r=>r.json()).then(devs=>{
    const pen=document.getElementById('pending'),app=document.getElementById('approved');
    let hp='',ha='';
    devs.forEach(d=>{
      if(!d.approved){
        hp+=`<div class="dev pending"><div><div class="dev-id">${d.slaveId}</div><div class="dev-meta">New device detected</div></div><div class="dev-actions"><button class="btn btn-approve" onclick="openApprove('${d.slaveId}')">Approve</button><button class="btn btn-del" onclick="delDev('${d.slaveId}')">Delete</button></div></div>`;
      }else{
        ha+=`<div class="dev"><div><div class="dev-id">${d.slaveId}</div><div class="dev-meta">${d.patientName} &middot; Bed ${d.bed} &middot; ${d.room}</div></div><div class="dev-actions"><button class="btn btn-edit" onclick="openEdit('${d.slaveId}','${esc(d.patientName)}','${esc(d.bed)}','${esc(d.room)}')">Edit</button><button class="btn btn-del" onclick="delDev('${d.slaveId}')">Delete</button></div></div>`;
      }
    });
    pen.innerHTML=hp||'<div class="empty">No pending devices</div>';
    app.innerHTML=ha||'<div class="empty">No approved devices</div>';
  });
}
function esc(s){return(s||'').replace(/'/g,"\\'")}
function openApprove(id){editId=id;editMode='approve';document.getElementById('modal-title').textContent='Approve: '+id;document.getElementById('m-name').value='';document.getElementById('m-bed').value='';document.getElementById('m-room').value='';document.getElementById('modal').style.display='flex'}
function openEdit(id,n,b,r){editId=id;editMode='edit';document.getElementById('modal-title').textContent='Edit: '+id;document.getElementById('m-name').value=n;document.getElementById('m-bed').value=b;document.getElementById('m-room').value=r;document.getElementById('modal').style.display='flex'}
function closeModal(){document.getElementById('modal').style.display='none'}
function submitModal(){
  const n=document.getElementById('m-name').value,b=document.getElementById('m-bed').value,r=document.getElementById('m-room').value;
  const url=editMode==='approve'?'/api/approve/'+editId:'/api/slaves/'+editId;
  const method=editMode==='approve'?'POST':'PUT';
  fetch(url,{method,headers:{'Content-Type':'application/json','X-Auth-Token':AUTH},body:JSON.stringify({patientName:n,bed:b,room:r})}).then(()=>{closeModal();refresh()});
}
function delDev(id){if(!confirm('Delete '+id+'?'))return;fetch('/api/slaves/'+id,{method:'DELETE',headers:{'X-Auth-Token':AUTH}}).then(()=>refresh())}
refresh();
</script>
</body>
</html>
)rawliteral";

#endif // DASHBOARD_H
