import express, { type Request, Response, NextFunction } from "express";
import { sql } from "drizzle-orm";
import { registerRoutes } from "./routes";
import { serveStatic } from "./static";
import { createServer } from "http";
import { db } from "./db";
import { devices } from "@shared/schema";

const app = express();
app.set("trust proxy", 1);

// ─── STAGE 0: Fast Health Checks (Highest Priority for Railway) ─────
app.get("/health", (_req, res) => {
  console.log("[HEALTH] Responding to /health check: OK");
  res.status(200).send("OK");
});
app.get("/api/health", (_req, res) => res.status(200).json({ status: "ok" }));

// ─── Body parsers ──
app.use(express.json());
app.use(express.urlencoded({ extended: false }));

export function log(message: string, source = "express") {
  const formattedTime = new Date().toLocaleTimeString("en-US", {
    hour: "numeric",
    minute: "2-digit",
    second: "2-digit",
    hour12: true,
  });
  console.log(`${formattedTime} [${source}] ${message}`);
}

const httpServer = createServer(app);

(async () => {
  const port = parseInt(process.env.PORT || "5000", 10);
  const isProd = process.env.NODE_ENV === "production" || !process.env.NODE_ENV;

  // ─── STAGE 1: Open the Gates (Pass Healthchecks Instantly) ────────
  httpServer.listen(port, "0.0.0.0", () => {
    log(`Server ready on port ${port} (Status: Healthy)`);
  });

  // ─── STAGE 2: Critical DB Migrations (Nomenclature Shift) ──────────
  try {
    log("Verifying system schema and terminology...");
    
    // Ensure table exists
    await db.execute(sql`
      CREATE TABLE IF NOT EXISTS system_settings (
        id INTEGER PRIMARY KEY DEFAULT 1,
        controller_last_seen BIGINT,
        controller_uptime INTEGER,
        controller_rssi INTEGER,
        controller_wifi_error TEXT,
        wifi_mode INTEGER NOT NULL DEFAULT 1,
        pending_command TEXT,
        command_params TEXT
      );
    `);

    // Force-rename old columns if they exist
    const renames = [
      ["master_last_seen", "controller_last_seen"],
      ["master_uptime", "controller_uptime"],
      ["master_rssi", "controller_rssi"]
    ];

    for (const [oldCol, newCol] of renames) {
      try {
        await db.execute(sql.raw(`ALTER TABLE system_settings RENAME COLUMN ${oldCol} TO ${newCol};`));
        log(`[MIGRATE] Renamed ${oldCol} to ${newCol}`);
      } catch(e) {}
    }

    // Force-add new columns if missing
    const newCols = [
      ["controller_last_seen", "BIGINT"],
      ["controller_uptime", "INTEGER"],
      ["controller_rssi", "INTEGER"],
      ["controller_wifi_error", "TEXT"],
      ["wifi_mode", "INTEGER NOT NULL DEFAULT 1"],
      ["pending_command", "TEXT"],
      ["command_params", "TEXT"]
    ];

    for (const [col, type] of newCols) {
      try {
        await db.execute(sql.raw(`ALTER TABLE system_settings ADD COLUMN ${col} ${type};`));
        log(`[MIGRATE] Added column: ${col}`);
      } catch(e) {}
    }

    // Ensure the singleton row exists
    await db.execute(sql`
      INSERT INTO system_settings (id) 
      VALUES (1) 
      ON CONFLICT (id) DO NOTHING;
    `);
    log("Database schema is now up to date.");
  } catch (err) {
    console.error("[CRITICAL] Migration failed:", err);
  }

  // ─── STAGE 3: API & Admin Routes ──────────────────────────────────
  try {
    log("Registering API routes...");
    await registerRoutes(httpServer, app);
    
    log("Activating Frontend Dashboard...");
    if (isProd) serveStatic(app);

    if (!isProd) {
      log("Mode: Development — starting Vite");
      const { setupVite } = await import("./vite");
      await setupVite(httpServer, app);
    }
  } catch (err) {
    console.error("[BOOT] Initialization error:", err);
  }

  // ── Global error handler ──
  app.use((err: any, _req: Request, res: Response, _next: NextFunction) => {
    console.error("[EXPRESS] Error Handler:", err);
    res.status(err.status || 500).json({ 
      message: err.message || "Internal Server Error",
      success: false 
    });
  });

  // ── Process-level safety guards (Prevent 502/Crashes on Railway) ──
  process.on("unhandledRejection", (reason, promise) => {
    console.error("[FATAL] Unhandled Rejection at:", promise, "reason:", reason);
    // On Railway, we stay alive but log the error for diagnostics
  });

  process.on("uncaughtException", (err) => {
    console.error("[FATAL] Uncaught Exception:", err);
    // We log but don't exit(1) unless it's a non-recoverable system error
    // express-session or DB failures shouldn't kill the dashboard
  });

})();
