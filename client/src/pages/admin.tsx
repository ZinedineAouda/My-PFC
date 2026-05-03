import { useState, useEffect, useRef } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import type { Device } from "@shared/schema";
import { apiRequest, apiUrl, getQueryFn } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import { useWebsocket } from "@/hooks/use-websocket";
import { Shield, LogIn, LogOut, Wifi, Trash2, Pencil, Activity, Database, Settings, AlertTriangle, CheckCircle2, Navigation, RefreshCw } from "lucide-react";
import { LocalNotifications } from "@capacitor/local-notifications";
import { Haptics } from "@capacitor/haptics";

// ═══════════════════════════════════════════════════════════════════
//  DESIGN TOKENS (Clinical Modern)
// ═══════════════════════════════════════════════════════════════════
const THEME = {
  primary: "#2563EB", // Clinical Blue
  danger: "#DC2626",  // Emergency Red
  bg: "#F8FAFC",      // Clean Slate
  card: "#FFFFFF",
  muted: "#64748B",
  border: "transparent", 
  shadow: "0 20px 25px -5px rgb(0 0 0 / 0.05), 0 8px 10px -6px rgb(0 0 0 / 0.05)"
};

// ═══════════════════════════════════════════════════════════════════
//  SETUP WIZARD (Premium Step Interaction)
// ═══════════════════════════════════════════════════════════════════
function SetupWizard({ onComplete }: { onComplete: () => void }) {
  const { toast } = useToast();
  const [step, setStep] = useState(1);
  const [mode, setMode] = useState(1);
  const [apSsid, setApSsid] = useState("HospitalAlarm");
  const [apPass, setApPass] = useState("");
  const [staSsid, setStaSsid] = useState("");
  const [staPass, setStaPass] = useState("");
  const [loading, setLoading] = useState(false);

  const { data: networks } = useQuery<any[]>({
    queryKey: ["/api/scan"],
    queryFn: async () => { 
      const res = await fetch(apiUrl("/api/scan")); 
      return res.json(); 
    },
    refetchInterval: (mode > 1 && step === 3) ? 5000 : false,
    enabled: mode > 1
  });

  const handleFinish = async () => {
    setLoading(true);
    try {
      if (mode > 1) { 
        await apiRequest("POST", "/api/connect-wifi", { ssid: staSsid, password: staPass });
      }
      await apiRequest("POST", "/api/setup", { mode, apSSID: apSsid, apPass: apPass });
      toast({ title: "Configuration Deployed", description: "Node is re-provisioning. Re-link to the new Controller SSID." });
      onComplete();
    } catch (e) {
      toast({ title: "Sync Failed", description: "Hardware was unreachable.", variant: "destructive" });
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-slate-50 flex items-center justify-center p-6 text-slate-900">
      <div className="w-full max-w-xl bg-white rounded-[40px] shadow-[0_40px_80px_-15px_rgba(0,0,0,0.08)] overflow-hidden">
        {/* Header Section */}
        <div className="bg-blue-600 p-12 text-white relative">
          <div className="absolute top-0 right-0 p-8 opacity-20"><Navigation className="w-24 h-24 rotate-45" /></div>
          <div className="relative z-10 flex items-center gap-4 mb-6">
            <div className="w-12 h-12 bg-white/20 backdrop-blur-md rounded-2xl flex items-center justify-center shadow-inner">
              <Shield className="w-6 h-6" />
            </div>
            <div>
              <p className="text-[10px] font-bold uppercase tracking-[4px] opacity-70">System</p>
              <h1 className="text-3xl font-bold tracking-tight uppercase">Setup Controller</h1>
            </div>
          </div>
          <div className="flex gap-2">
            {[1, 2, 3].map(s => (
              <div key={s} className={`h-1.5 rounded-full transition-all duration-700 ${step >= s ? "w-12 bg-white" : "w-4 bg-white/30"}`} />
            ))}
          </div>
        </div>

        <div className="p-12">
          {step === 1 && (
            <div className="space-y-8 animate-in fade-in slide-in-from-right-8 duration-500">
              <header>
                <h2 className="text-2xl font-bold">Connection Type</h2>
                <p className="text-slate-500 text-sm mt-1">Select how this device will link to the system.</p>
              </header>
              <div className="grid gap-3">
                {[
                   { m: 1, t: "Use just Controller Unit", d: "Private network for this building. No cloud.", icon: <Shield className="w-4 h-4" /> },
                   { m: 2, t: "Join Hospital WiFi", d: "Connect to the building's existing network.", icon: <Database className="w-4 h-4" /> },
                   { m: 3, t: "Bridge Mode", d: "Hybrid: Joins hospital WiFi while creating its own.", icon: <Wifi className="w-4 h-4" /> },
                   { m: 4, t: "Full Cloud Access", d: "Securely link to the internet for remote access.", icon: <Activity className="w-4 h-4" /> }
                ].map(opt => (
                  <button 
                    key={opt.m}
                    onClick={() => setMode(opt.m)}
                    className={`group w-full p-5 rounded-[24px] border-2 text-left transition-all relative overflow-hidden ${mode === opt.m ? "border-blue-600 bg-blue-50/50 shadow-lg shadow-blue-500/5" : "border-slate-100 hover:border-slate-300 bg-slate-50/30"}`}
                  >
                    <div className="flex items-center justify-between mb-1">
                      <span className={`text-[9px] font-bold uppercase tracking-widest ${mode === opt.m ? "text-blue-600" : "text-slate-400"}`}>Selection</span>
                      <div className={`p-1.5 rounded-lg ${mode === opt.m ? "bg-blue-600 text-white" : "bg-slate-200 text-slate-500"}`}>{opt.icon}</div>
                    </div>
                    <div className="font-bold text-slate-900">{opt.t}</div>
                    <div className="text-slate-500 text-[11px] font-medium leading-relaxed mt-1 opacity-80">{opt.d}</div>
                  </button>
                ))}
              </div>
              <button onClick={() => setStep(2)} className="w-full py-6 bg-slate-900 text-white font-bold uppercase text-xs tracking-[3px] rounded-[24px] shadow-2xl hover:translate-y-[-2px] active:translate-y-1 transition-all">Next</button>
            </div>
          )}

          {step === 2 && (
            <div className="space-y-8 animate-in fade-in slide-in-from-right-8 duration-500">
              <header>
                <h2 className="text-2xl font-bold">WiFi Setup</h2>
                <p className="text-slate-500 text-sm mt-1">Choose a name for your new Controller unit.</p>
              </header>
              <div className="space-y-5">
                <div className="space-y-2">
                  <label className="text-[10px] font-bold uppercase tracking-widest text-slate-400 ml-1">WiFi Name</label>
                  <input 
                    className="w-full p-6 bg-slate-50 border-2 border-slate-100 rounded-[24px] focus:border-blue-600 focus:bg-white outline-none transition-all font-bold text-lg placeholder:text-slate-300" 
                    placeholder="e.g. HospitalAlarm" 
                    value={apSsid} onChange={e => setApSsid(e.target.value)} 
                  />
                </div>
                <div className="space-y-2">
                  <label className="text-[10px] font-bold uppercase tracking-widest text-slate-400 ml-1">WiFi Password</label>
                  <input 
                    className="w-full p-6 bg-slate-50 border-2 border-slate-100 rounded-[24px] focus:border-blue-600 focus:bg-white outline-none transition-all font-bold text-lg placeholder:text-slate-300"
                    type="password" placeholder="At least 8 letters" value={apPass} onChange={e => setApPass(e.target.value)} 
                  />
                </div>
              </div>
              <div className="flex gap-4">
                <button onClick={() => setStep(1)} className="flex-1 py-6 bg-slate-100 text-slate-500 font-bold uppercase text-xs rounded-[24px]">Back</button>
                <button onClick={() => setStep(3)} className="flex-[2] py-6 bg-slate-900 text-white font-bold uppercase text-xs tracking-[3px] rounded-[24px] shadow-xl">Confirm</button>
              </div>
            </div>
          )}

          {step === 3 && (
            <div className="space-y-8 animate-in fade-in slide-in-from-right-8 duration-500">
              <header>
                <h2 className="text-2xl font-bold">Building WiFi</h2>
                <p className="text-slate-500 text-sm mt-1">Select the hospital network to connect with.</p>
              </header>
              {mode !== 1 ? (
                <div className="space-y-5">
                  <div className="max-h-[250px] overflow-y-auto space-y-2 pr-2 scrollbar-hide">
                    {!networks?.length && <div className="py-12 text-center text-slate-400 font-bold animate-pulse uppercase tracking-[2px] text-xs">Scanning...</div>}
                    {networks?.map(n => (
                      <button 
                        key={n.ssid} 
                        onClick={() => setStaSsid(n.ssid)} 
                        className={`w-full text-left p-5 rounded-[24px] flex items-center justify-between transition-all active:scale-[0.98] ${staSsid === n.ssid ? "bg-blue-600 text-white shadow-xl shadow-blue-500/30" : "bg-slate-50 border-2 border-slate-100 hover:border-slate-300"}`}
                      >
                        <div className="flex items-center gap-4">
                          <Wifi className={`w-5 h-5 ${staSsid === n.ssid ? "text-white" : "text-blue-600"}`} />
                          <span className="font-bold text-sm tracking-tight">{n.ssid}</span>
                        </div>
                        <span className={`text-[10px] font-bold ${staSsid === n.ssid ? "opacity-60" : "text-slate-400"}`}>{n.rssi} dBm</span>
                      </button>
                    ))}
                  </div>
                  <input 
                    className="w-full p-6 bg-slate-50 border-2 border-slate-100 rounded-[24px] focus:border-blue-600 focus:bg-white outline-none transition-all font-bold placeholder:text-slate-300" 
                    placeholder="WiFi Password..." type="password" value={staPass} onChange={e => setStaPass(e.target.value)} 
                  />
                </div>
              ) : (
                <div className="text-center py-16 bg-blue-50/50 rounded-[40px] border-4 border-dashed border-blue-100/50 flex flex-col items-center">
                  <div className="w-20 h-20 bg-white rounded-3xl shadow-lg flex items-center justify-center mb-6">
                    <Shield className="w-10 h-10 text-blue-600" />
                  </div>
                  <p className="text-blue-900 font-bold uppercase text-sm tracking-[2px]">Private Mode</p>
                  <p className="text-blue-600/60 text-xs px-12 mt-2 font-medium italic">This device will work as its own hub without internet.</p>
                </div>
              )}
              <div className="flex gap-4">
                <button onClick={() => setStep(2)} className="flex-1 py-6 bg-slate-100 text-slate-500 font-bold uppercase text-xs rounded-[24px]">Back</button>
                <button 
                  onClick={handleFinish} disabled={loading} 
                  className="flex-[2] py-6 bg-blue-600 text-white font-bold uppercase text-xs tracking-[4px] rounded-[24px] shadow-2xl shadow-blue-600/30 disabled:opacity-50"
                >
                  {loading ? "Joining..." : "Finish"}
                </button>
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

// ═══════════════════════════════════════════════════════════════════
//  LOGIN FORM (Clean Modern)
// ═══════════════════════════════════════════════════════════════════
function LoginForm({ onLogin }: { onLogin: () => void }) {
  const { toast } = useToast();
  const [user, setUser] = useState("");
  const [pass, setPass] = useState("");
  const [loading, setLoading] = useState(false);

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    setLoading(true);
    try {
      const res = await fetch(apiUrl("/api/admin/login"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        credentials: "include",
        body: JSON.stringify({ username: user, password: pass }),
      });
      if (!res.ok) throw new Error(res.status === 401 ? "Incorrect password" : "Error");
      onLogin();
      toast({ title: "Authorized", description: "Dashboard ready." });
    } catch (err: any) {
      toast({ title: "Authorized Error", description: err.message, variant: "destructive" });
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-white flex items-center justify-center p-6 text-slate-900">
      <div className="w-full max-sm:max-w-xs max-w-sm">
        <div className="text-center mb-12">
          <div className="w-20 h-20 bg-blue-600 text-white rounded-[30px] flex items-center justify-center mx-auto mb-6 shadow-[0_20px_40px_-5px_rgba(37,99,235,0.4)]">
            <Shield className="w-10 h-10" />
          </div>
          <h1 className="text-4xl font-bold tracking-tighter">Login</h1>
          <p className="text-blue-600/50 text-[10px] font-bold uppercase tracking-[5px] mt-3">Clinical Staff Only</p>
        </div>

        <form onSubmit={handleLogin} className="space-y-4">
          <div className="space-y-2">
            <label className="text-[10px] font-bold uppercase tracking-widest text-slate-400 ml-1">Username</label>
            <input 
              className="w-full p-5 bg-slate-50 border-2 border-slate-100 rounded-3xl outline-none focus:border-blue-600 focus:bg-white transition-all font-bold" 
              value={user} onChange={e => setUser(e.target.value)} placeholder="admin"
            />
          </div>
          <div className="space-y-2">
            <label className="text-[10px] font-bold uppercase tracking-widest text-slate-400 ml-1">Password</label>
            <input 
              type="password"
              className="w-full p-5 bg-slate-50 border-2 border-slate-100 rounded-3xl outline-none focus:border-blue-600 focus:bg-white transition-all font-bold" 
              value={pass} onChange={e => setPass(e.target.value)}
            />
          </div>
          <button 
            type="submit" disabled={loading}
            className="w-full py-5 bg-slate-900 text-white font-bold uppercase text-xs tracking-[4px] rounded-3xl shadow-2xl hover:translate-y-[-2px] transition-all disabled:opacity-50 mt-4"
          >
            {loading ? "Please wait..." : "Continue"}
          </button>
        </form>
      </div>
    </div>
  );
}

// ═══════════════════════════════════════════════════════════════════
//  ADMIN PANEL (Clinical Dashboard)
// ═══════════════════════════════════════════════════════════════════
function AdminPanel() {
  const { toast } = useToast();
  const queryClient = useQueryClient();
  const [modalDevice, setModalDevice] = useState<Device | null>(null);
  const [modalMode, setModalMode] = useState<"approve" | "edit">("approve");
  const [isSyncing, setIsSyncing] = useState(false);
  const sirenRef = useRef<HTMLAudioElement | null>(null);

  useEffect(() => {
    sirenRef.current = new Audio("https://raw.githubusercontent.com/Anis-Aouda/Zinedine-Audio/main/siren.mp3");
    sirenRef.current.loop = true;
    return () => { if (sirenRef.current) sirenRef.current.pause(); };
  }, []);

  useWebsocket((msg) => {
    queryClient.invalidateQueries({ queryKey: ["/api/devices"] });
    queryClient.invalidateQueries({ queryKey: ["/api/status"] });
    if (msg.type === "ALERT" || msg.type === "UPDATE") {
       const device = msg.payload;
       if (device?.alertActive) {
          if (sirenRef.current) sirenRef.current.play().catch(() => {});
          Haptics.vibrate({ duration: 1000 }).catch(() => {});
          toast({ title: "EMERGENCY ALARM", description: `Helping: ${device.patientName || device.deviceId}`, variant: "destructive" });
       }
    }
  });

  const { data: devices } = useQuery<Device[]>({
    queryKey: ["/api/devices", "all"],
    queryFn: async () => { const res = await fetch(apiUrl("/api/devices?all=1"), { credentials: "include" }); return res.json(); },
    refetchInterval: 5000,
  });

  const { data: status } = useQuery<{ 
    mode: number; 
    uptime: number; 
    rssi: number; 
    devices: number; 
    online: number; 
    alerts: number;
    isControllerOnline?: boolean;
    controllerIP?: string;
    wifiError?: string;
  }>({
    queryKey: ["/api/status"],
    queryFn: async () => { const res = await fetch(apiUrl("/api/status"), { credentials: "include" }); return res.json(); },
    refetchInterval: 2000,
  });

  const isCloud = status?.controllerIP === "cloud";
  const isCloudActive = isCloud ? !!status?.isControllerOnline : status?.mode === 4;

  const logoutMutation = useMutation({
    mutationFn: async () => { await apiRequest("POST", "/api/admin/logout"); },
    onSuccess: () => window.location.reload(),
  });

  const safeDevices = Array.isArray(devices) ? devices : [];
  const pending = safeDevices.filter(s => !s.approved);
  const approved = safeDevices.filter(s => s.approved);

  return (
    <div className="min-h-screen bg-[#F8FAFC] text-slate-900 pb-32 font-sans">
      {/* Premium Header */}
      <header className="bg-white px-8 py-6 border-b border-slate-100 flex justify-between items-center sticky top-0 z-40">
        <div className="flex items-center gap-4">
          <div className="w-10 h-10 bg-blue-600 rounded-2xl flex items-center justify-center text-white shadow-lg shadow-blue-600/20">
            <Shield className="w-6 h-6" />
          </div>
          <div>
            <h1 className="text-xl font-bold tracking-tight text-slate-900">Hospital Dashboard</h1>
            <p className="text-[10px] font-bold text-blue-600 opacity-60">Live Monitoring</p>
          </div>
        </div>
        
        <div className="flex items-center gap-6">
            <div className="flex items-center gap-2">
              <div className={`w-2 h-2 rounded-full ${isCloudActive ? "bg-emerald-500" : (isCloud ? "bg-red-500" : "bg-amber-500")}`} />
              <span className="text-[10px] font-bold">
                {isCloudActive ? (status?.wifiError === "Connected" || !status?.wifiError ? "All Syncing" : `WiFi Error: ${status.wifiError}`) : (isCloud ? "Main unit offline" : "Setup needed")}
              </span>
            </div>
            <div className="flex items-center gap-2 mt-1">
               <p className="text-[9px] text-slate-400 font-bold">Online: {Math.floor((status?.uptime || 0) / 60)} min</p>
               <span className="text-[9px] text-slate-300 font-bold">|</span>
               <p className="text-[9px] text-slate-400 font-bold">Signal: {status?.rssi || 0} dBm</p>
               {isCloudActive && (
                 <>
                   <span className="text-[9px] text-slate-300 font-bold">|</span>
                   <button 
                     onClick={() => {
                       setIsSyncing(true);
                       apiRequest("POST", "/api/system/sync")
                         .finally(() => setTimeout(() => setIsSyncing(false), 1000));
                     }}
                     className="flex items-center gap-1 group"
                   >
                     <RefreshCw className={`w-[10px] h-[10px] text-blue-600 group-hover:text-blue-700 ${isSyncing ? "animate-spin" : ""}`} />
                     <span className="text-[9px] text-blue-600 font-bold group-hover:text-blue-700">Sync Now</span>
                   </button>
                 </>
               )}
            </div>
          </div>
          <button onClick={() => logoutMutation.mutate()} className="w-10 h-10 bg-slate-50 border border-slate-200 rounded-2xl flex items-center justify-center text-slate-400 hover:text-red-500 hover:bg-red-50 transition-all">
            <LogOut className="w-5 h-5" />
          </button>
      </header>
      <main className="p-8 max-w-7xl mx-auto space-y-8 animate-in fade-in duration-500">
          {/* Stats Grid */}
          <div className="grid grid-cols-2 lg:grid-cols-4 gap-6">
            {[
              { l: "Total rooms", v: status?.devices || 0, i: <Database className="w-5 h-5" />, c: "text-slate-900" },
              { l: "Rooms online", v: status?.online || 0, i: <Wifi className="w-5 h-5" />, c: "text-blue-600" },
              { l: "Helping cases", v: status?.alerts || 0, i: <AlertTriangle className="w-5 h-5" />, c: "text-red-600" },
              { l: "Found devices", v: pending.length, i: <Settings className="w-5 h-5" />, c: "text-amber-500" }
            ].map((s, idx) => (
              <div key={idx} className="bg-white p-6 rounded-[32px] shadow-[0_10px_20px_-10px_rgba(0,0,0,0.03)] border-b-4 border-slate-100">
                <div className="flex justify-between items-start mb-2">
                  <div className={`p-2 rounded-xl bg-slate-50 ${s.c}`}>{s.i}</div>
                  <span className={`text-2xl font-bold ${s.c}`}>{s.v}</span>
                </div>
                <div className="text-[11px] font-bold text-slate-400 uppercase tracking-wider">{s.l}</div>
              </div>
            ))}
          </div>

          {/* Pending Queue */}
          {pending.length > 0 && (
            <div className="space-y-4">
              <h2 className="text-[11px] font-bold text-slate-400 ml-2 uppercase tracking-widest">New Units Detected</h2>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                {pending.map(s => (
                  <div key={s.deviceId} className="bg-amber-50 border-2 border-amber-100 p-6 rounded-[32px] flex justify-between items-center">
                    <div className="flex items-center gap-4">
                      <div className="w-12 h-12 bg-white rounded-2xl flex items-center justify-center text-amber-600 shadow-sm border border-amber-100">
                        <Wifi className="w-6 h-6" />
                      </div>
                      <div>
                        <div className="font-mono font-bold text-amber-900 text-lg uppercase">{s.deviceId}</div>
                        <p className="text-[10px] font-bold text-amber-700/60 mt-0.5">Found and waiting</p>
                      </div>
                    </div>
                    <button 
                      onClick={() => { setModalDevice(s); setModalMode("approve"); }}
                      className="bg-amber-600 text-white px-8 py-3 rounded-2xl text-[11px] font-bold shadow-lg shadow-amber-600/30 hover:scale-[1.02] active:scale-[0.98] transition-all"
                    >
                      Connect Patient
                    </button>
                    <button 
                      onClick={() => { if(confirm(`Ignore this unit?`)) apiRequest("DELETE", "/api/devices/" + s.deviceId); }}
                      className="ml-2 w-12 h-12 bg-white text-slate-300 rounded-2xl flex items-center justify-center hover:bg-red-50 hover:text-red-500 transition-all border border-amber-100"
                    >
                      <Trash2 size={18} />
                    </button>
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* Active Nodes */}
          <div className="space-y-4">
             <h2 className="text-[11px] font-bold text-slate-400 ml-2 uppercase tracking-widest">Active patient units</h2>
             <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
                {approved.map(d => (
                  <div key={d.deviceId} className={`group bg-white p-6 rounded-[40px] border-2 transition-all duration-500 shadow-[0_20px_40px_-10px_rgba(0,0,0,0.02)] ${d.alertActive ? "border-red-500 bg-red-50/30 shadow-red-500/10" : "border-slate-50 hover:border-blue-100 hover:shadow-blue-500/5 hover:-translate-y-1"}`}>
                    <div className="flex justify-between items-start mb-6">
                      <div className="flex items-center gap-4">
                        <div className={`w-14 h-14 rounded-3xl flex items-center justify-center transition-colors ${d.alertActive ? "bg-red-600 text-white" : "bg-blue-50 text-blue-600"}`}>
                          {d.alertActive ? <AlertTriangle className="w-8 h-8" /> : <Activity className="w-8 h-8" />}
                        </div>
                        <div>
                          <div className={`font-bold text-xl tracking-tight ${d.alertActive ? "text-red-900" : "text-slate-900"}`}>{d.patientName}</div>
                          <div className="flex items-center gap-2 mt-1">
                            <span className="text-[10px] font-bold text-slate-400 uppercase tracking-widest">Room: {d.room}</span>
                            <span className="w-1 h-1 rounded-full bg-slate-300" />
                            <span className="text-[10px] font-bold text-slate-400 uppercase tracking-widest">Bed: {d.bed}</span>
                          </div>
                        </div>
                      </div>
                      <div className={`px-3 py-1.5 rounded-xl text-[9px] font-bold uppercase tracking-wider ${d.alertActive ? "bg-red-600 text-white" : "bg-emerald-50 text-emerald-600"}`}>
                        {d.alertActive ? "HELPING" : "OK"}
                      </div>
                    </div>

                    <div className="flex gap-3">
                      <button 
                        onClick={() => { setModalDevice(d); setModalMode("edit"); }}
                        className="w-12 h-12 bg-slate-50 text-slate-400 rounded-2xl flex items-center justify-center hover:bg-slate-100 hover:text-slate-900 transition-all border border-slate-100"
                      >
                        <Pencil size={18} />
                      </button>
                      {d.alertActive ? (
                        <button 
                          onClick={() => apiRequest("POST", "/api/clearAlert", { deviceId: d.deviceId })}
                          className="flex-1 bg-red-600 text-white font-bold text-[10px] rounded-2xl shadow-xl shadow-red-600/30 hover:bg-red-700 active:translate-y-1 transition-all uppercase tracking-[2px]"
                        >
                          Stop Alarm
                        </button>
                      ) : (
                        <div className="flex-1" />
                      )}
                      <button 
                        onClick={() => { 
                          if(confirm(`Remove patient unit?`)) {
                            if (isCloud) {
                              apiRequest("DELETE", "/api/devices/" + d.deviceId);
                            } else {
                              apiRequest("POST", "/api/delete-device", { deviceId: d.deviceId });
                            }
                          }
                        }}
                        className="w-12 h-12 bg-slate-50 text-slate-300 rounded-2xl flex items-center justify-center hover:bg-red-50 hover:text-red-500 transition-all border border-slate-100"
                      >
                        <Trash2 size={18} />
                      </button>
                    </div>
                  </div>
                ))}
             </div>
             {approved.length === 0 && (
               <div className="text-center py-24 bg-white/50 rounded-[60px] border-4 border-dashed border-slate-100">
                 <Database className="w-16 h-16 text-slate-200 mx-auto mb-6" />
                 <p className="text-slate-400 font-bold uppercase tracking-[4px] text-xs">No active units</p>
               </div>
             )}
          </div>

          {/* Maintenance Section (Local Only) */}
          {!isCloud && (
            <div className="space-y-4 pt-12 animate-in fade-in slide-in-from-bottom-8 duration-700">
               <div className="flex items-center gap-3 ml-2 mb-2">
                 <div className="w-1.5 h-1.5 rounded-full bg-slate-300" />
                 <h2 className="text-[11px] font-bold text-slate-400 uppercase tracking-widest">System Settings</h2>
               </div>
               
               <div className="max-w-xl mx-auto">
                 <div className="bg-white p-8 rounded-[40px] border-2 border-red-50 relative overflow-hidden group hover:border-red-100 transition-all shadow-[0_20px_40px_-15px_rgba(220,38,38,0.02)]">
                    <div className="absolute top-0 right-0 p-6 opacity-5 group-hover:opacity-10 transition-opacity text-red-600"><AlertTriangle className="w-24 h-24" /></div>
                    <div className="relative z-10 text-center py-4">
                      <div className="w-16 h-16 bg-red-50 text-red-600 rounded-2xl flex items-center justify-center mx-auto mb-6 shadow-inner">
                        <Trash2 className="w-8 h-8" />
                      </div>
                      <h3 className="text-xl font-bold mb-2 text-slate-900 tracking-tight">Factory Reset</h3>
                      <p className="text-xs text-slate-400 mb-8 leading-relaxed mx-auto max-w-[320px]">
                        This will delete ALL database records, forget WiFi settings, and reset the device mode. 
                        The unit will restart and enter the initial Setup Wizard.
                      </p>
                      <button 
                        onClick={() => {
                          if(confirm("CRITICAL ACTION: This will erase ALL system data and reset WiFi. Are you sure you want to continue?")) {
                            apiRequest("POST", "/api/system/reset").then(() => window.location.reload());
                          }
                        }}
                        className="w-full py-5 bg-red-600 text-white font-bold uppercase text-[10px] tracking-[4px] rounded-2xl shadow-xl shadow-red-600/20 hover:scale-[1.01] active:scale-[0.99] transition-all"
                      >
                        Start Wipe Procedure
                      </button>
                    </div>
                 </div>
               </div>
            </div>
          )}
        </main>

      {/* Modal Overlay */}
      <DeviceModal
        title={modalMode === "approve" ? "Add Patient" : "Edit Details"}
        open={!!modalDevice}
        onClose={() => setModalDevice(null)}
        onSubmit={(name: string, bed: string, room: string) => {
          if (!modalDevice) return;
          const url = modalMode === "approve" ? `/api/approve` : `/api/update`;
          apiRequest("POST", url, { deviceId: modalDevice.deviceId, patientName: name, bed, room })
            .then(() => { queryClient.invalidateQueries(); setModalDevice(null); });
        }}
        loading={false}
        defaults={modalMode === "edit" ? (modalDevice ?? undefined) : undefined}
      />
    </div>
  );
}

function DeviceModal({ title, open, onClose, onSubmit, defaults }: any) {
  const [name, setName] = useState(defaults?.patientName || "");
  const [bed, setBed] = useState(defaults?.bed || "");
  const [room, setRoom] = useState(defaults?.room || "");

  useEffect(() => {
    if (defaults) {
      setName(defaults.patientName || "");
      setBed(defaults.bed || "");
      setRoom(defaults.room || "");
    }
  }, [defaults]);

  if (!open) return null;

  return (
    <div className="fixed inset-0 bg-slate-900/60 backdrop-blur-xl z-[100] flex items-center justify-center p-6 animate-in fade-in duration-300">
      <div className="bg-white w-full max-w-sm rounded-[48px] p-12 shadow-[0_50px_100px_-20px_rgba(0,0,0,0.3)] animate-in zoom-in-95 duration-300">
        <h2 className="text-2xl font-bold mb-8 tracking-tight uppercase text-slate-900">{title}</h2>
        <div className="space-y-6">
          <div className="space-y-2">
            <label className="text-[10px] font-bold uppercase tracking-widest text-slate-400 ml-1">Patient Name</label>
            <input className="w-full p-5 bg-slate-50 border-2 border-slate-100 rounded-3xl outline-none focus:border-blue-600 focus:bg-white transition-all font-bold" value={name} onChange={e => setName(e.target.value)} placeholder="Full Name..." autoFocus />
          </div>
          <div className="grid grid-cols-2 gap-4">
             <div className="space-y-2">
               <label className="text-[10px] font-bold uppercase tracking-widest text-slate-400 ml-1">Room</label>
               <input className="w-full p-5 bg-slate-50 border-2 border-slate-100 rounded-3xl outline-none focus:border-blue-600 focus:bg-white transition-all font-bold" value={room} onChange={e => setRoom(e.target.value)} placeholder="e.g. 101" />
             </div>
             <div className="space-y-2">
               <label className="text-[10px] font-bold uppercase tracking-widest text-slate-400 ml-1">Bed</label>
               <input className="w-full p-5 bg-slate-50 border-2 border-slate-100 rounded-3xl outline-none focus:border-blue-600 focus:bg-white transition-all font-bold" value={bed} onChange={e => setBed(e.target.value)} placeholder="e.g. 2" />
             </div>
          </div>
        </div>
        <div className="flex flex-col gap-3 mt-10">
          <button 
            onClick={() => onSubmit(name, bed, room)}
            className="w-full py-5 bg-blue-600 text-white font-bold uppercase text-[10px] tracking-[4px] rounded-3xl shadow-2xl shadow-blue-600/30 hover:bg-blue-700 transition-all"
          >
            Save Details
          </button>
          <button onClick={onClose} className="w-full py-5 bg-slate-50 text-slate-500 font-bold uppercase text-[10px] tracking-[4px] rounded-3xl hover:bg-slate-100 transition-all">Cancel</button>
        </div>
      </div>
    </div>
  );
}

// ═══════════════════════════════════════════════════════════════════
//  ENTRY WRAPPER (Unified Router)
// ═══════════════════════════════════════════════════════════════════
export default function AdminPage() {
  const [loggedIn, setLoggedIn] = useState(false);
  const [localSetupDone, setLocalSetupDone] = useState<boolean | null>(null);
  
  const { data: status, isLoading } = useQuery<any>({
    queryKey: ["/api/status"],
    queryFn: async () => { 
      try {
        const res = await fetch(apiUrl("/api/status"), { credentials: "include" }); 
        return res.json(); 
      } catch (e) { return null; }
    },
    refetchInterval: 5000
  });

  if (isLoading) return (
    <div className="min-h-screen bg-slate-50 flex items-center justify-center">
      <Activity className="w-12 h-12 text-blue-600 animate-pulse" />
    </div>
  );

  const setupDone = localSetupDone ?? status?.setup;

  // 1. First Boot Logic (Only on local gateway, cloud bypasses setup)
  const isCloud = status?.controllerIP === "cloud";
  if (setupDone === false && !isCloud) {
    return <SetupWizard onComplete={() => setLocalSetupDone(true)} />;
  }

  // 2. Auth Logic
  if (!(loggedIn || status?.authenticated)) {
    return <LoginForm onLogin={() => setLoggedIn(true)} />;
  }

  // 3. Admin Core
  return <AdminPanel />;
}
