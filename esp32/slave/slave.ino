#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* SLAVE_ID = "s1";

const char* WIFI_SSID     = "HospitalAlarm";
const char* WIFI_PASSWORD = "hospital123";

const char* MASTER_URL = "http://192.168.4.1";

#define BUTTON_PIN  0
#define LED_PIN     2

#define DEBOUNCE_MS       300
#define RECONNECT_MS      5000
#define REGISTER_RETRY_MS 10000

bool isRegistered = false;
bool buttonPressed = false;
unsigned long lastButtonPress = 0;
unsigned long lastReconnect = 0;
unsigned long lastRegisterAttempt = 0;

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_PIN, LOW);
  } else {
    Serial.println("\nFailed to connect. Will retry...");
    digitalWrite(LED_PIN, LOW);
  }
}

bool registerWithMaster() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = String(MASTER_URL) + "/api/register";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["slaveId"] = SLAVE_ID;
  String body;
  serializeJson(doc, body);

  Serial.print("Registering with master as: ");
  Serial.println(SLAVE_ID);

  int httpCode = http.POST(body);

  if (httpCode == 200) {
    String response = http.getString();
    Serial.print("Registration response: ");
    Serial.println(response);

    JsonDocument respDoc;
    deserializeJson(respDoc, response);
    bool success = respDoc["success"] | false;

    if (success) {
      Serial.println("Successfully registered!");
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
      }
      http.end();
      return true;
    } else {
      Serial.println("Registration failed - ID not in admin panel");
    }
  } else {
    Serial.printf("Registration HTTP error: %d\n", httpCode);
  }

  http.end();
  return false;
}

void sendAlert() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot send alert - not connected to Wi-Fi");
    for (int i = 0; i < 5; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
      delay(50);
    }
    return;
  }

  HTTPClient http;
  String url = String(MASTER_URL) + "/api/alert";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["slaveId"] = SLAVE_ID;
  String body;
  serializeJson(doc, body);

  Serial.println("Sending alert...");

  int httpCode = http.POST(body);

  if (httpCode == 200) {
    String response = http.getString();
    Serial.print("Alert response: ");
    Serial.println(response);

    JsonDocument respDoc;
    deserializeJson(respDoc, response);
    bool success = respDoc["success"] | false;

    if (success) {
      Serial.println("Alert sent successfully!");
      digitalWrite(LED_PIN, HIGH);
    } else {
      const char* reason = respDoc["reason"] | "unknown";
      Serial.print("Alert rejected: ");
      Serial.println(reason);
      for (int i = 0; i < 2; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
      }
    }
  } else {
    Serial.printf("Alert HTTP error: %d\n", httpCode);
  }

  http.end();
}

void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_MS) {
    buttonPressed = true;
    lastButtonPress = now;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Hospital Alarm - Slave Device ===");
  Serial.print("Slave ID: ");
  Serial.println(SLAVE_ID);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  WiFi.mode(WIFI_STA);
  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    isRegistered = registerWithMaster();
  }

  Serial.println("Ready. Press the button to send an alert.");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnect > RECONNECT_MS) {
      connectWiFi();
      lastReconnect = millis();
    }
  }

  if (!isRegistered && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastRegisterAttempt > REGISTER_RETRY_MS) {
      isRegistered = registerWithMaster();
      lastRegisterAttempt = millis();
    }
  }

  if (buttonPressed) {
    buttonPressed = false;
    Serial.println("Button pressed!");

    if (isRegistered) {
      sendAlert();
    } else {
      Serial.println("Not registered yet - trying to register first...");
      isRegistered = registerWithMaster();
      if (isRegistered) {
        sendAlert();
      }
    }
  }

  delay(10);
}
