import express, { type Express } from "express";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

export function serveStatic(app: Express) {
  // Try multiple common build structures to be ultra-resilient
  const paths = [
    path.resolve(__dirname, "public"),              // Best for: node dist/index.cjs -> dist/public
    path.resolve(__dirname, "..", "dist", "public"), // Best for: node dist/index.cjs -> app/dist/public
    path.resolve(__dirname, "..", "..", "dist", "public"), // Legacy structure
    path.resolve(__dirname, "..", "public"),        // Root public folder
  ];

  let foundPath: string | null = null;

  for (const p of paths) {
    console.log(`[DEBUG] Checking for static files at: ${p}`);
    if (fs.existsSync(p) && fs.readdirSync(p).length > 0) {
      foundPath = p;
      break;
    }
  }

  if (foundPath) {
    console.log(`[INFO] Found static assets at: ${foundPath}`);
    app.use(express.static(foundPath));
    app.get("*", (req, res, next) => {
      // If it's an API request, let the next middleware handle it
      if (req.path.startsWith("/api")) return next();
      res.sendFile(path.resolve(foundPath!, "index.html"));
    });
  } else {
    console.warn("[WARNING] No static build directory found. Dashboard will show loading fallback.");
    // The fallback is already in index.ts
  }
}
