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
void onWifiConnect(const char* ssid, const char* pass);
void onSetup(int mode, const char* apSSID, const char* apPass);
void onReSetup();
void onFactoryReset();
void handleBuzzer();

void checkNewFlash() {
    String currentBuild = String(__DATE__) + " " + String(__TIME__);
    prefs.begin("sys_info", false);
    String lastBuild = prefs.getString("build_id", "");
    
    if (lastBuild.length() > 0 && lastBuild != currentBuild) {
        Serial.println("╔═════════════════════════════════════════════════════╗");
        Serial.println("║  [NEW FLASH] New firmware detected!                 ║");
        Serial.println("║  Performing automatic factory reset to ensure clean  ║");
        Serial.println("║  setup as requested.                                ║");
        Serial.println("╚═════════════════════════════════════════════════════╝");
        prefs.putString("build_id", currentBuild);
        prefs.end();
        onFactoryReset(); // This restarts the device
    }
    
    prefs.putString("build_id", currentBuild);
    prefs.end();
}

void loadSettings() {
    prefs.begin("master_cfg", true); // Read-only mode
    setupDone = prefs.getBool("setupDone", false);
    currentMode = (WiFiOpMode)prefs.getInt("mode", (int)MODE_AP);
    
    if (setupDone) {
        String apS = prefs.getString("apSSID", AP_SSID_DEFAULT);
        String apP = prefs.getString("apPass", "");
        String staS = prefs.getString("staSSID", "");
        String staP = prefs.getString("staPass", "");
        
        wifiMgr.setAPCredentials(apS.c_str(), apP.c_str());
        if (staS.length() > 0) {
            wifiMgr.setSTACredentials(staS.c_str(), staP.c_str());
        }
        Serial.printf("[PREFS] Configuration Loaded: Mode=%d, SSID=%s\n", (int)currentMode, apS.c_str());
    } else {
        Serial.println("[PREFS] No valid configuration found. Entering Setup Mode.");
    }
    prefs.end();
}

void saveSettings() {
    prefs.begin("master_cfg", false); // Read-write mode
    prefs.putBool("setupDone", setupDone);
    prefs.putInt("mode", (int)currentMode);
    prefs.putString("apSSID", wifiMgr.apSSID());
    prefs.putString("apPass", wifiMgr.apPass());
    prefs.putString("staSSID", wifiMgr.staSSID());
    prefs.putString("staPass", wifiMgr.staPass());
    prefs.end();
    Serial.println("[PREFS] Configuration successfully committed to NVS.");
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

    // ── Check if code was just flashed ─────────────────────
    checkNewFlash();

    // ── Load Config & WiFi ──────────────────────────────────
    loadSettings();
    WiFi.setSleep(WIFI_PS_NONE); // Disable power saving for maximum responsiveness
    WiFi.setTxPower(WIFI_POWER_20dBm); // Force maximum transmission power
    if (!setupDone) {
        wifiMgr.beginSetup();
        WiFi.scanNetworks(true); // Start background scan
    } else {
        wifiMgr.applyMode(currentMode);
    }

    // ─── Initialize Registry ───
    registry.begin();
    registry.onDeviceChange(onDeviceChange);

    // ── MQTT Broker: start on port 1883 ─────────────────────
    mqttHandler.begin();

    // ── Web Dashboard: start with setup page ────────────────
    dashboard.onConnect(onWifiConnect);
    dashboard.onSetup(onSetup);
    dashboard.onReSetup(onReSetup);
    dashboard.onReset(onFactoryReset);
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

    // ── Local hardware feedback (Buzzer/LED) ───────────────
    handleHardwareFeedback();

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
        if (strcmp(eventType, "alert") == 0) {
            cloudSync.alertToCloud(deviceId);
        } else if (strcmp(eventType, "approve") == 0 ||
                   strcmp(eventType, "clear") == 0 ||
                   strcmp(eventType, "delete") == 0) {
            cloudSync.syncNow();
        }
    }
}

// ─── Hardware Feedback Logic ────────────────────────────────────────
void handleHardwareFeedback() {
    static unsigned long lastToggle = 0;
    static bool ledState = false;
    unsigned long now = millis();

    // 1. Check if any device has an active alert
    bool anyAlert = false;
    for (auto const& [id, dev] : registry.devices()) {
        if (dev.alertActive) {
            anyAlert = true;
            break;
        }
    }

    // 2. Drive Buzzer
    digitalWrite(BUZZER_PIN, anyAlert ? HIGH : LOW);

    // 3. Drive Status LED
    if (anyAlert) {
        // Fast blink during emergency
        if (now - lastToggle > 150) {
            lastToggle = now;
            ledState = !ledState;
            digitalWrite(STATUS_LED_PIN, ledState);
        }
    } else if (!setupDone) {
        // Solid for unconfigured AP mode
        digitalWrite(STATUS_LED_PIN, HIGH);
    } else {
        // Slow pulsing heartbeat for normal operation
        if (now - lastToggle > 1000) {
            lastToggle = now;
            ledState = !ledState;
            digitalWrite(STATUS_LED_PIN, ledState);
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

// Called by dashboard: entering setup mode again (soft reset)
void onReSetup() {
    Serial.println("[RESET] Re-setup requested. Clearing WiFi config...");
    prefs.begin("master_cfg", false);
    prefs.clear();
    prefs.end();
    
    Serial.println("[RESET] Restarting into Setup Wizard...");
    delay(1000);
    ESP.restart();
}

// Called by setup or admin page: full wipe
void onFactoryReset() {
    Serial.println("[RESET] Factory reset triggered! Wiping all data...");
    
    // 1. Clear Master Config (WiFi, Mode)
    Preferences pcfg;
    pcfg.begin("master_cfg", false);
    pcfg.clear();
    pcfg.end();
    
    // 2. Clear Device Registry
    registry.factoryReset();
    
    Serial.println("[RESET] Data wiped. Restarting...");
    delay(1000);
    ESP.restart();
}
