import express, { type Express } from "express";
import fs from "fs";
import path from "path";

export function serveStatic(app: Express) {
  const distPath = path.resolve(__dirname, "..", "..", "dist", "public");
  const altPath = path.resolve(__dirname, "..", "public");

  if (fs.existsSync(distPath)) {
    app.use(express.static(distPath));
    app.get("*", (_req, res) => res.sendFile(path.resolve(distPath, "index.html")));
    console.log(`[INFO] Serving static files from ${distPath}`);
  } else if (fs.existsSync(altPath)) {
    app.use(express.static(altPath));
    app.get("*", (_req, res) => res.sendFile(path.resolve(altPath, "index.html")));
    console.log(`[INFO] Serving static files from ${altPath}`);
  } else {
    // DO NOT THROW. Throws cause healthcheck failures on Railway.
    console.warn(`[WARNING] Static build directory not found at ${distPath} or ${altPath}. API only mode.`);
    app.get("/", (_req, res) => res.send("Hospital Alarm API is running. Cloud Dashboard is building..."));
  }
}
