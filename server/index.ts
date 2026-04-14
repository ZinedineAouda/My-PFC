import express, { type Request, Response, NextFunction } from "express";
import { sql } from "drizzle-orm";
import { registerRoutes } from "./routes";
import { serveStatic } from "./static";
import { createServer } from "http";
import { db } from "./db";
import { slaves } from "@shared/schema";

const app = express();
app.set("trust proxy", 1);

// ─── STAGE 0: Fast Health Checks (Highest Priority for Railway) ─────
app.get("/health", (_req, res) => res.status(200).send("OK"));
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

  // ─── STAGE 2: Static Files & Routing ──────────────────────────────
  try {
    if (isProd) serveStatic(app);
    
    log("Connecting to database in background...");
    // Database init happens while the server is already live
    await db.execute(sql`SELECT 1`);
    log("Database linked successfully.");

    await registerRoutes(httpServer, app);

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
    res.status(err.status || 500).json({ message: err.message || "Internal Server Error" });
  });
})();
