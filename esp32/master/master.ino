#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

const char* AP_SSID     = "HospitalAlarm";
const char* AP_PASSWORD = "hospital123";

const char* ADMIN_USER = "admin";
const char* ADMIN_PASS = "admin1234";

#define MAX_SLAVES 20
#define BUZZER_PIN 4

struct Slave {
  char slaveId[32];
  char patientName[64];
  char bed[16];ma
  char room[16];
  bool alertActive;
  bool registered;
  unsigned long lastAlertTime;
  bool used;
};

Slave slaves[MAX_SLAVES];
int slaveCount = 0;

AsyncWebServer server(80);

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
    if (slaves[i].used && strcmp(slaves[i].slaveId, slaveId) == 0) {
      return i;
    }
  }
  return -1;
}

int findFreeSlot() {
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (!slaves[i].used) return i;
  }
  return -1;
}

String formatTime(unsigned long ms) {
  if (ms == 0) return "null";
  unsigned long secs = ms / 1000;
  unsigned long mins = secs / 60;
  unsigned long hrs = mins / 60;
  secs %= 60;
  mins %= 60;
  char buf[32];
  snprintf(buf, sizeof(buf), "\"%02lu:%02lu:%02lu\"", hrs, mins, secs);
  return String(buf);
}

String getSlavesJson() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < MAX_SLAVES; i++) {
    if (!slaves[i].used) continue;
    JsonObject obj = arr.add<JsonObject>();
    obj["slaveId"] = slaves[i].slaveId;
    obj["patientName"] = slaves[i].patientName;
    obj["bed"] = slaves[i].bed;
    obj["room"] = slaves[i].room;
    obj["alertActive"] = slaves[i].alertActive;
    obj["registered"] = slaves[i].registered;
    if (slaves[i].lastAlertTime > 0) {
      obj["lastAlertTime"] = slaves[i].lastAlertTime;
    } else {
      obj["lastAlertTime"] = (char*)NULL;
    }
  }

  String output;
  serializeJson(doc, output);
  return output;
}

