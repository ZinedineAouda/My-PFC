import type { Express, Request, Response, NextFunction } from "express";
import { createServer, type Server } from "http";
import session from "express-session";
import cors from "cors";
import { storage } from "./storage";
import { initWss, broadcast } from "./wss";
import { insertSlaveSchema, updateSlaveSchema, approveSlaveSchema, alertRequestSchema, registerRequestSchema, setupSchema, connectWifiSchema } from "@shared/schema";

declare module "express-session" {
  interface SessionData {
    isAdmin: boolean;
  }
}

function requireAdmin(req: Request, res: Response, next: NextFunction) {
  if (req.session?.isAdmin) {
    return next();
  }
  return res.status(401).json({ message: "Unauthorized" });
}

function requireDeviceKey(req: Request, res: Response, next: NextFunction) {
  const key = req.headers["x-device-key"] as string;
  const expected = process.env.DEVICE_API_KEY;
  if (!expected) {
    return next();
  }
  if (!key || key !== expected) {
    return res.status(401).json({ success: false, message: "Invalid device key" });
  }
  return next();
}

function requireAdminOrDevice(req: Request, res: Response, next: NextFunction) {
  if (req.session?.isAdmin) {
    return next();
  }
  const key = req.headers["x-device-key"] as string;
  const expected = process.env.DEVICE_API_KEY;
  if (expected && key === expected) {
    return next();
  }
  if (!expected) {
    return next();
  }
  return res.status(401).json({ message: "Unauthorized" });
}

