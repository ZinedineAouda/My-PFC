import { useState } from "react";
import { useQuery, useMutation } from "@tanstack/react-query";
import { apiRequest } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import { Card, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Badge } from "@/components/ui/badge";
import { Wifi, Radio, Globe, ArrowRight, Check, Loader2, RefreshCw } from "lucide-react";

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
        toast({ title: "Connection Failed", description: data.message, variant: "destructive" });
      }
    },
    onError: (err: Error) => {
      toast({ title: "Error", description: err.message, variant: "destructive" });
    },
  });

  const setupMutation = useMutation({
    mutationFn: async () => {
      const res = await apiRequest("POST", "/api/setup", { mode: selectedMode });
      return res.json();
    },
    onSuccess: () => {
      setStep(3);
      toast({ title: "Setup Complete", description: "System is ready." });
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
    { id: 1, icon: <Radio className="w-5 h-5" />, title: "AP Mode", desc: "Master creates its own Wi-Fi network. Best for small areas where all devices are nearby." },
    { id: 2, icon: <Globe className="w-5 h-5" />, title: "STA Mode", desc: "Connect to hospital Wi-Fi. Best for large areas using existing network infrastructure." },
    { id: 3, icon: <Wifi className="w-5 h-5" />, title: "AP + STA Mode", desc: "Both modes combined. Nearby slaves use direct connection, distant ones use hospital Wi-Fi." },
  ];

  const modeNames: Record<number, string> = { 1: "AP Mode (Own Network)", 2: "STA Mode (Hospital Network)", 3: "AP + STA Mode (Hybrid)" };

  return (
    <div className="min-h-screen bg-background flex items-center justify-center p-4">
      <Card className="w-full max-w-lg">
        <CardContent className="pt-6">
          <div className="flex justify-center gap-2 mb-6">
            {[1, 2, 3].map((s) => (
              <div
                key={s}
                className={`w-3 h-3 rounded-full transition-all ${
                  s < step ? "bg-emerald-500" : s === step ? "bg-primary" : "bg-muted"
                }`}
              />
            ))}
          </div>

          <div className="text-center mb-6">
            <div className="flex items-center justify-center w-16 h-16 rounded-2xl bg-primary/10 mx-auto mb-3">
              <Wifi className="w-8 h-8 text-primary" />
            </div>
            <h1 className="text-2xl font-bold">Hospital Alarm System</h1>
            <p className="text-sm text-muted-foreground mt-1">Initial Setup</p>
          </div>

          {step === 1 && (
            <div>
              <h2 className="text-center font-semibold mb-1">Select Network Mode</h2>
              <p className="text-center text-sm text-muted-foreground mb-4">Choose how devices will communicate</p>
              <div className="space-y-3">
                {modeOptions.map((mode) => (
                  <div
                    key={mode.id}
                    data-testid={`mode-option-${mode.id}`}
                    onClick={() => setSelectedMode(mode.id)}
                    className={`p-4 rounded-xl border-2 cursor-pointer transition-all ${
                      selectedMode === mode.id
                        ? "border-primary bg-primary/5"
                        : "border-border hover:border-primary/40"
                    }`}
                  >
                    <div className="flex items-center gap-3 mb-1">
                      {mode.icon}
                      <span className="font-semibold">{mode.title}</span>
                    </div>
                    <p className="text-sm text-muted-foreground ml-8">{mode.desc}</p>
                  </div>
                ))}
              </div>
              <Button
                data-testid="button-continue-setup"
                className="w-full mt-4"
                disabled={selectedMode === 0}
                onClick={goStep2}
              >
                Continue
                <ArrowRight className="w-4 h-4 ml-2" />
              </Button>
            </div>
          )}

          {step === 2 && (
            <div>
              <h2 className="text-center font-semibold mb-1">Connect to Wi-Fi</h2>
              <p className="text-center text-sm text-muted-foreground mb-4">Select the hospital network</p>
              <div className="border rounded-lg max-h-48 overflow-y-auto mb-3">
                {scanning ? (
                  <div className="p-6 text-center text-muted-foreground">
                    <Loader2 className="w-5 h-5 animate-spin mx-auto mb-2" />
                    Scanning...
                  </div>
                ) : networks && networks.length > 0 ? (
                  networks.map((net) => (
                    <div
                      key={net.ssid}
                      data-testid={`wifi-item-${net.ssid}`}
                      onClick={() => setSelectedSSID(net.ssid)}
                      className={`px-4 py-3 flex justify-between items-center cursor-pointer border-b last:border-b-0 transition-colors ${
                        selectedSSID === net.ssid ? "bg-primary/5 border-l-2 border-l-primary" : "hover:bg-muted/50"
                      }`}
                    >
                      <span className="font-medium text-sm">{net.ssid}</span>
                      <span className="text-xs text-muted-foreground">
                        {net.rssi > -50 ? "Strong" : net.rssi > -65 ? "Good" : net.rssi > -75 ? "Fair" : "Weak"}
                      </span>
                    </div>
                  ))
                ) : (
                  <div className="p-6 text-center text-muted-foreground">No networks found</div>
                )}
              </div>
              <Button variant="secondary" size="sm" className="mb-3" onClick={() => refetchNetworks()}>
                <RefreshCw className="w-4 h-4 mr-2" />
                Rescan
              </Button>
              <div className="mb-4">
                <label className="text-sm font-medium mb-1 block">Wi-Fi Password</label>
                <Input
                  data-testid="input-wifi-password"
                  type="password"
                  placeholder="Enter network password"
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                />
              </div>
              <Button
                data-testid="button-connect-wifi"
                className="w-full"
                disabled={!selectedSSID || connectMutation.isPending}
                onClick={() => connectMutation.mutate()}
              >
                {connectMutation.isPending ? "Connecting..." : "Connect"}
              </Button>
            </div>
          )}

          {step === 3 && (
            <div className="text-center py-4">
              <div className="flex items-center justify-center w-16 h-16 rounded-full bg-emerald-100 dark:bg-emerald-900/30 mx-auto mb-4">
                <Check className="w-8 h-8 text-emerald-600" />
              </div>
              <h2 className="text-xl font-bold mb-2">Setup Complete!</h2>
              <p className="text-muted-foreground mb-1">Mode: {modeNames[selectedMode]}</p>
              {connectedIP && (
                <Badge variant="secondary" className="no-default-active-elevate mb-4">Network IP: {connectedIP}</Badge>
              )}
              <div className="flex gap-2 justify-center mt-4">
                <a href="/">
                  <Button data-testid="button-goto-dashboard">Open Dashboard</Button>
                </a>
                <a href="/admin">
                  <Button variant="secondary">Admin Panel</Button>
                </a>
              </div>
            </div>
          )}
        </CardContent>
      </Card>
    </div>
  );
}