void seedData() {
  const char* ids[]   = {"s1",           "s2",           "s3",         "s4",              "s5"};
  const char* names[] = {"Maria Garcia", "James Wilson", "Sarah Chen", "Robert Johnson",  "Emily Davis"};
  const char* beds[]  = {"1A",           "2B",           "3A",         "4C",              "5B"};
  const char* rooms[] = {"101",          "102",          "201",        "202",             "301"};

  for (int i = 0; i < 5; i++) {
    strncpy(slaves[i].slaveId, ids[i], sizeof(slaves[i].slaveId));
    strncpy(slaves[i].patientName, names[i], sizeof(slaves[i].patientName));
    strncpy(slaves[i].bed, beds[i], sizeof(slaves[i].bed));
    strncpy(slaves[i].room, rooms[i], sizeof(slaves[i].room));
    slaves[i].alertActive = false;
    slaves[i].registered = true;
    slaves[i].lastAlertTime = 0;
    slaves[i].used = true;
  }
  slaveCount = 5;

  slaves[1].alertActive = true;
  slaves[1].lastAlertTime = millis();
}

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Patient Alarm System</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:#f0f4f8;color:#1a202c;min-height:100vh}
header{background:#fff;border-bottom:1px solid #e2e8f0;padding:16px 24px;position:sticky;top:0;z-index:50;box-shadow:0 1px 3px rgba(0,0,0,.05)}
.header-inner{max-width:1200px;margin:0 auto;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:12px}
.logo{display:flex;align-items:center;gap:12px}
.logo-icon{width:40px;height:40px;background:#2563eb;border-radius:8px;display:flex;align-items:center;justify-content:center;color:#fff;font-size:20px}
.logo h1{font-size:20px;font-weight:700;letter-spacing:-0.5px}
.logo p{font-size:13px;color:#64748b}
.badges{display:flex;gap:8px;flex-wrap:wrap}
.badge{padding:4px 12px;border-radius:20px;font-size:12px;font-weight:600;background:#e2e8f0;color:#475569}
.badge.alert{background:#fef2f2;color:#dc2626;animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.7}}
main{max-width:1200px;margin:0 auto;padding:24px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(280px,1fr));gap:16px}
.card{background:#fff;border-radius:12px;padding:20px;border:1px solid #e2e8f0;transition:all .3s}
.card.alerting{border-color:#ef4444;background:#fef2f2;box-shadow:0 0 0 2px rgba(239,68,68,.2)}
.card.alerting::before{content:'';display:block;height:3px;background:#ef4444;border-radius:3px 3px 0 0;margin:-20px -20px 16px;animation:pulse 2s infinite}
.card-top{display:flex;justify-content:space-between;align-items:flex-start;margin-bottom:16px}
.patient-name{font-size:18px;font-weight:600;margin-bottom:2px}
.patient-id{font-size:13px;color:#94a3b8}
.status-dot{width:48px;height:48px;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:22px;flex-shrink:0}
.status-dot.alert{background:#ef4444;color:#fff;animation:pulse 2s infinite}
.status-dot.connected{background:#dcfce7;color:#16a34a}
.status-dot.offline{background:#f1f5f9;color:#94a3b8}
.alert-badge{display:inline-block;padding:3px 10px;border-radius:4px;font-size:11px;font-weight:700;text-transform:uppercase;letter-spacing:.5px;background:#fef2f2;color:#dc2626;margin-bottom:12px}
.info-row{display:flex;align-items:center;gap:8px;font-size:14px;margin-bottom:6px}
.info-row .label{color:#94a3b8;min-width:36px}
.info-row .value{font-weight:500}
.card-footer{margin-top:12px;padding-top:12px;border-top:1px solid #f1f5f9;display:flex;align-items:center;gap:8px;font-size:12px;color:#94a3b8}
.conn-dot{width:8px;height:8px;border-radius:50%}
.conn-dot.on{background:#22c55e}
.conn-dot.off{background:#94a3b8}
.empty{text-align:center;padding:80px 20px;background:#fff;border-radius:12px;border:1px solid #e2e8f0}
.empty h2{font-size:20px;font-weight:600;margin-bottom:8px}
.empty p{color:#94a3b8}
.nav-link{font-size:13px;color:#2563eb;text-decoration:none;font-weight:500}
.toast-container{position:fixed;top:80px;right:20px;z-index:100;display:flex;flex-direction:column;gap:8px}
.toast{background:#dc2626;color:#fff;padding:12px 20px;border-radius:8px;font-size:14px;font-weight:500;box-shadow:0 4px 12px rgba(0,0,0,.15);animation:slideIn .3s ease}
@keyframes slideIn{from{transform:translateX(100%);opacity:0}to{transform:translateX(0);opacity:1}}
</style>
</head>
<body>
<header>
<div class="header-inner">
<div class="logo">
<div class="logo-icon">&#9829;</div>
<div><h1>Patient Alarm System</h1><p>Real-time monitoring dashboard</p></div>
</div>
<div class="badges" id="badges"></div>
</div>
</header>
<div class="toast-container" id="toasts"></div>
<main><div class="grid" id="grid"></div></main>
<script>
let prevAlerts=new Set();
function showToast(msg){
  const c=document.getElementById('toasts');
  const t=document.createElement('div');
  t.className='toast';t.textContent=msg;
  c.appendChild(t);
  setTimeout(()=>t.remove(),5000);
}
function formatTime(ts){
  if(!ts)return'Never';
  if(typeof ts==='number'){
    let d=new Date();
    let s=Math.floor(ts/1000);
    let m=Math.floor(s/60);let h=Math.floor(m/60);
    s%=60;m%=60;
    return h.toString().padStart(2,'0')+':'+m.toString().padStart(2,'0')+':'+s.toString().padStart(2,'0');
  }
  return new Date(ts).toLocaleString();
}
async function refresh(){
  try{
    const r=await fetch('/api/slaves');
    const slaves=await r.json();
    const grid=document.getElementById('grid');
    const badges=document.getElementById('badges');
    const activeAlerts=slaves.filter(s=>s.alertActive).length;
    const connected=slaves.filter(s=>s.registered).length;
    badges.innerHTML=
      '<span class="badge">'+slaves.length+' Device'+(slaves.length!==1?'s':'')+'</span>'+
      '<span class="badge">'+connected+' Connected</span>'+
      (activeAlerts>0?'<span class="badge alert">&#9888; '+activeAlerts+' Alert'+(activeAlerts!==1?'s':'')+'</span>':'')+
      '<a href="/admin" class="nav-link" style="margin-left:8px">Admin Panel &rarr;</a>';
    const currentAlerts=new Set(slaves.filter(s=>s.alertActive).map(s=>s.slaveId));
    currentAlerts.forEach(id=>{
      if(!prevAlerts.has(id)){
        const sl=slaves.find(s=>s.slaveId===id);
        if(sl)showToast('ALERT: '+sl.patientName+' (Bed '+sl.bed+', Room '+sl.room+')');
      }
    });
    prevAlerts=currentAlerts;
    slaves.sort((a,b)=>{
      if(a.alertActive&&!b.alertActive)return -1;
      if(!a.alertActive&&b.alertActive)return 1;
      return a.slaveId.localeCompare(b.slaveId);
    });
    if(slaves.length===0){
      grid.innerHTML='<div class="empty"><h2>No Devices Registered</h2><p>Add patient devices through the admin panel.</p></div>';
      return;
    }
    grid.innerHTML=slaves.map(s=>`
      <div class="card ${s.alertActive?'alerting':''}">
        <div class="card-top">
          <div>
            <div class="patient-name">${s.patientName}</div>
            <div class="patient-id">ID: ${s.slaveId}</div>
          </div>
          <div class="status-dot ${s.alertActive?'alert':s.registered?'connected':'offline'}">
            ${s.alertActive?'&#128276;':s.registered?'&#9829;':'&#128263;'}
          </div>
        </div>
        ${s.alertActive?'<div class="alert-badge">&#9888; Alert Active</div>':''}
        <div class="info-row"><span class="label">Bed:</span><span class="value">${s.bed}</span></div>
        <div class="info-row"><span class="label">Room:</span><span class="value">${s.room}</span></div>
        <div class="info-row"><span class="label">Last:</span><span class="value">${formatTime(s.lastAlertTime)}</span></div>
        <div class="card-footer">
          <div class="conn-dot ${s.registered?'on':'off'}"></div>
          ${s.registered?'Connected':'Not Connected'}
        </div>
      </div>
    `).join('');
  }catch(e){console.error(e);}
}
refresh();
setInterval(refresh,2000);
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
<title>Admin Panel - Patient Alarm System</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:#f0f4f8;color:#1a202c;min-height:100vh}
header{background:#fff;border-bottom:1px solid #e2e8f0;padding:16px 24px;position:sticky;top:0;z-index:50;box-shadow:0 1px 3px rgba(0,0,0,.05)}
.header-inner{max-width:1200px;margin:0 auto;display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:12px}
.logo{display:flex;align-items:center;gap:12px}
.logo-icon{width:40px;height:40px;background:#2563eb;border-radius:8px;display:flex;align-items:center;justify-content:center;color:#fff;font-size:20px}
.logo h1{font-size:20px;font-weight:700}
.logo p{font-size:13px;color:#64748b}
main{max-width:1200px;margin:0 auto;padding:24px}
.btn{padding:8px 16px;border:none;border-radius:8px;font-size:14px;font-weight:600;cursor:pointer;transition:all .2s;display:inline-flex;align-items:center;gap:6px}
.btn-primary{background:#2563eb;color:#fff}.btn-primary:hover{background:#1d4ed8}
.btn-danger{background:#ef4444;color:#fff}.btn-danger:hover{background:#dc2626}
.btn-secondary{background:#e2e8f0;color:#475569}.btn-secondary:hover{background:#cbd5e1}
.btn-sm{padding:5px 10px;font-size:12px}
.btn-group{display:flex;gap:6px;flex-wrap:wrap}
.login-wrap{display:flex;align-items:center;justify-content:center;min-height:100vh;padding:20px}
.login-card{background:#fff;border-radius:12px;padding:32px;width:100%;max-width:380px;border:1px solid #e2e8f0;box-shadow:0 4px 12px rgba(0,0,0,.05)}
.login-card h2{text-align:center;margin-bottom:4px;font-size:22px}
.login-card p{text-align:center;color:#94a3b8;font-size:13px;margin-bottom:24px}
.login-icon{width:56px;height:56px;background:#eff6ff;border-radius:50%;display:flex;align-items:center;justify-content:center;margin:0 auto 16px;font-size:28px;color:#2563eb}
.form-group{margin-bottom:16px}
.form-group label{display:block;font-size:14px;font-weight:500;margin-bottom:6px}
.form-group input{width:100%;padding:10px 12px;border:1px solid #e2e8f0;border-radius:8px;font-size:14px;outline:none;transition:border .2s}
.form-group input:focus{border-color:#2563eb;box-shadow:0 0 0 3px rgba(37,99,235,.1)}
.error-msg{background:#fef2f2;color:#dc2626;padding:8px 12px;border-radius:6px;font-size:13px;margin-bottom:12px;display:none}
table{width:100%;border-collapse:collapse;background:#fff;border-radius:12px;overflow:hidden;border:1px solid #e2e8f0}
th,td{padding:12px 16px;text-align:left;font-size:14px;border-bottom:1px solid #f1f5f9}
th{background:#f8fafc;font-weight:600;color:#475569;font-size:13px;text-transform:uppercase;letter-spacing:.5px}
tr.alert-row{background:#fef2f2}
.badge{padding:3px 10px;border-radius:20px;font-size:11px;font-weight:600}
.badge-alert{background:#fef2f2;color:#dc2626}
.badge-ok{background:#f0fdf4;color:#16a34a}
.badge-off{background:#f1f5f9;color:#94a3b8}
.toolbar{display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;flex-wrap:wrap;gap:12px}
.toolbar h2{font-size:18px;font-weight:600}
.modal-overlay{position:fixed;inset:0;background:rgba(0,0,0,.4);z-index:100;display:none;align-items:center;justify-content:center;padding:20px}
.modal-overlay.show{display:flex}
.modal{background:#fff;border-radius:12px;padding:24px;width:100%;max-width:420px;box-shadow:0 8px 24px rgba(0,0,0,.15)}
.modal h3{font-size:18px;font-weight:600;margin-bottom:16px}
.modal-actions{display:flex;justify-content:flex-end;gap:8px;margin-top:20px}
.nav-links{display:flex;gap:8px}
.nav-link{font-size:13px;color:#2563eb;text-decoration:none;font-weight:500;padding:6px 12px;background:#eff6ff;border-radius:6px}
.empty{text-align:center;padding:60px 20px;background:#fff;border-radius:12px;border:1px solid #e2e8f0}
.empty p{color:#94a3b8;margin-top:8px;margin-bottom:16px}
@media(max-width:768px){
  table{font-size:12px}
  th,td{padding:8px}
  .hide-mobile{display:none}
}
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
<div><h1>Admin Panel</h1><p>Manage patients &amp; devices</p></div>
</div>
<div class="nav-links">
<a href="/" class="nav-link">&#9829; Dashboard</a>
<a href="#" class="nav-link" onclick="doLogout()">Logout</a>
</div>
</div>
</header>
<main>
<div class="toolbar">
<h2>Patient Devices</h2>
<button class="btn btn-primary" onclick="openAddModal()">+ Add Device</button>
</div>
<div id="table-area"></div>
</main>
</div>

<div class="modal-overlay" id="modal">
<div class="modal">
<h3 id="modal-title">Add Device</h3>
<div class="form-group"><label>Device ID</label><input type="text" id="m-id" placeholder="e.g. s1"></div>
<div class="form-group"><label>Patient Name</label><input type="text" id="m-name" placeholder="Patient name"></div>
<div class="form-group"><label>Bed Number</label><input type="text" id="m-bed" placeholder="e.g. 12A"></div>
<div class="form-group"><label>Room</label><input type="text" id="m-room" placeholder="e.g. 301"></div>
<div class="modal-actions">
<button class="btn btn-secondary" onclick="closeModal()">Cancel</button>
<button class="btn btn-primary" id="modal-submit" onclick="submitModal()">Add Device</button>
</div>
</div>
</div>

<script>
let authToken='';
let editingId=null;
function $(id){return document.getElementById(id)}

async function doLogin(){
  const u=$('username').value;
  const p=$('password').value;
  try{
    const r=await fetch('/api/admin/login',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({username:u,password:p})});
    if(!r.ok){
      $('login-error').style.display='block';
      $('login-error').textContent='Invalid credentials';
      return;
    }
    authToken=btoa(u+':'+p);
    $('login-page').style.display='none';
    $('admin-page').style.display='block';
    loadSlaves();
  }catch(e){$('login-error').style.display='block';$('login-error').textContent='Connection error';}
}

function doLogout(){
  authToken='';
  $('admin-page').style.display='none';
  $('login-page').style.display='flex';
  $('username').value='';$('password').value='';
}

function authHeaders(){
  return{'Content-Type':'application/json','Authorization':'Basic '+authToken};
}

function formatTime(ts){
  if(!ts)return'Never';
  if(typeof ts==='number'){
    let s=Math.floor(ts/1000);let m=Math.floor(s/60);let h=Math.floor(m/60);
    s%=60;m%=60;
    return h.toString().padStart(2,'0')+':'+m.toString().padStart(2,'0')+':'+s.toString().padStart(2,'0');
  }
  return new Date(ts).toLocaleString();
}

async function loadSlaves(){
  try{
    const r=await fetch('/api/slaves');
    const slaves=await r.json();
    const area=$('table-area');
    if(slaves.length===0){
      area.innerHTML='<div class="empty"><h2>No Devices Yet</h2><p>Add your first patient device to begin monitoring.</p><button class="btn btn-primary" onclick="openAddModal()">+ Add Device</button></div>';
      return;
    }
    let html='<table><thead><tr><th>Device ID</th><th>Patient</th><th>Bed</th><th>Room</th><th>Status</th><th class="hide-mobile">Last Alert</th><th>Actions</th></tr></thead><tbody>';
    slaves.forEach(s=>{
      const status=s.alertActive?'<span class="badge badge-alert">&#9888; Alert</span>':s.registered?'<span class="badge badge-ok">Connected</span>':'<span class="badge badge-off">Offline</span>';
      html+=`<tr class="${s.alertActive?'alert-row':''}">
        <td><code>${s.slaveId}</code></td><td>${s.patientName}</td><td>${s.bed}</td><td>${s.room}</td>
        <td>${status}</td><td class="hide-mobile">${formatTime(s.lastAlertTime)}</td>
        <td><div class="btn-group">
          ${s.alertActive?'<button class="btn btn-danger btn-sm" onclick="clearAlert(\''+s.slaveId+'\')">Clear</button>':''}
          <button class="btn btn-secondary btn-sm" onclick="openEditModal(\'${s.slaveId}\',\'${s.patientName}\',\'${s.bed}\',\'${s.room}\')">Edit</button>
          <button class="btn btn-secondary btn-sm" onclick="deleteSlave(\'${s.slaveId}\')">Del</button>
        </div></td></tr>`;
    });
    html+='</tbody></table>';
    area.innerHTML=html;
  }catch(e){console.error(e);}
}

async function clearAlert(id){
  await fetch('/api/clearAlert/'+id,{method:'POST',headers:authHeaders()});
  loadSlaves();
}

async function deleteSlave(id){
  if(!confirm('Delete device '+id+'?'))return;
  await fetch('/api/slaves/'+id,{method:'DELETE',headers:authHeaders()});
  loadSlaves();
}

function openAddModal(){
  editingId=null;
  $('modal-title').textContent='Add New Device';
  $('modal-submit').textContent='Add Device';
  $('m-id').value='';$('m-name').value='';$('m-bed').value='';$('m-room').value='';
  $('m-id').disabled=false;
  $('modal').classList.add('show');
}

function openEditModal(id,name,bed,room){
  editingId=id;
  $('modal-title').textContent='Edit Patient - '+id;
  $('modal-submit').textContent='Save Changes';
  $('m-id').value=id;$('m-id').disabled=true;
  $('m-name').value=name;$('m-bed').value=bed;$('m-room').value=room;
  $('modal').classList.add('show');
}

function closeModal(){$('modal').classList.remove('show');}

async function submitModal(){
  const id=$('m-id').value.trim();
  const name=$('m-name').value.trim();
  const bed=$('m-bed').value.trim();
  const room=$('m-room').value.trim();
  if(!name||!bed||!room||(editingId===null&&!id)){alert('Fill all fields');return;}
  if(editingId){
    await fetch('/api/slaves/'+editingId,{method:'PUT',headers:authHeaders(),body:JSON.stringify({patientName:name,bed:bed,room:room})});
  }else{
    await fetch('/api/slaves',{method:'POST',headers:authHeaders(),body:JSON.stringify({slaveId:id,patientName:name,bed:bed,room:room})});
  }
  closeModal();
  loadSlaves();
}

$('password').addEventListener('keypress',e=>{if(e.key==='Enter')doLogin();});
setInterval(()=>{if($('admin-page').style.display!=='none')loadSlaves();},3000);
</script>
</body>
</html>
)rawliteral";


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Hospital Patient Alarm System ===");
  Serial.println("Starting ESP32-S3 Master...");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  memset(slaves, 0, sizeof(slaves));

  seedData();

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(
    IPAddress(192, 168, 4, 1),
    IPAddress(192, 168, 4, 1),
    IPAddress(255, 255, 255, 0)
  );
  delay(100);
  WiFi.softAP(AP_SSID, NULL, 1, 0, 4);
  delay(500);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.println("AP Mode: Open (no password)");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", DASHBOARD_HTML);
  });

  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", ADMIN_HTML);
  });

  server.on("/api/slaves", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = getSlavesJson();
    request->send(200, "application/json", json);
  });

  AsyncCallbackJsonWebHandler* registerHandler = new AsyncCallbackJsonWebHandler("/api/register",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* slaveId = obj["slaveId"] | "";

      if (strlen(slaveId) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing slaveId\"}");
        return;
      }

      int idx = findSlaveIndex(slaveId);
      if (idx < 0) {
        request->send(404, "application/json", "{\"success\":false,\"message\":\"Unknown slave ID\"}");
        return;
      }

      slaves[idx].registered = true;
      request->send(200, "application/json", "{\"success\":true,\"message\":\"Registered\"}");
      Serial.printf("Slave %s registered\n", slaveId);
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

      if (slaves[idx].alertActive) {
        request->send(200, "application/json", "{\"success\":false,\"reason\":\"alert already active\"}");
        return;
      }

      slaves[idx].alertActive = true;
      slaves[idx].lastAlertTime = millis();

      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);

      String resp = "{\"success\":true,\"alertId\":\"alert-" + String(slaveId) + "-" + String(millis()) + "\"}";
      request->send(200, "application/json", resp);
      Serial.printf("ALERT from slave %s!\n", slaveId);
    }
  );
  server.addHandler(alertHandler);

  AsyncCallbackJsonWebHandler* loginHandler = new AsyncCallbackJsonWebHandler("/api/admin/login",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* user = obj["username"] | "";
      const char* pass = obj["password"] | "";

      if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Logged in\"}");
      } else {
        request->send(401, "application/json", "{\"message\":\"Invalid credentials\"}");
      }
    }
  );
  server.addHandler(loginHandler);

  AsyncCallbackJsonWebHandler* addSlaveHandler = new AsyncCallbackJsonWebHandler("/api/slaves",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }

      JsonObject obj = json.as<JsonObject>();
      const char* slaveId = obj["slaveId"] | "";
      const char* name = obj["patientName"] | "";
      const char* bed = obj["bed"] | "";
      const char* room = obj["room"] | "";

      if (strlen(slaveId) == 0 || strlen(name) == 0) {
        request->send(400, "application/json", "{\"message\":\"Missing required fields\"}");
        return;
      }

      if (findSlaveIndex(slaveId) >= 0) {
        request->send(409, "application/json", "{\"message\":\"Slave ID already exists\"}");
        return;
      }

      int slot = findFreeSlot();
      if (slot < 0) {
        request->send(507, "application/json", "{\"message\":\"Max slaves reached\"}");
        return;
      }

      strncpy(slaves[slot].slaveId, slaveId, sizeof(slaves[slot].slaveId));
      strncpy(slaves[slot].patientName, name, sizeof(slaves[slot].patientName));
      strncpy(slaves[slot].bed, bed, sizeof(slaves[slot].bed));
      strncpy(slaves[slot].room, room, sizeof(slaves[slot].room));
      slaves[slot].alertActive = false;
      slaves[slot].registered = false;
      slaves[slot].lastAlertTime = 0;
      slaves[slot].used = true;
      slaveCount++;

      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("Added slave %s (%s)\n", slaveId, name);
    }
  );
  server.addHandler(addSlaveHandler);

  server.on("/api/clearAlert/*", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }

    String url = request->url();
    int lastSlash = url.lastIndexOf('/');
    String slaveId = url.substring(lastSlash + 1);

    int idx = findSlaveIndex(slaveId.c_str());
    if (idx < 0) {
      request->send(404, "application/json", "{\"message\":\"Slave not found\"}");
      return;
    }

    slaves[idx].alertActive = false;
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Alert cleared\"}");
    Serial.printf("Alert cleared for %s\n", slaveId.c_str());
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    String url = request->url();

    if (request->method() == HTTP_DELETE && url.startsWith("/api/slaves/")) {
      if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }

      String slaveId = url.substring(12);
      int idx = findSlaveIndex(slaveId.c_str());
      if (idx < 0) {
        request->send(404, "application/json", "{\"message\":\"Slave not found\"}");
        return;
      }
      slaves[idx].used = false;
      memset(&slaves[idx], 0, sizeof(Slave));
      slaveCount--;
      request->send(200, "application/json", "{\"success\":true,\"message\":\"Deleted\"}");
      Serial.printf("Deleted slave %s\n", slaveId.c_str());
      return;
    }

    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
      return;
    }

    request->send(404, "application/json", "{\"message\":\"Not found\"}");
  });

  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String url = request->url();

    if (request->method() == HTTP_PUT && url.startsWith("/api/slaves/")) {
      if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }

      String slaveId = url.substring(12);
      int idx = findSlaveIndex(slaveId.c_str());
      if (idx < 0) {
        request->send(404, "application/json", "{\"message\":\"Slave not found\"}");
        return;
      }

      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, (const char*)data, len);
      if (err) {
        request->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        return;
      }

      if (doc.containsKey("patientName")) {
        strncpy(slaves[idx].patientName, doc["patientName"] | "", sizeof(slaves[idx].patientName));
      }
      if (doc.containsKey("bed")) {
        strncpy(slaves[idx].bed, doc["bed"] | "", sizeof(slaves[idx].bed));
      }
      if (doc.containsKey("room")) {
        strncpy(slaves[idx].room, doc["room"] | "", sizeof(slaves[idx].room));
      }

      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("Updated slave %s\n", slaveId.c_str());
    }
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  server.begin();

  Serial.println("Web server started on port 80");
  Serial.println("Connect to Wi-Fi: " + String(AP_SSID) + " / " + String(AP_PASSWORD));
  Serial.println("Open http://192.168.4.1 in your browser");
}

void loop() {
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
