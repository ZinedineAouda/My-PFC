import { useState } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import type { Slave } from "@shared/schema";
import { apiRequest, apiUrl } from "@/lib/queryClient";
import { getQueryFn } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import { useWebsocket } from "@/hooks/use-websocket";
import { Shield, RefreshCw, LogIn, LogOut, Wifi, Trash2, Pencil, BellOff } from "lucide-react";

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
      const res = await fetch(apiUrl("/api/admin/login"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        credentials: "include",
        body: JSON.stringify({ username: user, password: pass }),
      });
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
//  ADMIN PANEL — Unified design matching ESP32 dashboard
// ═══════════════════════════════════════════════════════════════
function AdminPanel() {
  const { toast } = useToast();
  const queryClient = useQueryClient();
  const [modalSlave, setModalSlave] = useState<Slave | null>(null);
  const [modalMode, setModalMode] = useState<"approve" | "edit">("approve");

  // ── WebSocket for live updates ──
  useWebsocket((msg) => {
    queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
    queryClient.invalidateQueries({ queryKey: ["/api/status"] });
    if (msg.type === "REGISTER") {
      toast({ title: "Signal Detected", description: "New hardware discovered." });
    }
  });

  // ── Data queries ──
  const { data: slaves } = useQuery<Slave[]>({
    queryKey: ["/api/slaves", "all"],
    queryFn: async () => {
      const res = await fetch(apiUrl("/api/slaves?all=1"), { credentials: "include" });
      if (!res.ok) throw new Error("Sync failed");
      return res.json();
    },
    staleTime: 0,
    refetchInterval: 10000,
  });

  const { data: status } = useQuery<{ mode: number; isMasterOnline?: boolean }>({
    queryKey: ["/api/status"],
    queryFn: async () => {
      const res = await fetch(apiUrl("/api/status"), { credentials: "include" });
      if (!res.ok) throw new Error("Status sync failed");
      return res.json();
    },
    staleTime: 0,
    refetchInterval: 8000,
  });

  // ── Mutations ──
  const approveMutation = useMutation({
    mutationFn: async ({ slaveId, data }: { slaveId: string; data: { patientName: string; bed: string; room: string } }) => {
      const res = await apiRequest("POST", `/api/approve/${slaveId}`, data);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Authorized", description: "Node added to network." });
      setModalSlave(null);
    },
    onError: (err) => toast({ title: "Failed", description: err.message, variant: "destructive" }),
  });

  const updateMutation = useMutation({
    mutationFn: async ({ slaveId, data }: { slaveId: string; data: { patientName: string; bed: string; room: string } }) => {
      const res = await apiRequest("PUT", `/api/slaves/${slaveId}`, data);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Updated", description: "Node parameters modified." });
      setModalSlave(null);
    },
    onError: (err) => toast({ title: "Failed", description: err.message, variant: "destructive" }),
  });

  const deleteMutation = useMutation({
    mutationFn: async (slaveId: string) => {
      const res = await apiRequest("DELETE", `/api/slaves/${slaveId}`);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Deleted", description: "Device link severed." });
    },
    onError: (err) => toast({ title: "Failed", description: err.message, variant: "destructive" }),
  });

  const clearAlertMutation = useMutation({
    mutationFn: async (slaveId: string) => {
      const res = await apiRequest("POST", `/api/clearAlert/${slaveId}`);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Cleared", description: "Alert silenced." });
    },
  });

  const logoutMutation = useMutation({
    mutationFn: async () => { await apiRequest("POST", "/api/admin/logout"); },
    onSuccess: () => window.location.reload(),
  });

  // ── Derived state ──
  const safeSlaves = Array.isArray(slaves) ? slaves : [];
  const pending = safeSlaves.filter((s) => !s.approved);
  const approved = safeSlaves.filter((s) => s.approved);
  const online = safeSlaves.filter((s) => s.online).length;
  const alerts = safeSlaves.filter((s) => s.alertActive).length;
  const isMasterOnline = status?.isMasterOnline ?? false;

  // ── Helpers ──
  const openApprove = (s: Slave) => { setModalMode("approve"); setModalSlave(s); };
  const openEdit = (s: Slave) => { setModalMode("edit"); setModalSlave(s); };

  const handleModalSubmit = (name: string, bed: string, room: string) => {
    if (!modalSlave) return;
    const data = { patientName: name, bed, room };
    if (modalMode === "approve") {
      approveMutation.mutate({ slaveId: modalSlave.slaveId, data });
    } else {
      updateMutation.mutate({ slaveId: modalSlave.slaveId, data });
    }
  };

  const formatTime = (t: string | number | null | undefined) => {
    if (!t) return "Never";
    const ms = typeof t === "string" ? new Date(t).getTime() : t;
    const s = Math.floor((Date.now() - ms) / 1000);
    if (s < 5) return "Just now";
    if (s < 60) return s + "s ago";
    if (s < 3600) return Math.floor(s / 60) + "m ago";
    return Math.floor(s / 3600) + "h ago";
  };

  return (
    <div className="min-h-screen bg-[#070b14] text-slate-100 font-['Segoe_UI',system-ui,-apple-system,sans-serif]">
      <div className="fixed inset-0 pointer-events-none bg-[radial-gradient(circle_at_0%_0%,rgba(16,185,129,0.05)_0%,transparent_40%)]" />

      {/* ── Header ── */}
      <header className="bg-[rgba(7,11,20,0.8)] backdrop-blur-2xl border-b border-white/5 px-6 py-4 flex justify-between items-center sticky top-0 z-50">
        <div className="flex items-center gap-3">
          <div className="w-12 h-12 bg-[rgba(15,23,42,0.8)] border border-white/10 rounded-2xl flex items-center justify-center">
            <Shield className="w-6 h-6 text-emerald-500" />
          </div>
          <div>
            <h1 className="text-lg font-extrabold text-white tracking-tight">Security Command</h1>
            <p className="text-[10px] text-emerald-500/50 uppercase tracking-[1.5px] font-bold">Administration</p>
          </div>
        </div>
        <div className="flex items-center gap-2.5">
          <div className={`px-3 py-1.5 rounded-xl text-[10px] font-bold tracking-wider uppercase flex items-center gap-1.5 border ${
            isMasterOnline
              ? "bg-emerald-500/[0.08] border-emerald-500/15 text-emerald-400"
              : "bg-red-500/[0.08] border-red-500/15 text-red-400"
          }`}>
            <div className={`w-1.5 h-1.5 rounded-full ${isMasterOnline ? "bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.25)]" : "bg-red-500"}`} />
            <RefreshCw className={`w-3 h-3 ${isMasterOnline ? "animate-spin" : ""}`} style={{ animationDuration: "3s" }} />
            {isMasterOnline ? "CONNECTED" : "OFFLINE"}
          </div>
          <button
            onClick={() => logoutMutation.mutate()}
            className="px-3 py-1.5 rounded-xl border border-white/10 bg-transparent text-slate-400 text-[11px] font-bold hover:bg-white/[0.04] hover:text-white transition-all flex items-center gap-1.5"
          >
            <LogOut className="w-3.5 h-3.5" /> Exit
          </button>
        </div>
      </header>

      {/* ── Stats ── */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-3 px-6 py-5">
        {[
          { icon: "📡", val: approved.length, lbl: "Total", cls: "bg-white/[0.04]" },
          { icon: "🟢", val: online, lbl: "Online", cls: "bg-emerald-500/10" },
          { icon: "🚨", val: alerts, lbl: "Alerts", cls: "bg-red-500/10" },
          { icon: "⏳", val: pending.length, lbl: "Pending", cls: "bg-amber-500/10" },
        ].map((s) => (
          <div key={s.lbl} className="bg-[rgba(15,23,42,0.8)] backdrop-blur-2xl border border-white/10 rounded-[14px] px-4 py-3.5 flex items-center gap-3">
            <div className={`w-10 h-10 rounded-xl flex items-center justify-center text-lg ${s.cls}`}>{s.icon}</div>
            <div>
              <div className="text-xl font-extrabold text-white">{s.val}</div>
              <div className="text-[9px] text-slate-500 uppercase tracking-wider font-bold">{s.lbl}</div>
            </div>
          </div>
        ))}
      </div>

      <main className="px-6 pb-8 relative z-10">
        {/* ── Discovery Queue ── */}
        {pending.length > 0 && (
          <section className="mb-6">
            <div className="flex items-center gap-2 mb-3">
              <span className="text-sm">⚠️</span>
              <h2 className="text-sm font-bold text-white">Discovery Queue</h2>
              <span className="px-2 py-0.5 rounded-lg bg-white/[0.04] border border-white/10 text-[10px] font-bold text-slate-400">
                {pending.length} NEW
              </span>
            </div>
            <div className="space-y-2">
              {pending.map((s) => (
                <div
                  key={s.slaveId}
                  className="bg-[rgba(15,23,42,0.4)] border border-white/5 border-l-[3px] border-l-amber-500 rounded-[14px] px-5 py-3.5 flex justify-between items-center backdrop-blur-xl"
                >
                  <div>
                    <div className="font-mono text-sm font-bold text-amber-500">{s.slaveId}</div>
                    <div className="text-[10px] text-slate-500 font-bold uppercase tracking-wider mt-0.5">Signal Detected</div>
                  </div>
                  <div className="flex gap-2">
                    <button
                      onClick={() => openApprove(s)}
                      className="px-4 py-2 bg-emerald-500 hover:shadow-lg hover:shadow-emerald-500/25 text-white rounded-xl text-[11px] font-bold transition-all"
                    >
                      Authorize
                    </button>
                    <button
                      onClick={() => { if (confirm(`Delete ${s.slaveId}?`)) deleteMutation.mutate(s.slaveId); }}
                      className="px-3 py-2 bg-red-500/[0.06] text-red-400 border border-red-500/10 rounded-xl text-[11px] font-bold hover:bg-red-500/10 transition-all"
                    >
                      <Trash2 className="w-3.5 h-3.5" />
                    </button>
                  </div>
                </div>
              ))}
            </div>
          </section>
        )}

        {/* ── Active Infrastructure ── */}
        <section>
          <div className="flex items-center gap-2 mb-3">
            <span className="text-sm">✅</span>
            <h2 className="text-sm font-bold text-white">Active Infrastructure</h2>
            <span className="px-2 py-0.5 rounded-lg bg-white/[0.04] border border-white/10 text-[10px] font-bold text-slate-400">
              {approved.length} NODES
            </span>
          </div>
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-3.5">
            {approved.map((d) => {
              const cls = d.alertActive ? "alert" : d.online ? "on" : "off";
              const badge = d.alertActive ? "EMERGENCY" : d.online ? "LINKED" : "OFFLINE";
              const statusText = d.alertActive ? "ALERT ACTIVE" : d.online ? "SYSTEM STABLE" : "LINK DOWN";
              return (
                <div
                  key={d.slaveId}
                  className={`bg-[rgba(15,23,42,0.8)] border border-white/10 rounded-2xl p-5 relative overflow-hidden backdrop-blur-2xl transition-all hover:border-emerald-500/20 ${
                    d.alertActive ? "border-red-500/40 bg-red-500/[0.05]" : ""
                  } ${!d.online && !d.alertActive ? "opacity-50" : ""}`}
                >
                  {d.alertActive && <div className="absolute inset-0 bg-red-500/[0.04] animate-pulse pointer-events-none" />}
                  <div className="flex justify-between items-start mb-3.5 relative">
                    <div>
                      <div className="text-base font-extrabold text-white tracking-tight">{d.patientName || "Unnamed"}</div>
                      <div className="text-[10px] text-slate-500 font-bold uppercase tracking-wider mt-0.5">ID: {d.slaveId}</div>
                    </div>
                    <div className={`px-2.5 py-1 rounded-lg text-[10px] font-bold uppercase tracking-wider ${
                      d.alertActive
                        ? "bg-red-500 text-white animate-pulse"
                        : d.online
                        ? "bg-white/[0.04] text-slate-500"
                        : "bg-white/[0.03] text-slate-600"
                    }`}>
                      {badge}
                    </div>
                  </div>
                  <div className="grid grid-cols-2 gap-2 mb-3.5 relative">
                    <div className="bg-white/[0.02] border border-white/5 rounded-xl p-2.5">
                      <div className="text-[9px] text-slate-500 uppercase tracking-wider font-bold mb-0.5">Bed</div>
                      <div className="text-sm font-bold text-white">{d.bed || "-"}</div>
                    </div>
                    <div className="bg-white/[0.02] border border-white/5 rounded-xl p-2.5">
                      <div className="text-[9px] text-slate-500 uppercase tracking-wider font-bold mb-0.5">Room</div>
                      <div className="text-sm font-bold text-white">{d.room || "-"}</div>
                    </div>
                  </div>
                  {/* Actions */}
                  <div className="flex gap-1.5 mb-3 relative">
                    <button
                      onClick={() => openEdit(d)}
                      className="flex-1 py-2 bg-white/[0.04] text-slate-400 border border-white/10 rounded-lg text-[10px] font-bold hover:bg-white/[0.06] transition-all flex items-center justify-center gap-1"
                    >
                      <Pencil className="w-3 h-3" /> Modify
                    </button>
                    {d.alertActive && (
                      <button
                        onClick={() => clearAlertMutation.mutate(d.slaveId)}
                        className="flex-1 py-2 bg-red-500/[0.08] text-red-400 border border-red-500/12 rounded-lg text-[10px] font-bold hover:bg-red-500/15 transition-all flex items-center justify-center gap-1"
                      >
                        <BellOff className="w-3 h-3" /> Silence
                      </button>
                    )}
                    <button
                      onClick={() => { if (confirm(`Delete ${d.slaveId}?`)) deleteMutation.mutate(d.slaveId); }}
                      className="py-2 px-2.5 bg-red-500/[0.06] text-red-400 border border-red-500/10 rounded-lg text-[10px] font-bold hover:bg-red-500/10 transition-all"
                    >
                      <Trash2 className="w-3 h-3" />
                    </button>
                  </div>
                  {/* Footer */}
                  <div className="pt-3 border-t border-white/5 flex justify-between items-center text-[9px] font-bold uppercase tracking-wider text-slate-500 relative">
                    <div className="flex items-center gap-1.5">
                      <div className={`w-1.5 h-1.5 rounded-full ${
                        d.alertActive ? "bg-red-500 shadow-[0_0_6px_rgba(239,68,68,0.25)]" : d.online ? "bg-emerald-500 shadow-[0_0_6px_rgba(16,185,129,0.25)]" : "bg-slate-600"
                      }`} />
                      {statusText}
                    </div>
                    <div>🕐 {formatTime(d.lastAlertTime)}</div>
                  </div>
                </div>
              );
            })}
            {approved.length === 0 && (
              <div className="col-span-full text-center py-16 border border-dashed border-white/10 rounded-2xl">
                <div className="text-4xl opacity-30 mb-3">📡</div>
                <div className="text-sm text-slate-500">Scanning for new hardware...<br />Connect a slave device to begin monitoring.</div>
              </div>
            )}
          </div>
        </section>
      </main>

      {/* ── Approve / Edit Modal ── */}
      <SlaveModal
        title={modalMode === "approve" ? `Node Authorization: ${modalSlave?.slaveId}` : `Modify: ${modalSlave?.slaveId}`}
        open={!!modalSlave}
        onClose={() => setModalSlave(null)}
        onSubmit={handleModalSubmit}
        loading={approveMutation.isPending || updateMutation.isPending}
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
