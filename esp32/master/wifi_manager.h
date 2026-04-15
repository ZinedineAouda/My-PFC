/*
 * ══════════════════════════════════════════════════════════════
 *  WiFi Manager — AP / STA / Hybrid mode management
 * ══════════════════════════════════════════════════════════════
 */
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "config.h"

class WifiManager {
public:
    WifiManager()
        : _mode(MODE_NONE), _staConnected(false),
          _lastReconnect(0), _reconnectInterval(5000), _lastReason(0) {
        memset(_apSSID, 0, sizeof(_apSSID));
        memset(_apPass, 0, sizeof(_apPass));
        memset(_staSSID, 0, sizeof(_staSSID));
        memset(_staPass, 0, sizeof(_staPass));
        strncpy(_apSSID, AP_SSID_DEFAULT, sizeof(_apSSID) - 1);
    }

    // ── Initial boot: start AP for setup page ───────────────
    void beginSetup() {
        WiFi.persistent(false);
        WiFi.setSleep(false);
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
        WiFi.softAP(_apSSID);
        Serial.printf("[WIFI] Setup AP started: %s @ %s\n",
                      _apSSID, WiFi.softAPIP().toString().c_str());
    }

    // ── Apply chosen mode ───────────────────────────────────
    void applyMode(WiFiOpMode mode) {
        _mode = mode;
        switch (mode) {
            case MODE_AP:
                WiFi.disconnect(true);
                WiFi.mode(WIFI_AP);
                _configureAP();
                _staConnected = false;
                Serial.println("[WIFI] Mode: AP Only");
                break;

            case MODE_STA:
                WiFi.softAPdisconnect(true);
                WiFi.mode(WIFI_STA);
                _connectSTA();
                Serial.println("[WIFI] Mode: STA Only");
                break;

            case MODE_AP_STA:
            case MODE_ONLINE:
                WiFi.mode(WIFI_AP_STA);
                _configureAP();
                if (strlen(_staSSID) > 0) {
                    _connectSTA();
                }
                Serial.printf("[WIFI] Mode: %s\n",
                    mode == MODE_ONLINE ? "Online (AP+STA+Cloud)" : "AP+STA");
                break;

            default:
                break;
        }
    }

    // ── Non-blocking loop handler ───────────────────────────
    void handle() {
        if (_mode == MODE_NONE || _mode == MODE_AP) return;

        // Track STA connection state changes
        bool connected = (WiFi.status() == WL_CONNECTED);
        if (connected && !_staConnected) {
            _staConnected = true;
            _staIP = WiFi.localIP().toString();
            Serial.printf("[WIFI] STA connected! IP: %s\n", _staIP.c_str());
        } else if (!connected && _staConnected) {
            _staConnected = false;
            _lastReason = WiFi.reasonCode();
            Serial.printf("[WIFI] STA disconnected. Reason: %d - %s\n", 
                          _lastReason, getLastError().c_str());
        }

        // Auto-reconnect STA (non-blocking)
        if (strlen(_staSSID) > 0) {
            wl_status_t status = WiFi.status();
            if (status != WL_CONNECTED && status != WL_IDLE_STATUS) {
                unsigned long now = millis();
                if (now - _lastReconnect > _reconnectInterval) {
                    _lastReconnect = now;
                    Serial.printf("[WIFI] Reconnecting to %s (Status: %d)...\n", _staSSID, (int)status);
                    WiFi.begin(_staSSID, _staPass);
                }
            }
        }
    }

    // ── Setters ─────────────────────────────────────────────
    void setAPCredentials(const char* ssid, const char* pass) {
        strncpy(_apSSID, ssid, sizeof(_apSSID) - 1);
        _apSSID[sizeof(_apSSID) - 1] = '\0';
        strncpy(_apPass, pass, sizeof(_apPass) - 1);
        _apPass[sizeof(_apPass) - 1] = '\0';
    }

    void setSTACredentials(const char* ssid, const char* pass) {
        strncpy(_staSSID, ssid, sizeof(_staSSID) - 1);
        _staSSID[sizeof(_staSSID) - 1] = '\0';
        strncpy(_staPass, pass, sizeof(_staPass) - 1);
        _staPass[sizeof(_staPass) - 1] = '\0';
    }

    // ── Initiate STA connection (from web setup) ────────────
    void connectSTA(const char* ssid, const char* pass) {
        if (WiFi.getMode() == WIFI_AP) {
            WiFi.mode(WIFI_AP_STA);
        }
        setSTACredentials(ssid, pass);
        WiFi.disconnect();
        WiFi.begin(_staSSID, _staPass);
        Serial.printf("[WIFI] Initiating connection to: %s\n", _staSSID);
    }

    // ── Getters ─────────────────────────────────────────────
    WiFiOpMode     mode()         const { return _mode; }
    bool           staConnected() const { return _staConnected; }
    const String&  staIP()        const { return _staIP; }
    const char*    apSSID()       const { return _apSSID; }
    const char*    apPass()       const { return _apPass; }
    const char*    staSSID()      const { return _staSSID; }
    const char*    staPass()      const { return _staPass; }
    String masterIP() const {
        if (_staConnected) return _staIP;
        return WiFi.softAPIP().toString();
    }

    String getLastError() const {
        if (_staConnected) return "Connected";
        
        wl_status_t status = WiFi.status();
        switch (status) {
            case WL_NO_SSID_AVAIL: return "SSID Not Found";
            case WL_CONNECT_FAILED: {
                if (_lastReason == 15 || _lastReason == 204) return "Invalid Password";
                return "Connection Failed (" + String(_lastReason) + ")";
            }
            case WL_CONNECTION_LOST: return "Connection Lost";
            case WL_DISCONNECTED: return "Disconnected";
            case WL_IDLE_STATUS: return "Connecting...";
            default: return "Status " + String((int)status);
        }
    }

    // ── Scan networks (async) ───────────────────────────────
    void startScan() {
        WiFi.scanNetworks(true);
    }

    String getScanResultsJson() {
        int n = WiFi.scanComplete();
        if (n == WIFI_SCAN_FAILED || n == WIFI_SCAN_RUNNING) {
            if (n == WIFI_SCAN_FAILED) WiFi.scanNetworks(true);
            return "[]";
        }
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < n && i < 20; i++) {
            JsonObject obj = arr.add<JsonObject>();
            obj["ssid"]   = WiFi.SSID(i);
            obj["rssi"]   = WiFi.RSSI(i);
            obj["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        }
        WiFi.scanDelete();
        WiFi.scanNetworks(true); // Start next scan
        String out;
        serializeJson(doc, out);
        return out;
    }

private:
    WiFiOpMode _mode;
    bool _staConnected;
    int  _lastReason;
    unsigned long _lastReconnect;
    unsigned long _reconnectInterval;
    String _staIP;

    char _apSSID[64];
    char _apPass[64];
    char _staSSID[64];
    char _staPass[64];

    void _configureAP() {
        WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
        delay(50); // Brief settle for AP config
        if (strlen(_apPass) >= 8) {
            WiFi.softAP(_apSSID, _apPass, AP_CHANNEL, 0, AP_MAX_CONNECTIONS);
        } else {
            WiFi.softAP(_apSSID, NULL, AP_CHANNEL, 0, AP_MAX_CONNECTIONS);
        }
        Serial.printf("[WIFI] AP active: %s @ %s\n",
                      _apSSID, WiFi.softAPIP().toString().c_str());
    }

    void _connectSTA() {
        WiFi.begin(_staSSID, _staPass);
        _lastReconnect = millis();
    }
};

#endif // WIFI_MANAGER_H
