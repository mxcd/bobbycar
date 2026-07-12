# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Bob-e-car is a converted Bobby Car (children's ride-on toy) with a VESC Duet XS 60V dual motor controller. The project consists of embedded firmware for vehicle control, a Go telemetry server, and web-based dashboards for monitoring.

## Repository Structure

- **system-controller/** — Main vehicle ECU firmware (STM32 Nucleo F767ZI, PlatformIO/Arduino)
- **steering-controller/** — Steering wheel input firmware (Arduino Nano ATmega328, PlatformIO/Arduino)
- **telemetry-server/** — Go server that receives UDP telemetry and serves a web dashboard via WebSocket
- **logger/** — unified Go onboard tool (runs on a Raspberry Pi wired to the system controller): records the UDP telemetry as delimited-protobuf `.bblog` files with SQLite metadata, triggered always or on DMS with pre/post ring buffer; dashboard on :8090 with fsw-dl-style dynamically resampled log viewer, live telemetry at /live/ (protojson via /ws websocket), and firmware OTA for the system controller via the Nucleo's ST-LINK on the Pi's USB (`st-flash`; POST /api/ota/flash, 409-guarded unless vehicle is IDLE). Remote access via Tailscale once the Pi has WiFi/hotspot internet (see logger/README.md)
- **system-controller-ui/** — Quasar/Vue 3 development UI for testing via UDP (dev tool, not deployed on vehicle)
- **steering-pcb/** — KiCad PCB design for the steering controller

## Build Commands

### System Controller & Steering Controller (PlatformIO)
```bash
cd system-controller && pio run                    # build
cd system-controller && pio run -t upload           # build + flash via local ST-LINK
cd system-controller && just ota                   # build + flash OTA via the logger Pi
cd steering-controller && pio run                   # build
cd steering-controller && pio run -t upload          # build + flash
```

### Telemetry Server (Go)
```bash
cd telemetry-server && go build -o tmp/main ./cmd/server   # build
cd telemetry-server && go run ./cmd/server                 # run
```

### System Controller UI (Quasar/Vue)
```bash
cd system-controller-ui && pnpm install
cd system-controller-ui && npx quasar dev           # dev server
```

## Architecture

### Vehicle State Machine (stateflow.cpp)

The system controller runs a state machine with these states:
```
IDLE → PRECHARGING (2s) → TS_ACTIVE (0.5s) → MC_ACTIVE
  ↑                                               |
  ← ← ← (steering panic or battery fail) ← ← ← ←
```

- **IDLE**: All relays off. Transitions to PRECHARGING when steering comms established and battery OK.
- **PRECHARGING**: Precharge relay on. 2s timer before TS_ACTIVE.
- **TS_ACTIVE**: SC_OK relay enabled, MC button pulsed. 0.5s to MC_ACTIVE.
- **MC_ACTIVE**: Vehicle can drive. Motor commands are sent. On fault, goes directly to IDLE.

`safetyCheck()` = no steering panic AND battery voltage OK. DMS is NOT a safety gate — it only zeroes the torque request (soft switch).

### Steering Protocol

The steering controller sends 4-byte serial messages at 9600 baud every 50ms:
- Byte 0: bit flags (startupCheckOk, deadManSwitch, accelerate, brake)
- Byte 1: throttle (analog input A7, 0-255)
- Byte 2: rolling counter (mod 128) for watchdog
- Byte 3: CRC-8/AUTOSAR (poly=0x2F, init=0xFF, xorout=0xFF) over bytes 0-2

The system controller validates the CRC-8, monitors counter continuity (max 5 skip), and enforces a 200ms watchdog timeout. Any violation triggers an irrecoverable steering panic. The DMS signal has a 500ms tolerance — brief releases don't trigger a safety fault.

### Motor Control (vd.cpp + vesc_serial.cpp)

Uses the VESC UART protocol (115200 baud, CRC-16/CCITT framed packets) to communicate with a VESC Duet XS 60V dual motor controller. Throttle (0-255 from steering analog input) maps linearly to motor current (0-MAX_CURRENT_MA, 35A). Positive torque is only commanded with DMS held; otherwise the vehicle brakes (safe stop 10A / coast 5A / brake button 20A). Both motors receive the same command (motor 2 via COMM_FORWARD_CAN). One UART packet per 10ms tick in a 4-tick cycle: command M1, command M2, telemetry M1, telemetry M2 → 25Hz commands and 25Hz telemetry per motor via COMM_GET_VALUES_SELECTIVE (field mask documented in docs/logging.md).

### Telemetry Pipeline

System controller (10.42.10.130) → UDP broadcast (10.42.10.255:4242, protobuf `bob.Telemetry`, 50Hz) → Go telemetry server (listens :4242, converts to JSON via protojson) → WebSocket broadcast (/ws) → embedded Vue.js dashboard (:8080). The logger (Raspberry Pi) listens on the same port via SO_REUSEPORT and re-broadcasts the same protojson on its own /ws for the live dashboard at :8090/live/ — onboard, the logger alone covers logging + live telemetry; the telemetry-server remains the laptop dev tool.

The schema lives in `proto/telemetry.proto` (single source of truth: nanopb on the firmware, protoc-gen-go for logger and telemetry-server — see docs/logging.md). JSON/channel names are the camelCase proto JSON names; consumers detect legacy JSON firmware by a `{` first byte.

### Relay Pins (active-low)
- D43: Motor controller enable
- D44: Precharge relay
- D45: SC_OK relay

## Key Domain Concepts

- **SC_OK**: System Controller OK signal — indicates the system controller considers the vehicle safe
- **Dead Man Switch (DMS)**: Soft switch — zeroes torque request when released (500ms tolerance). Does NOT affect state machine.
- **Steering Panic**: Irrecoverable fault state triggered by watchdog timeout or counter discontinuity — requires power cycle
- **Derating**: Gradual power reduction as battery voltage drops
- **Flag button**: The steering wheel "R" button (proto field `steering_acceleration_command`, kept for wire/log compatibility) has no motor function; the logger derives flag markers from its rising edges (long press = one flag), shows them in the log viewer, and treats a press as a recording trigger in dms mode
