import type { Express, Request, Response, NextFunction } from "express";
import { createServer, type Server } from "http";
import session from "express-session";
import { storage } from "./storage";
import { insertSlaveSchema, updateSlaveSchema, alertRequestSchema, registerRequestSchema } from "@shared/schema";

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

export async function registerRoutes(
  httpServer: Server,
  app: Express
): Promise<Server> {
  app.use(
    session({
      secret: process.env.SESSION_SECRET || "hospital-alarm-secret-key",
      resave: false,
      saveUninitialized: false,
      cookie: {
        secure: false,
        maxAge: 24 * 60 * 60 * 1000,
      },
    })
  );

  storage.seed();

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

  app.get("/api/slaves", (_req: Request, res: Response) => {
    const slaves = storage.getAllSlaves();
    return res.json(slaves);
  });

  app.post("/api/register", (req: Request, res: Response) => {
    const parsed = registerRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid request body" });
    }
    const { slaveId } = parsed.data;
    const registered = storage.registerSlave(slaveId);
    if (!registered) {
      return res.status(404).json({ success: false, message: "Unknown slave ID. Register via admin first." });
    }
    return res.json({ success: true, message: "Registered" });
  });

  app.post("/api/alert", (req: Request, res: Response) => {
    const parsed = alertRequestSchema.safeParse(req.body);
    if (!parsed.success) {
      return res.status(400).json({ success: false, message: "Invalid request body" });
    }
    const { slaveId } = parsed.data;
    const result = storage.triggerAlert(slaveId);
    if (!result.success) {
      return res.json({ success: false, reason: result.reason });
    }
    return res.json({ success: true, alertId: `alert-${slaveId}-${Date.now()}` });
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

  app.post("/api/clearAlert/:slaveId", requireAdmin, (req: Request, res: Response) => {
    const cleared = storage.clearAlert(req.params.slaveId as string);
    if (!cleared) {
      return res.status(404).json({ message: "Slave not found" });
    }
    return res.json({ success: true, message: "Alert cleared" });
  });

  return httpServer;
}
