import { useState, useEffect, useRef } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import type { Slave } from "@shared/schema";
import { apiRequest, apiUrl, getQueryFn } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import { useWebsocket } from "@/hooks/use-websocket";
import { Shield, LogIn, LogOut, Wifi, Trash2, Pencil, Activity, Database, Settings, AlertTriangle, CheckCircle2, Navigation } from "lucide-react";
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
  const [apSsid, setApSsid] = useState("");
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
      toast({ title: "Configuration Deployed", description: "Node is re-provisioning. Re-link to the new Master SSID." });
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
              <p className="text-[10px] font-black uppercase tracking-[4px] opacity-70">Infrastructure</p>
              <h1 className="text-3xl font-black tracking-tight uppercase">Base Init</h1>
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
                <h2 className="text-2xl font-black">Strategic Mode</h2>
                <p className="text-slate-500 text-sm mt-1">Select the operational grid architecture for this node.</p>
              </header>
              <div className="grid gap-3">
                {[
                  { m: 1, t: "Standalone AP", d: "Isolated clinical grid. Highest security.", icon: <Shield className="w-4 h-4" /> },
                  { m: 2, t: "Master Station", d: "Links to existing hospital infrastructure.", icon: <Database className="w-4 h-4" /> },
                  { m: 3, t: "Hybrid Bridge", d: "Internal broadcast + Hospital WiFi link.", icon: <Wifi className="w-4 h-4" /> },
                  { m: 4, t: "Cloud Sentinel", d: "Hybrid + Secure Railway Sync (Recommended).", icon: <Activity className="w-4 h-4" /> }
                ].map(opt => (
                  <button 
                    key={opt.m}
                    onClick={() => setMode(opt.m)}
                    className={`group w-full p-5 rounded-[24px] border-2 text-left transition-all relative overflow-hidden ${mode === opt.m ? "border-blue-600 bg-blue-50/50 shadow-lg shadow-blue-500/5" : "border-slate-100 hover:border-slate-300 bg-slate-50/30"}`}
                  >
                    <div className="flex items-center justify-between mb-1">
                      <span className={`text-[9px] font-black uppercase tracking-widest ${mode === opt.m ? "text-blue-600" : "text-slate-400"}`}>Protocol {opt.m}</span>
                      <div className={`p-1.5 rounded-lg ${mode === opt.m ? "bg-blue-600 text-white" : "bg-slate-200 text-slate-500"}`}>{opt.icon}</div>
                    </div>
                    <div className="font-extrabold text-slate-900">{opt.t}</div>
                    <div className="text-slate-500 text-[11px] font-medium leading-relaxed mt-1 opacity-80">{opt.d}</div>
                  </button>
                ))}
              </div>
              <button onClick={() => setStep(2)} className="w-full py-6 bg-slate-900 text-white font-black uppercase text-xs tracking-[3px] rounded-[24px] shadow-2xl hover:translate-y-[-2px] active:translate-y-1 transition-all">Continue to Radio Config</button>
            </div>
          )}

          {step === 2 && (
            <div className="space-y-8 animate-in fade-in slide-in-from-right-8 duration-500">
              <header>
                <h2 className="text-2xl font-black">Radio Identity</h2>
                <p className="text-slate-500 text-sm mt-1">Establish the local broadcast identity for patient nodes.</p>
              </header>
              <div className="space-y-5">
                <div className="space-y-2">
                  <label className="text-[10px] font-black uppercase tracking-widest text-slate-400 ml-1">Access SSID</label>
                  <input 
                    className="w-full p-6 bg-slate-50 border-2 border-slate-100 rounded-[24px] focus:border-blue-600 focus:bg-white outline-none transition-all font-bold text-lg placeholder:text-slate-300" 
                    placeholder="e.g. Master-Radio-Alpha" 
                    value={apSsid} onChange={e => setApSsid(e.target.value)} 
                  />
                </div>
                <div className="space-y-2">
                  <label className="text-[10px] font-black uppercase tracking-widest text-slate-400 ml-1">Access Password</label>
                  <input 
                    className="w-full p-6 bg-slate-50 border-2 border-slate-100 rounded-[24px] focus:border-blue-600 focus:bg-white outline-none transition-all font-bold text-lg placeholder:text-slate-300"
                    type="password" placeholder="Min 8 Characters" value={apPass} onChange={e => setApPass(e.target.value)} 
                  />
                </div>
              </div>
              <div className="flex gap-4">
                <button onClick={() => setStep(1)} className="flex-1 py-6 bg-slate-100 text-slate-500 font-black uppercase text-xs rounded-[24px]">Back</button>
                <button onClick={() => setStep(3)} className="flex-[2] py-6 bg-slate-900 text-white font-black uppercase text-xs tracking-[3px] rounded-[24px] shadow-xl">Confirm Radio</button>
              </div>
            </div>
          )}

          {step === 3 && (
            <div className="space-y-8 animate-in fade-in slide-in-from-right-8 duration-500">
              <header>
                <h2 className="text-2xl font-black">Bridge Interface</h2>
                <p className="text-slate-500 text-sm mt-1">Link this hub to the facility's primary network.</p>
              </header>
              {mode !== 1 ? (
                <div className="space-y-5">
                  <div className="max-h-[250px] overflow-y-auto space-y-2 pr-2 scrollbar-hide">
                    {!networks?.length && <div className="py-12 text-center text-slate-400 font-black animate-pulse uppercase tracking-[2px] text-xs">Scanning Frequencies...</div>}
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
                        <span className={`text-[10px] font-black ${staSsid === n.ssid ? "opacity-60" : "text-slate-400"}`}>{n.rssi} dBm</span>
                      </button>
                    ))}
                  </div>
                  <input 
                    className="w-full p-6 bg-slate-50 border-2 border-slate-100 rounded-[24px] focus:border-blue-600 focus:bg-white outline-none transition-all font-bold placeholder:text-slate-300" 
                    placeholder="Facility Credentials..." type="password" value={staPass} onChange={e => setStaPass(e.target.value)} 
                  />
                </div>
              ) : (
                <div className="text-center py-16 bg-blue-50/50 rounded-[40px] border-4 border-dashed border-blue-100/50 flex flex-col items-center">
                  <div className="w-20 h-20 bg-white rounded-3xl shadow-lg flex items-center justify-center mb-6">
                    <Shield className="w-10 h-10 text-blue-600" />
                  </div>
                  <p className="text-blue-900 font-black uppercase text-sm tracking-[2px]">Air-Gapped Protocol</p>
                  <p className="text-blue-600/60 text-xs px-12 mt-2 font-medium italic">No bridge required for standalone medical grids.</p>
                </div>
              )}
              <div className="flex gap-4">
                <button onClick={() => setStep(2)} className="flex-1 py-6 bg-slate-100 text-slate-500 font-black uppercase text-xs rounded-[24px]">Back</button>
                <button 
                  onClick={handleFinish} disabled={loading} 
                  className="flex-[2] py-6 bg-blue-600 text-white font-black uppercase text-xs tracking-[4px] rounded-[24px] shadow-2xl shadow-blue-600/30 disabled:opacity-50"
                >
                  {loading ? "Aligning..." : "Finalize Deployment"}
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
      if (!res.ok) throw new Error(res.status === 401 ? "Unauthorized Credentials" : "Command Failed");
      onLogin();
      toast({ title: "Command Confirmed", description: "Access granted to central hub." });
    } catch (err: any) {
      toast({ title: "Authorization Error", description: err.message, variant: "destructive" });
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
          <h1 className="text-4xl font-black tracking-tighter">Security Lock</h1>
          <p className="text-blue-600/50 text-[10px] font-black uppercase tracking-[5px] mt-3">Personnel Access Only</p>
        </div>

        <form onSubmit={handleLogin} className="space-y-4">
          <div className="space-y-2">
            <label className="text-[10px] font-black uppercase tracking-widest text-slate-400 ml-1">Identity</label>
            <input 
              className="w-full p-5 bg-slate-50 border-2 border-slate-100 rounded-3xl outline-none focus:border-blue-600 focus:bg-white transition-all font-bold" 
              value={user} onChange={e => setUser(e.target.value)} placeholder="admin"
            />
          </div>
          <div className="space-y-2">
            <label className="text-[10px] font-black uppercase tracking-widest text-slate-400 ml-1">Access Key</label>
            <input 
              type="password"
              className="w-full p-5 bg-slate-50 border-2 border-slate-100 rounded-3xl outline-none focus:border-blue-600 focus:bg-white transition-all font-bold" 
              value={pass} onChange={e => setPass(e.target.value)}
            />
          </div>
          <button 
            type="submit" disabled={loading}
            className="w-full py-5 bg-slate-900 text-white font-black uppercase text-xs tracking-[4px] rounded-3xl shadow-2xl hover:translate-y-[-2px] transition-all disabled:opacity-50 mt-4"
          >
            {loading ? "Verifying..." : "Establish Link"}
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
  const [modalSlave, setModalSlave] = useState<Slave | null>(null);
  const [modalMode, setModalMode] = useState<"approve" | "edit">("approve");
  const [activeTab, setActiveTab] = useState<"dashboard" | "settings">("dashboard");
  const sirenRef = useRef<HTMLAudioElement | null>(null);

  useEffect(() => {
    sirenRef.current = new Audio("https://raw.githubusercontent.com/Anis-Aouda/Zinedine-Audio/main/siren.mp3");
    sirenRef.current.loop = true;
    return () => { if (sirenRef.current) sirenRef.current.pause(); };
  }, []);

  useWebsocket((msg) => {
    queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
    queryClient.invalidateQueries({ queryKey: ["/api/status"] });
    if (msg.type === "ALERT" || msg.type === "UPDATE") {
       if (msg.device?.alertActive) {
          if (sirenRef.current) sirenRef.current.play().catch(() => {});
          Haptics.vibrate({ duration: 1000 }).catch(() => {});
          toast({ title: "EMERGENCY ALARM", description: `Alert: ${msg.device.patientName || msg.device.slaveId}`, variant: "destructive" });
       }
    }
  });

  const { data: slaves } = useQuery<Slave[]>({
    queryKey: ["/api/slaves", "all"],
    queryFn: async () => { const res = await fetch(apiUrl("/api/slaves?all=1"), { credentials: "include" }); return res.json(); },
    refetchInterval: 5000,
  });

  const { data: status } = useQuery<{ 
    mode: number; 
    uptime: number; 
    rssi: number; 
    slaves: number; 
    online: number; 
    alerts: number;
    isMasterOnline?: boolean;
    masterIP?: string;
  }>({
    queryKey: ["/api/status"],
    queryFn: async () => { const res = await fetch(apiUrl("/api/status"), { credentials: "include" }); return res.json(); },
    refetchInterval: 2000,
  });

  const isCloud = status?.masterIP === "cloud";
  // On cloud, we are active if master is online. On local, strictly mode 4 (Online) logic.
  const isCloudActive = isCloud ? !!status?.isMasterOnline : status?.mode === 4;

  const logoutMutation = useMutation({
    mutationFn: async () => { await apiRequest("POST", "/api/admin/logout"); },
    onSuccess: () => window.location.reload(),
  });

  const safeSlaves = Array.isArray(slaves) ? slaves : [];
  const pending = safeSlaves.filter(s => !s.approved);
  const approved = safeSlaves.filter(s => s.approved);
  const isCloudActive = status?.mode === 4;

  return (
    <div className="min-h-screen bg-[#F8FAFC] text-slate-900 pb-32 font-sans">
      {/* Premium Header */}
      <header className="bg-white px-8 py-6 border-b border-slate-100 flex justify-between items-center sticky top-0 z-40">
        <div className="flex items-center gap-4">
          <div className="w-10 h-10 bg-blue-600 rounded-2xl flex items-center justify-center text-white shadow-lg shadow-blue-600/20">
            <Shield className="w-6 h-6" />
          </div>
          <div>
            <h1 className="text-xl font-black tracking-tight text-slate-900">Hospital Command</h1>
            <p className="text-[9px] font-black uppercase tracking-[3px] text-blue-600 opacity-60">Session Active</p>
          </div>
        </div>
        
        <div className="flex items-center gap-6">
          <div className="hidden md:flex flex-col items-end">
            <div className="flex items-center gap-2">
              <div className={`w-2 h-2 rounded-full ${isCloudActive ? "bg-emerald-500" : (isCloud ? "bg-red-500" : "bg-amber-500")} animate-pulse`} />
              <span className="text-[10px] font-black uppercase tracking-widest">
                {isCloudActive ? "Cloud Link Active" : (isCloud ? "Master Offline" : "Local Logic Restricted")}
              </span>
            </div>
            <p className="text-[9px] text-slate-400 font-bold mt-1">UPTIME: {Math.floor((status?.uptime || 0) / 60)} MIN</p>
          </div>
          <button onClick={() => logoutMutation.mutate()} className="w-10 h-10 bg-slate-50 border border-slate-200 rounded-2xl flex items-center justify-center text-slate-400 hover:text-red-500 hover:bg-red-50 transition-all">
            <LogOut className="w-5 h-5" />
          </button>
        </div>
      </header>

      {activeTab === "dashboard" ? (
        <main className="p-8 max-w-7xl mx-auto space-y-8 animate-in fade-in duration-500">
          {/* Stats Grid */}
          <div className="grid grid-cols-2 lg:grid-cols-4 gap-6">
            {[
              { l: "Registry Count", v: status?.slaves || 0, i: <Database className="w-5 h-5" />, c: "text-slate-900" },
              { l: "Nodes Online", v: status?.online || 0, i: <Wifi className="w-5 h-5" />, c: "text-blue-600" },
              { l: "Emergency Status", v: status?.alerts || 0, i: <AlertTriangle className="w-5 h-5" />, c: "text-red-600" },
              { l: "Pending Link", v: pending.length, i: <Settings className="w-5 h-5" />, c: "text-amber-500" }
            ].map((s, idx) => (
              <div key={idx} className="bg-white p-6 rounded-[32px] shadow-[0_10px_20px_-10px_rgba(0,0,0,0.03)] border-b-4 border-slate-100">
                <div className="flex justify-between items-start mb-2">
                  <div className={`p-2 rounded-xl bg-slate-50 ${s.c}`}>{s.i}</div>
                  <span className={`text-2xl font-black ${s.c}`}>{s.v}</span>
                </div>
                <div className="text-[10px] uppercase font-black tracking-widest text-slate-400">{s.l}</div>
              </div>
            ))}
          </div>

          {/* Pending Queue */}
          {pending.length > 0 && (
            <div className="space-y-4">
              <h2 className="text-[10px] font-black uppercase tracking-[3px] text-slate-400 ml-2">Broadcast Discovery Queue</h2>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                {pending.map(s => (
                  <div key={s.slaveId} className="bg-amber-50 border-2 border-amber-100 p-6 rounded-[32px] flex justify-between items-center">
                    <div className="flex items-center gap-4">
                      <div className="w-12 h-12 bg-white rounded-2xl flex items-center justify-center text-amber-600 shadow-sm border border-amber-100">
                        <Wifi className="w-6 h-6 animate-pulse" />
                      </div>
                      <div>
                        <div className="font-mono font-black text-amber-900 text-lg uppercase">{s.slaveId}</div>
                        <p className="text-[10px] font-bold text-amber-700/60 uppercase tracking-widest mt-0.5">Device requesting pairing</p>
                      </div>
                    </div>
                    <button 
                      onClick={() => { setModalSlave(s); setModalMode("approve"); }}
                      className="bg-amber-600 text-white px-8 py-3 rounded-2xl text-[10px] font-black uppercase tracking-[2px] shadow-lg shadow-amber-600/30 hover:scale-[1.02] active:scale-[0.98] transition-all"
                    >
                      Authorize
                    </button>
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* Active Nodes */}
          <div className="space-y-4">
             <h2 className="text-[10px] font-black uppercase tracking-[3px] text-slate-400 ml-2">Active Clinical Nodes</h2>
             <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
                {approved.map(d => (
                  <div key={d.slaveId} className={`group bg-white p-6 rounded-[40px] border-2 transition-all duration-500 shadow-[0_20px_40px_-10px_rgba(0,0,0,0.02)] ${d.alertActive ? "border-red-500 bg-red-50/30 shadow-red-500/10 scale-[1.02]" : "border-slate-50 hover:border-blue-100 hover:shadow-blue-500/5 hover:-translate-y-1"}`}>
                    <div className="flex justify-between items-start mb-6">
                      <div className="flex items-center gap-4">
                        <div className={`w-14 h-14 rounded-3xl flex items-center justify-center transition-colors ${d.alertActive ? "bg-red-600 text-white animate-pulse" : "bg-blue-50 text-blue-600"}`}>
                          {d.alertActive ? <AlertTriangle className="w-8 h-8" /> : <Activity className="w-8 h-8" />}
                        </div>
                        <div>
                          <div className={`font-black text-xl tracking-tight ${d.alertActive ? "text-red-900" : "text-slate-900"}`}>{d.patientName}</div>
                          <div className="flex items-center gap-2 mt-1">
                            <span className="text-[10px] font-black uppercase tracking-widest text-slate-400">R: {d.room}</span>
                            <span className="w-1 h-1 rounded-full bg-slate-300" />
                            <span className="text-[10px] font-black uppercase tracking-widest text-slate-400">B: {d.bed}</span>
                          </div>
                        </div>
                      </div>
                      <div className={`px-3 py-1.5 rounded-xl text-[9px] font-black uppercase tracking-widest ${d.alertActive ? "bg-red-600 text-white" : "bg-emerald-50 text-emerald-600"}`}>
                        {d.alertActive ? "Emergency" : "Nominal"}
                      </div>
                    </div>

                    <div className="flex gap-3">
                      <button 
                        onClick={() => { setModalSlave(d); setModalMode("edit"); }}
                        className="w-12 h-12 bg-slate-50 text-slate-400 rounded-2xl flex items-center justify-center hover:bg-slate-100 hover:text-slate-900 transition-all border border-slate-100"
                      >
                        <Pencil size={18} />
                      </button>
                      {d.alertActive ? (
                        <button 
                          onClick={() => apiRequest("POST", "/api/clearAlert", { slaveId: d.slaveId })}
                          className="flex-1 bg-red-600 text-white font-black uppercase text-[10px] tracking-[4px] rounded-2xl shadow-xl shadow-red-600/30 hover:bg-red-700 active:translate-y-1 transition-all"
                        >
                          Silence Probe
                        </button>
                      ) : (
                        <div className="flex-1" />
                      )}
                      <button 
                        onClick={() => { if(confirm(`Revoke identity for ${d.patientName}?`)) apiRequest("POST", "/api/delete-slave", { slaveId: d.slaveId }); }}
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
                 <p className="text-slate-400 font-black uppercase tracking-[4px] text-xs">Awaiting node association</p>
               </div>
             )}
          </div>
        </main>
      ) : (
        <main className="p-8 max-w-2xl mx-auto space-y-8 animate-in fade-in duration-500">
          <div className="bg-white p-10 rounded-[48px] shadow-sm border border-slate-100">
            <h2 className="text-xl font-black mb-2">Strategy Re-Link</h2>
            <p className="text-slate-500 text-xs mb-8">Force a protocol shift for the entire clinical hub.</p>
            <div className="grid grid-cols-1 gap-4">
              {[
                { m: 1, t: "Transition to AP", d: "Isolated medical radio grid broadcast." },
                { m: 2, t: "Transition to STA", d: "Facilty infrastructure bridge link." },
                { m: 4, t: "Transition to CLOUD", d: "Secure global sync with Sentinel servers." }
              ].map(opt => (
                <button 
                  key={opt.m}
                  onClick={() => apiRequest("POST", "/api/setup", { mode: opt.m })}
                  className={`p-6 rounded-[32px] border-2 text-left transition-all relative ${status?.mode === opt.m ? "border-blue-600 bg-blue-50/50 shadow-lg shadow-blue-600/5 ring-4 ring-blue-500/5" : "border-slate-50 hover:border-slate-200 bg-slate-50/30"}`}
                >
                  <div className="flex justify-between items-center mb-1">
                    <span className="font-extrabold text-slate-900">{opt.t}</span>
                    {status?.mode === opt.m && <CheckCircle2 className="w-5 h-5 text-blue-600" />}
                  </div>
                  <p className="text-slate-500 text-[11px] font-medium leading-relaxed">{opt.d}</p>
                </button>
              ))}
            </div>
          </div>

          <div className="bg-red-50/50 p-10 rounded-[48px] border-2 border-red-100/50">
             <h2 className="text-lg font-black text-red-600 mb-2">Wipe Command</h2>
             <p className="text-red-800/60 text-[11px] font-medium leading-relaxed mb-8 italic">Executes a total cryptographic purge of all patient registries, WiFi credentials, and telemetry. Requires physical site visit for re-setup.</p>
             <button 
               onClick={() => { if(confirm("PERMANENT DATA WIPE?")) apiRequest("POST", "/api/system/reset"); }}
               className="bg-white text-red-600 border-2 border-red-200 px-10 py-5 rounded-[24px] text-xs font-black uppercase tracking-[3px] shadow-xl shadow-red-600/5 hover:bg-red-600 hover:text-white transition-all w-full"
             >
               Execute Total Wipe
             </button>
          </div>
        </main>
      )}

      {/* Premium Dock Navigation */}
      <nav className="fixed bottom-8 left-1/2 -translate-x-1/2 bg-slate-900/90 backdrop-blur-3xl px-8 py-5 rounded-[32px] shadow-[0_30px_60px_-15px_rgba(0,0,0,0.5)] flex items-center gap-12 z-50">
        <button 
          onClick={() => setActiveTab("dashboard")}
          className={`flex flex-col items-center gap-2 group transition-all ${activeTab === "dashboard" ? "text-blue-400" : "text-slate-500 hover:text-white"}`}
        >
          <div className={`w-2 h-2 rounded-full mb-1 transition-all ${activeTab === "dashboard" ? "bg-blue-400" : "bg-transparent"}`} />
          <Activity size={24} className="transition-all-duration-200 group-active:scale-95" />
          <span className="text-[10px] font-black uppercase tracking-widest">Grid</span>
        </button>
        <div className="w-[1px] h-10 bg-white/10" />
        <button 
          onClick={() => setActiveTab("settings")}
          className={`flex flex-col items-center gap-2 group transition-all ${activeTab === "settings" ? "text-blue-400" : "text-slate-500 hover:text-white"}`}
        >
          <div className={`w-2 h-2 rounded-full mb-1 transition-all ${activeTab === "settings" ? "bg-blue-400" : "bg-transparent"}`} />
          <Settings size={24} className="transition-all-duration-200 group-active:scale-95" />
          <span className="text-[10px] font-black uppercase tracking-widest">Config</span>
        </button>
      </nav>

      {/* Modal Overlay */}
      <SlaveModal
        title={modalMode === "approve" ? "Authorize Link" : "Protocol Modify"}
        open={!!modalSlave}
        onClose={() => setModalSlave(null)}
        onSubmit={(name: string, bed: string, room: string) => {
          if (!modalSlave) return;
          const url = modalMode === "approve" ? `/api/approve` : `/api/update`;
          apiRequest("POST", url, { slaveId: modalSlave.slaveId, patientName: name, bed, room })
            .then(() => { queryClient.invalidateQueries(); setModalSlave(null); });
        }}
        loading={false}
        defaults={modalMode === "edit" ? (modalSlave ?? undefined) : undefined}
      />
    </div>
  );
}

function SlaveModal({ title, open, onClose, onSubmit, defaults }: any) {
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
        <h2 className="text-2xl font-black mb-8 tracking-tight uppercase text-slate-900">{title}</h2>
        <div className="space-y-6">
          <div className="space-y-2">
            <label className="text-[10px] font-black uppercase tracking-widest text-slate-400 ml-1">Patient Name</label>
            <input className="w-full p-5 bg-slate-50 border-2 border-slate-100 rounded-3xl outline-none focus:border-blue-600 focus:bg-white transition-all font-extrabold" value={name} onChange={e => setName(e.target.value)} placeholder="Full Name..." autoFocus />
          </div>
          <div className="grid grid-cols-2 gap-4">
             <div className="space-y-2">
               <label className="text-[10px] font-black uppercase tracking-widest text-slate-400 ml-1">Room ID</label>
               <input className="w-full p-5 bg-slate-50 border-2 border-slate-100 rounded-3xl outline-none focus:border-blue-600 focus:bg-white transition-all font-extrabold" value={room} onChange={e => setRoom(e.target.value)} placeholder="ICU-1..." />
             </div>
             <div className="space-y-2">
               <label className="text-[10px] font-black uppercase tracking-widest text-slate-400 ml-1">Bed ID</label>
               <input className="w-full p-5 bg-slate-50 border-2 border-slate-100 rounded-3xl outline-none focus:border-blue-600 focus:bg-white transition-all font-extrabold" value={bed} onChange={e => setBed(e.target.value)} placeholder="B-2..." />
             </div>
          </div>
        </div>
        <div className="flex flex-col gap-3 mt-10">
          <button 
            onClick={() => onSubmit(name, bed, room)}
            className="w-full py-5 bg-blue-600 text-white font-black uppercase text-[10px] tracking-[4px] rounded-3xl shadow-2xl shadow-blue-600/30 hover:bg-blue-700 transition-all"
          >
            Commit Entry
          </button>
          <button onClick={onClose} className="w-full py-5 bg-slate-50 text-slate-500 font-black uppercase text-[10px] tracking-[4px] rounded-3xl hover:bg-slate-100 transition-all">Cancel</button>
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
  const isCloud = status?.masterIP === "cloud";
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
