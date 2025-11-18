/**
 * IoT Dashboard - ESP32 Firmware
 * 
 * Sesuai dengan Firebase structure di website:
 * - Sensor data: /sensors/{key}/current
 * - Time series: /sensors/{key}/series
 * - Actuator commands: /actuators/{key}/command
 * - System mode: /system/mode
 * 
 * Compatible dengan: Website React Dashboard
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
unsigned long lastSeriesSave = 0;
const unsigned long SENSOR_INTERVAL = 60000;      // 1 menit
const unsigned long COMMAND_INTERVAL = 30000;     // 30 detik
const unsigned long SERIES_INTERVAL = 60000;      // 1 menit

// Current state
bool fanOn = false;
bool pumpOn = false;
bool lampOn = false;
String currentMode = "AUTO";

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

  // Timestamp saat ini (milliseconds)
  unsigned long timestamp = millis();

  // ======== KIRIM SENSOR CURRENT KE FIREBASE ========
  // Path: /sensors/{key}/current
  // Format: { value: number, timestamp: number }
  
  FirebaseJson json_temp;
  json_temp.set("value", t);
  json_temp.set("timestamp", timestamp);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/airTemp/current", &json_temp);
  
  FirebaseJson json_humid;
  json_humid.set("value", h);
  json_humid.set("timestamp", timestamp);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/airHumidity/current", &json_humid);
  
  FirebaseJson json_soil;
  json_soil.set("value", soilPercent);
  json_soil.set("timestamp", timestamp);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/soilMoisture/current", &json_soil);
  
  FirebaseJson json_light;
  json_light.set("value", lux);
  json_light.set("timestamp", timestamp);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/light/current", &json_light);

  // ======== KIRIM SERIES DATA ========
  // Path: /sensors/{key}/series/{timestamp}
  // Format: { value: number, timestamp: number }
  
  FirebaseJson series_temp;
  series_temp.set("value", t);
  series_temp.set("timestamp", timestamp);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/airTemp/series/" + String(timestamp), &series_temp);
  
  FirebaseJson series_humid;
  series_humid.set("value", h);
  series_humid.set("timestamp", timestamp);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/airHumidity/series/" + String(timestamp), &series_humid);
  
  FirebaseJson series_soil;
  series_soil.set("value", soilPercent);
  series_soil.set("timestamp", timestamp);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/soilMoisture/series/" + String(timestamp), &series_soil);
  
  FirebaseJson series_light;
  series_light.set("value", lux);
  series_light.set("timestamp", timestamp);
  Firebase.RTDB.setJSON(&fbdo, "/sensors/light/series/" + String(timestamp), &series_light);

  // ======== DEBUG SERIAL ========
  Serial.println("===== SENSOR DATA SENT =====");
  Serial.printf("Suhu       : %.1f °C\n", t);
  Serial.printf("Kelembapan : %.1f %%\n", h);
  Serial.printf("Tanah      : %.1f %%\n", soilPercent);
  Serial.printf("Cahaya     : %.1f lux\n", lux);
  Serial.println("=============================\n");
}

// =======================
//   BACA MODE & COMMAND
// =======================
void readModeAndCommands() {
  // ======== BACA MODE ========
  // Path: /system/mode
  Firebase.RTDB.getString(&fbdo, "/system/mode");
  if (fbdo.stringData() != "") {
    currentMode = fbdo.stringData();
  }

  if (currentMode == "MANUAL") {
    // ======== MODE MANUAL - BACA COMMAND ========
    // Path: /actuators/{key}/command
    // Format: { isOn: boolean }
    
    bool mLamp = false, mFan = false, mPump = false;

    Firebase.RTDB.getJSON(&fbdo, "/actuators/lamp/command");
    if (fbdo.to<FirebaseJson>().get("isOn").success()) {
      mLamp = fbdo.to<FirebaseJson>().getBool("isOn");
    }

    Firebase.RTDB.getJSON(&fbdo, "/actuators/fan/command");
    if (fbdo.to<FirebaseJson>().get("isOn").success()) {
      mFan = fbdo.to<FirebaseJson>().getBool("isOn");
    }

    Firebase.RTDB.getJSON(&fbdo, "/actuators/pump/command");
    if (fbdo.to<FirebaseJson>().get("isOn").success()) {
      mPump = fbdo.to<FirebaseJson>().getBool("isOn");
    }

    setLamp(mLamp);
    setFan(mFan);
    setPump(mPump);

    Serial.println("===== MANUAL MODE - COMMAND RECEIVED =====");
    Serial.printf("Lampu: %s | Fan: %s | Pompa: %s\n", 
      mLamp ? "ON" : "OFF",
      mFan ? "ON" : "OFF", 
      mPump ? "ON" : "OFF"
    );
  }
}

// =======================
//   AUTO MODE LOGIC
// =======================
void autoModeLogic() {
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
  bool fanAutoOn  = (t > TEMP_HIGH || h < HUMID_LOW);
  bool pumpAutoOn = (soilPercent < SOIL_LOW_PERCENT);
  bool lampAutoOn = (lux < LUX_LOW);

  setFan(fanAutoOn);
  setPump(pumpAutoOn);
  setLamp(lampAutoOn);

  Serial.println("===== AUTO MODE LOGIC =====");
  Serial.printf("Temp > %.1f°C? %s\n", TEMP_HIGH, (t > TEMP_HIGH) ? "YES" : "NO");
  Serial.printf("Humid < %.1f%%? %s\n", HUMID_LOW, (h < HUMID_LOW) ? "YES" : "NO");
  Serial.printf("Soil < %.1f%%? %s\n", SOIL_LOW_PERCENT, (soilPercent < SOIL_LOW_PERCENT) ? "YES" : "NO");
  Serial.printf("Lux < %.1f? %s\n", LUX_LOW, (lux < LUX_LOW) ? "YES" : "NO");
  Serial.println("===========================\n");
}

// =======================
//   KIRIM STATUS AKTUATOR
// =======================
void sendActuatorStatus() {
  // Path: /actuators/{key}/state
  // Format: { isOn: boolean }
  
  FirebaseJson json_lamp;
  json_lamp.set("isOn", lampOn);
  Firebase.RTDB.setJSON(&fbdo, "/actuators/lamp/state", &json_lamp);
  
  FirebaseJson json_fan;
  json_fan.set("isOn", fanOn);
  Firebase.RTDB.setJSON(&fbdo, "/actuators/fan/state", &json_fan);
  
  FirebaseJson json_pump;
  json_pump.set("isOn", pumpOn);
  Firebase.RTDB.setJSON(&fbdo, "/actuators/pump/state", &json_pump);

  Serial.println("===== ACTUATOR STATUS SENT =====");
  Serial.printf("Lampu: %s | Fan: %s | Pompa: %s\n", 
    lampOn ? "ON" : "OFF",
    fanOn ? "ON" : "OFF", 
    pumpOn ? "ON" : "OFF"
  );
  Serial.println("=================================\n");
}

// =======================
//        SETUP
// =======================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n===== ESP32 IoT Dashboard Starting =====\n");

  dht.begin();

  // Setup Pin Outputs
  pinMode(FAN_IN1, OUTPUT);
  pinMode(FAN_IN2, OUTPUT);
  pinMode(FAN_ENA, OUTPUT);

  pinMode(PUMP_IN1, OUTPUT);
  pinMode(PUMP_IN2, OUTPUT);
  pinMode(PUMP_ENB, OUTPUT);

  pinMode(LAMP_PIN, OUTPUT);

  // Inisial State
  setFan(false);
  setPump(false);
  setLamp(false);

  // -------- WIFI ---------
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  int wifiRetry = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetry < 20) {
    delay(300);
    Serial.print(".");
    wifiRetry++;
  }
  Serial.println("\nWiFi Connected!");
  Serial.printf("IP: %s\n\n", WiFi.localIP().toString().c_str());

  // -------- FIREBASE ---------
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Dummy credentials (Firebase Realtime DB tidak butuh auth jika rules allow)
  auth.user.email = "test@test.com";
  auth.user.password = "12345678";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase Connected!");
  Serial.println("======================================\n");
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
    readModeAndCommands();

    // Jika AUTO mode, jalankan logic auto
    if (currentMode == "AUTO") {
      autoModeLogic();
    }

    // Kirim status aktuator
    sendActuatorStatus();

    lastCommandRead = now;
  }

  delay(100);
}
