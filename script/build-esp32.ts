import { build as viteBuild } from "vite";
import { readFile, writeFile } from "fs/promises";
import path from "path";
import zlib from "zlib";
import { promisify } from "util";

const gzip = promisify(zlib.gzip);

async function buildEsp32Dashboard() {
  console.log("Building React dashboard for ESP32 (Single File)...");
  
  await viteBuild({
    configFile: path.resolve(process.cwd(), "vite.config.esp32.ts")
  });

  const distPath = path.resolve(process.cwd(), "dist/esp32/index.html");
  const htmlContent = await readFile(distPath, "utf-8");

  console.log(`Original index.html size: ${(htmlContent.length / 1024).toFixed(2)} KB`);

  // Compress using gzip (maximum compression)
  const compressed = await gzip(htmlContent, { level: 9 });
  console.log(`Gzipped size: ${(compressed.length / 1024).toFixed(2)} KB`);

  // Convert to C++ PROGMEM format
  let headerContent = `// Auto-generated from React build — DO NOT EDIT
#ifndef DASHBOARD_STATIC_H
#define DASHBOARD_STATIC_H

#include <Arduino.h>

const size_t react_index_html_gz_len = ${compressed.length};
const uint8_t react_index_html_gz[] PROGMEM = {\n`;

  for (let i = 0; i < compressed.length; i++) {
    headerContent += `0x${compressed[i].toString(16).padStart(2, '0')}`;
    if (i < compressed.length - 1) {
      headerContent += ", ";
    }
    if ((i + 1) % 16 === 0) {
      headerContent += "\n  ";
    }
  }

  headerContent += `\n};

#endif // DASHBOARD_STATIC_H
`;

  const outPath = path.resolve(process.cwd(), "esp32/controller/dashboard_static.h");
  await writeFile(outPath, headerContent, "utf-8");
  
  console.log(`Successfully wrote to ${outPath}!`);
}

buildEsp32Dashboard().catch((err) => {
  console.error("Failed to build ESP32 dashboard:", err);
  process.exit(1);
});
