import express, { type Express } from "express";
import fs from "fs";
import path from "path";

// A safe fallback to find the current directory regardless of ESM or CJS context
function getBaseDir() {
  if (typeof __dirname !== 'undefined') {
    return __dirname;
  }
  return process.cwd();
}

function findPublicDir(): string | null {
  const root = process.cwd();
  const baseDir = getBaseDir();
  
  // Potential locations for compiled frontend files
  const targets = [
    path.join(root, "dist", "public"),
    path.join(root, "public"),
    path.join(root, "client", "dist"),
    path.join(root, "dist"),
    path.join(baseDir, "public"),
    path.join(baseDir, "..", "dist", "public"),
    path.join(baseDir, "..", "public"),
  ];

  console.log(`[STATIC] Searching for frontend in ${targets.length} locations...`);

  for (const p of targets) {
    const indexFile = path.resolve(p, "index.html");
    if (fs.existsSync(p) && fs.existsSync(indexFile)) {
      return p;
    }
  }

  return null;
}

export function serveStatic(app: Express) {
  const foundPath = findPublicDir();

  if (foundPath) {
    console.log(`[INIT] Frontend located: ${foundPath}`);
    app.use(express.static(foundPath));
    
    // SPA Fallback: Serve index.html for any route not starting with /api or /ws 
    app.get("*", (req, res, next) => {
      if (req.path.startsWith("/api") || req.path.startsWith("/ws")) {
        return next();
      }
      res.sendFile(path.resolve(foundPath, "index.html"), (err) => {
        if (err) {
          console.error("[ERROR] Failed to send index.html:", err);
          res.status(500).send("Frontend load error. Check logs.");
        }
      });
    });
  } else {
    const rootFiles = fs.readdirSync(process.cwd());
    console.error(`[CRITICAL] No frontend found! Root contents: ${rootFiles.join(", ")}`);
    
    // Provide a helpful error page instead of "Cannot GET /"
    app.get("/", (_req, res) => {
      res.status(500).send("<h1>System Online: Frontend Missing</h1><p>The API is working, but the client files were not found in the build.</p>");
    });
  }
}
