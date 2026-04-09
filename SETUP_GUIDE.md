# Hospital Patient Alarm System — Setup Guide

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Hardware Requirements](#hardware-requirements)
3. [Software Requirements](#software-requirements)
4. [ESP32-S3 Master Setup](#esp32-s3-master-setup)
5. [ESP8266 Slave Setup](#esp8266-slave-setup)
6. [Railway Cloud Server](#railway-cloud-server)
7. [Operating Modes](#operating-modes)
8. [MQTT Topic Reference](#mqtt-topic-reference)
9. [Troubleshooting](#troubleshooting)
10. [Performance Notes](#performance-notes)

---

## Architecture Overview

```
┌────────────────────────────────────────────────────────────┐
│                   CLOUD (Railway)                          │
│   Node.js + Express + WebSocket                            │
│   Remote dashboard access via HTTPS                        │
└───────────────────────┬────────────────────────────────────┘
                        │ HTTPS sync (Mode 4 only)
┌───────────────────────┴────────────────────────────────────┐
│                 ESP32-S3 MASTER                             │
│   PicoMQTT Broker (port 1883)                              │
│   AsyncWebServer + WebSocket (port 80)                     │
│   Dashboard: http://192.168.4.1                            │
└──────┬─────────────┬─────────────┬─────────────────────────┘
       │ MQTT        │ MQTT        │ MQTT
┌──────┴──────┐ ┌────┴──────┐ ┌───┴───────┐
│ ESP8266 #1  │ │ ESP8266 #2│ │ ESP8266 #N│
│ Button: D0  │ │ Button: D0│ │ Button: D0│
│ LED: GPIO2  │ │ LED: GPIO2│ │ LED: GPIO2│
└─────────────┘ └───────────┘ └───────────┘
```

**Communication Protocol**: MQTT (low latency, high scalability)
- Local: TCP on port 1883 (ESP32 runs embedded broker)
- Cloud: HTTPS sync every 10 seconds
- Dashboard: WebSocket (zero polling)

---

## Hardware Requirements

### Master (ESP32-S3)
| Component | Specification |
|-----------|---------------|
| Board | ESP32-S3 DevKitC-1 (or equivalent) |
| Flash | 16MB recommended |
| PSRAM | 8MB OPI (optional but recommended) |
| Buzzer | Active buzzer on GPIO4 |
| Power | USB-C or 5V supply |

### Slave (ESP8266)
| Component | Specification |
|-----------|---------------|
| Board | NodeMCU v1.0 / ESP-12E / ESP-01 |
| Button | Momentary push button on GPIO0 (D3) |
| LED | Built-in LED on GPIO2 (active LOW) |
| Power | USB or 3.3V supply |

### Wiring

**Master (ESP32-S3)**:
```
GPIO4 ──── Buzzer (+)
GND   ──── Buzzer (-)
```

**Slave (ESP8266 NodeMCU)**:
```
D3 (GPIO0) ──── Button ──── GND
D4 (GPIO2) ──── Built-in LED (already on board)
```

> Note: GPIO0 has an internal pull-up. The button should connect GPIO0 to GND when pressed.

---

## Software Requirements

### Arduino IDE Setup

1. **Install Arduino IDE** (v2.x recommended): https://www.arduino.cc/en/software

2. **Add ESP32 board support**:
   - File → Preferences → Additional Board Manager URLs:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
   - Tools → Board Manager → Search "esp32" → Install **esp32 by Espressif**

3. **Add ESP8266 board support**:
   - File → Preferences → Additional Board Manager URLs (add on new line):
   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
   - Tools → Board Manager → Search "esp8266" → Install **esp8266 by ESP8266 Community**

4. **Install Libraries** (Tools → Manage Libraries):

   | Library | Used By | Version |
   |---------|---------|---------|
   | **PicoMQTT** | Master | Latest |
   | **ESPAsyncWebServer** | Master + Slave | Latest |
   | **AsyncTCP** | Master | Latest |
   | **ESPAsyncTCP** | Slave | Latest |
   | **ArduinoJson** | Master + Slave | 7.x |
   | **PubSubClient** | Slave | Latest |

   > **Important**: For ESPAsyncWebServer, you may need to install from GitHub if the Library Manager version is outdated:
   > https://github.com/me-no-dev/ESPAsyncWebServer

---

## ESP32-S3 Master Setup

### Board Configuration

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | Default 4MB with spiffs |
| PSRAM | OPI PSRAM |
| Upload Speed | 921600 |

### Flashing

1. Open `esp32/master/master.ino` in Arduino IDE
2. Select the correct board and port
3. Edit `esp32/master/config.h` if needed:
   - `AP_SSID_DEFAULT` — WiFi network name
   - `CLOUD_SERVER_URL` — Railway server URL
   - `CLOUD_DEVICE_KEY` — API key for cloud auth
   - `ADMIN_PASS` — Admin dashboard password
4. Click **Upload**
5. Open Serial Monitor (115200 baud) to verify boot

### First Boot

1. Master starts in **AP mode** automatically
2. A WiFi network named **"HospitalAlarm"** appears
3. Connect to it from your phone/laptop
4. Open **http://192.168.4.1** in a browser
5. The Setup Wizard will guide you through mode selection

### File Structure

```
esp32/master/
├── master.ino          ← Main entry (setup + loop)
├── config.h            ← All configuration constants
├── wifi_manager.h      ← WiFi AP/STA/Hybrid management
├── mqtt_handler.h      ← PicoMQTT embedded broker
├── device_registry.h   ← Slave tracking & timeout detection
├── web_dashboard.h     ← AsyncWebServer + WebSocket + REST API
├── cloud_sync.h        ← HTTPS sync to Railway (Mode 4)
└── dashboard.h         ← Embedded HTML/CSS/JS (PROGMEM)
```

---

## ESP8266 Slave Setup

### Board Configuration

| Setting | Value |
|---------|-------|
| Board | NodeMCU 1.0 (ESP-12E Module) |
| Flash Size | 4MB (FS:2MB OTA:~1019KB) |
| CPU Frequency | 80 MHz |
| Upload Speed | 115200 |

### Configuration Options

**Option A: Setup Portal (default)**

Set in `esp32/slave/config.h`:
```cpp
#define USE_HARDCODED_WIFI false
```
- Slave starts its own AP named `Alarm-slave-XXXXXX`
- Connect and open `http://192.168.4.1`
- Enter the master's WiFi name and MQTT IP
- Slave connects and registers automatically

**Option B: Hardcoded Credentials (mass deployment)**

Set in `esp32/slave/config.h`:
```cpp
#define USE_HARDCODED_WIFI true
#define DEFAULT_WIFI_SSID  "HospitalAlarm"
#define DEFAULT_WIFI_PASS  ""
#define MQTT_BROKER_IP     "192.168.4.1"
```
- Slave connects immediately on boot
- Best for flashing many devices quickly

### Flashing

1. Open `esp32/slave/slave.ino` in Arduino IDE
2. Select board: **NodeMCU 1.0**
3. Edit `esp32/slave/config.h` with your settings
4. Click **Upload**
5. Open Serial Monitor (115200 baud) to verify

### File Structure

```
esp32/slave/
├── slave.ino    ← Main firmware (MQTT client + button)
└── config.h     ← Configuration constants
```

---

## Railway Cloud Server

### Prerequisites
- Node.js 20+
- Railway account (https://railway.app)
- Git

### Local Development

```bash
# Install dependencies
npm install

# Start development server
npm run dev
```

The server runs on `http://localhost:5000` with hot reload.

### Deploy to Railway

1. Push code to GitHub
2. Connect the repo in Railway dashboard
3. Set environment variables:
   ```
   NODE_ENV=production
   PORT=5000
   DEVICE_API_KEY=esp32
   SESSION_SECRET=your-random-secret-here
   ```
4. Railway auto-deploys on push

### Deployment Config

`railway.toml`:
```toml
[build]
dockerfilePath = "Dockerfile"

[deploy]
healthcheckPath = "/health"
healthcheckTimeout = 60
restartPolicyType = "ON_FAILURE"
restartPolicyMaxRetries = 5
```

### API Reference

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/health` | GET | None | Health check |
| `/api/status` | GET | None | System status |
| `/api/slaves?all=1` | GET | None | List all devices |
| `/api/admin/login` | POST | Body | Admin login |
| `/api/approve/:id` | POST | Session | Approve device |
| `/api/clearAlert/:id` | POST | Session/Key | Clear alert |
| `/api/master-sync` | POST | API Key | ESP32 state sync |
| `/ws` | WS | None | Real-time updates |

---

## Operating Modes

### Mode 1: AP Only
- Master creates WiFi network `HospitalAlarm`
- All slaves connect to this network
- Dashboard at `http://192.168.4.1`
- **No internet required**
- Best for: isolated deployments, testing

### Mode 2: STA Only
- Master joins an existing WiFi router
- Slaves also join the same router
- Dashboard at master's DHCP IP
- **Requires hospital WiFi**
- Best for: wider coverage via existing infrastructure

### Mode 3: AP + STA (Hybrid)
- Master runs AP AND joins router simultaneously
- Slaves can connect to either AP or router
- Dashboard at `192.168.4.1` (AP) or router IP (STA)
- **Recommended for production**
- Best for: reliability with fallback

### Mode 4: Online (AP + STA + Cloud)
- Same as Mode 3, plus HTTPS sync to Railway
- Remote dashboard at Railway URL
- Cloud is source of truth for approvals
- Alerts sync to cloud in real-time
- **Requires internet**
- Best for: remote monitoring, multi-site deployments

---

## MQTT Topic Reference

| Topic | Direction | QoS | Retained | Payload |
|-------|-----------|-----|----------|---------|
| `device/{id}/alert` | Slave → Master | 1 | No | `{"status":"pressed","deviceId":"...","timestamp":...}` |
| `device/{id}/heartbeat` | Slave → Master | 0 | No | `{"deviceId":"...","uptime":...,"rssi":...}` |
| `device/{id}/status` | Master → Slave | 1 | Yes | `{"approved":true,"patientName":"...","bed":"...","room":"..."}` |
| `device/{id}/command` | Master → Slave | 1 | No | `{"action":"clear_alert"}` |

---

## Troubleshooting

### Master won't start AP
- Check `config.h` — `AP_SSID_DEFAULT` must not be empty
- Check Serial Monitor for WiFi errors
- Try resetting the board

### Slave can't connect to Master
- Verify WiFi credentials match Master's AP
- Check slave's Serial Monitor for connection attempts
- Ensure master is in AP or AP+STA mode
- Try the fallback IP: `192.168.4.1`

### MQTT connection fails
- Verify PicoMQTT broker is running (check master Serial Monitor)
- Ensure slave is on the same network as master
- Check `MQTT_BROKER_IP` in slave's `config.h`
- Verify port 1883 is not blocked

### Dashboard not loading
- Try `http://192.168.4.1` directly (no HTTPS)
- Clear browser cache
- Check that WebSocket connects (green dot in header)

### Cloud sync not working (Mode 4)
- Verify `CLOUD_SERVER_URL` in master's `config.h`
- Check Railway deployment status
- Verify `DEVICE_API_KEY` matches on both sides
- Check master Serial Monitor for HTTP error codes

### Alert not triggering
- Check slave is approved in admin panel
- Verify button wiring (GPIO0 to GND)
- Check MQTT connection status in slave Serial Monitor
- Ensure alert isn't already active

---

## Performance Notes

### Latency
- **Button → Dashboard**: < 100ms (MQTT + WebSocket)
- **Button → Cloud**: ~10s (next sync cycle)
- **Dashboard load**: < 500ms (PROGMEM, no external resources)

### Scalability
- MQTT broker: tested up to 50 concurrent connections
- Device registry: supports 200 devices (configurable)
- WebSocket: supports 10+ simultaneous dashboard viewers

### Memory Usage (ESP32-S3)
- Free heap at boot: ~250KB
- Per-slave overhead: ~200 bytes
- Dashboard HTML: ~25KB flash

### Power Optimization
- ESP8266 modem sleep disabled for reliability
- MQTT keepalive: 15 seconds
- Heartbeat interval: 10 seconds

### Reliability Features
- WiFi auto-reconnect (non-blocking, 5s interval)
- MQTT auto-reconnect (non-blocking, 5s interval)
- Device timeout detection (30s threshold)
- Cloud sync retry on failure
- WebSocket auto-reconnect (2s interval in browser)
- No `delay()` calls anywhere — fully non-blocking
