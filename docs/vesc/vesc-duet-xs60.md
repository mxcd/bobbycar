# VESC Duet XS 60V

> Dual brushless motor controller capable of 6kW continuous power
> Datasheet revision: February 2026
> Source: [VESC Labs Product Page](https://www.vesclabs.com/product/vesc-duet-xs60/)

## Overview

The VESC Duet XS is a dual brushless motor controller which can provide up to 6kW of continuous power when cooled appropriately. An on-board STM32F405 microcontroller running open-source VESC software enables precise motor control alongside configurable custom applications and scripting.

Its breadth of external communication interfaces enables easy integration with many different systems and peripherals. It supports both sensorless and sensored brushless motor applications with full sinusoidal Field-Oriented-Control enabling quiet, efficient, and powerful motor dynamics.

It also supports other advanced control techniques including overmodulation and field weakening for maximizing power and speed in certain applications, and high frequency injection (HFI) for smooth sensorless control at and from zero speed.

## Features

- Fully independent Field-Oriented-Control (FOC) for dual brushless motors
- Maximum input voltage of 60V, 80A continuous current and 100A pulsed current per motor with adequate cooling
- Supports multiple control modes: current, duty cycle, speed, and position control
- Open-source firmware/scripting on STM32F405 MCU for custom applications
- Configurable sensor input ports compatible with hall sensors and encoders
- Communication interfaces: USB, CAN, SPI, I2C, UART, PWM, and ADC inputs
- Integrated IMU: 3-axis accelerometer and 3-axis gyroscope
- Integrated temperature sensors with automatic current protection (smooth current ramp-down)
- Easy setup with VESC Tool (Desktop and Phone applications)
- 12V switched output for external loads up to 0.5A (e.g., lights)
- Advanced High Frequency Injection (HFI) capability
- Overmodulation and field weakening support

## Specifications

### Absolute Maximum Ratings

| Parameter | Min | Typ | Max | Unit | Notes |
|---|---|---|---|---|---|
| Input Voltage | 15 | 54.6 | 60 | V | Spikes may not exceed 60V; allow margin. Max 13S Li-Ion (4.2V/cell) recommended |
| Battery Series (Li-Ion) | 6 | - | 13 | S | |
| Input Current | - | - | 80 | A | |
| Continuous Motor Current (per motor) | - | 80 | - | A | Peak amplitude current in Dq frame; stalled motor can experience peak continuously |
| Pulsed Motor Current (per motor) | - | - | 100 | A | 10-30 seconds depending on starting temperature and external cooling |
| PWM Switching Frequency | - | 30 | 50 | kHz | |
| 12V Auxiliary Current | - | - | 0.5 | A | |
| 5V Auxiliary Current | - | - | 2 | A | |
| 3.3V Auxiliary Current | - | - | 0.3 | A | |
| Servo PPM Input | - | 2 | 5.5 | V | |
| ADC Inputs | 0 | - | 5.5 | V | |

### Typical Operating Characteristics

| Parameter | Typ | Max | Unit |
|---|---|---|---|
| Sleep Input Current (50V Input) | 10 | 40 | uA |
| Awake Resting Input Current (50V Input) | 5 | 10 | mA |

### Recommended Operating Conditions

| Parameter | Min | Max | Unit |
|---|---|---|---|
| Operating Temperature | -40 | 85 | C |
| Humidity | - | Non-condensing | - |

### Physical Properties

| Parameter | Value | Unit |
|---|---|---|
| Length | 87 | mm |
| Width | 54 | mm |
| Height | 18 | mm |
| Mass | 163 | g (incl. cables and connectors) |

### Mechanical Dimensions

Mounting hole diameter: 3.2mm

```
         9.5
    |<-------->|
    +----------------------------+
    |                            |  ^
    |  (3.2)            (3.2)   |  |
    |                            |  | 53.5
    |                            |  |
    |  (3.2)            (3.2)   |  |
    |                            |  v
    +----------------------------+
    |<--- 81 --->|
    |<----- 83.5 ------>|
    |<------- 87.2 -------->|

    Height: 17.57mm (with components)
```

All dimensions in mm. See datasheet Figure 3 for exact drawing.

## Connectors

### Input Power
- **XT90 connector** for battery input

### Motor Output
- **4mm bullet connectors** (6x male included)

### Included Accessories
- 1x VESC Duet XS 60V controller
- 1x VESC RGB Momentary Power Button (JST-GH)
- 6x 4mm Bullet Connectors (Male)

## Pinout

All peripheral connectors are **JST-GH (1.25mm pitch)**. An alternative JST-PH (2.0mm) configuration is optionally available.

### Motor Sensors 1 (6-pin JST-GH)

| Pin | Name | Function | Limits | CPU Pin |
|---|---|---|---|---|
| 1 | GND | Ground | 1A | - |
| 2 | HC | Hall 3 / Encoder I | 5V | Hall 3 |
| 3 | HB | Hall 2 / Encoder B | 5V | Hall 2 |
| 4 | HA | Hall 1 / Encoder A | 5V | Hall 1 |
| 5 | TEMP | Motor Temp Sensor | 3.3V | ADC Temp |
| 6 | +5V | Power Out | 1A | - |

### Motor Sensors 2 (6-pin JST-GH)

| Pin | Name | Function | Limits | CPU Pin |
|---|---|---|---|---|
| 1 | GND | Ground | 1A | - |
| 2 | HC | Hall 3 / Encoder I | 5V | Hall 3 |
| 3 | HB | Hall 2 / Encoder B | 5V | Hall 2 |
| 4 | HA | Hall 1 / Encoder A | 5V | Hall 1 |
| 5 | TEMP | Motor Temp Sensor | 3.3V | ADC Temp |
| 6 | +5V | Power Out | 1A | - |

### AUX (2-pin JST-GH)

| Pin | Name | Function | Limits |
|---|---|---|---|
| 1 | GND | Ground | 1A |
| 2 | +12V | Switched 12V Output | 0.5A |

### Power Switch (6-pin JST-GH)

| Pin | Name | Function | Limits | Notes |
|---|---|---|---|---|
| 1 | RED | Red LED | 3.3V 0.05A | 220R to PD11 |
| 2 | GRN | Green LED | 3.3V 0.05A | 220R to PD10 |
| 3 | BLU | Blue LED | 3.3V 0.05A | 220R to PD15 |
| 4 | +5V | Power Out | 1A | - |
| 5 | MOM2 | Momentary Switch | - | Bridge MOM1 and MOM2 with momentary push button |
| 6 | MOM1 | Momentary Switch | - | Pressing toggles device on/off |

### PPM (3-pin JST-GH)

| Pin | Name | Function | Limits | CPU Pin |
|---|---|---|---|---|
| 1 | GND | Ground | 1A | - |
| 2 | +5V | Power Out | 1A | - |
| 3 | SIG | PPM Input | 5V | PPM |

### IO (8-pin JST-GH)

| Pin | Name | Function | Limits | CPU Pin |
|---|---|---|---|---|
| 1 | +5V | Power Out | 1A | - |
| 2 | 3V3 | +3.3V | 0.3A | - |
| 3 | GND | Ground | 1A | - |
| 4 | ADC1 | Throttle Input | 5V | ADC1 |
| 5 | TX | Transmit UART | 3.3V | UART TX |
| 6 | RX | Receive UART | 5V | UART RX |
| 7 | ADC2 | Brake Input | 5V | ADC2 |
| 8 | ADC3 | ADC3 | 5V | ADC3 |

### CAN (4-pin JST-GH)

| Pin | Name | Function | Limits |
|---|---|---|---|
| 1 | +5V | Power Out | 1A |
| 2 | CANH | CAN High | +/-60V |
| 3 | CANL | CAN Low | +/-60V |
| 4 | GND | Ground | 1A |

### SWD Debug (5-pin JST-GH)

| Pin | Name | Function | Limits | CPU Pin |
|---|---|---|---|---|
| 1 | RST | STM Reset | 3.3V | NRST |
| 2 | SWD | SWDIO | 3.3V | PA13 |
| 3 | GND | Ground | 1A | - |
| 4 | SWC | SWCLK | 3.3V | PA14 |
| 5 | 3V3 | +3.3V | 0.3A | - |

## Wiring Diagram

```
BLDC MOTOR 1                              BLDC MOTOR 2
  PHASE A ----+                      +---- PHASE A
  PHASE B ----+                      +---- PHASE B
  PHASE C ----+                      +---- PHASE C
              |                      |
        MOTOR SENSORS          MOTOR SENSORS
              |                      |
              v                      v
     +----[SENSORS 1]----[SENSORS 2]----+
     |                                   |
     |   +--[SWD]  [AUX]  [CAN]--+     |
     |   |                         |     |
     |   |     VESC DUET XS 60V   |     |
     |   |                         |     |
     |   +--[IO]   [PPM]  [PSW]--+     |
     |                                   |
     +----------[XT90 INPUT]------------+
                     |
              +------+------+
              |             |
           BAT +         BAT -
              |             |
        +-----+-----+------+
        | 100A FUSE MAX     |
        +-------------------+
              |
        +-----+-----+
        |   BMS WITH        |
        |  INTEGRATED       |
        |  PRECHARGE        |
        |  SWITCH           |
        +-------------------+
              |
        +-----+-----+
        |  BATTERY          |
        |  (6-13S)          |
        +-------------------+
```

**Important**: A precharge switch or XT90S connector must be fitted if the BMS does not have a built-in precharge switch.

## CAN Bus Protocol

VESC uses **29-bit extended CAN IDs** with the following structure:

| Bits | Field |
|---|---|
| B28-B16 | Unused |
| B15-B8 | Command ID |
| B7-B0 | VESC Device ID |

Each VESC device accepts commands only when its configured ID matches the frame's VESC ID field.

### Single-Frame Commands

Simple commands transmit 4 data bytes as a 32-bit big-endian signed integer with scaling applied.

| Command | ID | Scaling | Unit | Range |
|---|---|---|---|---|
| SET_DUTY | 0 | 100,000 | % / 100 | -1.0 to 1.0 |
| SET_CURRENT | 1 | 1,000 | A | +/-MOTOR_MAX |
| SET_CURRENT_BRAKE | 2 | 1,000 | A | +/-MOTOR_MAX |
| SET_RPM | 3 | 1 | RPM | +/-MAX_RPM |
| SET_POS | 4 | 1,000,000 | Degrees | 0-360 |
| SET_CURRENT_REL | 10 | 100,000 | % / 100 | -1.0 to 1.0 |
| SET_CURRENT_BRAKE_REL | 11 | 100,000 | % / 100 | -1.0 to 1.0 |
| SET_CURRENT_HANDBRAKE | 12 | 1,000 | A | +/-MOTOR_MAX |
| SET_CURRENT_HANDBRAKE_REL | 13 | 100,000 | % / 100 | -1.0 to 1.0 |

### Status Messages (Received from VESC)

| Message | ID | Data |
|---|---|---|
| STATUS | 9 | ERPM, Current, Duty Cycle |
| STATUS_2 | 14 | Amp-hours used/charged |
| STATUS_3 | 15 | Watt-hours used/charged |
| STATUS_4 | 16 | FET temp, Motor temp, Input current, PID position |
| STATUS_5 | 27 | Tachometer, Input voltage |
| STATUS_6 | 58 | ADC readings, PPM signal |

### CAN Bus Operational Notes

- **Timeout**: Default 0.5-second watchdog stops the motor if no CAN message is received
- **Command Rate**: Recommend 50 Hz continuous transmission to prevent timeout
- **Out-of-Range Values**: Automatically truncated to maximum limits (not rejected)

## LispBM Scripting

The VESC supports LispBM, a sandboxed Lisp interpreter for custom scripting on the controller. Key capabilities:

- **Motor Control**: `set-current`, `set-duty`, `set-rpm`, `set-pos` for direct motor operation
- **Sensor Reading**: Access to current, voltage, temperature, position, and encoder data
- **Communication**: CAN bus commands, UART/I2C interfaces
- **Peripheral Control**: GPIO, PWM, servo output management
- **Configuration**: Parameter storage via EEPROM and NVS
- **Data Processing**: Math functions, string manipulation, byte array operations

Development and testing happens in VESC Tool with live variable monitoring, plotting, and a REPL. Code can be uploaded to flash memory for automatic startup on boot. The Lisp environment is sandboxed -- errors in Lisp code cannot crash the main firmware.

## Configuration

### Connecting to VESC Tool

The controller can be connected to VESC Tool via **USB** or **CAN**. For instructions visit: https://www.vesclabs.com/category/getting-started/

### Motor Setup Wizard (Summary)

1. Select "Setup Motors FOC" wizard from the Welcome & Wizards page
2. Load default parameters
3. Select usage type and motor size
4. Configure battery type, cell series count, and capacity
5. Configure drive type (direct drive or geared), pulley sizes, and motor poles
6. Run motor detection (ensure motor is free from obstruction and lifted off ground)
7. Verify detection results and motor direction; invert if needed

### Configuring Motor Current

Navigate to General Motor Settings > Current tab:
- **Motor Current Max**: Rated for your motor or application
- **Motor Current Max Brake**: Rated for your motor or application
- **Absolute Maximum Current**: Rated for the motor or application
- Write configuration to controller; repeat for each CAN-connected device

### Configuring Battery Current

Navigate to General Motor Settings > Current tab:
- **Battery Current Max** = Max output current of battery/BMS / number of motors in system
- **Battery Current Max Regen** = Max charge current of battery/BMS / number of motors in system
- Repeat for each CAN-connected device

### Configuration Warnings

- **Ground loops**: When configuring over USB, run your laptop on battery power if the VESC is powered from a bench supply. USB + mains earth can create a ground loop that permanently damages the VESC.
- Always install the latest VESC Tool from the official website
- Do not exceed voltage or current limits for your specific battery/motor system
- Ensure motor and moving parts are free from obstruction before powering on

## Installation Warnings

- Installation must be performed by qualified personnel experienced with high-voltage battery systems
- Securely mount the device using threadlock in a location protected from vibration, moisture, dust, and direct heat
- Do not install in locations exposed to water, condensation, or flammable materials
- Install motor temperature sensors for smooth current ramp-down protection
- Only use manufacturer-specified connectors and cables
- Verify correct polarity for all connections -- incorrect wiring causes irreversible damage
- Insulate all exposed terminals and wiring
- Do not drill, cut, or modify the device enclosure or circuit board
- Power up for the first time in a controlled environment; be prepared to disconnect immediately

## Compliance

- **CE**: Directive 2014/30/EU (EMC), Directive 2014/35/EU (LVD), Directive 2011/65/EU (RoHS)
- **UKCA**: Electrical Equipment (Safety) Regulations 2016, EMC Regulations 2016, RoHS Regulations 2012
- **WEEE**: Compliant with Directive 2012/19/EU -- must be collected separately for recycling

## Pricing

| Quantity | Price (ex VAT) |
|---|---|
| 1-2 | EUR 199.99 |
| 3-9 | EUR 189.99 |
| 10+ | EUR 179.99 |

Origin: Made in Sweden
Warranty: 24 months (limited) -- [Policy](https://www.vesclabs.com/warranty-and-returns-policy/)

## Resources

- **Datasheet (PDF)**: https://www.vesclabs.com/wp-content/uploads/2026/02/vesc_duet_xs_60v_datasheet.pdf
- **STEP Model (CAD)**: https://www.vesclabs.com/wp-content/uploads/2026/01/duet_xs.zip
- **VESC Tool**: https://vesc-project.com/vesc_tool
- **Getting Started Guide**: https://www.vesclabs.com/category/getting-started/
- **CAN Bus Protocol Docs**: https://github.com/vedderb/bldc/blob/master/documentation/comm_can.md
- **LispBM Scripting Reference**: https://github.com/vedderb/bldc/blob/master/lispBM/README.md
- **Warranty & Returns**: https://www.vesclabs.com/warranty-and-returns-policy/

## Optional Accessories

| Accessory | Price |
|---|---|
| VESC Nanolog (Wi-Fi, Bluetooth, 4GB storage) | EUR 29.99 |
| BE220 GPS Module | EUR 12.99 |
| XT90S Connector (Female) | EUR 2.49 |
| VESC Dash 35B Display | Available |
