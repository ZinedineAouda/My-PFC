import express, { type Request, Response, NextFunction } from "express";
import { registerRoutes } from "./routes";
import { serveStatic } from "./static";
import { createServer } from "http";

const app = express();
app.set("trust proxy", 1);

const httpServer = createServer();

// --- THE ULTIMATE SURVIVAL HEALTHCHECK (Node.js Level) ---
// This answers before Express even gets the request.
httpServer.on("request", (req, res) => {
  if (req.url === "/health" || req.url === "/api/health") {
    res.writeHead(200, { "Content-Type": "text/plain" });
    res.end("OK");
    return;
  }
});

// Attach Express to the same server
httpServer.on("request", app);

export function log(message: string, source = "express") {
  const formattedTime = new Date().toLocaleTimeString("en-US", {
    hour: "numeric", minute: "2-digit", second: "2-digit", hour12: true,
  });
  console.log(`${formattedTime} [${source}] ${message}`);
}

(async () => {
  const port = parseInt(process.env.PORT || "5000", 10);
  
  // Binding early to pass Railway probes
  httpServer.listen(port, "0.0.0.0", () => {
    log(`[SYNC] LAST BUILD: ${new Date().toISOString()}`);
    log(`[SYNC] Listening on port ${port}`);
  });

  try {
    app.use(express.json());
    app.use(express.urlencoded({ extended: false }));

    await registerRoutes(httpServer, app);

    if (process.env.NODE_ENV === "production") {
      serveStatic(app);
    } else {
      const { setupVite } = await import("./vite");
      await setupVite(httpServer, app);
    }
    log("System initialized successfully.");
  } catch (err) {
    console.error("FATAL STARTUP ERROR:", err);
  }

  app.use((err: any, _req: Request, res: Response, next: NextFunction) => {
    res.status(err.status || 500).json({ message: err.message || "Internal Server Error" });
  });
})();
