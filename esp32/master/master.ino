#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "web_pages.h"

// --- ONLINE MODE SETTINGS ------------------------
// Pre-configure these before flashing the ESP32!
const String serverURL = "https://my-pfc-production.up.railway.app"; // NO trailing slash
const String deviceKey = "esp32";
unsigned long lastHeartbeat = 0;
bool cloudBusy = false;                        // Prevents overlapping HTTPS calls
const unsigned long HEARTBEAT_INTERVAL = 15000; // 15s is safe for a full SSL roundtrip
// -------------------------------------------------

char apSSID[64] = "HospitalAlarm";
char apPass[64] = "";
const char* ADMIN_USER = "admin";
const char* ADMIN_PASS = "admin1234";

#define MAX_SLAVES 20
#define BUZZER_PIN 4

struct Slave {
  char slaveId[32];
  char patientName[64];
  char bed[16];
  char room[16];
  bool alertActive;
  bool registered;
  bool approved;
  unsigned long lastAlertTime;
  unsigned long lastSeen;
  bool used;
  bool pendingRegisterCloud;
  bool pendingAlertCloud;
};

bool pendingMasterPingCloud = false;

Slave slaves[MAX_SLAVES];
int slaveCount = 0;
int wifiMode = 0;
bool setupDone = false;
char staSSID[64] = "";
char staPass[64] = "";
String masterIP = "192.168.4.1";

AsyncWebServer server(80);
WiFiUDP beacon;
#define BEACON_PORT 5555
#define BEACON_INTERVAL 3000
unsigned long lastBeacon = 0;

bool checkAdminAuth(AsyncWebServerRequest *request) {
  // Debug: Print all headers to Serial Monitor
  int headersCount = request->headers();
  for(int i=0; i<headersCount; i++) {
    const AsyncWebHeader* h = request->getHeader(i);
    Serial.printf("[DEBUG] Header %s: %s\n", h->name().c_str(), h->value().c_str());
  }

  // Use custom header to avoid browser-level Basic Auth interference
  if (request->hasHeader("X-Auth-Token")) {
    if (request->header("X-Auth-Token") == "admin1234") return true;
  }
  
  // Fallback to legacy check
  if (request->hasHeader("Authorization")) {
    String authHeader = request->header("Authorization");
    if (authHeader.startsWith("Basic ")) {
      String provided = authHeader.substring(6);
      provided.trim();
      if (provided == "YWRtaW46YWRtaW4xMjM0") return true;
    }
  }

  Serial.println("[AUTH] Rejecting request: Invalid or missing token");
  return false;
}

void sendUnauthorized(AsyncWebServerRequest *request) {
  request->send(401, "application/json", "{\"message\":\"Unauthorized\"}");
}

int findSlaveIndex(const char* slaveId) {
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (slaves[i].used && strcmp(slaves[i].slaveId, slaveId) == 0) return i;
  }
  return -1;
}

int findFreeSlot() {
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (!slaves[i].used) return i;
  }
  return -1;
}

String getSlavesJson(bool onlyApproved) {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (!slaves[i].used) continue;
    if (onlyApproved && !slaves[i].approved) continue;
    JsonObject obj = arr.add<JsonObject>();
    obj["slaveId"] = slaves[i].slaveId;
    obj["patientName"] = slaves[i].patientName;
    obj["bed"] = slaves[i].bed;
    obj["room"] = slaves[i].room;
    obj["alertActive"] = slaves[i].alertActive;
    obj["registered"] = slaves[i].registered;
    obj["approved"] = slaves[i].approved;
    if (slaves[i].lastAlertTime > 0) obj["lastAlertTime"] = slaves[i].lastAlertTime;
    else obj["lastAlertTime"] = (char*)NULL;
    obj["lastSeen"] = slaves[i].lastSeen;
  }
  String output;
  serializeJson(doc, output);
  return output;
}

