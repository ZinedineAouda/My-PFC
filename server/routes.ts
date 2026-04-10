import type { Express, Request, Response, NextFunction } from "express";
import type { Server } from "http";
import session from "express-session";
import MemoryStore from "memorystore";
import cors from "cors";
import { storage } from "./storage";
import { initWss, broadcast, broadcastDeviceUpdate, broadcastAllDevices } from "./wss";
import { log } from "./index";
import {
  insertSlaveSchema,
  updateSlaveSchema,
  approveSlaveSchema,
  alertRequestSchema,
  registerRequestSchema,
  setupSchema,
  connectWifiSchema,
  masterSyncSchema,
} from "@shared/schema";

declare module "express-session" {
  interface SessionData {
    isAdmin: boolean;
  }
}

// ─── Middleware: Require admin session ───────────────────────────────
function requireAdmin(req: Request, res: Response, next: NextFunction) {
  if (req.session?.isAdmin) return next();
  return res.status(401).json({ message: "Unauthorized" });
}

// ─── Middleware: Require device API key (ESP32 master) ───────────────
function requireDeviceKey(req: Request, res: Response, next: NextFunction) {
  const key = req.headers["x-device-key"] as string;
  const expected = process.env.DEVICE_API_KEY;
  if (!expected) return next(); // No key configured = open access
  if (!key || key !== expected) {
    return res.status(401).json({ success: false, message: "Invalid device key" });
  }
  return next();
}

// ─── Middleware: Admin OR device key ────────────────────────────────
function requireAdminOrDevice(req: Request, res: Response, next: NextFunction) {
  if (req.session?.isAdmin) return next();
  const key = req.headers["x-device-key"] as string;
  const expected = process.env.DEVICE_API_KEY;
  if (expected && key === expected) return next();
  if (!expected) return next();
  return res.status(401).json({ message: "Unauthorized" });
}

