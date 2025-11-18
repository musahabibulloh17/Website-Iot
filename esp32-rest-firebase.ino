/**
 * IoT Dashboard - ESP32 Firebase (REST API VERSION)
 * Menggunakan HTTP REST API langsung, lebih reliable
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// =======================
//   WIFI & FIREBASE
// =======================
#define WIFI_SSID     "F6"
#define WIFI_PASSWORD "kodok321"

#define DATABASE_URL  "https://iot-dashboard-ca956-default-rtdb.asia-southeast1.firebasedatabase.app"
#define DATABASE_SECRET ""  // Kosong untuk public database

// =======================
//   PIN SENSOR
// =======================
#define DHTPIN   23
#define DHTTYPE  DHT22
#define SOIL_PIN 34
#define LDR_PIN  35
DHT dht(DHTPIN, DHTTYPE);

// =======================
//  KALIBRASI SENSOR
// =======================
int airValue   = 3000;
int waterValue = 1200;
float Rfixed = 10000.0;

// =======================
//   PIN AKTUATOR
// =======================
#define FAN_IN1   16
#define FAN_IN2   17
#define FAN_ENA   5

#define PUMP_IN1  26
#define PUMP_IN2  27
#define PUMP_ENB  4

#define LAMP_PIN  25

// =======================
//   BATAS AUTO MODE
// =======================
const float TEMP_HIGH        = 32.0;
const float HUMID_LOW        = 71.0;
const float SOIL_LOW_PERCENT = 50.0;
const float LUX_LOW          = 22.0;

// =======================
//   TIMER & STATE
// =======================
unsigned long lastSensorSend = 0;
unsigned long lastCommandRead = 0;
const unsigned long SENSOR_INTERVAL = 60000;  // 1 menit
const unsigned long COMMAND_INTERVAL = 30000; // 30 detik

bool fanOn = false;
bool pumpOn = false;
bool lampOn = false;

// =======================
//   FUNGSI AKTUATOR
// =======================
void setFan(bool on) {
  digitalWrite(FAN_IN1, on ? HIGH : LOW);
  digitalWrite(FAN_IN2, LOW);
  digitalWrite(FAN_ENA, HIGH);
  fanOn = on;
}

void setPump(bool on) {
  digitalWrite(PUMP_IN1, on ? HIGH : LOW);
  digitalWrite(PUMP_IN2, LOW);
  digitalWrite(PUMP_ENB, HIGH);
  pumpOn = on;
}

void setLamp(bool on) {
  digitalWrite(LAMP_PIN, on ? HIGH : LOW);
  lampOn = on;
}

// =======================
//   HTTP REQUEST FUNCTION
// =======================
bool sendToFirebase(String path, String jsonData) {
  HTTPClient http;
  String url = DATABASE_URL + path + ".json";
  
  Serial.printf("Sending to: %s\n", url.c_str());
  Serial.printf("Data: %s\n", jsonData.c_str());
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.PUT(jsonData);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.printf("Response Code: %d\n", httpCode);
    Serial.printf("Response: %s\n\n", response.c_str());
    http.end();
    return (httpCode == 200);
  } else {
    Serial.printf("Error: %s\n\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}

bool readFromFirebase(String path, String& result) {
  HTTPClient http;
  String url = DATABASE_URL + path + ".json";
  
  Serial.printf("Reading from: %s\n", url.c_str());
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    result = http.getString();
    Serial.printf("Response Code: %d\n", httpCode);
    Serial.printf("Response: %s\n\n", result.c_str());
    http.end();
    return (httpCode == 200);
  } else {
    Serial.printf("Error: %s\n\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}

// =======================
//        SETUP
// =======================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n===== ESP32 IoT Dashboard (REST API) =====\n");

  dht.begin();

  pinMode(FAN_IN1, OUTPUT);
  pinMode(FAN_IN2, OUTPUT);
  pinMode(FAN_ENA, OUTPUT);

  pinMode(PUMP_IN1, OUTPUT);
  pinMode(PUMP_IN2, OUTPUT);
  pinMode(PUMP_ENB, OUTPUT);

  pinMode(LAMP_PIN, OUTPUT);

  setFan(false);
  setPump(false);
  setLamp(false);

  // -------- WIFI ---------
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int wifiRetry = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetry < 20) {
    delay(500);
    Serial.print(".");
    wifiRetry++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi Failed!");
    while(1) delay(1000);
  }

  // -------- FIREBASE REST API ---------
  Serial.println("\nFirebase REST API Ready!");
  Serial.printf("Database URL: %s\n", DATABASE_URL);
  Serial.println("==============================\n");

  delay(2000);
}

// =======================
//         LOOP
// =======================
void loop() {
  unsigned long now = millis();

  // ======== SEND SENSOR EVERY 1 MINUTE ========
  if (now - lastSensorSend >= SENSOR_INTERVAL) {
    readAndSendSensors();
    lastSensorSend = now;
  }

  // ======== READ MODE & COMMANDS EVERY 30 SECONDS ========
  if (now - lastCommandRead >= COMMAND_INTERVAL) {
    readModeAndControl();
    lastCommandRead = now;
  }

  delay(1000);
}

// =======================
//   BACA & KIRIM SENSOR
// =======================
void readAndSendSensors() {
  // ======== BACA SENSOR ========
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) { 
    h = 0; 
    t = 0; 
  }

  int soilRaw = analogRead(SOIL_PIN);
  float soilPercent = map(constrain(soilRaw, waterValue, airValue), waterValue, airValue, 100, 0);
  soilPercent = constrain(soilPercent, 0, 100);

  int ldrRaw = analogRead(LDR_PIN);
  float Vout = ldrRaw * (3.3 / 4095.0);
  Vout = constrain(Vout, 0.01, 3.29);
  float Rldr = (3.3 - Vout) * Rfixed / Vout;
  float R_k = max(Rldr / 1000.0, 0.1);
  float lux = 500.0 / pow(R_k, 1.4);

  unsigned long timestamp = millis();

  Serial.println("===== Sending Sensor Data =====");
  
  // ======== KIRIM SENSOR CURRENT ========
  DynamicJsonDocument doc_temp(200);
  doc_temp["value"] = t;
  doc_temp["timestamp"] = timestamp;
  String json_temp;
  serializeJson(doc_temp, json_temp);
  bool ok_temp = sendToFirebase("/sensors/airTemp/current", json_temp);
  Serial.printf("airTemp: %s (%.1f°C)\n", ok_temp ? "✅" : "❌", t);

  DynamicJsonDocument doc_humid(200);
  doc_humid["value"] = h;
  doc_humid["timestamp"] = timestamp;
  String json_humid;
  serializeJson(doc_humid, json_humid);
  bool ok_humid = sendToFirebase("/sensors/airHumidity/current", json_humid);
  Serial.printf("airHumidity: %s (%.1f%%)\n", ok_humid ? "✅" : "❌", h);

  DynamicJsonDocument doc_soil(200);
  doc_soil["value"] = soilPercent;
  doc_soil["timestamp"] = timestamp;
  String json_soil;
  serializeJson(doc_soil, json_soil);
  bool ok_soil = sendToFirebase("/sensors/soilMoisture/current", json_soil);
  Serial.printf("soilMoisture: %s (%.1f%%)\n", ok_soil ? "✅" : "❌", soilPercent);

  DynamicJsonDocument doc_light(200);
  doc_light["value"] = lux;
  doc_light["timestamp"] = timestamp;
  String json_light;
  serializeJson(doc_light, json_light);
  bool ok_light = sendToFirebase("/sensors/light/current", json_light);
  Serial.printf("light: %s (%.1f lux)\n\n", ok_light ? "✅" : "❌", lux);

  // ======== KIRIM SERIES DATA ========
  sendToFirebase("/sensors/airTemp/series/" + String(timestamp), json_temp);
  sendToFirebase("/sensors/airHumidity/series/" + String(timestamp), json_humid);
  sendToFirebase("/sensors/soilMoisture/series/" + String(timestamp), json_soil);
  sendToFirebase("/sensors/light/series/" + String(timestamp), json_light);
}

// =======================
//   BACA MODE & CONTROL
// =======================
void readModeAndControl() {
  Serial.println("===== Reading Mode & Commands =====");

  // ======== BACA SYSTEM MODE ========
  String mode = "auto";
  String modeResponse;
  if (readFromFirebase("/system/mode", modeResponse)) {
    // Remove quotes jika ada
    modeResponse.trim();
    if (modeResponse.startsWith("\"")) {
      modeResponse = modeResponse.substring(1, modeResponse.length() - 1);
    }
    if (modeResponse == "manual" || modeResponse == "auto") {
      mode = modeResponse;
    }
  }

  Serial.printf("Mode: %s\n", mode.c_str());

  if (mode == "manual") {
    // ======== MANUAL MODE ========
    readManualCommands();
  }
  else {
    // ======== AUTO MODE ========
    runAutoLogic();
  }

  // ======== KONTROL AKTUATOR ========
  setFan(fanOn);
  setPump(pumpOn);
  setLamp(lampOn);

  // ======== KIRIM STATUS AKTUATOR ========
  DynamicJsonDocument doc_fan(200);
  doc_fan["isOn"] = fanOn;
  String json_fan;
  serializeJson(doc_fan, json_fan);
  sendToFirebase("/actuators/fan/state", json_fan);

  DynamicJsonDocument doc_pump(200);
  doc_pump["isOn"] = pumpOn;
  String json_pump;
  serializeJson(doc_pump, json_pump);
  sendToFirebase("/actuators/pump/state", json_pump);

  DynamicJsonDocument doc_lamp(200);
  doc_lamp["isOn"] = lampOn;
  String json_lamp;
  serializeJson(doc_lamp, json_lamp);
  sendToFirebase("/actuators/lamp/state", json_lamp);

  Serial.println("===== Actuator Status =====");
  Serial.printf("Fan : %s\n", fanOn  ? "ON" : "OFF");
  Serial.printf("Pump: %s\n", pumpOn ? "ON" : "OFF");
  Serial.printf("Lamp: %s\n", lampOn ? "ON" : "OFF");
  Serial.println("===========================\n");
}

// =======================
//   MANUAL MODE COMMANDS
// =======================
void readManualCommands() {
  Serial.println(">>> MANUAL MODE <<<");

  // Baca lamp command
  String lampResponse;
  if (readFromFirebase("/actuators/lamp/command", lampResponse)) {
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, lampResponse);
    if (!error && doc.containsKey("isOn")) {
      lampOn = doc["isOn"].as<bool>();
    }
  }

  // Baca fan command
  String fanResponse;
  if (readFromFirebase("/actuators/fan/command", fanResponse)) {
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, fanResponse);
    if (!error && doc.containsKey("isOn")) {
      fanOn = doc["isOn"].as<bool>();
    }
  }

  // Baca pump command
  String pumpResponse;
  if (readFromFirebase("/actuators/pump/command", pumpResponse)) {
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, pumpResponse);
    if (!error && doc.containsKey("isOn")) {
      pumpOn = doc["isOn"].as<bool>();
    }
  }

  Serial.println("Commands received from website\n");
}

// =======================
//   AUTO MODE LOGIC
// =======================
void runAutoLogic() {
  Serial.println(">>> AUTO MODE <<<");

  // Baca sensor terbaru
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) { h = 0; t = 0; }

  int soilRaw = analogRead(SOIL_PIN);
  float soilPercent = map(constrain(soilRaw, waterValue, airValue), waterValue, airValue, 100, 0);
  soilPercent = constrain(soilPercent, 0, 100);

  int ldrRaw = analogRead(LDR_PIN);
  float Vout = ldrRaw * (3.3 / 4095.0);
  Vout = constrain(Vout, 0.01, 3.29);
  float Rldr = (3.3 - Vout) * Rfixed / Vout;
  float R_k = max(Rldr / 1000.0, 0.1);
  float lux = 500.0 / pow(R_k, 1.4);

  // ======== LOGIKA AUTO ========
  fanOn  = (t > TEMP_HIGH || h < HUMID_LOW);
  pumpOn = (soilPercent < SOIL_LOW_PERCENT);
  lampOn = (lux < LUX_LOW);

  Serial.println("Auto logic applied:");
  Serial.printf("  Temp > %.1f°C? %s → Fan %s\n", TEMP_HIGH, (t > TEMP_HIGH) ? "YES" : "NO", fanOn ? "ON" : "OFF");
  Serial.printf("  Soil < %.1f%%? %s → Pump %s\n", SOIL_LOW_PERCENT, (soilPercent < SOIL_LOW_PERCENT) ? "YES" : "NO", pumpOn ? "ON" : "OFF");
  Serial.printf("  Lux < %.1f? %s → Lamp %s\n\n", LUX_LOW, (lux < LUX_LOW) ? "YES" : "NO", lampOn ? "ON" : "OFF");
}
