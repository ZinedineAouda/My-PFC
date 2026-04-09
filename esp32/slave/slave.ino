/*
 * Hospital Alarm System - Unified Slave Device
 * Support: ESP8266 (ESP-01, NodeMCU, ESP-12)
 */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <WiFiUdp.h>
#include "slave_pages.h"

// --- CONFIGURATION ---
#define BUTTON_PIN     0    // GPIO0 (Built-in on ESP-01)
#define LED_PIN        2    // GPIO2 (Built-in on ESP-01)
#define DEBOUNCE_MS    300
#define HEARTBEAT_MS   15000
#define RECONNECT_MS   10000
#define BEACON_PORT    5555

#define LED_ON  LOW
#define LED_OFF HIGH

// --- STATE ---
String slaveId = "";
String targetSSID = "";
String targetPass = "";
String masterURL = "http://192.168.4.1";

bool setupDone = false;
bool isRegistered = false;
bool isApproved = false;
bool alertPending = false;
bool masterDiscovered = false;
volatile bool buttonPressed = false;
bool connectPending = false;

unsigned long lastButtonPress = 0;
unsigned long lastHeartbeat = 0;
unsigned long alertSentTime = 0;

AsyncWebServer server(80);
WiFiClient wifiClient;
WiFiUDP udp;

// --- UTILS ---
void ICACHE_RAM_ATTR buttonISR() {
  if (millis() - lastButtonPress > DEBOUNCE_MS) {
    buttonPressed = true;
    lastButtonPress = millis();
  }
}

void ledBlink(int n, int d1, int d2) {
  for(int i=0; i<n; i++) {
    digitalWrite(LED_PIN, LED_ON); delay(d1);
    digitalWrite(LED_PIN, LED_OFF); if(i<n-1) delay(d2);
  }
}

// --- LOGIC ---
// Returns empty string on success, or the "reason" field from the JSON response on failure
String sendToMaster(String path, String payload) {
  if (WiFi.status() != WL_CONNECTED) return "wifi_disconnected";
  HTTPClient http;
  http.begin(wifiClient, masterURL + path);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  int code = http.POST(payload);
  Serial.printf("[HTTP] POST %s -> status: %d\n", path.c_str(), code);
  if (code == 200) {
    String body = http.getString();
    http.end();
    // Parse reason from JSON if success == false
    JsonDocument respDoc;
    DeserializationError err = deserializeJson(respDoc, body);
    if (!err && respDoc["success"] == false) {
      const char* reason = respDoc["reason"] | "unknown";
      return String(reason);
    }
    return ""; // empty = success
  }
  http.end();
  return "http_" + String(code);
}

void doRegister() {
  JsonDocument doc;
  doc["slaveId"] = slaveId;
  String body;
  serializeJson(doc, body);
  String reason = sendToMaster("/api/register", body);
  if (reason == "") {
    isRegistered = true;
    Serial.println("[SYNC] registered successfully with Master");
    ledBlink(2, 100, 100);
  } else {
    Serial.printf("[SYNC] registration result: %s\n", reason.c_str());
  }
}

