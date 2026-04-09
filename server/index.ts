import express, { type Request, Response, NextFunction } from "express";
import { registerRoutes } from "./routes";
import { serveStatic } from "./static";
import { createServer } from "http";

const app = express();
app.set("trust proxy", 1);

// --- PRIORITY 1: HEALTHCHECKS (Before any middleware) ---
app.get("/health", (_req, res) => res.status(200).send("OK"));
app.get("/api/health", (_req, res) => res.status(200).json({ status: "ok" }));
app.get("/api/status", (_req, res) => res.status(200).json({ status: "ok", mode: 4 }));

// --- PRIORITY 2: ESSENTIAL MIDDLEWARE ---
app.use(express.json());
app.use(express.urlencoded({ extended: false }));

export function log(message: string, source = "express") {
  const formattedTime = new Date().toLocaleTimeString("en-US", {
    hour: "numeric", minute: "2-digit", second: "2-digit", hour12: true,
  });
  console.log(`${formattedTime} [${source}] ${message}`);
}

const httpServer = createServer(app);

(async () => {
  const port = parseInt(process.env.PORT || "5000", 10);
  
  // Start server early for Railway
  httpServer.listen(port, "0.0.0.0", () => {
    log(`[BOOT] Server listening on port ${port}`);
  });

  try {
    log("[BOOT] Registering routes...");
    await registerRoutes(httpServer, app);

    // Default to production if not specified
    const isProd = process.env.NODE_ENV === "production" || !process.env.NODE_ENV;
    
    if (isProd) {
      log("[BOOT] Mode: Production. Serving static files.");
      serveStatic(app);
    } else {
      log("[BOOT] Mode: Development. Starting Vite.");
      const { setupVite } = await import("./vite");
      await setupVite(httpServer, app);
    }
  } catch (err) {
    console.error("[BOOT] CRITICAL ERROR:", err);
  }

  // Last-resort fallback if static serving fails
  app.get("/", (_req, res) => {
    res.send("<h1>Hospital Alarm System</h1><p>The dashboard is loading. Please refresh in a moment.</p>");
  });

  app.use((err: any, _req: Request, res: Response, next: NextFunction) => {
    res.status(err.status || 500).json({ message: err.message || "Internal Server Error" });
  });
})();
