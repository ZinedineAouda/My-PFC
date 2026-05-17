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
          _consecutiveFails(0), _consecutiveDnsFails(0),
          _dnsBackoffUntil(0), _timeOffset(0) {
        _client = new WiFiClientSecure();
        _client->setInsecure();
        _client->setHandshakeTimeout(30000); // 30s for heavy Cloudflare/Railway handshakes
        _http = new HTTPClient();
        _http->setTimeout(30000); 
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
        
        // Safety: Wait until NTP syncs (> year 2020) to ensure TLS handshake works
        time_t now_time = time(nullptr);
        if (now_time < 1600000000) return; 

        if (_busy) return;

        // DNS backoff: skip all cloud ops if server hostname was recently unreachable
        if (_dnsBackoffUntil > 0 && millis() < _dnsBackoffUntil) return;

        _currentMode = currentMode;
        _lastWifiError = wifiError;
        unsigned long now = millis();
        
        // ── 1. Process pending alert queue (highest priority — NO THROTTLE) ─
        if (!_alertQueue.empty()) {
            _processAlertQueue();
            _lastOpTime = now; 
            return;
        }

        // ── 2. Background throttled tasks ────────────────────
        if (now - _lastOpTime < 1000) return; 

        // ── 3. Lightweight heartbeat ping every 10s ──────────
        // Lighter load during alerts: skip pings/syncs if registry has active alerts
        bool hasAlerts = _registry.alertCount() > 0;

        if (!hasAlerts && (now - _lastPing >= CLOUD_PING_INTERVAL)) {
            _lastPing = now;
            _lastOpTime = now;
            _sendPing();
            return;
        }

        // ── 4. Full state sync ───────────────────────────────
        if (_forceSyncPending || (!hasAlerts && (now - _lastSync >= CLOUD_SYNC_INTERVAL))) {
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

    // Extract hostname from a URL like "https://host.example.com/path"
    static String _extractHost(const char* url) {
        String s(url);
        int start = s.indexOf("://");
        if (start < 0) start = 0; else start += 3;
        int end = s.indexOf('/', start);
        if (end < 0) end = s.length();
        return s.substring(start, end);
    }

    int _executePost(const String& path, const String& payload) {
        String url = String(CLOUD_SERVER_URL) + path;
        String serverHost = _extractHost(CLOUD_SERVER_URL);
        
        // 1. DNS check with retries (Railway CNAME chains can be slow/complex)
        IPAddress serverIP;
        bool serverDNS = false;
        for (int i = 0; i < 3; i++) {
            if (WiFi.hostByName(serverHost.c_str(), serverIP) && serverIP[0] != 0) {
                serverDNS = true;
                break;
            }
            if (i < 2) {
                delay(1000);
            }
        }
        
        time_t now_time = time(nullptr);

        if (!serverDNS) {
            _consecutiveDnsFails++;
            // FALLBACK: Use confirmed IP if DNS is failing.
            serverIP = IPAddress(66, 33, 22, 34); 
            serverDNS = true;
        } else {
            _consecutiveDnsFails = 0;
            _dnsBackoffUntil = 0;
        }

        // 2. Persistent Client management
        // MOBILE HOTSPOT OPTIMIZATION: Set MTU to 1400 to avoid fragmentation
        _client->setInsecure();
        _client->setHandshakeTimeout(30000); 
        
        delay(50); 
        
        // In Core 3.3.0, SNI is set by using this specific connect overload.
        // We pre-connect to the IP so HTTPClient doesn't have to resolve DNS.
        if (!_client->connected()) {
            if (!_client->connect(serverIP, 443, serverHost.c_str(), NULL, NULL, NULL)) {
                Serial.printf("[CLOUD] Socket connect to %s FAILED\n", serverIP.toString().c_str());
            }
        }
        
        // Use the original URL here. Because _client is already connected, 
        // HTTPClient will skip DNS and reuse the active secure socket.
        _http->begin(*_client, url);
        _http->setTimeout(CLOUD_HTTP_TIMEOUT);
        _http->setReuse(true);
        
        _http->addHeader("Content-Type", "application/json");
        _http->addHeader("x-device-key", CLOUD_DEVICE_KEY);
        _http->addHeader("Host", serverHost.c_str());  // Derived from URL, not hardcoded
        _http->setUserAgent("ESP32-SmartChair/1.1"); 
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
            Serial.printf("[CLOUD] HTTP 200 OK (%lums)\n", dur);
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
        
        _http->end(); // Always end to clean up resources
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
    int                 _consecutiveDnsFails;
    unsigned long       _dnsBackoffUntil;
    int                 _currentMode = 1;
    String              _lastWifiError = "OK";
    uint64_t            _timeOffset;
    WiFiClientSecure*   _client;
    HTTPClient*         _http;
    std::vector<String> _alertQueue;
    CloudCommandCallback _commandCallback = nullptr;
};

#endif // CLOUD_SYNC_H
