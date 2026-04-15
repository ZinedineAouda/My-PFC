/*
 * ══════════════════════════════════════════════════════════════
 *  Device Registry — tracks all slave devices with NVS persistence
 * ══════════════════════════════════════════════════════════════
 */
#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <Preferences.h>
#include "config.h"

// ─── Slave Device Data ──────────────────────────────────────
struct SlaveDevice {
    String slaveId;
    String patientName;
    String bed;
    String room;
    bool   alertActive   = false;
    bool   registered    = false;
    bool   approved      = false;
    bool   online        = false;
    unsigned long lastAlertTime = 0;
    unsigned long lastClearTime = 0;
    unsigned long lastSeen      = 0;
    uint64_t      lastUpdatedAt = 0;
};

// ─── Callback type for change notifications ─────────────────
typedef void (*RegistryChangeCallback)(const String& deviceId, const char* eventType);

// ─── Device Registry Class ──────────────────────────────────
class DeviceRegistry {
public:
    DeviceRegistry() : _lastTimeoutCheck(0), _changeCallback(nullptr), _timeOffset(0) {}

    void begin() {
        load();
    }

    void onDeviceChange(RegistryChangeCallback cb) {
        _changeCallback = cb;
    }

    void updateTimeOffset(uint64_t offset) {
        _timeOffset = offset;
    }

    uint64_t getCurrentTime() {
        return _timeOffset > 0 ? (millis() + _timeOffset) : 0;
    }

