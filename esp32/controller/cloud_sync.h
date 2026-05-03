/*
 * ══════════════════════════════════════════════════════════════
 *  Cloud Sync — HTTPS-based sync with Railway server (Mode 4)
 *  Persistent TLS session for reliable connectivity
 * ══════════════════════════════════════════════════════════════
 */
#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"
#include "device_registry.h"

class CloudSync {
public:
    typedef void (*CloudCommandCallback)(const String& cmd, const String& params);
    
    CloudSync(DeviceRegistry& reg)
        : _registry(reg), _lastSync(0), _lastPing(0), _lastOpTime(0),
          _busy(false), _forceSyncPending(false),
          _consecutiveFails(0), _timeOffset(0) {
        _client = new WiFiClientSecure();
        _client->setInsecure();
        _client->setHandshakeTimeout(30000);
        _http = new HTTPClient();
        _http->setTimeout(3000); // Drastically reduced to prevent Controller loop blocking (critical for MQTT server)
    }

    void onCommand(CloudCommandCallback cb) { _commandCallback = cb; }

    void setTimeout(unsigned long timeoutMs) {
        if (_http) _http->setTimeout(timeoutMs);
    }

    ~CloudSync() {
        if (_http) {
            _http->end();
            delete _http;
        }
        if (_client) {
            _client->stop();
            delete _client;
        }
    }

    void handle(int currentMode, String wifiError = "OK") {
        if (WiFi.status() != WL_CONNECTED) return;
        if (_busy) return;

        _currentMode = currentMode;
        _lastWifiError = wifiError;
        unsigned long now = millis();
        
        // ── 1. Process pending alert queue (highest priority — NO THROTTLE) ─
        if (!_alertQueue.empty()) {
            _processAlertQueue();
            _lastOpTime = now; 
            delay(100); // Settle network stack after critical TX
            return;
        }

        // ── 2. Background throttled tasks ────────────────────
        if (now - _lastOpTime < 1000) return; 

        // ── 3. Lightweight heartbeat ping every 10s ──────────
        if (now - _lastPing >= CLOUD_PING_INTERVAL) {
            _lastPing = now;
            _lastOpTime = now;
            _sendPing();
            return;
        }

        // ── 4. Full state sync ───────────────────────────────
        if (_forceSyncPending || (now - _lastSync >= CLOUD_SYNC_INTERVAL)) {
            _forceSyncPending = false;
            _lastSync = now;
            _lastOpTime = now;
            _syncToCloud();
            return;
        }
    }

    // Force an immediate sync
    // Non-blocking: flags for execution on the next loop iteration
    void syncNow() {
        _forceSyncPending = true;
    }

    // Send a specific alert to the cloud (called when device triggers alert)
    void alertToCloud(const String& deviceId) {
        // Only queue if not already pending to avoid spamming
        for (const auto& id : _alertQueue) {
            if (id == deviceId) return;
        }
        // Memory safety: limit queue size to 20 pending alerts
        if (_alertQueue.size() >= 20) {
            Serial.println("[CLOUD] Queue FULL, dropping oldest alert");
            _alertQueue.erase(_alertQueue.begin());
        }
        _alertQueue.push_back(deviceId);
        Serial.printf("[CLOUD] Alert queued: %s (Queue size: %d)\n", 
                      deviceId.c_str(), _alertQueue.size());
    }

private:

    uint64_t _getCurrentTime() {
        return _timeOffset > 0 ? (millis() + _timeOffset) : 0;
    }

    void _processAlertQueue() {
        if (_alertQueue.empty()) return;

        String deviceId = _alertQueue[0];
        if (_sendAlertToCloud(deviceId)) {
            _alertQueue.erase(_alertQueue.begin());
            _consecutiveFails = 0;
        } else {
            // Move to back of queue to allow other alerts to try
            _alertQueue.erase(_alertQueue.begin());
            _alertQueue.push_back(deviceId);
            _handleFailure();
        }
    }

    // ── Ensure persistent TLS client exists ─────────────────

    // ── Reset the TLS client (after repeated failures) ──────
    void _resetClient() {
        if (_http) _http->end();
        if (_client) _client->stop();
        Serial.println("[CLOUD] Persistent TLS reset");
    }

    // ── Internal POST helper with auto-recovery for -1 ──────
    int _postWithRetry(const String& path, const String& payload) {
        int code = _executePost(path, payload);
        
        // If it failed with a transport error (negative codes like -1, -5, etc), reset and retry
        if (code < 0) {
            Serial.printf("[CLOUD] Transport error %d. Resetting for retry...\n", code);
            _resetClient();
            delay(500); 
            code = _executePost(path, payload);
        }
        return code;
    }