export async function registerRoutes(
  httpServer: Server,
  app: Express
): Promise<Server> {
  log("Starting route registration...");
  initWss(httpServer);

  // Allow the ESP32 to hit the API routes without CORS blocking it.
  app.use(cors({
    origin: "*",
    methods: ["GET", "POST", "PUT", "DELETE", "OPTIONS"],
    allowedHeaders: ["Content-Type", "Authorization", "x-device-key"]
  }));

  app.use(
    session({
      secret: process.env.SESSION_SECRET || "hospital-alarm-secret-key",
      resave: false,
      saveUninitialized: false,
      proxy: true,
      cookie: {
        secure: false, // Railway handles HTTPS, cookie works fine without secure flag if on same origin
        sameSite: "lax",
        maxAge: 24 * 60 * 60 * 1000,
      },
    })
  );

  app.get("/api/health", (_req, res) => res.status(200).json({ status: "ok" }));
  app.get("/health", (_req, res) => res.status(200).send("OK"));

  app.post("/api/admin/login", (req: Request, res: Response) => {
    const { username, password } = req.body;
    if (username === "admin" && password === "admin1234") {
      req.session.isAdmin = true;
      req.session.save((err) => {
        if (err) console.error("Session save error:", err);
        res.json({ success: true, message: "Logged in" });
      });
      return;
    }
    return res.status(401).json({ message: "Invalid credentials" });
  });

  app.post("/api/admin/logout", (req: Request, res: Response) => {
    req.session.destroy(() => {
      res.json({ success: true, message: "Logged out" });
    });
  });

  app.get("/api/admin/session", (req: Request, res: Response) => {
    if (req.session?.isAdmin) {
      return res.json({ authenticated: true });
    }
    return res.status(401).json({ authenticated: false });
  });

  app.get("/api/status", (_req: Request, res: Response) => {
    return res.json({
      mode: storage.getMode(),
      setup: storage.isSetupDone(),
      masterIP: "localhost",
      slaves: storage.getAllSlaves().length,
      isMasterOnline: storage.isMasterOnline(),
    });
  });

  app.post("/api/setup", (req: Request, res: Response) => {
    const parsed = setupSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid mode" });
    }
    storage.setMode(parsed.data.mode);
    return res.json({ success: true, mode: parsed.data.mode });
  });

  app.get("/api/scan", (_req: Request, res: Response) => {
    const fakeNetworks = [
      { ssid: "Hospital-WiFi", rssi: -45, secure: true },
      { ssid: "Hospital-Guest", rssi: -55, secure: true },
      { ssid: "Staff-Network", rssi: -62, secure: true },
      { ssid: "IoT-Network", rssi: -70, secure: false },
    ];
    return res.json(fakeNetworks);
  });

  app.post("/api/connect-wifi", (req: Request, res: Response) => {
    const parsed = connectWifiSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Missing SSID" });
    }
    return res.json({ success: true, ip: "192.168.1.100" });
  });

  app.get("/api/slaves", (req: Request, res: Response) => {
    if (req.query.approved === "1") {
      return res.json(storage.getApprovedSlaves());
    }
    if (req.query.all === "1") {
      return res.json(storage.getAllSlaves());
    }
    return res.json(storage.getApprovedSlaves());
  });

  // Master Sync Endpoint: The ESP32 calls this to sync its local state with the server
  app.post("/api/master-sync", requireDeviceKey, (req: Request, res: Response) => {
    storage.updateMasterHeartbeat();
    const { slaves } = req.body; // Array of { slaveId, alertActive, ... }
    
    // 1. Process incoming data from Master (e.g. alerts triggered on Master AP mode)
    if (Array.isArray(slaves)) {
      slaves.forEach((s: any) => {
        const local = storage.getSlave(s.slaveId);
        if (local && local.approved) {
          if (s.alertActive && !local.alertActive) {
            storage.triggerAlert(s.slaveId);
            broadcast({ type: "UPDATE", payload: storage.getSlave(s.slaveId) });
          } else if (!s.alertActive && local.alertActive) {
            storage.clearAlert(s.slaveId);
            broadcast({ type: "UPDATE", payload: storage.getSlave(s.slaveId) });
          }
        } else if (!local) {
          // Auto-register new slaves discovered by Master
          storage.registerSlave(s.slaveId);
          broadcast({ type: "REGISTER", payload: storage.getSlave(s.slaveId) });
        }
      });
    }

    // 2. Return the Full Source of Truth to the Master
    // ESP32 will use this to update its local 'approved' and 'alertActive' flags
    return res.json({
      success: true,
      mode: storage.getMode(),
      slaves: storage.getAllSlaves()
    });
  });

  app.post("/api/register", requireDeviceKey, (req: Request, res: Response) => {
    const parsed = registerRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid request body" });
    }
    const { slaveId } = parsed.data;
    const slave = storage.registerSlave(slaveId);
    broadcast({ type: "REGISTER", payload: slave });
    if (slave.approved) {
      return res.json({ success: true, message: "Registered" });
    }
    return res.json({ success: true, message: "Pending approval" });
  });

  app.post("/api/alert", requireDeviceKey, (req: Request, res: Response) => {
    const parsed = alertRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid request body" });
    }
    const { slaveId } = parsed.data;
    const result = storage.triggerAlert(slaveId);
    if (!result.success) {
      return res.json({ success: false, reason: result.reason });
    }
    broadcast({ type: "ALERT", payload: { slaveId } });
    return res.json({ success: true });
  });

  app.post("/api/heartbeat", requireDeviceKey, (req: Request, res: Response) => {
    const parsed = registerRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid request body" });
    }
    const { slaveId } = parsed.data;
    const slave = storage.registerSlave(slaveId);
    return res.json({ success: true, approved: slave.approved, alertActive: slave.alertActive });
  });

  app.post("/api/master-ping", requireDeviceKey, (_req: Request, res: Response) => {
    storage.updateMasterHeartbeat();
    broadcast({ type: "MASTER_STATUS", payload: { online: true } });
    return res.json({ success: true });
  });

  app.post("/api/approve/:slaveId", requireAdmin, (req: Request, res: Response) => {
    const parsed = approveSlaveSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid data", errors: parsed.error.flatten() });
    }
    const slave = storage.approveSlave(req.params.slaveId as string, parsed.data);
    if (!slave) {
      return res.status(404).json({ message: "Slave not found" });
    }
    broadcast({ type: "UPDATE", payload: slave });
    return res.json({ success: true, slave });
  });

  app.post("/api/slaves", requireAdmin, (req: Request, res: Response) => {
    const parsed = insertSlaveSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid data", errors: parsed.error.flatten() });
    }
    try {
      const slave = storage.addSlave(parsed.data);
      broadcast({ type: "REGISTER", payload: slave });
      return res.json({ success: true, slave });
    } catch (err: any) {
      return res.status(409).json({ message: err.message });
    }
  });

  app.put("/api/slaves/:slaveId", requireAdmin, (req: Request, res: Response) => {
    const parsed = updateSlaveSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid data", errors: parsed.error.flatten() });
    }
    const slave = storage.updateSlave(req.params.slaveId as string, parsed.data);
    if (!slave) {
      return res.status(404).json({ message: "Slave not found" });
    }
    broadcast({ type: "UPDATE", payload: slave });
    return res.json({ success: true, slave });
  });

  app.delete("/api/slaves/:slaveId", requireAdmin, (req: Request, res: Response) => {
    const deleted = storage.deleteSlave(req.params.slaveId as string);
    if (!deleted) {
      return res.status(404).json({ message: "Slave not found" });
    }
    broadcast({ type: "DELETE", payload: { slaveId: req.params.slaveId } });
    return res.json({ success: true, message: "Deleted" });
  });

  app.post("/api/clearAlert/:slaveId", requireAdminOrDevice, (req: Request, res: Response) => {
    const cleared = storage.clearAlert(req.params.slaveId as string);
    if (!cleared) {
      return res.status(404).json({ message: "Slave not found" });
    }
    broadcast({ type: "UPDATE", payload: { slaveId: req.params.slaveId, alertActive: false } });
    return res.json({ success: true, message: "Alert cleared" });
  });

  return httpServer;
}
