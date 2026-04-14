import { useState, useEffect, useRef } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import type { Slave } from "@shared/schema";
import { apiRequest, apiUrl, getQueryFn } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import { useWebsocket } from "@/hooks/use-websocket";
import { Shield, RefreshCw, LogIn, LogOut, Wifi, Trash2, Pencil, BellOff, Volume2, VolumeX } from "lucide-react";
import { LocalNotifications } from "@capacitor/local-notifications";
import { Haptics } from "@capacitor/haptics";

// ═══════════════════════════════════════════════════════════════
//  LOGIN FORM — Shown before admin access
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
      console.log(`[LOGIN] Sending request to: ${apiUrl("/api/admin/login")}`);
      const res = await fetch(apiUrl("/api/admin/login"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        credentials: "include",
        body: JSON.stringify({ username: user, password: pass }),
      });
      console.log(`[LOGIN] Response status: ${res.status}`);
      if (!res.ok) throw new Error("Invalid credentials");
      onLogin();
      toast({ title: "Authorized", description: "Terminal access granted." });
    } catch {
      toast({ title: "Login Failed", description: "Invalid credentials", variant: "destructive" });
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-[#070b14] flex items-center justify-center p-5">
      <div className="absolute inset-0 pointer-events-none bg-[radial-gradient(circle_at_50%_50%,rgba(16,185,129,0.05)_0%,transparent_50%)]" />
      <div className="w-full max-w-[420px] relative">
        <div className="absolute -inset-[1px] bg-gradient-to-br from-emerald-500/15 to-transparent rounded-3xl" />
        <div className="bg-[rgba(15,23,42,0.8)] backdrop-blur-xl border border-white/10 rounded-3xl p-10 relative">
          <div className="text-center mb-8">
            <div className="w-16 h-16 bg-emerald-500/15 border border-emerald-500/20 rounded-[18px] inline-flex items-center justify-center mb-3">
              <Shield className="w-8 h-8 text-emerald-500" />
            </div>
            <h1 className="text-2xl font-extrabold text-white tracking-tight">Security Command</h1>
            <p className="text-[11px] text-emerald-500/60 mt-1 font-bold uppercase tracking-[2px]">Administrator Access</p>
          </div>
          <form onSubmit={handleLogin} className="space-y-3">
            <div>
              <label className="block text-[10px] text-slate-500 mb-1 font-bold uppercase tracking-wider">Username</label>
              <input
                className="w-full px-3.5 py-2.5 bg-white/[0.04] border border-white/10 rounded-xl text-white text-sm outline-none focus:border-emerald-500 transition-colors"
                value={user}
                onChange={(e) => setUser(e.target.value)}
                placeholder="admin"
              />
            </div>
            <div>
              <label className="block text-[10px] text-slate-500 mb-1 font-bold uppercase tracking-wider">Password</label>
              <input
                type="password"
                className="w-full px-3.5 py-2.5 bg-white/[0.04] border border-white/10 rounded-xl text-white text-sm outline-none focus:border-emerald-500 transition-colors"
                value={pass}
                onChange={(e) => setPass(e.target.value)}
                placeholder="••••••••"
              />
            </div>
            <button
              type="submit"
              disabled={loading}
              className="w-full py-3 bg-emerald-500 hover:shadow-lg hover:shadow-emerald-500/25 text-white rounded-xl text-sm font-bold transition-all mt-2 disabled:opacity-30 flex items-center justify-center gap-2"
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
//  APPROVE / EDIT MODAL — same layout as ESP32 dashboard.h
// ═══════════════════════════════════════════════════════════════
function SlaveModal({
  title,
  open,
  onClose,
  onSubmit,
  loading,
  defaults,
}: {
  title: string;
  open: boolean;
  onClose: () => void;
  onSubmit: (name: string, bed: string, room: string) => void;
  loading: boolean;
  defaults?: { patientName?: string; bed?: string; room?: string };
}) {
  const [name, setName] = useState(defaults?.patientName || "");
  const [bed, setBed] = useState(defaults?.bed || "");
  const [room, setRoom] = useState(defaults?.room || "");

  // Reset form when opening
  const prevOpen = useState(open)[0];
  if (open && !prevOpen) {
    // eslint-disable-next-line react-hooks/rules-of-hooks
  }

  if (!open) return null;

  const handleSubmit = () => {
    if (!name || !bed || !room) return alert("All fields required");
    onSubmit(name, bed, room);
  };

  return (
    <div
      className="fixed inset-0 bg-black/70 backdrop-blur-lg z-[999] flex items-center justify-center p-4"
      onClick={onClose}
    >
      <div
        className="bg-[rgba(15,23,42,0.95)] border border-white/10 rounded-[20px] p-8 w-full max-w-[400px]"
        onClick={(e) => e.stopPropagation()}
      >
        <h2 className="text-lg font-extrabold text-white mb-5">{title}</h2>
        <label className="block text-[9px] text-slate-500 mb-1 font-bold uppercase tracking-wider">Subject Name</label>
        <input
          className="w-full px-3.5 py-2.5 bg-white/[0.04] border border-white/10 rounded-xl text-white text-sm outline-none focus:border-emerald-500 mb-3"
          value={name}
          onChange={(e) => setName(e.target.value)}
          placeholder="e.g. John Doe"
          autoFocus
        />
        <div className="grid grid-cols-2 gap-3">
          <div>
            <label className="block text-[9px] text-slate-500 mb-1 font-bold uppercase tracking-wider">Bed</label>
            <input
              className="w-full px-3.5 py-2.5 bg-white/[0.04] border border-white/10 rounded-xl text-white text-sm outline-none focus:border-emerald-500"
              value={bed}
              onChange={(e) => setBed(e.target.value)}
              placeholder="e.g. B-12"
            />
          </div>
          <div>
            <label className="block text-[9px] text-slate-500 mb-1 font-bold uppercase tracking-wider">Room</label>
            <input
              className="w-full px-3.5 py-2.5 bg-white/[0.04] border border-white/10 rounded-xl text-white text-sm outline-none focus:border-emerald-500"
              value={room}
              onChange={(e) => setRoom(e.target.value)}
              placeholder="e.g. ICU-3"
            />
          </div>
        </div>
        <div className="flex gap-2 mt-5">
          <button
            onClick={handleSubmit}
            disabled={loading}
            className="flex-1 py-2.5 bg-emerald-500 hover:shadow-lg hover:shadow-emerald-500/25 text-white rounded-xl text-sm font-bold transition-all disabled:opacity-30"
          >
            {loading ? "Saving..." : "Activate Node"}
          </button>
          <button
            onClick={onClose}
            className="flex-1 py-2.5 bg-white/[0.04] text-slate-400 rounded-xl text-sm font-bold border border-white/10"
          >
            Cancel
          </button>
        </div>
      </div>
    </div>
  );
}

// ═══════════════════════════════════════════════════════════════
//  ADMIN PANEL — Unified design with Settings
// ═══════════════════════════════════════════════════════════════
function AdminPanel() {
  const { toast } = useToast();
  const queryClient = useQueryClient();
  const [modalSlave, setModalSlave] = useState<Slave | null>(null);
  const [modalMode, setModalMode] = useState<"approve" | "edit">("approve");
  const [activeTab, setActiveTab] = useState<"dashboard" | "settings">("dashboard");
  const sirenRef = useRef<HTMLAudioElement | null>(null);

  // Initialize Siren and Permissions
  useEffect(() => {
    sirenRef.current = new Audio("https://raw.githubusercontent.com/Anis-Aouda/Zinedine-Audio/main/siren.mp3");
    sirenRef.current.loop = true;

    const requestPerms = async () => {
      try {
        const status = await LocalNotifications.checkPermissions();
        if (status.display !== 'granted') await LocalNotifications.requestPermissions();
      } catch (err) { console.log("Capacitor not detected"); }
    };
    requestPerms();
    
    return () => {
      if (sirenRef.current) sirenRef.current.pause();
    };
  }, []);

  // ── WebSocket ──
  useWebsocket((msg) => {
    queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
    queryClient.invalidateQueries({ queryKey: ["/api/status"] });
    
    if (msg.type === "ALERT") {
      if (sirenRef.current) sirenRef.current.play().catch(() => {});
      Haptics.vibrate({ duration: 1000 }).catch(() => {});
      toast({ title: "EMERGENCY", description: `Alert from ${msg.payload.slaveId}`, variant: "destructive" });
    }
  });

  // ── Data ──
  const { data: slaves } = useQuery<Slave[]>({
    queryKey: ["/api/slaves", "all"],
    queryFn: async () => {
      const res = await fetch(apiUrl("/api/slaves?all=1"), { credentials: "include" });
      return res.json();
    },
    refetchInterval: 10000,
  });

  const { data: status } = useQuery<{ mode: number; isMasterOnline?: boolean }>({
    queryKey: ["/api/status"],
    queryFn: async () => {
      const res = await fetch(apiUrl("/api/status"), { credentials: "include" });
      return res.json();
    },
    refetchInterval: 8000,
  });

  // ── Mutations ──
  const syncMutation = (method: string, url: string) => 
    useMutation({
      mutationFn: async (data?: any) => {
        const res = await apiRequest(method, url, data);
        return res.json();
      },
      onSuccess: () => queryClient.invalidateQueries({ queryKey: ["/api"] }),
    });

  const approveMutation = syncMutation("POST", ""); // Dynamic below
  const resetMutation = useMutation({
    mutationFn: async () => {
      const res = await apiRequest("POST", "/api/reset");
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries();
      toast({ title: "System Reset", description: "All data purged." });
    }
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
    <div className="min-h-screen bg-[#070b14] text-slate-100 pb-20">
      <header className="bg-[rgba(7,11,20,0.8)] backdrop-blur-xl border-b border-white/5 px-6 py-4 flex justify-between items-center sticky top-0 z-50">
        <div className="flex items-center gap-3">
          <Shield className="w-6 h-6 text-emerald-500" />
          <h1 className="text-lg font-extrabold text-white">Security Command</h1>
        </div>
        <div className="flex items-center gap-4">
          <div className={`px-2 py-1 rounded-lg text-[10px] font-bold border ${isMasterOnline ? "border-emerald-500/20 text-emerald-400" : "border-red-500/20 text-red-400"}`}>
            {isMasterOnline ? "● CONNECTED" : "○ OFFLINE"}
          </div>
          <button onClick={() => logoutMutation.mutate()} className="text-slate-500 hover:text-white"><LogOut size={18}/></button>
        </div>
      </header>

      {activeTab === "dashboard" ? (
        <main className="p-6">
          {/* Stats, Pending, Active Nodes... Same as before but cleaner */}
          <div className="grid grid-cols-2 md:grid-cols-4 gap-3 mb-6">
             <div className="bg-white/[0.03] border border-white/5 p-4 rounded-xl">
               <div className="text-2xl font-bold">{approved.length}</div>
               <div className="text-[10px] text-slate-500 uppercase font-bold">Nodes</div>
             </div>
             <div className="bg-white/[0.03] border border-white/5 p-4 rounded-xl">
               <div className="text-2xl font-bold">{safeSlaves.filter(s => s.online).length}</div>
               <div className="text-[10px] text-emerald-500 uppercase font-bold">Online</div>
             </div>
             <div className="bg-white/[0.03] border border-white/5 p-4 rounded-xl">
               <div className="text-2xl font-bold">{safeSlaves.filter(s => s.alertActive).length}</div>
               <div className="text-[10px] text-red-500 uppercase font-bold">Alerts</div>
             </div>
             <div className="bg-white/[0.03] border border-white/5 p-4 rounded-xl">
               <div className="text-2xl font-bold">{pending.length}</div>
               <div className="text-[10px] text-amber-500 uppercase font-bold">Pending</div>
             </div>
          </div>

          {pending.length > 0 && (
            <div className="mb-6 space-y-2">
              <h2 className="text-xs font-bold text-slate-400 uppercase tracking-widest mb-2">Discovery Queue</h2>
              {pending.map(s => (
                <div key={s.slaveId} className="bg-amber-500/5 border border-amber-500/10 p-4 rounded-xl flex justify-between items-center">
                  <div className="font-mono text-amber-500">{s.slaveId}</div>
                  <button onClick={() => { setModalSlave(s); setModalMode("approve"); }} className="bg-amber-500 text-black px-4 py-1.5 rounded-lg text-xs font-bold">Authorize</button>
                </div>
              ))}
            </div>
          )}

          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3">
            {approved.map(d => (
              <div key={d.slaveId} className={`p-5 rounded-2xl border ${d.alertActive ? "bg-red-500/10 border-red-500/20" : "bg-white/[0.03] border-white/10"}`}>
                <div className="flex justify-between mb-4">
                  <div className="font-bold">{d.patientName}</div>
                  <div className={`text-[10px] px-2 py-0.5 rounded ${d.alertActive ? "bg-red-500 text-white" : "bg-white/5 text-slate-500"}`}>{d.alertActive ? "ALERT" : "STABLE"}</div>
                </div>
                <div className="text-xs text-slate-500 mb-4">Room {d.room} • Bed {d.bed}</div>
                <div className="flex gap-2">
                  <button onClick={() => { setModalSlave(d); setModalMode("edit"); }} className="bg-white/5 hover:bg-white/10 p-2 rounded-lg"><Pencil size={14}/></button>
                  {d.alertActive && <button onClick={() => apiRequest("POST", `/api/clearAlert/${d.slaveId}`)} className="bg-red-500 text-white flex-1 rounded-lg text-xs font-bold">Silence</button>}
                  <button onClick={() => { if(confirm("Purge node?")) apiRequest("DELETE", `/api/slaves/${d.slaveId}`); }} className="bg-red-500/10 text-red-500 p-2 rounded-lg"><Trash2 size={14}/></button>
                </div>
              </div>
            ))}
          </div>
        </main>
      ) : (
        <main className="p-6 max-w-2xl mx-auto space-y-6">
          <div className="bg-white/[0.03] border border-white/10 rounded-2xl p-6">
            <h3 className="text-sm font-bold mb-4">System Operation Mode</h3>
            <div className="grid grid-cols-1 gap-2">
              {[
                { m: 1, t: "AP Mode", d: "Local Only (No Cloud)" },
                { m: 2, t: "STA Mode", d: "Local Network" },
                { m: 3, t: "Bridge Mode", d: "Hybrid Network" },
                { m: 4, t: "Cloud Mode", d: "Full Global Remote Access" }
              ].map(m => (
                <button 
                  key={m.m}
                  onClick={() => apiRequest("POST", "/api/setup", { mode: m.m })}
                  className={`p-4 rounded-xl border text-left transition-all ${status?.mode === m.m ? "bg-emerald-500/10 border-emerald-500/30 ring-1 ring-emerald-500/20" : "bg-white/[0.02] border-white/5"}`}
                >
                  <div className="font-bold text-sm">{m.t}</div>
                  <div className="text-[11px] text-slate-500">{m.d}</div>
                </button>
              ))}
            </div>
          </div>

          <div className="bg-red-500/5 border border-red-500/10 rounded-2xl p-6">
            <h3 className="text-sm font-bold text-red-500 mb-2">Danger Zone</h3>
            <p className="text-xs text-slate-500 mb-4">Resetting the system will permanently wipe all approved slaves, patient data, and connection history. This cannot be undone.</p>
            <button 
              onClick={() => { if(confirm("WIPE ALL DATA? This is permanent.")) resetMutation.mutate(); }}
              className="bg-red-500 hover:bg-red-600 text-white px-6 py-2.5 rounded-xl text-sm font-bold transition-all"
            >
              Reset All Infrastructure
            </button>
          </div>
        </main>
      )}

      {/* ── Navigation ── */}
      <nav className="fixed bottom-0 left-0 right-0 bg-[rgba(15,23,42,0.95)] backdrop-blur-xl border-t border-white/5 flex p-2 pt-3 pb-8 gap-2 z-50">
        <button 
          onClick={() => setActiveTab("dashboard")}
          className={`flex-1 flex flex-col items-center gap-1 py-1 ${activeTab === "dashboard" ? "text-emerald-500" : "text-slate-500"}`}
        >
          <div className={`p-1 rounded-lg ${activeTab === "dashboard" ? "bg-emerald-500/10" : ""}`}><Wifi size={20}/></div>
          <span className="text-[10px] font-bold uppercase tracking-tighter">Nodes</span>
        </button>
        <button 
          onClick={() => setActiveTab("settings")}
          className={`flex-1 flex flex-col items-center gap-1 py-1 ${activeTab === "settings" ? "text-emerald-500" : "text-slate-500"}`}
        >
          <div className={`p-1 rounded-lg ${activeTab === "settings" ? "bg-emerald-500/10" : ""}`}><Shield size={20}/></div>
          <span className="text-[10px] font-bold uppercase tracking-tighter">Settings</span>
        </button>
      </nav>

      <SlaveModal
        title={modalMode === "approve" ? "Authorize Node" : "Modify Node"}
        open={!!modalSlave}
        onClose={() => setModalSlave(null)}
        onSubmit={(name, bed, room) => {
          if (!modalSlave) return;
          if (modalMode === "approve") {
            apiRequest("POST", `/api/approve/${modalSlave.slaveId}`, { patientName: name, bed, room })
              .then(() => { queryClient.invalidateQueries(); setModalSlave(null); });
          } else {
            apiRequest("PUT", `/api/slaves/${modalSlave.slaveId}`, { patientName: name, bed, room })
              .then(() => { queryClient.invalidateQueries(); setModalSlave(null); });
          }
        }}
        loading={false}
        defaults={modalMode === "edit" ? modalSlave ?? undefined : undefined}
      />
    </div>
  );
}

// ═══════════════════════════════════════════════════════════════
//  ENTRY — session check then render
// ═══════════════════════════════════════════════════════════════
export default function AdminPage() {
  const [loggedIn, setLoggedIn] = useState(false);
  const { data: session, isLoading } = useQuery<{ authenticated: boolean } | null>({
    queryKey: ["/api/admin/session"],
    queryFn: getQueryFn({ on401: "returnNull" }),
    staleTime: 0,
  });

  if (isLoading) {
    return (
      <div className="min-h-screen bg-[#070b14] flex items-center justify-center text-emerald-500 font-mono text-sm">
        STABILIZING_HANDSHAKE...
      </div>
    );
  }

  const isAuth = loggedIn || !!session?.authenticated;

  if (!isAuth) {
    return <LoginForm onLogin={() => setLoggedIn(true)} />;
  }

  return <AdminPanel />;
}