    int _executePost(const String& path, const String& payload) {
        if (!_client || !_http) return -3;

        String url = String(CLOUD_SERVER_URL) + path;
        Serial.printf("[CLOUD] Requesting: %s\n", url.c_str());
        
        _http->setReuse(true); 
        if (!_http->begin(*_client, url)) {
            Serial.println("[CLOUD] Failed to init HTTPClient");
            return -2;
        }

        _http->addHeader("Content-Type", "application/json");
        _http->addHeader("x-device-key", CLOUD_DEVICE_KEY);
        _http->addHeader("Connection", "keep-alive"); 
        
        unsigned long startReq = millis();
        int code = _http->POST(payload);
        unsigned long dur = millis() - startReq;
        
        if (code < 0) {
            Serial.printf("[CLOUD] Request failed (%d) after %lums\n", code, dur);
            _resetClient(); 
        } else if (code != 200) {
            String errorBody = _http->getString();
            Serial.printf("[CLOUD] Error Response (%d): %s\n", code, errorBody.c_str());
        } else {
            // Success — Process Response
            String response = _http->getString();
            JsonDocument recvDoc;
            if (deserializeJson(recvDoc, response) == DeserializationError::Ok) {
                // Time Synchronization
                if (!recvDoc["serverTime"].isNull()) {
                    uint64_t sTime = recvDoc["serverTime"].as<uint64_t>();
                    _timeOffset = sTime - (uint64_t)millis();
                    _registry.updateTimeOffset(_timeOffset);
                }

                if (path == "/api/controller-sync") {
                    // 1. Merge Device Updates
                    JsonArray remoteDevices = recvDoc["devices"];
                    if (!remoteDevices.isNull()) {
                        _registry.mergeFromCloud(remoteDevices);
                    }

                    // 2. Listen for Remote Commands
                    if (!recvDoc["command"].isNull()) {
                        String cmd = recvDoc["command"].as<String>();
                        String params = recvDoc["params"] | "";
                        Serial.printf("[CLOUD] Remote Cmd: %s\n", cmd.c_str());
                        if (_commandCallback) _commandCallback(cmd, params);
                    }
                }
            }
        }
        
        // Note: NO http->end() here to keep socket open
        return code;
    }

    // ── Lightweight ping ────────────────────────────────────
    void _sendPing() {
        JsonDocument doc;
        doc["mode"] = _currentMode;
        doc["uptime"] = millis() / 1000;
        doc["rssi"] = WiFi.RSSI();
        doc["wifiError"] = _lastWifiError;
        String payload;
        serializeJson(doc, payload);

        int code = _postWithRetry("/api/controller-ping", payload);
        if (code == 200) {
            Serial.println("[CLOUD] Ping OK");
        } else {
            Serial.printf("[CLOUD] Ping failed: HTTP %d\n", code);
        }
    }

    // ── Send alert to cloud ────────────────────────────────
    bool _sendAlertToCloud(const String& deviceId) {
        String payload = "{\"deviceId\":\"" + deviceId + "\"}";
        int code = _postWithRetry("/api/alert", payload);

        if (code == 200) {
            Serial.printf("[CLOUD] Alert sent: %s\n", deviceId.c_str());
            return true;
        } else {
            Serial.printf("[CLOUD] Alert failed: HTTP %d\n", code);
            _handleFailure();
            return false;
        }
    }

    // ── Full bidirectional state sync ───────────────────────
    void _syncToCloud() {
        _busy = true;

        // ── Build sync payload ──────────────────────────────
        JsonDocument sendDoc;
        sendDoc["mode"] = _currentMode;
        sendDoc["uptime"] = millis() / 1000;
        sendDoc["rssi"] = WiFi.RSSI();
        sendDoc["wifiError"] = _lastWifiError;

        JsonArray arr = sendDoc["devices"].to<JsonArray>();
        for (auto& kv : _registry.devices()) {
            const PatientDevice& d = kv.second;
            JsonObject obj = arr.add<JsonObject>();
            obj["deviceId"]     = d.deviceId;
            obj["patientName"] = d.patientName;
            obj["bed"]         = d.bed;
            obj["room"]        = d.room;
            obj["alertActive"] = d.alertActive;
            obj["approved"]    = d.approved;
            obj["online"]      = d.online;
            if (d.lastUpdatedAt > 0) {
                obj["lastUpdatedAt"] = d.lastUpdatedAt;
            }
        }

        String payload;
        serializeJson(sendDoc, payload);

        // ── POST to cloud via retry handler ─────────────────
        int code = _postWithRetry("/api/controller-sync", payload);

        if (code == 200) {
            _consecutiveFails = 0;
            Serial.println("[CLOUD] Sync OK");
        } else {
            Serial.printf("[CLOUD] Sync failed: HTTP %d\n", code);
            _handleFailure();
        }

        _busy = false;
    }

    // ── Handle consecutive failures ─────────────────────────
    void _handleFailure() {
        _consecutiveFails++;
        Serial.printf("[CLOUD] Consecutive failures: %d\n", _consecutiveFails);

        // After 3 consecutive failures, destroy and recreate the TLS client
        // This fixes cases where the TLS session became stale or corrupted
        if (_consecutiveFails >= 3) {
            _resetClient();
            _consecutiveFails = 0;
        }
    }

private:
    DeviceRegistry&     _registry;
    unsigned long       _lastSync;
    unsigned long       _lastPing;
    unsigned long       _lastOpTime;
    bool                _busy;
    bool                _forceSyncPending;
    int                 _consecutiveFails;
    int                 _currentMode = 1;
    String              _lastWifiError = "OK";
    uint64_t            _timeOffset;
    WiFiClientSecure*   _client;
    HTTPClient*         _http;
    std::vector<String> _alertQueue;
    CloudCommandCallback _commandCallback = nullptr;
};

#endif // CLOUD_SYNC_H
