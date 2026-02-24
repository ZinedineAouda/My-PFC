import { useQuery } from "@tanstack/react-query";
import { apiUrl } from "@/lib/queryClient";
import type { Slave } from "@shared/schema";
import { Card, CardContent } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { Skeleton } from "@/components/ui/skeleton";
import { Bell, BellOff, Activity, Bed, DoorOpen, Clock, AlertTriangle, Heart } from "lucide-react";
import { useToast } from "@/hooks/use-toast";
import { useEffect, useRef } from "react";

function SlaveCard({ slave }: { slave: Slave }) {
  const isAlert = slave.alertActive;

  return (
    <Card
      data-testid={`card-slave-${slave.slaveId}`}
      className={`relative transition-all duration-500 ${
        isAlert
          ? "ring-2 ring-red-500 dark:ring-red-400 bg-red-50 dark:bg-red-950/30"
          : ""
      }`}
    >
      {isAlert && (
        <div className="absolute top-0 left-0 right-0 h-1 bg-red-500 rounded-t-md animate-pulse" />
      )}
      <CardContent className="p-5">
        <div className="flex items-start justify-between gap-2 mb-4">
          <div className="flex-1 min-w-0">
            <h3
              data-testid={`text-patient-name-${slave.slaveId}`}
              className="text-lg font-semibold truncate"
            >
              {slave.patientName}
            </h3>
            <p className="text-sm text-muted-foreground mt-0.5">
              ID: {slave.slaveId}
            </p>
          </div>
          <div
            data-testid={`status-indicator-${slave.slaveId}`}
            className={`flex-shrink-0 flex items-center justify-center w-12 h-12 rounded-full transition-all duration-300 ${
              isAlert
                ? "bg-red-500 dark:bg-red-600 text-white animate-pulse"
                : slave.registered
                  ? "bg-emerald-100 dark:bg-emerald-900/40 text-emerald-600 dark:text-emerald-400"
                  : "bg-muted text-muted-foreground"
            }`}
          >
            {isAlert ? (
              <Bell className="w-6 h-6" />
            ) : slave.registered ? (
              <Heart className="w-5 h-5" />
            ) : (
              <BellOff className="w-5 h-5" />
            )}
          </div>
        </div>

        {isAlert && (
          <Badge
            data-testid={`badge-alert-${slave.slaveId}`}
            variant="destructive"
            className="mb-3 text-xs font-bold uppercase tracking-wider no-default-active-elevate"
          >
            <AlertTriangle className="w-3 h-3 mr-1" />
            ALERT ACTIVE
          </Badge>
        )}

        <div className="space-y-2">
          <div className="flex items-center gap-2 text-sm">
            <Bed className="w-4 h-4 text-muted-foreground flex-shrink-0" />
            <span className="text-muted-foreground">Bed:</span>
            <span data-testid={`text-bed-${slave.slaveId}`} className="font-medium">
              {slave.bed}
            </span>
          </div>
          <div className="flex items-center gap-2 text-sm">
            <DoorOpen className="w-4 h-4 text-muted-foreground flex-shrink-0" />
            <span className="text-muted-foreground">Room:</span>
            <span data-testid={`text-room-${slave.slaveId}`} className="font-medium">
              {slave.room}
            </span>
          </div>
          <div className="flex items-center gap-2 text-sm">
            <Clock className="w-4 h-4 text-muted-foreground flex-shrink-0" />
            <span className="text-muted-foreground">Last Alert:</span>
            <span
              data-testid={`text-last-alert-${slave.slaveId}`}
              className="font-medium"
            >
              {slave.lastAlertTime
                ? new Date(slave.lastAlertTime).toLocaleString()
                : "Never"}
            </span>
          </div>
        </div>

        <div className="mt-4 pt-3 border-t flex items-center gap-2">
          <div
            className={`w-2 h-2 rounded-full ${
              slave.registered
                ? "bg-emerald-500"
                : "bg-muted-foreground"
            }`}
          />
          <span className="text-xs text-muted-foreground">
            {slave.registered ? "Connected" : "Not Connected"}
          </span>
        </div>
      </CardContent>
    </Card>
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
  const prevAlertsRef = useRef<Set<string>>(new Set());

  const { data: slaves, isLoading, error } = useQuery<Slave[]>({
    queryKey: ["/api/slaves", "approved"],
    queryFn: async () => {
      const res = await fetch(apiUrl("/api/slaves?approved=1"), { credentials: "include" });
      if (!res.ok) throw new Error("Failed to fetch");
      return res.json();
    },
    refetchInterval: 2000,
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
            title: "Patient Alert!",
            description: `${slave.patientName} (Bed ${slave.bed}, Room ${slave.room}) needs assistance!`,
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

  return (
    <div className="min-h-screen bg-background">
      <header className="sticky top-0 z-50 border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 py-4">
          <div className="flex items-center justify-between gap-4 flex-wrap">
            <div className="flex items-center gap-3">
              <div className="flex items-center justify-center w-10 h-10 rounded-md bg-primary text-primary-foreground">
                <Activity className="w-5 h-5" />
              </div>
              <div>
                <h1 className="text-xl font-bold tracking-tight">Patient Alarm System</h1>
                <p className="text-sm text-muted-foreground">Real-time monitoring dashboard</p>
              </div>
            </div>
            <div className="flex items-center gap-3 flex-wrap">
              <Badge variant="secondary" className="no-default-active-elevate">
                {totalDevices} Device{totalDevices !== 1 ? "s" : ""}
              </Badge>
              <Badge variant="secondary" className="no-default-active-elevate">
                {connectedDevices} Connected
              </Badge>
              {activeAlerts > 0 && (
                <Badge variant="destructive" data-testid="badge-active-alerts" className="no-default-active-elevate animate-pulse">
                  <AlertTriangle className="w-3 h-3 mr-1" />
                  {activeAlerts} Active Alert{activeAlerts !== 1 ? "s" : ""}
                </Badge>
              )}
            </div>
          </div>
        </div>
      </header>

      <main className="max-w-7xl mx-auto px-4 sm:px-6 py-6">
        {isLoading ? (
          <DashboardSkeleton />
        ) : error ? (
          <Card>
            <CardContent className="p-8 text-center">
              <AlertTriangle className="w-12 h-12 text-destructive mx-auto mb-4" />
              <h2 className="text-lg font-semibold mb-2">Connection Error</h2>
              <p className="text-muted-foreground">Unable to connect to the alarm system. Retrying...</p>
            </CardContent>
          </Card>
        ) : slaves && slaves.length === 0 ? (
          <Card>
            <CardContent className="p-12 text-center">
              <Bed className="w-16 h-16 text-muted-foreground/40 mx-auto mb-4" />
              <h2 className="text-xl font-semibold mb-2">No Devices Registered</h2>
              <p className="text-muted-foreground">
                Add patient devices through the admin panel to get started.
              </p>
            </CardContent>
          </Card>
        ) : (
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
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
      </main>
    </div>
  );
}
