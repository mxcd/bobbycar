# VESC Labs -- Technical Overview & Documentation

> Scraped from [vesclabs.com](https://www.vesclabs.com/) and related VESC ecosystem resources.
> Last updated: 2026-03-28

---

## Table of Contents

- [Company Overview](#company-overview)
- [Product Lineup](#product-lineup)
  - [Motor Controllers](#motor-controllers)
  - [Motor Controller Comparison Table](#motor-controller-comparison-table)
  - [Battery Management Systems (BMS)](#battery-management-systems-bms)
  - [Accessories](#accessories)
- [Communication Protocols](#communication-protocols)
  - [CAN Bus Protocol](#can-bus-protocol)
  - [UART Protocol](#uart-protocol)
  - [Other Interfaces](#other-interfaces)
- [Software Ecosystem](#software-ecosystem)
  - [VESC Tool](#vesc-tool)
  - [VESC Firmware (BLDC)](#vesc-firmware-bldc)
  - [LispBM Scripting](#lispbm-scripting)
- [Fault Codes Reference](#fault-codes-reference)
- [Example Project: Suzuki TS125 Electric Conversion](#example-project-suzuki-ts125-electric-conversion)
- [Choosing a Motor Controller](#choosing-a-motor-controller)
- [Developer Resources & Links](#developer-resources--links)

---

## Company Overview

VESC Labs designs the official range of motor controllers, battery management systems (BMS), and accessories, configured through VESC Tool. The company was founded in 2025 by Benjamin Vedder and several key members of the VESC project. Based in Sweden, all products ship worldwide with final assembly, testing, and programming completed in-house.

**Core philosophy:** "Hardware and Software, Developed as One" -- VESC Labs hardware is developed by the same team behind VESC software.

### Key Team Members

| Name | Role | Background |
|------|------|------------|
| **Benjamin Vedder** | Lead Electrical & Software Engineer, VESC Founder | BEng Automation/Mechatronics (Chalmers 2010), MEng Communication Engineering (Chalmers 2012), PhD Computer Science. Creator of VESC firmware, VESC Tool desktop and mobile. |
| **Jeffrey Friesen** | Lead Mechanical Engineer | Motor controllers for Light EVs, aerospace (800V prototype for Heart Aerospace) |
| **Joel Svensson** | Programmer | Created LispBM, the functional programming language that runs on VESC controllers |

### Core Pillars

- **Open Source Software** -- VESC Tool and firmware enable deep customization
- **Sustainability Focus** -- Products targeting efficient electric propulsion
- **High-Power Engineering** -- Designed for demanding applications
- **Community-Driven Development** -- Collaborative improvement from developers, riders, and engineers

---

## Product Lineup

### Motor Controllers

All VESC motor controllers share these common features:
- **MCU:** STM32F405
- **Motor Control:** Field-Oriented Control (FOC) for brushless motors
- **HFI:** High Frequency Injection for sensorless torque from standstill
- **Sensor Support:** Hall sensors, ABI encoders, SPI-based magnetic encoders
- **IMU:** Integrated 3-axis accelerometer + 3-axis gyroscope
- **Communication:** USB, CAN, SPI, I2C, UART, PWM, ADC
- **Software:** Open-source VESC firmware with LispBM scripting
- **Configuration:** VESC Tool (desktop and mobile)

#### VESC Minim (100V)

- **Voltage:** 12-100V (4-22S)
- **Current:** 50A continuous / 65A burst
- **Power:** ~3 kW
- **Dimensions:** 72 x 43 x 23 mm, 115g
- **Connectors:** XT60 input, 4mm bullet motor outputs
- **Extras:** Three low-side switches with flyback diodes (up to 1.5A @ full Vin)
- **ESP32:** No (add via Nanolog)
- **Weatherproof:** No
- **Datasheet (PDF, image-based):** https://www.vesclabs.com/wp-content/uploads/2025/08/vesc_minim_datasheet.pdf
- **Product page:** https://www.vesclabs.com/product/vl-minim-100v/

#### VESC Classic (100V)

- **Voltage:** 12-100V (4-22S)
- **Current:** 160A continuous / 200A burst
- **Power:** Up to 10 kW
- **Dimensions:** 100 x 48 x 19 mm, 153g
- **Connectors:** XT90 input, 5.5mm bullet motor outputs, JST-GH I/O
- **12V Output:** 0.5A switched
- **ESP32:** No (add via Nanolog)
- **Weatherproof:** No
- **Product page:** https://www.vesclabs.com/product/vesc-classic-100v/

#### VESC Classic Plus (100V)

- **Voltage:** 12-100V (4-22S)
- **Current:** 300A continuous / 400A burst
- **Power:** Up to 20 kW
- **Dimensions:** 107 x 72 x 19 mm, 306g
- **Connectors:** XT90 input, 8mm bullet motor outputs (6AWG), JST-GH I/O
- **CAN Default:** 500 kbps baudrate
- **12V Output:** 0.5A switched
- **ESP32:** No (add via Nanolog)
- **Weatherproof:** No
- **Product page:** https://www.vesclabs.com/product/vesc-classic-plus-100v/

#### VESC Duet XS 60V (Dual Motor)

- **Voltage:** 15-60V (6-13S)
- **Current:** 80A per motor continuous / 100A burst
- **Power:** Up to 5 kW (dual)
- **Dimensions:** 87 x 54 x 18 mm, 163g
- **Connectors:** XT90 input, 4mm bullet motor outputs, JST-GH I/O
- **CAN Default:** 500 kbps baudrate
- **12V Output:** 0.5A switched
- **ESP32:** No (add via Nanolog)
- **Dual FOC:** Yes, fully independent per motor
- **Note:** Encoder support limited to Motor 1 only
- **Weatherproof:** No
- **Product page:** https://www.vesclabs.com/product/vesc-duet-xs60/

#### VESC Duet XS 100V (Dual Motor)

- **Voltage:** 15-100V (6-22S)
- **Current:** 50A per motor continuous / 65A burst
- **Power:** Up to 6 kW (dual)
- **Dimensions:** 87 x 54 x 18 mm, 163g
- **Connectors:** XT90 input, 4mm bullet motor outputs, JST-GH I/O
- **12V Output:** 0.5A switched
- **ESP32:** No (add via Nanolog)
- **Dual FOC:** Yes, fully independent per motor
- **Note:** Encoder support limited to Motor 1 only
- **Weatherproof:** No
- **Product page:** https://www.vesclabs.com/product/vesc-duet-xs/

#### VESC Duet (100V, Dual Motor)

- **Voltage:** 15-100V (6-22S)
- **Current:** 140A per motor continuous / 200A burst
- **Power:** Up to 8 kW (dual)
- **Dimensions:** 134 x 70 x 24 mm, 418g
- **Connectors:** XT90 input, 5.5mm bullet motor outputs (8AWG), JST-PH I/O
- **ESP32:** Yes (ESP32-C3, Wi-Fi + Bluetooth 5.0)
- **Storage:** 4 GB integrated
- **Dual FOC:** Yes, fully independent per motor
- **Note:** Encoder support limited to Motor 1 only, 6 low-side shunts
- **12V Output:** Switched + bus voltage auxiliary outputs
- **Weatherproof:** No
- **Product page:** https://www.vesclabs.com/product/vl-duet/

#### VESC Pronto (100V)

- **Voltage:** 30-100V (8-22S)
- **Current:** 150A continuous / 200A burst
- **Power:** ~10 kW
- **Dimensions:** 121 x 47 x 27 mm, 365g
- **Connectors:** XT90 input, 5.5mm bullet motor outputs (8AWG), **39-pin automotive connector**
- **ESP32:** Yes (ESP32-C3, Wi-Fi + BLE 5, station and AP modes)
- **Storage:** 4 GB integrated
- **12V Output:** 5A switchable, protected
- **ADC:** 5 channels
- **Weatherproof:** Yes (fully potted)
- **Note:** Green PCBs shipped before 2026-03-15 have A/I signal swap on ABI encoder
- **Product page:** https://www.vesclabs.com/product/vl-pronto/

#### VESC Maxim 120V

- **Voltage:** 30-120V (8-26S)
- **Current:** 400A continuous / 600A burst
- **Power:** Up to 30 kW
- **Dimensions:** 126 x 117 x 37 mm, 713g
- **Connectors:** M6 terminal (input + motor phases), **39-pin automotive connector** (pre-crimped, 500mm cables)
- **Fuse:** 350A automotive fuse (included)
- **ESP32:** Yes (ESP32-C3, Wi-Fi + Bluetooth)
- **Storage:** 4 GB integrated
- **12V Output:** 5A switchable
- **ADC:** 5 channels
- **Weatherproof:** Yes (fully potted)
- **Product page:** https://www.vesclabs.com/product/vl-maxim-120v/

#### VESC Maxim 150V

- **Voltage:** 30-150V (8-32S)
- **Current:** 250A continuous / 400A burst
- **Power:** Up to 30 kW
- **Dimensions:** 126 x 117 x 37 mm, 713g
- **Connectors:** M6 terminal, **39-pin automotive connector**
- **Fuse:** 250A automotive fuse (included)
- **ESP32:** Yes (ESP32-C3, Wi-Fi + Bluetooth)
- **Storage:** 4 GB integrated
- **12V Output:** 5A switchable
- **ADC:** 5 channels
- **Weatherproof:** Yes (fully potted)
- **Product page:** https://www.vesclabs.com/product/vl-maxim-150v/

#### VESC Maxim Plus 120V

- **Voltage:** 20-120V (6-26S)
- **Current:** 660A continuous / 1000A burst
- **Power:** Up to 50 kW
- **Dimensions:** 183 x 132 x 37 mm, 1120g
- **Connectors:** M6 terminal, **39-pin automotive connector**
- **Fuse:** 350A automotive fuse (included)
- **ESP32:** Yes (ESP32-C3, Wi-Fi + Bluetooth 5)
- **Storage:** 4 GB integrated
- **12V Output:** 5A switchable, protected
- **ADC:** 5 channels
- **Weatherproof:** Yes (fully potted)
- **Product page:** https://www.vesclabs.com/product/vl-maxim-plus-120v/

#### VESC Maxim Plus 150V

- **Voltage:** 20-150V (6-32S)
- **Current:** 420A continuous / 660A burst
- **Power:** Up to 50 kW
- **Dimensions:** 183 x 132 x 37 mm, 1120g
- **Connectors:** M6 terminal, **39-pin automotive connector**
- **ESP32:** Yes (ESP32-C3, Wi-Fi + Bluetooth 5)
- **Storage:** 4 GB integrated
- **12V Output:** 5A switchable, protected
- **ADC:** 5 channels
- **Weatherproof:** Yes (fully potted)
- **Product page:** https://www.vesclabs.com/product/vl-maxim-plus-150v/

---

### Motor Controller Comparison Table

| Controller | Max V | Series | Cont. A | Burst A | Dual | Power | 12V Out | 5V Out | ESP32 | Storage | IMU | CAN | USB | ADC | Potted | L mm | W mm | H mm | Weight |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Minim | 100V | 4-22S | 50A | 65A | No | 3kW | -- | 2A | No | No | Yes | Yes | USB-C | 3 | No | 72 | 43 | 23 | 115g |
| Duet XS 60V | 60V | 6-13S | 80A | 100A | Yes | 5kW | 0.5A | 1A | No | No | Yes | Yes | USB-C | 3 | No | 87 | 54 | 18 | 163g |
| Duet XS 100V | 100V | 6-22S | 50A | 65A | Yes | 6kW | 0.5A | 1A | No | No | Yes | Yes | USB-C | 3 | No | 87 | 54 | 18 | 163g |
| Duet | 100V | 6-22S | 140A | 200A | Yes | 8kW | 0.8A | 1A | Yes | 4GB | Yes | Yes | USB-C | 3 | No | 134 | 70 | 24 | 418g |
| Classic | 100V | 4-22S | 160A | 200A | No | 10kW | 0.5A | 1A | No | No | Yes | Yes | USB-C | 2 | No | 100 | 48 | 19 | 153g |
| Classic+ | 100V | 4-22S | 300A | 400A | No | 20kW | 0.5A | 1A | No | No | Yes | Yes | USB-C | 2 | No | 107 | 72 | 19 | 306g |
| Pronto | 100V | 8-22S | 150A | 200A | No | 10kW | 8A | 2A | Yes | 4GB | Yes | Yes | dongle | 5 | Yes | 121 | 47 | 27 | 365g |
| Maxim 120V | 120V | 8-26S | 400A | 600A | No | 30kW | 8A | 2A | Yes | 4GB | Yes | Yes | dongle | 5 | Yes | 126 | 117 | 37 | 713g |
| Maxim 150V | 150V | 8-32S | 250A | 400A | No | 30kW | 8A | 2A | Yes | 4GB | Yes | Yes | dongle | 5 | Yes | 126 | 117 | 37 | 713g |
| Maxim+ 120V | 120V | 6-26S | 660A | 1000A | No | 50kW | 8A | 2A | Yes | 4GB | Yes | Yes | dongle | 5 | Yes | 183 | 132 | 37 | 1120g |
| Maxim+ 150V | 150V | 6-32S | 420A | 660A | No | 50kW | 8A | 2A | Yes | 4GB | Yes | Yes | dongle | 5 | Yes | 183 | 132 | 37 | 1120g |

**Notes:**
- All controllers use STM32F405 MCU
- All controllers support CAN bus and USB
- "dongle" = USB-C via dongle adapter (on potted models with 39-pin connector)
- Current ratings assume adequate cooling; burst ratings for up to 60 seconds
- Dual motor controllers run fully independent FOC per motor

---

### Battery Management Systems (BMS)

#### VESC Harmony 16

- **Cell Count:** Up to 16S
- **Price:** From EUR 119.99 (ex VAT)
- **Product page:** https://www.vesclabs.com/product/vl-harmony-16/

#### VESC Harmony 32

- **Cell Count:** 4-32S Li-Ion
- **Max Input Voltage:** 135V (32S)
- **Max Discharge Current:** 600A
- **Max Charge Current:** 30A (configurable)
- **Short Circuit Protection:** 600A, 10us response time
- **Dimensions:** 123 x 71 x 28 mm, 180g
- **MCU:** ESP32-C3 (open-source firmware)
- **Communication:** USB-C, Wi-Fi (STA + AP), BLE 5, Isolated CAN bus
- **CAN Default:** 500 kbps baudrate, ID 3
- **Temperature Sensors:** 4x NTC 10k Ohm inputs (T4 = ambient)
- **Features:**
  - Real-time cell monitoring and balancing (800 mA balance current)
  - Integrated power switch with precharge circuit (3s max precharge)
  - Voltage or coulomb counting state-of-charge
  - Configurable sleep mode with ultra-low current draw
  - LispBM scripting on ESP32-C3
- **Product page:** https://www.vesclabs.com/product/vl-harmony-32/

---

### Accessories

| Accessory | Price (ex VAT) | Key Specs | Link |
|-----------|---------------|-----------|------|
| **VESC Nanolog** | EUR 39.99 | ESP32-C3, Wi-Fi + BT, 4GB storage, CAN (500kbps default, ID 2), UART for GPS, 23x25mm, 4.3g | [Product page](https://www.vesclabs.com/product/vl-nanolog/) |
| **VESC Dash 35B** | EUR 119.99 | 3.5" 480x320 display, 800 nits, IP67, ESP32-C3, CAN (500kbps), Wi-Fi + BLE 5, 4 buttons (IP65), programmable UI | [Product page](https://www.vesclabs.com/product/vesc-dash-35b/) |
| **BE220 GPS Module** | EUR 29.99 | GPS/GNSS receiver, connects via Nanolog UART | [Product page](https://www.vesclabs.com/product/be220-gps-module/) |
| **VESC ABI Encoder** | EUR 34.99 | Motor position encoder | [Product page](https://www.vesclabs.com/product/vl-abi-encoder/) |
| **VESC Sin/Cos Encoder** | EUR 34.99 | Coming soon | [Product page](https://www.vesclabs.com/product/vl-sin-cos-encoder/) |
| **VESC Rmcore** | EUR 39.99 | Remote control core | [Product page](https://www.vesclabs.com/product/vesc-rmcore/) |
| **VESC Link** | EUR 84.99 | Communication module, coming soon | [Product page](https://www.vesclabs.com/product/vl-link/) |
| **ADC Thumb Throttle / Brake** | EUR 14.99 | Analog throttle/brake input | [Product page](https://www.vesclabs.com/product/thumb-throttle/) |
| **ADC Twist Throttle** | EUR 29.99 | Twist-grip analog throttle | [Product page](https://www.vesclabs.com/product/adc-twist-throttle/) |

---

## Communication Protocols

### CAN Bus Protocol

All VESC controllers include CAN bus support. The default baudrate across all VESC Labs products is **500 kbps**.

**Default CAN IDs by device type:**
- Motor controllers: configurable (typically 0 or 1)
- Nanolog: ID 2
- Harmony BMS: ID 3
- Dash 35B: configurable

All devices on the same CAN bus must share the same baudrate and have unique CAN IDs.

#### CAN Frame Format

VESC uses **29-bit extended CAN IDs** structured as:

```
Bit 28-16: Unused (0)
Bit 15-8:  Command ID (CAN_PACKET_ID)
Bit 7-0:   VESC ID (controller ID, configurable per device)
```

#### CAN Operating Modes

Selectable via VESC Tool under App Settings:

| Mode | Description |
|------|-------------|
| **VESC** | Default mode. CAN forwarding and multi-device configuration. |
| **UAVCAN** | Basic `uavcan.equipment.esc` implementation |
| **Comm Bridge** | Bridges CAN bus to commands for generic interface/debugging |
| **Unused** | Frames ignored; custom LispBM scripts can still process them |

#### CAN Timeout Behavior

- Default timeout: **0.5 seconds** without received commands
- On timeout: motor release (default) or configurable brake current
- Recommended: send commands at fixed intervals (e.g., 50 Hz)
- Configurable via App Settings > General

#### CAN Single-Frame Commands

All simple commands use **4 data bytes** representing 32-bit big-endian signed integers with scaling factors.

| Command | ID | Scaling | Unit | Range |
|---------|-----|---------|------|-------|
| `CAN_PACKET_SET_DUTY` | 0 | 100000 | %/100 | -1.0 to 1.0 |
| `CAN_PACKET_SET_CURRENT` | 1 | 1000 | A | -MAX to MAX |
| `CAN_PACKET_SET_CURRENT_BRAKE` | 2 | 1000 | A | -MAX to MAX |
| `CAN_PACKET_SET_RPM` | 3 | 1 | RPM | -MAX to MAX |
| `CAN_PACKET_SET_POS` | 4 | 1000000 | Degrees | 0-360 |
| `CAN_PACKET_SET_CURRENT_REL` | 10 | 100000 | %/100 | -1.0 to 1.0 |
| `CAN_PACKET_SET_CURRENT_BRAKE_REL` | 11 | 100000 | %/100 | -1.0 to 1.0 |
| `CAN_PACKET_SET_CURRENT_HANDBRAKE` | 12 | 1000 | A | -MAX to MAX |
| `CAN_PACKET_SET_CURRENT_HANDBRAKE_REL` | 13 | 100000 | %/100 | -1.0 to 1.0 |

**Out-of-range handling:** Commands exceeding configured limits are clamped to boundaries (not rejected). E.g., requesting 60A brake when limit is 50A results in 50A applied.

Extended variants support off-delay parameters for keeping controllers active below minimum current thresholds.

#### CAN Status Messages

Six status message types transmit telemetry at configurable rates. Activate via VESC Tool: App Settings > General > CAN Status Messages Rate.

Two sets of messages can be enabled independently, each transmitted at a specified rate, each containing any combination of status messages.

| Status Message | ID | Data Fields (scaling) |
|---------------|-----|----------------------|
| `CAN_PACKET_STATUS` | 9 | ERPM (1x), Current (10x), Duty Cycle (1000x) |
| `CAN_PACKET_STATUS_2` | 14 | Amp Hours (10000x), Amp Hours Charged (10000x) |
| `CAN_PACKET_STATUS_3` | 15 | Watt Hours (10000x), Watt Hours Charged (10000x) |
| `CAN_PACKET_STATUS_4` | 16 | Temp FET (10x C), Temp Motor (10x C), Current In (10x A), PID Pos (50x deg) |
| `CAN_PACKET_STATUS_5` | 27 | Tachometer (6x EREV), Voltage In (10x V) |
| `CAN_PACKET_STATUS_6` | 58 | ADC1-3 (1000x V each), PPM (1000x %/100) |

#### CAN Forwarding via UART

When using UART, you can forward messages over CAN by making the first byte of the payload `COMM_FORWARD_CAN` and the second byte the target CAN ID. This allows communicating with multiple VESCs over a single UART port.

**Official CAN documentation:** https://github.com/vedderb/bldc/blob/master/documentation/comm_can.md
**CAN implementation source:** https://github.com/vedderb/bldc/blob/master/comm/comm_can.c

---

### UART Protocol

VESC communicates over UART using a structured packet format. The higher-level communication code is the same for USB, UART, and CAN ports -- everything that can be done from VESC Tool can be done from any port.

#### Packet Structure

```
[Start Byte] [Length] [Payload] [CRC16] [Stop Byte]
```

| Field | Short Packet | Long Packet |
|-------|-------------|-------------|
| Start Byte | `0x02` | `0x03` |
| Length | 1 byte (payload length) | 2 bytes (payload length) |
| Payload | Variable | Variable |
| CRC | 2 bytes (CRC16 over payload) | 2 bytes (CRC16 over payload) |
| Stop Byte | `0x03` | `0x03` |

- Short packets: payload length fits in 1 byte
- Long packets: payload length requires 2 bytes

#### Implementation Files (from firmware source)

| File | Purpose |
|------|---------|
| `bldc_interface.c/h` | Assembles payloads for all VESC commands; interprets response packets |
| `packet.c/h` | Packet assembly with start/stop bytes and checksum validation |
| `crc.c/h` | CRC16 checksum calculation |
| `buffer.c/h` | Conversion between C types and byte arrays |
| `bldc_interface_uart.c/h` | Bridges packet layer and UART interface |
| `comm_uart.c` | Platform-specific UART implementation |

#### Core API Functions

```c
// Initialize UART with send function pointer
bldc_interface_uart_init(send_func);

// Process received bytes one at a time
bldc_interface_uart_process_byte(byte);

// Call every millisecond for timeout management
bldc_interface_uart_run_timer();

// Set motor current (example command)
bldc_interface_set_current(10.0);  // 10A

// Read real-time data with callback
bldc_interface_set_rx_value_func(callback_function);
bldc_interface_get_values();
```

#### Important Notes

- The original UART documentation by Benjamin Vedder (2015) is partially outdated
- For current packet payload specs, consult the firmware source code directly
- UART baud rate is configurable via VESC Tool

**Original UART documentation:** http://vedder.se/2015/10/communicating-with-the-vesc-using-uart/

#### Community UART Libraries

- **Arduino/Teensy:** https://github.com/SolidGeek/VescUart (tested on Teensy 4, FW5+)
- **UART-CAN Forward:** https://github.com/GTU-Robotics-Club-2023/VESC-UART-CAN-Forward
- **CAN bus Arduino:** https://github.com/craigg96/vesc_can_bus_arduino

---

### Other Interfaces

All VESC controllers support these communication interfaces:

| Interface | Notes |
|-----------|-------|
| **USB** | USB-C on non-potted models; USB-C dongle on potted (39-pin) models. Supports data transfer and VESC Tool connection. |
| **CAN Bus** | 500 kbps default. 29-bit extended IDs. See dedicated section above. |
| **UART** | Packet-based protocol. Same command set as USB/CAN. See dedicated section above. |
| **SPI** | Available on all models. Used for SPI-based magnetic encoders. |
| **I2C** | Available on all models. |
| **PWM** | Input for RC-style signals, servo control. |
| **ADC** | 2-5 analog input channels depending on model (see comparison table). Used for throttle/brake inputs. |
| **Wi-Fi** | On ESP32-equipped models (Pronto, Maxim, Maxim+, Duet, Nanolog, Harmony 32, Dash 35B). Station and AP modes. |
| **Bluetooth** | BLE 5 on ESP32-equipped models. |

**Ground loop warning:** When configuring over USB, ensure your laptop/PC runs on battery power if the VESC is powered from a bench power supply. This avoids ground loops.

---

## Software Ecosystem

### VESC Tool

The primary configuration and monitoring application for all VESC devices. Available on desktop (Linux, Windows, macOS) and mobile (Android, iOS).

**Download:** Available through VESC Tool website/app stores.

#### Connection Methods

1. **USB** -- Ensure cable supports data transfer
2. **CAN Bus** -- For networked devices; use "Scan CAN" to discover
3. **Wi-Fi** -- Enable on both VESC and host device
4. **Bluetooth** -- Enable on both VESC and host device

#### Key Interface Controls

| Function | Purpose |
|----------|---------|
| Read Motor Config | Retrieve current motor settings from device |
| Write Motor Config | Save motor configuration to device |
| Read App Config | Retrieve application settings |
| Write App Config | Save application settings |
| Real-Time Data Stream | Monitor live motor/device telemetry |
| IMU Sampling | Activate motion sensor monitoring |
| BMS Data | Stream battery management data |
| CAN Forwarding | Route communication via CAN bus to other devices |
| Alive Packets | Prevent connection timeout |

#### Setup Wizards

VESC Tool includes a "Welcome & Wizards" page for guided initial setup including motor detection, input calibration, and application configuration.

**VESC Tool tutorials:** https://www.vesclabs.com/category/vesc-tool/

---

### VESC Firmware (BLDC)

Open-source motor controller firmware running on STM32F405.

- **Repository:** https://github.com/vedderb/bldc
- **License:** GNU General Public License v3.0
- **Author:** Benjamin Vedder

#### Building from Source

**Prerequisites (Linux/macOS):** git, wget, make, libgl-dev, libxcb-xinerama0

```bash
git clone https://github.com/vedderb/bldc.git
cd bldc
make arm_sdk_install
make <target>   # e.g., make 100_250
# Output: bldc/builds/<target>/
```

Run `make` without arguments to list all supported hardware targets.

#### Upload Methods

1. **STLink SWD Debugger** -- Direct hardware flashing (requires bootloader)
2. **VESC Tool via USB** -- Custom firmware upload with .bin file

#### Key Source Directories

| Path | Contents |
|------|----------|
| `comm/comm_can.c` | CAN bus communication implementation |
| `documentation/comm_can.md` | CAN protocol documentation |
| `lispBM/` | LispBM integration |
| `conf_general.h` | Configurable firmware options |

---

### LispBM Scripting

LispBM is a functional programming language created by Joel Svensson that runs in a sandboxed environment on VESC controllers. It allows custom applications to run directly on the STM32F405 (and ESP32-C3 on equipped models) without risking core firmware crashes.

**Reference manual:** https://www.lispbm.com/lispbm-reference-manual/html/
**VESC-specific docs:** https://www.lispbm.com/lispbm-reference-manual/html/vesc-lisp-documentation.html
**GitHub (LispBM core):** https://github.com/svenssonjoel/lispBM
**Getting started:** https://www.lispbm.com/getting-started.html

#### Key Function Categories

**Motor Control:**
- `set-current`, `set-duty`, `set-rpm`, `set-pos` -- Drive commands
- `set-brake`, `set-brake-rel`, `set-handbrake` -- Braking
- `foc-openloop`, `foc-play-tone`, `foc-play-samples` -- FOC testing/audio

**Motor Monitoring:**
- `get-current`, `get-rpm`, `get-duty`, `get-speed`, `get-dist`
- `get-temp-fet`, `get-temp-mot` -- Temperature readings
- `get-ah`, `get-wh` -- Energy consumption

**Position & Encoder:**
- `get-encoder`, `get-pos`, `phase-motor`, `phase-encoder`, `phase-hall`, `phase-observer`
- `enc-corr`, `enc-sample` -- Encoder correction/calibration

**Input Interfaces:**
- `get-ppm`, `get-ppm-age` -- PPM input
- `get-adc` -- ADC channels (0-4, plus 20-21 for encoder)
- `set-servo` -- Servo output
- `get-remote-state`, `set-remote-state` -- Remote control

**CAN Bus:**
- `canset-current`, `canset-rpm`, `canget-duty`, `canget-speed` -- Remote motor control
- `can-scan`, `can-ping` -- Device discovery
- `can-cmd` -- Remote script execution
- `canmsg-send`, `canmsg-recv` -- Raw byte-array CAN messages

**GPIO & Hardware:**
- `gpio-configure` -- Pin modes (output, input, open-drain)
- `gpio-write`, `gpio-read` -- Digital I/O
- `pwm-start`, `pwm-stop`, `pwm-set-duty` -- PWM generation
- `icu-start`, `icu-width`, `icu-period` -- Input capture

**UART:**
- `uart-start`, `uart-write`, `uart-read`, `uart-read-bytes`
- `uartcomm-start` -- VESC Tool-compatible UART

**I2C:**
- `i2c-start`, `i2c-tx-rx`, `i2c-detect-addr`

**Configuration & Storage:**
- `conf-set`, `conf-get` -- Read/write parameters
- `conf-store` -- Persist configuration
- `conf-detect-foc`, `conf-measure-res`, `conf-measure-ind` -- Motor detection
- `eeprom-store-f`, `eeprom-read-f` -- Nonvolatile storage (float/int)

**BMS Integration:**
- `get-bms-val` -- Read voltage, current, SOC, health, temperatures
- `set-bms-val` -- Write BMS values
- `set-bms-chg-allowed`, `bms-force-balance`, `bms-zero-offset`
- `bms-st` -- BMS self-test

**IMU / Sensors:**
- `get-imu-rpy` -- Roll/pitch/yaw
- `get-imu-quat` -- Quaternion
- `get-imu-acc`, `get-imu-gyro`, `get-imu-mag` -- Raw sensor data
- Derotated variants available

**ESP32-Specific Extensions (VESC Express):**
- Display drivers: ST7789, ILI9341, SSD1306, ST7735
- Wi-Fi/TCP networking
- BLE custom server
- GNSS receiver integration

**Utility:**
- `systime`, `secs-since`, `sysinfo`, `stats`, `timeout-reset`
- Trig functions, `sqrt`, `pow`, `floor`, `ceil`, `throttle-curve`
- `crc16`, `crc32` -- Checksums
- `rand`, `rand-max` -- Random numbers

---

## Fault Codes Reference

### Motor Controller Fault Codes

| Code | Fault Name | Description |
|------|-----------|-------------|
| 0 | None | No fault |
| 1 | Over Voltage | Input voltage exceeded maximum |
| 2 | Under Voltage | Input voltage below minimum |
| 3 | DRV | Gate driver fault |
| 4 | Absolute Over Current | Hardware overcurrent protection triggered |
| 5 | Over Temp FET | MOSFET temperature exceeded limit |
| 6 | Over Temp Motor | Motor temperature exceeded limit |
| 7 | Gate Driver Over Voltage | Gate driver supply too high |
| 8 | MCU Under Voltage | MCU supply voltage too low |
| 9 | Booting from Watchdog Reset | Controller recovered from watchdog timeout |
| 10 | Gate Driver Under Voltage | Gate driver supply too low |
| 11 | Encoder SPI | SPI encoder communication error |
| 12 | Encoder Sincos Below Min Amplitude | Sin/cos encoder signal too weak |
| 13 | Encoder Sincos Above Max Amplitude | Sin/cos encoder signal too strong |
| 14 | Flash Corruption | General flash memory corruption |
| 15 | High Offset Current Sensor 1 | Phase 1 current sensor offset out of range |
| 16 | High Offset Current Sensor 2 | Phase 2 current sensor offset out of range |
| 17 | High Offset Current Sensor 3 | Phase 3 current sensor offset out of range |
| 18 | Unbalanced Currents | Phase currents significantly unequal |
| 19 | BRK | Brake fault |
| 20 | Resolver LOT | Resolver loss of tracking |
| 21 | Resolver DOS | Resolver degraded signal |
| 22 | Resolver LOS | Resolver loss of signal |
| 23 | Flash Corruption App Config | Application config flash corruption |
| 24 | Flash Corruption MC Config | Motor config flash corruption |
| 25 | Encoder No Magnet | Magnetic encoder: no magnet detected |
| 26 | Encoder Magnet Too Strong | Magnetic encoder: field too strong |
| 27 | Phase Filter | Phase filter fault |
| 28 | Encoder Fault | General encoder fault |
| 29 | LV Output Fault | Low-voltage output fault |

Fault codes can be read via VESC Tool or observed via the RGB power switch LED on compatible devices.

### BMS Fault Codes

| Code | Fault Name |
|------|-----------|
| 0 | None |
| 1 | Charge Over Current |
| 2 | Charge Over Temp |
| 3 | Humidity |

---

## Example Project: Suzuki TS125 Electric Conversion

A documented full conversion of a 1982 Suzuki TS125 using VESC Labs products. Original 125cc 15HP engine replaced with a 30kW (~3x original power) electric drivetrain.

**Full article:** https://www.vesclabs.com/2025/12/13/suzuki-ts125-electric-conversion/

### Components Used

| Component | Model | Role |
|-----------|-------|------|
| Motor Controller | VESC Maxim 120V | Main motor controller |
| BMS | VESC Harmony 32 | Battery management |
| Display | VESC Dash 35B | Dashboard |
| Motor | SIA 155 64 + VESC ABI Encoder | Drive motor |
| Battery | 22S 42Ah Li-Ion NMC | Energy storage |
| IO Module | STR365 | Lights, indicators, signals |

### CAN Bus Architecture

All components communicate over CAN bus:

```
VESC Maxim 120V <--CAN--> Harmony 32 BMS
       |                        |
       +--------CAN BUS---------+
       |                        |
  Dash 35B              STR365 IO Module
```

- **Maxim -> Dash35B:** Speed, power mode, real-time metrics
- **Harmony 32 -> All:** Battery voltage, cell balancing, temperatures
- **STR365 IO -> Dash35B:** Indicator/light status via CAN

### Drivetrain

Two-stage chain reduction: 420 chain (stage 1) + 428 chain (stage 2) = **8.7:1 final ratio**, 600mm rear wheel diameter.

### Key Configuration

- Motor poles: 8
- Detection current: 100A
- Motor current limit: 600A
- Throttle ramping (positive): 0.2s
- Throttle ramping (negative): 0.1s
- Input deadband: 5%
- Control type: Current mode
- M6 terminal torque spec: max 4 Nm

### Wiring Notes

- M6 threaded terminals for motor phases and battery
- 39-pin automotive connector with 22 AWG extension cables
- 12V DC-DC converter for lights, indicators, horn
- LED headlight required (incandescent draws excessive startup current)

---

## Choosing a Motor Controller

**Guide:** https://www.vesclabs.com/2025/07/01/choosing-a-motor-controller/

### Key Selection Criteria

1. **Battery Voltage:** Controller must support max battery voltage. Li-Ion cells reach 4.2V fully charged. Total = cells_in_series x 4.2V. Select controller with headroom above this for voltage spikes during regen/acceleration.

2. **Motor Current:** Match controller continuous/burst ratings to motor specs with safety margin.

### Phase Current vs Battery Current

- **Phase current** causes the most heating in controller and motor
- Heat is proportional to **phase current squared** (I^2)
- Phase current is the limiting factor, not power
- High torque at low RPM generates significant heat despite moderate power

### Motor kV Rating

```
kV x Voltage = RPM (no-load)
```

Example: 100 kV motor on 48V = 4800 RPM. Same RPM achievable via different voltage/kV combinations, each with different thermal characteristics.

### Dual Motor Controllers

Duet and Duet XS models enable synchronized control of two motors with simplified wiring. Useful for dual-motor e-skates, cargo bikes, or vehicles with front/rear propulsion.

---

## Developer Resources & Links

### Official VESC Labs

| Resource | URL |
|----------|-----|
| Website | https://www.vesclabs.com/ |
| Resources Hub | https://www.vesclabs.com/resources/ |
| Getting Started | https://www.vesclabs.com/category/getting-started/ |
| VESC Tool Tutorials | https://www.vesclabs.com/category/vesc-tool/ |
| Example Projects | https://www.vesclabs.com/category/example-projects/ |
| Product Downloads | https://www.vesclabs.com/category/product-downloads/ (currently empty) |
| Motor Controller Comparison | https://www.vesclabs.com/motor-controller-comparison/ |
| About VESC Labs | https://www.vesclabs.com/about-us/ |
| Contact | https://www.vesclabs.com/contact-us/ |

### VESC Project / Community

| Resource | URL |
|----------|-----|
| VESC Project (forum, news) | https://vesc-project.com/ |
| VESC Firmware (BLDC) GitHub | https://github.com/vedderb/bldc |
| CAN Protocol Documentation | https://github.com/vedderb/bldc/blob/master/documentation/comm_can.md |
| CAN Implementation Source | https://github.com/vedderb/bldc/blob/master/comm/comm_can.c |
| VESC Forum | https://vesc-project.com/forum |
| VESC Discord | (linked from GitHub README) |

### LispBM

| Resource | URL |
|----------|-----|
| LispBM Website | https://www.lispbm.com/ |
| LispBM Reference Manual | https://www.lispbm.com/lispbm-reference-manual/html/ |
| VESC LispBM Documentation | https://www.lispbm.com/lispbm-reference-manual/html/vesc-lisp-documentation.html |
| Getting Started with LispBM | https://www.lispbm.com/getting-started.html |
| LispBM GitHub | https://github.com/svenssonjoel/lispBM |
| VESC LispBM README | https://github.com/vedderb/bldc/blob/master/lispBM/README.md |

### UART Communication

| Resource | URL |
|----------|-----|
| Original UART Guide (2015, partially outdated) | http://vedder.se/2015/10/communicating-with-the-vesc-using-uart/ |
| Arduino VescUart Library | https://github.com/SolidGeek/VescUart |
| UART-CAN Forward Library | https://github.com/GTU-Robotics-Club-2023/VESC-UART-CAN-Forward |
| CAN Bus Arduino Library | https://github.com/craigg96/vesc_can_bus_arduino |
| libVescCan (CAN encoding/decoding) | https://github.com/AlvaroBajceps/libVescCan |

### Social

| Platform | URL |
|----------|-----|
| Facebook | https://www.facebook.com/vesclabs |
| Instagram | https://www.instagram.com/vesc.labs/ |
| LinkedIn | https://www.linkedin.com/company/vesclabs/ |
| YouTube | https://www.youtube.com/@vesclabs |
