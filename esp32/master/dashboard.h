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
body::before{content:'';position:fixed;inset:0;background:radial-gradient(circle at 50% 50%,rgba(16,185,129,0.08) 0%,transparent 60%);pointer-events:none}
.card{background:rgba(15,23,42,.85);backdrop-filter:blur(30px);border:1px solid var(--glass2);border-radius:28px;padding:40px 32px;width:100%;max-width:460px;position:relative;box-shadow:0 25px 50px -12px rgba(0,0,0,0.5)}
.card::before{content:'';position:absolute;inset:-1px;background:linear-gradient(135deg,rgba(16,185,129,0.2),transparent 50%);border-radius:28px;z-index:-1}
.logo{text-align:center;margin-bottom:32px}
.logo-icon{width:64px;height:64px;background:rgba(16,185,129,0.1);border:1px solid rgba(16,185,129,0.2);border-radius:20px;display:inline-flex;align-items:center;justify-content:center;margin-bottom:12px;box-shadow:0 0 20px rgba(16,185,129,0.1)}
.logo-icon svg{width:32px;height:32px;color:var(--emerald)}
.logo h1{font-size:26px;font-weight:800;color:var(--white);letter-spacing:-1px}
.logo p{font-size:11px;color:rgba(16,185,129,0.6);margin-top:4px;font-weight:700;text-transform:uppercase;letter-spacing:2px}
.step{display:none;animation:fadeUp 0.4s cubic-bezier(0.16, 1, 0.3, 1)}.step.active{display:block}
@keyframes fadeUp{from{opacity:0;transform:translateY(15px)}to{opacity:1;transform:translateY(0)}}
.modes{display:flex;flex-direction:column;gap:10px;margin:20px 0}
.mode-card{background:rgba(255,255,255,0.02);border:1px solid var(--glass2);border-radius:16px;padding:18px 20px;cursor:pointer;transition:all 0.25s ease}
.mode-card:hover{border-color:rgba(16,185,129,0.3);background:rgba(16,185,129,0.05);transform:scale(1.02)}
.mode-card.selected{border-color:var(--emerald);background:rgba(16,185,129,0.1);box-shadow:0 0 15px rgba(16,185,129,0.1)}
.mode-card h3{font-size:15px;display:flex;align-items:center;gap:8px;margin-bottom:4px;font-weight:700;color:var(--white)}
.mode-card p{font-size:12px;color:var(--s400);line-height:1.5;opacity:0.8}
.badge{font-size:9px;padding:3px 10px;border-radius:12px;font-weight:700;text-transform:uppercase;letter-spacing:1px;margin-left:auto}
.badge-local{background:rgba(255,255,255,0.05);color:var(--s400)}
.badge-hybrid{background:rgba(16,185,129,0.1);color:#34d399}
.badge-cloud{background:rgba(168,85,247,0.15);color:#c084fc}
.btn{width:100%;padding:14px;border:none;border-radius:14px;font-size:14px;font-weight:700;cursor:pointer;transition:all 0.2s cubic-bezier(0.4, 0, 0.2, 1);margin-top:10px;background:var(--emerald);color:var(--white);box-shadow:0 4px 15px rgba(16,185,129,0.2)}
.btn:hover{transform:translateY(-2px);box-shadow:0 8px 25px rgba(16,185,129,0.3)}
.btn:active{transform:translateY(0)}
.btn:disabled{opacity:.3;cursor:not-allowed;box-shadow:none;transform:none}
.btn-ghost{background:rgba(255,255,255,0.04);color:var(--s400);border:1px solid var(--glass2)}
.btn-ghost:hover{background:rgba(255,255,255,0.08);color:var(--white)}
.fg{margin-bottom:16px}
.fg label{display:block;font-size:10px;color:var(--s500);margin-bottom:6px;font-weight:700;text-transform:uppercase;letter-spacing:1px}
.fg input{width:100%;padding:12px 16px;background:rgba(255,255,255,0.03);border:1px solid var(--glass2);border-radius:12px;color:var(--white);outline:none;font-size:14px;transition:all 0.2s}
.fg input:focus{border-color:var(--emerald);background:rgba(255,255,255,0.06);box-shadow:0 0 10px rgba(16,185,129,0.1)}
.wl{max-height:200px;overflow-y:auto;border-radius:16px;border:1px solid var(--glass2);margin-bottom:16px;background:rgba(0,0,0,0.2)}
.wl::-webkit-scrollbar{width:4px}.wl::-webkit-scrollbar-thumb{background:var(--emerald);border-radius:4px}
.wi{padding:14px 16px;border-bottom:1px solid var(--glass);cursor:pointer;display:flex;justify-content:space-between;align-items:center;transition:all 0.2s}
.wi:last-child{border-bottom:none}
.wi:hover{background:rgba(16,185,129,0.04)}
.wi.selected{background:rgba(16,185,129,0.12);border-left:4px solid var(--emerald)}
.wi-rssi{font-size:11px;color:var(--emerald);font-weight:700;font-family:monospace}
.scanning{text-align:center;padding:25px;color:var(--s400);font-size:13px}
.spinner{width:18px;height:18px;border:3px solid rgba(16,185,129,0.1);border-top-color:var(--emerald);border-radius:50%;animation:spin .8s linear infinite;display:inline-block;vertical-align:middle;margin-right:8px}
@keyframes spin{to{transform:rotate(360deg)}}
.status-msg{padding:12px 16px;border-radius:12px;text-align:center;font-size:13px;margin-top:12px;font-weight:600;border:1px solid transparent}
.status-msg.info{background:rgba(16,185,129,0.08);color:#34d399;border-color:rgba(16,185,129,0.15)}
.status-msg.error{background:rgba(239,68,68,0.08);color:#f87171;border-color:rgba(239,68,68,0.15)}
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
    <h2 style="font-size:16px;text-align:center;margin-bottom:16px;font-weight:700;color:#fff">Select Operations Mode</h2>
    <div class="modes">
      <div class="mode-card" onclick="pickMode(1)" id="m1">
        <h3>AP Standalone <span class="badge badge-local">Local</span></h3>
        <p>Independent nursing network. No router required.</p>
      </div>
      <div class="mode-card" onclick="pickMode(2)" id="m2">
        <h3>STA Client <span class="badge badge-local">Router</span></h3>
        <p>Integrate with existing hospital WiFi network.</p>
      </div>
      <div class="mode-card" onclick="pickMode(3)" id="m3">
        <h3>Hybrid Mode <span class="badge badge-hybrid">PRO</span></h3>
        <p>Local channel for devices + Bridge for router.</p>
      </div>
      <div class="mode-card" onclick="pickMode(4)" id="m4">
        <h3>Cloud Sync <span class="badge badge-cloud">Global</span></h3>
        <p>Remote monitoring with full Railway integration.</p>
      </div>
    </div>
    <button class="btn" id="nextBtn" onclick="goStep2()" disabled>Initialize System</button>
  </div>

  <div id="stepAP" class="step">
    <h2 style="font-size:16px;text-align:center;margin-bottom:16px;font-weight:700;color:#fff">Establish Access Point</h2>
    <div class="fg"><label>Network Name (SSID)</label><input id="ap-ssid" value="HospitalAlarm" placeholder="e.g. Hospital_Nurse_Station"></div>
    <div class="fg"><label>Security Secret (Optional)</label><input id="ap-pass" type="password" placeholder="Min 8 characters or leave empty"></div>
    <button class="btn" onclick="goStepWiFi()">Link Configuration</button>
    <button class="btn btn-ghost" onclick="showStep('step1')">Back</button>
  </div>

  <div id="stepWiFi" class="step">
    <h2 style="font-size:16px;text-align:center;margin-bottom:14px;font-weight:700;color:#fff">Network Discovery</h2>
    <div id="scan-list" class="wl"><div class="scanning"><div class="spinner"></div>Analyzing Environment...</div></div>
    <div class="fg"><label>Network Password</label><input type="password" id="wifi-pass" placeholder="Enter network key"></div>
    <div id="setup-status"></div>
    <button class="btn" id="connectBtn" onclick="doConnect()" disabled>Authorize & Finish</button>
    <button class="btn btn-ghost" onclick="scanWiFi()">Rescan Environment</button>
    <button class="btn btn-ghost" onclick="showStep('stepAP')">Back</button>
  </div>

  <div id="stepDone" class="step">
    <div style="text-align:center;padding:25px 0">
      <div style="font-size:52px;margin-bottom:15px;text-shadow: 0 0 20px rgba(16,185,129,0.4)">✅</div>
      <h2 style="font-size:22px;font-weight:800;margin-bottom:8px;color:#fff">System Online</h2>
      <p id="done-info" style="color:var(--s400);font-size:14px;line-height:1.6"></p>
      <button class="btn" style="margin-top:25px" onclick="location.href='/'">Launch Command UI</button>
    </div>
  </div>
</div>

<script>
let selMode=0,selSSID='';
function pickMode(m){selMode=m;document.querySelectorAll('.mode-card').forEach(c=>c.classList.remove('selected'));document.getElementById('m'+m).classList.add('selected');document.getElementById('nextBtn').disabled=false}
function showStep(s){document.querySelectorAll('.step').forEach(e=>e.classList.remove('active'));document.getElementById(s).classList.add('active')}
function goStep2(){showStep('stepAP')}
function goStepWiFi(){if(selMode===1){finishSetup();return}showStep('stepWiFi');scanWiFi()}
function scanWiFi(){const l=document.getElementById('scan-list');l.innerHTML='<div class="scanning"><div class="spinner"></div>Searching for networks...</div>';fetch('/api/scan').then(r=>r.json()).then(nets=>{let h='';nets.forEach(n=>{h+=`<div class="wi" onclick="pickWiFi(this,'${n.ssid}')"><span>${n.ssid}</span><span class="wi-rssi">${n.rssi} dBm</span></div>`});l.innerHTML=h||'<div class="scanning">No networks found</div>'})}
function pickWiFi(el,ssid){selSSID=ssid;document.querySelectorAll('.wi').forEach(i=>i.classList.remove('selected'));el.classList.add('selected');document.getElementById('connectBtn').disabled=false}
function doConnect(){
  const p=document.getElementById('wifi-pass').value, s=document.getElementById('setup-status');
  s.innerHTML='<div class="status-msg info"><div class="spinner"></div> Linking with '+selSSID+'...</div>';
  fetch('/api/connect-wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:selSSID,password:p})})
    .then(r=>r.json())
    .then(d=>{
      if(!d.success){s.innerHTML='<div class="status-msg error">'+d.message+'</div>';return;}
      let a=0, i=setInterval(()=>{
        fetch('/api/wifi-status').then(r=>r.json()).then(st=>{
          a++;
          if(st.status===3){clearInterval(i);s.innerHTML='<div class="status-msg info">Connection Verified. Initializing...</div>';setTimeout(finishSetup,1000);}
          else if(st.status===4||st.status===1||a>=15){clearInterval(i);s.innerHTML='<div class="status-msg error">Handshake Failed. Check Credentials.</div>';}
        }).catch(()=>{a++;if(a>=15){clearInterval(i);s.innerHTML='<div class="status-msg error">Network Timeout.</div>';}});
      },1000);
    });
}
function finishSetup(){
  const as=document.getElementById('ap-ssid').value||'HospitalAlarm', ap=document.getElementById('ap-pass').value;
  document.getElementById('setup-status').innerHTML='<div class="status-msg info"><div class="spinner"></div> Launching UI...</div>';
  fetch('/api/setup',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:selMode,apSSID:as,apPass:ap})}).catch(()=>console.log("Rebooting..."));
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
:root{--bg:#070b14;--surface:rgba(15,23,42,.4);--emerald:#10b981;--emerald-g:rgba(16,185,129,.35);--red:#ef4444;--red-g:rgba(239,68,68,.3);--amber:#f59e0b;--white:#fff;--s100:#f1f5f9;--s400:#94a3b8;--s500:#64748b;--s600:#475569;--glass:rgba(255,255,255,.05);--glass2:rgba(255,255,255,.1)}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:var(--bg);color:var(--s100);min-height:100vh;display:flex;flex-direction:column;overflow-x:hidden}
body::before{content:'';position:fixed;inset:0;background:radial-gradient(circle at 0% 0%,rgba(16,185,129,0.08) 0%,transparent 45%);pointer-events:none;z-index:0}

/* ── Header ── */
header{background:rgba(7,11,20,0.85);backdrop-filter:blur(24px);border-bottom:1px solid var(--glass2);padding:18px 28px;display:flex;justify-content:space-between;align-items:center;position:sticky;top:0;z-index:100;box-shadow:0 10px 30px rgba(0,0,0,0.4)}
.logo{display:flex;align-items:center;gap:14px}
.logo-icon{width:50px;height:50px;background:rgba(15,23,42,0.9);border:1px solid var(--glass2);border-radius:18px;display:flex;align-items:center;justify-content:center;box-shadow:0 0 15px rgba(16,185,129,0.1)}
.logo-icon svg{width:26px;height:26px;color:var(--emerald)}
.logo h1{font-size:20px;font-weight:800;color:var(--white);letter-spacing:-0.5px}
.logo p{font-size:10px;color:rgba(16,185,129,0.6);text-transform:uppercase;letter-spacing:2px;font-weight:700}
.hdr-right{display:flex;align-items:center;gap:12px}
.ws-badge{padding:7px 14px;border-radius:12px;font-size:10px;font-weight:800;letter-spacing:0.8px;display:flex;align-items:center;gap:7px;text-transform:uppercase;transition:all 0.3s}
.ws-badge.off{background:rgba(239,68,68,0.1);border:1px solid rgba(239,68,68,0.2);color:#f87171}
.ws-badge.on{background:rgba(16,185,129,0.1);border:1px solid rgba(16,185,129,0.2);color:#34d399;box-shadow:0 0 15px rgba(16,185,129,0.1)}
.ws-dot{width:7px;height:7px;border-radius:50%;background:var(--s600)}
.ws-badge.on .ws-dot{background:var(--emerald);box-shadow:0 0 10px var(--emerald);animation:pulse-ws 2s infinite}
@keyframes pulse-ws{0%,100%{opacity:1}50%{opacity:0.4}}
.hdr-btn{padding:8px 18px;border-radius:12px;border:1px solid var(--glass2);background:rgba(255,255,255,0.03);color:var(--s400);font-size:11px;font-weight:700;cursor:pointer;transition:all 0.2s ease;text-decoration:none}
.hdr-btn:hover{background:var(--emerald);color:white;border-color:var(--emerald);transform:translateY(-1px);box-shadow:0 5px 15px var(--emerald-g)}

/* ── Stats ── */
.stats{display:grid;grid-template-columns:repeat(4,1fr);gap:14px;padding:24px 28px;position:relative;z-index:1}
.stat{background:rgba(15,23,42,0.85);backdrop-filter:blur(20px);border:1px solid var(--glass2);border-radius:18px;padding:18px;display:flex;align-items:center;gap:15px;transition:all 0.3s}
.stat:hover{border-color:rgba(255,255,255,0.2);transform:translateY(-2px)}
.stat-icon{width:44px;height:44px;border-radius:14px;display:flex;align-items:center;justify-content:center;font-size:20px;flex-shrink:0}
.stat-icon.em{background:rgba(16,185,129,0.15);color:var(--emerald)}
.stat-icon.rd{background:rgba(239,68,68,0.15);color:var(--red)}
.stat-icon.am{background:rgba(245,158,11,0.15);color:var(--amber)}
.stat-icon.bl{background:rgba(255,255,255,0.08);color:white}
.stat-val{font-size:24px;font-weight:800;color:var(--white);line-height:1}
.stat-lbl{font-size:10px;color:var(--s500);text-transform:uppercase;letter-spacing:1px;font-weight:700;margin-top:2px}

/* ── Main ── */
main{padding:0 28px 40px;flex:1;position:relative;z-index:1}
.sec{margin-bottom:28px}
.sec-hdr{display:flex;align-items:center;gap:10px;margin-bottom:16px;font-size:15px;font-weight:700;color:var(--white);letter-spacing:-0.2px}
.sec-hdr .cnt{padding:3px 10px;border-radius:10px;background:rgba(255,255,255,0.06);border:1px solid var(--glass2);font-size:11px;font-weight:700;color:var(--s400)}

/* ── Discovery Queue ── */
.pend{background:rgba(245,158,11,0.05);border:1px solid rgba(245,158,11,0.2);border-left:4px solid var(--amber);border-radius:18px;padding:16px 22px;display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;backdrop-filter:blur(16px);animation:slideIn 0.4s ease}
@keyframes slideIn{from{opacity:0;transform:translateX(-10px)}to{opacity:1;transform:translateX(0)}}
.pend-id{font-family:'JetBrains Mono','Fira Code',monospace;font-size:15px;font-weight:800;color:var(--amber)}
.pend-sub{font-size:10px;color:var(--s500);margin-top:3px;font-weight:700;text-transform:uppercase;letter-spacing:1px}

/* ── Device Grid ── */
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(320px,1fr));gap:18px}
.dev{background:rgba(15,23,42,0.8);border:1px solid var(--glass2);border-radius:20px;padding:24px;position:relative;overflow:hidden;backdrop-filter:blur(20px);transition:all 0.3s cubic-bezier(0.4, 0, 0.2, 1)}
.dev:hover{border-color:rgba(16,185,129,0.3);transform:scale(1.01);box-shadow:0 15px 35px rgba(0,0,0,0.3)}
.dev.alert{border-color:rgba(239,68,68,0.5);background:rgba(239,68,68,0.05);box-shadow:0 0 30px rgba(239,68,68,0.15)}
.dev.alert::before{content:'';position:absolute;inset:0;background:rgba(239,68,68,0.08);animation:pulse-alert 1.5s infinite;pointer-events:none}
@keyframes pulse-alert{0%,100%{opacity:0.2}50%{opacity:0.8}}
.dev-top{display:flex;justify-content:space-between;align-items:start;margin-bottom:18px}
.dev-name{font-size:18px;font-weight:800;color:var(--white);letter-spacing:-0.3px}
.dev-sid{font-size:11px;color:var(--s500);margin-top:3px;font-weight:700;text-transform:uppercase;letter-spacing:1px}
.dev-badge{padding:5px 12px;border-radius:10px;font-size:10px;font-weight:800;text-transform:uppercase;letter-spacing:1px;border:none;flex-shrink:0}
.dev.on .dev-badge{background:rgba(255,255,255,0.06);color:var(--s400)}
.dev.alert .dev-badge{background:var(--red);color:var(--white);animation:blink-badge 1s infinite;box-shadow:0 0 15px var(--red-g)}
@keyframes blink-badge{0%,100%{transform:scale(1)}50%{transform:scale(1.05);opacity:0.9}}
.dev-fields{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:18px}
.dev-field{background:rgba(255,255,255,0.03);border:1px solid var(--glass);border-radius:12px;padding:12px 14px}
.dev-field-lbl{font-size:9px;color:var(--s500);margin-bottom:3px;text-transform:uppercase;letter-spacing:1px;font-weight:700}
.dev-field-val{font-size:15px;font-weight:700;color:var(--white)}
.dev-foot{padding-top:16px;border-top:1px solid var(--glass);display:flex;justify-content:space-between;align-items:center;font-size:10px;font-weight:700;text-transform:uppercase;letter-spacing:1px;color:var(--s500)}
.dev.alert .dev-foot{border-color:rgba(239,68,68,0.2)}
.dot{width:8px;height:8px;border-radius:50%;display:inline-block;margin-right:8px}
.dev.on .dot{background:var(--emerald);box-shadow:0 0 10px var(--emerald)}
.dev.alert .dot{background:var(--red);box-shadow:0 0 10px var(--red);animation:pulse-dot 1s infinite}
@keyframes pulse-dot{0%,100%{transform:scale(1);opacity:1}50%{transform:scale(1.5);opacity:0.5}}
.clr-btn{width:100%;margin-top:10px;padding:10px;background:rgba(239,68,68,0.1);color:#f87171;border:1px solid rgba(239,68,68,0.3);border-radius:12px;font-size:11px;font-weight:800;cursor:pointer;text-transform:uppercase;letter-spacing:1px;transition:all 0.2s}
.clr-btn:hover{background:var(--red);color:white;box-shadow:0 5px 15px var(--red-g)}

/* ── Modal ── */
.modal-bg{position:fixed;inset:0;background:rgba(0,0,0,0.75);backdrop-filter:blur(10px);z-index:999;display:none;align-items:center;justify-content:center;padding:20px}
.modal{background:rgba(15,23,42,0.98);border:1px solid var(--glass2);border-radius:24px;padding:36px;width:100%;max-width:420px;box-shadow:0 30px 60px rgba(0,0,0,0.6);animation:zoomIn 0.3s cubic-bezier(0.16, 1, 0.3, 1)}
@keyframes zoomIn{from{opacity:0;transform:scale(0.9)}to{opacity:1;transform:scale(1)}}
.modal h2{font-size:20px;font-weight:800;margin-bottom:24px;color:var(--white);text-align:center}
.m-in{width:100%;padding:13px 16px;background:rgba(255,255,255,0.04);border:1px solid var(--glass2);border-radius:14px;color:var(--white);margin-bottom:14px;outline:none;font-size:14px;transition:all 0.2s}
.m-in:focus{border-color:var(--emerald);background:rgba(255,255,255,0.07)}
.btn-m{width:100%;padding:14px;background:var(--emerald);border:none;border-radius:14px;color:white;font-weight:700;font-size:14px;cursor:pointer;transition:all 0.2s;box-shadow:0 4px 15px var(--emerald-g)}
.btn-m:hover{transform:translateY(-2px);box-shadow:0 8px 25px var(--emerald-g)}

.empty{grid-column:1/-1;text-align:center;padding:80px 30px;border:2px dashed var(--glass2);border-radius:24px;background:rgba(255,255,255,0.01)}
.empty-icon{font-size:50px;margin-bottom:15px;opacity:0.25;animation:float 3s ease-in-out infinite}
@keyframes float{0%,100%{transform:translateY(0)}50%{transform:translateY(-10px)}}

@media(max-width:768px){
  header{padding:15px 20px}
  .stats{grid-template-columns:1fr 1fr;padding:15px 20px;gap:10px}
  main{padding:0 20px 30px}
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
      <p id="mode-label">LOCAL SYSTEM ONLINE</p>
    </div>
  </div>
  <div class="hdr-right">
    <div class="ws-badge off" id="ws-indicator"><div class="ws-dot"></div><span id="ws-text">DISCONNECTED</span></div>
    <a href="/admin" class="hdr-btn">System</a>
  </div>
</header>

<div class="stats">
  <div class="stat"><div class="stat-icon bl">&#128225;</div><div><div class="stat-val" id="s-total">0</div><div class="stat-lbl">Nodes</div></div></div>
  <div class="stat"><div class="stat-icon em">&#128994;</div><div><div class="stat-val" id="s-online">0</div><div class="stat-lbl">Active</div></div></div>
  <div class="stat"><div class="stat-icon rd">&#128680;</div><div><div class="stat-val" id="s-alerts">0</div><div class="stat-lbl">Alerts</div></div></div>
  <div class="stat"><div class="stat-icon am">&#9203;</div><div><div class="stat-val" id="s-pending">0</div><div class="stat-lbl">New</div></div></div>
</div>

<main>
  <div class="sec" id="pending-section" style="display:none">
    <div class="sec-hdr">&#128269; Discovery Queue <span class="cnt" id="pend-cnt">0 DETECTED</span></div>
    <div id="pending-list"></div>
  </div>

  <div class="sec">
    <div class="sec-hdr">&#9878;&#65039; Deployed Infrastructure <span class="cnt" id="dev-cnt">0 NODES</span></div>
    <div class="grid" id="device-grid"></div>
  </div>
</main>

<div class="modal-bg" id="modal">
  <div class="modal">
    <h2 id="modal-title">Authorize Node</h2>
    <div><div class="dev-field-lbl" style="margin-left:4px">Subject Name</div><input class="m-in" id="m-name" placeholder="Assigned patient"></div>
    <div style="display:flex;gap:12px;margin-top:5px">
      <div style="flex:1"><div class="dev-field-lbl" style="margin-left:4px">Bed</div><input class="m-in" id="m-bed" placeholder="B-01"></div>
      <div style="flex:1"><div class="dev-field-lbl" style="margin-left:4px">Room</div><input class="m-in" id="m-room" placeholder="ICU-1"></div>
    </div>
    <button class="btn-m" onclick="submitModal()">Activate Node</button>
    <button class="btn-m" style="background:transparent;margin-top:8px;box-shadow:none;color:var(--s500)" onclick="closeModal()">Dismiss</button>
  </div>
</div>

<script>
let devices={},ws=null,modalId=null;
const AUTH='admin1234';

function connectWS(){
  const proto=location.protocol==='https:'?'wss:':'ws:';
  ws=new WebSocket(proto+'//'+location.host+'/ws');
  ws.onopen=()=>{document.getElementById('ws-indicator').className='ws-badge on';document.getElementById('ws-text').textContent='LIVE LINK'};
  ws.onclose=()=>{document.getElementById('ws-indicator').className='ws-badge off';document.getElementById('ws-text').textContent='LINK DOWN';setTimeout(connectWS,2000)};
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
    }catch(err){console.error('[WS] Error:',err)}
  };
}
connectWS();

fetch('/api/slaves?all=1').then(r=>r.json()).then(list=>{if(Array.isArray(list))list.forEach(d=>devices[d.slaveId]=d);render();});

function render(){
  const all=Object.values(devices);
  const approved=all.filter(d=>d.approved), pending=all.filter(d=>!d.approved);
  const online=all.filter(d=>d.online), alerts=all.filter(d=>d.alertActive);

  document.getElementById('s-total').textContent=approved.length;
  document.getElementById('s-online').textContent=online.length;
  document.getElementById('s-alerts').textContent=alerts.length;
  document.getElementById('s-pending').textContent=pending.length;
  document.getElementById('pend-cnt').textContent=pending.length+' DETECTED';
  document.getElementById('dev-cnt').textContent=approved.length+' NODES';

  const pl=document.getElementById('pending-list');
  if(pending.length>0){
    document.getElementById('pending-section').style.display='block';
    pl.innerHTML=pending.map(d=>`
      <div class="pend">
        <div><div class="pend-id">${d.slaveId}</div><div class="pend-sub">Encrypted Signal detected</div></div>
        <button class="hdr-btn" style="background:var(--amber);color:black;border:none" onclick="openModal('${d.slaveId}')">Authorize Unit</button>
      </div>`).join('');
  }else{document.getElementById('pending-section').style.display='none'}

  const grid=document.getElementById('device-grid');
  if(approved.length===0){
    grid.innerHTML='<div class="empty"><div class="empty-icon">&#128225;</div><div class="empty-text">Infrastructure Status: Silent<br>Authorize a terminal unit to begin monitoring.</div></div>';
    return;
  }
  grid.innerHTML=approved.map(d=>{
    const cls=d.alertActive?'alert':(d.online?'on':'off');
    const badge=d.alertActive?'EMERGENCY':(d.online?'SYNCED':'LOST');
    const status=d.alertActive?'ALARM TRIGGERED':(d.online?'STABLE ENCRYPTION':'LOST SIGNAL');
    return `
      <div class="dev ${cls}">
        <div class="dev-top">
          <div><div class="dev-name">${d.patientName||'Unknown Unit'}</div><div class="dev-sid">UUID: ${d.slaveId}</div></div>
          <div class="dev-badge">${badge}</div>
        </div>
        <div class="dev-fields">
          <div class="dev-field"><div class="dev-field-lbl">Bed</div><div class="dev-field-val">${d.bed||'N/A'}</div></div>
          <div class="dev-field"><div class="dev-field-lbl">Zone</div><div class="dev-field-val">${d.room||'N/A'}</div></div>
        </div>
        ${d.alertActive?`<button class="clr-btn" onclick="clearAlert('${d.slaveId}')">Deactivate Alarm</button>`:''}
        <div class="dev-foot"><div><span class="dot"></span>${status}</div></div>
      </div>`;
  }).join('');
}

function openModal(id){modalId=id;document.getElementById('modal').style.display='flex';document.getElementById('m-name').value='';document.getElementById('m-name').focus()}
function closeModal(){document.getElementById('modal').style.display='none'}
function submitModal(){
  const n=document.getElementById('m-name').value,b=document.getElementById('m-bed').value,r=document.getElementById('m-room').value;
  if(!n||!b||!r){alert('Required fields missing');return}
  fetch('/api/approve',{method:'POST',headers:{'Content-Type':'application/json','X-Auth-Token':AUTH},body:JSON.stringify({slaveId:modalId,patientName:n,bed:b,room:r})}).then(()=>{closeModal();});
}
function clearAlert(id){
  fetch('/api/clearAlert',{method:'POST',headers:{'Content-Type':'application/json','X-Auth-Token':AUTH},body:JSON.stringify({slaveId:id})});
}
setInterval(render,10000);
</script>
</body>
</html>
)rawliteral";

// ─── ADMIN PAGE (Simplified for memory) ──────────────────────
const char ADMIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Console — Hospital System</title>
<style>
:root{--bg:#070b14;--emerald:#10b981;--glass2:rgba(255,255,255,.1)}
body{font-family:system-ui,sans-serif;background:var(--bg);color:white;padding:24px;display:flex;justify-content:center;align-items:center;min-height:100vh}
.card{background:rgba(15,23,42,0.95);border:1px solid var(--glass2);border-radius:24px;padding:32px;width:100%;max-width:400px;text-align:center}
h1{font-size:22px;margin-bottom:5px;font-weight:800;letter-spacing:-0.5px}
input, select, button{width:100%;padding:14px;border-radius:12px;border:1px solid var(--glass2);background:rgba(255,255,255,0.05);color:white;margin-bottom:10px;font-weight:700;outline:none}
button{background:var(--emerald);border:none;cursor:pointer;transition:all 0.2s}
button:hover{transform:translateY(-2px);box-shadow:0 8px 20px rgba(16,185,129,0.3)}
.danger{background:rgba(239,68,68,0.1);color:#f87171;border:1px solid rgba(239,68,68,0.2)}
.danger:hover{background:#ef4444;color:white}
</style>
</head>
<body>
<div id="login" class="card">
  <div style="margin-bottom:20px"><svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="#10b981" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg></div>
  <h1>Security Command</h1><p style="font-size:10px;color:#10b981;margin-bottom:20px;text-transform:uppercase;letter-spacing:2px;font-weight:700;opacity:0.6">Administrator Access</p>
  <input type="password" id="pass" placeholder="••••••••">
  <button onclick="check()">Establish Secure Link</button>
</div>
<div id="panel" class="card" style="display:none">
  <h1>System Profile</h1>
  <label style="display:block;font-size:10px;text-align:left;color:#64748b;margin-bottom:5px;font-weight:700;text-transform:uppercase">Operation Mode</label>
  <select id="mode" onchange="chMode()">
    <option value="1">AP Standalone</option>
    <option value="2">STA Client</option>
    <option value="3">EPP+STA Hybrid</option>
    <option value="4">Cloud Sync Online</option>
  </select>
  <button onclick="location.href='/'">Return to Dashboard</button>
  <div style="margin-top:20px;padding-top:20px;border-top:1px solid var(--glass2);display:flex;flex-direction:column;gap:10px">
    <button style="background:rgba(16,185,129,0.1);color:var(--emerald);border:1px solid rgba(16,185,129,0.2)" onclick="reSetup()">Re-run Setup Wizard</button>
    <button class="danger" onclick="reset()">Wipe Data & Factory Reset</button>
  </div>
</div>
<script>
const AUTH='admin1234';
function check(){if(document.getElementById('pass').value===AUTH){document.getElementById('login').style.display='none';document.getElementById('panel').style.display='block'}else alert('Link Failure: Invalid credentials')}
function chMode(){fetch('/api/system/mode',{method:'POST',headers:{'Content-Type':'application/json','X-Auth-Token':AUTH},body:JSON.stringify({mode:parseInt(document.getElementById('mode').value)})})}
function reSetup(){if(confirm('Re-run setup? WiFi configuration will be cleared.'))fetch('/api/system/re-setup',{method:'POST',headers:{'X-Auth-Token':AUTH}}).then(()=>alert('Entering Setup Mode...'))}
function reset(){if(confirm('ERASE EVERYTHING? This cannot be undone.'))fetch('/api/system/reset',{method:'POST',headers:{'X-Auth-Token':AUTH}}).then(()=>alert('System Wiping...'))}
fetch('/api/status').then(r=>r.json()).then(s=>document.getElementById('mode').value=s.mode);
</script>
</body>
</html>
)rawliteral";


#endif // DASHBOARD_H
