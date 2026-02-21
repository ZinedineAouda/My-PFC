import { useState } from "react";
import { useQuery, useMutation } from "@tanstack/react-query";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import type { Slave, LoginData, ApproveSlave } from "@shared/schema";
import { loginSchema, approveSlaveSchema, updateSlaveSchema } from "@shared/schema";
import { apiRequest, queryClient } from "@/lib/queryClient";
import { getQueryFn } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
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
} from "lucide-react";

function LoginForm({ onLogin }: { onLogin: () => void }) {
  const { toast } = useToast();
  const form = useForm<LoginData>({
    resolver: zodResolver(loginSchema),
    defaultValues: { username: "", password: "" },
  });

  const loginMutation = useMutation({
    mutationFn: async (data: LoginData) => {
      const res = await apiRequest("POST", "/api/admin/login", data);
      return res.json();
    },
    onSuccess: () => {
      onLogin();
      toast({ title: "Logged in", description: "Welcome to admin panel." });
    },
    onError: (err: Error) => {
      toast({
        title: "Login Failed",
        description: err.message.includes("401") ? "Invalid credentials" : err.message,
        variant: "destructive",
      });
    },
  });

  return (
    <div className="min-h-screen bg-background flex items-center justify-center p-4">
      <Card className="w-full max-w-sm">
        <CardContent className="pt-6">
          <div className="flex flex-col items-center mb-6">
            <div className="flex items-center justify-center w-14 h-14 rounded-full bg-primary/10 mb-4">
              <Lock className="w-7 h-7 text-primary" />
            </div>
            <h1 className="text-2xl font-bold">Admin Login</h1>
            <p className="text-sm text-muted-foreground mt-1">Hospital Alarm System</p>
          </div>
          <Form {...form}>
            <form
              onSubmit={form.handleSubmit((data) => loginMutation.mutate(data))}
              className="space-y-4"
            >
              <FormField
                control={form.control}
                name="username"
                render={({ field }) => (
                  <FormItem>
                    <FormLabel>Username</FormLabel>
                    <FormControl>
                      <Input
                        data-testid="input-username"
                        placeholder="Enter username"
                        {...field}
                      />
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
                    <FormLabel>Password</FormLabel>
                    <FormControl>
                      <Input
                        data-testid="input-password"
                        type="password"
                        placeholder="Enter password"
                        {...field}
                      />
                    </FormControl>
                    <FormMessage />
                  </FormItem>
                )}
              />
              <Button
                data-testid="button-login"
                type="submit"
                className="w-full"
                disabled={loginMutation.isPending}
              >
                {loginMutation.isPending ? "Signing in..." : "Sign In"}
                <LogIn className="w-4 h-4 ml-2" />
              </Button>
            </form>
          </Form>
        </CardContent>
      </Card>
    </div>
  );
}

