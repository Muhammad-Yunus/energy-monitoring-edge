# PRD — STM32F103C6T_PLC

## 1. Overview
Firmware untuk STM32F103 Blue Pill yang berfungsi sebagai node IoT/PLC sederhana:
- **Door sensor** (PC13) → kirim event ke REST API saat state berubah
- **Modbus TH sensor** (RN-WS-N01-2D-LCD via RS485/UART2) → poll tiap 5 detik, kirim ke REST API

## 2. Hardware
| Komponen | Pin | Keterangan |
|---|---|---|
| Door sensor | PC13 | Input, pull-up, LOW=open / HIGH=closed |
| Modbus TH | UART2 TX=PA2, RX=PA3 | RN-WS-N01-2D-LCD, 115200 8N1, Unit ID 0x01 |
| W5500 | SPI1: MOSI=PA7, MISO=PA6, SCK=PA5, CS=PA4, RST=PA1 | Static IP 192.168.1.135 |
| Console/log | UART1: TX=PA9, RX=PA10 | printf redirect, 115200 |
| ST-Link | SWD | Build & flash |

## 3. Features

### F1 — Door State Change Notification
- **Trigger:** edge detection on PC13 (LOW↔HIGH)
- **Debounc:** minimum 2 detik antar POST
- **POST body:**
  ```json
  {"status":200,"data":{"event":"door","state":"open"}}
  {"status":200,"data":{"event":"door","state":"closed"}}
  ```

### F2 — Modbus Temperature & Humidity Polling
- **Poll interval:** 5 detik
- **Modbus:** Func 0x03, Unit 0x01, Register 0-1
  - Reg 0 = temperature × 10 (integer) → /10 = °C
  - Reg 1 = humidity (%)
- **POST body (2 POST terpisah):**
  ```json
  {"status":200,"data":{"event":"temperature","unit":"celcius","value":32.5}}
  {"status":200,"data":{"event":"humidity","unit":"percentage","value":65}}
  ```

### F3 — REST API Communication
- **Endpoint:** `POST http://192.168.1.105:1880/rest/v1/event`
- **Headers:** `Content-Type: application/json`, `x-api-key: CHANGE_ME_API_KEY`
- **Transport:** W5500 TCP socket (blocking, satu per satu)

## 4. Non-Goals
- Tidak ada auth/OTP
- Tidak ada retry logic (API failure → log, skip, coba lagi di cycle berikutnya)
- Tidak ada persistent storage (semua in-RAM)
- Tidak ada OTA update

## 5. Acceptance Criteria
1. Board boot → W5500 init sukses → log IP via UART1
2. Gerak door sensor → POST diterima API dalam < 2 detik
3. Poll Modbus tiap 5 detik → 2 POST (temp + humidity) diterima API
4. Jika RS485 tidak terhubung → log error, firmware tidak hang
5. Jika API tidak reachable → log error, firmware tidak hang
