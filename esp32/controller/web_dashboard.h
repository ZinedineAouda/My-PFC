/*
 * ══════════════════════════════════════════════════════════════
 *  Web Dashboard — AsyncWebServer + WebSocket real-time push
 * ══════════════════════════════════════════════════════════════
 */
#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "device_registry.h"
#include "dashboard_static.h"

// Forward declarations — the MqttHandler reference is set from controller.ino
class MqttHandler;
extern MqttHandler mqttHandler;

class WebDashboard {
public:
    WebDashboard(DeviceRegistry& reg)
        : _registry(reg), _server(WEB_PORT), _ws("/ws") {}

    void begin(bool setupDone, WiFiOpMode mode) {
        _setupDone = setupDone;
        _mode = mode;

        // ── WebSocket events ────────────────────────────────
        _ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                           AwsEventType type, void* arg, uint8_t* data, size_t len) {
            _onWsEvent(server, client, type, arg, data, len);
        });
        _server.addHandler(&_ws);

        // ── Page routes ─────────────────────────────────────
        _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
            AsyncWebServerResponse *response = req->beginResponse_P(200, "text/html", react_index_html_gz, react_index_html_gz_len);
            response->addHeader("Content-Encoding", "gzip");
            req->send(response);
        });

        _server.on("/setup", HTTP_GET, [this](AsyncWebServerRequest* req) {
            AsyncWebServerResponse *response = req->beginResponse_P(200, "text/html", react_index_html_gz, react_index_html_gz_len);
            response->addHeader("Content-Encoding", "gzip");
            req->send(response);
        });

        _server.on("/admin", HTTP_GET, [](AsyncWebServerRequest* req) {
            AsyncWebServerResponse *response = req->beginResponse_P(200, "text/html", react_index_html_gz, react_index_html_gz_len);
            response->addHeader("Content-Encoding", "gzip");
            req->send(response);
        });

        // ── API: Status ─────────────────────────────────────
        _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
            JsonDocument doc;
            doc["mode"]     = (int)_mode;
            doc["setup"]    = _setupDone;
            doc["authenticated"] = _checkAuth(req);
            doc["controllerIP"] = WiFi.localIP().toString(); 
            doc["uptime"]   = millis() / 1000;
            doc["rssi"]     = WiFi.RSSI();
            doc["devices"]  = (int)_registry.count();
            doc["online"]   = (int)_registry.onlineCount();
            doc["alerts"]   = (int)_registry.alertCount();
            String out;
            serializeJson(doc, out);
            req->send(200, "application/json", out);
        });

        // ── API: WiFi Connection Status ─────────────────────
        _server.on("/api/wifi-status", HTTP_GET, [](AsyncWebServerRequest* req) {
            int status = WiFi.status();
            String json = "{\"status\":" + String(status) + "}";
            req->send(200, "application/json", json);
        });

        // ── API: WiFi scan ──────────────────────────────────
        _server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest* req) {
            int n = WiFi.scanComplete();
            if (n == WIFI_SCAN_FAILED) {
                WiFi.scanNetworks(true);
                req->send(200, "application/json", "[]");
                return;
            }
            if (n == WIFI_SCAN_RUNNING) {
                req->send(200, "application/json", "[]");
                return;
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
            WiFi.scanNetworks(true);
            String out;
            serializeJson(doc, out);
            req->send(200, "application/json", out);
        });

        // ── API: Get devices ─────────────────────────────────
        _server.on("/api/devices", HTTP_GET, [this](AsyncWebServerRequest* req) {
            bool onlyApproved = !req->hasParam("all");
            req->send(200, "application/json", _registry.toJson(onlyApproved));
        });

        // ── API: Connect WiFi (JSON body) ───────────────────
        auto* connectHandler = new AsyncCallbackJsonWebHandler("/api/connect-wifi",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                JsonObject obj = json.as<JsonObject>();
                const char* ssid = obj["ssid"] | "";
                const char* pass = obj["password"] | "";
                if (strlen(ssid) == 0) {
                    req->send(400, "application/json",
                               "{\"success\":false,\"message\":\"Missing SSID\"}");
                    return;
                }
                if (_connectCallback) _connectCallback(ssid, pass);
                req->send(200, "application/json",
                           "{\"success\":true,\"message\":\"Connecting...\"}");
            });
        _server.addHandler(connectHandler);

        // ── API: Setup (mode selection) ─────────────────────
        auto* setupHandler = new AsyncCallbackJsonWebHandler("/api/setup",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                JsonObject obj = json.as<JsonObject>();
                int mode = obj["mode"] | 0;
                const char* apSSID = obj["apSSID"] | "";
                const char* apPass = obj["apPass"] | "";
                if (mode < 1 || mode > 4) {
                    req->send(400, "application/json", "{\"success\":false}");
                    return;
                }
                if (_setupCallback) _setupCallback(mode, apSSID, apPass);
                _setupDone = true;
                _mode = (WiFiOpMode)mode;
                req->send(200, "application/json",
                    "{\"success\":true,\"mode\":" + String(mode) + "}");
            });
        _server.addHandler(setupHandler);

        // ── API: Register device ─────────────────────────────
        auto* registerHandler = new AsyncCallbackJsonWebHandler("/api/register",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                const char* id = json["deviceId"] | "";
                if (strlen(id) == 0) {
                    req->send(400, "application/json",
                               "{\"success\":false,\"message\":\"Missing deviceId\"}");
                    return;
                }
                PatientDevice* dev = _registry.registerDevice(String(id));
                if (!dev) {
                    req->send(507, "application/json",
                               "{\"success\":false,\"message\":\"Max devices\"}");
                    return;
                }
                req->send(200, "application/json", "{\"success\":true}");
            });
        _server.addHandler(registerHandler);

        // ── API: Alert ──────────────────────────────────────
        auto* alertHandler = new AsyncCallbackJsonWebHandler("/api/alert",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                const char* id = json["deviceId"] | "";
                if (strlen(id) == 0) {
                    req->send(400, "application/json",
                               "{\"success\":false,\"message\":\"Missing deviceId\"}");
                    return;
                }
                PatientDevice* dev = _registry.getDevice(String(id));
                if (!dev) {
                    req->send(200, "application/json",
                               "{\"success\":false,\"reason\":\"unknown device\"}");
                    return;
                }
                if (!dev->approved) {
                    req->send(200, "application/json",
                               "{\"success\":false,\"reason\":\"not approved\"}");
                    return;
                }
                if (dev->alertActive) {
                    req->send(200, "application/json",
                               "{\"success\":false,\"reason\":\"alert already active\"}");
                    return;
                }
                _registry.triggerAlert(String(id));
                req->send(200, "application/json", "{\"success\":true}");
            });
        _server.addHandler(alertHandler);

        // ── API: Admin login ────────────────────────────────
        auto* loginHandler = new AsyncCallbackJsonWebHandler("/api/admin/login",
            [](AsyncWebServerRequest* req, JsonVariant& json) {
                const char* user = json["username"] | "";
                const char* pass = json["password"] | "";
                if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
                    req->send(200, "application/json", "{\"success\":true}");
                } else {
                    req->send(401, "application/json",
                               "{\"message\":\"Invalid credentials\"}");
                }
            });
        _server.addHandler(loginHandler);

        // ── API: Approve device ──────────────────────────────
        auto* approveHandler = new AsyncCallbackJsonWebHandler("/api/approve",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                if (!_checkAuth(req)) {
                    req->send(401, "application/json", "{\"message\":\"Unauthorized\"}");
                    return;
                }
                JsonObject obj = json.as<JsonObject>();
                String deviceId = obj["deviceId"] | "";
                bool ok = _registry.approveDevice(deviceId,
                    obj["patientName"] | "", obj["bed"] | "", obj["room"] | "");
                if (!ok) {
                    req->send(404, "application/json", "{\"message\":\"Not found\"}");
                    return;
                }
                broadcastDeviceUpdate(deviceId);
                req->send(200, "application/json", "{\"success\":true}");
            });
        _server.addHandler(approveHandler);

        // ── API: Update device ───────────────────────────────
        auto* updateHandler = new AsyncCallbackJsonWebHandler("/api/update",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                if (!_checkAuth(req)) {
                    req->send(401, "application/json", "{\"message\":\"Unauthorized\"}");
                    return;
                }
                JsonObject obj = json.as<JsonObject>();
                String deviceId = obj["deviceId"] | "";
                bool ok = _registry.updateDevice(deviceId,
                    obj["patientName"] | "", obj["bed"] | "", obj["room"] | "");
                if (!ok) {
                    req->send(404, "application/json", "{\"message\":\"Not found\"}");
                    return;
                }
                broadcastDeviceUpdate(deviceId);
                req->send(200, "application/json", "{\"success\":true}");
            });
        _server.addHandler(updateHandler);

        // ── API: Clear alert ────────────────────────────────
        auto* clearHandler = new AsyncCallbackJsonWebHandler("/api/clearAlert",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                if (!_checkAuth(req)) {
                    req->send(401, "application/json", "{\"message\":\"Unauthorized\"}");
                    return;
                }
                JsonObject obj = json.as<JsonObject>();
                String deviceId = obj["deviceId"] | "";
                _registry.clearAlert(deviceId);
                broadcastDeviceUpdate(deviceId);
                req->send(200, "application/json", "{\"success\":true}");
            });
        _server.addHandler(clearHandler);

        // ── API: Delete device ───────────────────────────────
        auto* deleteHandler = new AsyncCallbackJsonWebHandler("/api/delete-device",
            [this](AsyncWebServerRequest* req, JsonVariant& json) {
                if (!_checkAuth(req)) {
                    req->send(401, "application/json", "{\"message\":\"Unauthorized\"}");
                    return;
                }
                JsonObject obj = json.as<JsonObject>();
                String deviceId = obj["deviceId"] | "";
                if (_registry.deleteDevice(deviceId)) {
                    broadcastDelete(deviceId);
                    req->send(200, "application/json", "{\"success\":true}");
                } else {
                    req->send(404, "application/json", "{\"message\":\"Not found\"}");
                }
            });
        _server.addHandler(deleteHandler);

        // ── API: Re-setup ────────────────────────────────────
        _server.on("/api/system/re-setup", HTTP_POST, [this](AsyncWebServerRequest* req) {
            if (!_checkAuth(req)) {
                req->send(401, "application/json", "{\"message\":\"Unauthorized\"}");
                return;
            }
            if (_reSetupCallback) _reSetupCallback();
            req->send(200, "application/json", "{\"success\":true}");
        });

        // ── API: Factory Reset ───────────────────────────────
        _server.on("/api/system/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
            if (!_checkAuth(req)) {
                req->send(401, "application/json", "{\"message\":\"Unauthorized\"}");
                return;
            }
            if (_resetCallback) _resetCallback();
            req->send(200, "application/json", "{\"success\":true}");
        });

        // ── API: Clear Device Registry ────────────────────────
        _server.on("/api/system/clear-devices", HTTP_POST, [this](AsyncWebServerRequest* req) {
            if (!_checkAuth(req)) {
                req->send(401, "application/json", "{\"message\":\"Unauthorized\"}");
                return;
            }
            if (_clearDevicesCallback) _clearDevicesCallback();
            req->send(200, "application/json", "{\"success\":true}");
        });

        // ── API: Force Cloud Sync ────────────────────────────
        _server.on("/api/system/sync", HTTP_POST, [this](AsyncWebServerRequest* req) {
            if (!_checkAuth(req)) {
                req->send(401, "application/json", "{\"message\":\"Unauthorized\"}");
                return;
            }
            if (_syncCallback) _syncCallback();
            req->send(200, "application/json", "{\"success\":true}");
        });

        // ── Catch-all: DELETE + OPTIONS + 404 ───────────────
        _server.onNotFound([this](AsyncWebServerRequest* req) {
            if (req->method() == HTTP_OPTIONS) {
                req->send(200);
                return;
            }
            req->send(404, "application/json", "{\"message\":\"Not found\"}");
        });

        // ── CORS headers ────────────────────────────────────
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods",
                                             "GET,POST,PUT,DELETE,OPTIONS");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers",
                                             "Content-Type,Authorization,X-Auth-Token");

        _server.begin();
        Serial.printf("[WEB] Server started on port %d\n", WEB_PORT);
    }

    void handle() {
        _ws.cleanupClients();
    }

    // ── WebSocket broadcast ─────────────────────────────────
    void broadcastFullState() {
        String json = "{\"type\":\"FULL_STATE\",\"devices\":" +
                      _registry.toJson(false) + "}";
        _ws.textAll(json);
    }

    void broadcastDeviceUpdate(const String& deviceId) {
        String json = "{\"type\":\"UPDATE\",\"device\":" +
                      _registry.deviceToJson(deviceId) + "}";
        _ws.textAll(json);
    }

    void broadcastDelete(const String& deviceId) {
        String json = "{\"type\":\"DELETE\",\"deviceId\":\"" + deviceId + "\"}";
        _ws.textAll(json);
    }

    // ── Callbacks for controller.ino to wire up ─────────────────
    typedef void (*ConnectCallback)(const char* ssid, const char* pass);
    typedef void (*SetupCallback)(int mode, const char* apSSID, const char* apPass);
    typedef void (*ResetCallback)();
    typedef void (*ReSetupCallback)();
    typedef void (*ClearDevicesCallback)();
    typedef void (*SyncCallback)();

    void onConnect(ConnectCallback cb) { _connectCallback = cb; }
    void onSetup(SetupCallback cb)     { _setupCallback = cb; }
    void onReSetup(ReSetupCallback cb) { _reSetupCallback = cb; }
    void onReset(ResetCallback cb)     { _resetCallback = cb; }
    void onClearDevices(ClearDevicesCallback cb) { _clearDevicesCallback = cb; }
    void onSync(SyncCallback cb)       { _syncCallback = cb; }

    void setSetupDone(bool done) { _setupDone = done; }
    void setMode(WiFiOpMode m)   { _mode = m; }

