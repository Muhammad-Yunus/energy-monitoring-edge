# Tech Stack — STM32F103C6T_PLC

## MCU
- **STM32F103C6T6** (Cortex-M3, 72 MHz, 20 KB RAM, 32 KB Flash)
- Board: Blue Pill STM32F103
- Clock: HSE 8 MHz → PLL ×9 → SYSCLK 72 MHz, APB1 36 MHz, APB2 72 MHz

## Firmware / Framework
- **STM32CubeIDE 1.18.1** — project generated via STM32CubeMX
- **STM32 HAL** (v1.8.7) — `stm32f1xx_hal`
- Build config: **Debug** (dengan printf via UART1)

## Peripherals Used
| Peripheral | Pins | Kegunaan |
|---|---|---|
| GPIO PC13 | PC13 | Door sensor input (pull-up) |
| EXTI13 | PC13 | Interrupt on state change |
| USART1 | PA9(TX), PA10(RX) | Log/printf, 115200 |
| USART2 | PA2(TX), PA3(RX) | Modbus RTU master, 115200 8N1 |
| SPI1 | PA7(MOSI), PA6(MISO), PA5(SCK) | W5500, 18 Mbit/s |
| GPIO PA4 | PA4 | W5500 CS (software) |
| GPIO PA1 | PA1 | W5500 RST |
| RTC (LSE) | PC14/PC15 | Real-time clock (sudah ada, belum dipakai) |

## Network
- **W5500** Ethernet MAC, IP static:
  - IP: `192.168.1.135`, Subnet: `255.255.255.0`, GW: `192.168.1.1`, DNS: `8.8.8.8`
  - MAC: `AA:BB:CC:DD:EE:FF`
- API target: `192.168.1.105:1880`

## Libraries / Drivers
| Library | Source | Kegunaan |
|---|---|---|
| WizNet W5500 | `Drivers/Ethernet_W5500/` | Socket TCP, SPI, netinfo |
| STM32 HAL | `Drivers/STM32F1xx_HAL_Driver/` | GPIO, UART, SPI, NVIC |
| CMSIS | `Drivers/CMSIS/` | Device headers |

## Protocols
| Protocol | Spesifikasi |
|---|---|
| **HTTP/1.1 POST** | W5500 TCP socket, raw HTTP request |
| **Modbus RTU** | Master, Unit 0x01, Func 0x03, 115200 8N1, CRC16 |
| **JSON** | REST API body format |

## Build & Deploy

### How to Build
```
# Opsi 1: Via MCP (rekomendasi — build + flash + verify + reset dalam 1 call)
MCP: stm32_build_and_flash(
  project_path="C:\D\MY\DEV\STM32CubeIDE\workspace_1.18.1\STM32F103C6T_PLC",
  configuration="Debug",
  verify=true,
  reset=true
)

# Opsi 2: Build saja, flash manual
MCP: stm32_build(
  project_path="C:\D\MY\DEV\STM32CubeIDE\workspace_1.18.1\STM32F103C6T_PLC",
  configuration="Debug"
)
```

### How to Flash (STM32_Programmer_CLI)
> Path CLI: `C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`
> (bukan `STM32CubeProgrammer_CLI.exe` — nama file yang benar adalah `STM32_Programmer_CLI.exe`)

```powershell
$CLI = "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
$ELF = "C:\D\MY\DEV\STM32CubeIDE\workspace_1.18.1\STM32F103C6T_PLC\Debug\STM32F103C6T_PLC.elf"

& $CLI --connect port=SWD --write $ELF --verify
# Tekan tombol RESET pada Blue Pill untuk menjalankan firmware
```

Syntax CLI:
| Command | Fungsi |
|---|---|
| `--connect port=SWD` | Connect via SWD |
| `--write <file>` | Flash file (`.elf`, `.bin`, `.hex`) |
| `--verify` | Verify flash contents |
| `--start` | Start MCU execution |
| `--go` | Run (jalankan dari address tertentu) |
| `--connect port=SWD mode=HWRSTPULSE` | Force reset saat connect |

Prasyarat:
- ST-Link terpasang via USB
- Blue Pill ON (LED PWR menyala)
- Koneksi SWD: SWDIO→PA13, SWCLK→PA14, GND→GND, 3.3V→3.3V

### How to Monitor
```
# 1. Cek board terhubung
MCP: stm32_list_probes

# 2. Cek info board (voltage, CPU, flash size)
MCP: stm32_board_info(probe="<nickname-atau-sn>")

# 3. Baca log UART (W5500 init, error message, dll)
MCP: serial_list_ports          → cari port (ST-Link VCP ditandai [ST-Link VCP])
MCP: serial_connect(port="COM5")
MCP: serial_read(connection_id="COM5", timeout=3.0)
MCP: serial_send(connection_id="COM5", data="PING", read_response=true)

# 4. Live memory monitoring (SWD, tanpa serial — real-time)
MCP: live_memory_start(
  elf_path="C:\D\MY\DEV\STM32CubeIDE\workspace_1.18.1\STM32F103C6T_PLC\Debug\STM32F103C6T_PLC.elf",
  variables='[
    "door_state_changed",
    "modbus_last_temp",
    "modbus_last_humid"
  ]',
  interval_ms=500
)
MCP: live_memory_read(session_id="<id>", last_n=20)
MCP: live_memory_stop(session_id="<id>")

# 5. On-demand memory read (SWD)
MCP: stm32_read_memory(symbol="door_state_changed", elf_path=".../Debug/STM32F103C6T_PLC.elf")
MCP: stm32_read_memory(symbol="modbus_last_temp",  elf_path=".../Debug/STM32F103C6T_PLC.elf")

# 6. Multi-step serial test sequence (timing-sensitive)
MCP: serial_sequence(steps='[
  {"send": "POLL_TEMP", "to": "COM5", "expect": "TEMP=", "read_timeout": 3.0},
  {"delay_ms": 200},
  {"send": "POLL_HUMID", "to": "COM5", "expect": "HUMID=", "read_timeout": 3.0}
]')
```

## Architecture (per ADR-001: no RTOS)
```
main()
├─ W5500_Init()
├─ MX_GPIO_Init()      → PC13 pull-up, EXTI13
├─ MX_USART2_Init()    → Modbus UART
├─ Modbus_Init()
├─ DoorSensor_Init()
│
while(1)
├─ if door_state_changed
│  └─ HTTP_Post(door_json)     ← blocking, socket 0
├─ if tick ≥ last_th_poll+5000
│  ├─ Modbus_Read_Holding(0x01, 0, 2, raw)
│  ├─ HTTP_Post(temp_json)
│  └─ HTTP_Post(humidity_json)
└─ HAL_Delay(1)
```

## Constraints
- **RAM:** 20 KB — semua buffer di RAM, hati-hati
- **Flash:** 32 KB — CRC table + HTTP buffer + code < 32 KB
- **No dynamic allocation** — semua static
- **Blocking I/O** — tidak ada async
- **W5500 timeout:** set via `setsockopt`, default 5 detik
