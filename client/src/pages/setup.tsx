import { useState } from "react";
import { useQuery, useMutation } from "@tanstack/react-query";
import { apiRequest } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import { Card, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Badge } from "@/components/ui/badge";
import { Wifi, Radio, Globe, ArrowRight, Check, Loader2, RefreshCw, Cloud, Activity, Shield } from "lucide-react";
import { motion, AnimatePresence } from "framer-motion";

type WifiNetwork = { ssid: string; rssi: number; secure: boolean };

export default function SetupPage() {
  const { toast } = useToast();
  const [step, setStep] = useState(1);
  const [selectedMode, setSelectedMode] = useState(0);
  const [selectedSSID, setSelectedSSID] = useState("");
  const [password, setPassword] = useState("");
  const [connectedIP, setConnectedIP] = useState("");

  const { data: networks, refetch: refetchNetworks, isFetching: scanning } = useQuery<WifiNetwork[]>({
    queryKey: ["/api/scan"],
    enabled: step === 2,
  });

  const connectMutation = useMutation({
    mutationFn: async () => {
      const res = await apiRequest("POST", "/api/connect-wifi", { ssid: selectedSSID, password });
      return res.json();
    },
    onSuccess: (data: any) => {
      if (data.success) {
        setConnectedIP(data.ip);
        applySetup();
      } else {
        toast({ title: "Auth Failure", description: data.message, variant: "destructive" });
      }
    },
  });

  const setupMutation = useMutation({
    mutationFn: async () => {
      const res = await apiRequest("POST", "/api/setup", { mode: selectedMode });
      return res.json();
    },
    onSuccess: () => {
      setStep(3);
      toast({ title: "Configuration Applied", description: "Node is now online." });
    },
  });

  function applySetup() {
    setupMutation.mutate();
  }

  function goStep2() {
    if (selectedMode === 1) {
      applySetup();
      return;
    }
    setStep(2);
  }

  const modeOptions = [
    { id: 1, icon: <Radio className="w-5 h-5 text-emerald-400" />, title: "AP Native", desc: "Master creates a localized broadcast cell. Ideal for isolated wards." },
    { id: 2, icon: <Wifi className="w-5 h-5 text-emerald-400" />, title: "Intranet STA", desc: "Integrate with hospital LAN infrastructure. Wide-area coverage." },
    { id: 3, icon: <Activity className="w-5 h-5 text-emerald-400" />, title: "Hybrid Mesh", desc: "Combines AP and STA for maximum redundancy and signal reach." },
    { id: 4, icon: <Cloud className="w-5 h-5 text-emerald-400" />, title: "Cloud Relay", desc: "Direct uplink to Railway servers. Enables global remote monitoring." },
  ];

  const modeNames: Record<number, string> = { 1: "Local AP", 2: "Hospital STA", 3: "Hybrid Hub", 4: "Railway Cloud" };

  return (
    <div className="min-h-screen bg-[#070b14] flex items-center justify-center p-6 font-['Outfit'] pt-20 pb-20 overflow-hidden">
      <div className="fixed inset-0 pointer-events-none bg-[radial-gradient(circle_at_50%_0%,rgba(16,185,129,0.05)_0%,transparent_50%)]" />
      
      <motion.div initial={{ opacity: 0, y: 20 }} animate={{ opacity: 1, y: 0 }} className="w-full max-w-xl relative">
        <div className="absolute -inset-1 bg-gradient-to-r from-emerald-500 to-cyan-500 rounded-2xl blur opacity-10" />
        
        <Card className="bg-[#0f172a]/80 border-white/10 backdrop-blur-2xl relative shadow-2xl">
          <CardContent className="pt-10 px-8 pb-10">
            <div className="flex justify-center gap-3 mb-10">
              {[1, 2, 3].map((s) => (
                <div
                  key={s}
                  className={`h-1.5 rounded-full transition-all duration-500 ${
                    s < step ? "w-8 bg-emerald-500" : s === step ? "w-12 bg-emerald-400 shadow-[0_0_8px_rgba(52,211,153,0.5)]" : "w-4 bg-white/10"
                  }`}
                />
              ))}
            </div>

            <AnimatePresence mode="wait">
              {step === 1 && (
                <motion.div key="step1" initial={{ opacity: 0, x: 20 }} animate={{ opacity: 1, x: 0 }} exit={{ opacity: 0, x: -20 }}>
                  <div className="text-center mb-8">
                    <div className="w-16 h-16 rounded-2xl bg-emerald-500/10 border border-emerald-500/20 flex items-center justify-center mx-auto mb-4">
                      <Shield className="w-8 h-8 text-emerald-400" />
                    </div>
                    <h1 className="text-2xl font-bold text-white tracking-tight">Deployment Config</h1>
                    <p className="text-slate-400 text-sm mt-1">Select the operational protocol</p>
                  </div>

                  <div className="space-y-3">
                    {modeOptions.map((mode) => (
                      <div
                        key={mode.id}
                        onClick={() => setSelectedMode(mode.id)}
                        className={`p-4 rounded-2xl border transition-all cursor-pointer group ${
                          selectedMode === mode.id
                            ? "border-emerald-500/50 bg-emerald-500/5 ring-1 ring-emerald-500/20 shadow-[0_0_20px_rgba(16,185,129,0.1)]"
                            : "border-white/5 bg-white/5 hover:border-white/20"
                        }`}
                      >
                        <div className="flex items-center gap-4">
                          <div className={`p-2.5 rounded-xl transition-colors ${selectedMode === mode.id ? 'bg-emerald-500 text-white' : 'bg-white/5 text-emerald-400'}`}>
                            {mode.icon}
                          </div>
                          <div className="flex-1">
                            <span className="font-bold text-white block leading-none">{mode.title}</span>
                            <p className="text-xs text-slate-500 mt-1.5 leading-relaxed">{mode.desc}</p>
                          </div>
                        </div>
                      </div>
                    ))}
                  </div>

                  <Button
                    className="w-full h-14 mt-8 bg-emerald-500 hover:bg-emerald-600 text-white font-bold rounded-2xl transition-all shadow-lg shadow-emerald-500/20"
                    disabled={selectedMode === 0}
                    onClick={goStep2}
                  >
                    Initialize Connection
                    <ArrowRight className="w-4 h-4 ml-3" />
                  </Button>
                </motion.div>
              )}

              {step === 2 && (
                <motion.div key="step2" initial={{ opacity: 0, x: 20 }} animate={{ opacity: 1, x: 0 }} exit={{ opacity: 0, x: -20 }}>
                  <div className="text-center mb-8">
                    <div className="w-16 h-16 rounded-2xl bg-emerald-500/10 border border-emerald-500/20 flex items-center justify-center mx-auto mb-4">
                      <Wifi className="w-8 h-8 text-emerald-400" />
                    </div>
                    <h2 className="text-2xl font-bold text-white tracking-tight">Bridge Establishment</h2>
                    <p className="text-slate-400 text-sm mt-1">Search and connect to hospital network</p>
                  </div>

                  <div className="bg-black/20 rounded-2xl border border-white/5 overflow-hidden mb-6">
                    <div className="max-h-48 overflow-y-auto custom-scrollbar">
                      {scanning ? (
                        <div className="p-10 text-center">
                          <Loader2 className="w-6 h-6 animate-spin text-emerald-500 mx-auto mb-3" />
                          <p className="text-slate-500 text-xs font-bold tracking-widest uppercase">Scanning Frequencies...</p>
                        </div>
                      ) : (networks || []).length > 0 ? (
                        networks?.map((net) => (
                          <div
                            key={net.ssid}
                            onClick={() => setSelectedSSID(net.ssid)}
                            className={`px-5 py-4 flex justify-between items-center cursor-pointer transition-all border-b border-white/5 last:border-none ${
                              selectedSSID === net.ssid ? "bg-emerald-500/10" : "hover:bg-white/5"
                            }`}
                          >
                            <div>
                              <span className="text-sm font-bold text-white block">{net.ssid}</span>
                              <span className="text-[10px] text-slate-500 uppercase font-bold">{net.secure ? 'Encrypted' : 'Open'}</span>
                            </div>
                            <div className="flex flex-col items-end">
                              <div className="h-1 w-8 bg-white/10 rounded-full overflow-hidden">
                                <div className={`h-full bg-emerald-500 ${net.rssi > -60 ? 'w-full' : net.rssi > -70 ? 'w-2/3' : 'w-1/3'}`} />
                              </div>
                              <span className="text-[10px] text-emerald-500 mt-1 font-bold">LINK_ACT</span>
                            </div>
                          </div>
                        ))
                      ) : (
                        <div className="p-10 text-center text-slate-500 font-bold text-xs uppercase tracking-widest">No Transmissions Found</div>
                      )}
                    </div>
                  </div>

                  <div className="space-y-4 mb-8">
                    <Button variant="ghost" size="sm" className="text-emerald-400 font-bold text-xs uppercase p-0 h-auto hover:bg-transparent" onClick={() => refetchNetworks()}>
                      <RefreshCw className="w-3 h-3 mr-2" /> Rescan Range
                    </Button>
                    
                    <div className="relative group">
                      <div className="absolute left-4 top-1/2 -translate-y-1/2 text-slate-500 group-focus-within:text-emerald-500 transition-colors">
                        <Shield className="w-4 h-4" />
                      </div>
                      <Input
                        type="password"
                        placeholder="Network Passphrase"
                        value={password}
                        onChange={(e) => setPassword(e.target.value)}
                        className="bg-white/5 border-white/10 text-white h-14 pl-12 rounded-2xl focus:ring-emerald-500/50"
                      />
                    </div>
                  </div>

                  <Button
                    className="w-full h-14 bg-emerald-500 hover:bg-emerald-600 text-white font-bold rounded-2xl"
                    disabled={!selectedSSID || connectMutation.isPending}
                    onClick={() => connectMutation.mutate()}
                  >
                    {connectMutation.isPending ? "Authenticating Link..." : "Establish Connection"}
                  </Button>
                </motion.div>
              )}

              {step === 3 && (
                <motion.div key="step3" initial={{ opacity: 0, scale: 0.95 }} animate={{ opacity: 1, scale: 1 }} className="text-center py-6">
                  <div className="w-20 h-20 rounded-full bg-emerald-500/20 border-2 border-emerald-500/30 flex items-center justify-center mx-auto mb-8 relative">
                    <Check className="w-10 h-10 text-emerald-400" />
                    <motion.div initial={{ scale: 0.8 }} animate={{ scale: 1.2, opacity: 0 }} transition={{ repeat: Infinity, duration: 1.5 }} className="absolute inset-0 rounded-full bg-emerald-500" />
                  </div>
                  
                  <h2 className="text-3xl font-bold text-white mb-2">Protocol Active</h2>
                  <p className="text-slate-400 text-sm mb-8 leading-relaxed">System successfully calibrated to <span className="text-emerald-400 font-bold">{modeNames[selectedMode]}</span>. All telemetry channels are operational.</p>
                  
                  {connectedIP && (
                    <div className="inline-flex items-center gap-2 px-4 py-2 rounded-xl bg-white/5 border border-white/10 text-emerald-400 font-mono text-sm mb-10">
                      <Globe className="w-4 h-4" />
                      {connectedIP}
                    </div>
                  )}

                  <div className="grid grid-cols-2 gap-4">
                    <a href="/" className="block">
                      <Button className="w-full h-14 bg-emerald-500 hover:bg-emerald-600 text-white font-bold rounded-2xl">Terminal</Button>
                    </a>
                    <a href="/admin" className="block">
                      <Button variant="outline" className="w-full h-14 border-white/10 bg-white/5 hover:bg-white/10 text-white font-bold rounded-2xl">Management</Button>
                    </a>
                  </div>
                </motion.div>
              )}
            </AnimatePresence>
          </CardContent>
        </Card>
      </motion.div>
    </div>
  );
}
