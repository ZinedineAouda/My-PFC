import { useState } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import type { Slave, LoginData, ApproveSlave, UpdateSlave } from "@shared/schema";
import { loginSchema, approveSlaveSchema, updateSlaveSchema } from "@shared/schema";
import { apiRequest, apiUrl, queryClient } from "@/lib/queryClient";
import { getQueryFn } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import { useWebsocket } from "@/hooks/use-websocket";
import { Card, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Badge } from "@/components/ui/badge";
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
  DialogClose,
} from "@/components/ui/dialog";
import {
  Form,
  FormControl,
  FormField,
  FormItem,
  FormLabel,
  FormMessage,
} from "@/components/ui/form";
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
  AlertDialogTrigger,
} from "@/components/ui/alert-dialog";
import {
  Shield,
  Pencil,
  Trash2,
  BellOff,
  LogIn,
  LogOut,
  Activity,
  AlertTriangle,
  Lock,
  CheckCircle,
  Clock,
  Settings,
  Wifi,
  RefreshCw,
} from "lucide-react";
import { motion, AnimatePresence } from "framer-motion";

function LoginForm({ onLogin }: { onLogin: () => void }) {
  const { toast } = useToast();
  const form = useForm<LoginData>({
    resolver: zodResolver(loginSchema),
    defaultValues: { username: "", password: "" },
  });

  const loginMutation = useMutation({
    mutationFn: async (data: LoginData) => {
      const res = await apiRequest("POST", "/api/admin/login", data);
      if (!res.ok) throw new Error("Invalid credentials");
      return res.json();
    },
    onSuccess: () => {
      onLogin();
      toast({ title: "Authorized", description: "Terminal access granted." });
    },
    onError: (err: Error) => {
      toast({
        title: "Login Failed",
        description: err.message,
        variant: "destructive",
      });
    },
  });

  return (
    <div className="w-full font-['Outfit'] relative">
      <div className="fixed inset-0 pointer-events-none bg-[radial-gradient(circle_at_50%_50%,rgba(16,185,129,0.05)_0%,transparent_50%)]" />
      <motion.div 
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        className="w-full relative"
      >
        <div className="absolute -inset-1 bg-gradient-to-r from-emerald-500 to-cyan-500 rounded-2xl blur opacity-20" />
        <Card className="bg-[#0f172a]/80 border-white/10 backdrop-blur-2xl relative shadow-2xl">
          <CardContent className="pt-8 px-8 pb-10">
            <div className="flex flex-col items-center mb-10">
              <div className="w-16 h-16 rounded-2xl bg-emerald-500/20 border border-emerald-500/20 flex items-center justify-center mb-4">
                <Shield className="w-8 h-8 text-emerald-400" />
              </div>
              <h1 className="text-3xl font-extrabold text-white tracking-tight">Management Hub</h1>
              <p className="text-slate-400 text-sm mt-2">Secure Administrator Access</p>
            </div>
            <Form {...form}>
              <form onSubmit={form.handleSubmit((data) => loginMutation.mutate(data))} className="space-y-6">
                <FormField
                  control={form.control}
                  name="username"
                  render={({ field }) => (
                    <FormItem>
                      <FormLabel className="text-slate-400 text-xs font-bold uppercase tracking-widest">Username</FormLabel>
                      <FormControl>
                        <Input className="bg-white/5 border-white/10 text-white h-12 rounded-xl focus:ring-emerald-500/50" {...field} />
                      </FormControl>
                      <FormMessage />
                    </FormItem>
                  )}
                />
                <FormField
                  control={form.control}
                  name="password"
                  render={({ field }) => (
                    <FormItem>
                      <FormLabel className="text-slate-400 text-xs font-bold uppercase tracking-widest">Password</FormLabel>
                      <FormControl>
                        <Input type="password" className="bg-white/5 border-white/10 text-white h-12 rounded-xl focus:ring-emerald-500/50" {...field} />
                      </FormControl>
                      <FormMessage />
                    </FormItem>
                  )}
                />
                <Button 
                  type="submit" 
                  className="w-full h-12 bg-emerald-500 hover:bg-emerald-600 text-white font-bold rounded-xl transition-all shadow-lg shadow-emerald-500/20"
                  disabled={loginMutation.isPending}
                >
                  {loginMutation.isPending ? "Authenticating..." : "Establish Secure Link"}
                  {!loginMutation.isPending && <LogIn className="w-4 h-4 ml-2" />}
                </Button>
              </form>
            </Form>
          </CardContent>
        </Card>
      </motion.div>
    </div>
  );
}

