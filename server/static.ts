import express, { type Express } from "express";
import fs from "fs";
import path from "path";

function findPublicDir(startPath: string): string | null {
  const targets = ["dist/public", "public", "client/dist", "dist"];
  
  // 1. Check direct paths relative to process.cwd() (the project root on Railway)
  const root = process.cwd();
  for (const t of targets) {
    const p = path.resolve(root, t);
    if (fs.existsSync(p) && fs.existsSync(path.resolve(p, "index.html"))) {
      return p;
    }
  }

  // 2. Check relative to __dirname (where the script bundle is)
  for (const t of targets) {
    const p = path.resolve(__dirname, t);
    if (fs.existsSync(p) && fs.existsSync(path.resolve(p, "index.html"))) {
      return p;
    }
    // and sibling check
    const sibling = path.resolve(__dirname, "..", t);
    if (fs.existsSync(sibling) && fs.existsSync(path.resolve(sibling, "index.html"))) {
      return sibling;
    }
  }

  return null;
}

export function serveStatic(app: Express) {
  const foundPath = findPublicDir(__dirname);

  if (foundPath) {
    console.log(`[SUCCESS] Static assets located at: ${foundPath}`);
    app.use(express.static(foundPath));
    app.get("*", (req, res, next) => {
      if (req.path.startsWith("/api") || req.path.startsWith("/ws")) return next();
      res.sendFile(path.resolve(foundPath, "index.html"));
    });
  } else {
    const rootFiles = fs.readdirSync(process.cwd());
    console.warn(`[FAILURE] Could not find static directory. Root files: ${rootFiles.join(", ")}`);
  }
}
