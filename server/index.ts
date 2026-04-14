import express, { type Request, Response, NextFunction } from "express";
import { sql } from "drizzle-orm";
import { registerRoutes } from "./routes";
import { serveStatic } from "./static";
import { createServer } from "http";
import { db } from "./db";
import { slaves } from "@shared/schema";

const app = express();
app.set("trust proxy", 1);

// ─── PRIORITY 1: Health checks (before any middleware) ──────────────
app.get("/health", (_req, res) => res.status(200).send("OK"));
app.get("/api/health", (_req, res) => res.status(200).json({ status: "ok" }));

// ─── PRIORITY 2: Body parsers ──────────────────────────────────────
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
  const isProd = process.env.NODE_ENV === "production" || !process.env.NODE_ENV;
  const port = parseInt(process.env.PORT || "5000", 10);

  log(`Booting in ${isProd ? "PRODUCTION" : "DEVELOPMENT"} mode...`);

  // ─── STAGE 1: Static Files (Highest Priority) ─────────────────────
  if (isProd) {
    serveStatic(app);
  }

  // ─── STAGE 2: Start Listener ──────────────────────────────────────
  // Start listening early so the platform knows we are alive
  httpServer.listen(port, "0.0.0.0", () => {
    log(`Server listening on port ${port}`);
  });

  // ─── STAGE 3: Database & API Routes ───────────────────────────────
  try {
    log("Initializing database connection...");
    await db.execute(sql`SELECT 1`);
    log("Database connection successful.");

    log("Registering API routes...");
    await registerRoutes(httpServer, app);

    if (!isProd) {
      log("Mode: Development — starting Vite");
      const { setupVite } = await import("./vite");
      await setupVite(httpServer, app);
    }
  } catch (err) {
    console.error("[BOOT] Initialization error (API might be limited):", err);
  }

  // ── Global error handler ──
  app.use((err: any, _req: Request, res: Response, _next: NextFunction) => {
    res.status(err.status || 500).json({ message: err.message || "Internal Server Error" });
  });
})();
