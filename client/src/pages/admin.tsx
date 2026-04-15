import { useState, useEffect, useRef } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import type { Slave } from "@shared/schema";
import { apiRequest, apiUrl, getQueryFn } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import { useWebsocket } from "@/hooks/use-websocket";
import { Shield, LogIn, LogOut, Wifi, Trash2, Pencil } from "lucide-react";
import { LocalNotifications } from "@capacitor/local-notifications";
import { Haptics } from "@capacitor/haptics";

// ═══════════════════════════════════════════════════════════════
//  SETUP WIZARD — Non-Auth First Boot
// ═══════════════════════════════════════════════════════════════
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
    queryFn: async () => { const res = await fetch(apiUrl("/api/scan")); return res.json(); },
    refetchInterval: 5000,
    enabled: mode === 2 || mode === 4
  });

  const handleFinish = async () => {
    setLoading(true);
    try {
      if (mode === 2 || mode === 4) {
        await apiRequest("POST", "/api/connect-wifi", { ssid: staSsid, password: staPass });
      }
      await apiRequest("POST", "/api/setup", { mode, apSSID: apSsid, apPass: apPass });
      toast({ title: "Configuration Applied", description: "System is restarting in selected mode." });
      onComplete();
    } catch {
      toast({ title: "Setup Failed", description: "Could not communicate with Master.", variant: "destructive" });
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-slate-50 flex flex-col items-center justify-center p-6 sm:p-12">
      <div className="w-full max-w-md bg-white border border-slate-200 shadow-2xl rounded-[32px] overflow-hidden">
        <div className="bg-blue-600 p-8 text-white text-center">
          <Shield className="w-10 h-10 mb-2 mx-auto" />
          <h1 className="text-2xl font-black mb-1">System Initialize</h1>
          <p className="text-blue-100 text-[10px] font-bold uppercase tracking-wider">Step {step} of 3 • Configuration Wizard</p>
        </div>
        
        <div className="p-8">
          {step === 1 && (
            <div className="space-y-4">
              <h2 className="text-slate-900 font-extrabold text-lg mb-4">Select Operating Mode</h2>
              {[
                { m: 1, t: "Standalone AP", d: "Local emergency network only" },
                { m: 2, t: "Facility Station", d: "Connect to hospital WiFi infrastructure" },
                { m: 4, t: "Railway Cloud", d: "Enable remote monitoring & cloud sync" }
              ].map(opt => (
                <button 
                  key={opt.m}
                  onClick={() => setMode(opt.m)}
                  className={`w-full p-4 rounded-2xl border text-left transition-all ${mode === opt.m ? "border-blue-500 bg-blue-50 ring-2 ring-blue-500/20" : "border-slate-100 hover:border-slate-200"}`}
                >
                  <div className="font-bold text-slate-800 text-sm">{opt.t}</div>
                  <div className="text-slate-500 text-[11px] mt-0.5">{opt.d}</div>
                </button>
              ))}
              <button onClick={() => setStep(2)} className="w-full mt-6 py-4 bg-slate-900 text-white font-bold rounded-2xl shadow-lg">Continue</button>
            </div>
          )}

          {step === 2 && (
            <div className="space-y-4">
              <h2 className="text-slate-900 font-extrabold text-lg mb-4">Device Credentials</h2>
              <div className="space-y-3">
                <div>
                  <label className="text-[10px] font-black uppercase text-slate-400 tracking-wider">Master SSID (e.g. HospitalAlarm)</label>
                  <input className="w-full mt-1 p-3 bg-slate-50 border border-slate-200 rounded-xl" placeholder="HospitalAlarm" value={apSsid} onChange={e => setApSsid(e.target.value)} />
                </div>
                <div>
                  <label className="text-[10px] font-black uppercase text-slate-400 tracking-wider">AP Password (Optional)</label>
                  <input className="w-full mt-1 p-3 bg-slate-50 border border-slate-200 rounded-xl" type="password" placeholder="Leave blank for open" value={apPass} onChange={e => setApPass(e.target.value)} />
                </div>
              </div>
              <div className="flex gap-3 mt-6">
                <button onClick={() => setStep(1)} className="flex-1 py-4 bg-slate-100 text-slate-600 font-bold rounded-2xl">Back</button>
                <button onClick={() => setStep(3)} className="flex-1 py-4 bg-slate-900 text-white font-bold rounded-2xl shadow-lg">Next</button>
              </div>
            </div>
          )}

          {step === 3 && (
            <div className="space-y-4">
              <h2 className="text-slate-900 font-extrabold text-lg mb-4">Network Integration</h2>
              {(mode === 2 || mode === 4) ? (
                <div className="space-y-3">
                  <div className="max-h-40 overflow-y-auto border border-slate-100 rounded-xl p-2 space-y-1">
                    {networks?.map(n => (
                      <button key={n.ssid} onClick={() => setStaSsid(n.ssid)} className={`w-full text-left p-2 rounded-lg text-xs font-bold ${staSsid === n.ssid ? "bg-blue-600 text-white" : "hover:bg-slate-50"}`}>
                        {n.ssid} ({n.rssi}dBm)
                      </button>
                    ))}
                  </div>
                  <input className="w-full p-3 bg-slate-50 border border-slate-200 rounded-xl text-sm" placeholder="Network Password" type="password" value={staPass} onChange={e => setStaPass(e.target.value)} />
                </div>
              ) : (
                <div className="text-center py-6">
                  <Wifi className="mx-auto w-12 h-12 text-slate-200 mb-2" />
                  <p className="text-slate-400 text-xs font-medium text-center">Standalone Mode active.<br/>WiFi connection bypassed.</p>
                </div>
              )}
              <div className="flex gap-3 mt-6">
                <button onClick={() => setStep(2)} className="flex-1 py-4 bg-slate-100 text-slate-600 font-bold rounded-2xl">Back</button>
                <button onClick={handleFinish} disabled={loading} className="flex-1 py-4 bg-blue-600 text-white font-bold rounded-2xl shadow-lg shadow-blue-500/30">
                  {loading ? "Configuring..." : "Launch System"}
                </button>
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

// ═══════════════════════════════════════════════════════════════
//  LOGIN FORM — Light Mode
// ═══════════════════════════════════════════════════════════════
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
      if (!res.ok) {
        if (res.status === 401) throw new Error("Invalid credentials");
        throw new Error(`Server error: ${res.status}`);
      }
      onLogin();
      toast({ title: "Authorized", description: "Terminal access granted." });
    } catch (err: any) {
      console.error("Login Error:", err);
      toast({ 
        title: "Login Failed", 
        description: err.message || "Could not connect to security server.", 
        variant: "destructive" 
      });
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-slate-50 flex items-center justify-center p-5">
      <div className="absolute inset-0 pointer-events-none bg-[radial-gradient(circle_at_50%_50%,rgba(59,130,246,0.08)_0%,transparent_50%)]" />
      <div className="w-full max-w-[420px] relative">
        <div className="absolute -inset-[1px] bg-gradient-to-br from-blue-500/10 to-transparent rounded-3xl" />
        <div className="bg-white/80 backdrop-blur-xl border border-slate-200/60 shadow-xl rounded-3xl p-10 relative">
          <div className="text-center mb-8">
            <div className="w-16 h-16 bg-blue-50 border border-blue-100 rounded-[18px] inline-flex items-center justify-center mb-3 shadow-sm">
              <Shield className="w-8 h-8 text-blue-600" />
            </div>
            <h1 className="text-2xl font-extrabold text-slate-900 tracking-tight">Security Command</h1>
            <p className="text-[11px] text-blue-600/70 mt-1 font-bold uppercase tracking-[2px]">Administrator Access</p>
          </div>
          <form onSubmit={handleLogin} className="space-y-3">
            <div>
              <label className="block text-[10px] text-slate-500 mb-1 font-bold uppercase tracking-wider">Username</label>
              <input
                className="w-full px-3.5 py-2.5 bg-slate-50 border border-slate-200 rounded-xl text-slate-900 text-sm outline-none focus:border-blue-500 focus:ring-2 focus:ring-blue-500/20 transition-all placeholder:text-slate-400"
                value={user}
                onChange={(e) => setUser(e.target.value)}
                placeholder="admin"
              />
            </div>
            <div>
              <label className="block text-[10px] text-slate-500 mb-1 font-bold uppercase tracking-wider">Password</label>
              <input
                type="password"
                className="w-full px-3.5 py-2.5 bg-slate-50 border border-slate-200 rounded-xl text-slate-900 text-sm outline-none focus:border-blue-500 focus:ring-2 focus:ring-blue-500/20 transition-all placeholder:text-slate-400"
                value={pass}
                onChange={(e) => setPass(e.target.value)}
                placeholder="••••••••"
              />
            </div>
            <button
              type="submit"
              disabled={loading}
              className="w-full py-3 bg-blue-600 hover:bg-blue-700 hover:shadow-lg hover:shadow-blue-600/25 text-white rounded-xl text-sm font-bold transition-all mt-2 disabled:opacity-50 flex items-center justify-center gap-2"
            >
              {loading ? "Authenticating..." : "Establish Secure Link"}
              {!loading && <LogIn className="w-4 h-4" />}
            </button>
          </form>
        </div>
      </div>
    </div>
  );
}

// ═══════════════════════════════════════════════════════════════
//  APPROVE / EDIT MODAL — Light Mode
// ═══════════════════════════════════════════════════════════════
function SlaveModal({
  title, open, onClose, onSubmit, loading, defaults,
}: {
  title: string; open: boolean; onClose: () => void;
  onSubmit: (name: string, bed: string, room: string) => void;
  loading: boolean; defaults?: { patientName?: string; bed?: string; room?: string };
}) {
  const [name, setName] = useState(defaults?.patientName || "");
  const [bed, setBed] = useState(defaults?.bed || "");
  const [room, setRoom] = useState(defaults?.room || "");

  if (!open) return null;

  const handleSubmit = () => {
    if (!name || !bed || !room) return alert("All fields required");
    onSubmit(name, bed, room);
  };

  return (
    <div className="fixed inset-0 bg-slate-900/40 backdrop-blur-sm z-[999] flex items-center justify-center p-4" onClick={onClose}>
      <div className="bg-white border border-slate-200 shadow-2xl rounded-[24px] p-8 w-full max-w-[400px]" onClick={(e) => e.stopPropagation()}>
        <h2 className="text-xl font-extrabold text-slate-900 mb-6">{title}</h2>
        <label className="block text-[10px] text-slate-500 mb-1.5 font-bold uppercase tracking-wider">Subject Name</label>
        <input
          className="w-full px-4 py-3 bg-slate-50 border border-slate-200 rounded-xl text-slate-900 text-sm outline-none focus:border-blue-500 focus:ring-2 focus:ring-blue-500/20 transition-all mb-4"
          value={name} onChange={(e) => setName(e.target.value)} placeholder="e.g. John Doe" autoFocus
        />
        <div className="grid grid-cols-2 gap-4">
          <div>
            <label className="block text-[10px] text-slate-500 mb-1.5 font-bold uppercase tracking-wider">Bed</label>
            <input
              className="w-full px-4 py-3 bg-slate-50 border border-slate-200 rounded-xl text-slate-900 text-sm outline-none focus:border-blue-500 focus:ring-2 focus:ring-blue-500/20 transition-all"
              value={bed} onChange={(e) => setBed(e.target.value)} placeholder="e.g. B-12"
            />
          </div>
          <div>
            <label className="block text-[10px] text-slate-500 mb-1.5 font-bold uppercase tracking-wider">Room</label>
            <input
              className="w-full px-4 py-3 bg-slate-50 border border-slate-200 rounded-xl text-slate-900 text-sm outline-none focus:border-blue-500 focus:ring-2 focus:ring-blue-500/20 transition-all"
              value={room} onChange={(e) => setRoom(e.target.value)} placeholder="e.g. ICU-3"
            />
          </div>
        </div>
        <div className="flex gap-3 mt-8">
          <button onClick={handleSubmit} disabled={loading} className="flex-1 py-3 bg-blue-600 hover:bg-blue-700 hover:shadow-lg hover:shadow-blue-600/25 text-white rounded-xl text-sm font-bold transition-all disabled:opacity-50">
            {loading ? "Saving..." : "Confirm & Save"}
          </button>
          <button onClick={onClose} className="flex-1 py-3 bg-white text-slate-700 hover:bg-slate-50 rounded-xl text-sm font-bold border border-slate-200 transition-colors">
            Cancel
          </button>
        </div>
      </div>
    </div>
  );
}

// ═══════════════════════════════════════════════════════════════
//  ADMIN PANEL — Light Mode
// ═══════════════════════════════════════════════════════════════
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
    const requestPerms = async () => {
      try {
        const status = await LocalNotifications.checkPermissions();
        if (status.display !== 'granted') await LocalNotifications.requestPermissions();
      } catch { } // Web fallback
    };
    requestPerms();
    return () => { if (sirenRef.current) sirenRef.current.pause(); };
  }, []);

  useWebsocket((msg) => {
    queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
    queryClient.invalidateQueries({ queryKey: ["/api/status"] });
    
    if (msg.type === "ALERT") {
      if (sirenRef.current) sirenRef.current.play().catch(() => {});
      Haptics.vibrate({ duration: 1000 }).catch(() => {});
      toast({ title: "EMERGENCY ALARM", description: `Alert triggered from ${msg.payload.slaveId}`, variant: "destructive" });
    }
  });

  const { data: slaves } = useQuery<Slave[]>({
    queryKey: ["/api/slaves", "all"],
    queryFn: async () => { const res = await fetch(apiUrl("/api/slaves?all=1"), { credentials: "include" }); return res.json(); },
    refetchInterval: 10000,
  });

  const { data: status } = useQuery<{ mode: number; isMasterOnline?: boolean }>({
    queryKey: ["/api/status"],
    queryFn: async () => { const res = await fetch(apiUrl("/api/status"), { credentials: "include" }); return res.json(); },
    refetchInterval: 8000,
  });

  const resetMutation = useMutation({
    mutationFn: async () => { const res = await apiRequest("POST", "/api/reset"); return res.json(); },
    onSuccess: () => { queryClient.invalidateQueries(); toast({ title: "System Reset", description: "Data purged successfully." }); }
  });

  const logoutMutation = useMutation({
    mutationFn: async () => { await apiRequest("POST", "/api/admin/logout"); },
    onSuccess: () => window.location.reload(),
  });

  const safeSlaves = Array.isArray(slaves) ? slaves : [];
  const pending = safeSlaves.filter((s) => !s.approved);
  const approved = safeSlaves.filter((s) => s.approved);
  const isMasterOnline = status?.isMasterOnline ?? false;

  return (
    <div className="min-h-screen bg-slate-50 text-slate-900 pb-24 font-sans selection:bg-blue-500/20">
      <header className="bg-white/80 backdrop-blur-xl border-b border-slate-200/60 shadow-sm px-6 py-4 flex justify-between items-center sticky top-0 z-40">
        <div className="flex items-center gap-3">
          <div className="p-2 bg-blue-50 rounded-xl border border-blue-100 shadow-sm">
            <Shield className="w-5 h-5 text-blue-600" />
          </div>
          <h1 className="text-lg font-extrabold text-slate-900 hidden sm:block">Command Control</h1>
        </div>
        <div className="flex items-center gap-4">
          <div className={`px-2.5 py-1.5 rounded-lg text-[10px] font-bold border shadow-sm flex items-center gap-1.5 ${isMasterOnline ? "border-emerald-200 bg-emerald-50 text-emerald-700" : "border-red-200 bg-red-50 text-red-700"}`}>
            <div className={`w-1.5 h-1.5 rounded-full ${isMasterOnline ? "bg-emerald-500" : "bg-red-500"}`} />
            {isMasterOnline ? "SYSTEM ONLINE" : "SYSTEM OFFLINE"}
          </div>
          <button onClick={() => logoutMutation.mutate()} className="p-2 text-slate-500 hover:text-slate-900 hover:bg-slate-100 rounded-lg transition-colors"><LogOut size={18}/></button>
        </div>
      </header>

      {activeTab === "dashboard" ? (
        <main className="p-4 sm:p-6 max-w-6xl mx-auto space-y-6">
          <div className="grid grid-cols-2 md:grid-cols-4 gap-3 sm:gap-4">
             <div className="bg-white border border-slate-200/60 p-5 rounded-2xl shadow-sm">
               <div className="text-3xl font-extrabold text-slate-800">{approved.length}</div>
               <div className="text-[10px] text-slate-500 uppercase font-bold mt-1 tracking-wider">Active Nodes</div>
             </div>
             <div className="bg-white border border-slate-200/60 p-5 rounded-2xl shadow-sm">
               <div className="text-3xl font-extrabold text-blue-600">{safeSlaves.filter(s => s.online && s.approved).length}</div>
               <div className="text-[10px] text-blue-500 uppercase font-bold mt-1 tracking-wider">Nodes Online</div>
             </div>
             <div className="bg-white border border-slate-200/60 p-5 rounded-2xl shadow-sm">
               <div className="text-3xl font-extrabold text-red-600">{safeSlaves.filter(s => s.alertActive).length}</div>
               <div className="text-[10px] text-red-500 uppercase font-bold mt-1 tracking-wider">Active Alerts</div>
             </div>
             <div className="bg-white border border-slate-200/60 p-5 rounded-2xl shadow-sm">
               <div className="text-3xl font-extrabold text-amber-500">{pending.length}</div>
               <div className="text-[10px] text-amber-500 uppercase font-bold mt-1 tracking-wider">Pending Auth</div>
             </div>
          </div>

          {pending.length > 0 && (
            <div className="space-y-3">
              <h2 className="text-xs font-bold text-slate-500 uppercase tracking-widest pl-1">Discovery Queue</h2>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
                {pending.map(s => (
                  <div key={s.slaveId} className="bg-amber-50/50 border border-amber-200 p-4 rounded-2xl flex justify-between items-center shadow-sm">
                    <div className="font-mono text-sm font-semibold text-amber-800 bg-amber-100/50 px-2.5 py-1 rounded-md">{s.slaveId}</div>
                    <button onClick={() => { setModalSlave(s); setModalMode("approve"); }} className="bg-amber-500 hover:bg-amber-600 shadow-sm text-white px-4 py-2 rounded-xl text-xs font-bold transition-colors">Authorize Device</button>
                  </div>
                ))}
              </div>
            </div>
          )}

          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            {approved.map(d => (
              <div key={d.slaveId} className={`p-5 rounded-2xl border transition-all duration-300 ${d.alertActive ? "bg-red-50 border-red-300 shadow-lg shadow-red-500/10 scale-[1.02]" : "bg-white border-slate-200/60 shadow-sm hover:shadow-md"}`}>
                <div className="flex justify-between items-start mb-4">
                  <div>
                    <div className="font-extrabold text-lg text-slate-900">{d.patientName}</div>
                    <div className="text-xs font-medium text-slate-500 mt-1">Room {d.room} • Bed {d.bed}</div>
                  </div>
                  <div className={`text-[10px] px-2.5 py-1 rounded-full font-bold shadow-sm ${d.alertActive ? "bg-red-500 text-white animate-pulse" : "bg-slate-100 text-slate-500 border border-slate-200"}`}>
                    {d.alertActive ? "EMERGENCY" : "STABLE"}
                  </div>
                </div>
                
                <div className="flex gap-2">
                  <button onClick={() => { setModalSlave(d); setModalMode("edit"); }} className="bg-slate-50 hover:bg-slate-100 border border-slate-200 text-slate-600 p-2.5 rounded-xl transition-colors shadow-sm"><Pencil size={16}/></button>
                  {d.alertActive ? (
                    <button onClick={() => apiRequest("POST", `/api/clearAlert/${d.slaveId}`)} className="bg-red-600 hover:bg-red-700 shadow-md shadow-red-600/20 text-white flex-1 rounded-xl text-sm font-bold transition-all">
                      Silence Alarm
                    </button>
                  ) : (
                    <div className="flex-1" /> // Spacer
                  )}
                  <button onClick={() => { if(confirm(`Revoke access for ${d.patientName}?`)) apiRequest("DELETE", `/api/slaves/${d.slaveId}`); }} className="bg-slate-50 hover:bg-red-50 border border-slate-200 hover:border-red-200 text-slate-400 hover:text-red-500 p-2.5 rounded-xl transition-colors shadow-sm"><Trash2 size={16}/></button>
                </div>
              </div>
            ))}
          </div>
          {approved.length === 0 && pending.length === 0 && (
            <div className="text-center py-20">
              <Wifi className="w-12 h-12 text-slate-300 mx-auto mb-4" />
              <p className="text-slate-500 font-medium tracking-tight">No active nodes connected to the network.</p>
            </div>
          )}
        </main>
      ) : (
        <main className="p-4 sm:p-6 max-w-2xl mx-auto space-y-6">
          <div className="bg-white border border-slate-200/60 rounded-3xl p-6 md:p-8 shadow-sm">
            <h3 className="text-sm font-extrabold text-slate-900 mb-5 uppercase tracking-wide">Infrastructure Mode</h3>
            <div className="grid grid-cols-1 gap-3">
              {[
                { m: 1, t: "AP Mode", d: "Isolated network broadcast (Master acts as Router)" },
                { m: 2, t: "Station Mode", d: "Connecting to existing local facility infrastructure" },
                { m: 4, t: "Cloud Connect", d: "Bridging local nodes to secure global Railway servers" }
              ].map(m => (
                <button 
                  key={m.m}
                  onClick={() => apiRequest("POST", "/api/setup", { mode: m.m })}
                  className={`p-4 rounded-2xl border text-left transition-all ${status?.mode === m.m ? "bg-blue-50 border-blue-300 ring-4 ring-blue-500/10 shadow-sm" : "bg-slate-50 border-slate-200 hover:bg-slate-100"}`}
                >
                  <div className={`font-bold text-sm ${status?.mode === m.m ? "text-blue-900" : "text-slate-700"}`}>{m.t}</div>
                  <div className={`text-[11px] mt-1 font-medium ${status?.mode === m.m ? "text-blue-700/70" : "text-slate-500"}`}>{m.d}</div>
                </button>
              ))}
            </div>
          </div>

          <div className="bg-red-50/50 border border-red-200 rounded-3xl p-6 md:p-8 shadow-sm">
            <h3 className="text-sm font-extrabold text-red-600 mb-2 uppercase tracking-wide">Danger Zone</h3>
            <p className="text-xs text-red-800/60 mb-5 font-medium leading-relaxed">System resets permanently wipe all approved nodes, telemetry history, and patient assignments. Ensure physical local access to the Master before executing.</p>
            <button 
              onClick={() => { if(confirm("WIPE ALL DATA? This is permanent.")) resetMutation.mutate(); }}
              className="bg-white border border-red-200 hover:bg-red-50 text-red-600 shadow-sm px-6 py-3 rounded-xl text-sm font-bold transition-all w-full md:w-auto"
            >
              Factory Reset Infrastructure
            </button>
          </div>
        </main>
      )}

      {/* ── Dock Navigation ── */}
      <div className="fixed bottom-6 left-1/2 -translate-x-1/2">
        <nav className="bg-white/90 backdrop-blur-xl border border-slate-200/60 shadow-2xl rounded-2xl p-2 flex gap-2">
          <button 
            onClick={() => setActiveTab("dashboard")}
            className={`w-24 flex flex-col items-center gap-1.5 p-2 rounded-xl transition-colors ${activeTab === "dashboard" ? "bg-blue-50 text-blue-600" : "text-slate-500 hover:bg-slate-50 hover:text-slate-700"}`}
          >
            <Wifi size={20} className={activeTab === "dashboard" ? "opacity-100" : "opacity-70"} />
            <span className="text-[10px] font-bold uppercase tracking-wider">Nodes</span>
          </button>
          <button 
            onClick={() => setActiveTab("settings")}
            className={`w-24 flex flex-col items-center gap-1.5 p-2 rounded-xl transition-colors ${activeTab === "settings" ? "bg-blue-50 text-blue-600" : "text-slate-500 hover:bg-slate-50 hover:text-slate-700"}`}
          >
            <Shield size={20} className={activeTab === "settings" ? "opacity-100" : "opacity-70"} />
            <span className="text-[10px] font-bold uppercase tracking-wider">Settings</span>
          </button>
        </nav>
      </div>

      <SlaveModal
        title={modalMode === "approve" ? "Authorize Node Access" : "Modify Node Details"}
        open={!!modalSlave}
        onClose={() => setModalSlave(null)}
        onSubmit={(name, bed, room) => {
          if (!modalSlave) return;
          if (modalMode === "approve") {
             apiRequest("POST", `/api/approve/${modalSlave.slaveId}`, { patientName: name, bed, room }).then(() => { queryClient.invalidateQueries(); setModalSlave(null); });
          } else {
             apiRequest("PUT", `/api/slaves/${modalSlave.slaveId}`, { patientName: name, bed, room }).then(() => { queryClient.invalidateQueries(); setModalSlave(null); });
          }
        }}
        loading={false}
        defaults={modalMode === "edit" ? modalSlave ?? undefined : undefined}
      />
    </div>
  );
}

export default function AdminPage() {
  const [loggedIn, setLoggedIn] = useState(false);
  const [forceSetup, setForceSetup] = useState(false);
  
  const { data: status, isLoading } = useQuery<{ mode: number; setup: boolean; authenticated: boolean } | null>({
    queryKey: ["/api/status"],
    queryFn: async () => { 
      const res = await fetch(apiUrl("/api/status"), { credentials: "include" }); 
      return res.json(); 
    },
    staleTime: 5000,
  });

  if (isLoading) {
    return (
      <div className="min-h-screen bg-slate-50 flex items-center justify-center text-blue-500 font-mono text-xs font-bold uppercase tracking-[3px] animate-pulse">
        Stabilizing Handshake...
      </div>
    );
  }

  // If the device is not yet configured, show the SetupWizard directly (No Login Required)
  if (status && !status.setup && !forceSetup) {
    return <SetupWizard onComplete={() => setForceSetup(true)} />;
  }

  if (!(loggedIn || !!status?.authenticated)) return <LoginForm onLogin={() => setLoggedIn(true)} />;
  return <AdminPanel />;
}
