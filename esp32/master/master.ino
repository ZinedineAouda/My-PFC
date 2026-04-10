/*
 * ══════════════════════════════════════════════════════════════════════
 *  Hospital Patient Alarm System — ESP32-S3 Master
 * ══════════════════════════════════════════════════════════════════════
 *
 *  Architecture:
 *    - PicoMQTT embedded broker for slave communication (port 1883)
 *    - ESPAsyncWebServer + WebSocket for local dashboard (port 80)
 *    - HTTPS sync to Railway cloud server (Mode 4)
 *
 *  Required Libraries (install via Arduino Library Manager):
 *    - PicoMQTT           (MQTT broker + client)
 *    - ESPAsyncWebServer   (async HTTP server)
 *    - AsyncTCP            (async TCP for ESP32)
 *    - ArduinoJson         (JSON serialization)
 *
 *  Board: ESP32-S3 Dev Module
 *  Flash: 16MB, PSRAM: 8MB (OPI)
 *
 * ══════════════════════════════════════════════════════════════════════
 */

#include <WiFi.h>
#include <Preferences.h>
#include "config.h"
#include "device_registry.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "web_dashboard.h"
#include "cloud_sync.h"

// ─── Global State ───────────────────────────────────────────────────
WiFiOpMode   currentMode = MODE_NONE;
bool         setupDone   = false;
Preferences  prefs;

// ─── Module Instances ───────────────────────────────────────────────
DeviceRegistry registry;
WifiManager    wifiMgr;
MqttHandler    mqttHandler(registry);
WebDashboard   dashboard(registry);
CloudSync      cloudSync(registry);

// ─── Buzzer State (non-blocking) ────────────────────────────────────
unsigned long lastBuzzerToggle = 0;
bool          buzzerState      = false;

// ─── Forward Declarations ───────────────────────────────────────────
void onDeviceChange(const String& deviceId, const char* eventType);
void onWifiConnect(const char* ssid, const char* pass);
void onSetup(int mode, const char* apSSID, const char* apPass);
void handleBuzzer();

void loadSettings() {
    prefs.begin("master_cfg", false);
    setupDone = prefs.getBool("setupDone", false);
    if (setupDone) {
        currentMode = (WiFiOpMode)prefs.getInt("mode", (int)MODE_AP);
        String apS = prefs.getString("apSSID", AP_SSID_DEFAULT);
        String apP = prefs.getString("apPass", "");
        String staS = prefs.getString("staSSID", "");
        String staP = prefs.getString("staPass", "");
        
        wifiMgr.setAPCredentials(apS.c_str(), apP.c_str());
        if (staS.length() > 0) {
            wifiMgr.setSTACredentials(staS.c_str(), staP.c_str());
        }
        Serial.println("[PREFS] Loaded saved configuration.");
    }
    prefs.end();
}

void saveSettings() {
    prefs.begin("master_cfg", false);
    prefs.putBool("setupDone", setupDone);
    prefs.putInt("mode", (int)currentMode);
    prefs.putString("apSSID", wifiMgr.apSSID());
    prefs.putString("apPass", wifiMgr.apPass());
    prefs.putString("staSSID", wifiMgr.staSSID());
    prefs.putString("staPass", wifiMgr.staPass());
    prefs.end();
    Serial.println("[PREFS] Configuration saved.");
}

// ═══════════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("╔══════════════════════════════════════════╗");
    Serial.println("║  Hospital Patient Alarm System — Master  ║");
    Serial.println("║  MQTT Broker + Web Dashboard + Cloud     ║");
    Serial.println("╚══════════════════════════════════════════╝");

    // ── Hardware ────────────────────────────────────────────
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // ── Load Config & WiFi ──────────────────────────────────
    loadSettings();
    if (!setupDone) {
        wifiMgr.beginSetup();
        WiFi.scanNetworks(true); // Start background scan
    } else {
        wifiMgr.applyMode(currentMode);
    }

    // ── Device Registry: wire change callback ───────────────
    registry.onDeviceChange(onDeviceChange);

    // ── MQTT Broker: start on port 1883 ─────────────────────
    mqttHandler.begin();

    // ── Web Dashboard: start with setup page ────────────────
    dashboard.onConnect(onWifiConnect);
    dashboard.onSetup(onSetup);
    dashboard.begin(setupDone, currentMode);

    Serial.println("[BOOT] System ready");
    Serial.println("[BOOT] Dashboard: http://192.168.4.1");
    Serial.println("[BOOT] MQTT Broker: 192.168.4.1:1883");
    Serial.printf("[BOOT] Free heap: %d bytes\n", ESP.getFreeHeap());
}

