import { useState } from "react";
import { useQuery, useMutation } from "@tanstack/react-query";
import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import type { Slave, InsertSlave, LoginData } from "@shared/schema";
import { insertSlaveSchema, loginSchema, updateSlaveSchema } from "@shared/schema";
import { apiRequest, queryClient } from "@/lib/queryClient";
import { getQueryFn } from "@/lib/queryClient";
import { useToast } from "@/hooks/use-toast";
import { Card, CardContent, CardHeader } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Badge } from "@/components/ui/badge";
import {
  Dialog,
  DialogContent,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
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
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";
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
  Plus,
  Pencil,
  Trash2,
  BellOff,
  LogIn,
  LogOut,
  Activity,
  AlertTriangle,
  Lock,
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

function AddSlaveDialog({ onAdded }: { onAdded: () => void }) {
  const { toast } = useToast();
  const [open, setOpen] = useState(false);
  const form = useForm<InsertSlave>({
    resolver: zodResolver(insertSlaveSchema),
    defaultValues: { slaveId: "", patientName: "", bed: "", room: "" },
  });

  const addMutation = useMutation({
    mutationFn: async (data: InsertSlave) => {
      const res = await apiRequest("POST", "/api/slaves", data);
      return res.json();
    },
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["/api/slaves"] });
      toast({ title: "Device Added", description: "New patient device has been registered." });
      form.reset();
      setOpen(false);
      onAdded();
    },
    onError: (err: Error) => {
      toast({ title: "Error", description: err.message, variant: "destructive" });
    },
  });

  return (
    <Dialog open={open} onOpenChange={setOpen}>
      <DialogTrigger asChild>
        <Button data-testid="button-add-slave">
          <Plus className="w-4 h-4 mr-2" />
          Add Device
        </Button>
      </DialogTrigger>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>Add New Patient Device</DialogTitle>
        </DialogHeader>
        <Form {...form}>
          <form
            onSubmit={form.handleSubmit((data) => addMutation.mutate(data))}
            className="space-y-4"
          >
            <FormField
              control={form.control}
              name="slaveId"
              render={({ field }) => (
                <FormItem>
                  <FormLabel>Device ID</FormLabel>
                  <FormControl>
                    <Input data-testid="input-slave-id" placeholder="e.g. s1, MAC address" {...field} />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />
            <FormField
              control={form.control}
              name="patientName"
              render={({ field }) => (
                <FormItem>
                  <FormLabel>Patient Name</FormLabel>
                  <FormControl>
                    <Input data-testid="input-patient-name" placeholder="Patient full name" {...field} />
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
                    <Input data-testid="input-bed" placeholder="e.g. 12A" {...field} />
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
                    <Input data-testid="input-room" placeholder="e.g. 301" {...field} />
                  </FormControl>
                  <FormMessage />
                </FormItem>
              )}
            />
            <div className="flex justify-end gap-2 pt-2">
              <DialogClose asChild>
                <Button type="button" variant="secondary">Cancel</Button>
              </DialogClose>
              <Button data-testid="button-submit-add" type="submit" disabled={addMutation.isPending}>
                {addMutation.isPending ? "Adding..." : "Add Device"}
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
  onEdited,
}: {
  slave: Slave;
  onEdited: () => void;
}) {
  const { toast } = useToast();
  const [open, setOpen] = useState(false);
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
      setOpen(false);
      onEdited();
    },
    onError: (err: Error) => {
      toast({ title: "Error", description: err.message, variant: "destructive" });
    },
  });

  return (
    <Dialog open={open} onOpenChange={setOpen}>
      <DialogTrigger asChild>
        <Button size="icon" variant="secondary" data-testid={`button-edit-${slave.slaveId}`}>
          <Pencil className="w-4 h-4" />
        </Button>
      </DialogTrigger>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>Edit Patient - {slave.slaveId}</DialogTitle>
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

  const { data: slaves, isLoading } = useQuery<Slave[]>({
    queryKey: ["/api/slaves"],
    refetchInterval: 3000,
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

  const activeAlerts = slaves?.filter((s) => s.alertActive).length ?? 0;

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
                <p className="text-sm text-muted-foreground">Manage patients & devices</p>
              </div>
            </div>
            <div className="flex items-center gap-2 flex-wrap">
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

      <main className="max-w-7xl mx-auto px-4 sm:px-6 py-6">
        <div className="flex items-center justify-between gap-4 mb-6 flex-wrap">
          <h2 className="text-lg font-semibold">Patient Devices</h2>
          <AddSlaveDialog onAdded={() => {}} />
        </div>

        {isLoading ? (
          <Card>
            <CardContent className="p-8 text-center text-muted-foreground">
              Loading devices...
            </CardContent>
          </Card>
        ) : slaves && slaves.length === 0 ? (
          <Card>
            <CardContent className="p-12 text-center">
              <Shield className="w-16 h-16 text-muted-foreground/40 mx-auto mb-4" />
              <h2 className="text-xl font-semibold mb-2">No Devices Yet</h2>
              <p className="text-muted-foreground mb-4">
                Add your first patient device to begin monitoring.
              </p>
              <AddSlaveDialog onAdded={() => {}} />
            </CardContent>
          </Card>
        ) : (
          <Card>
            <div className="overflow-x-auto">
              <Table>
                <TableHeader>
                  <TableRow>
                    <TableHead>Device ID</TableHead>
                    <TableHead>Patient Name</TableHead>
                    <TableHead>Bed</TableHead>
                    <TableHead>Room</TableHead>
                    <TableHead>Status</TableHead>
                    <TableHead>Last Alert</TableHead>
                    <TableHead className="text-right">Actions</TableHead>
                  </TableRow>
                </TableHeader>
                <TableBody>
                  {slaves?.map((slave) => (
                    <TableRow
                      key={slave.slaveId}
                      data-testid={`row-slave-${slave.slaveId}`}
                      className={slave.alertActive ? "bg-red-50/50 dark:bg-red-950/20" : ""}
                    >
                      <TableCell className="font-mono text-sm" data-testid={`text-table-id-${slave.slaveId}`}>
                        {slave.slaveId}
                      </TableCell>
                      <TableCell className="font-medium" data-testid={`text-table-name-${slave.slaveId}`}>
                        {slave.patientName}
                      </TableCell>
                      <TableCell>{slave.bed}</TableCell>
                      <TableCell>{slave.room}</TableCell>
                      <TableCell>
                        {slave.alertActive ? (
                          <Badge variant="destructive" className="no-default-active-elevate">
                            <AlertTriangle className="w-3 h-3 mr-1" />
                            Alert
                          </Badge>
                        ) : slave.registered ? (
                          <Badge variant="secondary" className="no-default-active-elevate">Connected</Badge>
                        ) : (
                          <Badge variant="secondary" className="no-default-active-elevate text-muted-foreground">Offline</Badge>
                        )}
                      </TableCell>
                      <TableCell className="text-sm text-muted-foreground">
                        {slave.lastAlertTime
                          ? new Date(slave.lastAlertTime).toLocaleString()
                          : "Never"}
                      </TableCell>
                      <TableCell>
                        <div className="flex items-center justify-end gap-1">
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
                          <EditSlaveDialog slave={slave} onEdited={() => {}} />
                          <AlertDialog>
                            <AlertDialogTrigger asChild>
                              <Button
                                size="icon"
                                variant="secondary"
                                data-testid={`button-delete-${slave.slaveId}`}
                              >
                                <Trash2 className="w-4 h-4" />
                              </Button>
                            </AlertDialogTrigger>
                            <AlertDialogContent>
                              <AlertDialogHeader>
                                <AlertDialogTitle>Delete Device</AlertDialogTitle>
                                <AlertDialogDescription>
                                  Are you sure you want to remove device "{slave.slaveId}" ({slave.patientName})? This action cannot be undone.
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
                      </TableCell>
                    </TableRow>
                  ))}
                </TableBody>
              </Table>
            </div>
          </Card>
        )}
      </main>
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
