import { useQuery, useQueryClient } from "@tanstack/react-query";
import { apiUrl } from "@/lib/queryClient";
import type { Slave } from "@shared/schema";
import { Card, CardContent } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { Skeleton } from "@/components/ui/skeleton";
import { Bell, BellOff, Activity, Bed, DoorOpen, Clock, AlertTriangle, Heart, Shield, RefreshCw } from "lucide-react";
import { useToast } from "@/hooks/use-toast";
import { useEffect, useRef, useState } from "react";
import { useWebsocket } from "@/hooks/use-websocket";
import { motion, AnimatePresence } from "framer-motion";

function SlaveCard({ slave }: { slave: Slave }) {
  const isAlert = slave.alertActive;

  return (
    <motion.div
      layout
      initial={{ opacity: 0, scale: 0.9 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.9 }}
      transition={{ duration: 0.3 }}
      className="relative"
    >
      <div
        className={`relative overflow-hidden group rounded-2xl border transition-all duration-500 shadow-xl ${
          isAlert
            ? "bg-red-950/40 border-red-500/50 ring-2 ring-red-500/50 shadow-red-900/40"
            : "bg-slate-900/60 border-white/10 hover:border-emerald-500/30 shadow-black/40"
        } backdrop-blur-xl`}
      >
        {isAlert && (
          <div className="absolute inset-0 bg-red-500/10 animate-pulse pointer-events-none" />
        )}

        <div className="p-6 relative z-10">
          <div className="flex items-start justify-between mb-6">
            <div className="flex-1 min-w-0">
              <h3 className="text-xl font-bold text-white tracking-tight truncate leading-tight">
                {slave.patientName || "Unnamed Station"}
              </h3>
              <p className="text-xs font-medium text-slate-400 mt-1 flex items-center gap-1.5">
                <Shield className="w-3 h-3" /> {slave.slaveId}
              </p>
            </div>
            <div
              className={`flex-shrink-0 flex items-center justify-center w-14 h-14 rounded-2xl transition-all duration-500 ${
                isAlert
                  ? "bg-red-500 shadow-lg shadow-red-500/50 scale-110"
                  : slave.registered
                  ? "bg-emerald-500/20 text-emerald-400 border border-emerald-500/20"
                  : "bg-slate-800 text-slate-500 border border-white/5"
              }`}
            >
              {isAlert ? (
                <Bell className="w-7 h-7 text-white animate-bounce" />
              ) : (
                <Heart className={`w-6 h-6 ${slave.registered ? "fill-emerald-400" : ""}`} />
              )}
            </div>
          </div>

          <div className="space-y-4">
            <div className="flex items-center justify-between p-3 rounded-xl bg-white/5 border border-white/5">
              <div className="flex items-center gap-3">
                <Bed className="w-4 h-4 text-emerald-400" />
                <span className="text-sm text-slate-300">Bed</span>
              </div>
              <span className="text-sm font-bold text-white">{slave.bed || "-"}</span>
            </div>
            
            <div className="flex items-center justify-between p-3 rounded-xl bg-white/5 border border-white/5">
              <div className="flex items-center gap-3">
                <DoorOpen className="w-4 h-4 text-emerald-400" />
                <span className="text-sm text-slate-300">Room</span>
              </div>
              <span className="text-sm font-bold text-white">{slave.room || "-"}</span>
            </div>

            <div className="pt-2 flex items-center justify-between text-[11px] font-medium uppercase tracking-widest text-slate-500">
              <div className="flex items-center gap-2">
                <div className={`w-2 h-2 rounded-full ${slave.registered ? 'bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.5)]' : 'bg-slate-700'}`} />
                {slave.registered ? 'SYSTEM STABLE' : 'LINK DOWN'}
              </div>
              <div className="flex items-center gap-1">
                <Clock className="w-3 h-3" />
                {slave.lastAlertTime ? new Date(slave.lastAlertTime).toLocaleTimeString([], {hour:'2-digit', minute:'2-digit'}) : "NO EVENTS"}
              </div>
            </div>
          </div>
        </div>
      </div>
    </motion.div>
  );
}

function DashboardSkeleton() {
  return (
    <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
      {Array.from({ length: 4 }).map((_, i) => (
        <Card key={i}>
          <CardContent className="p-5">
            <div className="flex items-start justify-between gap-2 mb-4">
              <div className="flex-1">
                <Skeleton className="h-6 w-32 mb-2" />
                <Skeleton className="h-4 w-20" />
              </div>
              <Skeleton className="w-12 h-12 rounded-full" />
            </div>
            <div className="space-y-2">
              <Skeleton className="h-4 w-24" />
              <Skeleton className="h-4 w-28" />
              <Skeleton className="h-4 w-36" />
            </div>
          </CardContent>
        </Card>
      ))}
    </div>
  );
}