// ═══════════════════════════════════════════════════════════════════
//  LOOP — fully non-blocking
// ═══════════════════════════════════════════════════════════════════
void loop() {
    // ── WiFi management (auto-reconnect) ────────────────────
    wifiMgr.handle();

    // ── MQTT broker processing ──────────────────────────────
    mqttHandler.handle();

    // ── WebSocket client cleanup ────────────────────────────
    dashboard.handle();

    // ── Device timeout detection ────────────────────────────
    registry.checkTimeouts();

    // ── Cloud sync (Mode 4 only) ────────────────────────────
    if (currentMode == MODE_ONLINE && wifiMgr.staConnected()) {
        cloudSync.handle();
    }

    // ── Buzzer for active alerts (non-blocking) ─────────────
    handleBuzzer();
}

// ═══════════════════════════════════════════════════════════════════
//  CALLBACKS
// ═══════════════════════════════════════════════════════════════════

// Called by DeviceRegistry on any state change
void onDeviceChange(const String& deviceId, const char* eventType) {
    Serial.printf("[EVENT] %s → %s\n", deviceId.c_str(), eventType);

    // Push update to all WebSocket clients
    if (strcmp(eventType, "delete") == 0) {
        dashboard.broadcastDelete(deviceId);
    } else {
        dashboard.broadcastDeviceUpdate(deviceId);
    }

    // Publish MQTT status to the slave (retained)
    if (strcmp(eventType, "approve") == 0 || strcmp(eventType, "update") == 0) {
        mqttHandler.publishDeviceStatus(deviceId);
    }

    // Send clear command to slave via MQTT
    if (strcmp(eventType, "clear") == 0) {
        mqttHandler.sendCommand(deviceId, "clear_alert");
    }

    // Force immediate cloud sync on important events
    if (currentMode == MODE_ONLINE && wifiMgr.staConnected()) {
        if (strcmp(eventType, "alert") == 0 ||
            strcmp(eventType, "approve") == 0 ||
            strcmp(eventType, "clear") == 0) {
            cloudSync.syncNow();
        }
    }
}

// Called by setup page: connect to WiFi
void onWifiConnect(const char* ssid, const char* pass) {
    wifiMgr.connectSTA(ssid, pass);
    saveSettings(); // Save STA credentials immediately
}

// Called by setup page: finalize mode selection
void onSetup(int mode, const char* apSSID, const char* apPass) {
    currentMode = (WiFiOpMode)mode;
    setupDone = true;

    // Update AP credentials if provided
    if (strlen(apSSID) > 0) {
        wifiMgr.setAPCredentials(apSSID, apPass);
    }

    // Save to Non-Volatile Storage so configuration survives reboots!
    saveSettings();

    // Apply WiFi mode
    wifiMgr.applyMode(currentMode);

    // Update dashboard state
    dashboard.setSetupDone(true);
    dashboard.setMode(currentMode);

    Serial.printf("[SETUP] Mode %d applied. Setup complete.\n", mode);
}

// ═══════════════════════════════════════════════════════════════════
//  BUZZER — non-blocking alert sound
// ═══════════════════════════════════════════════════════════════════
void handleBuzzer() {
    if (!registry.hasActiveAlerts()) {
        if (buzzerState) {
            digitalWrite(BUZZER_PIN, LOW);
            buzzerState = false;
        }
        return;
    }

    unsigned long now = millis();
    if (now - lastBuzzerToggle > 3000) { // Beep every 3 seconds
        lastBuzzerToggle = now;
        digitalWrite(BUZZER_PIN, HIGH);
        buzzerState = true;
    }
    // Turn off after 100ms beep
    if (buzzerState && (now - lastBuzzerToggle > 100)) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerState = false;
    }
}
