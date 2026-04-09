# Hospital Patient Alarm System — ESP32/ESP8266 Firmware

## System Architecture

This is an MQTT-based IoT alarm system for hospital patient monitoring.

### Components

| Component | MCU | Role |
|-----------|-----|------|
| **Master** | ESP32-S3 | MQTT broker, web dashboard, device manager, cloud bridge |
| **Slave** | ESP8266 | Button sensor, MQTT client, LED feedback |

### Communication Flow

```
[Button Press]
    │
    ▼
ESP8266 Slave ──MQTT──▶ ESP32 Master ──WebSocket──▶ Browser Dashboard
                              │
                              ├──HTTPS──▶ Railway Cloud Server
                              │
                              └──Buzzer──▶ Audible Alert
```

## Quick Start

### 1. Flash the Master (ESP32-S3)

```
Board:     ESP32S3 Dev Module
Libraries: PicoMQTT, ESPAsyncWebServer, AsyncTCP, ArduinoJson
File:      esp32/master/master.ino
```

### 2. Flash the Slave(s) (ESP8266)

```
Board:     NodeMCU 1.0 (ESP-12E)
Libraries: PubSubClient, ArduinoJson, ESPAsyncWebServer, ESPAsyncTCP
File:      esp32/slave/slave.ino
```

### 3. Configure

1. Connect to **"HospitalAlarm"** WiFi
2. Open **http://192.168.4.1**
3. Select operating mode
4. Approve slave devices via dashboard

## Directory Structure

```
esp32/
├── master/
│   ├── master.ino          # Main entry point
│   ├── config.h            # System configuration
│   ├── wifi_manager.h      # WiFi AP/STA/Hybrid
│   ├── mqtt_handler.h      # Embedded MQTT broker (PicoMQTT)
│   ├── device_registry.h   # Slave tracking & timeouts
│   ├── web_dashboard.h     # HTTP server + WebSocket
│   ├── cloud_sync.h        # HTTPS cloud bridge
│   └── dashboard.h         # Embedded HTML/CSS/JS
└── slave/
    ├── slave.ino            # Main firmware
    └── config.h             # Slave configuration
```

## Configuration

All settings are in `config.h` files. Key parameters:

### Master (`esp32/master/config.h`)
- `AP_SSID_DEFAULT` — WiFi AP name
- `CLOUD_SERVER_URL` — Railway deployment URL
- `MAX_SLAVES` — Maximum device count (default: 200)
- `BUZZER_PIN` — GPIO for alert buzzer

### Slave (`esp32/slave/config.h`)
- `MQTT_BROKER_IP` — Master's IP (default: 192.168.4.1)
- `USE_HARDCODED_WIFI` — Skip setup portal
- `BUTTON_PIN` — GPIO for alert button
- `HEARTBEAT_MS` — Heartbeat interval

See [SETUP_GUIDE.md](../SETUP_GUIDE.md) for detailed instructions.
