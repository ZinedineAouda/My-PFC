/*
 * ══════════════════════════════════════════════════════════════
 *  Device Registry — tracks all slave devices
 * ══════════════════════════════════════════════════════════════
 */
#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
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
};

// ─── Callback type for change notifications ─────────────────
typedef void (*RegistryChangeCallback)(const String& deviceId, const char* eventType);

// ─── Device Registry Class ──────────────────────────────────
class DeviceRegistry {
public:
    DeviceRegistry() : _lastTimeoutCheck(0), _changeCallback(nullptr) {}

    void onDeviceChange(RegistryChangeCallback cb) {
        _changeCallback = cb;
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

        Serial.printf("[REG] New slave: %s (pending approval)\n", id.c_str());
        _notify(id, "register");
        return &_devices[id];
    }

    // ── Heartbeat update ────────────────────────────────────
    void heartbeat(const String& id) {
        auto it = _devices.find(id);
        if (it != _devices.end()) {
            it->second.lastSeen = millis();
            bool wasOffline = !it->second.online;
            it->second.online = true;
            if (wasOffline) {
                Serial.printf("[REG] %s back online\n", id.c_str());
                _notify(id, "online");
            }
        } else {
            // Auto-register on first heartbeat
            registerDevice(id);
        }
    }

    // ── Trigger alert ───────────────────────────────────────
    bool triggerAlert(const String& id) {
        auto it = _devices.find(id);
        if (it == _devices.end()) return false;
        // Removed !approved requirement: emergency alerts should always go through
        if (it->second.alertActive) return false;

        // Skip if alert was cleared very recently (cooldown to prevent hardware bounce)
        if (it->second.lastClearTime > 0 && millis() - it->second.lastClearTime < 2000) {
            Serial.printf("[ALERT] Suppressed (cooldown): %s\n", id.c_str());
            return false;
        }

        it->second.alertActive = true;
        it->second.lastAlertTime = millis();
        it->second.lastSeen = millis();
        it->second.online = true;

        Serial.printf("[ALERT] Active: %s\n", id.c_str());
        _notify(id, "alert");
        return true;
    }

    // ── Clear alert ─────────────────────────────────────────
    bool clearAlert(const String& id) {
        auto it = _devices.find(id);
        if (it == _devices.end()) return false;
        it->second.alertActive = false;
        it->second.lastClearTime = millis();
        Serial.printf("[ALERT] Cleared: %s\n", id.c_str());
        _notify(id, "clear");
        return true;
    }

    // ── Approve device ──────────────────────────────────────
    bool approveDevice(const String& id, const String& name,
                       const String& bed, const String& room) {
        auto it = _devices.find(id);
        if (it == _devices.end()) return false;
        it->second.patientName = name;
        it->second.bed = bed;
        it->second.room = room;
        it->second.approved = true;
        Serial.printf("[REG] Approved: %s → %s\n", id.c_str(), name.c_str());
        _notify(id, "approve");
        return true;
    }

    // ── Update device info ──────────────────────────────────
    bool updateDevice(const String& id, const String& name,
                      const String& bed, const String& room) {
        auto it = _devices.find(id);
        if (it == _devices.end()) return false;
        if (name.length()) it->second.patientName = name;
        if (bed.length())  it->second.bed = bed;
        if (room.length()) it->second.room = room;
        _notify(id, "update");
        return true;
    }

    // ── Delete device ───────────────────────────────────────
    bool deleteDevice(const String& id) {
        auto it = _devices.find(id);
        if (it == _devices.end()) return false;
        _devices.erase(it);
        Serial.printf("[REG] Deleted: %s\n", id.c_str());
        _notify(id, "delete");
        return true;
    }

    // ── Lookup ──────────────────────────────────────────────
    SlaveDevice* getDevice(const String& id) {
        auto it = _devices.find(id);
        return (it != _devices.end()) ? &it->second : nullptr;
    }

