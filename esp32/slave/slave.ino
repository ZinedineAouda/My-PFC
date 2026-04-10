/*
 * ══════════════════════════════════════════════════════════════════════
 *  Hospital Patient Alarm System — ESP8266 Slave
 * ══════════════════════════════════════════════════════════════════════
 *
 *  Communication: MQTT (PubSubClient) → Master's PicoMQTT broker
 *
 *  Topics Published:
 *    device/{id}/alert      — button pressed (QoS 1)
 *    device/{id}/heartbeat  — alive ping (QoS 0)
 *
 *  Topics Subscribed:
 *    device/{id}/status     — approval info from master (retained)
 *    device/{id}/command    — commands from master (clear alert, etc.)
 *
 *  Required Libraries (install via Arduino Library Manager):
 *    - PubSubClient        (MQTT client)
 *    - ArduinoJson         (JSON serialization)
 *    - ESPAsyncWebServer   (setup portal — optional)
 *    - ESPAsyncTCP         (for async web server)
 *
 *  Board: NodeMCU 1.0 / ESP-12E / ESP-01
 *
 * ══════════════════════════════════════════════════════════════════════
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "config.h"

// ─── Conditional: Setup portal (disable for minimal builds) ─────────
#if !USE_HARDCODED_WIFI
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#endif

// ─── Device Identity ────────────────────────────────────────────────
String deviceId;

// ─── MQTT Topics (built at runtime from deviceId) ───────────────────
String topicAlert;
String topicHeartbeat;
String topicStatus;
String topicCommand;

// ─── Config Persistence ─────────────────────────────────────────────
struct SlaveConfig {
    uint32_t magic;
    bool setupDone;
    char ssid[64];
    char pass[64];
    char mqtt[32];
};
const uint32_t CONFIG_MAGIC = 0xAA55CC33;

// ─── State ──────────────────────────────────────────────────────────
bool setupDone       = USE_HARDCODED_WIFI;
bool mqttConnected   = false;
bool isApproved      = false;
bool alertActive     = false;

volatile bool buttonPressed = false;
unsigned long lastButtonTime  = 0;
unsigned long lastHeartbeat   = 0;
unsigned long lastReconnect   = 0;
unsigned long alertLedTime    = 0;
unsigned long lastClearTime   = 0;  // Localized cooldown after master clear

// ─── WiFi Credentials ──────────────────────────────────────────────
char wifiSSID[64] = DEFAULT_WIFI_SSID;
char wifiPass[64] = DEFAULT_WIFI_PASS;
char mqttIP[32]   = MQTT_BROKER_IP;

void loadSlaveConfig() {
    EEPROM.begin(sizeof(SlaveConfig));
    SlaveConfig cfg;
    EEPROM.get(0, cfg);
    if (cfg.magic == CONFIG_MAGIC) {
        setupDone = cfg.setupDone;
        strncpy(wifiSSID, cfg.ssid, sizeof(wifiSSID));
        strncpy(wifiPass, cfg.pass, sizeof(wifiPass));
        strncpy(mqttIP, cfg.mqtt, sizeof(mqttIP));
        Serial.println("[EEPROM] Loaded saved configuration");
    }
}

void saveSlaveConfig() {
    SlaveConfig cfg;
    cfg.magic = CONFIG_MAGIC;
    cfg.setupDone = setupDone;
    strncpy(cfg.ssid, wifiSSID, sizeof(cfg.ssid));
    strncpy(cfg.pass, wifiPass, sizeof(cfg.pass));
    strncpy(cfg.mqtt, mqttIP, sizeof(cfg.mqtt));
    EEPROM.begin(sizeof(SlaveConfig));
    EEPROM.put(0, cfg);
    EEPROM.commit();
    Serial.println("[EEPROM] Configuration saved");
}

// ─── Network Objects ────────────────────────────────────────────────
WiFiClient   espClient;
PubSubClient mqtt(espClient);

// ─── Setup Portal (only when USE_HARDCODED_WIFI is false) ───────────
#if !USE_HARDCODED_WIFI
AsyncWebServer setupServer(80);
bool connectPending = false;
#endif

// ═══════════════════════════════════════════════════════════════════
//  INTERRUPT: Button press (hardware debounce via ISR)
// ═══════════════════════════════════════════════════════════════════
void IRAM_ATTR buttonISR() {
    unsigned long now = millis();
    if (now - lastButtonTime > DEBOUNCE_MS) {
        buttonPressed = true;
        lastButtonTime = now;
    }
}

// ═══════════════════════════════════════════════════════════════════
//  LED Feedback (non-blocking blink patterns)
// ═══════════════════════════════════════════════════════════════════
void ledBlink(int count, int onMs, int offMs) {
    for (int i = 0; i < count; i++) {
        digitalWrite(LED_PIN, LED_ON);
        delay(onMs);
        digitalWrite(LED_PIN, LED_OFF);
        if (i < count - 1) delay(offMs);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  MQTT Callback: handle incoming messages
// ═══════════════════════════════════════════════════════════════════
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Parse payload as JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        Serial.printf("[MQTT] JSON parse error: %s\n", err.c_str());
        return;
    }

    String t(topic);
    Serial.printf("[MQTT] Received on %s\n", topic);

    // ── Status update from master (approval info) ───────────
    if (t == topicStatus) {
        bool wasApproved = isApproved;
        isApproved = doc["approved"] | false;
        if (isApproved && !wasApproved) {
            Serial.println("[STATUS] Device APPROVED by admin");
            ledBlink(3, 80, 80); // Confirmation blink
        } else if (!isApproved && wasApproved) {
            Serial.println("[STATUS] Approval revoked");
        }
    }

    // ── Command from master ─────────────────────────────────
    if (t == topicCommand) {
        const char* action = doc["action"] | "";
        if (strcmp(action, "clear_alert") == 0) {
            alertActive = false;
            lastClearTime = millis();
            digitalWrite(LED_PIN, LED_OFF);
            Serial.println("[CMD] Alert cleared by master");
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
//  MQTT Connection (non-blocking)
// ═══════════════════════════════════════════════════════════════════
bool connectMQTT() {
    if (mqtt.connected()) return true;

    unsigned long now = millis();
    if (now - lastReconnect < RECONNECT_MS) return false;
    lastReconnect = now;

    Serial.printf("[MQTT] Connecting to %s:%d...\n", mqttIP, MQTT_BROKER_PORT);
    mqtt.setServer(mqttIP, MQTT_BROKER_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setKeepAlive(MQTT_KEEPALIVE);
    mqtt.setBufferSize(512);

    if (mqtt.connect(deviceId.c_str())) {
        Serial.println("[MQTT] Connected!");

        // Subscribe to our topics
        mqtt.subscribe(topicStatus.c_str(), 1);  // QoS 1 for status
        mqtt.subscribe(topicCommand.c_str(), 1);  // QoS 1 for commands

        // Send initial heartbeat (acts as registration)
        _publishHeartbeat();

        mqttConnected = true;
        ledBlink(2, 100, 100); // Connection confirmation
        return true;
    }

    Serial.printf("[MQTT] Failed, rc=%d\n", mqtt.state());
    mqttConnected = false;
    return false;
}

// ═══════════════════════════════════════════════════════════════════
//  MQTT Publish Helpers
// ═══════════════════════════════════════════════════════════════════
void _publishHeartbeat() {
    JsonDocument doc;
    doc["deviceId"] = deviceId;
    doc["uptime"]   = millis() / 1000;
    doc["rssi"]     = WiFi.RSSI();
    String payload;
    serializeJson(doc, payload);
    mqtt.publish(topicHeartbeat.c_str(), payload.c_str());
}

void publishAlert() {
    if (!mqtt.connected()) {
        Serial.println("[ALERT] Not connected to MQTT");
        ledBlink(4, 50, 50); // Error blink
        return;
    }

    if (!isApproved) {
        Serial.println("[ALERT] Not approved by admin");
        ledBlink(6, 80, 80); // "Waiting for approval" pattern
        return;
    }

    if (alertActive) {
        Serial.println("[ALERT] Already active");
        return;
    }

    JsonDocument doc;
    doc["status"]    = "pressed";
    doc["deviceId"]  = deviceId;
    doc["timestamp"] = millis();
    String payload;
    serializeJson(doc, payload);

    // Localized cooldown: prevent re-triggering within 2s of a remote clear
    if (millis() - lastClearTime < 2000) {
        Serial.println("[ALERT] Suppressed: recently cleared");
        return;
    }

    // Publish with QoS 0 (at-most-once delivery)
    // Note: PubSubClient publish() is QoS 0 by default.Master handles the state.
    if (mqtt.publish(topicAlert.c_str(), payload.c_str(), false)) {
        alertActive = true;
        alertLedTime = millis();
        digitalWrite(LED_PIN, LED_ON);
        Serial.println("[ALERT] Published successfully");
    } else {
        Serial.println("[ALERT] Publish failed");
        ledBlink(4, 50, 50);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  WiFi Connection (non-blocking)
// ═══════════════════════════════════════════════════════════════════
void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    static unsigned long lastTry = 0;
    unsigned long now = millis();
    if (now - lastTry < RECONNECT_MS) return;
    lastTry = now;

    Serial.printf("[WIFI] Connecting to %s...\n", wifiSSID);
    WiFi.begin(wifiSSID, wifiPass);
}

// ═══════════════════════════════════════════════════════════════════
//  SETUP PORTAL (optional web-based WiFi configuration)
// ═══════════════════════════════════════════════════════════════════
#if !USE_HARDCODED_WIFI
void startSetupPortal() {
    WiFi.mode(WIFI_AP);
    String apName = "Alarm-" + deviceId;
    WiFi.softAP(apName.c_str());
    Serial.printf("[SETUP] Portal AP: %s @ %s\n",
                  apName.c_str(), WiFi.softAPIP().toString().c_str());

    // Minimal setup page
    setupServer.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        String html = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Slave Setup</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:system-ui,sans-serif;background:#0a0e1a;color:#e2e8f0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.card{background:rgba(15,23,42,.85);border:1px solid rgba(255,255,255,.06);border-radius:24px;padding:36px;width:100%;max-width:420px}
h1{font-size:20px;text-align:center;margin-bottom:8px;font-weight:800}
.sub{text-align:center;font-size:12px;color:#64748b;margin-bottom:24px;font-family:monospace}
label{display:block;font-size:12px;color:#94a3b8;margin-bottom:6px;font-weight:600}
input{width:100%;padding:12px;background:rgba(0,0,0,.3);border:1px solid rgba(255,255,255,.06);border-radius:10px;color:#fff;margin-bottom:14px;outline:none;font-size:14px}
input:focus{border-color:#6366f1}
button{width:100%;padding:14px;background:linear-gradient(135deg,#6366f1,#7c3aed);color:#fff;border:none;border-radius:12px;font-size:14px;font-weight:700;cursor:pointer}
.msg{text-align:center;padding:12px;border-radius:10px;margin-top:12px;font-size:13px}
.ok{background:rgba(16,185,129,.1);color:#34d399}
.err{background:rgba(239,68,68,.1);color:#f87171}
</style></head><body>
<div class="card">
<h1>&#128276; Slave Setup</h1>
<div class="sub">)rawliteral" + deviceId + R"rawliteral(</div>
<label>WiFi Network (Master AP)</label>
<input id="ssid" value="HospitalAlarm" placeholder="e.g. HospitalAlarm">
<label>Password</label>
<input id="pass" type="password" placeholder="Leave empty for open network">
<label>Master MQTT IP</label>
<input id="mqtt" value="192.168.4.1" placeholder="192.168.4.1">
<button onclick="save()">Connect</button>
<div id="msg"></div>
</div>
<script>
function save(){
  const d={ssid:document.getElementById('ssid').value,password:document.getElementById('pass').value,mqtt:document.getElementById('mqtt').value};
  fetch('/api/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)})
  .then(r=>r.json()).then(r=>{document.getElementById('msg').innerHTML='<div class="msg ok">Connecting... Device will restart.</div>'})
  .catch(()=>{document.getElementById('msg').innerHTML='<div class="msg err">Error</div>'});
}
</script></body></html>)rawliteral";
        req->send(200, "text/html", html);
    });

    setupServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["id"]        = deviceId;
        doc["connected"] = (WiFi.status() == WL_CONNECTED);
        doc["mqtt"]      = mqttConnected;
        doc["approved"]  = isApproved;
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    // Handle connect request with body parser
    setupServer.on("/api/connect", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            JsonDocument doc;
            deserializeJson(doc, data, len);
            
            strncpy(wifiSSID, doc["ssid"] | "HospitalAlarm", sizeof(wifiSSID) - 1);
            wifiSSID[sizeof(wifiSSID) - 1] = '\0';
            
            strncpy(wifiPass, doc["password"] | "", sizeof(wifiPass) - 1);
            wifiPass[sizeof(wifiPass) - 1] = '\0';
            
            strncpy(mqttIP,   doc["mqtt"] | MQTT_BROKER_IP, sizeof(mqttIP) - 1);
            mqttIP[sizeof(mqttIP) - 1] = '\0';
            
            saveSlaveConfig();
            
            connectPending = true;
            req->send(200, "application/json", "{\"success\":true}");
        });

    setupServer.begin();
}
#endif

// ═══════════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(200);

    // ── Load Config ─────────────────────────────────────────
    loadSlaveConfig();

    // ── Hardware ────────────────────────────────────────────
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_OFF);

    // ── Generate unique device ID from chip ID ──────────────
    uint32_t chipId = ESP.getChipId();
    char idBuf[32];
    snprintf(idBuf, sizeof(idBuf), "slave-%06X", chipId & 0xFFFFFF);
    deviceId = String(idBuf);

    // ── Build MQTT topic strings ────────────────────────────
    topicAlert     = "device/" + deviceId + "/alert";
    topicHeartbeat = "device/" + deviceId + "/heartbeat";
    topicStatus    = "device/" + deviceId + "/status";
    topicCommand   = "device/" + deviceId + "/command";

    Serial.println();
    Serial.println("╔══════════════════════════════════════╗");
    Serial.println("║  Hospital Alarm — Slave Device       ║");
    Serial.println("╚══════════════════════════════════════╝");
    Serial.printf("  Device ID: %s\n", deviceId.c_str());
    Serial.printf("  Button:    GPIO%d\n", BUTTON_PIN);
    Serial.printf("  LED:       GPIO%d\n", LED_PIN);

    // ── Attach button interrupt ─────────────────────────────
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

    // ── WiFi setup ──────────────────────────────────────────
    WiFi.persistent(false);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    if (USE_HARDCODED_WIFI) {
        // Direct connection mode — no setup portal
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiSSID, wifiPass);
        Serial.printf("[WIFI] Connecting to %s (hardcoded)...\n", wifiSSID);
        setupDone = true;
    } else {
        // Start setup portal
        #if !USE_HARDCODED_WIFI
        startSetupPortal();
        #endif
    }

    // Boot-up LED pattern
    ledBlink(3, 100, 100);
    Serial.println("[BOOT] Ready");
}

// ═══════════════════════════════════════════════════════════════════
//  LOOP — fully non-blocking
// ═══════════════════════════════════════════════════════════════════
void loop() {

    // ── Handle setup portal connect request ─────────────────
    #if !USE_HARDCODED_WIFI
    if (connectPending) {
        connectPending = false;
        Serial.printf("[SETUP] Connecting to %s, MQTT broker: %s\n",
                      wifiSSID, mqttIP);

        // Switch from AP to STA
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiSSID, wifiPass);

        // Non-blocking: let the loop handle connection
        setupDone = true;
    }
    #endif

    // ── WiFi management ─────────────────────────────────────
    if (setupDone) {
        connectWiFi();

        if (WiFi.status() == WL_CONNECTED) {
            // ── MQTT connection + message processing ────────
            if (connectMQTT()) {
                mqtt.loop(); // Process incoming messages

                // ── Periodic heartbeat ──────────────────────
                unsigned long now = millis();
                if (now - lastHeartbeat > HEARTBEAT_MS) {
                    lastHeartbeat = now;
                    _publishHeartbeat();
                }
            }
        }
    }

    // ── Button press handling ───────────────────────────────
    if (buttonPressed) {
        buttonPressed = false;

        if (!setupDone) {
            Serial.println("[BTN] Setup not complete");
            ledBlink(3, 200, 200);
        } else if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[BTN] WiFi not connected");
            ledBlink(2, 300, 300);
        } else if (!mqtt.connected()) {
            Serial.println("[BTN] MQTT not connected");
            ledBlink(4, 100, 100);
        } else {
            publishAlert();
        }
    }

    // ── Alert LED auto-off ──────────────────────────────────
    if (alertActive && (millis() - alertLedTime > ALERT_LED_MS)) {
        alertActive = false;
        digitalWrite(LED_PIN, LED_OFF);
        Serial.println("[ALERT] LED timeout, turning off");
    }

    yield(); // Critical for ESP8266 WiFi stack
}
