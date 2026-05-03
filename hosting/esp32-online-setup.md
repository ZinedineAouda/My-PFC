# Configure ESP32 for Online Mode

After deploying the backend and frontend, you need to flash and configure the ESP32 controller to send data to your hosted server.

---

## Prerequisites

- Railway URL ready (e.g. `https://your-app.up.railway.app`)
- `DEVICE_API_KEY` set on Railway
- Arduino IDE installed with ESP32 board support

---

## Step 1 — Flash the Firmware

1. Open **`esp32/controller/controller.ino`** in Arduino IDE.
2. Select your board: **ESP32-S3 Dev Module**.
3. Connect the ESP32 via USB and click **Upload**.

> You do NOT need to hardcode URLs in the firmware anymore — they are configured via the web setup wizard.

---

## Step 2 — Connect to the ESP32 Setup Wi-Fi

After flashing, the ESP32 creates a Wi-Fi access point:

- **SSID:** `HospitalAlarm`
- **Password:** *(none — open network)*

Connect your phone or laptop to this network, then open:

```
http://192.168.4.1
```

---

## Step 3 — Configure the Server (Step 1 of Setup Wizard)

You will see the **Online Mode Setup** wizard. On the first screen:

| Field | Value |
|---|---|
| **Server URL** | `https://your-app.up.railway.app` |
| **Device API Key** | The value you set as `DEVICE_API_KEY` on Railway |

Click **"Test Connection"** — the ESP32 saves these settings. If successful, you move to Step 2.

---

## Step 4 — Choose Your Network Mode (Step 2 of Setup Wizard)

Choose the mode that fits your hospital setup:

| Mode | When to use |
|---|---|
| **STA Mode** | All devices (controller + units) connect to hospital Wi-Fi |
| **AP + STA Mode** | Devices connect to the controller's own AP, controller connects to hospital Wi-Fi |
| **Online + AP + STA** | Same as AP+STA but explicitly for direct cloud forwarding — internet required |

> **Recommended for online use:** Select **"Online + AP + STA"** if devices are close to the controller but you need cloud access. Select **"STA Mode"** if the whole hospital has good Wi-Fi coverage.

---

## Step 5 — Connect to Hospital Wi-Fi (Step 3 of Setup Wizard)

For **STA**, **AP+STA**, and **Online+AP+STA** modes:

1. The wizard scans and shows available Wi-Fi networks.
2. Select your hospital's internet-connected Wi-Fi.
3. Enter the Wi-Fi password and click **"Connect"**.
4. The ESP32 connects and displays its IP address.

For **Online + AP + STA** mode specifically, you will also be asked to:
- Set the **AP name** (default: `HospitalAlarm`) for devices to connect to.
- Set an **AP password** (optional, must be 8+ characters or leave empty for open).

---

## Step 6 — Setup Complete

The wizard confirms setup is complete. The ESP32 will now:

- ✅ Accept connections from patient devices (call buttons)
- ✅ Forward all alerts, registrations, and heartbeats to your Railway server
- ✅ Trigger the local buzzer on alerts
- ✅ Sync all devices every 30 seconds

---

## Step 7 — Approve Patient Devices

1. Open your Railway dashboard: `https://your-app.up.railway.app/admin`
2. Login: `admin` / `admin1234`
3. Any device that pressed its button or powered on will appear under **"Pending Approval"**.
4. Click **"Approve"**, enter the patient name, bed, and room number.
5. The device is now active and will appear on the main dashboard.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Setup wizard not loading | Make sure you are connected to `HospitalAlarm` Wi-Fi and open `http://192.168.4.1` |
| "Test Connection" fails | Check the Railway URL has `https://` and no trailing slash. Check `DEVICE_API_KEY` matches. |
| Controller can't connect to Wi-Fi | Move controller closer to router. Check password. |
| Devices not appearing on dashboard | Make sure devices are powered and connected to controller. Check device is on same network as controller. |
| Alerts not forwarding | Check Railway backend logs. Check `hasInternet` on the controller status page at `http://CONTROLLER_IP` |
