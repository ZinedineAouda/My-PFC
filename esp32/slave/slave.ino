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
#include <ESP8266mDNS.h>
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
String topicDiscovery; // Global discovery/migration topic

// ─── Config Persistence ─────────────────────────────────────────────
struct SlaveConfig {
    uint32_t magic;
    bool setupDone;
    bool approved; // Persistent approval status
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
    EEPROM.begin(512); // Use standard 512 byte sector
    SlaveConfig cfg;
    EEPROM.get(0, cfg);
    if (cfg.magic == CONFIG_MAGIC) {
        setupDone = cfg.setupDone;
        isApproved = cfg.approved;
        strncpy(wifiSSID, cfg.ssid, sizeof(wifiSSID));
        strncpy(wifiPass, cfg.pass, sizeof(wifiPass));
        strncpy(mqttIP, cfg.mqtt, sizeof(mqttIP));
        Serial.println("[EEPROM] Loaded saved configuration");
    } else {
        Serial.println("[EEPROM] No valid configuration found");
    }
}

void saveSlaveConfig() {
    SlaveConfig cfg;
    cfg.magic = CONFIG_MAGIC;
    cfg.setupDone = setupDone;
    cfg.approved = isApproved;
    strncpy(cfg.ssid, wifiSSID, sizeof(cfg.ssid));
    strncpy(cfg.pass, wifiPass, sizeof(cfg.pass));
    strncpy(cfg.mqtt, mqttIP, sizeof(cfg.mqtt));
    
    EEPROM.begin(512);
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
        
        if (isApproved != wasApproved) {
            saveSlaveConfig(); // Persist the new status
            if (isApproved) {
                Serial.println("[STATUS] Device APPROVED by admin");
                ledBlink(3, 80, 80); 
            } else {
                Serial.println("[STATUS] Approval revoked");
            }
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
        } else if (strcmp(action, "RESET_UNIT") == 0) {
            Serial.println("[CMD] FACTORY RESET REQUESTED");
            ledBlink(10, 50, 50); // Warning blink
            SlaveConfig cfg;
            cfg.magic = 0; // Invalidate config
            EEPROM.begin(sizeof(SlaveConfig));
            EEPROM.put(0, cfg);
            EEPROM.commit();
            delay(500);
            ESP.restart();
        }
    }

    // ── DISCOVERY: Handle master migration/provisioning ─────
    if (t == DISCOVERY_TOPIC) {
        const char* type = doc["type"] | "";
        if (strcmp(type, "MIGRATION") == 0) {
            const char* newSsid = doc["ssid"] | "";
            const char* newPass = doc["pass"] | "";
            const char* newIp   = doc["ip"]   | MQTT_BROKER_IP;

            if (strlen(newSsid) > 0) {
                Serial.printf("[MIGRATE] Received new Master details: %s\n", newSsid);
                strncpy(wifiSSID, newSsid, sizeof(wifiSSID));
                strncpy(wifiPass, newPass, sizeof(wifiPass));
                strncpy(mqttIP,   newIp,   sizeof(mqttIP));
                
                // Save and restart networking
                saveSlaveConfig();
                ledBlink(5, 50, 50); // Visual cue for migration
                
                WiFi.disconnect();
                WiFi.begin(wifiSSID, wifiPass);
                Serial.println("[MIGRATE] Reconnecting to new network...");
            }
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
    
    // ── mDNS Resolution Fallback ───────────────────────────
    String targetIP = String(mqttIP);
    if (targetIP.endsWith(".local")) {
        // Resolve hostname to IP
        if (!MDNS.begin(deviceId.c_str())) {
            Serial.println("[MDNS] Failed to start resolver");
        } else {
            Serial.printf("[MDNS] Querying %s...\n", mqttIP);
            int n = MDNS.queryService("mqtt", "tcp"); // Look for the service
            if (n > 0) {
                targetIP = MDNS.IP(0).toString();
                Serial.printf("[MDNS] Service found at: %s\n", targetIP.c_str());
            } else {
                // Fallback to host query
                IPAddress res;
                if (WiFi.hostByName(mqttIP, res)) {
                    targetIP = res.toString();
                    Serial.printf("[MDNS] Host resolved to: %s\n", targetIP.c_str());
                } else {
                    Serial.println("[MDNS] Resolution FAILED");
                }
            }
        }
    }

    mqtt.setServer(targetIP.c_str(), MQTT_BROKER_PORT);
    mqtt.setCallback(mqttCallback);
    mqtt.setKeepAlive(MQTT_KEEPALIVE);
    mqtt.setBufferSize(512);

    if (mqtt.connect(deviceId.c_str())) {
        Serial.println("[MQTT] Connected!");

        // Subscribe to our topics
        mqtt.subscribe(topicStatus.c_str(), 1);  // QoS 1 for status
        mqtt.subscribe(topicCommand.c_str(), 1);  // QoS 1 for commands
        mqtt.subscribe(DISCOVERY_TOPIC, 1);       // QoS 1 for migration broadcasts

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

    // We now allow alerts even if not yet approved so that 
    // emergency signals are never lost.
    if (!isApproved) {
        Serial.println("[ALERT] Warning: device not yet approved (sending anyway)");
    }

    if (alertActive) {
        Serial.println("[ALERT] Already active (re-publishing for reliability)");
    }

    JsonDocument doc;
    doc["status"]    = "pressed";
    doc["deviceId"]  = deviceId;
    doc["timestamp"] = millis();
    String payload;
    serializeJson(doc, payload);

    // Localized cooldown: prevent re-triggering within 100ms of a remote clear
    if (lastClearTime > 0 && millis() - lastClearTime < 100) {
        Serial.println("[ALERT] Suppressed: recently cleared (fast re-arm)");
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

    // Premium setup/status page (Mirroring Master UX/UI)
    setupServer.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        String html = R"rawliteral(<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Patient Unit</title><style>@import url('https://fonts.googleapis.com/css2?family=Outfit:wght@400;500;700;800&display=swap');:root{--pri:#2563eb;--bg:#f8fafc;--card:#ffffff;--text:#0f172a;--muted:#64748b;--danger:#dc2626;--ok:#10b981}*{margin:0;padding:0;box-sizing:border-box;font-family:'Outfit',sans-serif}body{background:var(--bg);color:var(--text);min-height:100vh}.header{background:var(--pri);padding:40px;color:#fff;position:relative;overflow:hidden}.header-bg{position:absolute;top:-20px;right:-20px;opacity:0.1;transform:rotate(45deg)}.header-content{position:relative;z-index:10;display:flex;items-center;gap:16px}.icon-box{width:48px;height:48px;background:rgba(255,255,255,0.2);backdrop-filter:blur(8px);border-radius:16px;display:flex;align-items:center;justify-content:center}.header h1{font-size:24px;font-weight:700}.header p{font-size:10px;font-weight:700;text-transform:uppercase;letter-spacing:3px;opacity:0.7}.content{padding:32px;max-width:500px;margin:0 auto}.card{background:var(--card);border-radius:40px;padding:40px;box-shadow:0 40px 80px -15px rgba(0,0,0,0.05)}.status-card{text-align:center;padding:60px 20px}.status-icon{width:80px;height:80px;border-radius:30px;display:flex;align-items:center;justify-content:center;margin:0 auto 24px}.status-ok{background:rgba(16,185,129,0.1);color:var(--ok)}.status-help{background:rgba(220,38,38,0.1);color:var(--danger);animation:pulse 2s infinite}@keyframes pulse{0%{transform:scale(1)}50%{transform:scale(1.05)}100%{transform:scale(1)}}.status-label{font-size:40px;font-weight:700;margin-bottom:8px}.status-sub{font-size:12px;color:var(--muted);font-weight:700;text-transform:uppercase;letter-spacing:2px}.field{margin-bottom:24px}label{display:block;font-size:10px;font-weight:700;text-transform:uppercase;letter-spacing:2px;color:var(--muted);margin-bottom:8px}input{width:100%;padding:20px;background:var(--bg);border:2px solid transparent;border-radius:24px;font-size:16px;font-weight:700;outline:none}input:focus{background:#fff;border-color:var(--pri)}button{width:100%;padding:20px;background:#0f172a;color:#fff;border:none;border-radius:24px;font-size:12px;font-weight:700;text-transform:uppercase;letter-spacing:3px;cursor:pointer}#msg{margin-top:24px;text-align:center;font-size:12px;font-weight:800;color:var(--ok)}</style></head><body><div class="header"><div class="header-bg"><svg width="120" height="120" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/></svg></div><div class="header-content"><div class="icon-box"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg></div><div><p>System</p><h1>Patient Unit</h1></div></div></div><div class="content"><div id="main-card" class="card"><div id="loader" style="text-align:center;padding:40px;color:var(--muted);font-weight:800;text-transform:uppercase;letter-spacing:2px">Loading...</div></div></div><script>
function updateUI(d){const c=document.getElementById('main-card');if(!d.connected||!d.approved){c.innerHTML=`<h2 style="font-size:24px;font-weight:700;margin-bottom:8px">Setup Unit</h2><p style="color:var(--muted);font-size:13px;margin-bottom:32px;font-weight:500">Link this device to the Main Unit.</p><div class="field"><label>Main Station WiFi</label><input id="ssid" value="HospitalAlarm"></div><div class="field"><label>WiFi Password</label><input id="pass" type="password" placeholder="If any"></div><div class="field"><label>Main Unit Address</label><input id="mqtt" value="hospital-alarm.local"></div><button onclick="save()">Connect Now</button><div id="msg"></div>`;}else{fetch('/api/live-status').then(r=>r.json()).then(st=>{const isA=st.alert;c.innerHTML=`<div class="status-card"><div class="status-icon ${isA?'status-help':'status-ok'}"><svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3"><path d="${isA?'M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0zM12 9v4M12 17h.01':'M22 11.08V12a10 10 0 1 1-5.93-9.14M22 4L12 14.01l-3-3'}"/></svg></div><div class="status-label">${isA?'HELPING':'MONITORING'}</div><div class="status-sub">${isA?'Pull handle or push button':'Device is active'}</div><div style="margin-top:32px;padding-top:32px;border-top:1px solid #f1f5f9"><p style="font-size:9px;font-weight:700;color:var(--muted);text-transform:uppercase;letter-spacing:1px">Unit ID: ${d.id}</p></div></div>`;});}}function save(){const d={ssid:document.getElementById('ssid').value,password:document.getElementById('pass').value,mqtt:document.getElementById('mqtt').value};document.querySelector('button').innerHTML='Connecting...';fetch('/api/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)}).then(r=>r.json()).then(r=>{document.getElementById('msg').innerHTML='Done! Reconnecting...';setTimeout(poll,3000);}).catch(()=>{document.getElementById('msg').innerHTML='Error.';});}function poll(){fetch('/api/status').then(r=>r.json()).then(updateUI).catch(e=>console.error(e));}poll();setInterval(poll,10000);</script></body></html>)rawliteral";
        req->send(200, "text/html", html);
    });

    setupServer.on("/api/live-status", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["alert"] = alertActive;
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
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

    // ── WiFi setup & Power Management ───────────────────────
    WiFi.persistent(false);
    
    // Switch to Light Sleep: allows the CPU/Radio to pause between beacons
    // This reduces idle current significantly without losing MQTT connection.
    WiFi.setSleepMode(WIFI_LIGHT_SLEEP);

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
        saveSlaveConfig();
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

    // ── Power Management ────────────────────────────────────
    // Small delay allows the ESP8266 SDK to enter Light Sleep
    // while waiting for the next beacon or interrupt.
    delay(10); 
    yield(); 
}
