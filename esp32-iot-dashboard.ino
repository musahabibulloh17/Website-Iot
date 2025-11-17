/*
  ESP32 IoT Dashboard - Sensor & Actuator Controller
  
  Fitur:
  - Membaca 4 sensor (light, humidity, soil moisture, temperature)
  - Mengontrol 3 aktuator (lamp, fan, pump)
  - Send data ke Firebase Realtime Database setiap 1 menit
  - Menerima commands dari Firebase untuk actuator control
  
  Hardware Required:
  - ESP32 Development Board
  - 4x Analog Sensors
  - 3x Relay Modules (untuk actuators)
  
  Setup:
  1. Install Arduino IDE
  2. Add ESP32 board manager
  3. Install ArduinoJson library
  4. Update WiFi & Firebase credentials di bawah
  5. Upload ke ESP32
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ===== WiFi Configuration =====
const char* SSID = "YOUR_WIFI_SSID";
const char* PASSWORD = "YOUR_WIFI_PASSWORD";

// ===== Firebase Configuration =====
// Dapatkan dari Firebase Console > Project Settings
const char* FIREBASE_HOST = "https://your-project-id.firebaseio.com";
const char* FIREBASE_SECRET = "your-database-secret";

// ===== Sensor Pin Configuration =====
// Change pins sesuai dengan ESP32 pin layout Anda
const int LIGHT_PIN = 34;           // Analog pin untuk light sensor
const int HUMIDITY_PIN = 35;        // Analog pin untuk humidity sensor
const int SOIL_MOISTURE_PIN = 32;   // Analog pin untuk soil moisture
const int TEMP_PIN = 33;            // Analog pin untuk temperature sensor

// ===== Actuator Pin Configuration =====
// GPIO pins untuk relay/actuator control
const int LAMP_PIN = 13;    // GPIO untuk lamp (lampu)
const int FAN_PIN = 14;     // GPIO untuk fan (kipas)
const int PUMP_PIN = 15;    // GPIO untuk pump (pompa)

// ===== Update Interval =====
const unsigned long UPDATE_INTERVAL = 60000; // 60 second (1 menit)
unsigned long lastUpdate = 0;

// ===== Actuator State =====
bool lampState = false;
bool fanState = false;
bool pumpState = false;

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n");
  Serial.println("================================");
  Serial.println("ESP32 IoT Dashboard Starting...");
  Serial.println("================================");
  
  // Setup sensor pins (INPUT)
  pinMode(LIGHT_PIN, INPUT);
  pinMode(HUMIDITY_PIN, INPUT);
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(TEMP_PIN, INPUT);
  
  // Setup actuator pins (OUTPUT)
  pinMode(LAMP_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  
  // Initial actuator state OFF
  digitalWrite(LAMP_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);
  
  // Connect to WiFi
  connectToWiFi();
}

// ===== Main Loop =====
void loop() {
  // Check WiFi connection setiap loop
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectToWiFi();
  }
  
  // Send sensor data setiap UPDATE_INTERVAL
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    
    Serial.println("\n--- Reading Sensors ---");
    
    // Read sensor raw values (0-4095 untuk ESP32 ADC)
    int lightRaw = analogRead(LIGHT_PIN);
    int humidityRaw = analogRead(HUMIDITY_PIN);
    int soilRaw = analogRead(SOIL_MOISTURE_PIN);
    int tempRaw = analogRead(TEMP_PIN);
    
    // Convert raw values to readable format
    // Sesuaikan mapping dengan sensor Anda
    float light = map(lightRaw, 0, 4095, 0, 100);              // 0-100 %
    float humidity = map(humidityRaw, 0, 4095, 30, 100);       // 30-100 %
    float soilMoisture = map(soilRaw, 0, 4095, 10, 90);        // 10-90 %
    float temperature = map(tempRaw, 0, 4095, 10, 45);         // 10-45 °C
    
    Serial.printf("Light: %.2f%%\n", light);
    Serial.printf("Humidity: %.2f%%\n", humidity);
    Serial.printf("Soil: %.2f%%\n", soilMoisture);
    Serial.printf("Temp: %.2f°C\n", temperature);
    
    // Send semua sensor data ke Firebase
    sendSensorData("light", light);
    sendSensorData("airHumidity", humidity);
    sendSensorData("soilMoisture", soilMoisture);
    sendSensorData("airTemp", temperature);
    
    // Check untuk commands dari Firebase (control actuator)
    Serial.println("\n--- Checking Actuator Commands ---");
    checkActuatorCommands();
  }
  
  delay(1000); // Check interval 1 detik
}

// ===== WiFi Connection =====
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n✗ Failed to connect to WiFi");
  }
}

// ===== Send Sensor Data ke Firebase =====
void sendSensorData(const char* sensorName, float value) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Skipping sensor upload.");
    return;
  }
  
  HTTPClient http;
  
  // Construct Firebase URL
  String url = String(FIREBASE_HOST) + "/sensors/" + sensorName + "/current.json";
  
  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["value"] = value;
  doc["timestamp"] = millis();
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  // Send PUT request ke Firebase
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  Serial.printf("Sending %s: %.2f to Firebase\n", sensorName, value);
  
  int httpCode = http.PUT(jsonPayload);
  
  if (httpCode > 0) {
    Serial.printf("✓ HTTP Response: %d\n", httpCode);
  } else {
    Serial.printf("✗ HTTP Error: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

// ===== Check Actuator Commands dari Firebase =====
void checkActuatorCommands() {
  // Check command untuk lamp
  updateActuatorState("lamp", LAMP_PIN);
  updateActuatorState("fan", FAN_PIN);
  updateActuatorState("pump", PUMP_PIN);
}

// ===== Update Actuator State =====
void updateActuatorState(const char* actuatorName, int pin) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Skipping command check.");
    return;
  }
  
  HTTPClient http;
  
  // Construct Firebase URL untuk command
  String url = String(FIREBASE_HOST) + "/actuators/" + actuatorName + "/command.json";
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String response = http.getString();
    
    // Parse JSON response
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      if (doc.containsKey("isOn")) {
        bool isOn = doc["isOn"].as<bool>();
        digitalWrite(pin, isOn ? HIGH : LOW);
        
        // Store state
        if (pin == LAMP_PIN) lampState = isOn;
        else if (pin == FAN_PIN) fanState = isOn;
        else if (pin == PUMP_PIN) pumpState = isOn;
        
        Serial.printf("✓ %s: %s\n", actuatorName, isOn ? "ON" : "OFF");
      }
    } else {
      Serial.printf("JSON Parse Error: %s\n", error.c_str());
    }
  } else if (httpCode == 404) {
    // Command not found, set default state
    Serial.printf("✗ %s command not found (404)\n", actuatorName);
  } else {
    Serial.printf("✗ HTTP Error %d for %s\n", httpCode, actuatorName);
  }
  
  http.end();
}

// ===== Utility Functions =====

// Print ESP32 info
void printESP32Info() {
  Serial.println("\n=== ESP32 System Info ===");
  Serial.printf("CPU Freq: %d MHz\n", getCpuFreqMHz());
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("PSRAM Free: %d bytes\n", ESP.getFreePsram());
}

/*
  TESTING & DEBUGGING
  
  Jika dashboard tidak menerima data:
  1. Check serial monitor untuk error messages
  2. Pastikan WiFi connected (lihat IP Address)
  3. Pastikan Firebase credentials benar
  4. Check Firebase Realtime Database rules (pastikan write enabled)
  5. Test Firebase connection dengan Postman/curl
  
  Example curl test:
  curl -X GET "https://your-project.firebaseio.com/sensors/light/current.json"
  
  Manual PUT command:
  curl -X PUT \
    -d '{"value":75.5,"timestamp":1234567890}' \
    https://your-project.firebaseio.com/sensors/light/current.json
*/
