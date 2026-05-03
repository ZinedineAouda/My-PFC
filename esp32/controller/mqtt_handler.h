/*
 * ══════════════════════════════════════════════════════════════
 *  MQTT Handler — Embedded PicoMQTT broker + message routing
 *  Library: PicoMQTT (install via Arduino Library Manager)
 * ══════════════════════════════════════════════════════════════
 */
#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <PicoMQTT.h>
#include <ArduinoJson.h>
#include "config.h"
#include "device_registry.h"

class MqttHandler {
public:
    MqttHandler(DeviceRegistry& reg) : _registry(reg), _broker(MQTT_PORT) {}

    void begin() {
        // ── Subscribe to device alerts ───────────────────────
        _broker.subscribe("device/+/alert", [this](const char* topic, const char* payload) {
            String deviceId = _extractDeviceId(topic);
            if (deviceId.isEmpty()) return;

            Serial.printf("[MQTT] Alert from %s\n", deviceId.c_str());

            // Parse payload
            JsonDocument doc;
            if (deserializeJson(doc, payload) == DeserializationError::Ok) {
                // Register if unknown
                _registry.registerDevice(deviceId);
                _registry.triggerAlert(deviceId);
            }
        });

        // ── Subscribe to device heartbeats ───────────────────
        _broker.subscribe("device/+/heartbeat", [this](const char* topic, const char* payload) {
            String deviceId = _extractDeviceId(topic);
            if (deviceId.isEmpty()) return;
            
            _registry.heartbeat(deviceId);
            
            // Refresh approval status for the device on every heartbeat
            // This fixes the "ghost setup page" after a device reboots
            publishDeviceStatus(deviceId);
        });

        // ── Start broker ────────────────────────────────────
        _broker.begin();
        Serial.printf("[MQTT] Broker started on port %d\n", MQTT_PORT);
    }

    void handle() {
        _broker.loop();
    }

    // ── Publish approval status to a device (retained) ───────
    void publishDeviceStatus(const String& deviceId) {
        PatientDevice* dev = _registry.getDevice(deviceId);
        if (!dev) return;

        JsonDocument doc;
        doc["approved"]    = dev->approved;
        doc["patientName"] = dev->patientName;
        doc["bed"]         = dev->bed;
        doc["room"]        = dev->room;

        String payload;
        serializeJson(doc, payload);
        String topic = "device/" + deviceId + "/status";

        _broker.publish(topic.c_str(), payload.c_str(), 1, true);
        Serial.printf("[MQTT] Published status for %s\n", deviceId.c_str());
    }

    // ── Send command to a specific device ────────────────────
    void sendCommand(const String& deviceId, const char* action) {
        JsonDocument doc;
        doc["action"] = action;
        String payload;
        serializeJson(doc, payload);
        String topic = "device/" + deviceId + "/command";
        _broker.publish(topic.c_str(), payload.c_str(), 1, false);
    }

    // ── Broadcast to all: system announcement ───────────────
    void broadcast(const char* message) {
        _broker.publish("controller/broadcast", message, 0, false);
    }

    // ── Migration Broadcast (New in 4D Rebuild) ──────────────
    void broadcastMigration(const char* newSSID, const char* newPass) {
        JsonDocument doc;
        doc["type"] = "MIGRATION";
        doc["ssid"] = newSSID;
        doc["pass"] = newPass;
        doc["ip"]   = String(MDNS_NAME) + ".local"; 

        String payload;
        serializeJson(doc, payload);
        _broker.publish(DISCOVERY_TOPIC, payload.c_str(), 1, true); // Retained for late joiners
        Serial.printf("[MQTT] Migration Broadcast Sent: target=%s\n", newSSID);
    }

    size_t connectedClients() const {
        // PicoMQTT doesn't expose client count directly,
        // rely on registry online count instead
        return _registry.onlineCount();
    }

private:
    DeviceRegistry& _registry;
    PicoMQTT::Server _broker;

    // Extract device ID from topic like "device/{id}/alert"
    static String _extractDeviceId(const char* topic) {
        String t(topic);
        int first = t.indexOf('/');
        if (first < 0) return "";
        int second = t.indexOf('/', first + 1);
        if (second < 0) return "";
        return t.substring(first + 1, second);
    }
};

#endif // MQTT_HANDLER_H