void syncToCloud() {
  if (wifiMode != 4 || WiFi.status() != WL_CONNECTED) return;
  if (cloudBusy) return;
  cloudBusy = true;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, serverURL + "/api/master-sync");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-device-key", deviceKey);

  JsonDocument sendDoc;
  JsonArray arr = sendDoc["slaves"].to<JsonArray>();
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (slaves[i].used) {
      JsonObject obj = arr.add<JsonObject>();
      obj["slaveId"] = slaves[i].slaveId;
      obj["alertActive"] = slaves[i].alertActive;
    }
  }
  String payload;
  serializeJson(sendDoc, payload);

  int httpCode = http.POST(payload);
  if (httpCode == 200) {
    String response = http.getString();
    JsonDocument recvDoc;
    deserializeJson(recvDoc, response);
    JsonArray remoteSlaves = recvDoc["slaves"];
    for (JsonObject remote : remoteSlaves) {
      const char* rid = remote["slaveId"];
      int idx = findSlaveIndex(rid);
      if (idx >= 0) {
        slaves[idx].approved = remote["approved"] | false;
        // If server says alert is inactive but we thought it was active
        if (remote.containsKey("alertActive")) {
          bool rAlert = remote["alertActive"] | false;
          if (!rAlert && slaves[idx].alertActive) {
            slaves[idx].alertActive = false;
            Serial.printf("[CLOUD] Remote cleared alert for %s\n", rid);
          }
        }
      } else {
        // Slave on server but not here? Add it.
        int slot = findFreeSlot();
        if (slot >= 0) {
          strncpy(slaves[slot].slaveId, rid, 31);
          strncpy(slaves[slot].patientName, remote["patientName"] | "", 63);
          slaves[slot].approved = remote["approved"] | false;
          slaves[slot].used = true;
          slaveCount++;
        }
      }
    }
    Serial.println("[CLOUD] Sync successful");
  } else {
    Serial.printf("[CLOUD] Sync failed: %d\n", httpCode);
  }
  http.end();
  cloudBusy = false;
}