function AdminPanel({ isAuthenticated }: { isAuthenticated: boolean }) {
  const { toast } = useToast();
  const queryClient = useQueryClient();
  const [approveSlaveData, setApproveSlaveData] = useState<Slave | null>(null);
  const [editSlaveData, setEditSlaveData] = useState<Slave | null>(null);
  const [showLogin, setShowLogin] = useState(false);

  const requireAuth = (callback: () => void) => {
    if (!isAuthenticated) {
      setShowLogin(true);
      return;
    }
    callback();
  };

  useWebsocket((msg) => {
    queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
    queryClient.invalidateQueries({ queryKey: ["/api/status"] });
    if (msg.type === "REGISTER") {
      toast({ title: "Sensory Input", description: "New hardware detected." });
    }
  });

  const { data: slaves, isLoading } = useQuery<Slave[]>({
    queryKey: ["/api/slaves", "all"],
    queryFn: async () => {
      const res = await fetch(apiUrl("/api/slaves?all=1"), { credentials: "include" });
      if (!res.ok) throw new Error("Sync failed");
      return res.json();
    },
  });

  const { data: status } = useQuery<{ mode: number; setup: boolean; isMasterOnline?: boolean }>({
    queryKey: ["/api/status"],
    queryFn: async () => {
      const res = await fetch(apiUrl("/api/status"), { credentials: "include" });
      if (!res.ok) throw new Error("Status sync failed");
      return res.json();
    },
  });

  const clearAlertMutation = useMutation({
    mutationFn: async (slaveId: string) => {
      const res = await apiRequest("POST", `/api/clearAlert/${slaveId}`);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "System Ready", description: "Alert state cleared." });
    },
    onError: (err) => toast({ title: "Link Failed", description: err.message, variant: "destructive" })
  });

  const deleteMutation = useMutation({
    mutationFn: async (slaveId: string) => {
      const res = await apiRequest("DELETE", `/api/slaves/${slaveId}`);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Node Decommissioned", description: "Device link severed." });
    },
    onError: (err) => toast({ title: "Decommission Failed", description: err.message, variant: "destructive" })
  });

  const logoutMutation = useMutation({
    mutationFn: async () => {
      await apiRequest("POST", "/api/admin/logout");
    },
    onSuccess: () => window.location.reload(),
    onError: (err) => toast({ title: "Logout Failed", description: err.message, variant: "destructive" })
  });

  const safeSlaves = Array.isArray(slaves) ? slaves : [];
  const pendingDevices = safeSlaves.filter((s) => !s.approved);
  const approvedDevices = safeSlaves.filter((s) => s.approved);
  const activeAlerts = approvedDevices.filter((s) => s.alertActive).length;
  const isMasterOnline = status?.isMasterOnline ?? false;

  return (
    <div className="min-h-screen bg-[#070b14] text-slate-100 font-['Outfit'] pb-20">
      <div className="fixed inset-0 pointer-events-none bg-[radial-gradient(circle_at_0%_0%,rgba(16,185,129,0.05)_0%,transparent_40%)]" />
      
      <header className="sticky top-0 z-50 border-b border-white/5 bg-[#070b14]/80 backdrop-blur-xl">
        <div className="max-w-7xl mx-auto px-6 py-5">
          <div className="flex items-center justify-between gap-6 flex-wrap">
            <div className="flex items-center gap-4">
              <div className="flex items-center justify-center w-12 h-12 rounded-2xl bg-slate-800 border border-white/10">
                <Shield className="w-6 h-6 text-emerald-400" />
              </div>
              <div>
                <h1 className="text-xl font-bold text-white">Security Command</h1>
                <p className="text-xs font-bold text-emerald-500/60 tracking-widest uppercase">Encryption Active</p>
              </div>
            </div>
            
            <div className="flex items-center gap-2 flex-wrap">
              <div className={`px-4 py-2 rounded-xl border text-sm font-black flex items-center gap-2 shadow-lg tracking-wide ${isMasterOnline ? 'bg-emerald-500/20 border-emerald-500/40 text-emerald-400 shadow-emerald-500/10' : 'bg-red-500/10 border-red-500/20 text-red-500 shadow-red-500/10'}`}>
                <RefreshCw className={`w-4 h-4 ${isMasterOnline ? 'animate-spin-slow text-emerald-400' : 'text-red-500'}`} />
                {isMasterOnline ? 'ESP32 MASTER: CONNECTED' : 'ESP32 MASTER: OFFLINE'}
              </div>

              {isAuthenticated ? (
                <Button variant="ghost" size="sm" onClick={() => logoutMutation.mutate()} className="text-slate-500 hover:text-red-400"><LogOut className="w-4 h-4 mr-2" /> Exit</Button>
              ) : (
                <Button variant="ghost" size="sm" onClick={() => setShowLogin(true)} className="text-emerald-500 hover:text-emerald-400"><LogIn className="w-4 h-4 mr-2" /> Login</Button>
              )}
            </div>
          </div>
        </div>
      </header>

      <main className="max-w-7xl mx-auto px-6 py-10 space-y-12 relative">
        <section>
          <div className="flex items-center gap-3 mb-6">
            <Clock className="w-5 h-5 text-amber-500" />
            <h2 className="text-lg font-bold text-white tracking-tight">Discovery Queue</h2>
            <div className="px-2 py-0.5 rounded-lg bg-white/5 border border-white/10 text-xs font-bold text-slate-400">{pendingDevices.length} NEW</div>
          </div>
          
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            <AnimatePresence mode="popLayout">
              {pendingDevices.map((slave) => (
                <motion.div
                  key={slave.slaveId}
                  layout
                  initial={{ opacity: 0, scale: 0.95 }}
                  animate={{ opacity: 1, scale: 1 }}
                  exit={{ opacity: 0, scale: 0.95 }}
                >
                  <Card className="bg-slate-900/40 border-amber-500/20 backdrop-blur-xl group hover:border-amber-500/40 transition-colors">
                    <CardContent className="p-5">
                      <div className="flex justify-between items-start mb-4">
                        <div>
                          <p className="text-xs font-bold text-amber-500 tracking-widest uppercase mb-1">Signal Detected</p>
                          <h3 className="text-lg font-bold text-white font-mono">{slave.slaveId}</h3>
                        </div>
                        <div className="w-8 h-8 rounded-lg bg-amber-500/10 flex items-center justify-center">
                          <Wifi className="w-4 h-4 text-amber-500 animate-ping" />
                        </div>
                      </div>
                      <div className="flex gap-2 mt-6">
                        <Button className="flex-1 bg-amber-500 hover:bg-amber-600 text-white font-bold rounded-xl" onClick={() => requireAuth(() => setApproveSlaveData(slave))}>Authorize</Button>
                        <Button variant="ghost" className="text-slate-500 hover:text-red-400" onClick={() => requireAuth(() => deleteMutation.mutate(slave.slaveId))}><Trash2 className="w-4 h-4" /></Button>
                      </div>
                    </CardContent>
                  </Card>
                </motion.div>
              ))}
            </AnimatePresence>
            {pendingDevices.length === 0 && (
              <div className="col-span-full py-12 text-center rounded-2xl border border-dashed border-white/10 text-slate-500 font-medium">Scanning for new hardware...</div>
            )}
          </div>
        </section>

        <section>
          <div className="flex items-center gap-3 mb-6">
            <CheckCircle className="w-5 h-5 text-emerald-500" />
            <h2 className="text-lg font-bold text-white tracking-tight">Active Infrastructure</h2>
            <div className="px-2 py-0.5 rounded-lg bg-white/5 border border-white/10 text-xs font-bold text-slate-400">{approvedDevices.length} NODES</div>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            <AnimatePresence mode="popLayout">
              {approvedDevices.map((slave) => (
                <motion.div
                  key={slave.slaveId}
                  layout
                  initial={{ opacity: 0 }}
                  animate={{ opacity: 1 }}
                >
                  <Card className={`bg-slate-900/40 backdrop-blur-xl border-l-4 transition-all ${slave.alertActive ? "border-red-500/50 shadow-lg shadow-red-500/10" : "border-emerald-500/30"}`}>
                    <CardContent className="p-5">
                      <div className="flex justify-between mb-4">
                        <div className="flex-1 min-w-0">
                          <h4 className="text-white font-bold truncate leading-tight">{slave.patientName || "NODE_01"}</h4>
                          <p className="text-[10px] font-bold text-slate-500 tracking-tighter uppercase mt-1">ID: {slave.slaveId}</p>
                        </div>
                        <Badge className={`${slave.alertActive ? 'bg-red-500 animate-pulse' : 'bg-white/5 text-slate-500'} border-none`}>{slave.alertActive ? 'EMERGENCY' : 'LINKED'}</Badge>
                      </div>
                      
                      <div className="grid grid-cols-2 gap-2 mb-5">
                        <div className="p-2 rounded-lg bg-white/5 border border-white/5 text-[11px]">
                          <span className="block text-slate-500 uppercase font-bold tracking-widest">Bed</span>
                          <span className="text-white font-bold">{slave.bed || "-"}</span>
                        </div>
                        <div className="p-2 rounded-lg bg-white/5 border border-white/5 text-[11px]">
                          <span className="block text-slate-500 uppercase font-bold tracking-widest">Room</span>
                          <span className="text-white font-bold">{slave.room || "-"}</span>
                        </div>
                      </div>

                      <div className="flex items-center gap-2">
                        {slave.alertActive && <Button size="sm" className="flex-1 bg-red-500 hover:bg-red-600 text-white font-bold rounded-lg" onClick={() => requireAuth(() => clearAlertMutation.mutate(slave.slaveId))}>Silence</Button>}
                        <Button size="sm" variant="secondary" className="flex-1 bg-white/5 text-slate-300 rounded-lg hover:bg-white/10" onClick={() => requireAuth(() => setEditSlaveData(slave))}><Pencil className="w-3 h-3 mr-2" /> Modify</Button>
                        <AlertDialog>
                          <AlertDialogTrigger asChild><Button size="sm" variant="ghost" className="text-slate-600 hover:text-red-400"><Trash2 className="w-4 h-4" /></Button></AlertDialogTrigger>
                          <AlertDialogContent className="bg-slate-900 border-white/10 text-white">
                            <AlertDialogHeader><AlertDialogTitle>Confirm Purge</AlertDialogTitle><AlertDialogDescription className="text-slate-400">Sever all links with {slave.patientName}? Critical alert capability will be lost.</AlertDialogDescription></AlertDialogHeader>
                            <AlertDialogFooter><AlertDialogCancel className="bg-white/5 text-white border-none">Abort</AlertDialogCancel><AlertDialogAction onClick={() => requireAuth(() => deleteMutation.mutate(slave.slaveId))} className="bg-red-500 hover:bg-red-600">Execute Delete</AlertDialogAction></AlertDialogFooter>
                          </AlertDialogContent>
                        </AlertDialog>
                      </div>
                    </CardContent>
                  </Card>
                </motion.div>
              ))}
            </AnimatePresence>
          </div>
        </section>
      </main>

      <Dialog open={!!approveSlaveData} onOpenChange={(o) => !o && setApproveSlaveData(null)}>
        <DialogContent className="bg-[#0f172a] border-white/10 text-white font-['Outfit'] sm:max-w-md">
          <DialogHeader><DialogTitle className="text-xl font-bold">Node Authorization: {approveSlaveData?.slaveId}</DialogTitle></DialogHeader>
          <ApproveForm slaveId={approveSlaveData?.slaveId || ""} onComplete={() => setApproveSlaveData(null)} />
        </DialogContent>
      </Dialog>

      <Dialog open={!!editSlaveData} onOpenChange={(o) => !o && setEditSlaveData(null)}>
        <DialogContent className="bg-[#0f172a] border-white/10 text-white font-['Outfit'] sm:max-w-md">
          <DialogHeader><DialogTitle className="text-xl font-bold">Modify Infrastructure</DialogTitle></DialogHeader>
          {editSlaveData && <EditForm slave={editSlaveData} onComplete={() => setEditSlaveData(null)} />}
        </DialogContent>
      </Dialog>

      <Dialog open={showLogin} onOpenChange={setShowLogin}>
        <DialogContent className="bg-transparent border-none p-0 max-w-md shadow-none">
          <LoginForm onLogin={() => { 
            setShowLogin(false); 
            queryClient.invalidateQueries({ queryKey: ["/api/admin/session"] }); 
          }} />
        </DialogContent>
      </Dialog>
    </div>
  );
}

