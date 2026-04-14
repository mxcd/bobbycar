# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Bob-e-car is a converted Bobby Car (children's ride-on toy) with a VESC Duet XS 60V dual motor controller. The project consists of embedded firmware for vehicle control, a Go telemetry server, and web-based dashboards for monitoring.

## Repository Structure

- **system-controller/** — Main vehicle ECU firmware (STM32 Nucleo F767ZI, PlatformIO/Arduino)
- **steering-controller/** — Steering wheel input firmware (Arduino Nano ATmega328, PlatformIO/Arduino)
- **telemetry-server/** — Go server that receives UDP telemetry and serves a web dashboard via WebSocket
- **system-controller-ui/** — Quasar/Vue 3 development UI for testing via UDP (dev tool, not deployed on vehicle)
- **steering-pcb/** — KiCad PCB design for the steering controller

## Build Commands

### System Controller & Steering Controller (PlatformIO)
```bash
cd system-controller && pio run                    # build
cd system-controller && pio run -t upload           # build + flash
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

Uses the VESC UART protocol (115200 baud, CRC-16/CCITT framed packets) to communicate with a VESC Duet XS 60V dual motor controller. Throttle (0-255 from steering analog input) maps linearly to motor current (0-MAX_CURRENT_MA, default 20A). DMS gates the current to zero when released. Brake button sends COMM_SET_CURRENT_BRAKE. Both motors receive the same command (motor 2 via COMM_FORWARD_CAN, CAN ID 1). Telemetry (battery voltage, temps, ERPM, fault code) is requested every 200ms via COMM_GET_VALUES. Battery derating scales current based on voltage (12S pack, derating between 3.65V-3.7V per cell).

### Telemetry Pipeline

System controller → UDP broadcast (10.42.1.255:4242, JSON, every 200ms) → Go telemetry server (listens :4242) → WebSocket broadcast (/ws) → embedded Vue.js dashboard (:8080).

The telemetry server embeds its HTML dashboard via `go:embed`. The state JSON keys match the `getKeyString()` mapping in `state.cpp`.

### Relay Pins (active-low)
- D43: Motor controller enable
- D44: Precharge relay
- D45: SC_OK relay

## Key Domain Concepts

- **SC_OK**: System Controller OK signal — indicates the system controller considers the vehicle safe
- **Dead Man Switch (DMS)**: Soft switch — zeroes torque request when released (500ms tolerance). Does NOT affect state machine.
- **Steering Panic**: Irrecoverable fault state triggered by watchdog timeout or counter discontinuity — requires power cycle
- **Derating**: Gradual power reduction as battery voltage drops
