# IoT Dashboard - Setup Firebase & ESP32

## 🎯 Arsitektur Sistem

```
ESP32 IoT Device
    ↓ (HTTP POST / MQTT)
Firebase Realtime Database
    ↓ (WebSocket / REST)
React Dashboard (Realtime Updates)
```

## 📋 Setup Firebase

### 1. Buat Firebase Project

1. Buka [Firebase Console](https://console.firebase.google.com)
2. Klik "Create Project"
3. Masukkan nama project (misal: `iot-dashboard`)
4. Ikuti wizard setup

### 2. Setup Realtime Database

1. Di Firebase Console, pilih project Anda
2. Masuk ke "Build" → "Realtime Database"
3. Klik "Create Database"
4. Pilih region (misal: asia-southeast1)
5. Mulai dalam "Test mode" untuk development

### 3. Konfigurasi Database Rules

Edit rules agar ESP32 bisa menulis data:

```json
{
  "rules": {
    "sensors": {
      ".read": true,
      ".write": true,
      "$sensor": {
        ".validate": true
      }
    },
    "actuators": {
      ".read": true,
      ".write": true,
      "$actuator": {
        ".validate": true
      }
    },
    "system": {
      ".read": true,
      ".write": true
    }
  }
}
```

### 4. Dapatkan Credentials

1. Klik tombol ⚙️ (Settings) di samping project name
2. Pilih "Project Settings"
3. Tab "Service Accounts" → "Database Secrets"
4. Copy `databaseURL`
5. Tab "General" → Cari "Web Apps" section
6. Copy Firebase config credentials

### 5. Isi File `.env.local`

```bash
# Copy dari .env.example
cp .env.example .env.local
```

Edit `.env.local`:

```env
VITE_FIREBASE_API_KEY=your_api_key_here
VITE_FIREBASE_AUTH_DOMAIN=your-project.firebaseapp.com
VITE_FIREBASE_PROJECT_ID=your-project-id
VITE_FIREBASE_STORAGE_BUCKET=your-project.appspot.com
VITE_FIREBASE_MESSAGING_SENDER_ID=123456789012
VITE_FIREBASE_APP_ID=1:123456789012:web:abc123def456ghi789
VITE_FIREBASE_DATABASE_URL=https://your-project.firebaseio.com

VITE_MODE=firebase
```

## 📱 Setup ESP32 Arduino Code

### 1. Install Arduino IDE & Libraries

- Download [Arduino IDE](https://www.arduino.cc/en/software)
- Install library:
  - ESP32 Board Manager
  - ArduinoJson by Benoit Blanchon
  - Firebase Arduino Library

### 2. Upload Code ke ESP32

```cpp
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi Configuration
const char* SSID = "YOUR_WIFI_SSID";
const char* PASSWORD = "YOUR_WIFI_PASSWORD";

// Firebase Configuration
const char* FIREBASE_HOST = "https://your-project.firebaseio.com";
const char* FIREBASE_SECRET = "your-database-secret";

// Sensor Pins
const int LIGHT_PIN = 34;
const int HUMIDITY_PIN = 35;
const int SOIL_PIN = 32;
const int TEMP_PIN = 33;

// Actuator Pins
const int LAMP_PIN = 13;
const int FAN_PIN = 14;
const int PUMP_PIN = 15;

// Interval pengiriman data (ms)
const unsigned long UPDATE_INTERVAL = 60000; // 1 menit

unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Setup sensor pins
  pinMode(LIGHT_PIN, INPUT);
  pinMode(HUMIDITY_PIN, INPUT);
  pinMode(SOIL_PIN, INPUT);
  pinMode(TEMP_PIN, INPUT);
  
  // Setup actuator pins
  pinMode(LAMP_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  
  // Connect to WiFi
  connectToWiFi();
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }
  
  // Update sensors every interval
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();
    
    // Read sensor values
    int lightValue = analogRead(LIGHT_PIN);
    int humidityValue = analogRead(HUMIDITY_PIN);
    int soilValue = analogRead(SOIL_PIN);
    int tempValue = analogRead(TEMP_PIN);
    
    // Convert to readable values
    float light = map(lightValue, 0, 4095, 0, 100);
    float humidity = map(humidityValue, 0, 4095, 30, 100);
    float soil = map(soilValue, 0, 4095, 10, 90);
    float temp = map(tempValue, 0, 4095, 10, 45);
    
    // Send data to Firebase
    sendSensorData("light", light);
    sendSensorData("airHumidity", humidity);
    sendSensorData("soilMoisture", soil);
    sendSensorData("airTemp", temp);
    
    // Check for actuator commands
    checkActuatorCommands();
  }
  
  delay(1000);
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(SSID);
  
  WiFi.begin(SSID, PASSWORD);
  int attempts = 0;
  
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

void sendSensorData(const char* sensorName, float value) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String url = String(FIREBASE_HOST) + "/sensors/" + sensorName + "/current.json";
  
  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["value"] = value;
  doc["timestamp"] = millis();
  
  String payload;
  serializeJson(doc, payload);
  
  // Send PUT request
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.PUT(payload);
  
  if (httpCode > 0) {
    Serial.printf("Sensor %s: %.2f (HTTP %d)\n", sensorName, value, httpCode);
  } else {
    Serial.printf("Error sending sensor %s: %s\n", sensorName, http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

void checkActuatorCommands() {
  // Check lamp command
  checkActuator("lamp", LAMP_PIN);
  checkActuator("fan", FAN_PIN);
  checkActuator("pump", PUMP_PIN);
}

void checkActuator(const char* actuatorName, int pin) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String url = String(FIREBASE_HOST) + "/actuators/" + actuatorName + "/command.json";
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String response = http.getString();
    
    // Parse JSON response
    StaticJsonDocument<200> doc;
    deserializeJson(doc, response);
    
    if (doc.containsKey("isOn")) {
      bool isOn = doc["isOn"].as<bool>();
      digitalWrite(pin, isOn ? HIGH : LOW);
      Serial.printf("%s: %s\n", actuatorName, isOn ? "ON" : "OFF");
    }
  }
  
  http.end();
}
```

### 3. Upload ke ESP32

1. Select Board: "ESP32 Dev Module"
2. Select Port: COM port tempat ESP32 terhubung
3. Upload sketch

## 🚀 Jalankan Dashboard

### Development Mode (Demo)

```bash
npm install
npm run dev
```

Dashboard akan berjalan di `http://localhost:5174` dengan data simulasi.

### Production Mode (Dengan Firebase)

```bash
# Isi .env.local dengan credentials Firebase Anda
VITE_MODE=firebase npm run build
```

## 📊 Database Structure

Firebase Realtime Database harus memiliki struktur:

```
root/
├── sensors/
│   ├── light/
│   │   ├── current: { value, timestamp }
│   │   └── series: [{ timestamp, value }, ...]
│   ├── airHumidity/
│   │   ├── current: { value, timestamp }
│   │   └── series: [...]
│   ├── soilMoisture/
│   │   ├── current: { value, timestamp }
│   │   └── series: [...]
│   └── airTemp/
│       ├── current: { value, timestamp }
│       └── series: [...]
├── actuators/
│   ├── lamp/
│   │   ├── state: { isOn, timestamp }
│   │   └── command: { isOn, timestamp }
│   ├── fan/
│   │   ├── state: { isOn, timestamp }
│   │   └── command: { isOn, timestamp }
│   └── pump/
│       ├── state: { isOn, timestamp }
│       └── command: { isOn, timestamp }
└── system/
    └── mode: { mode: "auto" | "manual", timestamp }
```

## 🔧 Troubleshooting

### ESP32 tidak bisa connect WiFi
- Pastikan SSID dan password benar
- Cek WiFi signal strength
- Restart ESP32

### Firebase credentials tidak valid
- Cek credentials di Firebase Console
- Pastikan database URL benar
- Test dengan Firebase REST API

### Dashboard tidak menerima data
- Check browser console untuk errors
- Pastikan .env.local sudah diisi
- Restart dev server
- Check Firebase rules

## 📚 Resources

- [Firebase Console](https://console.firebase.google.com)
- [Firebase Realtime Database Docs](https://firebase.google.com/docs/database)
- [Arduino IDE Guide](https://www.arduino.cc/en/Guide)
- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
