# AGENTS.md — STM32F103C6T_PLC

## Aturan Utama

1. **Jangan overthinking.** Sederhana, langsung, blocking. Sesuai ADR-001: no RTOS, no dynamic allocation, no async.

2. **Hardware pasti jalan.** ST-Link terhubung, Blue Pill STM32F103C6T6 siap. Anggap semua koneksi fisik sudah benar — fokus ke code, bukan debug koneksi SWD/USB.

3. **Ikuti dokumen.**
   - **PRD.md** → apa yang mau di-build (features, JSON format, acceptance criteria)
   - **ADR.md** → keputusan yang sudah diambil (tidak boleh diabaikan atau diganti tanpa alasan kuat)
   - **TECH_STACK.md** → tooling, pin map, protokol, how-to build/flash/monitor

4. **File yang harus ada:**
   - `Core/Src/main.c` — main loop, GPIO/UART init, API calls
   - `Core/Src/stm32f1xx_it.c` — EXTI callback PC13
   - `Core/Src/door_sensor.c/h` — door sensor logic
   - `Core/Src/modbus_rtu.c/h` — Modbus RTU master + CRC16
   - `Core/Src/http_client.c/h` — W5500 TCP HTTP POST

5. **API endpoint & header (fix, dari curl_api_*.txt):**
   - Host: `192.168.1.105`, Port: `1880`, Path: `/rest/v1/event`
   - Header: `x-api-key: CHANGE_ME_API_KEY`, `Content-Type: application/json`

6. **Modbus (fix, default RN-WS-N01):**
   - Unit ID: `0x01`, Func: `0x03`, Register `0`=temp, Register `1`=humidity
   - Temp: integer × 10 (value 325 = 32.5°C)
   - Poll interval: 5 detik

7. **Door sensor (fix):**
   - PC13, internal pull-up
   - LOW = `"open"`, HIGH = `"closed"`
   - Debounce/cooldown: 2 detik antar POST
   - EXTI13 interrupt → set flag → main loop kirim

8. **HTTP (fix, sesuai ADR-004):**
   - W5500 socket 0, TCP, blocking
   - Fresh connection per POST (open→connect→send→recv→close)
   - Timeout 5 detik
   - Tidak ada retry logic — log error, cycle berikutnya coba lagi

9. **Flash workflow:**
   - Build: MCP `stm32_build_and_flash`
   - Flash: STM32CubeProgrammer CLI (lihat TECH_STACK.md)
   - Monitor: MCP serial + live_memory (lihat TECH_STACK.md)

10. **Jangan:**
    - Tambah file baru yang belum ada di list item 4
    - Ubah W5500 driver di `Drivers/Ethernet_W5500/`
    - Pakai `malloc`/`free`
    - Ubah .ioc atau regenerator CubeMX
    - Skip ADR tanpa konfirmasi user
