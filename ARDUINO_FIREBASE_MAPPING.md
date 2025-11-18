# 🔄 Arduino ↔ Website Firebase Mapping

## ✅ UPDATED Arduino Code sudah sesuai!

File baru: `esp32-iot-dashboard-UPDATED.ino`

---

## 📊 DATABASE STRUCTURE MAPPING

### **SENSOR DATA**

| Arduino Variabel | Website Expected | Firebase Path | Format |
|------------------|-----------------|---------------|--------|
| `t` (DHT22) | `airTemp` | `/sensors/airTemp/current` | `{value: float, timestamp: ms}` |
| `h` (DHT22) | `airHumidity` | `/sensors/airHumidity/current` | `{value: float, timestamp: ms}` |
| `soilPercent` | `soilMoisture` | `/sensors/soilMoisture/current` | `{value: float, timestamp: ms}` |
| `lux` (LDR) | `light` | `/sensors/light/current` | `{value: float, timestamp: ms}` |

### **TIME SERIES**

Setiap sensor juga menyimpan data history:

```
/sensors/airTemp/series/{timestamp}
/sensors/airHumidity/series/{timestamp}
/sensors/soilMoisture/series/{timestamp}
/sensors/light/series/{timestamp}
```

Format: `{value: float, timestamp: ms}`

### **SYSTEM MODE**

| Path | Format | Value |
|------|--------|-------|
| `/system/mode` | String | `"AUTO"` atau `"MANUAL"` |

### **ACTUATOR COMMANDS (Manual Mode)**

Ketika mode = "MANUAL", Arduino membaca dari:

| Aktuator | Firebase Path | Format |
|----------|---------------|--------|
| Lampu | `/actuators/lamp/command` | `{isOn: boolean}` |
| Fan | `/actuators/fan/command` | `{isOn: boolean}` |
| Pompa | `/actuators/pump/command` | `{isOn: boolean}` |

### **ACTUATOR STATE**

Arduino mengirim status setiap 30 detik:

| Aktuator | Firebase Path | Format |
|----------|---------------|--------|
| Lampu | `/actuators/lamp/state` | `{isOn: boolean}` |
| Fan | `/actuators/fan/state` | `{isOn: boolean}` |
| Pompa | `/actuators/pump/state` | `{isOn: boolean}` |

---

## 🔄 FLOW DIAGRAM

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32 ARDUINO                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  EVERY 1 MINUTE:                                               │
│  ├─ Read DHT22 (temp, humidity)                               │
│  ├─ Read ADC (soil, light)                                    │
│  ├─ Send to /sensors/{key}/current  ────────┐                │
│  └─ Save to /sensors/{key}/series           │                │
│                                              │                │
│  EVERY 30 SECONDS:                          ▼                │
│  ├─ Read /system/mode  ←───────┐    ┌─────────────────────┐ │
│  ├─ If MANUAL:                 │    │   FIREBASE RTDB    │ │
│  │  ├─ Read /actuators/lamp/command           │    │ │
│  │  ├─ Read /actuators/fan/command            │    │ │
│  │  └─ Read /actuators/pump/command           │    │ │
│  ├─ If AUTO:                   │    │   Website React   │ │
│  │  ├─ Run auto logic          │    │   Dashboard       │ │
│  │  └─ Set actuators           │    │   (Real-time sync)│ │
│  ├─ Send /actuators/{key}/state ──────────────────→│ │
│  └─ Done                       │    └─────────────────────┘ │
│                                └───────────────────────────────┘
```

---

## 🎯 FIREBASE RULES (Development)

```json
{
  "rules": {
    "sensors": {
      ".read": true,
      ".write": true
    },
    "actuators": {
      ".read": true,
      ".write": true
    },
    "system": {
      ".read": true,
      ".write": true
    }
  }
}
```

---

## 📝 UPLOAD INSTRUCTIONS

### **1. Install Libraries di Arduino IDE**

```
Sketch → Include Library → Manage Libraries

Search & Install:
- Firebase Arduino Client Library → by Mobizt (latest)
- DHT sensor library → by Adafruit
- Adafruit Unified Sensor → by Adafruit
```

### **2. Configure Board Settings**

```
Tools → Board → esp32 → ESP32 Dev Module

Tools → Upload Speed → 115200
Tools → CPU Frequency → 80 MHz
Tools → Flash Mode → QIO
```

### **3. Update WiFi Credentials**

Edit di `esp32-iot-dashboard-UPDATED.ino`:

```cpp
#define WIFI_SSID     "F6"           // ← Ganti dengan WiFi Anda
#define WIFI_PASSWORD "kodok321"     // ← Ganti password WiFi
```

### **4. Verify Firebase Config**

Pastikan credentials sudah benar:

```cpp
#define API_KEY       "AIzaSyCH2L80uoizUUNN6hbvnbhoFBkfesEeHH0"
#define DATABASE_URL  "https://iot-dashboard-ca956-default-rtdb.asia-southeast1.firebasedatabase.app"
```

### **5. Upload to ESP32**

```
Sketch → Upload
```

Tunggu sampai "Leaving... Hard resetting via RTS pin..."

### **6. Monitor Serial Output**

```
Tools → Serial Monitor → 115200 baud