    void load() {
        Preferences prefs;
        prefs.begin("registry", true);
        String data = prefs.getString("slaves", "");
        prefs.end();

        if (data.length() > 0) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data);
            if (!error) {
                JsonArray arr = doc.as<JsonArray>();
                for (JsonObject obj : arr) {
                    SlaveDevice dev;
                    dev.slaveId = obj["id"] | "";
                    if (dev.slaveId.isEmpty()) continue;
                    dev.patientName = obj["n"] | "";
                    dev.bed = obj["b"] | "";
                    dev.room = obj["r"] | "";
                    dev.approved = obj["a"] | false;
                    dev.alertActive = obj["al"] | false; // Restore alert state
                    dev.registered = true;
                    dev.online = false; // Reset to false on reboot
                    _devices[dev.slaveId] = dev;
                }
                Serial.printf("[REG] Loaded %d devices from storage\n", _devices.size());
            }
        }
    }

    void save() {
        Preferences prefs;
        prefs.begin("registry", false);
        
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (auto const& item : _devices) {
            const SlaveDevice& d = item.second;
            JsonObject obj = arr.add<JsonObject>();
            obj["id"] = d.slaveId;
            obj["n"]  = d.patientName;
            obj["b"]  = d.bed;
            obj["r"]  = d.room;
            obj["a"]  = d.approved;
            obj["al"] = d.alertActive;
        }
        
        String out;
        serializeJson(doc, out);
        
        if (out.length() > 3800) {
            Serial.printf("[REG] WARNING: Registry size (%d bytes) is approaching NVS limit (4096)!\n", out.length());
        }
        
        prefs.putString("slaves", out);
        prefs.end();
        Serial.printf("[REG] Saved %d devices (%d bytes)\n", _devices.size(), out.length());
    }

    void factoryReset() {
        Preferences prefs;
        prefs.begin("registry", false);
        prefs.clear();
        prefs.end();
        _devices.clear();
        _notify("", "reset");
    }

    // ── Register / heartbeat (creates if new) ───────────────
    SlaveDevice* registerDevice(const String& id) {
        auto it = _devices.find(id);
        if (it != _devices.end()) {
            it->second.registered = true;
            it->second.lastSeen = millis();
            it->second.online = true;
            return &it->second;
        }
        if (_devices.size() >= MAX_SLAVES) return nullptr;

        SlaveDevice dev;
        dev.slaveId    = id;
        dev.registered = true;
        dev.lastSeen   = millis();
        dev.online     = true;
        _devices[id]   = dev;

        Serial.printf("[REG] New slave: %s\n", id.c_str());
        save();
        _notify(id, "register");
        return &_devices[id];
    }

    void heartbeat(const String& id) {
        auto it = _devices.find(id);
        if (it != _devices.end()) {
            it->second.lastSeen = millis();
            bool wasOffline = !it->second.online;
            it->second.online = true;
            if (wasOffline) _notify(id, "online");
        } else {
            registerDevice(id);
        }
    }

    bool triggerAlert(const String& id) {
        auto it = _devices.find(id);
        if (it == _devices.end() || it->second.alertActive) return false;

        if (it->second.lastClearTime > 0 && millis() - it->second.lastClearTime < 2000) return false;

        it->second.alertActive = true;
        it->second.lastAlertTime = millis();
        it->second.lastSeen = millis();
        it->second.online = true;
        it->second.lastUpdatedAt = getCurrentTime();

        Serial.printf("[ALERT] Active: %s\n", id.c_str());
        save(); // Persist alert state
        _notify(id, "alert");
        return true;
    }

    bool clearAlert(const String& id) {
        auto it = _devices.find(id);
        if (it == _devices.end()) return false;
        it->second.alertActive = false;
        it->second.lastClearTime = millis();
        it->second.lastUpdatedAt = getCurrentTime();
        save(); // Persist clear state
        _notify(id, "clear");
        return true;
    }

    void clearAllAlerts() {
        for (auto& kv : _devices) {
            if (kv.second.alertActive) {
                kv.second.alertActive = false;
                kv.second.lastClearTime = millis();
                kv.second.lastUpdatedAt = getCurrentTime();
                _notify(kv.second.slaveId, "clear");
            }
        }
        save();
    }

    bool approveDevice(const String& id, const String& name,
                       const String& bed, const String& room) {
        auto it = _devices.find(id);
        if (it == _devices.end()) return false;
        it->second.patientName = name;
        it->second.bed = bed;
        it->second.room = room;
        it->second.approved = true;
        it->second.lastUpdatedAt = getCurrentTime();
        save();
        _notify(id, "approve");
        return true;
    }

    bool updateDevice(const String& id, const String& name,
                      const String& bed, const String& room) {
        auto it = _devices.find(id);
        if (it == _devices.end()) return false;
        if (name.length()) it->second.patientName = name;
        if (bed.length())  it->second.bed = bed;
        if (room.length()) it->second.room = room;
        it->second.lastUpdatedAt = getCurrentTime();
        save();
        _notify(id, "update");
        return true;
    }

    bool deleteDevice(const String& id) {
        auto it = _devices.find(id);
        if (it == _devices.end()) return false;
        _devices.erase(it);
        save();
        _notify(id, "delete");
        return true;
    }

    SlaveDevice* getDevice(const String& id) {
        auto it = _devices.find(id);
        return (it != _devices.end()) ? &it->second : nullptr;
    }

    size_t onlineCount() const {
        size_t c = 0;
        for (auto const& kv : _devices) if (kv.second.online) c++;
        return c;
    }

    size_t alertCount() const {
        size_t c = 0;
        for (auto const& kv : _devices) if (kv.second.alertActive) c++;
        return c;
    }

    String deviceToJson(const String& id) {
        auto it = _devices.find(id);
        if (it == _devices.end()) return "{}";
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        _serializeDevice(it->second, obj);
        String out;
        serializeJson(doc, out);
        return out;
    }

    size_t count() const { return _devices.size(); }
    bool hasActiveAlerts() const {
        for (auto const& kv : _devices) if (kv.second.alertActive) return true;
        return false;
    }

    void checkTimeouts() {
        unsigned long now = millis();
        if (now - _lastTimeoutCheck < TIMEOUT_CHECK_MS) return;
        _lastTimeoutCheck = now;

        for (auto& kv : _devices) {
            if (kv.second.online && (now - kv.second.lastSeen > HEARTBEAT_TIMEOUT_MS)) {
                kv.second.online = false;
                _notify(kv.second.slaveId, "offline");
            }
        }
    }

    String toJson(bool onlyApproved = false) const {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (auto const& kv : _devices) {
            const SlaveDevice& d = kv.second;
            if (onlyApproved && !d.approved) continue;
            JsonObject obj = arr.add<JsonObject>();
            _serializeDevice(d, obj);
        }
        String out;
        serializeJson(doc, out);
        return out;
    }

    void mergeFromCloud(JsonArray& remoteSlaves) {
        bool changed = false;
        for (JsonObject remote : remoteSlaves) {
            String rid = remote["slaveId"] | "";
            if (rid.isEmpty()) continue;

            auto it = _devices.find(rid);
            if (it != _devices.end()) {
                uint64_t tRemote = remote["lastUpdatedAt"].isNull() ? 0 : remote["lastUpdatedAt"].as<uint64_t>();
                uint64_t tLocal = it->second.lastUpdatedAt;

                if (tRemote >= tLocal || tLocal == 0) {
                    if (remote.containsKey("approved")) {
                        bool newApp = remote["approved"] | false;
                        if (newApp != it->second.approved) {
                            it->second.approved = newApp;
                            it->second.patientName = remote["patientName"] | "";
                            it->second.bed = remote["bed"] | "";
                            it->second.room = remote["room"] | "";
                            changed = true;
                            _notify(rid, "approve");
                        }
                    }
                    if (remote.containsKey("alertActive")) {
                        bool rAlert = remote["alertActive"] | false;
                        if (!rAlert && it->second.alertActive) {
                            it->second.alertActive = false;
                            it->second.lastClearTime = millis();
                            _notify(rid, "clear");
                            changed = true;
                        } else if (rAlert && !it->second.alertActive) {
                            it->second.alertActive = true;
                            it->second.lastAlertTime = millis();
                            _notify(rid, "alert");
                            changed = true;
                        }
                    }
                    if (tRemote > 0) {
                        it->second.lastUpdatedAt = tRemote;
                    }
                }
            } else {
                if (_devices.size() < MAX_SLAVES) {
                    SlaveDevice dev;
                    dev.slaveId     = rid;
                    dev.patientName = remote["patientName"] | "";
                    dev.bed         = remote["bed"] | "";
                    dev.room        = remote["room"] | "";
                    dev.approved    = remote["approved"] | false;
                    dev.registered  = true;
                    dev.lastUpdatedAt = remote["lastUpdatedAt"].isNull() ? getCurrentTime() : remote["lastUpdatedAt"].as<uint64_t>();
                    _devices[rid]   = dev;
                    changed = true;
                    _notify(rid, "register");
                }
            }
        }
        if (changed) save();
    }

    const std::map<String, SlaveDevice>& devices() const { return _devices; }

private:
    std::map<String, SlaveDevice> _devices;
    unsigned long _lastTimeoutCheck;
    RegistryChangeCallback _changeCallback;
    uint64_t _timeOffset;

    void _notify(const String& id, const char* event) const {
        if (_changeCallback) _changeCallback(id, event);
    }

    static void _serializeDevice(const SlaveDevice& d, JsonObject& obj) {
        obj["slaveId"]       = d.slaveId;
        obj["patientName"]   = d.patientName;
        obj["bed"]           = d.bed;
        obj["room"]          = d.room;
        obj["alertActive"]   = d.alertActive;
        obj["registered"]    = d.registered;
        obj["approved"]      = d.approved;
        obj["online"]        = d.online;
        obj["lastSeen"]      = d.lastSeen;
        if (d.lastUpdatedAt > 0) {
            obj["lastUpdatedAt"] = d.lastUpdatedAt;
        }
    }
};

#endif // DEVICE_REGISTRY_H
