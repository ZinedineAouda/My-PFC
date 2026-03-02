# ESP32 Hospital Patient Alarm System

## Overview
A hospital patient alarm system with a Master ESP32-S3 and multiple Slave ESP8266 ESP-01 devices (patient call buttons). Available in **two versions**:

### Version 1: Offline (Local Network)
Everything runs locally. The Master ESP32-S3 hosts the web dashboard, admin panel, and API on its own. No internet required.
- Master: `offline/master/master.ino`

### Version 2: Online (Cloud Dashboard)
The Master ESP32-S3 collects data locally from slaves, then forwards it to a hosted web server (e.g. Replit). The dashboard and admin panel run online, accessible from anywhere.
- Master: `online/master/master.ino`

### Slave (Shared)
The slave firmware is the same for both versions — one file, flash to all ESP-01 devices.
- Slave: `slave/slave.ino`

### File Structure
```
esp32/
├── offline/master/master.ino   ← Offline master firmware
├── online/master/master.ino    ← Online master firmware
├── slave/slave.ino             ← Shared slave firmware (both versions)
└── README.md                   ← This file
```

## Required Hardware
- 1x ESP32-S3 Dev Module (Master)
- 1+ ESP8266 ESP-01 or ESP-01S **OR** NodeMCU ESP8266 (Slaves - one per bed)
- Push buttons (one per slave)
- 3.3V power supply/USB for each Slave device
- Buzzer (optional, for master on GPIO 4)

## Required Arduino Libraries

### Master (ESP32-S3)
Install via Arduino IDE Library Manager:
1. **ESPAsyncWebServer** by me-no-dev
2. **AsyncTCP** by me-no-dev
3. **ArduinoJson** by Benoit Blanchon (v7+)

### Slave (ESP8266 ESP-01 or NodeMCU ESP8266)
Install via Arduino IDE Library Manager:
1. **ESPAsyncWebServer** (ESP8266 version) by me-no-dev
2. **ESPAsyncTCP** by me-no-dev
3. **ArduinoJson** by Benoit Blanchon (v7+)

---

## Version 1: Offline Setup

### 1. Flash the Master (Offline)
- Open `offline/master/master.ino` in Arduino IDE
- Board: **ESP32-S3 Dev Module**
- Upload

### 2. Flash Each Slave
Choose the correct file based on your hardware:

**For ESP-01 / ESP-01S:**
- Open `slave/slave.ino` in Arduino IDE
- Board: **Generic ESP8266 Module**
- Flash Size: **1MB** (or 512KB for older ESP-01)
- Upload Speed: **115200**
- *Note:* You need a USB-to-serial adapter. Hold GPIO0 LOW while powering on to enter flash mode.

**For NodeMCU ESP8266:**
- Open `slave/nodemcu_slave.ino` in Arduino IDE
- Board: **NodeMCU 1.0 (ESP-12E Module)**
- Flash Size: **4MB**
- Upload Speed: **115200**

### 3. Setup Master (Offline)
1. Connect your phone/laptop to Wi-Fi **"HospitalAlarm"** (open, no password)
2. Open **http://192.168.4.1** in your browser
3. Choose a network mode:
   - **AP Mode** - Master creates its own network (best for small areas)
   - **STA Mode** - Master joins hospital Wi-Fi (best for large areas)
   - **AP+STA Mode** - Both combined (hybrid)
4. For STA/AP+STA: select hospital Wi-Fi from scan list and enter password

### 4. Setup Each Slave
1. Each slave creates its own setup Wi-Fi named **"SlaveSetup-XXXX"** (unique per device)
2. Connect your phone to that network
3. Open **http://192.168.4.1**
4. Enter a Device ID (e.g. "bed-101")
5. Enter the Master IP address (192.168.4.1 for AP mode, or the hospital network IP)
6. Select the Wi-Fi network to connect to and enter password
7. Click "Connect & Register"

### 5. Approve Devices & Use Dashboard (Offline)
1. Open the Master's **Admin Panel** (http://192.168.4.1/admin or master's network IP/admin)
2. New slave devices appear under "Pending Approval"
3. Click "Approve", enter patient name, bed number, and room
4. The device now appears on the Dashboard at http://192.168.4.1/

---

## Version 2: Online Setup

### 1. Deploy the Web Server
1. Deploy/publish the Replit project (the web app in this repo)
2. Note the URL (e.g. `https://your-app.replit.app`)
3. Set the `DEVICE_API_KEY` environment variable in your Replit project (any random string)

### 2. Flash the Master (Online)
- Open `online/master/master.ino` in Arduino IDE
- **Before uploading**, edit these two lines at the top:
  ```cpp
  char serverURL[128] = "https://YOUR-APP.replit.app";  // Your Replit URL
  char deviceKey[64] = "YOUR_DEVICE_API_KEY";           // Must match server's DEVICE_API_KEY
  ```
- Board: **ESP32-S3 Dev Module**
- Upload

### 3. Flash Each Slave
Choose the correct file based on your hardware:

**For ESP-01 / ESP-01S:**
- Open `slave/slave.ino`
- Board: **Generic ESP8266 Module**, Flash Size: **1MB**
- Upload

**For NodeMCU ESP8266:**
- Open `slave/nodemcu_slave.ino`
- Board: **NodeMCU 1.0 (ESP-12E Module)**, Flash Size: **4MB**
- Upload

