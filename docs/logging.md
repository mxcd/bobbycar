# Logging & Telemetry Signals

Data path: `system-controller` → UDP broadcast `10.42.10.255:4242` (protobuf
`bob.Telemetry`, **50Hz**) → logger (Raspberry Pi, records every packet to
`.bblog`) and telemetry-server (dashboard).

Schema: [`proto/telemetry.proto`](../proto/telemetry.proto) — single source of
truth, compiled with nanopb (firmware) and protoc-gen-go (logger,
telemetry-server). Channel/JSON names are the camelCase proto JSON names.

## Rates

| Stage | Rate | Where |
|---|---|---|
| Main loop / state machine | 100Hz | `main.cpp` `SAMPLE_TIME 10` |
| Steering frames (DMS, throttle, buttons) | 20Hz | steering firmware, 50ms |
| VESC telemetry per motor | 25Hz | `vd.cpp` 4-tick schedule |
| VESC motor commands per motor | 25Hz | `vd.cpp` 4-tick schedule |
| UDP telemetry broadcast | 50Hz | `telemetry.cpp` `TELEMETRY_MESSAGE_INTERVAL 20` |
| Log records | 50Hz | logger writes every received packet |

`vd.cpp` sends exactly one VESC UART packet per 10ms tick (collision-free):
tick 0 = command motor 1, tick 1 = command motor 2, tick 2 = telemetry request
motor 1, tick 3 = telemetry request motor 2.

## VESC signals — `COMM_GET_VALUES_SELECTIVE` (id 50)

Request carries a uint32 field mask; the response echoes the mask and packs
the selected fields in bit order. Field order/widths per vedderb/bldc
`commands.c` (identical to the full `COMM_GET_VALUES` layout). The parser
(`vesc_serial.cpp processGetValuesSelective`) walks the **echoed** mask, so it
stays aligned even if the VESC serves a different field set.

Mask used: `VESC_TELEMETRY_MASK = 0x000281CF` (`vd.cpp`) — response is 27
payload bytes / 32 on the wire vs ~76 for the full `COMM_GET_VALUES`.

| Bit | Signal | Encoding | Scale | Retrieved | Notes |
|----:|---|---|---|:---:|---|
| 0 | FET temperature | int16 | /10 °C | ✅ | `vdFetTemp` |
| 1 | Motor temperature | int16 | /10 °C | ✅ | `vdMotorTemp` |
| 2 | Avg motor current | int32 | /100 A | ✅ | `vdMotorCurrentLeft/Right` |
| 3 | Avg input current | int32 | /100 A | ✅ | `vdInputCurrent` (motor 1 = pack) |
| 4 | Avg Id current | int32 | /100 A | ❌ | FOC diagnostic, not needed |
| 5 | Avg Iq current | int32 | /100 A | ❌ | FOC diagnostic, not needed |
| 6 | Duty cycle | int16 | /1000 | ✅ | `vdDutyCycle` |
| 7 | ERPM | int32 | 1 | ✅ | `vdErpmLeft/Right` |
| 8 | Input voltage | int16 | /10 V | ✅ | `vdBatteryVoltage` — feeds undervoltage protection |
| 9 | Amp hours used | int32 | /1e4 Ah | ❌ | energy counters not used yet |
| 10 | Amp hours charged | int32 | /1e4 Ah | ❌ | |
| 11 | Watt hours used | int32 | /1e4 Wh | ❌ | |
| 12 | Watt hours charged | int32 | /1e4 Wh | ❌ | |
| 13 | Tachometer | int32 | 1 | ❌ | distance derivable from ERPM |
| 14 | Tachometer abs | int32 | 1 | ❌ | |
| 15 | Fault code | uint8 | enum | ✅ | `vdFaultCode` |
| 16 | PID position | int32 | /1e6 | ❌ | position control unused |
| 17 | Controller ID | uint8 | — | ✅ | routes response to motor 1 vs 2 — **must always be set** |
| 18 | MOSFET temps 1-3 | 3× int16 | /10 °C | ❌ | per-FET detail, bit 0 suffices |
| 19 | Avg Vd voltage | int32 | /1e3 V | ❌ | FOC diagnostic |
| 20 | Avg Vq voltage | int32 | /1e3 V | ❌ | FOC diagnostic |
| 21 | Status/kill-switch | uint8 | flags | ❌ | newer firmware only |

Motor 2 is polled via `COMM_FORWARD_CAN`; only the motor 1 (UART-local)
response carries pack-level data (voltage, temps, input current, fault).

## Steering signals (RS232 frame, 20Hz)

| Signal | Source | Telemetry field | Logged |
|---|---|---|:---:|
| Startup check OK | bit 0 | `steeringStartupCheckOk` | ✅ |
| Dead man switch | bit 1 | `steeringDeadManSwitchPressed` | ✅ — log trigger |
| Flag button ("R", former acceleration button) | bit 2 | `steeringAccelerationCommand` | ✅ — no motor function since sweep removal; the logger derives flag markers from rising edges (a held press = one flag) and a press also triggers recording in dms mode |
| Brake button | bit 3 | `steeringBrakeCommand` | ✅ |
| Throttle | byte 1 | `steeringThrottle` | ✅ 0-255 |
| Rolling counter | byte 2 | `steeringWatchdogIndex` | ✅ |
| — derived | — | `steeringPanic`, `steeringTimeSinceLastWatchdogMessage` | ✅ |

## System controller signals

| Signal | Telemetry field | Logged |
|---|---|:---:|
| Loop counter | `loopCounter` | ✅ 100Hz source |
| Vehicle state (IDLE/PRECHARGING/TS_ACTIVE/MC_ACTIVE) | `vehicleState` | ✅ |
| Relays (MC enable, precharge, SC_OK) | `relayMcEn`, `relayPrecharge`, `relayScOk` | ✅ |
| Battery OK flag | `batteryOk` | ✅ |
| Derating factor | `vdDeratingFactor` | ✅ (computed, currently unused by motor control) |
| Current requests per motor | `vdCurrentRequestLeft/Right` | ✅ negative = braking |
| VESC UART diagnostics (rx/tx bytes, packet counters, CRC fails, …) | `vdVesc*` | ✅ 10 channels |

## Wire & file format

- UDP payload = raw `bob.Telemetry` protobuf (~60-150B vs ~950B legacy JSON).
  Consumers detect legacy JSON firmware by a `{` first byte and stay
  backward compatible.
- Log records embed the received frame verbatim (`LogRecord.telemetry`) plus
  the receive timestamp — nothing is resampled or converted before writing.
  Old JSON-era `.bblog` files (map format) remain readable.
