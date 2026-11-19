# IoT Edge & Door Sensor Monitoring System

![STM32](https://img.shields.io/badge/MCU-STM32F103C6T6-03234B?style=flat-square&logo=stmicroelectronics&logoColor=white)
![BeagleBone](https://img.shields.io/badge/Edge-BeagleBone_Black-E66000?style=flat-square&logo=linux&logoColor=white)
![Node-RED](https://img.shields.io/badge/Flow-Node--RED-8F0000?style=flat-square&logo=nodered&logoColor=white)
![SQLite](https://img.shields.io/badge/Database-SQLite-003B57?style=flat-square&logo=sqlite&logoColor=white)
![FFmpeg](https://img.shields.io/badge/Video-FFmpeg_RTSP-007808?style=flat-square&logo=ffmpeg&logoColor=white)

Sistem monitoring IoT berbasis **STM32** sebagai *I/O controller* (sensor pintu & lingkungan) dan **BeagleBone Black** sebagai *Edge IoT Computer* yang menerima data event, merekam video dari kamera RTSP, serta menampilkan dashboard monitoring.

## 📂 Struktur Repository

```text
.
├── edge-iot-computer/       # Service Node-RED di BeagleBone Black (API, DB, RTSP Recorder, Dashboard)
└── stm32-io-controller/     # Firmware STM32F103C6T6 (Door Sensor & Modbus TH via Ethernet W5500)
```

## 🔗 Cara Kerja & Koneksi Sistem

```text
[Door Sensor & Modbus TH] 
          │
          ▼
[stm32-io-controller] ──(HTTP POST /rest/v1/event)──▶ [edge-iot-computer (BeagleBone)]
   IP: 192.168.1.135                                      IP: 192.168.1.105:1880
                                                                  │
                                                                  ├──▶ Simpan Log ke SQLite
                                                                  ├──▶ Rekam Video Kamera (RTSP)
                                                                  └──▶ Tampilkan di Dashboard UI
```

1. **`stm32-io-controller` (Sensor Node):**
   * Mendeteksi perubahan status pintu (*open/closed*) secara *real-time* melalui pin `PC13` (EXTI) dan membaca sensor suhu/kelembapan via Modbus RTU.
   * Terhubung ke jaringan lokal menggunakan modul Ethernet **W5500** (`192.168.1.135`) dan mengirimkan event melalui **HTTP POST** ke Edge IoT Computer.

2. **`edge-iot-computer` (BeagleBone Black):**
   * Menjalankan **Node-RED** (`192.168.1.105:1880`) yang menyediakan endpoint REST API `/rest/v1/event` (dengan header `x-api-key`).
   * Saat menerima event dari `stm32-io-controller` (misalnya pintu terbuka), sistem otomatis mencatat log ke **SQLite**, memicu perekaman video **RTSP** via `ffmpeg`, memperbarui tampilan **Dashboard**, dan menyinkronkan data ke server pusat.

## 🚀 Quick Start

### 1. Setup `edge-iot-computer/` (BeagleBone Black)
1. Instal dependensi sistem (`ffmpeg`, `sqlite3`) dan buat direktori `/data` serta `/sqlite/energy-monitoring`.
2. Instal palette Node-RED: `@flowfuse/node-red-dashboard`, `node-red-node-sqlite`, dan `node-red-contrib-condition-monitoring`.
3. Aktifkan `httpStatic: '/data/'` pada `~/.node-red/settings.js`, lalu restart service Node-RED.
4. Import file `nodered-flows.json` dan klik **Deploy**.
5. Akses dashboard di `http://192.168.1.105:1880/dashboard/event-viewer`.

### 2. Setup `stm32-io-controller/` (STM32F103 Blue Pill)
1. Pastikan wiring hardware sesuai:
   * **Door Sensor:** `PC13` (Pull-up)
   * **W5500 Ethernet:** `SPI1` (`PA4`–`PA7`, `RST=PA1`)
   * **Modbus RS485:** `USART2` (`PA2`, `PA3`)
2. Build dan flash firmware menggunakan **STM32CubeIDE** atau **STM32CubeProgrammer CLI** via ST-Link (SWD).
3. Hubungkan kabel LAN; board akan otomatis mengirim event setiap kali status pintu berubah.