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
    CloudSync(DeviceRegistry& reg)
        : _registry(reg), _lastSync(0), _lastPing(0),
          _busy(false), _forceSyncPending(false),
          _consecutiveFails(0) {
        _client = nullptr;
    }

    ~CloudSync() {
        if (_client) {
            delete _client;
            _client = nullptr;
        }
    }

    void handle() {
        if (WiFi.status() != WL_CONNECTED) return;
        if (_busy) return;

        unsigned long now = millis();

        // ── Process pending alert queue ─────────────────────
        _processAlertQueue();

        // ── Lightweight heartbeat ping every 15s ────────────
        // This keeps the "Master Online" indicator alive on the
        // cloud dashboard even if full syncs occasionally fail.
        if (now - _lastPing >= CLOUD_PING_INTERVAL) {
            _lastPing = now;
            _sendPing();
        }

        // ── Full state sync ─────────────────────────────────
        if (_forceSyncPending || (now - _lastSync >= CLOUD_SYNC_INTERVAL)) {
            _forceSyncPending = false;
            _lastSync = now;
            _syncToCloud();
        }
    }

    // Force an immediate sync
    // Non-blocking: flags for execution on the next loop iteration
    void syncNow() {
        _forceSyncPending = true;
    }

    // Send a specific alert to the cloud (called when slave triggers alert)
    void alertToCloud(const String& slaveId) {
        // Only queue if not already pending to avoid spamming
        for (const auto& id : _alertQueue) {
            if (id == slaveId) return;
        }
        _alertQueue.push_back(slaveId);
        Serial.printf("[CLOUD] Alert queued: %s (Queue size: %d)\n", 
                      slaveId.c_str(), _alertQueue.size());
    }

private:
    DeviceRegistry&   _registry;
    unsigned long     _lastSync;
    unsigned long     _lastPing;
    bool              _busy;
    bool              _forceSyncPending;
    int               _consecutiveFails;
    WiFiClientSecure* _client;
    std::vector<String> _alertQueue;

    void _processAlertQueue() {
        if (_alertQueue.empty()) return;

        // Process only one per loop to keep it non-blocking
        String slaveId = _alertQueue[0];
        if (_sendAlertToCloud(slaveId)) {
            _alertQueue.erase(_alertQueue.begin());
        }
    }

    // ── Ensure persistent TLS client exists ─────────────────
    WiFiClientSecure& _ensureClient() {
        if (!_client) {
            _client = new WiFiClientSecure();
            _client->setInsecure();
            _client->setHandshakeTimeout(8);  // 8s — generous for Railway cold starts
            Serial.println("[CLOUD] TLS client created");
        }
        return *_client;
    }

    // ── Reset the TLS client (after repeated failures) ──────
    void _resetClient() {
        if (_client) {
            _client->stop();
            delete _client;
            _client = nullptr;
            Serial.println("[CLOUD] TLS client reset");
        }
    }

    // ── Lightweight ping to keep heartbeat alive ────────────
    void _sendPing() {
        WiFiClientSecure& client = _ensureClient();
        HTTPClient http;
        http.setTimeout(CLOUD_HTTP_TIMEOUT);

        String url = String(CLOUD_SERVER_URL) + "/api/master-ping";
        if (!http.begin(client, url)) {
            Serial.println("[CLOUD] Ping connection failed");
            return;
        }

        http.addHeader("Content-Type", "application/json");
        http.addHeader("x-device-key", CLOUD_DEVICE_KEY);

        int httpCode = http.POST("{}");
        if (httpCode == 200) {
            Serial.println("[CLOUD] Ping OK");
        } else {
            Serial.printf("[CLOUD] Ping failed: HTTP %d\n", httpCode);
        }
        http.end();
    }

    // ── Send alert to cloud ────────────────────────────────
    bool _sendAlertToCloud(const String& slaveId) {
        WiFiClientSecure& client = _ensureClient();
        HTTPClient http;
        http.setTimeout(CLOUD_HTTP_TIMEOUT);

        String url = String(CLOUD_SERVER_URL) + "/api/alert";
        if (!http.begin(client, url)) {
            Serial.println("[CLOUD] Alert connection failed");
            return false;
        }

        http.addHeader("Content-Type", "application/json");
        http.addHeader("x-device-key", CLOUD_DEVICE_KEY);

        String payload = "{\"slaveId\":\"" + slaveId + "\"}";
        int httpCode = http.POST(payload);
        http.end();

        if (httpCode == 200) {
            Serial.printf("[CLOUD] Alert sent: %s\n", slaveId.c_str());
            return true;
        } else {
            Serial.printf("[CLOUD] Alert failed: HTTP %d\n", httpCode);
            _handleFailure();
            return false;
        }
    }

    // ── Full bidirectional state sync ───────────────────────
    void _syncToCloud() {
        _busy = true;

        WiFiClientSecure& client = _ensureClient();
        HTTPClient http;
        http.setTimeout(CLOUD_HTTP_TIMEOUT);
        http.setReuse(true);  // Enable HTTP keep-alive

        String url = String(CLOUD_SERVER_URL) + "/api/master-sync";
        if (!http.begin(client, url)) {
            Serial.println("[CLOUD] Connection failed");
            _handleFailure();
            _busy = false;
            return;
        }

        http.addHeader("Content-Type", "application/json");
        http.addHeader("x-device-key", CLOUD_DEVICE_KEY);
        http.addHeader("Connection", "keep-alive");

        // ── Build sync payload ──────────────────────────────
        JsonDocument sendDoc;
        JsonArray arr = sendDoc["slaves"].to<JsonArray>();
        for (auto& kv : _registry.devices()) {
            const SlaveDevice& d = kv.second;
            JsonObject obj = arr.add<JsonObject>();
            obj["slaveId"]     = d.slaveId;
            obj["patientName"] = d.patientName;
            obj["bed"]         = d.bed;
            obj["room"]        = d.room;
            obj["alertActive"] = d.alertActive;
            obj["approved"]    = d.approved;
            obj["online"]      = d.online;
        }

        String payload;
        serializeJson(sendDoc, payload);

        // ── POST to cloud ───────────────────────────────────
        int httpCode = http.POST(payload);
        if (httpCode == 200) {
            _consecutiveFails = 0;  // Reset failure counter
            String response = http.getString();
            JsonDocument recvDoc;
            if (deserializeJson(recvDoc, response) == DeserializationError::Ok) {
                JsonArray remoteSlaves = recvDoc["slaves"];
                if (!remoteSlaves.isNull()) {
                    _registry.mergeFromCloud(remoteSlaves);
                }
            }
            Serial.println("[CLOUD] Sync OK");
        } else {
            Serial.printf("[CLOUD] Sync failed: HTTP %d\n", httpCode);
            _handleFailure();
        }

        http.end();
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
};

#endif // CLOUD_SYNC_H