void setupRoutes() {
  server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", SETUP_HTML);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!setupDone) {
      request->send_P(200, "text/html", SETUP_HTML);
      return;
    }
    request->send_P(200, "text/html", DASHBOARD_HTML);
  });

  server.on("/online-setup", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", ONLINE_INFO_HTML);
  });

  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", ADMIN_HTML);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"mode\":" + String(wifiMode) + ",\"setup\":" + (setupDone ? "true" : "false") + ",\"masterIP\":\"" + masterIP + "\",\"slaves\":" + String(slaveCount) + "}";
    request->send(200, "application/json", json);
  });

  server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
      WiFi.scanNetworks(true);
      request->send(200, "application/json", "[]");
      return;
    }
    if (n == WIFI_SCAN_RUNNING) {
      request->send(200, "application/json", "[]");
      return;
    }
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < n && i < 20; i++) {
      JsonObject obj = arr.add<JsonObject>();
      obj["ssid"] = WiFi.SSID(i);
      obj["rssi"] = WiFi.RSSI(i);
      obj["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  server.on("/api/slaves", HTTP_GET, [](AsyncWebServerRequest *request) {
    bool onlyApproved = request->hasParam("approved");
    bool showAll = request->hasParam("all");
    String json;
    if (showAll) json = getSlavesJson(false);
    else json = getSlavesJson(onlyApproved || !showAll);
    request->send(200, "application/json", json);
  });

  AsyncCallbackJsonWebHandler* connectHandler = new AsyncCallbackJsonWebHandler("/api/connect-wifi",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* ssid = obj["ssid"] | "";
      const char* pass = obj["password"] | "";
      if (strlen(ssid) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing SSID\"}");
        return;
      }
      strncpy(staSSID, ssid, sizeof(staSSID));
      strncpy(staPass, pass, sizeof(staPass));
      
      // Start connection asynchronously
      Serial.printf("Initiating connection to STA: %s...\n", staSSID);
      WiFi.begin(staSSID, staPass);
      
      // Return immediately so the server doesn't hang
      request->send(200, "application/json", "{\"success\":true,\"message\":\"Connection initiated. Checking status...\"}");
    }
  );
  server.addHandler(connectHandler);

  AsyncCallbackJsonWebHandler* setupHandler = new AsyncCallbackJsonWebHandler("/api/setup",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      wifiMode = obj["mode"] | 1;
      const char* newSSID = obj["apSSID"] | "";
      const char* newPass = obj["apPass"] | "";
      if (strlen(newSSID) > 0) {
        strncpy(apSSID, newSSID, sizeof(apSSID) - 1);
        apSSID[sizeof(apSSID) - 1] = '\0';
      }
      strncpy(apPass, newPass, sizeof(apPass) - 1);
      apPass[sizeof(apPass) - 1] = '\0';
      setupDone = true;
      if (wifiMode == 1) {
        WiFi.disconnect();
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
        delay(100);
        if (strlen(apPass) >= 8) {
          WiFi.softAP(apSSID, apPass, 1, 0, 8);
        } else {
          WiFi.softAP(apSSID, NULL, 1, 0, 8);
        }
        masterIP = "192.168.4.1";
      } else if (wifiMode == 2) {
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
      } else if (wifiMode == 4) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
        delay(100);
        if (strlen(apPass) >= 8) {
          WiFi.softAP(apSSID, apPass, 1, 0, 8);
        } else {
          WiFi.softAP(apSSID, NULL, 1, 0, 8);
        }
        Serial.println("[MODE 4] Online Mode Active. AP+STA configured.");
      } else if (wifiMode == 3) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
        delay(100);
        if (strlen(apPass) >= 8) {
          WiFi.softAP(apSSID, apPass, 1, 0, 8);
        } else {
          WiFi.softAP(apSSID, NULL, 1, 0, 8);
        }
      }
      String resp = "{\"success\":true,\"mode\":" + String(wifiMode) + ",\"apSSID\":\"" + String(apSSID) + "\",\"staIP\":\"" + masterIP + "\"}";
      request->send(200, "application/json", resp);
      Serial.printf("Setup complete. Mode: %d, AP SSID: %s\n", wifiMode, apSSID);
    }
  );
  server.addHandler(setupHandler);

  AsyncCallbackJsonWebHandler* registerHandler = new AsyncCallbackJsonWebHandler("/api/register",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* slaveId = obj["slaveId"] | "";
      if (strlen(slaveId) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing slaveId\"}");
        return;
      }
      int idx = findSlaveIndex(slaveId);
      if (idx >= 0) {
        slaves[idx].registered = true;
        slaves[idx].lastSeen = millis();
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Registered\"}");
        Serial.printf("Slave %s re-registered\n", slaveId);
        if (wifiMode == 4) slaves[idx].pendingRegisterCloud = true;
        return;
      }
      int slot = findFreeSlot();
      if (slot < 0) {
        request->send(507, "application/json", "{\"success\":false,\"message\":\"Max devices reached\"}");
        return;
      }
      strncpy(slaves[slot].slaveId, slaveId, sizeof(slaves[slot].slaveId));
      slaves[slot].patientName[0] = '\0';
      slaves[slot].bed[0] = '\0';
      slaves[slot].room[0] = '\0';
      slaves[slot].alertActive = false;
      slaves[slot].registered = true;
      slaves[slot].approved = false;
      slaves[slot].lastAlertTime = 0;
      slaves[slot].lastSeen = millis();
      slaves[slot].used = true;
      slaves[slot].pendingRegisterCloud = false;
      slaves[slot].pendingAlertCloud = false;
      slaveCount++;
      request->send(200, "application/json", "{\"success\":true,\"message\":\"Pending approval\"}");
      Serial.printf("New slave %s detected - pending approval\n", slaveId);
      if (wifiMode == 4) slaves[slot].pendingRegisterCloud = true;
    }
  );
  server.addHandler(registerHandler);

  AsyncCallbackJsonWebHandler* alertHandler = new AsyncCallbackJsonWebHandler("/api/alert",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* slaveId = obj["slaveId"] | "";
      if (strlen(slaveId) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing slaveId\"}");
        return;
      }
      int idx = findSlaveIndex(slaveId);
      if (idx < 0) {
        request->send(200, "application/json", "{\"success\":false,\"reason\":\"unknown slave\"}");
        return;
      }
      if (!slaves[idx].approved) {
        request->send(200, "application/json", "{\"success\":false,\"reason\":\"not approved\"}");
        return;
      }
      if (slaves[idx].alertActive) {
        request->send(200, "application/json", "{\"success\":false,\"reason\":\"alert already active\"}");
        return;
      }
      slaves[idx].alertActive = true;
      slaves[idx].lastAlertTime = millis();
      slaves[idx].lastSeen = millis();
      
      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("[ALERT] Received from Slave: %s\n", slaveId);
      if (wifiMode == 4) {
        slaves[idx].pendingAlertCloud = true;
        Serial.printf("[ALERT] Queued for Cloud: %s\n", slaveId);
      }
    }
  );
  server.addHandler(alertHandler);

  AsyncCallbackJsonWebHandler* loginHandler = new AsyncCallbackJsonWebHandler("/api/admin/login",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      JsonObject obj = json.as<JsonObject>();
      const char* user = obj["username"] | "";
      const char* pass = obj["password"] | "";
      if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(401, "application/json", "{\"message\":\"Invalid credentials\"}");
      }
    }
  );
  server.addHandler(loginHandler);

  AsyncCallbackJsonWebHandler* approveHandler = new AsyncCallbackJsonWebHandler("/api/approve/",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }
      String url = request->url();
      int lastSlash = url.lastIndexOf('/');
      String slaveId = url.substring(lastSlash + 1);
      int idx = findSlaveIndex(slaveId.c_str());
      if (idx < 0) { request->send(404, "application/json", "{\"message\":\"Not found\"}"); return; }
      
      JsonObject obj = json.as<JsonObject>();
      strncpy(slaves[idx].patientName, obj["patientName"] | "", sizeof(slaves[idx].patientName));
      strncpy(slaves[idx].bed, obj["bed"] | "", sizeof(slaves[idx].bed));
      strncpy(slaves[idx].room, obj["room"] | "", sizeof(slaves[idx].room));
      slaves[idx].approved = true;
      slaves[idx].lastSeen = millis();
      
      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("Approved via JSON: %s\n", slaveId.c_str());
    }
  );
  server.addHandler(approveHandler);

  AsyncCallbackJsonWebHandler* updateSlaveHandler = new AsyncCallbackJsonWebHandler("/api/slaves/",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      if (request->method() != HTTP_PUT) return; 
      if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }
      String url = request->url();
      int lastSlash = url.lastIndexOf('/');
      String slaveId = url.substring(lastSlash + 1);
      int idx = findSlaveIndex(slaveId.c_str());
      if (idx < 0) { request->send(404, "application/json", "{\"message\":\"Not found\"}"); return; }

      JsonObject obj = json.as<JsonObject>();
      if (obj.containsKey("patientName")) strncpy(slaves[idx].patientName, obj["patientName"] | "", sizeof(slaves[idx].patientName));
      if (obj.containsKey("bed")) strncpy(slaves[idx].bed, obj["bed"] | "", sizeof(slaves[idx].bed));
      if (obj.containsKey("room")) strncpy(slaves[idx].room, obj["room"] | "", sizeof(slaves[idx].room));
      
      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("Updated via JSON: %s\n", slaveId.c_str());
    }
  );
  server.addHandler(updateSlaveHandler);

  AsyncCallbackJsonWebHandler* clearAlertHandler = new AsyncCallbackJsonWebHandler("/api/clearAlert/",
    [](AsyncWebServerRequest *request, JsonVariant &json) {
      if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }
      String url = request->url();
      int lastSlash = url.lastIndexOf('/');
      String slaveId = url.substring(lastSlash + 1);
      int idx = findSlaveIndex(slaveId.c_str());
      if (idx < 0) { request->send(404, "application/json", "{\"message\":\"Not found\"}"); return; }
      slaves[idx].alertActive = false;
      request->send(200, "application/json", "{\"success\":true}");
      Serial.printf("Alert cleared: %s\n", slaveId.c_str());
    }
  );
  server.addHandler(clearAlertHandler);

  server.onNotFound([](AsyncWebServerRequest *request) {
    String url = request->url();
    if (request->method() == HTTP_DELETE && url.startsWith("/api/slaves/")) {
      if (!checkAdminAuth(request)) { sendUnauthorized(request); return; }
      String slaveId = url.substring(12);
      int idx = findSlaveIndex(slaveId.c_str());
      if (idx < 0) { request->send(404, "application/json", "{\"message\":\"Not found\"}"); return; }
      slaves[idx].used = false;
      memset(&slaves[idx], 0, sizeof(Slave));
      slaveCount--;
      request->send(200, "application/json", "{\"success\":true}");
      return;
    }
    if (request->method() == HTTP_OPTIONS) { request->send(200); return; }
    request->send(404, "application/json", "{\"message\":\"Not found\"}");
  });
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Hospital Patient Alarm System ===");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  memset(slaves, 0, sizeof(slaves));

  WiFi.persistent(false); // Prevents slow flash writes on every boot
  WiFi.setSleep(false);   // Disables modem sleep for lowest AP latency
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  WiFi.softAP(apSSID, NULL, 1, 0, 8);
  // Disabled the heavy 500ms delay to make boot instantaneous

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  WiFi.scanNetworks(true);

  setupRoutes();

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  server.begin();

  beacon.begin(BEACON_PORT);
  Serial.println("Web server started. Open http://192.168.4.1");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && (masterIP == "192.168.4.1" || masterIP == "")) {
    masterIP = WiFi.localIP().toString();
    Serial.print("[WIFI] Connected! Local IP: ");
    Serial.println(masterIP);
  }

  if (wifiMode == 4 && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
      lastHeartbeat = millis();
      syncToCloud();
    }
  }

  if (setupDone && millis() - lastBeacon > BEACON_INTERVAL) {
    String msg = "HOSPITAL_ALARM:" + masterIP;
    IPAddress broadcastIP;
    if (wifiMode == 1) {
      broadcastIP = IPAddress(192,168,4,255);
    } else {
      broadcastIP = IPAddress(255,255,255,255);
    }
    beacon.beginPacket(broadcastIP, BEACON_PORT);
    beacon.write((uint8_t*)msg.c_str(), msg.length());
    beacon.endPacket();
    lastBeacon = millis();
  }

  bool anyAlert = false;
  for (int i = 0; i < MAX_SLAVES; i++) {
    if (slaves[i].used && slaves[i].alertActive) {
      anyAlert = true;
      break;
    }
  }
  if (anyAlert) {
    static unsigned long lastBeep = 0;
    if (millis() - lastBeep > 5000) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
      lastBeep = millis();
    }
  }

  // Clear pending cloud flags when not in Mode 4 (no cloud connection)
  if (wifiMode != 4) {
    for (int i = 0; i < MAX_SLAVES; i++) {
      if (slaves[i].used) {
        slaves[i].pendingRegisterCloud = false;
        slaves[i].pendingAlertCloud = false;
      }
    }
    pendingMasterPingCloud = false;
  }

  delay(10);
}
