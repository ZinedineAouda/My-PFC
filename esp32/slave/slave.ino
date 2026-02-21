/*
 * Hospital Patient Alarm System - ESP32 Slave Device
 * 
 * This code runs on each patient bedside ESP32 device.
 * It connects to the master's Wi-Fi and sends an HTTP alert
 * when the patient presses the call button.
 * 
 * Required Libraries:
 *   - HTTPClient (built-in with ESP32)
 *   - ArduinoJson (by Benoit Blanchon)
 * 
 * Board: ESP32 / ESP32-S3 Dev Module
 * 
 * Wiring:
 *   - Button: Connect between BUTTON_PIN and GND (uses internal pull-up)
 *   - LED:    Connect to LED_PIN with a 220 ohm resistor to GND
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ============ CONFIGURATION ============
// Change these for each slave device:
const char* SLAVE_ID = "s1";  // Unique ID for this slave (must match admin panel)

// Master Wi-Fi credentials (must match master AP settings):
const char* WIFI_SSID     = "HospitalAlarm";
const char* WIFI_PASSWORD = "hospital123";

// Master server address (ESP32 AP default IP):
const char* MASTER_URL = "http://192.168.4.1";

// Hardware pins:
#define BUTTON_PIN  0   // GPIO0 - BOOT button on most ESP32 boards (or wire your own)
#define LED_PIN     2   // GPIO2 - Built-in LED on most ESP32 boards

// Timing:
#define DEBOUNCE_MS       300     // Button debounce time
#define RECONNECT_MS      5000    // Wi-Fi reconnect interval
#define REGISTER_RETRY_MS 10000   // Registration retry interval

// ============ STATE ============
bool isRegistered = false;
bool buttonPressed = false;
unsigned long lastButtonPress = 0;
unsigned long lastReconnect = 0;
unsigned long lastRegisterAttempt = 0;

// ============ CONNECT TO WIFI ============
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
    // Blink LED while connecting
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

// ============ REGISTER WITH MASTER ============
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
      // Confirmation blink
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

// ============ SEND ALERT ============
void sendAlert() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot send alert - not connected to Wi-Fi");
    // Rapid blink to indicate error
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
      // Solid LED on to indicate alert is active
      digitalWrite(LED_PIN, HIGH);
    } else {
      const char* reason = respDoc["reason"] | "unknown";
      Serial.print("Alert rejected: ");
      Serial.println(reason);
      // Double blink to indicate already active
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

// ============ BUTTON ISR ============
void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_MS) {
    buttonPressed = true;
    lastButtonPress = now;
  }
}

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Hospital Alarm - Slave Device ===");
  Serial.print("Slave ID: ");
  Serial.println(SLAVE_ID);

  // Setup pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Attach interrupt for button press (falling edge = button pressed)
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  // Connect to master Wi-Fi
  WiFi.mode(WIFI_STA);
  connectWiFi();

  // Register with master
  if (WiFi.status() == WL_CONNECTED) {
    isRegistered = registerWithMaster();
  }

  Serial.println("Ready. Press the button to send an alert.");
}

// ============ LOOP ============
void loop() {
  // Handle Wi-Fi reconnection
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastReconnect > RECONNECT_MS) {
      connectWiFi();
      lastReconnect = millis();
    }
  }

  // Retry registration if not registered
  if (!isRegistered && WiFi.status() == WL_CONNECTED) {
    if (millis() - lastRegisterAttempt > REGISTER_RETRY_MS) {
      isRegistered = registerWithMaster();
      lastRegisterAttempt = millis();
    }
  }

  // Handle button press
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
