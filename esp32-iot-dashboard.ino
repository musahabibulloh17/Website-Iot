#include <DHT.h>

// =======================
//       PIN SENSOR
// =======================
#define DHTPIN   23     // tetap D23
#define DHTTYPE  DHT22
#define SOIL_PIN 34     // sensor soil (input only, boleh)
#define LDR_PIN  35     // LDR (input only, boleh)

DHT dht(DHTPIN, DHTTYPE);

// =======================
//   KALIBRASI SOIL & LDR
// =======================
int airValue   = 3000;  // tanah kering
int waterValue = 1200;  // tanah basah
float Rfixed = 10000.0;

// =======================
//     PIN AKTUATOR (PAKAI LABEL Dxx YANG AMAN)
// =======================
// L298N Channel A → Kipas
#define FAN_IN1   16
#define FAN_IN2   17
#define FAN_ENA   5    // Enable A (dulu sering GPIO5 / D5)

// L298N Channel B → Pompa
#define PUMP_IN1  26   // D26 sangat aman
#define PUMP_IN2  27   // D27 sangat aman
#define PUMP_ENB  4    // Enable B (D4 juga aman dan banyak dipakai)

// Lampu LED
#define LAMP_PIN  25

// =======================
//  BATAS KENDALI
// =======================
const float TEMP_HIGH        = 32.0;
const float HUMID_LOW        = 71.0;
const float SOIL_LOW_PERCENT = 50.0;
const float LUX_LOW          = 22.0;

// =======================
//   FUNGSI MOTOR
// =======================
void setFan(bool on) {
  digitalWrite(FAN_IN1, on ? HIGH : LOW);
  digitalWrite(FAN_IN2, on ? LOW  : LOW);
  digitalWrite(FAN_ENA, HIGH);   // selalu enable (atau PWM nanti)
}

void setPump(bool on) {
  digitalWrite(PUMP_IN1, on ? HIGH : LOW);
  digitalWrite(PUMP_IN2, on ? LOW  : LOW);
  digitalWrite(PUMP_ENB, HIGH);
}

void setLamp(bool on) {
  digitalWrite(LAMP_PIN, on ? HIGH : LOW);
}

// =======================
//        SETUP
// =======================
void setup() {
  Serial.begin(115200);
  dht.begin();

  // Pin aktuator
  pinMode(FAN_IN1,  OUTPUT);
  pinMode(FAN_IN2,  OUTPUT);
  pinMode(FAN_ENA,  OUTPUT);
  pinMode(PUMP_IN1, OUTPUT);
  pinMode(PUMP_IN2, OUTPUT);
  pinMode(PUMP_ENB, OUTPUT);
  pinMode(LAMP_PIN, OUTPUT);

  // Enable selalu HIGH
  digitalWrite(FAN_ENA, HIGH);
  digitalWrite(PUMP_ENB, HIGH);

  // Matikan semua dulu
  setFan(false);
  setPump(false);
  setLamp(false);

  delay(2000);
  Serial.println("Sistem siap dengan pin Dxx!");
}

// =======================
//         LOOP
// =======================
void loop() {
  delay(2000);

  // DHT22
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("ERROR DHT22!");
    h = 0; t = 0;
  }

  // Soil
  int soilRaw = analogRead(SOIL_PIN);
  float soilPercent = map(constrain(soilRaw, waterValue, airValue), waterValue, airValue, 100, 0);
  soilPercent = constrain(soilPercent, 0, 100);

  // LDR → Lux
  int ldrRaw = analogRead(LDR_PIN);
  float Vout = ldrRaw * (3.3 / 4095.0);
  Vout = constrain(Vout, 0.01, 3.29);
  float Rldr = (3.3 - Vout) * Rfixed / Vout;
  float R_k = max(Rldr / 1000.0, 0.1);
  float lux = 500.0 / pow(R_k, 1.4);

  // Logika kendali
  bool fanOn  = (t > TEMP_HIGH || h < HUMID_LOW);
  bool pumpOn = (soilPercent < SOIL_LOW_PERCENT);
  bool lampOn = (lux < LUX_LOW);

  setFan(fanOn);
  setPump(pumpOn);
  setLamp(lampOn);

  // Print
  Serial.println("===== SENSOR =====");
  Serial.printf("Suhu       : %.1f °C\n", t);
  Serial.printf("Kelembapan : %.1f %%\n", h);
  Serial.printf("Tanah      : %.1f %%\n", soilPercent);
  Serial.printf("Cahaya     : %.1f lux\n", lux);
  Serial.println("----- AKTUATOR -----");
  Serial.printf("Kipas (D16-D17) : %s\n", fanOn  ? "ON " : "OFF");
  Serial.printf("Pompa (D26-D27) : %s\n", pumpOn ? "ON " : "OFF");
  Serial.printf("Lampu (D25)     : %s\n", lampOn ? "ON " : "OFF");
  Serial.println("========================\n");
}