export async function registerRoutes(
  httpServer: Server,
  app: Express
): Promise<Server> {
  log("Registering routes...");
  initWss(httpServer);

  // ── CORS ──────────────────────────────────────────────────
  app.use(
    cors({
      origin: "*",
      methods: ["GET", "POST", "PUT", "DELETE", "OPTIONS"],
      allowedHeaders: ["Content-Type", "Authorization", "x-device-key"],
    })
  );

  // ── Session ───────────────────────────────────────────────
  const SessionStore = MemoryStore(session);
  app.use(
    session({
      secret: process.env.SESSION_SECRET || "hospital-alarm-secret-key",
      resave: false,
      saveUninitialized: false,
      proxy: true,
      store: new SessionStore({ checkPeriod: 86400000 }),
      cookie: {
        secure: process.env.NODE_ENV === "production",
        sameSite: process.env.NODE_ENV === "production" ? "none" : "lax",
        maxAge: 30 * 24 * 60 * 60 * 1000, // 30 days for persistent login
      },
    })
  );

  // ═════════════════════════════════════════════════════════════
  //  HEALTH CHECKS
  // ═════════════════════════════════════════════════════════════
  app.get("/api/health", (_req, res) => res.status(200).json({ status: "ok" }));
  app.get("/health", (_req, res) => res.status(200).send("OK"));

  // ═════════════════════════════════════════════════════════════
  //  AUTHENTICATION
  // ═════════════════════════════════════════════════════════════
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

  // ═════════════════════════════════════════════════════════════
  //  SYSTEM STATUS
  // ═════════════════════════════════════════════════════════════
  app.get("/api/status", (_req: Request, res: Response) => {
    const all = storage.getAllSlaves();
    return res.json({
      mode: storage.getMode(),
      setup: storage.isSetupDone(),
      masterIP: "cloud",
      slaves: all.length,
      online: all.filter((s) => s.online).length,
      alerts: all.filter((s) => s.alertActive).length,
      pending: all.filter((s) => !s.approved).length,
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

  // ═════════════════════════════════════════════════════════════
  //  WIFI SCAN (mock for cloud dashboard)
  // ═════════════════════════════════════════════════════════════
  app.get("/api/scan", (_req: Request, res: Response) => {
    return res.json([
      { ssid: "Hospital-WiFi", rssi: -45, secure: true },
      { ssid: "Hospital-Guest", rssi: -55, secure: true },
      { ssid: "Staff-Network", rssi: -62, secure: true },
    ]);
  });

  app.post("/api/connect-wifi", (req: Request, res: Response) => {
    const parsed = connectWifiSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Missing SSID" });
    }
    return res.json({ success: true, ip: "192.168.1.100" });
  });

  // ═════════════════════════════════════════════════════════════
  //  SLAVE MANAGEMENT
  // ═════════════════════════════════════════════════════════════
  app.get("/api/slaves", (req: Request, res: Response) => {
    if (req.query.all === "1" || req.query.all === "") {
      return res.json(storage.getAllSlaves());
    }
    if (req.query.approved === "1") {
      return res.json(storage.getApprovedSlaves());
    }
    return res.json(storage.getApprovedSlaves());
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
    broadcast({ type: "DELETE", payload: { slaveId: req.params.slaveId as string } });
    return res.json({ success: true, message: "Deleted" });
  });

  // ═════════════════════════════════════════════════════════════
  //  DEVICE REGISTRATION & ALERTS (from ESP32 master)
  // ═════════════════════════════════════════════════════════════
  app.post("/api/register", requireDeviceKey, (req: Request, res: Response) => {
    const parsed = registerRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid body" });
    }
    const slave = storage.registerSlave(parsed.data.slaveId);
    broadcast({ type: "REGISTER", payload: slave });
    return res.json({
      success: true,
      message: slave.approved ? "Registered" : "Pending approval",
    });
  });

  app.post("/api/alert", requireDeviceKey, (req: Request, res: Response) => {
    const parsed = alertRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid body" });
    }
    const result = storage.triggerAlert(parsed.data.slaveId);
    if (!result.success) {
      return res.json({ success: false, reason: result.reason });
    }
    broadcast({ type: "ALERT", payload: { slaveId: parsed.data.slaveId } });
    broadcastDeviceUpdate(parsed.data.slaveId);
    return res.json({ success: true });
  });

  app.post("/api/heartbeat", requireDeviceKey, (req: Request, res: Response) => {
    const parsed = registerRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid body" });
    }
    const slave = storage.registerSlave(parsed.data.slaveId);
    return res.json({
      success: true,
      approved: slave.approved,
      alertActive: slave.alertActive,
    });
  });

  // ═════════════════════════════════════════════════════════════
  //  MASTER SYNC (bidirectional state exchange)
  // ═════════════════════════════════════════════════════════════
  app.post("/api/master-sync", requireDeviceKey, (req: Request, res: Response) => {
    const parsed = masterSyncSchema.safeParse(req.body);
    if (!parsed.success) {
      // Accept even if validation fails — backwards compat
      storage.updateMasterHeartbeat();
      return res.json({
        success: true,
        mode: storage.getMode(),
        slaves: storage.getAllSlaves(),
      });
    }

    // Sync incoming state from master
    storage.syncFromMaster(parsed.data.slaves);

    // Broadcast updates to all WebSocket dashboard clients
    broadcastAllDevices();

    // Return full state back to master (cloud is source of truth for approvals)
    return res.json({
      success: true,
      mode: storage.getMode(),
      slaves: storage.getAllSlaves(),
    });
  });

  app.post("/api/master-ping", requireDeviceKey, (_req: Request, res: Response) => {
    storage.updateMasterHeartbeat();
    broadcast({ type: "MASTER_STATUS", payload: { online: true } });
    return res.json({ success: true });
  });

  // ═════════════════════════════════════════════════════════════
  //  ADMIN ACTIONS (approve, clear alerts)
  // ═════════════════════════════════════════════════════════════
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

  app.post("/api/clearAlert/:slaveId", requireAdminOrDevice, (req: Request, res: Response) => {
    const cleared = storage.clearAlert(req.params.slaveId as string);
    if (!cleared) {
      return res.status(404).json({ message: "Slave not found" });
    }
    broadcastDeviceUpdate(req.params.slaveId as string);
    return res.json({ success: true, message: "Alert cleared" });
  });

  return httpServer;
}
