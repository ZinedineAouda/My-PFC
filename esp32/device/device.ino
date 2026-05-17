#define VIBRATION_PIN 4  // change to your GPIO

void setup() {
  Serial.begin(115200);
  pinMode(VIBRATION_PIN, INPUT);

  Serial.println("SW-420 Test ESP32-S3 Started");
}

void loop() {
  int state = digitalRead(VIBRATION_PIN);

  if (state == HIGH) {
    Serial.println("⚠️ Vibration detected!");
  } else {
    Serial.println("No vibration");
  }

  delay(150);
}