## IoT Realtime Dashboard (React + TypeScript)

Dashboard pemantauan IoT dengan tema gradasi hijau-biru menampilkan:
- Intensitas cahaya, kelembapan udara, kelembapan tanah, dan suhu udara secara realtime
- Grafik per sensor: data per menit, grid 1 jam, jendela 24 jam
- Panel aktuator: Lampu, Kipas, Pompa dengan mode Otomatis/Manual dan kontrol per-perangkat

### Menjalankan
1. Install dependencies:
```bash
npm install
```
2. Jalankan development server:
```bash
npm run dev
```
3. Buka `http://localhost:5173`

### Integrasi Data Realtime Sungguhan
Saat ini data disimulasikan. Untuk integrasi:
- Ganti update di `advanceOneMinute`/interval pada `src/App.tsx` dengan subscription WebSocket dari backend Anda.
- Dorong data baru (per menit) ke `series` masing-masing sensor dengan format `{ timestamp: number, value: number }`.
- Pastikan timestamp dalam milidetik epoch.

### Integrasi Kontrol Aktuator
- Ubah handler `setActuator` dan `setAllActuators` di `src/App.tsx` agar memanggil REST/WebSocket ke backend.
- Pada mode `auto`, status aktuator dikendalikan logika otomatis (placeholder), override jika backend memiliki status tersinkronisasi.

### Teknis
- Bundler: Vite
- UI: CSS custom (tanpa framework)
- Chart: Recharts
- Util: date-fns


