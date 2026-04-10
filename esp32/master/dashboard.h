/*
 * ══════════════════════════════════════════════════════════════
 *  Embedded Dashboard — Unified Design System
 *  Matches the hosted Railway admin dashboard UI
 *  Served from ESP32 flash via PROGMEM
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
:root{--bg:#070b14;--surface:rgba(15,23,42,.4);--emerald:#10b981;--emerald-g:rgba(16,185,129,.25);--red:#ef4444;--amber:#f59e0b;--white:#fff;--s100:#f1f5f9;--s400:#94a3b8;--s500:#64748b;--glass:rgba(255,255,255,.05);--glass2:rgba(255,255,255,.1)}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:var(--bg);color:var(--s100);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
body::before{content:'';position:fixed;inset:0;background:radial-gradient(circle at 50% 50%,rgba(16,185,129,.05) 0%,transparent 50%);pointer-events:none}
.card{background:rgba(15,23,42,.8);backdrop-filter:blur(20px);border:1px solid var(--glass2);border-radius:24px;padding:40px 32px;width:100%;max-width:460px;position:relative}
.card::before{content:'';position:absolute;inset:-1px;background:linear-gradient(135deg,rgba(16,185,129,.15),transparent 50%);border-radius:24px;z-index:-1}
.logo{text-align:center;margin-bottom:32px}
.logo-icon{width:64px;height:64px;background:rgba(16,185,129,.15);border:1px solid rgba(16,185,129,.2);border-radius:18px;display:inline-flex;align-items:center;justify-content:center;margin-bottom:12px}
.logo-icon svg{width:32px;height:32px;color:var(--emerald)}
.logo h1{font-size:24px;font-weight:800;color:var(--white);letter-spacing:-.5px}
.logo p{font-size:11px;color:rgba(16,185,129,.6);margin-top:4px;font-weight:700;text-transform:uppercase;letter-spacing:2px}
.step{display:none;animation:fadeUp .3s ease}.step.active{display:block}
@keyframes fadeUp{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:translateY(0)}}
.modes{display:flex;flex-direction:column;gap:8px;margin:16px 0}
.mode-card{background:rgba(255,255,255,.02);border:1px solid var(--glass2);border-radius:14px;padding:16px 18px;cursor:pointer;transition:all .2s}
.mode-card:hover{border-color:rgba(16,185,129,.2);background:rgba(16,185,129,.03)}
.mode-card.selected{border-color:var(--emerald);background:rgba(16,185,129,.08)}
.mode-card h3{font-size:14px;display:flex;align-items:center;gap:8px;margin-bottom:2px;font-weight:700;color:var(--white)}
.mode-card p{font-size:11px;color:var(--s400);line-height:1.5}
.badge{font-size:9px;padding:2px 8px;border-radius:12px;font-weight:700;text-transform:uppercase;letter-spacing:.8px}
.badge-local{background:rgba(255,255,255,.05);color:var(--s400)}
.badge-hybrid{background:rgba(16,185,129,.1);color:#34d399}
.badge-cloud{background:rgba(168,85,247,.1);color:#c084fc}
.btn{width:100%;padding:13px;border:none;border-radius:12px;font-size:13px;font-weight:700;cursor:pointer;transition:all .2s;margin-top:8px;background:var(--emerald);color:var(--white)}
.btn:hover{box-shadow:0 6px 20px var(--emerald-g)}
.btn:disabled{opacity:.3;cursor:not-allowed;box-shadow:none}
.btn-ghost{background:rgba(255,255,255,.04);color:var(--s400);border:1px solid var(--glass2)}
.btn-ghost:hover{background:rgba(255,255,255,.06)}
.fg{margin-bottom:12px}
.fg label{display:block;font-size:10px;color:var(--s500);margin-bottom:5px;font-weight:700;text-transform:uppercase;letter-spacing:.8px}
.fg input{width:100%;padding:11px 14px;background:rgba(255,255,255,.04);border:1px solid var(--glass2);border-radius:10px;color:var(--white);outline:none;font-size:13px;transition:border .2s}
.fg input:focus{border-color:var(--emerald)}
.wl{max-height:180px;overflow-y:auto;border-radius:12px;border:1px solid var(--glass2);margin-bottom:12px}
.wl::-webkit-scrollbar{width:3px}.wl::-webkit-scrollbar-thumb{background:var(--s500);border-radius:3px}
.wi{padding:12px 14px;border-bottom:1px solid var(--glass);cursor:pointer;display:flex;justify-content:space-between;align-items:center;transition:all .15s}
.wi:last-child{border-bottom:none}
.wi:hover{background:rgba(255,255,255,.02)}
.wi.selected{background:rgba(16,185,129,.06);border-left:3px solid var(--emerald)}
.wi-rssi{font-size:10px;color:var(--s500);font-family:monospace}
.scanning{text-align:center;padding:20px;color:var(--s400);font-size:12px}
.spinner{width:16px;height:16px;border:2px solid var(--glass2);border-top-color:var(--emerald);border-radius:50%;animation:spin .7s linear infinite;display:inline-block;vertical-align:middle;margin-right:6px}
@keyframes spin{to{transform:rotate(360deg)}}
.status-msg{padding:10px 14px;border-radius:10px;text-align:center;font-size:12px;margin-top:10px;font-weight:600}
.status-msg.info{background:rgba(16,185,129,.06);color:#34d399;border:1px solid rgba(16,185,129,.12)}
.status-msg.error{background:rgba(239,68,68,.06);color:#f87171;border:1px solid rgba(239,68,68,.12)}
.status-msg.ok{background:rgba(16,185,129,.1);color:#34d399;border:1px solid rgba(16,185,129,.2)}
</style>
</head>
<body>
<div class="card">
  <div class="logo">
    <div class="logo-icon"><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg></div>
    <h1>Hospital Alarm</h1>
    <p>System Configuration</p>
  </div>

  <div id="step1" class="step active">
    <h2 style="font-size:15px;text-align:center;margin-bottom:14px;font-weight:700;color:#fff">Select Operating Mode</h2>
    <div class="modes">
      <div class="mode-card" onclick="pickMode(1)" id="m1">
        <h3>&#128225; AP Mode <span class="badge badge-local">Local</span></h3>
        <p>Standalone network. No router needed.</p>
      </div>
      <div class="mode-card" onclick="pickMode(2)" id="m2">
        <h3>&#128246; STA Mode <span class="badge badge-local">Router</span></h3>
        <p>Connects to hospital WiFi router.</p>
      </div>
      <div class="mode-card" onclick="pickMode(3)" id="m3">
        <h3>&#128260; AP + STA <span class="badge badge-hybrid">Hybrid</span></h3>
        <p>Local AP for slaves + router. Recommended.</p>
      </div>
      <div class="mode-card" onclick="pickMode(4)" id="m4">
        <h3>&#9729;&#65039; Online <span class="badge badge-cloud">Cloud</span></h3>
        <p>AP + STA + Cloud sync. Monitor anywhere.</p>
      </div>
    </div>
    <button class="btn" id="nextBtn" onclick="goStep2()" disabled>Continue</button>
  </div>

  <div id="stepAP" class="step">
    <h2 style="font-size:15px;text-align:center;margin-bottom:14px;font-weight:700;color:#fff">Configure Access Point</h2>
    <div class="fg"><label>AP Network Name (SSID)</label><input id="ap-ssid" value="HospitalAlarm" placeholder="e.g. HospitalAlarm"></div>
    <div class="fg"><label>AP Password (optional, 8+ chars)</label><input id="ap-pass" type="password" placeholder="Leave empty for open"></div>
    <button class="btn" onclick="goStepWiFi()">Continue</button>
    <button class="btn btn-ghost" onclick="showStep('step1')">Back</button>
  </div>

  <div id="stepWiFi" class="step">
    <h2 style="font-size:15px;text-align:center;margin-bottom:12px;font-weight:700;color:#fff">Connect to Network</h2>
    <div id="scan-list" class="wl"><div class="scanning"><div class="spinner"></div>Scanning...</div></div>
    <div class="fg"><label>Password</label><input type="password" id="wifi-pass" placeholder="Network password"></div>
    <div id="setup-status"></div>
    <button class="btn" id="connectBtn" onclick="doConnect()" disabled>Connect &amp; Finish</button>
    <button class="btn btn-ghost" onclick="scanWiFi()">Rescan Networks</button>
    <button class="btn btn-ghost" onclick="showStep('stepAP')">Back</button>
  </div>

  <div id="stepDone" class="step">
    <div style="text-align:center;padding:20px 0">
      <div style="font-size:48px;margin-bottom:12px">&#9989;</div>
      <h2 style="font-size:20px;font-weight:800;margin-bottom:6px;color:#fff">Setup Complete</h2>
      <p id="done-info" style="color:var(--s400);font-size:13px;line-height:1.5"></p>
      <button class="btn" style="margin-top:20px" onclick="location.href='/'">Open Dashboard</button>
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
      if(!d.success){s.innerHTML='<div class="status-msg error">'+d.message+'</div>';return;}
      let a=0, i=setInterval(()=>{
        fetch('/api/wifi-status').then(r=>r.json()).then(st=>{
          a++;
          if(st.status===3){clearInterval(i);s.innerHTML='<div class="status-msg ok">Connected successfully! Applying...</div>';setTimeout(finishSetup,1000);}
          else if(st.status===4||st.status===1||a>=15){clearInterval(i);s.innerHTML='<div class="status-msg error">Incorrect password or network unreachable.</div>';}
        }).catch(()=>{a++;if(a>=15){clearInterval(i);s.innerHTML='<div class="status-msg error">Timeout connecting.</div>';}});
      },1000);
    });
}
function finishSetup(){
  const as=document.getElementById('ap-ssid').value||'HospitalAlarm', ap=document.getElementById('ap-pass').value;
  document.getElementById('setup-status').innerHTML='<div class="status-msg info"><div class="spinner"></div> Starting Dashboard...</div>';
  fetch('/api/setup',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:selMode,apSSID:as,apPass:ap})}).catch(()=>console.log("Expected disconnect"));
  setTimeout(() => { window.location.href = '/'; }, 4000);
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
:root{--bg:#070b14;--surface:rgba(15,23,42,.4);--emerald:#10b981;--emerald-g:rgba(16,185,129,.25);--red:#ef4444;--red-g:rgba(239,68,68,.25);--amber:#f59e0b;--white:#fff;--s100:#f1f5f9;--s400:#94a3b8;--s500:#64748b;--s600:#475569;--glass:rgba(255,255,255,.05);--glass2:rgba(255,255,255,.1)}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:var(--bg);color:var(--s100);min-height:100vh;display:flex;flex-direction:column}
body::before{content:'';position:fixed;inset:0;background:radial-gradient(circle at 0% 0%,rgba(16,185,129,.05) 0%,transparent 40%);pointer-events:none;z-index:0}

/* ── Header ── */
header{background:rgba(7,11,20,.8);backdrop-filter:blur(16px);border-bottom:1px solid var(--glass);padding:16px 24px;display:flex;justify-content:space-between;align-items:center;position:sticky;top:0;z-index:100}
.logo{display:flex;align-items:center;gap:12px}
.logo-icon{width:48px;height:48px;background:rgba(15,23,42,.8);border:1px solid var(--glass2);border-radius:16px;display:flex;align-items:center;justify-content:center}
.logo-icon svg{width:24px;height:24px;color:var(--emerald)}
.logo h1{font-size:18px;font-weight:800;color:var(--white);letter-spacing:-.3px}
.logo p{font-size:10px;color:rgba(16,185,129,.5);text-transform:uppercase;letter-spacing:1.5px;font-weight:700}
.hdr-right{display:flex;align-items:center;gap:10px}
.ws-badge{padding:6px 12px;border-radius:10px;font-size:10px;font-weight:700;letter-spacing:.5px;display:flex;align-items:center;gap:6px;text-transform:uppercase}
.ws-badge.off{background:rgba(239,68,68,.08);border:1px solid rgba(239,68,68,.15);color:#f87171}
.ws-badge.on{background:rgba(16,185,129,.08);border:1px solid rgba(16,185,129,.15);color:#34d399}
.ws-dot{width:6px;height:6px;border-radius:50%;background:var(--s600)}
.ws-badge.on .ws-dot{background:var(--emerald);box-shadow:0 0 8px var(--emerald-g)}
.ws-badge.off .ws-dot{background:var(--red)}
.hdr-btn{padding:7px 14px;border-radius:10px;border:1px solid var(--glass2);background:transparent;color:var(--s400);font-size:11px;font-weight:700;cursor:pointer;transition:all .15s;text-decoration:none}
.hdr-btn:hover{background:rgba(255,255,255,.04);color:var(--white)}

/* ── Stats ── */
.stats{display:grid;grid-template-columns:repeat(4,1fr);gap:12px;padding:20px 24px;position:relative;z-index:1}
.stat{background:rgba(15,23,42,0.8);backdrop-filter:blur(16px);border:1px solid var(--glass2);border-radius:14px;padding:16px;display:flex;align-items:center;gap:12px}
.stat-icon{width:40px;height:40px;border-radius:12px;display:flex;align-items:center;justify-content:center;font-size:18px;flex-shrink:0}
.stat-icon.em{background:rgba(16,185,129,.1)}
.stat-icon.rd{background:rgba(239,68,68,.1)}
.stat-icon.am{background:rgba(245,158,11,.1)}
.stat-icon.bl{background:rgba(255,255,255,.04)}
.stat-val{font-size:22px;font-weight:800;color:var(--white)}
.stat-lbl{font-size:9px;color:var(--s500);text-transform:uppercase;letter-spacing:.8px;font-weight:700}

/* ── Main ── */
main{padding:0 24px 32px;flex:1;position:relative;z-index:1}
.sec{margin-bottom:24px}
.sec-hdr{display:flex;align-items:center;gap:8px;margin-bottom:14px;font-size:14px;font-weight:700;color:var(--white);letter-spacing:-.2px}
.sec-hdr .cnt{padding:2px 8px;border-radius:8px;background:rgba(255,255,255,.04);border:1px solid var(--glass2);font-size:10px;font-weight:700;color:var(--s400)}

/* ── Pending ── */
.pend{background:var(--surface);border:1px solid var(--glass);border-left:3px solid var(--amber);border-radius:14px;padding:14px 18px;display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;backdrop-filter:blur(12px)}
.pend:hover{border-color:rgba(245,158,11,.3)}
.pend-id{font-family:'Cascadia Code','Fira Code',monospace;font-size:14px;font-weight:700;color:var(--amber)}
.pend-sub{font-size:10px;color:var(--s500);margin-top:2px;font-weight:700;text-transform:uppercase;letter-spacing:.5px}

/* ── Device Grid ── */
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(300px,1fr));gap:14px}
.dev{background:rgba(15,23,42,0.8);border:1px solid var(--glass2);border-radius:16px;padding:20px;position:relative;overflow:hidden;backdrop-filter:blur(16px);transition:all .2s}
.dev:hover{border-color:rgba(16,185,129,.2)}
.dev.alert{border-color:rgba(239,68,68,.3);background:rgba(239,68,68,.03)}
.dev.alert::before{content:'';position:absolute;inset:0;background:rgba(239,68,68,.04);animation:pulse 2s infinite;pointer-events:none}
@keyframes pulse{0%,100%{opacity:.2}50%{opacity:.8}}
.dev.off{opacity:.5}
.dev-top{display:flex;justify-content:space-between;align-items:start;margin-bottom:14px}
.dev-name{font-size:16px;font-weight:800;color:var(--white);letter-spacing:-.2px}
.dev-sid{font-size:10px;color:var(--s500);margin-top:2px;font-weight:700;text-transform:uppercase;letter-spacing:.3px}
.dev-badge{padding:4px 10px;border-radius:8px;font-size:10px;font-weight:700;text-transform:uppercase;letter-spacing:.5px;border:none;flex-shrink:0}
.dev.on .dev-badge{background:rgba(255,255,255,.04);color:var(--s500)}
.dev.alert .dev-badge{background:var(--red);color:var(--white);animation:blink 1.5s infinite}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.6}}
.dev.off .dev-badge{background:rgba(255,255,255,.03);color:var(--s600)}
.dev-fields{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:14px}
.dev-field{background:rgba(255,255,255,.02);border:1px solid var(--glass);border-radius:10px;padding:10px 12px}
.dev-field-lbl{font-size:9px;color:var(--s500);margin-bottom:2px;text-transform:uppercase;letter-spacing:.6px;font-weight:700}
.dev-field-val{font-size:14px;font-weight:700;color:var(--white)}
.dev-foot{padding-top:12px;border-top:1px solid var(--glass);display:flex;justify-content:space-between;align-items:center;font-size:9px;font-weight:700;text-transform:uppercase;letter-spacing:1px;color:var(--s500)}
.dot{width:6px;height:6px;border-radius:50%;display:inline-block;margin-right:5px}
.dev.on .dot{background:var(--emerald);box-shadow:0 0 6px var(--emerald-g)}
.dev.alert .dot{background:var(--red);box-shadow:0 0 6px var(--red-g)}
.dev.off .dot{background:var(--s600)}
.dev-act{display:flex;gap:6px;margin-top:10px}
.dev-act button{padding:7px 14px;border-radius:8px;border:none;font-size:10px;font-weight:700;cursor:pointer;text-transform:uppercase;letter-spacing:.5px;transition:all .15s;flex:1}
.dev-act .clr-btn{background:rgba(239,68,68,.08);color:#f87171;border:1px solid rgba(239,68,68,.12)}
.dev-act .clr-btn:hover{background:rgba(239,68,68,.15)}

/* ── Buttons ── */
.btn{padding:10px 18px;border-radius:10px;border:none;font-weight:700;cursor:pointer;color:var(--white);transition:all .15s;font-size:12px}
.btn:hover{transform:translateY(-1px)}
.btn-em{background:var(--emerald);box-shadow:0 4px 14px var(--emerald-g)}
.btn-del{background:rgba(239,68,68,.06);color:var(--red);border:1px solid rgba(239,68,68,.1)}

/* ── Modal ── */
.modal-bg{position:fixed;inset:0;background:rgba(0,0,0,.7);backdrop-filter:blur(8px);z-index:999;display:none;align-items:center;justify-content:center}
.modal{background:rgba(15,23,42,.95);border:1px solid var(--glass2);border-radius:20px;padding:32px;width:100%;max-width:400px}
.modal h2{font-size:18px;font-weight:800;margin-bottom:20px;color:var(--white)}
.m-lbl{display:block;font-size:9px;color:var(--s500);margin-bottom:4px;font-weight:700;text-transform:uppercase;letter-spacing:.6px}
.m-in{width:100%;padding:11px 14px;background:rgba(255,255,255,.04);border:1px solid var(--glass2);border-radius:10px;color:var(--white);margin-bottom:12px;outline:none;font-size:13px;font-family:inherit}
.m-in:focus{border-color:var(--emerald)}
.m-row{display:flex;gap:10px}.m-row>div{flex:1}
.m-btns{display:flex;gap:8px;margin-top:6px}
.m-btns .btn{flex:1}

/* ── Empty ── */
.empty{grid-column:1/-1;text-align:center;padding:60px 20px;border:1px dashed var(--glass2);border-radius:16px}
.empty-icon{font-size:40px;margin-bottom:12px;opacity:.3}
.empty-text{font-size:13px;color:var(--s500);line-height:1.5}

/* ── Responsive ── */
@media(max-width:768px){
  header{padding:12px 16px}
  .stats{grid-template-columns:1fr 1fr;padding:14px 16px;gap:8px}
  main{padding:0 16px 24px}
  .grid{grid-template-columns:1fr}
}
</style>
</head>
<body>
<header>
  <div class="logo">
    <div class="logo-icon"><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg></div>
    <div>
      <h1>Security Command</h1>
      <p id="mode-label">Initializing...</p>
    </div>
  </div>
  <div class="hdr-right">
    <div class="ws-badge off" id="ws-indicator"><div class="ws-dot"></div><span id="ws-text">OFFLINE</span></div>
    <a href="/admin" class="hdr-btn">&#128274; Admin</a>
  </div>
</header>

<div class="stats">
  <div class="stat"><div class="stat-icon bl">&#128225;</div><div><div class="stat-val" id="s-total">0</div><div class="stat-lbl">Total</div></div></div>
  <div class="stat"><div class="stat-icon em">&#128994;</div><div><div class="stat-val" id="s-online">0</div><div class="stat-lbl">Online</div></div></div>
  <div class="stat"><div class="stat-icon rd">&#128680;</div><div><div class="stat-val" id="s-alerts">0</div><div class="stat-lbl">Alerts</div></div></div>
  <div class="stat"><div class="stat-icon am">&#9203;</div><div><div class="stat-val" id="s-pending">0</div><div class="stat-lbl">Pending</div></div></div>
</div>

<main>
  <div class="sec" id="pending-section" style="display:none">
    <div class="sec-hdr">&#9888;&#65039; Discovery Queue <span class="cnt" id="pend-cnt">0 NEW</span></div>
    <div id="pending-list"></div>
  </div>

  <div class="sec">
    <div class="sec-hdr">&#9989; Active Infrastructure <span class="cnt" id="dev-cnt">0 NODES</span></div>
    <div class="grid" id="device-grid"></div>
  </div>
</main>

<!-- Approval Modal -->
<div class="modal-bg" id="modal">
  <div class="modal">
    <h2 id="modal-title">Node Authorization</h2>
    <div><div class="m-lbl">Subject Name</div><input class="m-in" id="m-name" placeholder="e.g. John Doe"></div>
    <div class="m-row">
      <div><div class="m-lbl">Bed</div><input class="m-in" id="m-bed" placeholder="e.g. B-12"></div>
      <div><div class="m-lbl">Room</div><input class="m-in" id="m-room" placeholder="e.g. ICU-3"></div>
    </div>
    <div class="m-btns">
      <button class="btn btn-em" onclick="submitModal()">Activate Node</button>
      <button class="btn" style="background:rgba(255,255,255,.04);color:var(--s400)" onclick="closeModal()">Cancel</button>
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
  ws.onopen=()=>{document.getElementById('ws-indicator').className='ws-badge on';document.getElementById('ws-text').textContent='LIVE'};
  ws.onclose=()=>{document.getElementById('ws-indicator').className='ws-badge off';document.getElementById('ws-text').textContent='OFFLINE';setTimeout(connectWS,2000)};
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
  document.getElementById('mode-label').textContent=modes[s.mode]||'Encryption Active';
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
  document.getElementById('pend-cnt').textContent=pending.length+' NEW';
  document.getElementById('dev-cnt').textContent=approved.length+' NODES';

  // Pending section
  const ps=document.getElementById('pending-section');
  const pl=document.getElementById('pending-list');
  if(pending.length>0){
    ps.style.display='block';
    pl.innerHTML=pending.map(d=>`
      <div class="pend">
        <div><div class="pend-id">${d.slaveId}</div><div class="pend-sub">Signal Detected</div></div>
        <button class="btn btn-em" onclick="openModal('${d.slaveId}')">Authorize</button>
      </div>`).join('');
  }else{ps.style.display='none';pl.innerHTML=''}

  // Device grid
  const grid=document.getElementById('device-grid');
  if(approved.length===0){
    grid.innerHTML='<div class="empty"><div class="empty-icon">&#128225;</div><div class="empty-text">Scanning for new hardware...<br>Connect a slave device to begin monitoring.</div></div>';
    return;
  }
  grid.innerHTML=approved.map(d=>{
    const cls=d.alertActive?'alert':(d.online?'on':'off');
    const badge=d.alertActive?'EMERGENCY':(d.online?'LINKED':'OFFLINE');
    const status=d.alertActive?'ALERT ACTIVE':(d.online?'SYSTEM STABLE':'LINK DOWN');
    const time=d.lastAlertTime?formatTime(d.lastAlertTime):'No Events';
    return `
      <div class="dev ${cls}">
        <div class="dev-top">
          <div><div class="dev-name">${d.patientName||'Unnamed'}</div><div class="dev-sid">ID: ${d.slaveId}</div></div>
          <div class="dev-badge">${badge}</div>
        </div>
        <div class="dev-fields">
          <div class="dev-field"><div class="dev-field-lbl">Bed</div><div class="dev-field-val">${d.bed||'-'}</div></div>
          <div class="dev-field"><div class="dev-field-lbl">Room</div><div class="dev-field-val">${d.room||'-'}</div></div>
        </div>
        ${d.alertActive?`<div class="dev-act"><button class="clr-btn" onclick="clearAlert('${d.slaveId}')">&#10006; Silence</button></div>`:''}
        <div class="dev-foot"><div><span class="dot"></span>${status}</div><div>&#128336; ${time}</div></div>
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
function openModal(id){modalId=id;document.getElementById('modal').style.display='flex';document.getElementById('m-name').value='';document.getElementById('m-bed').value='';document.getElementById('m-room').value='';document.getElementById('modal-title').textContent='Node Authorization: '+id;document.getElementById('m-name').focus()}
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
:root{--bg:#070b14;--surface:rgba(15,23,42,.4);--emerald:#10b981;--emerald-g:rgba(16,185,129,.25);--red:#ef4444;--amber:#f59e0b;--white:#fff;--s100:#f1f5f9;--s400:#94a3b8;--s500:#64748b;--s600:#475569;--glass:rgba(255,255,255,.05);--glass2:rgba(255,255,255,.1)}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,sans-serif;background:var(--bg);color:var(--s100);min-height:100vh;padding:24px}
body::before{content:'';position:fixed;inset:0;background:radial-gradient(circle at 0% 0%,rgba(16,185,129,.05) 0%,transparent 40%);pointer-events:none}
.container{max-width:860px;margin:0 auto;position:relative}
header{display:flex;justify-content:space-between;align-items:center;margin-bottom:28px;padding-bottom:16px;border-bottom:1px solid var(--glass)}
header .hdr-l{display:flex;align-items:center;gap:12px}
header .hdr-icon{width:48px;height:48px;background:rgba(15,23,42,.8);border:1px solid var(--glass2);border-radius:16px;display:flex;align-items:center;justify-content:center}
header .hdr-icon svg{width:24px;height:24px;color:var(--emerald)}
header h1{font-size:18px;font-weight:800;color:var(--white)}
header .hdr-sub{font-size:10px;color:rgba(16,185,129,.5);font-weight:700;text-transform:uppercase;letter-spacing:1.5px}
header a{color:var(--emerald);text-decoration:none;font-size:12px;font-weight:700;padding:8px 16px;border-radius:10px;border:1px solid rgba(16,185,129,.15);transition:all .15s}
header a:hover{background:rgba(16,185,129,.06)}
.sec-title{font-size:13px;font-weight:700;color:var(--white);margin-bottom:12px;margin-top:24px;display:flex;align-items:center;gap:8px}
.sec-title .cnt{padding:2px 8px;border-radius:8px;background:rgba(255,255,255,.04);border:1px solid var(--glass2);font-size:10px;font-weight:700;color:var(--s400)}
.dev{background:var(--surface);border:1px solid var(--glass);border-radius:14px;padding:16px 20px;margin-bottom:8px;display:flex;align-items:center;justify-content:space-between;backdrop-filter:blur(12px);transition:all .15s}
.dev:hover{border-color:rgba(16,185,129,.12)}
.dev.pending{border-left:3px solid var(--amber)}
.dev-id{font-family:'Cascadia Code','Fira Code',monospace;font-weight:700;color:var(--emerald);font-size:13px}
.dev-meta{font-size:11px;color:var(--s400);margin-top:2px}
.dev-actions{display:flex;gap:6px}
.btn{padding:7px 14px;border-radius:8px;border:none;font-weight:700;cursor:pointer;font-size:11px;transition:all .15s}
.btn:hover{transform:translateY(-1px)}
.btn-em{background:var(--emerald);color:var(--white);box-shadow:0 2px 8px var(--emerald-g)}
.btn-edit{background:rgba(255,255,255,.04);color:var(--s400);border:1px solid var(--glass2)}
.btn-del{background:rgba(239,68,68,.06);color:var(--red);border:1px solid rgba(239,68,68,.1)}
.modal-bg{position:fixed;inset:0;background:rgba(0,0,0,.7);backdrop-filter:blur(8px);display:none;align-items:center;justify-content:center;z-index:999}
.modal{background:rgba(15,23,42,.95);border:1px solid var(--glass2);border-radius:20px;padding:28px;width:100%;max-width:380px}
.modal h2{margin-bottom:18px;font-size:17px;font-weight:800;color:var(--white)}
.m-in{width:100%;padding:11px 14px;background:rgba(255,255,255,.04);border:1px solid var(--glass2);border-radius:10px;color:var(--white);margin-bottom:10px;outline:none;font-size:13px;font-family:inherit}
.m-in:focus{border-color:var(--emerald)}
.m-lbl{display:block;font-size:9px;color:var(--s500);margin-bottom:4px;font-weight:700;text-transform:uppercase;letter-spacing:.6px}
.empty{padding:20px;text-align:center;color:var(--s600);font-size:12px;border:1px dashed var(--glass2);border-radius:12px}
</style>
</head>
<body>
<div class="container">
  <header>
    <div class="hdr-l">
      <div class="hdr-icon"><svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg></div>
      <div>
        <h1>Security Command</h1>
        <div class="hdr-sub">Administration</div>
      </div>
    </div>
    <a href="/">&larr; Dashboard</a>
  </header>
  <div class="sec-title">&#9888;&#65039; Discovery Queue <span class="cnt" id="p-cnt">0</span></div>
  <div id="pending"></div>
  <div class="sec-title">&#9989; Active Infrastructure <span class="cnt" id="a-cnt">0</span></div>
  <div id="approved"></div>
</div>

<div class="modal-bg" id="modal">
  <div class="modal">
    <h2 id="modal-title">Node Authorization</h2>
    <div class="m-lbl">Subject Name</div><input class="m-in" id="m-name">
    <div class="m-lbl">Bed</div><input class="m-in" id="m-bed">
    <div class="m-lbl">Room</div><input class="m-in" id="m-room">
    <div style="display:flex;gap:8px;margin-top:8px">
      <button class="btn btn-em" style="flex:1" onclick="submitModal()">Activate Node</button>
      <button class="btn" style="flex:1;background:rgba(255,255,255,.04);color:var(--s400)" onclick="closeModal()">Cancel</button>
    </div>
  </div>
</div>

<script>
let editId=null,editMode='approve';
const AUTH='admin1234';
function refresh(){
  fetch('/api/slaves?all=1').then(r=>r.json()).then(devs=>{
    const pen=document.getElementById('pending'),app=document.getElementById('approved');
    let hp='',ha='',pc=0,ac=0;
    devs.forEach(d=>{
      if(!d.approved){
        pc++;
        hp+=`<div class="dev pending"><div><div class="dev-id">${d.slaveId}</div><div class="dev-meta">Signal Detected</div></div><div class="dev-actions"><button class="btn btn-em" onclick="openApprove('${d.slaveId}')">Authorize</button><button class="btn btn-del" onclick="delDev('${d.slaveId}')">Delete</button></div></div>`;
      }else{
        ac++;
        ha+=`<div class="dev"><div><div class="dev-id">${d.slaveId}</div><div class="dev-meta">${d.patientName} &middot; Bed ${d.bed} &middot; ${d.room}</div></div><div class="dev-actions"><button class="btn btn-edit" onclick="openEdit('${d.slaveId}','${esc(d.patientName)}','${esc(d.bed)}','${esc(d.room)}')">Modify</button><button class="btn btn-del" onclick="delDev('${d.slaveId}')">Delete</button></div></div>`;
      }
    });
    document.getElementById('p-cnt').textContent=pc;
    document.getElementById('a-cnt').textContent=ac;
    pen.innerHTML=hp||'<div class="empty">Scanning for new hardware...</div>';
    app.innerHTML=ha||'<div class="empty">No active nodes</div>';
  });
}
function esc(s){return(s||'').replace(/'/g,"\\'")}
function openApprove(id){editId=id;editMode='approve';document.getElementById('modal-title').textContent='Node Authorization: '+id;document.getElementById('m-name').value='';document.getElementById('m-bed').value='';document.getElementById('m-room').value='';document.getElementById('modal').style.display='flex'}
function openEdit(id,n,b,r){editId=id;editMode='edit';document.getElementById('modal-title').textContent='Modify: '+id;document.getElementById('m-name').value=n;document.getElementById('m-bed').value=b;document.getElementById('m-room').value=r;document.getElementById('modal').style.display='flex'}
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
