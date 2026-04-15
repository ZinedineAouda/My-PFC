/*
 * ══════════════════════════════════════════════════════════════
 *  Hospital Patient Alarm System — Master Configuration
 *  Board: ESP32-S3
 * ══════════════════════════════════════════════════════════════
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <IPAddress.h>

// ─── WiFi Access Point ──────────────────────────────────────
#define AP_SSID_DEFAULT       "HospitalAlarm"
#define AP_PASS_DEFAULT       ""             // Open by default; set 8+ chars for WPA2
#define AP_CHANNEL            1
#define AP_MAX_CONNECTIONS    16
static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GATEWAY(192, 168, 4, 1);
static const IPAddress AP_SUBNET(255, 255, 255, 0);

// ─── MQTT Broker (runs on this ESP32) ───────────────────────
#define MQTT_PORT             1883
#define DISCOVERY_TOPIC       "hospital/discovery"
#define MQTT_MAX_CLIENTS      50

// ─── Device Management ──────────────────────────────────────
#define MAX_SLAVES            200
#define HEARTBEAT_TIMEOUT_MS  30000   // Mark slave offline after 30s silence
#define TIMEOUT_CHECK_MS      5000    // Run timeout scan every 5s

// ─── Cloud Sync (Mode 4 only) ───────────────────────────────
#define CLOUD_SERVER_URL      "https://my-pfc-production.up.railway.app"
#define CLOUD_DEVICE_KEY      "esp32_master_super_secure_key_123"
#define CLOUD_SYNC_INTERVAL   3000    // Reduced to 3s for faster "Stop Alert" response
#define CLOUD_PING_INTERVAL   1500    // Reduced to 1.5s for "Live" status feel
#define CLOUD_HTTP_TIMEOUT    10000   // Reduced to 10s to fail/retry faster on bad connections

// ─── Hardware Pins ──────────────────────────────────────────
#define BUZZER_PIN            4
#define STATUS_LED_PIN        2

// ─── Admin Credentials ─────────────────────────────────────
#define ADMIN_USER            "admin"
#define ADMIN_PASS            "admin1234"
#define ADMIN_TOKEN           "admin1234"

// ─── Persistence & Post-Upload ─────────────────────────────
#define ERASE_ON_FLASH        true  // Wipe all settings on new firmware upload
#define WEB_PORT              80

// ─── Operating Modes ────────────────────────────────────────
enum WiFiOpMode : uint8_t {
    MODE_NONE    = 0,   // Not yet configured (Setup Wizard)
    MODE_AP      = 1,   // Standalone: Master creates WiFi
    MODE_STA     = 2,   // Facility: Master joins Hospital WiFi
    MODE_AP_STA  = 3,   // Hybrid: Master creates WiFi + joins Hospital WiFi
    MODE_ONLINE  = 4    // Cloud: AP_STA + Active Railway Sync
};

// ─── Slave Migration Settings ──────────────────────────────
#define DISCOVERY_TOPIC       "hospital/discovery"
#define FALLBACK_SSID         "HospitalAlarm"

// ─── MQTT Topics ────────────────────────────────────────────
// Slaves publish to:
//   device/{id}/alert          — button pressed
//   device/{id}/heartbeat      — alive ping
// Master publishes to:
//   device/{id}/status         — approval info (retained)
//   device/{id}/command        — clear alert, etc.

#endif // CONFIG_H