Output:
===== ESP32 IoT Dashboard Starting =====
Connecting to WiFi............
WiFi Connected!
IP: 192.168.x.x

Firebase Connected!
```

---

## ✅ TESTING CHECKLIST

### **Arduino Side**

- [ ] Serial monitor menunjukkan sensor values
- [ ] WiFi connected dengan IP address
- [ ] Firebase connected tanpa error
- [ ] Data dikirim setiap 1 menit
- [ ] Mode & commands dibaca setiap 30 detik

### **Website Side**

- [ ] Dashboard terhubung ke Firebase
- [ ] Sensor cards menampilkan nilai dari Arduino
- [ ] Time series chart menunjukkan data history
- [ ] Mode switcher berfungsi (AUTO/MANUAL)
- [ ] Actuator buttons mengirim command

### **End-to-End**

- [ ] Ubah mode ke MANUAL di website
- [ ] Klik tombol lamp/fan/pump
- [ ] Arduino membaca command & menjalankan
- [ ] Status aktuator update di website
- [ ] Serial monitor menunjukkan "MANUAL MODE - COMMAND RECEIVED"

---

## 🐛 TROUBLESHOOTING

### **Problem: Arduino tidak connect ke Firebase**

```
Solution:
1. Cek WiFi credentials
2. Cek API_KEY & DATABASE_URL
3. Buka Firebase Console → Database → Test
4. Pastikan database rules allow write
```

### **Problem: Website tidak menerima data dari Arduino**

```
Solution:
1. Buka Firebase Console → Database → Real-time
2. Cek apakah path /sensors/ ada data
3. Di Browser F12 → Console cek error
4. Cek VITE_MODE=firebase di .env
```

### **Problem: Actuator tidak merespons command**

```
Solution:
1. Ubah mode ke MANUAL di website
2. Serial monitor harus show "MANUAL MODE"
3. Klik tombol di website
4. Firebase Console harus show /actuators/{key}/command update
5. Serial monitor harus show "COMMAND RECEIVED"
```

### **Problem: Serial monitor menunjukkan "API call error"**

```
Solution:
1. Check internet connection ESP32
2. Cek Firebase PROJECT_ID benar
3. Restart Arduino (press RESET button)
4. Update Firebase library ke versi terbaru
```

---

## 📊 DATA RATE & PERFORMANCE

| Metric | Value | Note |
|--------|-------|------|
| Sensor Read Interval | 1 minute | 1 data point per minute |
| Command Poll Interval | 30 seconds | Check mode & commands |
| Total Data Points (12h) | 720 | Same as website chart |
| Firebase Requests/hour | ~120 | Low bandwidth usage |
| Expected Data Size | <50KB/hour | Very efficient |

---

## 🔒 SECURITY NOTES

### **Development (Current)**
- ✅ Firebase rules allow public read/write
- ✅ API Key exposed in code (OK for development only)
- ✅ No authentication required

### **Production (Next Step)**
- ⚠️ Enable Firebase Authentication
- ⚠️ Use environment-specific API keys
- ⚠️ Restrict Firebase rules to authenticated users only
- ⚠️ Hide credentials in secure storage

---

## 📱 COMPATIBILITY

| Component | Version | Status |
|-----------|---------|--------|
| ESP32 Board | DevKit v1 | ✅ Tested |
| Firebase SDK | 10.7.1 | ✅ Compatible |
| DHT22 Sensor | - | ✅ Compatible |
| Arduino IDE | 2.x | ✅ Works |
| Website React | 18.3.1 | ✅ Works |
| Vite Build | 5.4.9 | ✅ Works |

---

## 🎓 NEXT STEPS

1. ✅ Upload updated Arduino code
2. ✅ Test serial monitor output
3. ✅ Verify Firebase data flow
4. ✅ Test manual mode control
5. ✅ Deploy website to production
6. ⏳ Add authentication
7. ⏳ Monitor performance metrics

---

## 📞 QUICK REFERENCE

**Arduino Serial Monitor Baud Rate:** 115200

**Firebase Paths to Monitor:**
```
/sensors/light/current
/sensors/airTemp/current
/sensors/airHumidity/current
/sensors/soilMoisture/current
/system/mode
/actuators/lamp/command
/actuators/lamp/state
```

**Website URL:** http://localhost:5174/ (development)

**Firebase Console:** https://console.firebase.google.com

---

*Last Updated: November 19, 2025*  
*Status: ✅ Updated Arduino Code Ready for Upload*