function ApproveForm({ slaveId, onComplete }: { slaveId: string, onComplete: () => void }) {
  const { toast } = useToast();
  const queryClient = useQueryClient();
  const form = useForm<ApproveSlave>({
    resolver: zodResolver(approveSlaveSchema),
    defaultValues: { patientName: "", bed: "", room: "" },
  });

  const mutation = useMutation({
    mutationFn: (data: ApproveSlave) => apiRequest("POST", `/api/approve/${slaveId}`, data),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Authorized", description: "Node added to network." });
      onComplete();
    },
  });

  return (
    <Form {...form}>
      <form onSubmit={form.handleSubmit((d) => mutation.mutate(d))} className="space-y-4 pt-4">
        <FormField control={form.control} name="patientName" render={({ field }) => (
          <FormItem><FormLabel className="text-slate-500 text-[10px] font-bold uppercase">Subject Name</FormLabel><FormControl><Input className="bg-white/5 border-white/10" {...field} /></FormControl></FormItem>
        )} />
        <div className="grid grid-cols-2 gap-4">
          <FormField control={form.control} name="bed" render={({ field }) => (
            <FormItem><FormLabel className="text-slate-500 text-[10px] font-bold uppercase">Bed</FormLabel><FormControl><Input className="bg-white/5 border-white/10" {...field} /></FormControl></FormItem>
          )} />
          <FormField control={form.control} name="room" render={({ field }) => (
            <FormItem><FormLabel className="text-slate-500 text-[10px] font-bold uppercase">Room</FormLabel><FormControl><Input className="bg-white/5 border-white/10" {...field} /></FormControl></FormItem>
          )} />
        </div>
        <Button type="submit" className="w-full bg-emerald-500 hover:bg-emerald-600 font-bold mt-4" disabled={mutation.isPending}>Activate Node</Button>
      </form>
    </Form>
  );
}