function ApproveDialog({
  slave,
  open,
  onOpenChange,
}: {
  slave: Slave;
  open: boolean;
  onOpenChange: (open: boolean) => void;
}) {
  const { toast } = useToast();
  const form = useForm<ApproveSlave>({
    resolver: zodResolver(approveSlaveSchema),
    defaultValues: { patientName: "", bed: "", room: "" },
  });

  const approveMutation = useMutation({
    mutationFn: async (data: ApproveSlave) => {
      const res = await apiRequest("POST", `/api/approve/${slave.slaveId}`, data);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Device Approved", description: `${slave.slaveId} is now active.` });
      form.reset();
      onOpenChange(false);
    },
    onError: (err: Error) => {
      toast({ title: "Error", description: err.message, variant: "destructive" });
    },
  });

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>Approve Device: {slave.slaveId}</DialogTitle>
        </DialogHeader>
        <Form {...form}>
          <form
            onSubmit={form.handleSubmit((data) => approveMutation.mutate(data))}
            className="space-y-4"
          >
            <FormField
              control={form.control}
              name="patientName"
              render={({ field }) => (
                <FormItem>
                  <FormLabel>Patient Name</FormLabel>
                  <FormControl>
                    <Input data-testid="input-approve-patient-name" placeholder="Patient full name" {...field} />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />
            <FormField
              control={form.control}
              name="bed"
              render={({ field }) => (
                <FormItem>
                  <FormLabel>Bed Number</FormLabel>
                  <FormControl>
                    <Input data-testid="input-approve-bed" placeholder="e.g. 12A" {...field} />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />
            <FormField
              control={form.control}
              name="room"
              render={({ field }) => (
                <FormItem>
                  <FormLabel>Room</FormLabel>
                  <FormControl>
                    <Input data-testid="input-approve-room" placeholder="e.g. 301" {...field} />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />
            <div className="flex justify-end gap-2 pt-2">
              <DialogClose asChild>
                <Button type="button" variant="secondary">Cancel</Button>
              </DialogClose>
              <Button data-testid="button-submit-approve" type="submit" disabled={approveMutation.isPending}>
                {approveMutation.isPending ? "Approving..." : "Approve & Save"}
              </Button>
            </div>
          </form>
        </Form>
      </DialogContent>
    </Dialog>
  );
}

function EditSlaveDialog({
  slave,
  open,
  onOpenChange,
}: {
  slave: Slave;
  open: boolean;
  onOpenChange: (open: boolean) => void;
}) {
  const { toast } = useToast();
  const form = useForm({
    resolver: zodResolver(
      updateSlaveSchema.extend({
        patientName: updateSlaveSchema.shape.patientName.unwrap(),
        bed: updateSlaveSchema.shape.bed.unwrap(),
        room: updateSlaveSchema.shape.room.unwrap(),
      })
    ),
    defaultValues: {
      patientName: slave.patientName,
      bed: slave.bed,
      room: slave.room,
    },
  });

  const editMutation = useMutation({
    mutationFn: async (data: { patientName: string; bed: string; room: string }) => {
      const res = await apiRequest("PUT", `/api/slaves/${slave.slaveId}`, data);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Updated", description: "Patient information updated." });
      onOpenChange(false);
    },
    onError: (err: Error) => {
      toast({ title: "Error", description: err.message, variant: "destructive" });
    },
  });

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>Edit: {slave.slaveId}</DialogTitle>
        </DialogHeader>
        <Form {...form}>
          <form
            onSubmit={form.handleSubmit((data) => editMutation.mutate(data))}
            className="space-y-4"
          >
            <FormField
              control={form.control}
              name="patientName"
              render={({ field }) => (
                <FormItem>
                  <FormLabel>Patient Name</FormLabel>
                  <FormControl>
                    <Input data-testid="input-edit-patient-name" {...field} />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />
            <FormField
              control={form.control}
              name="bed"
              render={({ field }) => (
                <FormItem>
                  <FormLabel>Bed Number</FormLabel>
                  <FormControl>
                    <Input data-testid="input-edit-bed" {...field} />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />
            <FormField
              control={form.control}
              name="room"
              render={({ field }) => (
                <FormItem>
                  <FormLabel>Room</FormLabel>
                  <FormControl>
                    <Input data-testid="input-edit-room" {...field} />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />
            <div className="flex justify-end gap-2 pt-2">
              <DialogClose asChild>
                <Button type="button" variant="secondary">Cancel</Button>
              </DialogClose>
              <Button data-testid="button-submit-edit" type="submit" disabled={editMutation.isPending}>
                {editMutation.isPending ? "Saving..." : "Save Changes"}
              </Button>
            </div>
          </form>
        </Form>
      </DialogContent>
    </Dialog>
  );
}

function AdminPanel() {
  const { toast } = useToast();
  const [approveSlaveData, setApproveSlaveData] = useState<Slave | null>(null);
  const [editSlaveData, setEditSlaveData] = useState<Slave | null>(null);

  const { data: slaves, isLoading } = useQuery<Slave[]>({
    queryKey: ["/api/slaves", "all"],
    queryFn: async () => {
      const res = await fetch("/api/slaves?all=1");
      if (!res.ok) throw new Error("Failed to fetch");
      return res.json();
    },
    refetchInterval: 3000,
  });

  const { data: status } = useQuery<{ mode: number; setup: boolean }>({
    queryKey: ["/api/status"],
  });

  const clearAlertMutation = useMutation({
    mutationFn: async (slaveId: string) => {
      const res = await apiRequest("POST", `/api/clearAlert/${slaveId}`);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Alert Cleared", description: "The alert has been dismissed." });
    },
    onError: (err: Error) => {
      toast({ title: "Error", description: err.message, variant: "destructive" });
    },
  });

  const deleteMutation = useMutation({
    mutationFn: async (slaveId: string) => {
      const res = await apiRequest("DELETE", `/api/slaves/${slaveId}`);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Deleted", description: "Device has been removed." });
    },
    onError: (err: Error) => {
      toast({ title: "Error", description: err.message, variant: "destructive" });
    },
  });

  const logoutMutation = useMutation({
    mutationFn: async () => {
      const res = await apiRequest("POST", "/api/admin/logout");
      return res.json();
    },
    onSuccess: () => {
      window.location.reload();
    },
  });

  const pendingDevices = slaves?.filter((s) => !s.approved) ?? [];
  const approvedDevices = slaves?.filter((s) => s.approved) ?? [];
  const activeAlerts = approvedDevices.filter((s) => s.alertActive).length;
  const modeNames: Record<number, string> = { 1: "AP Mode", 2: "STA Mode", 3: "AP+STA Mode" };

  return (
    <div className="min-h-screen bg-background">
      <header className="sticky top-0 z-50 border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 py-4">
          <div className="flex items-center justify-between gap-4 flex-wrap">
            <div className="flex items-center gap-3">
              <div className="flex items-center justify-center w-10 h-10 rounded-md bg-primary text-primary-foreground">
                <Shield className="w-5 h-5" />
              </div>
              <div>
                <h1 className="text-xl font-bold tracking-tight">Admin Panel</h1>
                <p className="text-sm text-muted-foreground">Device Management</p>
              </div>
            </div>
            <div className="flex items-center gap-2 flex-wrap">
              {status?.mode ? (
                <Badge variant="secondary" className="no-default-active-elevate">
                  <Settings className="w-3 h-3 mr-1" />
                  {modeNames[status.mode] || "Unknown"}
                </Badge>
              ) : null}
              {activeAlerts > 0 && (
                <Badge variant="destructive" className="no-default-active-elevate animate-pulse">
                  <AlertTriangle className="w-3 h-3 mr-1" />
                  {activeAlerts} Alert{activeAlerts !== 1 ? "s" : ""}
                </Badge>
              )}
              <a href="/">
                <Button variant="secondary" size="sm">
                  <Activity className="w-4 h-4 mr-2" />
                  Dashboard
                </Button>
              </a>
              <a href="/setup">
                <Button variant="secondary" size="sm">
                  <Settings className="w-4 h-4 mr-2" />
                  Setup
                </Button>
              </a>
              <Button
                data-testid="button-logout"
                variant="secondary"
                size="sm"
                onClick={() => logoutMutation.mutate()}
              >
                <LogOut className="w-4 h-4 mr-2" />
                Logout
              </Button>
            </div>
          </div>
        </div>
      </header>

      <main className="max-w-7xl mx-auto px-4 sm:px-6 py-6 space-y-8">
        <section>
          <div className="flex items-center gap-3 mb-4">
            <Clock className="w-5 h-5 text-amber-500" />
            <h2 className="text-lg font-semibold">Pending Approval</h2>
            <Badge variant="secondary" className="no-default-active-elevate">{pendingDevices.length}</Badge>
          </div>
          {isLoading ? (
            <Card><CardContent className="p-8 text-center text-muted-foreground">Loading...</CardContent></Card>
          ) : pendingDevices.length === 0 ? (
            <Card>
              <CardContent className="p-6 text-center text-muted-foreground">
                No pending devices. Waiting for slave devices to connect and register...
              </CardContent>
            </Card>
          ) : (
            <div className="space-y-2">
              {pendingDevices.map((slave) => (
                <Card key={slave.slaveId} className="border-l-4 border-l-amber-400">
                  <CardContent className="p-4 flex items-center justify-between gap-4 flex-wrap">
                    <div>
                      <p className="font-mono text-sm font-bold" data-testid={`text-pending-id-${slave.slaveId}`}>{slave.slaveId}</p>
                      <p className="text-sm text-muted-foreground">Detected - Awaiting approval</p>
                    </div>
                    <div className="flex items-center gap-2">
                      <Badge variant="secondary" className="no-default-active-elevate bg-amber-50 text-amber-700 dark:bg-amber-950/30 dark:text-amber-400">Pending</Badge>
                      <Button
                        size="sm"
                        data-testid={`button-approve-${slave.slaveId}`}
                        onClick={() => setApproveSlaveData(slave)}
                      >
                        <CheckCircle className="w-4 h-4 mr-1" />
                        Approve
                      </Button>
                      <AlertDialog>
                        <AlertDialogTrigger asChild>
                          <Button size="sm" variant="secondary" data-testid={`button-reject-${slave.slaveId}`}>
                            <Trash2 className="w-4 h-4 mr-1" />
                            Reject
                          </Button>
                        </AlertDialogTrigger>
                        <AlertDialogContent>
                          <AlertDialogHeader>
                            <AlertDialogTitle>Reject Device</AlertDialogTitle>
                            <AlertDialogDescription>
                              Remove device "{slave.slaveId}"? It can register again later.
                            </AlertDialogDescription>
                          </AlertDialogHeader>
                          <AlertDialogFooter>
                            <AlertDialogCancel>Cancel</AlertDialogCancel>
                            <AlertDialogAction onClick={() => deleteMutation.mutate(slave.slaveId)}>
                              Reject
                            </AlertDialogAction>
                          </AlertDialogFooter>
                        </AlertDialogContent>
                      </AlertDialog>
                    </div>
                  </CardContent>
                </Card>
              ))}
            </div>
          )}
        </section>

        <section>
          <div className="flex items-center gap-3 mb-4">
            <CheckCircle className="w-5 h-5 text-emerald-500" />
            <h2 className="text-lg font-semibold">Approved Devices</h2>
            <Badge variant="secondary" className="no-default-active-elevate">{approvedDevices.length}</Badge>
          </div>
          {approvedDevices.length === 0 ? (
            <Card>
              <CardContent className="p-6 text-center text-muted-foreground">
                No approved devices yet. Approve pending devices above.
              </CardContent>
            </Card>
          ) : (
            <div className="space-y-2">
              {approvedDevices.map((slave) => (
                <Card
                  key={slave.slaveId}
                  className={`border-l-4 ${slave.alertActive ? "border-l-red-500 bg-red-50/50 dark:bg-red-950/20" : "border-l-emerald-500"}`}
                  data-testid={`card-approved-${slave.slaveId}`}
                >
                  <CardContent className="p-4 flex items-center justify-between gap-4 flex-wrap">
                    <div>
                      <p className="font-mono text-sm font-bold">{slave.slaveId}</p>
                      <p className="text-sm font-medium" data-testid={`text-approved-name-${slave.slaveId}`}>
                        {slave.patientName || "No name"} - Bed {slave.bed || "-"}, Room {slave.room || "-"}
                      </p>
                    </div>
                    <div className="flex items-center gap-2 flex-wrap">
                      {slave.alertActive ? (
                        <Badge variant="destructive" className="no-default-active-elevate">
                          <AlertTriangle className="w-3 h-3 mr-1" />
                          Alert
                        </Badge>
                      ) : slave.registered ? (
                        <Badge variant="secondary" className="no-default-active-elevate">Online</Badge>
                      ) : (
                        <Badge variant="secondary" className="no-default-active-elevate text-muted-foreground">Offline</Badge>
                      )}
                      {slave.alertActive && (
                        <Button
                          size="sm"
                          variant="destructive"
                          data-testid={`button-clear-alert-${slave.slaveId}`}
                          onClick={() => clearAlertMutation.mutate(slave.slaveId)}
                          disabled={clearAlertMutation.isPending}
                        >
                          <BellOff className="w-4 h-4 mr-1" />
                          Clear
                        </Button>
                      )}
                      <Button
                        size="sm"
                        variant="secondary"
                        data-testid={`button-edit-${slave.slaveId}`}
                        onClick={() => setEditSlaveData(slave)}
                      >
                        <Pencil className="w-4 h-4 mr-1" />
                        Edit
                      </Button>
                      <AlertDialog>
                        <AlertDialogTrigger asChild>
                          <Button size="sm" variant="secondary" data-testid={`button-delete-${slave.slaveId}`}>
                            <Trash2 className="w-4 h-4" />
                          </Button>
                        </AlertDialogTrigger>
                        <AlertDialogContent>
                          <AlertDialogHeader>
                            <AlertDialogTitle>Delete Device</AlertDialogTitle>
                            <AlertDialogDescription>
                              Remove device "{slave.slaveId}" ({slave.patientName})? This cannot be undone.
                            </AlertDialogDescription>
                          </AlertDialogHeader>
                          <AlertDialogFooter>
                            <AlertDialogCancel>Cancel</AlertDialogCancel>
                            <AlertDialogAction
                              data-testid={`button-confirm-delete-${slave.slaveId}`}
                              onClick={() => deleteMutation.mutate(slave.slaveId)}
                            >
                              Delete
                            </AlertDialogAction>
                          </AlertDialogFooter>
                        </AlertDialogContent>
                      </AlertDialog>
                    </div>
                  </CardContent>
                </Card>
              ))}
            </div>
          )}
        </section>
      </main>

      {approveSlaveData && (
        <ApproveDialog
          slave={approveSlaveData}
          open={!!approveSlaveData}
          onOpenChange={(open) => { if (!open) setApproveSlaveData(null); }}
        />
      )}
      {editSlaveData && (
        <EditSlaveDialog
          slave={editSlaveData}
          open={!!editSlaveData}
          onOpenChange={(open) => { if (!open) setEditSlaveData(null); }}
        />
      )}
    </div>
  );
}

export default function AdminPage() {
  const { data: session, isLoading } = useQuery<{ authenticated: boolean } | null>({
    queryKey: ["/api/admin/session"],
    queryFn: getQueryFn({ on401: "returnNull" }),
  });

  if (isLoading) {
    return (
      <div className="min-h-screen bg-background flex items-center justify-center">
        <div className="text-muted-foreground">Loading...</div>
      </div>
    );
  }

  if (!session?.authenticated) {
    return (
      <LoginForm
        onLogin={() => queryClient.invalidateQueries({ queryKey: ["/api/admin/session"] })}
      />
    );
  }

  return <AdminPanel />;
}
