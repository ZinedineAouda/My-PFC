import type { Express, Request, Response, NextFunction } from "express";
import { createServer, type Server } from "http";
import session from "express-session";
import { storage } from "./storage";
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
  // No CORS needed — frontend and backend are served from the same Railway URL (same origin).

  const isProduction = process.env.NODE_ENV === "production";

  app.use(
    session({
      secret: process.env.SESSION_SECRET || "hospital-alarm-secret-key",
      resave: false,
      saveUninitialized: false,
      cookie: {
        secure: isProduction,
        sameSite: isProduction ? "none" : "lax",
        maxAge: 24 * 60 * 60 * 1000,
      },
    })
  );

  app.post("/api/admin/login", (req: Request, res: Response) => {
    const { username, password } = req.body;
    if (username === "admin" && password === "admin1234") {
      req.session.isAdmin = true;
      return res.json({ success: true, message: "Logged in" });
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

  app.post("/api/register", requireDeviceKey, (req: Request, res: Response) => {
    const parsed = registerRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid request body" });
    }
    const { slaveId } = parsed.data;
    const slave = storage.registerSlave(slaveId);
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

  app.post("/api/approve/:slaveId", requireAdmin, (req: Request, res: Response) => {
    const parsed = approveSlaveSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid data", errors: parsed.error.flatten() });
    }
    const slave = storage.approveSlave(req.params.slaveId as string, parsed.data);
    if (!slave) {
      return res.status(404).json({ message: "Slave not found" });
    }
    return res.json({ success: true, slave });
  });

  app.post("/api/slaves", requireAdmin, (req: Request, res: Response) => {
    const parsed = insertSlaveSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid data", errors: parsed.error.flatten() });
    }
    try {
      const slave = storage.addSlave(parsed.data);
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
    return res.json({ success: true, slave });
  });

  app.delete("/api/slaves/:slaveId", requireAdmin, (req: Request, res: Response) => {
    const deleted = storage.deleteSlave(req.params.slaveId as string);
    if (!deleted) {
      return res.status(404).json({ message: "Slave not found" });
    }
    return res.json({ success: true, message: "Deleted" });
  });

  app.post("/api/clearAlert/:slaveId", requireAdminOrDevice, (req: Request, res: Response) => {
    const cleared = storage.clearAlert(req.params.slaveId as string);
    if (!cleared) {
      return res.status(404).json({ message: "Slave not found" });
    }
    return res.json({ success: true, message: "Alert cleared" });
  });

  return httpServer;
}