private:
    DeviceRegistry& _registry;
    AsyncWebServer  _server;
    AsyncWebSocket  _ws;
    bool            _setupDone = false;
    WiFiOpMode      _mode = MODE_NONE;
    ConnectCallback _connectCallback = nullptr;
    SetupCallback   _setupCallback = nullptr;
    ResetCallback   _resetCallback = nullptr;
    ReSetupCallback _reSetupCallback = nullptr;
    ClearDevicesCallback _clearDevicesCallback = nullptr;
    SyncCallback    _syncCallback = nullptr;

    // ── Auth check ──────────────────────────────────────────
    static bool _checkAuth(AsyncWebServerRequest* req) {
        if (req->hasHeader("X-Auth-Token")) {
            return (req->header("X-Auth-Token") == ADMIN_TOKEN);
        }
        if (req->hasHeader("Authorization")) {
            String auth = req->header("Authorization");
            if (auth.startsWith("Basic ")) {
                auth = auth.substring(6);
                auth.trim();
                return (auth == "YWRtaW46YWRtaW4xMjM0"); // admin:admin1234
            }
        }
        return false;
    }

    // ── Extract last URL segment ────────────────────────────
    static String _extractLastSegment(const String& url) {
        int pos = url.lastIndexOf('/');
        return (pos >= 0) ? url.substring(pos + 1) : url;
    }

    // ── WebSocket event handler ─────────────────────────────
    void _onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                    AwsEventType type, void* arg, uint8_t* data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("[WS] Client #%u connected\n", client->id());
            // Send full state on connect
            String json = "{\"type\":\"FULL_STATE\",\"devices\":" +
                          _registry.toJson(false) + "}";
            client->text(json);
        } else if (type == WS_EVT_DISCONNECT) {
            Serial.printf("[WS] Client #%u disconnected\n", client->id());
        }
    }
};

#endif // WEB_DASHBOARD_H
