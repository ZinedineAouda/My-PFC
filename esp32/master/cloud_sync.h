/*
 * ══════════════════════════════════════════════════════════════
 *  Cloud Sync — HTTPS-based sync with Railway server (Mode 4)
 * ══════════════════════════════════════════════════════════════
 */
#ifndef CLOUD_SYNC_H
#define CLOUD_SYNC_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"
#include "device_registry.h"

class CloudSync {
public:
    CloudSync(DeviceRegistry& reg)
        : _registry(reg), _lastSync(0), _busy(false) {}

    void handle() {
        if (WiFi.status() != WL_CONNECTED) return;
        if (_busy) return;

        unsigned long now = millis();
        if (now - _lastSync < CLOUD_SYNC_INTERVAL) return;
        _lastSync = now;

        _syncToCloud();
    }

    // Force an immediate sync (called after important events)
    void syncNow() {
        if (WiFi.status() != WL_CONNECTED || _busy) return;
        _syncToCloud();
    }

private:
    DeviceRegistry& _registry;
    unsigned long    _lastSync;
    bool             _busy;

    void _syncToCloud() {
        _busy = true;

        WiFiClientSecure client;
        client.setInsecure(); // Skip cert verification (production: use CA cert)
        client.setHandshakeTimeout(3);

        HTTPClient http;
        http.setTimeout(CLOUD_HTTP_TIMEOUT);

        String url = String(CLOUD_SERVER_URL) + "/api/master-sync";
        if (!http.begin(client, url)) {
            Serial.println("[CLOUD] Connection failed");
            _busy = false;
            return;
        }

        http.addHeader("Content-Type", "application/json");
        http.addHeader("x-device-key", CLOUD_DEVICE_KEY);

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
        }

        http.end();
        _busy = false;
    }
};

#endif // CLOUD_SYNC_H