### 4. Setup Master (Online)
1. Connect your phone to Wi-Fi **"HospitalAlarm"**
2. Open **http://192.168.4.1**
3. **Step 1 — Server Config**: Enter your hosted server URL and device API key, click "Test Connection"
4. **Step 2 — Network Mode**: Choose STA or AP+STA (internet required for online mode)
   - **STA Mode** (recommended) — All devices join hospital Wi-Fi
   - **AP+STA Mode** — Master creates AP for slaves AND joins hospital Wi-Fi
5. Select hospital Wi-Fi and enter password
6. Setup complete! Data will forward to the hosted dashboard

### 5. Setup Slaves
Same as offline version — each slave creates "SlaveSetup-XXXX" Wi-Fi, configure via web page.

### 6. Approve Devices & Monitor (Online)
1. Open your hosted dashboard URL (e.g. `https://your-app.replit.app`)
2. Go to `/admin`, login with admin/admin1234
3. Approve pending devices with patient name, bed, room
4. Monitor all patients on the main dashboard from anywhere!

### How Online Mode Works
```
[Slave 1] ---button press---> [Master ESP32] ---HTTP POST---> [Hosted Server]
[Slave 2] ---button press---> [Master ESP32] ---HTTP POST---> [Hosted Server]
                                     |                              |
                              Local buzzer                   Online Dashboard
                              Status page                    Admin Panel
                              UDP beacon                     (accessible anywhere)
```

- Slaves send alerts to Master via local network (same as offline)
- Master immediately forwards register/alert/clear events to the hosted server
- Master sends periodic heartbeats (every 30s) to keep slave status updated online
- The hosted dashboard and admin panel show real-time data
- Master still beeps the buzzer locally for alerts

---

## Network Modes

| Mode | How it works | Available in |
|------|-------------|--------------|
| AP | Master creates "HospitalAlarm" Wi-Fi, all devices connect to it | Offline only |
| STA | Master and slaves both join hospital Wi-Fi | Both versions |
| AP+STA | Master runs both its own AP and joins hospital Wi-Fi | Both versions |

**Note:** Online mode requires internet, so AP-only mode is not available for online version.

## Wiring

### Slave Option 1: ESP8266 ESP-01 / ESP-01S
| Pin | Function | Notes |
|-----|----------|-------|
| GPIO 0 | Call Button | Wire push button between GPIO0 and GND. Must be HIGH (open) at boot! |
| GPIO 2 | Onboard LED | Active LOW on ESP-01S. Must be HIGH at boot. Status indicator |
| VCC | Power | 3.3V only! Do NOT connect to 5V |
| GND | Ground | Connect to GND |
| CH_PD/EN | Enable | Must be pulled HIGH (3.3V) for chip to work |

**Important ESP-01 notes:**
- GPIO0 must be HIGH (button not pressed) during power-on to boot normally
- Press the call button only after the device has booted (LED blinks twice)
- ESP-01S has a blue LED on GPIO2 (active LOW); original ESP-01 may differ
- No buzzer pin available on ESP-01 (only 2 GPIO pins)

### Slave Option 2: NodeMCU ESP8266
| Pin | Function | Notes |
|-----|----------|-------|
| D1 (GPIO5) | Call Button | Wire push button between D1 and GND. |
| D2 (GPIO4) | External LED | Wire LED to D2 and GND (with a resistor) |
| 3V3 / Vin | Power | Complete power setup via 3.3V/5V pin or use USB |
| GND | Ground | Connect to GND |

**Important NodeMCU notes:**
- Unlike the ESP-01, the chosen D1 and D2 pins do not interfere with the boot process.
- The button should trigger an active LOW signal when pressed (D1 to GND).

### Master (ESP32-S3)
| Pin | Function | Notes |
|-----|----------|-------|
| GPIO 4 | Buzzer (optional) | Wire buzzer(+) to GPIO4, buzzer(-) to GND |

## How Alerts Work
1. Patient presses bedside button
2. Slave sends alert to master via HTTP
3. Master buzzes locally AND (online mode) forwards to hosted server
4. Dashboard shows red alert with animation
5. Alert stays active until cleared by admin
6. One-shot: pressing again while active does nothing

## API Endpoints (Web Server)

| Method | Endpoint | Auth | Description |
|--------|----------|------|-------------|
| GET | /api/slaves | None | List devices (?approved=1 or ?all=1) |
| GET | /api/scan | None | Scan nearby Wi-Fi (offline only) |
| GET | /api/status | None | System status and mode info |
| POST | /api/setup | None | Set network mode (body: {mode: 1/2/3}) |
| POST | /api/connect-wifi | None | Connect to Wi-Fi (body: {ssid, password}) |
| POST | /api/register | Device Key | Slave registration (body: {slaveId}) |
| POST | /api/alert | Device Key | Trigger alert (body: {slaveId}) |
| POST | /api/heartbeat | Device Key | Update slave lastSeen (body: {slaveId}) |
| POST | /api/approve/:id | Admin | Approve device (body: {patientName, bed, room}) |
| PUT | /api/slaves/:id | Admin | Update device info |
| DELETE | /api/slaves/:id | Admin | Remove device |
| POST | /api/clearAlert/:id | Admin/Device | Clear alert |

**Device Key Auth:** Set `DEVICE_API_KEY` env var on server. ESP32 sends `X-Device-Key` header. If not set, device endpoints are open (for offline/testing).
