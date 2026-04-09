import express, { type Request, Response, NextFunction } from "express";
import { registerRoutes } from "./routes";
import { serveStatic } from "./static";
import { createServer } from "http";

const app = express();
app.set("trust proxy", 1);

// ─── PRIORITY 1: Health checks (before any middleware) ──────────────
app.get("/health", (_req, res) => res.status(200).send("OK"));
app.get("/api/health", (_req, res) => res.status(200).json({ status: "ok" }));
app.get("/api/status", (_req, res) => res.status(200).json({ status: "ok", mode: 4 }));

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
  const port = parseInt(process.env.PORT || "5000", 10);

  // Start server early for Railway health checks
  httpServer.listen(port, "0.0.0.0", () => {
    log(`Server listening on port ${port}`);
  });

  try {
    log("Registering routes...");
    await registerRoutes(httpServer, app);

    const isProd =
      process.env.NODE_ENV === "production" || !process.env.NODE_ENV;

    if (isProd) {
      log("Mode: Production — serving static files");
      serveStatic(app);
    } else {
      log("Mode: Development — starting Vite");
      const { setupVite } = await import("./vite");
      await setupVite(httpServer, app);
    }
  } catch (err) {
    console.error("[BOOT] CRITICAL ERROR:", err);
  }

  // ── Global error handler ──────────────────────────────────────
  app.use((err: any, _req: Request, res: Response, _next: NextFunction) => {
    res
      .status(err.status || 500)
      .json({ message: err.message || "Internal Server Error" });
  });
})();