void doAlert() {
  JsonDocument doc;
  doc["slaveId"] = slaveId;
  String body;
  serializeJson(doc, body);
  String reason = sendToMaster("/api/alert", body);
  if (reason == "") {
    alertPending = true;
    alertSentTime = millis();
    digitalWrite(LED_PIN, LED_ON);
    Serial.println("[ALERT] Signal RECEIVED by Master");
  } else if (reason == "not approved") {
    // Distinct 6-blink pattern: "waiting for admin approval"
    Serial.println("[ALERT] Not yet approved by admin — waiting...");
    ledBlink(6, 80, 80);
  } else if (reason == "alert already active") {
    // Solid LED already on — just log
    Serial.println("[ALERT] Alert already active on Master");
    digitalWrite(LED_PIN, LED_ON);
  } else {
    // Generic failure: 4 fast blinks
    Serial.printf("[ALERT] FAILED: %s\n", reason.c_str());
    ledBlink(4, 50, 50);
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF);
  
  uint32_t id = ESP.getChipId();
  char buf[32];
  snprintf(buf, 31, "slave-%06X", id & 0xFFFFFF);
  slaveId = String(buf);
  
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  // WiFi AP for Setup
  WiFi.mode(WIFI_AP);
  WiFi.softAP(String("Setup-" + slaveId).c_str());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", setupDone ? SLAVE_STATUS_HTML : SLAVE_SETUP_HTML);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["id"] = slaveId;
    doc["ip"] = WiFi.localIP().toString();
    doc["master"] = masterURL;
    doc["connected"] = (WiFi.status() == WL_CONNECTED);
    doc["alertPending"] = alertPending;
    String out; serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int n = WiFi.scanComplete();
    if (n == -2) { WiFi.scanNetworks(true); request->send(200, "application/json", "[]"); return; }
    JsonDocument doc; JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < n && i < 15; i++) {
      JsonObject obj = arr.add<JsonObject>();
      obj["ssid"] = WiFi.SSID(i); obj["rssi"] = WiFi.RSSI(i);
    }
    WiFi.scanDelete(); WiFi.scanNetworks(true);
    String out; serializeJson(doc, out);
    request->send(200, "application/json", out);
  });

  server.on("/api/connect", HTTP_POST, [](AsyncWebServerRequest *r){}, NULL, 
    [](AsyncWebServerRequest *r, uint8_t *data, size_t len, size_t idx, size_t tot) {
      JsonDocument doc; deserializeJson(doc, data);
      targetSSID = doc["ssid"] | "";
      targetPass = doc["password"] | "";
      connectPending = true;
      r->send(200, "application/json", "{\"success\":true}");
  });

  server.on("/api/trigger-alert", HTTP_POST, [](AsyncWebServerRequest *r) {
    buttonPressed = true;
    r->send(200, "application/json", "{\"success\":true}");
  });

  server.begin();
}

void loop() {
  if (connectPending) {
    connectPending = false;
    Serial.println("Connecting to network...");
    WiFi.begin(targetSSID.c_str(), targetPass.c_str());
    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      setupDone = true;
      udp.begin(BEACON_PORT);
      Serial.println("WiFi Connected. AP shut down.");
    }
  }

  if (setupDone && WiFi.status() == WL_CONNECTED) {
    int sz = udp.parsePacket();
    if (sz > 0) {
      char buf[64]; int len = udp.read(buf, 63); buf[len] = '\0';
      String pkt = String(buf);
      if (pkt.startsWith("HOSPITAL_ALARM:")) {
        String ip = pkt.substring(15);
        String newURL = "http://" + ip;
        if (!masterDiscovered || masterURL != newURL) {
          masterURL = newURL;
          masterDiscovered = true;
          Serial.printf("Master located at: %s\n", masterURL.c_str());
          doRegister(); 
        }
      }
    }

    // Fallback: if no beacon received in 30s, try the default AP IP
    static unsigned long beaconWaitStart = 0;
    if (!masterDiscovered) {
      if (beaconWaitStart == 0) beaconWaitStart = millis();
      if (millis() - beaconWaitStart > 30000) {
        Serial.println("[FALLBACK] No beacon found. Trying 192.168.4.1...");
        masterURL = "http://192.168.4.1";
        masterDiscovered = true;
        doRegister();
      }
    } else {
      beaconWaitStart = 0; // reset in case master goes offline and comes back
    }

    if (millis() - lastHeartbeat > HEARTBEAT_MS) {
      lastHeartbeat = millis();
      doRegister();
    }
  }

  if (buttonPressed) {
    buttonPressed = false;
    if (!setupDone) {
      ledBlink(3, 100, 100);
      Serial.println("Action denied: Setup not complete.");
    } else if (WiFi.status() != WL_CONNECTED) {
      ledBlink(2, 200, 200);
      Serial.println("Action denied: Not connected to WiFi.");
    } else if (!masterDiscovered) {
      // Try fallback immediately when button is pressed
      Serial.println("[ALERT] No beacon yet — using fallback 192.168.4.1");
      masterURL = "http://192.168.4.1";
      masterDiscovered = true;
      doRegister();
      doAlert();
    } else {
      doAlert();
    }
  }

  if (alertPending && millis() - alertSentTime > 30000) {
    alertPending = false;
    digitalWrite(LED_PIN, LED_OFF);
  }

  yield();
}
