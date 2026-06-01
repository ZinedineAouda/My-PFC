/*
 * ══════════════════════════════════════════════════════════════
 *  Cloud Sync — MQTT-based sync with Railway server (Mode 4)
 *  Optimised: pre-cached topics, QoS 1 alerts, heap-safe
 * ══════════════════════════════════════════════════════════════
 */
#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PicoMQTT.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"
#include "device_registry.h"

class CloudSync {
public:
    typedef void (*CloudCommandCallback)(const String& cmd, const String& params);

    CloudSync(DeviceRegistry& reg)
        : _registry(reg), _lastSync(0), _lastPing(0), _lastOpTime(0),
          _busy(false), _forceSyncPending(false), _initialized(false), _connected(false),
          _plainClient(nullptr), _secureClient(nullptr), _mqtt(nullptr) {}

    void onCommand(CloudCommandCallback cb) { _commandCallback = cb; }
    void setTimeout(unsigned long) {} // No-op: stub kept for API compatibility only

    ~CloudSync() {
        if (_mqtt)         delete _mqtt;
        if (_plainClient)  delete _plainClient;
        if (_secureClient) delete _secureClient;
    }

    void handle(int currentMode, String wifiError = "OK") {
        if (WiFi.status() != WL_CONNECTED) return;

        // One-time init: build topics and start MQTT client
        if (!_initialized) {
            _initMqtt();
            _initialized = true;
        }

        // PicoMQTT handles reconnection automatically via loop()
        _mqtt->loop();
        if (_busy) return;

        _currentMode  = currentMode;
        _lastWifiError = wifiError;
        unsigned long now = millis();

        // ── Priority 1: Flush alert queue — no throttle ─────────────────
        if (_connected && !_alertQueue.empty()) {
            _processAlertQueue();
            _lastOpTime = now;
            return;
        }

        // ── Global throttle: max 1 background op per second ─────────────
        if (now - _lastOpTime < 1000) return;

        bool hasAlerts = _registry.alertCount() > 0;

        // ── Priority 2: Lightweight ping every 60s ───────────────────────
        if (_connected && !hasAlerts && (now - _lastPing >= CLOUD_PING_INTERVAL)) {
            _lastPing   = now;
            _lastOpTime = now;
            _sendPing();
            return;
        }

        // ── Priority 3: Full bidirectional state sync every 30s ──────────
        if (_connected && (_forceSyncPending || (!hasAlerts && (now - _lastSync >= CLOUD_SYNC_INTERVAL)))) {
            _forceSyncPending = false;
            _lastSync   = now;
            _lastOpTime = now;
            _syncToCloud();
        }
    }

    // Trigger an immediate sync on the next loop() iteration
    void syncNow() { _forceSyncPending = true; }

    // Queue an alert for cloud delivery (guaranteed order, capped at 20)
    void alertToCloud(const String& deviceId) {
        for (const auto& id : _alertQueue) {
            if (id == deviceId) return; // De-duplicate
        }
        if (_alertQueue.size() >= 20) {
            Serial.println("[CLOUD] Alert queue FULL — dropping oldest");
            _alertQueue.erase(_alertQueue.begin());
        }
        _alertQueue.push_back(deviceId);
        Serial.printf("[CLOUD] Alert queued: %s (queue: %d)\n",
                      deviceId.c_str(), _alertQueue.size());
    }

private:

    // ── One-time initialisation ──────────────────────────────────────────
    void _initMqtt() {
        Serial.printf("[CLOUD] Initialising MQTT → %s:%d\n",
                      CLOUD_MQTT_HOST, CLOUD_MQTT_PORT);

        // Pre-cache all topics once — prevents repeated String heap allocs
        // on every publish() call (important for ESP32-S3 heap health)
        String key   = CLOUD_DEVICE_KEY;
        _topicPing    = "controller/" + key + "/ping";
        _topicSync    = "controller/" + key + "/sync";
        _topicAlert   = "controller/" + key + "/alert";
        _topicCommand = "controller/" + key + "/command";
        _topicSyncRes = "controller/" + key + "/sync_response";

        // TLS (MQTTS port 8883) or plain (port 1883)
        if (CLOUD_MQTT_PORT == 8883) {
            _secureClient = new WiFiClientSecure();
            _secureClient->setInsecure(); // Skip cert verification (suitable for Railway self-signed)
            _mqtt = new PicoMQTT::Client(*_secureClient, CLOUD_MQTT_HOST, CLOUD_MQTT_PORT, key.c_str());
        } else {
            _plainClient = new WiFiClient();
            _mqtt = new PicoMQTT::Client(*_plainClient, CLOUD_MQTT_HOST, CLOUD_MQTT_PORT, key.c_str());
        }

        if (strlen(CLOUD_MQTT_USER) > 0) {
            _mqtt->username = CLOUD_MQTT_USER;
            _mqtt->password = CLOUD_MQTT_PASS;
        }

        // ── Connection lifecycle callbacks ───────────────────────────────
        _mqtt->connected_callback = [this]() {
            Serial.println("[CLOUD] MQTT connected ✓");
            _connected = true;
            _forceSyncPending = true; // Push state to cloud immediately on (re)connect
        };

        _mqtt->disconnected_callback = [this]() {
            Serial.println("[CLOUD] MQTT disconnected — PicoMQTT will auto-retry");
            _connected = false;
        };

        // ── Subscribe: admin commands from cloud dashboard ───────────────
        _mqtt->subscribe(_topicCommand.c_str(), [this](const char* topic, const char* payload) {
            JsonDocument doc;
            if (deserializeJson(doc, payload) != DeserializationError::Ok) return;
            String cmd    = doc["command"] | "";
            String params = doc["params"]  | "";
            Serial.printf("[CLOUD] Command ← %s\n", cmd.c_str());
            if (cmd == "SYNC_NOW") {
                _forceSyncPending = true; // Handled internally, not forwarded
            } else if (_commandCallback) {
                _commandCallback(cmd, params);
            }
        });

        // ── Subscribe: authoritative device list from cloud ──────────────
        _mqtt->subscribe(_topicSyncRes.c_str(), [this](const char* topic, const char* payload) {
            JsonDocument doc;
            if (deserializeJson(doc, payload) != DeserializationError::Ok) return;
            JsonArray arr = doc["devices"].as<JsonArray>();
            if (!arr.isNull()) {
                _registry.mergeFromCloud(arr);
                Serial.printf("[CLOUD] Sync response applied: %d devices\n", arr.size());
            }
        });

        _mqtt->begin();
    }

