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
#define HEARTBEAT_MS   30000
#define RECONNECT_MS   5000
#define BEACON_PORT    5555

#define LED_ON  LOW
#define LED_OFF HIGH

// --- STATE ---
char slaveId[32] = "";
char targetSSID[64] = "";
char targetPass[64] = "";
char masterURL[128] = "http://192.168.4.1";

bool setupDone = false;
bool isRegistered = false;
bool isApproved = false;
bool alertPending = false;
bool masterDiscovered = false;
volatile bool buttonPressed = false;
bool connectPending = false;

unsigned long lastButtonPress = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastReconnect = 0;
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
bool sendToMaster(String path, String payload) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  http.begin(wifiClient, String(masterURL) + path);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);
  int code = http.POST(payload);
  http.end();
  return (code == 200);
}

void doRegister() {
  JsonDocument doc;
  doc["slaveId"] = slaveId;
  String body;
  serializeJson(doc, body);
  if (sendToMaster("/api/register", body)) {
    isRegistered = true;
    ledBlink(2, 100, 100);
  }
}

void doAlert() {
  JsonDocument doc;
  doc["slaveId"] = slaveId;
  String body;
  serializeJson(doc, body);
  if (sendToMaster("/api/alert", body)) {
    alertPending = true;
    alertSentTime = millis();
    digitalWrite(LED_PIN, LED_ON);
  } else {
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
  snprintf(slaveId, sizeof(slaveId), "slave-%06X", id & 0xFFFFFF);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  // WiFi AP for Setup
  WiFi.mode(WIFI_AP);
  WiFi.softAP(String("Setup-" + String(slaveId)).c_str());

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
      strncpy(targetSSID, doc["ssid"] | "", 63);
      strncpy(targetPass, doc["password"] | "", 63);
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
    WiFi.begin(targetSSID, targetPass);
    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      setupDone = true;
      udp.begin(BEACON_PORT);
    }
  }

  if (setupDone && WiFi.status() == WL_CONNECTED) {
    // Check for Master Beacon
    int sz = udp.parsePacket();
    if (sz > 0) {
      char buf[64]; int len = udp.read(buf, 63); buf[len] = '\0';
      String pkt = String(buf);
      if (pkt.startsWith("HOSPITAL_ALARM:")) {
        String ip = pkt.substring(15);
        snprintf(masterURL, 127, "http://%s", ip.c_str());
        masterDiscovered = true;
      }
    }

    // Heartbeat/Register
    if (millis() - lastHeartbeat > HEARTBEAT_MS) {
      doRegister();
      lastHeartbeat = millis();
    }
  }

  if (buttonPressed) {
    buttonPressed = false;
    if (setupDone) doAlert();
    else ledBlink(3, 100, 100);
  }

  if (alertPending && millis() - alertSentTime > 30000) {
    alertPending = false;
    digitalWrite(LED_PIN, LED_OFF);
  }

  yield();
}
