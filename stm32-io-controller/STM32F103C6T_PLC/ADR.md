# ADR — STM32F103C6T_PLC

## ADR-001: Blocking HTTP Client (tanpa RTOS)
**Status:** Accepted
**Date:** 2026-11-11

**Context:**
Firmware harus mengirim 3 POST API (door, temp, humidity). STM32F103C6 punya RAM 20KB — FreeRTOS (stack + heap) akan memakan ~5-10KB. W5500 sudah punya driver TCP blocking.

**Decision:**
Gunakan main loop blocking berbasis `HAL_GetTick()` tanpa RTOS:
- Setiap POST HTTP blocking (socket 0, TCP)
- Poll Modbus tiap 5 detik, kirim 2 POST saat data ada
- Door state change via EXTI → set flag → kirim di main loop
- `HAL_Delay(1)` antar iterasi

**Consequences:**
- ✅ RAM kecil, sederhana, mudah debug
- ✅ Cukup untuk 3 event terpisah
- ❌ POST mem-block main loop (Modbus poll delay sampai HTTP selesai)
- ❌ Tidak bisa handle 2 concurrent connection

**Alternatif yang ditolak:** FreeRTOS (overkill untuk project ini, RAM terpakai banyak)

---

## ADR-002: EXTI untuk Door Sensor, Polling untuk Modbus
**Status:** Accepted
**Date:** 2026-11-11

**Context:**
Door sensor butuh respon cepat saat berubah. Modbus poll periodik (5 detik) tidak butuh interrupt.

**Decision:**
- **Door PC13:** EXTI13_IRQn interrupt → set flag → main loop kirim POST
- **Modbus:** Polling di main loop berdasarkan timestamp `HAL_GetTick()`

**Consequences:**
- ✅ Door responsif, tidak memblok
- ✅ Modbus sederhana, tidak perlu ISR UART
- ✅ Debounce door: cooldown 2 detik di main loop (tidak di ISR)

---

## ADR-003: CRC16-Modbus Lookup Table
**Status:** Accepted
**Date:** 2026-11-11

**Context:**
CRC16-Modbus (polynomial 0xA001) untuk Modbus RTU. Hitung software bit-by-bit atau lookup table?

**Decision:**
Lookup table 256 entries (precomputed, ~512 bytes Flash + 512 bytes RAM).
Alasan: 512 bytes RAM masih aman untuk STM32F103C6 (20KB total).

**Consequences:**
- ✅ Fast (2 lookup per byte)
- ✅ Deterministic timing
- ❌ +512 bytes RAM vs bit-by-bit

---

## ADR-004: HTTP Socket 0, Blocking, Tidak Keep-Alive
**Status:** Accepted
**Date:** 2026-11-11

**Context:**
W5500 punya 8 socket. Socket 6 sudah dipakai DNS oleh W5500_Init. Sisa: 0-5.

**Decision:**
- Gunakan **socket 0** untuk semua HTTP POST
- Setiap POST: open → connect → send → recv → close (fresh connection)
- Timeout W5500: 5 detik per operasi

**Consequences:**
- ✅ Sederhana, tidak ada stateful connection
- ✅ Tidak ada resource leak
- ❌ Overhead TCP handshake per POST (negligible untuk skala ini)
- ❌ Jika API hang > 5 detik → timeout → log error, coba lagi cycle berikutnya

---

## ADR-005: 3 POST Terpisah (Bukan Gabung)
**Status:** Accepted
**Date:** 2026-11-11

**Context:**
Format API per curl_api_door.txt dan curl_api_th.txt: setiap event = 1 POST terpisah.

**Decision:**
Kirim 3 POST terpisah sesuai format yang sudah ada di API server:
1. `event: "door"` — saat state change
2. `event: "temperature"` — setiap poll Modbus
3. `event: "humidity"` — setiap poll Modbus

**Consequences:**
- ✅ Sesuai format API yang sudah ada
- ✅ Tidak perlu ubah API server
- ❌ 3 connection sequential per cycle (jika semua trigger bersamaan)

---

## ADR-006: Flash via STM32CubeProgrammer CLI
**Status:** Accepted
**Date:** 2026-11-11

**Context:**
MCP tool stm32_flash memakai OpenOCD. User mau pakai STM32CubeProgrammer CLI.

**Decision:**
Build: CubeIDE headless via MCP `stm32_build_and_flash`
Flash: `"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\STM32CubeProgrammer_CLI.exe" --port SWD --write <path>.elf --verify`

**Consequences:**
- ✅ Gunakan tool yang sudah user punya
- ✅ Verify flash contents otomatis
