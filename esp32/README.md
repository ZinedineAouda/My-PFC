# ESP32 Hospital Patient Alarm System

## Overview
A hospital patient alarm system with a Master ESP32-S3 (web server + control panel) and multiple Slave ESP32 devices (patient call buttons). Supports 3 network modes.

## Required Hardware
- 1x ESP32-S3 Dev Module (Master)
- 1+ ESP32 Dev Module (Slaves - one per bed)
- Push buttons (one per slave)
- LEDs + 220 ohm resistors (optional, one per slave)
- Buzzer (optional, for master on GPIO 4)

## Required Arduino Libraries
Install via Arduino IDE Library Manager:
1. **ESPAsyncWebServer** by me-no-dev
2. **AsyncTCP** by me-no-dev
3. **ArduinoJson** by Benoit Blanchon (v7+)

## Quick Start

### 1. Flash the Master
- Open `master/master.ino` in Arduino IDE
- Board: **ESP32-S3 Dev Module**
- Upload

### 2. Flash Each Slave
- Open `slave/slave.ino` in Arduino IDE
- Board: **ESP32 Dev Module** (or ESP32-S3)
- Upload to each slave device

### 3. Setup Master
1. Connect your phone/laptop to Wi-Fi **"HospitalAlarm"** (open, no password)
2. Open **http://192.168.4.1** in your browser
3. Choose a network mode:
   - **AP Mode** - Master creates its own network (best for small areas)
   - **STA Mode** - Master joins hospital Wi-Fi (best for large areas)
   - **AP+STA Mode** - Both combined (hybrid, nearby + distant devices)
4. For STA/AP+STA: select hospital Wi-Fi from scan list and enter password

### 4. Setup Each Slave
1. Each slave creates its own setup Wi-Fi named **"SlaveSetup-XXXX"** (unique per device)
2. Connect your phone to that network
3. Open **http://192.168.4.1**
4. Enter a Device ID (e.g. "bed-101")
5. Enter the Master IP address (192.168.4.1 for AP mode, or the hospital network IP shown after master setup)
6. Select the Wi-Fi network to connect to and enter password if needed
7. Click "Connect & Register"

### 5. Approve Devices
1. Open the Master's **Admin Panel** (http://192.168.4.1/admin or master's hospital IP/admin)
2. New slave devices appear under "Pending Approval"
3. Click "Approve", enter patient name, bed number, and room
4. The device now appears on the Dashboard

## Network Modes

| Mode | How it works | Best for |
|------|-------------|----------|
| AP | Master creates "HospitalAlarm" Wi-Fi, all devices connect to it | Small ward, no existing Wi-Fi |
| STA | Master and slaves both join hospital Wi-Fi | Large hospital with existing network |
| AP+STA | Master runs both its own AP and joins hospital Wi-Fi | Mixed setup, some devices nearby, some far |

## Wiring

**Slave Button:** GPIO 0 to Button, other side to GND (uses internal pull-up)

**Slave LED (optional):** GPIO 2 to 220 ohm resistor to LED(+), LED(-) to GND

**Master Buzzer (optional):** GPIO 4 to Buzzer(+), Buzzer(-) to GND

## How Alerts Work
1. Patient presses bedside button
2. Slave sends alert to master via HTTP
3. Dashboard shows red alert with animation and sound notification
4. Alert stays active until cleared by admin
5. One-shot: pressing again while active does nothing

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | /api/slaves | List devices (add ?approved=1 or ?all=1) |
| GET | /api/scan | Scan nearby Wi-Fi networks |
| GET | /api/status | System status and mode info |
| POST | /api/setup | Set network mode (body: {mode: 1/2/3}) |
| POST | /api/connect-wifi | Connect to Wi-Fi (body: {ssid, password}) |
| POST | /api/register | Slave auto-registration (body: {slaveId}) |
| POST | /api/alert | Trigger alert (body: {slaveId}) |
| POST | /api/approve/:id | Approve device (admin, body: {patientName, bed, room}) |
| PUT | /api/slaves/:id | Update device info (admin) |
| DELETE | /api/slaves/:id | Remove device (admin) |
| POST | /api/clearAlert/:id | Clear alert (admin) |
