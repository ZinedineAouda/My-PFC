import type { Express, Request, Response, NextFunction } from "express";
import type { Server } from "http";
import session from "express-session";
import MemoryStore from "memorystore";
import cors from "cors";
import { storage } from "./storage";
import { initWss, broadcast, broadcastDeviceUpdate, broadcastAllDevices } from "./wss";
import { log } from "./index";
import {
  insertDeviceSchema,
  updateDeviceSchema,
  approveDeviceSchema,
  alertRequestSchema,
  registerRequestSchema,
  setupSchema,
  connectWifiSchema,
  controllerSyncSchema,
} from "@shared/schema";

// ─── Utility: Async Error Wrapper ─────────────────────────────────────
const asyncHandler = (fn: any) => (req: Request, res: Response, next: NextFunction) => {
  return Promise.resolve(fn(req, res, next)).catch(next);
};

declare module "express-session" {
  interface SessionData {
    isAdmin: boolean;
  }
}

// ─── Middleware: Require admin session or token ───────────────────────
function requireAdmin(req: Request, res: Response, next: NextFunction) {
  if (req.session?.isAdmin) return next();
  
  // Allow token-based auth for mobile/local testing
  const token = req.headers["x-admin-token"];
  if (token === "admin1234") return next();
  
  return res.status(401).json({ message: "Unauthorized" });
}

