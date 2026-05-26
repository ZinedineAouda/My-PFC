# Hospital Patient Alarm System — Setup Guide

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Requirements](#software-requirements)
4. [ESP32-S3 Controller Setup](#esp32-s3-controller-setup)
5. [ESP8266 Device Setup](#esp8266-device-setup)
6. [Railway Cloud Server](#railway-cloud-server)
7. [Operating Modes](#operating-modes)
8. [MQTT Topic Reference](#mqtt-topic-reference)
9. [Troubleshooting](#troubleshooting)
10. [Performance & Reliability Notes](#performance--reliability-notes)

---

## Architecture Overview

```
┌────────────────────────────────────────────────────────────┐
│                   CLOUD (Railway)                          │
│   Node.js + Express + Embedded Aedes MQTT Broker           │
│   Remote real-time dashboard access via WebSockets         │
└───────────────────────▲────────────────────────────────────┘
                        │ Real-Time MQTT / MQTTS (Mode 4 only)
┌───────────────────────┴────────────────────────────────────┐
│                 ESP32-S3 CONTROLLER (Gateway)              │
│   PicoMQTT Broker (port 1883)                              │
│   AsyncWebServer + WebSocket (port 80)                     │
│   Dashboard: http://192.168.4.1                            │
└──────┬─────────────┬─────────────┬─────────────────────────┘
       │ MQTT        │ MQTT        │ MQTT
┌──────┴──────┐ ┌────┴──────┐ ┌───┴───────┐
│ ESP8266 #1  │ │ ESP8266 #2│ │ ESP8266 #N│
│ Button: D3  │ │ Button: D3│ │ Button: D3│
│ LED: GPIO2  │ │ LED: GPIO2│ │ LED: GPIO2│
└─────────────┘ └───────────┘ └───────────┘
```

**Communication Architecture**:
- **Local Network**: Low-latency MQTT over TCP port `1883` (ESP32-S3 runs the embedded `PicoMQTT` broker).
- **Cloud Network**: Real-time MQTT/MQTTS connection (using `PicoMQTT::Client` on the ESP32) to the embedded `Aedes` MQTT broker on Railway.
- **Web App**: Real-time bidirectional WebSockets (`ws://` / `wss://`) between the Express backend and the React frontend dashboard.

---

## Hardware Requirements

### Controller (ESP32-S3)
| Component | Specification |
|-----------|---------------|
| Board | ESP32-S3 DevKitC-1 (or equivalent) |
| Flash | 16MB recommended |
| PSRAM | 8MB OPI (optional but recommended) |
| Buzzer | Active buzzer on GPIO4 |
| LED | Status LED on GPIO2 |
| Power | USB-C or 5V supply |

### Device (ESP8266)
| Component | Specification |
|-----------|---------------|
| Board | NodeMCU v1.0 / ESP-12E / ESP-01 |
| Button | Momentary push button on GPIO0 (D3) |
| LED | Built-in LED on GPIO2 (D4 - active LOW) |
| Power | USB or 3.3V supply |

### Wiring

**Controller (ESP32-S3)**:
```
GPIO4 ──── Buzzer (+)
GND   ──── Buzzer (-)
```

**Device (ESP8266 NodeMCU)**:
```
D3 (GPIO0) ──── Button ──── GND
D4 (GPIO2) ──── Built-in LED (already on board)
```

---

## Software Requirements

### Arduino IDE Setup

1. **Install Arduino IDE** (v2.x recommended): https://www.arduino.cc/en/software

2. **Add ESP32 and ESP8266 board support**:
   - Go to **File → Preferences**
   - In **Additional Board Manager URLs**, add:
     ```
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     https://arduino.esp8266.com/stable/package_esp8266com_index.json
     ```
   - Go to **Tools → Board → Board Manager**, search and install **esp32 by Espressif** and **esp8266 by ESP8266 Community**.

3. **Install Libraries** (Tools → Manage Libraries):

   | Library | Used By | Version / Source |
   |---------|---------|------------------|
   | **PicoMQTT** | Controller | Latest (via Library Manager) |
   | **ESPAsyncWebServer** | Controller | Latest (https://github.com/me-no-dev/ESPAsyncWebServer) |
   | **AsyncTCP** | Controller | Latest (https://github.com/me-no-dev/AsyncTCP) |
   | **ESPAsyncTCP** | Device | Latest (https://github.com/me-no-dev/ESPAsyncTCP) |
   | **ArduinoJson** | Controller + Device | 7.x (via Library Manager) |
   | **PubSubClient** | Device | Latest (via Library Manager) |

---

## ESP32-S3 Controller Setup

### Board Configuration (Arduino IDE)

| Setting | Value |
|---------|-------|
| Board | **ESP32S3 Dev Module** |
| USB CDC On Boot | **Enabled** |
| Flash Size | **16MB (128Mb)** |
| Partition Scheme | **Default 4MB with spiffs** (or 16MB if available) |
| PSRAM | **OPI PSRAM** (if your board has it, otherwise Disabled) |
| Upload Speed | **921600** |

### Flashing

1. Open `esp32/controller/controller.ino` in Arduino IDE.
2. Verify the configuration constants in `esp32/controller/config.h`:
   - `CLOUD_MQTT_HOST` — Domain of your Railway deployment (e.g. `your-app.up.railway.app` or a Railway TCP proxy host)
   - `CLOUD_MQTT_PORT` — Set to `1883` for standard MQTT or `8883` for TLS-encrypted MQTTS
   - `CLOUD_DEVICE_KEY` — API Key that matches the `DEVICE_API_KEY` set on Railway
   - `ADMIN_PASS` — Admin credentials for the local dashboard
3. Connect your ESP32-S3 via USB-C.
4. Click **Upload**.
5. Open Serial Monitor (115200 baud) to monitor system startup.

---

## ESP8266 Device Setup

### Board Configuration (Arduino IDE)

| Setting | Value |
|---------|-------|
| Board | **NodeMCU 1.0 (ESP-12E Module)** |
| Flash Size | **4MB (FS:2MB OTA:~1019KB)** |
| CPU Frequency | **80 MHz** |
| Upload Speed | **115200** |

### Configuration Options

Open `esp32/device/config.h`:
* **Use Hardcoded WiFi (Quick Setup)**:
  Set `USE_HARDCODED_WIFI` to `true` and define:
  ```cpp
  #define DEFAULT_WIFI_SSID  "HospitalAlarm"
  #define DEFAULT_WIFI_PASS  ""
  #define MQTT_BROKER_IP     "192.168.4.1" // Fallback to Controller SoftAP
  ```
* **Use Setup Portal (Production)**:
  Set `USE_HARDCODED_WIFI` to `false`. The device will spawn its own network `Alarm-device-XXXXXX`. Connect to it and enter the hospital WiFi and the ESP32-S3's IP address.

Click **Upload** to flash the device.

---

## Railway Cloud Server

The Railway server runs as a monolith: it contains the Express API backend and serves the React frontend compiled into `dist/public`.

### Local Development Setup

```bash
# 1. Install dependencies
npm install

# 2. Run Drizzle migrations
npm run db:push

# 3. Start development server (runs Express and hot-reloaded Vite dashboard)
npm run dev
```
Open `http://localhost:5000` to view the dashboard.

### Deploy to Railway

1. Push your project code to GitHub.
2. In Railway, click **"New Project"** → **"Deploy from GitHub repo"**.
3. Set the required variables in your service's **Variables** tab:
   - `NODE_ENV` = `production`
   - `SESSION_SECRET` = `a_long_random_string_for_cookies`
   - `DEVICE_API_KEY` = `super` (must match `CLOUD_DEVICE_KEY` in `config.h`)
   - `DATABASE_URL` = (Automatically injected if you add PostgreSQL on Railway)
4. Expose the MQTT TCP port in Railway:
   - Go to your service's **Settings** → **Networking** → **Add TCP Port**.
   - Set the internal port to `1883`. This maps a public address (e.g. `roundhouse.proxy.rlwy.net:12345`) to Aedes.
   - Put this host and port into `esp32/controller/config.h` for Mode 4!

---

## Operating Modes

1. **Mode 1: AP Only** — The controller runs a local `HospitalAlarm` WiFi network. Devices connect directly. Local dashboard at `http://192.168.4.1`. No internet required.
2. **Mode 2: STA Only** — The controller and devices connect to the facility's existing WiFi router. Local dashboard served on the DHCP-assigned IP.
3. **Mode 3: AP + STA (Hybrid)** — Controller runs AP and joins facility router simultaneously. Unbeatable local reliability.
4. **Mode 4: Online (Cloud Sync)** — Hybrid local mode + real-time cloud sync. The controller establishes a continuous MQTT/MQTTS connection to Railway. System status, approvals, and alert states are synced instantly.

---

## MQTT Topic Reference

### Local Broker Topics (ESP32 ↔ ESP8266 Devices)
| Topic | Publisher | Subscriber | QoS | Description |
|---|---|---|---|---|
| `device/{id}/alert` | ESP8266 | ESP32-S3 | 1 | Triggers an active alert |
| `device/{id}/heartbeat` | ESP8266 | ESP32-S3 | 0 | Regular ping to track online status |
| `device/{id}/status` | ESP32-S3 | ESP8266 | 1 (Retained) | Holds approval state and room assignment |
| `device/{id}/command` | ESP32-S3 | ESP8266 | 1 | Issues remote clear alert commands |

### Cloud Broker Topics (ESP32 ↔ Railway Server)
| Topic | Publisher | Subscriber | QoS | Description |
|---|---|---|---|---|
| `controller/{key}/ping` | ESP32-S3 | Railway | 0 | Sends controller uptime, RSSI, and status |
| `controller/{key}/sync` | ESP32-S3 | Railway | 1 | Publishes local devices state array |
| `controller/{key}/sync_response` | Railway | ESP32-S3 | 1 | Responds with the cloud database authority state |
| `controller/{key}/alert` | ESP32-S3 | Railway | 1 | Real-time push notification of a patient alert |
| `controller/{key}/command` | Railway | ESP32-S3 | 1 | Instantly sends clear or admin commands |

---

## Troubleshooting

### Controller won't start AP
- Ensure `AP_SSID_DEFAULT` in `config.h` is not empty.
- Power down and restart. Check Serial Monitor for boot logs.

### PicoMQTT Broker fails to connect (Mode 4)
- Check that the `CLOUD_MQTT_HOST` matches your Railway domain or proxy host exactly.
- Verify `CLOUD_DEVICE_KEY` matches the `DEVICE_API_KEY` set on Railway.
- Ensure the controller is successfully connected to the internet (has an IP from facility WiFi router).

### Web Dashboard doesn't update in real time
- Open browser console; verify WebSocket connection (`wss://...`) is established.
- In production, ensure `NODE_ENV` is set to `production` in Railway to enable compiled client assets.

---

## Performance & Reliability Notes

- **Ultra-low latency**: Button pressed → Local alert triggered in `< 50ms`. Button pressed → Cloud dashboard alert in `< 150ms`.
- **Heap Safety**: In the ESP32-S3 firmware, all JSON buffers are optimized and topic strings are pre-cached on boot, avoiding heap fragmentation and out-of-memory crashes.
- **Connection Resilience**: On broker disconnect or network loss, the server and controller dynamically queue admin commands and automatically retry connections without blocking main loops or local alerting capabilities.