export default function Dashboard() {
  const { toast } = useToast();
  const queryClient = useQueryClient();
  const prevAlertsRef = useRef<Set<string>>(new Set());

  useWebsocket((msg) => {
    console.log("WS Event:", msg.type);
    queryClient.invalidateQueries({ queryKey: ["/api/status"] });
    queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
  });

  const { data: status } = useQuery({
    queryKey: ["/api/status"],
    queryFn: async () => {
      const res = await fetch(apiUrl("/api/status"), { credentials: "include" });
      if (!res.ok) throw new Error("Failed to fetch status");
      return res.json();
    },
  });

  const { data: slaves, isLoading, error } = useQuery<Slave[]>({
    queryKey: ["/api/slaves", "approved"],
    queryFn: async () => {
      const res = await fetch(apiUrl("/api/slaves?approved=1"), { credentials: "include" });
      if (!res.ok) throw new Error("Failed to fetch");
      return res.json();
    },
  });

  useEffect(() => {
    if (!slaves) return;
    const currentAlerts = new Set(
      slaves.filter((s) => s.alertActive).map((s) => s.slaveId)
    );

    currentAlerts.forEach((id) => {
      if (!prevAlertsRef.current.has(id)) {
        const slave = slaves.find((s) => s.slaveId === id);
        if (slave) {
          toast({
            title: "CRITICAL ALERT",
            description: `${slave.patientName} (Bed ${slave.bed}) Emergency request!`,
            variant: "destructive",
          });
        }
      }
    });

    prevAlertsRef.current = currentAlerts;
  }, [slaves, toast]);

  const activeAlerts = slaves?.filter((s) => s.alertActive).length ?? 0;
  const totalDevices = slaves?.length ?? 0;
  const connectedDevices = slaves?.filter((s) => s.registered).length ?? 0;
  const isMasterOnline = status?.isMasterOnline ?? false;

  return (
    <div className="min-h-screen bg-[#070b14] text-slate-100 font-['Outfit']">
      <div className="fixed inset-0 pointer-events-none bg-[radial-gradient(circle_at_50%_0%,rgba(16,185,129,0.08)_0%,transparent_50%)]" />
      
      <header className="sticky top-0 z-50 border-b border-white/5 bg-[#070b14]/80 backdrop-blur-xl">
        <div className="max-w-7xl mx-auto px-6 py-5">
          <div className="flex items-center justify-between gap-6 flex-wrap">
            <div className="flex items-center gap-4">
              <div className="flex items-center justify-center w-12 h-12 rounded-2xl bg-gradient-to-br from-emerald-400 to-emerald-600 shadow-lg shadow-emerald-500/20">
                <Activity className="w-6 h-6 text-white" />
              </div>
              <div>
                <h1 className="text-2xl font-bold tracking-tight bg-gradient-to-r from-white to-slate-400 bg-clip-text text-transparent">
                  Signal Command Center
                </h1>
                <div className="flex items-center gap-2 text-xs font-semibold text-emerald-500/80 tracking-widest uppercase">
                  <div className="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse" />
                  Live Monitoring Active
                </div>
              </div>
            </div>
            
            <div className="flex items-center gap-2 flex-wrap">
              <div className={`flex items-center gap-2 px-4 py-2 rounded-xl border ${isMasterOnline ? 'bg-emerald-500/10 border-emerald-500/20 text-emerald-400' : 'bg-slate-900 border-white/5 text-slate-500'}`}>
                <RefreshCw className={`w-4 h-4 ${isMasterOnline ? 'animate-spin-slow' : ''}`} />
                <span className="text-sm font-bold">Master: {isMasterOnline ? 'ONLINE' : 'OFFLINE'}</span>
              </div>
              
              <div className="flex items-center gap-2 px-4 py-2 rounded-xl bg-white/5 border border-white/5 text-slate-300">
                <Shield className="w-4 h-4" />
                <span className="text-sm font-bold">{connectedDevices}/{totalDevices} Devices</span>
              </div>

              {activeAlerts > 0 && (
                <div className="flex items-center gap-2 px-4 py-2 rounded-xl bg-red-500 text-white shadow-lg shadow-red-500/30 animate-bounce">
                  <AlertTriangle className="w-4 h-4" />
                  <span className="text-sm font-extrabold">{activeAlerts} EMERGENCY</span>
                </div>
              )}
            </div>
          </div>
        </div>
      </header>

      <main className="max-w-7xl mx-auto px-6 py-10 relative">
        <AnimatePresence mode="popLayout">
          {isLoading ? (
            <DashboardSkeleton key="skeleton" />
          ) : error ? (
            <motion.div initial={{ opacity: 0 }} animate={{ opacity: 1 }} key="error">
              <Card className="bg-red-950/20 border-red-500/20 backdrop-blur-xl">
                <CardContent className="p-12 text-center">
                  <AlertTriangle className="w-16 h-16 text-red-500 mx-auto mb-6" />
                  <h2 className="text-2xl font-bold text-white mb-3">System Link Failure</h2>
                  <p className="text-slate-400 max-w-md mx-auto">Lost connection to the primary server. Attempting to re-establish secure handshake...</p>
                </CardContent>
              </Card>
            </motion.div>
          ) : slaves && slaves.length === 0 ? (
            <motion.div initial={{ opacity: 0 }} animate={{ opacity: 1 }} key="empty">
              <Card className="bg-slate-900/40 border-white/5 backdrop-blur-xl">
                <CardContent className="p-16 text-center">
                  <div className="w-24 h-24 rounded-full bg-white/5 flex items-center justify-center mx-auto mb-8">
                    <Bed className="w-12 h-12 text-slate-600" />
                  </div>
                  <h2 className="text-2xl font-bold text-white mb-3">No Active Channels</h2>
                  <p className="text-slate-400 max-w-sm mx-auto">
                    Please register patient stations in the Management Hub to begin monitoring.
                  </p>
                </CardContent>
              </Card>
            </motion.div>
          ) : (
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-6">
              {slaves
                ?.sort((a, b) => {
                  if (a.alertActive && !b.alertActive) return -1;
                  if (!a.alertActive && b.alertActive) return 1;
                  return a.slaveId.localeCompare(b.slaveId);
                })
                .map((slave) => (
                  <SlaveCard key={slave.slaveId} slave={slave} />
                ))}
            </div>
          )}
        </AnimatePresence>
      </main>
    </div>
  );
}
