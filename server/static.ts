import express, { type Express } from "express";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

// ESM-compatible __dirname
const __filename_esm = fileURLToPath(import.meta.url);
const __dirname_esm = path.dirname(__filename_esm);

function findPublicDir(): string | null {
  const targets = ["dist/public", "public", "client/dist", "dist"];
  
  // 1. Check direct paths relative to process.cwd() (the project root on Railway)
  const root = process.cwd();
  for (const t of targets) {
    const p = path.resolve(root, t);
    if (fs.existsSync(p) && fs.existsSync(path.resolve(p, "index.html"))) {
      return p;
    }
  }

  // 2. Check relative to this file's directory
  for (const t of targets) {
    const p = path.resolve(__dirname_esm, t);
    if (fs.existsSync(p) && fs.existsSync(path.resolve(p, "index.html"))) {
      return p;
    }
    // and sibling check
    const sibling = path.resolve(__dirname_esm, "..", t);
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
