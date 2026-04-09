import express, { type Express } from "express";
import fs from "fs";
import path from "path";

export function serveStatic(app: Express) {
  const distPath = path.resolve(__dirname, "..", "..", "dist", "public");
  if (!fs.existsSync(distPath)) {
    // Fallback for different build structures
    const altPath = path.resolve(__dirname, "..", "public");
    if (fs.existsSync(altPath)) {
      app.use(express.static(altPath));
      app.get("*", (_req, res) => res.sendFile(path.resolve(altPath, "index.html")));
      return;
    }
    throw new Error(`Could not find build directory at ${distPath} or ${altPath}`);
  }

  app.use(express.static(distPath));

  app.get("*", (_req, res) => {
    res.sendFile(path.resolve(distPath, "index.html"));
  });
}
