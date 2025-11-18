/**
 * IoT Dashboard - ESP32 Firebase (SIMPLIFIED VERSION)
 * Kompatibel dengan semua versi Firebase library
 */

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <DHT.h>

// =======================
//   WIFI & FIREBASE
// =======================
#define WIFI_SSID     "F6"
#define WIFI_PASSWORD "kodok321"

#define API_KEY       "AIzaSyCH2L80uoizUUNN6hbvnbhoFBkfesEeHH0"
#define DATABASE_URL  "https://iot-dashboard-ca956-default-rtdb.asia-southeast1.firebasedatabase.app"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

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
//        SETUP
// =======================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n===== ESP32 IoT Dashboard =====\n");

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

  // -------- FIREBASE ---------
  Serial.println("\nInitializing Firebase...");
  Serial.printf("API Key: %.20s...\n", API_KEY);
  Serial.printf("Database URL: %s\n", DATABASE_URL);
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = "test@test.com";
  auth.user.password = "12345678";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Wait longer for Firebase to initialize
  Serial.println("Waiting for Firebase...");
  for (int i = 0; i < 10; i++) {
    delay(1000);
    Serial.print(".");
    if (Firebase.ready()) {
      Serial.println("\n✅ Firebase Connected!");
      break;
    }
  }
  
  if (!Firebase.ready()) {
    Serial.println("\n⚠️ Firebase Connection TIMEOUT!");
    Serial.println("Will retry in loop...");
  }
  
  Serial.println("==============================\n");
}

// =======================
//         LOOP
// =======================
void loop() {
  unsigned long now = millis();

  // ======== CHECK FIREBASE STATUS EVERY 5 SECONDS ========
  static unsigned long lastStatusCheck = 0;
  if (now - lastStatusCheck >= 5000) {
    if (Firebase.ready()) {
      Serial.println("✅ Firebase Ready - Proceeding with operations");
    } else {
      Serial.println("⏳ Firebase Not Ready Yet - Waiting...");
    }
    lastStatusCheck = now;
  }

  // ======== SEND SENSOR EVERY 1 MINUTE ========
  if (now - lastSensorSend >= SENSOR_INTERVAL) {
    if (Firebase.ready()) {
      readAndSendSensors();
      lastSensorSend = now;
    } else {
      Serial.println("❌ Firebase not ready - skipping sensor send");
    }
  }

  // ======== READ MODE & COMMANDS EVERY 30 SECONDS ========
  if (now - lastCommandRead >= COMMAND_INTERVAL) {
    if (Firebase.ready()) {
      readModeAndControl();
      lastCommandRead = now;
    } else {
      Serial.println("❌ Firebase not ready - skipping mode read");
    }
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

  Serial.println("Sending sensor data...");
  
  // ======== KIRIM SENSOR CURRENT ========
  // airTemp
  FirebaseJson json_temp;
  json_temp.set("value", t);
  json_temp.set("timestamp", timestamp);
  bool ok_temp = Firebase.RTDB.setJSON(&fbdo, "/sensors/airTemp/current", &json_temp);
  Serial.printf("  airTemp: %s (%.1f°C)\n", ok_temp ? "✅" : "❌", t);
  
  // airHumidity
  FirebaseJson json_humid;
  json_humid.set("value", h);
  json_humid.set("timestamp", timestamp);
  bool ok_humid = Firebase.RTDB.setJSON(&fbdo, "/sensors/airHumidity/current", &json_humid);
  Serial.printf("  airHumidity: %s (%.1f%%)\n", ok_humid ? "✅" : "❌", h);
  
  // soilMoisture
  FirebaseJson json_soil;
  json_soil.set("value", soilPercent);
  json_soil.set("timestamp", timestamp);
  bool ok_soil = Firebase.RTDB.setJSON(&fbdo, "/sensors/soilMoisture/current", &json_soil);
  Serial.printf("  soilMoisture: %s (%.1f%%)\n", ok_soil ? "✅" : "❌", soilPercent);
  
  // light
  FirebaseJson json_light;
  json_light.set("value", lux);
  json_light.set("timestamp", timestamp);
  bool ok_light = Firebase.RTDB.setJSON(&fbdo, "/sensors/light/current", &json_light);
  Serial.printf("  light: %s (%.1f lux)\n", ok_light ? "✅" : "❌", lux);

  // ======== KIRIM SERIES DATA ========
  Firebase.RTDB.setJSON(&fbdo, "/sensors/airTemp/series/" + String(timestamp), &json_temp);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/airHumidity/series/" + String(timestamp), &json_humid);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/soilMoisture/series/" + String(timestamp), &json_soil);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/light/series/" + String(timestamp), &json_light);

  Serial.println();
}

// =======================
//   BACA MODE & CONTROL
// =======================
void readModeAndControl() {
  Serial.println("Reading mode and commands...");

  // ======== BACA SYSTEM MODE ========
  String mode = "auto";
  if (Firebase.RTDB.getString(&fbdo, "/system/mode")) {
    String modeData = fbdo.stringData();
    if (modeData.length() > 0) {
      mode = modeData;
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
  FirebaseJson json_fan, json_pump, json_lamp;
  json_fan.set("isOn", fanOn);
  json_pump.set("isOn", pumpOn);
  json_lamp.set("isOn", lampOn);
  
  Firebase.RTDB.setJSON(&fbdo, "/actuators/fan/state", &json_fan);
  Firebase.RTDB.setJSON(&fbdo, "/actuators/pump/state", &json_pump);
  Firebase.RTDB.setJSON(&fbdo, "/actuators/lamp/state", &json_lamp);

  Serial.println("===== ACTUATOR STATUS =====");
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
  if (Firebase.RTDB.getJSON(&fbdo, "/actuators/lamp/command")) {
    FirebaseJson json = fbdo.to<FirebaseJson>();
    FirebaseJsonData jsonData;
    json.get(jsonData, "isOn");
    if (jsonData.success) {
      lampOn = jsonData.boolValue;
    }
  }

  // Baca fan command
  if (Firebase.RTDB.getJSON(&fbdo, "/actuators/fan/command")) {
    FirebaseJson json = fbdo.to<FirebaseJson>();
    FirebaseJsonData jsonData;
    json.get(jsonData, "isOn");
    if (jsonData.success) {
      fanOn = jsonData.boolValue;
    }
  }

  // Baca pump command
  if (Firebase.RTDB.getJSON(&fbdo, "/actuators/pump/command")) {
    FirebaseJson json = fbdo.to<FirebaseJson>();
    FirebaseJsonData jsonData;
    json.get(jsonData, "isOn");
    if (jsonData.success) {
      pumpOn = jsonData.boolValue;
    }
  }

  Serial.println("Commands received from website");
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
  Serial.printf("  Lux < %.1f? %s → Lamp %s\n", LUX_LOW, (lux < LUX_LOW) ? "YES" : "NO", lampOn ? "ON" : "OFF");
}
