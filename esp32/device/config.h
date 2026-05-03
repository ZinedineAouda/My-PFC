/*
 * ══════════════════════════════════════════════════════════════
 *  Hospital Alarm — ESP8266 Device Configuration
 * ══════════════════════════════════════════════════════════════
 */
#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

// ─── Hardware Pins ──────────────────────────────────────────
#define BUTTON_PIN        5     // GPIO5 / D1 on NodeMCU
#define LED_PIN           2     // GPIO2 / D4 (built-in LED, active LOW)
#define LED_ON            LOW
#define LED_OFF           HIGH

// ─── Timing ─────────────────────────────────────────────────
#define DEBOUNCE_MS       50    // Hair-trigger button debounce
#define HEARTBEAT_MS      10000 // MQTT heartbeat every 10s
#define RECONNECT_MS      5000  // WiFi/MQTT reconnect interval
#define ALERT_LED_MS      30000 // LED stays on 30s after alert
#define MQTT_KEEPALIVE    15    // MQTT keepalive in seconds

// ─── MQTT Broker (Controller ESP32) ─────────────────────────────
#define MQTT_BROKER_IP    "192.168.4.1"
#define MQTT_BROKER_PORT  1883
#define DISCOVERY_TOPIC   "hospital/discovery"

// ─── Default WiFi (Controller AP) ───────────────────────────────
// Change these before flashing, or use the setup portal
#define DEFAULT_WIFI_SSID "HospitalAlarm"
#define DEFAULT_WIFI_PASS ""

// Set to true to skip the setup portal and use the defaults above
#define USE_HARDCODED_WIFI false

#endif // DEVICE_CONFIG_H
