# STM32F103C6T_PLC — Edge IoT & PLC Monitoring System

Sistem monitoring IoT/PLC berbasis **STM32F103C6T6 (Blue Pill)** sebagai *sensor node* dan **BeagleBone Black (Node-RED)** sebagai *edge gateway*, dashboard visualisasi, perekam video event (RTSP), serta penerus data (*telemetry & event*) ke server cloud/pusat.

---

## 📋 Daftar Isi
1. [Arsitektur Sistem](#-arsitektur-sistem)
2. [Fitur Utama](#-fitur-utama)
3. [Hardware & Pinout (STM32)](#-hardware--pinout-stm32)
4. [Konfigurasi Jaringan & API](#-konfigurasi-jaringan--api)
5. [Struktur Proyek & Dokumentasi Terkait](#-struktur-proyek--dokumentasi-terkait)
6. [Panduan Build & Flash Firmware (STM32)](#-panduan-build--flash-firmware-stm32)
7. [Panduan Instalasi Edge Gateway (Node-RED / BeagleBone Black)](#-panduan-instalasi-edge-gateway-node-red--beaglebone-black)
8. [Debugging & Monitoring](#-debugging--monitoring)

---

## 🏗 Arsitektur Sistem

Sistem ini terbagi menjadi dua lapisan utama: **Edge Node (MCU)** dan **Edge Gateway (SBC/Linux)**.

```text
+-----------------------------------+         HTTP POST (JSON)         +-----------------------------------------+
|      STM32F103C6T6 (Blue Pill)    |  ----------------------------->  |    BeagleBone Black (Node-RED Edge)     |
|                                   |     192.168.1.105:1880/rest/v1   |                                         |
| - Door Sensor (PC13 / EXTI13)     |                                  | - REST API Receiver & Auth Check        |
| - Modbus TH Sensor (UART2/RS485)  |                                  | - Event Router (Door, Current, TH)      |
| - W5500 Ethernet (SPI1)           |                                  | - SQLite Local Logger                   |
+-----------------------------------+                                  | - Anomaly Detector (Current Monitoring) |
                                                                       | - RTSP Video Recorder (FFmpeg)          |
+-----------------------------------+         RTSP Stream (554)        | - Dashboard 2.0 UI (/event-viewer)      |
|         IP Camera (RTSP)          |  ----------------------------->  | - Cloud/Server API Sync & Heartbeat     |
+-----------------------------------+                                  +-----------------------------------------+
```

### Keputusan Desain Utama (Ringkasan ADR)
* **Bare-metal Blocking Loop (Tanpa RTOS):** Menghemat penggunaan RAM pada STM32F103C6 (total 20 KB RAM) dengan main loop berbasis `HAL_GetTick()` tanpa overhead stack FreeRTOS.
* **Interrupt-Driven Door Sensor:** Menggunakan `EXTI13` untuk deteksi perubahan status pintu secara instan dengan *software debounce* (cooldown 2 detik) di main loop.
* **Modbus RTU Polling:** Pembacaan sensor suhu & kelembapan dilakukan secara periodik setiap 5 detik menggunakan *Lookup Table* CRC16-Modbus agar eksekusi cepat dan deterministik.
* **Single-Socket Stateless HTTP:** W5500 menggunakan Socket 0 secara bergantian (*open → connect → send → recv → close*) dengan timeout 5 detik per transaksi untuk mencegah *resource leak*.

---

## ✨ Fitur Utama

### 1. Edge Node Firmware (`STM32F103C6T_PLC`)
* **Deteksi Status Pintu (*Real-time*):** Mendeteksi perubahan logika pada pin `PC13` (`open` / `closed`) dan langsung mengirimkan payload event melalui HTTP POST.
* **Akuisisi Data Suhu & Kelembapan:** Membaca register Modbus RTU (Function Code `0x03`, Unit ID `0x01`) dari sensor `RN-WS-N01-2D-LCD` setiap 5 detik.
* **Fault-Tolerant Loop:** Jika koneksi RS485 terputus atau API server tidak merespons (> 5 detik), firmware mencatat error via UART1 tanpa mengalami *hang*.

### 2. Edge Gateway & Dashboard (`Node-RED`)
* **REST API Endpoint & Autentikasi:** Menerima payload JSON pada endpoint `POST /rest/v1/event` dengan validasi header `x-api-key`.
* **Penyimpanan Lokal (SQLite):** Menyimpan log historis sensor ke dalam database `/sqlite/energy-monitoring/sqlite`.
* **Deteksi Anomali Arus Listrik:** Menggunakan algoritma *Exponential Moving Average (EMA)* untuk mendeteksi kondisi `WARNING`, `CRITICAL`, atau `ZERO` pada sensor arus (`AmpPhase1`).
* **Perekaman Video Otomatis (FFmpeg):** Merekam klip video berdurasi 10 detik dari kamera RTSP setiap kali terjadi event penting (pintu terbuka/tertutup atau anomali arus kritis) dan menyimpannya ke `/data/`.
* **Dashboard 2.0 Event Viewer:** Antarmuka web interaktif untuk memantau grafik arus listrik, memutar ulang rekaman video kejadian, dan melihat daftar riwayat event.
* **Integrasi Cloud & Heartbeat:** Mengirimkan batch telemetri, event, unggahan video multipart, serta *system heartbeat* (uptime & sisa disk) setiap 10 detik ke server pusat.

---

## 🔌 Hardware & Pinout (STM32)

* **MCU:** STM32F103C6T6 (ARM Cortex-M3, 72 MHz, 20 KB RAM, 32 KB Flash)
* **Framework:** STM32CubeIDE 1.18.1 / STM32 HAL v1.8.7

| Komponen / Modul | Peripheral | Pin STM32 | Keterangan |
| :--- | :--- | :--- | :--- |
| **Door Sensor** | GPIO / EXTI13 | `PC13` | Input Pull-up (`LOW` = open, `HIGH` = closed) |
| **Modbus RS485 (TH)** | USART2 | `PA2` (TX), `PA3` (RX) | Sensor `RN-WS-N01-2D-LCD`, 115200 8N1, Unit `0x01` |
| **Ethernet W5500** | SPI1 + GPIO | `PA7` (MOSI), `PA6` (MISO), `PA5` (SCK), `PA4` (CS), `PA1` (RST) | Static IP `192.168.1.135` |
| **Serial Debug Log** | USART1 | `PA9` (TX), `PA10` (RX) | `printf` redirect, Baudrate 115200 |
| **Programmer** | SWD | `PA13` (SWDIO), `PA14` (SWCLK) | ST-Link V2 |

---

## 🌐 Konfigurasi Jaringan & API

### Parameter Jaringan STM32 (W5500)
* **IP Address:** `192.168.1.135`
* **Subnet Mask:** `255.255.255.0`
* **Gateway:** `192.168.1.1`
* **DNS:** `8.8.8.8`
* **MAC Address:** `AA:BB:CC:DD:EE:FF`

### Spesifikasi Endpoint REST API (Node-RED)
* **URL Target:** `POST http://192.168.1.105:1880/rest/v1/event`
* **Headers:**
  ```http
  Content-Type: application/json
  x-api-key: CHANGE_ME_API_KEY
  ```
* **Contoh Payload Event Pintu:**
  ```json
  {
    "status": 200,
    "data": {
      "event": "door",
      "state": "open"
    }
  }
  ```
* **Contoh Payload Suhu & Kelembapan (Dikirim Terpisah):**
  ```json
  {
    "status": 200,
    "data": {
      "event": "temperature",
      "unit": "celcius",
      "value": 32.5
    }
  }
  ```
  ```json
  {
    "status": 200,
    "data": {
      "event": "humidity",
      "unit": "percentage",
      "value": 65
    }
  }
  ```

---

## 📁 Struktur Proyek & Dokumentasi Terkait

* `PRD.md` — *Product Requirements Document* (spesifikasi fitur, hardware, dan kriteria penerimaan).
* `ADR.md` — *Architecture Decision Records* (catatan keputusan teknis firmware STM32).
* `TECH_STACK.md` — Rincian periferal, arsitektur kode, dan perintah build/flash/monitoring.
* `nodered-flows.json` — Konfigurasi lengkap alur kerja (*flows*) dan Dashboard Node-RED.
* `panduan-instalasi-nodered-bbb.md` — Panduan langkah demi langkah instalasi sistem di BeagleBone Black.

---

## 🛠 Panduan Build & Flash Firmware (STM32)

### Prasyarat
* **STM32CubeIDE** (v1.18.1 atau kompatibel)
* **STM32CubeProgrammer** (`STM32_Programmer_CLI.exe`)
* ST-Link terhubung ke pin SWD Blue Pill dan board dalam keadaan menyala.

### Build & Flash via CLI (PowerShell)
```powershell
$CLI = "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
$ELF = "C:\D\MY\DEV\STM32CubeIDE\workspace_1.18.1\STM32F103C6T_PLC\Debug\STM32F103C6T_PLC.elf"

& $CLI --connect port=SWD --write $ELF --verify
```
*(Setelah proses verifikasi selesai, tekan tombol **RESET** pada board Blue Pill untuk menjalankan firmware).*

---

## 🚀 Panduan Instalasi Edge Gateway (Node-RED / BeagleBone Black)

Untuk menyiapkan server Node-RED di **BeagleBone Black** (user `debian`), jalankan langkah ringkas berikut (lihat `panduan-instalasi-nodered-bbb.md` untuk detail lengkap):

1. **Instal Dependensi Sistem (`ffmpeg` & `sqlite3`):**
   ```bash
   sudo apt-get update
   sudo apt-get install -y ffmpeg sqlite3 build-essential python3
   ```

2. **Siapkan Direktori Video & Database:**
   ```bash
   sudo mkdir -p /data /sqlite/energy-monitoring
   sudo chown -R debian:debian /data /sqlite/energy-monitoring
   ```

3. **Instal Library Node-RED:**
   ```bash
   cd ~/.node-red
   npm install @flowfuse/node-red-dashboard@1.35.0 node-red-node-sqlite@2.0.1 node-red-contrib-condition-monitoring@0.3.3
   ```

4. **Aktifkan `httpStatic` di `~/.node-red/settings.js`:**
   ```javascript
   httpStatic: '/data/',
   ```

5. **Restart Node-RED & Import Flow:**
   ```bash
   sudo systemctl restart nodered
   ```
   Buka editor di `http://<IP_BEAGLEBONE>:1880`, import file `nodered-flows.json`, klik **Deploy**, lalu klik tombol inject **`CREATE TABLE SENSOR LOGS`** pada tab **Insert DB** untuk inisialisasi tabel database.

6. **Akses Dashboard:**
   Buka browser dan arahkan ke `http://<IP_BEAGLEBONE>:1880/dashboard/event-viewer`.

---

## 🔍 Debugging & Monitoring

* **Serial Log (STM32):** Hubungkan USB-to-TTL ke `PA9` (TX) / `PA10` (RX) dengan baudrate `115200` untuk memantau status inisialisasi W5500, hasil pembacaan Modbus, dan status pengiriman HTTP POST.
* **Live Memory Watch (SWD):** Variabel global berikut dapat dipantau secara *real-time* melalui ST-Link tanpa menghentikan eksekusi program:
  * `door_state_changed`
  * `modbus_last_temp`
  * `modbus_last_humid`
* **Node-RED Debug Sidebar:** Aktifkan node `debug` pada tab *Service*, *Router*, atau *Integration* di editor Node-RED untuk menginspeksi payload masuk dan respons API.