    size_t count() const { return _devices.size(); }

    size_t approvedCount() const {
        size_t n = 0;
        for (auto& kv : _devices) if (kv.second.approved) n++;
        return n;
    }

    size_t alertCount() const {
        size_t n = 0;
        for (auto& kv : _devices) if (kv.second.alertActive) n++;
        return n;
    }

    size_t onlineCount() const {
        size_t n = 0;
        for (auto& kv : _devices) if (kv.second.online) n++;
        return n;
    }

    bool hasActiveAlerts() const {
        for (auto& kv : _devices)
            if (kv.second.alertActive) return true;
        return false;
    }

    // ── Timeout check (call from loop) ──────────────────────
    void checkTimeouts() {
        unsigned long now = millis();
        if (now - _lastTimeoutCheck < TIMEOUT_CHECK_MS) return;
        _lastTimeoutCheck = now;

        for (auto& kv : _devices) {
            if (kv.second.online &&
                (now - kv.second.lastSeen > HEARTBEAT_TIMEOUT_MS)) {
                kv.second.online = false;
                Serial.printf("[REG] %s timed out (offline)\n",
                              kv.second.slaveId.c_str());
                _notify(kv.second.slaveId, "offline");
            }
        }
    }

    // ── JSON serialization ──────────────────────────────────
    String toJson(bool onlyApproved = false) const {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (auto& kv : _devices) {
            const SlaveDevice& d = kv.second;
            if (onlyApproved && !d.approved) continue;
            JsonObject obj = arr.add<JsonObject>();
            _serializeDevice(d, obj);
        }
        String out;
        serializeJson(doc, out);
        return out;
    }

    String deviceToJson(const String& id) const {
        auto it = _devices.find(id);
        if (it == _devices.end()) return "{}";
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        _serializeDevice(it->second, obj);
        String out;
        serializeJson(doc, out);
        return out;
    }

    // ── Iterate (for cloud sync, etc.) ──────────────────────
    const std::map<String, SlaveDevice>& devices() const {
        return _devices;
    }

    // ── Import from cloud response ──────────────────────────
    void mergeFromCloud(JsonArray& remoteSlaves) {
        for (JsonObject remote : remoteSlaves) {
            String rid = remote["slaveId"] | "";
            if (rid.isEmpty()) continue;

            auto it = _devices.find(rid);
            if (it != _devices.end()) {
                // Merge approval status from cloud
                if (remote.containsKey("approved")) {
                    bool wasApproved = it->second.approved;
                    it->second.approved = remote["approved"] | false;
                    if (!wasApproved && it->second.approved) {
                        it->second.patientName = remote["patientName"] | "";
                        it->second.bed = remote["bed"] | "";
                        it->second.room = remote["room"] | "";
                        _notify(rid, "approve");
                    }
                }
                // Merge alert clear from cloud
                if (remote.containsKey("alertActive")) {
                    bool rAlert = remote["alertActive"] | false;
                    if (!rAlert && it->second.alertActive) {
                        it->second.alertActive = false;
                        it->second.lastClearTime = millis();
                        _notify(rid, "clear");
                    }
                }
            } else {
                // New device from cloud
                if (_devices.size() < MAX_SLAVES) {
                    SlaveDevice dev;
                    dev.slaveId     = rid;
                    dev.patientName = remote["patientName"] | "";
                    dev.bed         = remote["bed"] | "";
                    dev.room        = remote["room"] | "";
                    dev.approved    = remote["approved"] | false;
                    dev.registered  = true;
                    _devices[rid]   = dev;
                    _notify(rid, "register");
                }
            }
        }
    }

private:
    std::map<String, SlaveDevice> _devices;
    unsigned long _lastTimeoutCheck;
    RegistryChangeCallback _changeCallback;

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
        obj["lastAlertTime"] = d.lastAlertTime;
        obj["lastSeen"]      = d.lastSeen;
    }
};

#endif // DEVICE_REGISTRY_H