// ─── Middleware: Require device API key (ESP32 controller) ───────────────
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
        secure: false, // Set to false to allow testing on local/non-https while we debug
        sameSite: "lax",
        maxAge: 30 * 24 * 60 * 60 * 1000, 
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
    try {
      const usernameInput = (req.body.username || "").toString().trim().toLowerCase();
      const passwordInput = (req.body.password || "").toString().trim();

      console.log(`[AUTH] Incoming login request for user: "${usernameInput}"`);

      if (usernameInput === "admin" && passwordInput === "admin1234") {
        console.log("[AUTH] Credentials verified. Initializing session...");
        
        req.session.isAdmin = true;
        req.session.save((err) => {
          if (err) {
            console.error("[AUTH] FATAL: Session persistence failure:", err);
            return res.status(500).json({ success: false, message: "Internal Session Error" });
          }
          console.log("[AUTH] Success: Session committed. Returning 200 OK.");
          return res.json({ success: true });
        });
      } else {
        console.warn(`[AUTH] Denied: Invalid password for user "${usernameInput}"`);
        return res.status(401).json({ success: false, message: "Invalid Credentials" });
      }
    } catch (crash) {
      console.error("[AUTH] Crash during login handler:", crash);
      return res.status(500).json({ success: false, message: "Critical Auth Error" });
    }
  });

  app.post("/api/admin/logout", (req: Request, res: Response) => {
    req.session.destroy(() => {
      res.json({ success: true, message: "Logged out" });
    });
  });

  app.get("/api/admin/session", (req: Request, res: Response) => {
    const isAdmin = !!req.session?.isAdmin;
    console.log(`[AUTH] Session verification check: ${isAdmin ? "LOGGED IN" : "ANONYMOUS"}`);
    if (isAdmin) {
      return res.json({ authenticated: true });
    }
    return res.status(401).json({ authenticated: false });
  });

  // ═════════════════════════════════════════════════════════════
  //  SYSTEM STATUS
  // ═════════════════════════════════════════════════════════════
  app.get("/api/status", asyncHandler(async (_req: Request, res: Response) => {
    const all = await storage.getAllDevices();
    const settings = await storage.getSettings();
    return res.json({
      mode: settings?.wifiMode ?? await storage.getMode(),
      setup: await storage.isSetupDone(),
      controllerIP: "cloud",
      devices: all.length,
      online: all.filter((s) => s.online).length,
      alerts: all.filter((s) => s.alertActive).length,
      pending: all.filter((s) => !s.approved).length,
      isControllerOnline: await storage.isControllerOnline(),
      uptime: settings?.controllerUptime ?? 0,
      rssi: settings?.controllerRSSI ?? 0,
      wifiError: settings?.controllerWifiError ?? null,
    });
  }));

  app.post("/api/setup", asyncHandler(async (req: Request, res: Response) => {
    const parsed = setupSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid mode" });
    }
    await storage.setMode(parsed.data.mode);
    return res.json({ success: true, mode: parsed.data.mode });
  }));


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
  //  DEVICE MANAGEMENT
  // ═════════════════════════════════════════════════════════════
  app.get("/api/devices", asyncHandler(async (req: Request, res: Response) => {
    if (req.query.all === "1" || req.query.all === "") {
      return res.json(await storage.getAllDevices());
    }
    return res.json(await storage.getApprovedDevices());
  }));

  app.post("/api/devices", requireAdmin, asyncHandler(async (req: Request, res: Response) => {
    const parsed = insertDeviceSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid data", errors: parsed.error.flatten() });
    }
    try {
      const device = await storage.addDevice(parsed.data);
      broadcast({ type: "REGISTER", payload: device });
      return res.json({ success: true, device });
    } catch (err: any) {
      return res.status(409).json({ message: err.message });
    }
  }));

  app.put("/api/devices/:deviceId", requireAdmin, asyncHandler(async (req: Request, res: Response) => {
    const parsed = updateDeviceSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid data", errors: parsed.error.flatten() });
    }
    const device = await storage.updateDevice(req.params.deviceId as string, parsed.data);
    if (!device) {
      return res.status(404).json({ message: "Device not found" });
    }
    broadcast({ type: "UPDATE", payload: device });
    return res.json({ success: true, device });
  }));

  app.delete("/api/devices/:deviceId", requireAdmin, asyncHandler(async (req: Request, res: Response) => {
    const deleted = await storage.deleteDevice(req.params.deviceId as string);
    if (!deleted) {
      return res.status(404).json({ message: "Device not found" });
    }
    broadcast({ type: "DELETE", payload: { deviceId: req.params.deviceId as string } });
    return res.json({ success: true, message: "Deleted" });
  }));

  // ═════════════════════════════════════════════════════════════
  //  DEVICE REGISTRATION & ALERTS (from ESP32 controller)
  // ═════════════════════════════════════════════════════════════
  app.post("/api/register", requireDeviceKey, asyncHandler(async (req: Request, res: Response) => {
    const parsed = registerRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid body" });
    }
    const device = await storage.registerDevice(parsed.data.deviceId);
    await storage.updateControllerHeartbeat();
    broadcast({ type: "REGISTER", payload: device });
    return res.json({
      success: true,
      message: device.approved ? "Registered" : "Pending approval",
    });
  }));

  app.post("/api/alert", requireDeviceKey, asyncHandler(async (req: Request, res: Response) => {
    const parsed = alertRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid body" });
    }
    const result = await storage.triggerAlert(parsed.data.deviceId);
    await storage.updateControllerHeartbeat();
    if (!result.success) {
      return res.json({ success: false, reason: result.reason });
    }
    broadcast({ type: "ALERT", payload: { deviceId: parsed.data.deviceId } });
    broadcastDeviceUpdate(parsed.data.deviceId);
    return res.json({ success: true });
  }));

  app.post("/api/heartbeat", requireDeviceKey, asyncHandler(async (req: Request, res: Response) => {
    const parsed = registerRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid body" });
    }
    const device = await storage.registerDevice(parsed.data.deviceId);
    await storage.updateControllerHeartbeat();
    return res.json({
      success: true,
      approved: device.approved,
      alertActive: device.alertActive,
    });
  }));

  // ═════════════════════════════════════════════════════════════
  //  CONTROLLER SYNC (bidirectional state exchange)
  // ═════════════════════════════════════════════════════════════
  app.post("/api/controller-sync", requireDeviceKey, asyncHandler(async (req: Request, res: Response) => {
    try {
      const cmd = await storage.getAndClearCommand();
      const parsed = controllerSyncSchema.safeParse(req.body);

      if (!parsed.success) {
        console.warn("[SYNC] Validation failed:", parsed.error.format());
        await storage.updateControllerHeartbeat();
        return res.json({
          success: true,
          mode: await storage.getMode(),
          devices: await storage.getAllDevices(),
          command: cmd?.command,
          params: cmd?.params
        });
      }

      // 1. Authoritative Merge & Pruning FIRST
      await storage.syncFromController(parsed.data.devices, {
        mode: parsed.data.mode,
        uptime: parsed.data.uptime,
        rssi: parsed.data.rssi,
        wifiError: parsed.data.wifiError
      });

      // 2. Build Response AFTER Pruning
      const responseData: any = {
        success: true,
        mode: await storage.getMode(),
        devices: await storage.getAllDevices(),
      };

      if (cmd) {
        responseData.command = cmd.command;
        responseData.params = cmd.params;
        console.log(`[SYNC] Delivering command to Controller: ${cmd.command}`);
      }

      broadcastAllDevices();
      return res.json(responseData);
    } catch (err: any) {
      const errMsg = err?.message || String(err);
      console.error("[SYNC] Fatal error during bidirectional sync:", errMsg, err?.stack);
      return res.status(500).json({ success: false, message: "Sync Error: " + errMsg });
    }
  }));

  app.post("/api/controller-ping", requireDeviceKey, asyncHandler(async (req: Request, res: Response) => {
    const { mode, uptime, rssi, wifiError } = req.body;
    await storage.updateControllerHeartbeat(mode, uptime, rssi, wifiError);
    console.log(`[CONTROLLER] Heartbeat received at ${new Date().toISOString()} (Uptime: ${uptime}s, RSSI: ${rssi}dBm, Error: ${wifiError || "None"})`);
    broadcast({ type: "CONTROLLER_STATUS", payload: { online: true, uptime, rssi, wifiError } });
    return res.json({ success: true, serverTime: Date.now() });
  }));

  // ═════════════════════════════════════════════════════════════
  //  ADMIN ACTIONS (approve, clear alerts)
  // ═════════════════════════════════════════════════════════════
  app.post("/api/approve", requireAdmin, asyncHandler(async (req: Request, res: Response) => {
    const deviceId = req.body.deviceId;
    const parsed = approveDeviceSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid data", errors: parsed.error.flatten() });
    }
    const device = await storage.approveDevice(deviceId, parsed.data);
    if (!device) {
      return res.status(404).json({ message: "Device not found" });
    }
    broadcast({ type: "UPDATE", payload: device });
    return res.json({ success: true, device });
  }));

  app.post("/api/update", requireAdmin, asyncHandler(async (req: Request, res: Response) => {
    const deviceId = req.body.deviceId;
    const parsed = updateDeviceSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ message: "Invalid data", errors: parsed.error.flatten() });
    }
    const device = await storage.updateDevice(deviceId, parsed.data);
    if (!device) {
      return res.status(404).json({ message: "Device not found" });
    }
    broadcast({ type: "UPDATE", payload: device });
    return res.json({ success: true, device });
  }));

  app.post("/api/clearAlert", requireAdminOrDevice, asyncHandler(async (req: Request, res: Response) => {
    const deviceId = req.body.deviceId || req.params.deviceId;
    const cleared = await storage.clearAlert(deviceId);
    if (!cleared) {
      return res.status(404).json({ message: "Device not found" });
    }
    await broadcastDeviceUpdate(deviceId);
    return res.json({ success: true, message: "Alert cleared" });
  }));

  return httpServer;
}