    // ── Send the first alert in the queue ────────────────────────────────
    void _processAlertQueue() {
        if (_alertQueue.empty() || !_connected) return;
        const String& deviceId = _alertQueue[0];
        _sendAlertToCloud(deviceId);
        _alertQueue.erase(_alertQueue.begin());
    }

    // ── Lightweight controller heartbeat (QoS 0 — loss acceptable) ──────
    void _sendPing() {
        JsonDocument doc;
        doc["mode"]      = _currentMode;
        doc["uptime"]    = millis() / 1000;
        doc["rssi"]      = WiFi.RSSI();
        doc["wifiError"] = _lastWifiError;
        String payload;
        serializeJson(doc, payload);
        _mqtt->publish(_topicPing.c_str(), payload.c_str(), 0, false);
        Serial.printf("[CLOUD] Ping ↑ (uptime=%lus rssi=%ddBm)\n",
                      millis() / 1000, (int)WiFi.RSSI());
    }

    // ── Alert publish (QoS 1 — delivery guaranteed) ──────────────────────
    void _sendAlertToCloud(const String& deviceId) {
        JsonDocument doc;
        doc["deviceId"] = deviceId;
        String payload;
        serializeJson(doc, payload);
        _mqtt->publish(_topicAlert.c_str(), payload.c_str(), 1, false);
        Serial.printf("[CLOUD] Alert ↑ %s\n", deviceId.c_str());
    }

    // ── Full bidirectional state sync (QoS 1) ────────────────────────────
    void _syncToCloud() {
        _busy = true;

        JsonDocument sendDoc;
        sendDoc["mode"]      = _currentMode;
        sendDoc["uptime"]    = millis() / 1000;
        sendDoc["rssi"]      = WiFi.RSSI();
        sendDoc["wifiError"] = _lastWifiError;

        JsonArray arr = sendDoc["devices"].to<JsonArray>();
        for (auto& kv : _registry.devices()) {
            const PatientDevice& d = kv.second;
            JsonObject obj = arr.add<JsonObject>();
            obj["deviceId"]    = d.deviceId;
            obj["patientName"] = d.patientName;
            obj["bed"]         = d.bed;
            obj["room"]        = d.room;
            obj["alertActive"] = d.alertActive;
            obj["approved"]    = d.approved;
            obj["online"]      = d.online;
            if (d.lastUpdatedAt > 0) obj["lastUpdatedAt"] = d.lastUpdatedAt;
        }

        String payload;
        serializeJson(sendDoc, payload);
        _mqtt->publish(_topicSync.c_str(), payload.c_str(), 1, false);
        Serial.printf("[CLOUD] Sync ↑ (%d devices, %d bytes)\n",
                      (int)_registry.count(), payload.length());

        _busy = false;
    }

// ── Member variables ─────────────────────────────────────────────────────────
private:
    DeviceRegistry&       _registry;
    unsigned long         _lastSync;
    unsigned long         _lastPing;
    unsigned long         _lastOpTime;
    bool                  _busy;
    bool                  _forceSyncPending;
    bool                  _initialized;
    bool                  _connected;
    int                   _currentMode   = 1;
    String                _lastWifiError = "OK";

    // Pre-cached topics — set once in _initMqtt(), reused on every publish()
    String _topicPing;
    String _topicSync;
    String _topicAlert;
    String _topicCommand;
    String _topicSyncRes;

    WiFiClient*          _plainClient;
    WiFiClientSecure*    _secureClient;
    PicoMQTT::Client*    _mqtt;
    std::vector<String>  _alertQueue;
    CloudCommandCallback _commandCallback = nullptr;
};

#endif // CLOUD_SYNC_H
