# 🎯 QUICK REFERENCE - IoT Dashboard

## 🚀 Start Here

### 1. Dashboard (5 menit)
```bash
npm install
npm run dev
# Open http://localhost:5174/
```

### 2. Firebase Setup (10 menit)
1. Go: https://console.firebase.google.com
2. Create project "iot-dashboard"
3. Create Realtime Database (asia-southeast1)
4. Copy credentials to `.env`
5. Change `VITE_MODE=firebase`

### 3. ESP32 Upload (10 menit)
1. Open `esp32-iot-dashboard.ino`
2. Update WiFi SSID & password
3. Update Firebase host & secret
4. Upload to ESP32

---

## 📊 Data Structure

```json
{
  "sensors": {
    "light": {
      "current": { "value": 75.5, "timestamp": 1234567890 },
      "series": [{ "timestamp": 1234567890, "value": 75.5 }]
    }
  },
  "actuators": {
    "lamp": {
      "state": { "isOn": false, "timestamp": 1234567890 },
      "command": { "isOn": false, "timestamp": 1234567890 }
    }
  },
  "system": {
    "mode": { "mode": "auto", "timestamp": 1234567890 }
  }
}
```

---

## 🔌 ESP32 Pins

| Component | Pin | Type |
|-----------|-----|------|
| Light | 34 | Analog |
| Humidity | 35 | Analog |
| Soil | 32 | Analog |
| Temp | 33 | Analog |
| Lamp | 13 | Digital |
| Fan | 14 | Digital |
| Pump | 15 | Digital |

---

## 🧪 Test Commands

### Send Sensor Data (cURL)
```bash
curl -X PUT \
  -d '{"value":80.5,"timestamp":'$(date +%s)'000}' \
  "https://YOUR_DB.firebaseio.com/sensors/light/current.json"
```

### Read Sensor Data
```bash
curl "https://YOUR_DB.firebaseio.com/sensors/light/current.json"
```

### Send Actuator Command
```bash
curl -X PUT \
  -d '{"isOn":true,"timestamp":'$(date +%s)'000}' \
  "https://YOUR_DB.firebaseio.com/actuators/lamp/command.json"
```

---

## 🔧 Environment Variables

| Variable | Example |
|----------|---------|
| VITE_FIREBASE_API_KEY | AIzaSy... |
| VITE_FIREBASE_AUTH_DOMAIN | iot-xxx.firebaseapp.com |
| VITE_FIREBASE_PROJECT_ID | iot-dashboard-ca956 |
| VITE_FIREBASE_DATABASE_URL | https://xxx.firebaseio.com |
| VITE_MODE | firebase \| demo |

---

## 🎨 UI Components

| Component | File | Purpose |
|-----------|------|---------|
| SensorCard | components/SensorCard.tsx | Display sensor value |
| TimeSeriesChart | components/TimeSeriesChart.tsx | Show 12-hour graph |
| ActuatorPanel | components/ActuatorPanel.tsx | Control buttons |

---

## 📱 Firebase Functions

| Function | Purpose |
|----------|---------|
| subscribeToSensorCurrent() | Read current sensor value |
| subscribeSensorTimeSeries() | Read historical data |
| subscribeActuatorState() | Read actuator status |
| updateActuatorCommand() | Send command to actuator |
| updateSystemMode() | Change auto/manual mode |
| subscribeSystemMode() | Listen to mode changes |

---

## 🐛 Debugging

### Check Firebase Connection
```bash
# Browser console
F12 → Console
# Look for errors or "Connected to Firebase"
```

### Check ESP32 Serial
```
Arduino IDE → Tools → Serial Monitor (115200)
# Should show WiFi connected and data sent
```

### Check Database
Firebase Console → Realtime Database → data structure visible

---

## ⚡ Performance

| Metric | Value |
|--------|-------|
| Update Interval | 1 minute |
| Graph Window | 12 hours |
| Grid Resolution | 1 hour |
| Data Points | 1/minute |
| WebSocket Latency | <100ms |

---

## ✅ Checklist

Before production:

- [ ] All `.env` variables filled
- [ ] Firebase rules configured
- [ ] ESP32 connected to WiFi
- [ ] Dashboard shows "Connected to Firebase"
- [ ] Sensor data flowing
- [ ] Actuator control working
- [ ] No console errors
- [ ] 12-hour graph has data

---

## 📚 Files

| File | Purpose |
|------|---------|
| App.tsx | Main component with Firebase |
| firebase.ts | Firebase integration |
| esp32-iot-dashboard.ino | ESP32 code |
| theme.css | UI styling |
| .env | Configuration |

---

## 🆘 Emergency Reset

```bash
# Clear cache
rm -rf node_modules package-lock.json
npm install

# Kill port 5174
# Windows: netstat -ano | findstr :5174
# Mac/Linux: lsof -i :5174

# Restart
npm run dev
```

---

**Need Help? Check these files:**
- README.md - Full documentation
- SETUP_FIREBASE.md - Firebase guide
- PROJECT_COMPLETE.md - Project summary

**🚀 Happy IoT Building!**
