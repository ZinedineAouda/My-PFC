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
  const targets = ["dist/public", "public", "client/dist", "dist"];
  const root = process.cwd();
  
  // 1. Check direct paths relative to process.cwd() (the project root on Railway)
  for (const t of targets) {
    const p = path.resolve(root, t);
    if (fs.existsSync(p) && fs.existsSync(path.resolve(p, "index.html"))) {
      return p;
    }
  }

  // 2. Check relative to script's directory
  const baseDir = getBaseDir();
  for (const t of targets) {
    const p = path.resolve(baseDir, t);
    if (fs.existsSync(p) && fs.existsSync(path.resolve(p, "index.html"))) {
      return p;
    }
    // and sibling check
    const sibling = path.resolve(baseDir, "..", t);
    if (fs.existsSync(sibling) && fs.existsSync(path.resolve(sibling, "index.html"))) {
      return sibling;
    }
  }

  return null;
}

export function serveStatic(app: Express) {
  const foundPath = findPublicDir();

  if (foundPath) {
    console.log(`[SUCCESS] Static assets located at: ${foundPath}`);
    app.use(express.static(foundPath));
    app.use((req, res, next) => {
      if (req.method !== "GET" || req.path.startsWith("/api") || req.path.startsWith("/ws")) return next();
      res.sendFile(path.resolve(foundPath, "index.html"));
    });
  } else {
    const rootFiles = fs.readdirSync(process.cwd());
    console.warn(`[FAILURE] Could not find static directory. Root files: ${rootFiles.join(", ")}`);
  }
}
