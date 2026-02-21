# ESP32 Hospital Patient Alarm System

## Overview
This system consists of a **Master** ESP32-S3 device that hosts a Wi-Fi network and web server, and multiple **Slave** ESP32 devices (one per patient bed) with call buttons.

## Required Hardware
- 1x ESP32-S3 Dev Module (Master)
- 1+ ESP32 Dev Module (Slaves - one per bed)
- Push buttons (one per slave)
- LEDs + 220 ohm resistors (one per slave, optional)
- Buzzer (optional, for master)

## Required Arduino Libraries
Install these via **Arduino IDE > Sketch > Include Library > Manage Libraries**:

1. **ESPAsyncWebServer** by me-no-dev
2. **AsyncTCP** by me-no-dev
3. **ArduinoJson** by Benoit Blanchon (v7+)

> If ESPAsyncWebServer is not available in Library Manager, install manually:
> - Download from https://github.com/me-no-dev/ESPAsyncWebServer
> - Download from https://github.com/me-no-dev/AsyncTCP
> - Place in your Arduino/libraries folder

## Setup Instructions

### 1. Flash the Master
1. Open `master/master.ino` in Arduino IDE
2. Select Board: **ESP32-S3 Dev Module**
3. Configure if needed (in the code):
   - `AP_SSID` - Wi-Fi network name (default: "HospitalAlarm")
   - `AP_PASSWORD` - Wi-Fi password (default: "hospital123")
   - `ADMIN_USER` / `ADMIN_PASS` - Admin login (default: admin / admin1234)
   - `BUZZER_PIN` - GPIO pin for optional buzzer (default: GPIO 4)
4. Upload to your ESP32-S3

### 2. Flash Each Slave
1. Open `slave/slave.ino` in Arduino IDE
2. Select Board: **ESP32 Dev Module** (or ESP32-S3)
3. **IMPORTANT: Change `SLAVE_ID` for each device** (e.g., "s1", "s2", "s3")
4. Configure if needed:
   - `WIFI_SSID` / `WIFI_PASSWORD` - Must match master AP settings
   - `BUTTON_PIN` - GPIO for call button (default: GPIO 0 = BOOT button)
   - `LED_PIN` - GPIO for status LED (default: GPIO 2 = built-in LED)
5. Upload to each ESP32

### 3. Wiring

**Slave Button Wiring:**
```
Button Pin 1 --> GPIO 0 (BUTTON_PIN)
Button Pin 2 --> GND
```
The code uses internal pull-up, so no external resistor needed.

**Slave LED Wiring (optional):**
```
GPIO 2 (LED_PIN) --> 220 ohm resistor --> LED Anode (+)
LED Cathode (-) --> GND
```

**Master Buzzer Wiring (optional):**
```
GPIO 4 (BUZZER_PIN) --> Buzzer (+)
Buzzer (-) --> GND
```

## Usage

1. Power on the Master device first
2. Power on Slave devices - they auto-connect to the Master's Wi-Fi
3. Connect your phone/laptop to Wi-Fi network **"HospitalAlarm"** (password: hospital123)
4. Open a browser and go to **http://192.168.4.1**

### Dashboard (http://192.168.4.1)
- Shows all patient devices in a card grid
- Active alerts highlight in red with pulsing animation
- Auto-refreshes every 2 seconds

### Admin Panel (http://192.168.4.1/admin)
- Login: username `admin`, password `admin1234`
- Add, edit, delete patient devices
- Clear active alerts

### How Alerts Work
1. Patient presses the bedside button
2. Slave sends HTTP POST to master's `/api/alert` endpoint
3. Dashboard instantly shows the alert (red highlight + notification)
4. Alert stays active until a nurse/admin clears it from the admin panel
5. Pressing the button again while alert is active does nothing (one-shot)

## API Endpoints
All return JSON.

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | /api/slaves | List all devices with status |
| POST | /api/register | Slave registration (body: {slaveId}) |
| POST | /api/alert | Trigger alert (body: {slaveId}) |
| POST | /api/admin/login | Admin login (body: {username, password}) |
| POST | /api/slaves | Add device (admin, body: {slaveId, patientName, bed, room}) |
| PUT | /api/slaves/:id | Update device (admin) |
| DELETE | /api/slaves/:id | Delete device (admin) |
| POST | /api/clearAlert/:id | Clear alert (admin) |

## Troubleshooting
- **Slave won't connect:** Check SSID/password match the master settings
- **"Unknown slave ID" on register:** Add the device ID in admin panel first
- **Button does nothing:** Check wiring, and make sure the slave ID exists in the admin panel
- **No web page loads:** Make sure you're connected to the "HospitalAlarm" Wi-Fi, then go to 192.168.4.1
