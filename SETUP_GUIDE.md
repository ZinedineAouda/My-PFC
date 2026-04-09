# 🏥 Hospital Alarm System - Railway Setup Guide

Since you are deploying everything on **Railway**, the setup is even simpler. Your Railway service will serve both the **Real-Time API** and the **Management Portal** on the same domain.

---

## 1. Cloud Deployment (Railway)
1.  **Clone & Push**: Ensure your latest code (including my updates) is pushed to your GitHub.
2.  **Deploy**:
    - Go to [railway.app](https://railway.app).
    - Create a **New Project** → **Deploy from GitHub repo**.
    - Railway will build the `Dockerfile`.
3.  **Environment Variables**:
    - Go to **Variables** in Railway.
    - Add `SESSION_SECRET`: (Any random string).
    - Add `DEVICE_API_KEY`: `esp32`.
    - **Note:** Do NOT add `VITE_API_URL`. The app will automatically detect it is running on the same domain.
4.  **Networking**:
    - Go to **Settings** → **Networking** → **Generate Domain**.
    - Your portal will be live at: `https://your-app.up.railway.app`.

---

## 2. Hardware Setup (Arduino IDE)

### A. Prerequisites
- [Arduino IDE](https://www.arduino.cc/en/software) installed.
- ESP32 & ESP8266 Board support added.
- Libraries installed: `ArduinoJson`, `ESPAsyncWebServer`, `AsyncTCP` (ESP32), `ESPAsyncTCP` (ESP8266).

### B. Flashing the Master (ESP32)
1.  Open `esp32/master/master.ino`.
2.  **Configuration** (Lines 12-13):
    - `serverURL`: Paste your **Railway URL**.
    - `deviceKey`: `esp32` (Must match Railway `DEVICE_API_KEY`).
3.  Upload to your ESP32-S3.

### C. Flashing the Slaves (ESP8266)
1.  Open `esp32/slave/slave.ino`.
2.  Upload to your ESP8266 nodes.

---

## 3. Operational Workflow

### 🛰️ Step 1: Connect Master to Hospital WiFi
1.  Power on the Master. 
2.  Connect phone/PC to WiFi: `HospitalAlarm`.
3.  Navigate to `http://192.168.4.1`.
4.  Select **Cloud Mode (Mode 4)**.
5.  Enter your **Hospital WiFi** name and password.

### 🛡️ Step 2: Access Management Hub
1.  Open your **Railway URL** in any browser.
2.  Navigate to `/admin` (e.g., `https://your-app.up.railway.app/admin`).
3.  Login: `admin` / `admin1234`.
4.  When Slaves appear, click **Authorize** and assign Room & Bed.

---

> [!IMPORTANT]
> **Signal Check:** Once connected to WiFi, the Master device will begin heartbeating. You will see the **Hub Link** icon on your portal turn Green (Spinning) once the handshake is successful.