function EditForm({ slave, onComplete }: { slave: Slave, onComplete: () => void }) {
  const { toast } = useToast();
  const queryClient = useQueryClient();
  const form = useForm({
    resolver: zodResolver(updateSlaveSchema),
    defaultValues: { patientName: slave.patientName || "", bed: slave.bed || "", room: slave.room || "" },
  });

  const mutation = useMutation({
    mutationFn: (data: any) => apiRequest("PUT", `/api/slaves/${slave.slaveId}`, data),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Updated", description: "Node parameters modified." });
      onComplete();
    },
  });

  return (
    <Form {...form}>
      <form onSubmit={form.handleSubmit((d) => mutation.mutate(d))} className="space-y-4 pt-4">
        <FormField control={form.control} name="patientName" render={({ field }) => (
          <FormItem><FormLabel className="text-slate-500 text-[10px] font-bold uppercase">Subject Name</FormLabel><FormControl><Input className="bg-white/5 border-white/10" {...field} /></FormControl></FormItem>
        )} />
        <div className="grid grid-cols-2 gap-4">
          <FormField control={form.control} name="bed" render={({ field }) => (
            <FormItem><FormLabel className="text-slate-500 text-[10px] font-bold uppercase">Bed</FormLabel><FormControl><Input className="bg-white/5 border-white/10" {...field} /></FormControl></FormItem>
          )} />
          <FormField control={form.control} name="room" render={({ field }) => (
            <FormItem><FormLabel className="text-slate-500 text-[10px] font-bold uppercase">Room</FormLabel><FormControl><Input className="bg-white/5 border-white/10" {...field} /></FormControl></FormItem>
          )} />
        </div>
        <Button type="submit" className="w-full bg-emerald-500 hover:bg-emerald-600 font-bold mt-4" disabled={mutation.isPending}>Apply Changes</Button>
      </form>
    </Form>
  );
}

export default function AdminPage() {
  const { data: session, isLoading } = useQuery<{ authenticated: boolean } | null>({
    queryKey: ["/api/admin/session"],
    queryFn: getQueryFn({ on401: "returnNull" }),
  });

  if (isLoading) return <div className="min-h-screen bg-[#070b14] flex items-center justify-center text-emerald-500 font-mono">STABILIZING_HANDSHAKE...</div>;

  return <AdminPanel isAuthenticated={!!session?.authenticated} />;
}
