import express, { type Request, Response, NextFunction } from "express";
import { registerRoutes } from "./routes";
import { serveStatic } from "./static";
import { createServer } from "http";

const app = express();
app.set("trust proxy", 1);

// --- EMERGENCY BOOTSTRAP HEALTHCHECK ---
// This handles Railway probes immediately, even before the rest of the app boots.
app.get("/health", (_req, res) => res.status(200).send("OK"));
app.get("/api/health", (_req, res) => res.status(200).json({ status: "ok" }));

const httpServer = createServer(app);

app.use(express.json());
app.use(express.urlencoded({ extended: false }));

export function log(message: string, source = "express") {
  const formattedTime = new Date().toLocaleTimeString("en-US", {
    hour: "numeric", minute: "2-digit", second: "2-digit", hour12: true,
  });
  console.log(`${formattedTime} [${source}] ${message}`);
}

(async () => {
  const port = parseInt(process.env.PORT || "5000", 10);
  
  // START LISTENING IMMEDIATELY
  // This satisfies Railway healthchecks immediately.
  httpServer.listen(port, "0.0.0.0", () => {
    log(`Server listening on port ${port} (0.0.0.0)`);
  });

  try {
    log("Initializing routes and static files...");
    await registerRoutes(httpServer, app);

    if (process.env.NODE_ENV === "production") {
      serveStatic(app);
    } else {
      const { setupVite } = await import("./vite");
      await setupVite(httpServer, app);
    }
    log("App initialization complete.");
  } catch (err) {
    console.error("CRITICAL BOOT ERROR:", err);
    // Still keep the server alive so we can see logs
  }

  app.use((err: any, _req: Request, res: Response, next: NextFunction) => {
    const status = err.status || err.statusCode || 500;
    res.status(status).json({ message: err.message || "Internal Server Error" });
  });
})();
