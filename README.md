# IoT Edge & Door Sensor Monitoring System

![STM32](https://img.shields.io/badge/MCU-STM32F103C6T6-03234B?style=flat-square&logo=stmicroelectronics&logoColor=white)
![BeagleBone](https://img.shields.io/badge/Edge-BeagleBone_Black-E66000?style=flat-square&logo=linux&logoColor=white)
![Node-RED](https://img.shields.io/badge/Flow-Node--RED-8F0000?style=flat-square&logo=nodered&logoColor=white)
![SQLite](https://img.shields.io/badge/Database-SQLite-003B57?style=flat-square&logo=sqlite&logoColor=white)
![FFmpeg](https://img.shields.io/badge/Video-FFmpeg_RTSP-007808?style=flat-square&logo=ffmpeg&logoColor=white)

An IoT monitoring system built with **STM32** as the I/O controller (door and environmental sensors) and **BeagleBone Black** as the **Edge IoT Computer** that receives events, records video from an RTSP camera, and displays a monitoring dashboard.

## 📂 Repository Structure

```text
.
├── edge-iot-computer/       # Node-RED service on BeagleBone Black (API, DB, RTSP recorder, Dashboard)
└── stm32-io-controller/     # STM32F103C6T6 firmware (Door Sensor & Modbus TH over Ethernet W5500)
```

## 🔗 How It Works & Connections

```text
[Door Sensor & Modbus TH] 
           │
           ▼
[stm32-io-controller] ──(HTTP POST /rest/v1/event)──▶ [edge-iot-computer (BeagleBone)]
   IP: 192.168.1.135                                      IP: 192.168.1.105:1880
                                                                   │
                                                                   ├──▶ Save logs to SQLite
                                                                   ├──▶ Record RTSP camera video (FFmpeg)
                                                                   └──▶ Display on the Dashboard UI
```

1. **`stm32-io-controller` (Sensor Node):**
   * Detects door state changes (*open/closed*) in real time on pin `PC13` (EXTI) and reads temperature/humidity sensors via Modbus RTU.
   * Connects to the local network using the **W5500** Ethernet module (`192.168.1.135`) and sends events via **HTTP POST** to the Edge IoT Computer.

2. **`edge-iot-computer` (BeagleBone Black):**
   * Runs **Node-RED** (`192.168.1.105:1880`), which provides the REST API endpoint `/rest/v1/event` (validated with an `x-api-key` header).
   * When an event arrives from `stm32-io-controller` (for example, the door opens), the system logs it to **SQLite**, triggers **RTSP** video recording via `ffmpeg`, updates the **Dashboard** UI, and syncs the data to the central server.

## 🚀 Quick Start

### 1. Set up `edge-iot-computer/` (BeagleBone Black)
1. Install system dependencies (`ffmpeg`, `sqlite3`) and create the `/data` and `/sqlite/energy-monitoring` directories.
2. Install the Node-RED palettes: `@flowfuse/node-red-dashboard`, `node-red-node-sqlite`, and `node-red-contrib-condition-monitoring`.
3. Enable `httpStatic: '/data/'` in `~/.node-red/settings.js`, then restart the Node-RED service.
4. Import `nodered-flows.json` and click **Deploy**.
5. Open the dashboard at `http://192.168.1.105:1880/dashboard/event-viewer`.

### 2. Set up `stm32-io-controller/` (STM32F103 Blue Pill)
1. Make sure the hardware wiring matches:
   * **Door Sensor:** `PC13` (pull-up)
   * **W5500 Ethernet:** `SPI1` (`PA4`–`PA7`, `RST=PA1`)
   * **Modbus RS485:** `USART2` (`PA2`, `PA3`)
2. Build and flash the firmware using **STM32CubeIDE** or the **STM32CubeProgrammer CLI** via ST-Link (SWD).
3. Connect the LAN cable; the board will automatically send an event whenever the door state changes.
