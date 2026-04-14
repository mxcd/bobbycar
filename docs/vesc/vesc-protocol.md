# VESC Communication Protocol Reference

Comprehensive documentation of the VESC motor controller communication protocol, covering UART packet format, CAN bus communication, command reference, and interfacing from external microcontrollers.

Primary sources: [Benjamin Vedder's blog](http://vedder.se/2015/10/communicating-with-the-vesc-using-uart/), [VESC firmware source (vedderb/bldc)](https://github.com/vedderb/bldc), [VescUart Arduino library](https://github.com/SolidGeek/VescUart), [VESC CAN protocol docs](https://github.com/vedderb/bldc/blob/master/documentation/comm_can.md), [OpenRobot CAN tutorial](https://dongilc.gitbook.io/openrobot-inc/tutorials/control-with-can).

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [UART Communication](#uart-communication)
   - [Packet Format](#uart-packet-format)
   - [CRC-16 Checksum](#crc-16-checksum)
   - [UART Configuration](#uart-configuration)
3. [COMM_PACKET_ID Command Reference](#comm_packet_id-command-reference)
   - [Full Enum](#full-comm_packet_id-enum)
   - [Motor Control Commands](#motor-control-commands)
   - [Telemetry: COMM_GET_VALUES](#telemetry-comm_get_values)
   - [COMM_GET_VALUES_SELECTIVE](#comm_get_values_selective)
   - [Other Useful Commands](#other-useful-commands)
4. [CAN Bus Communication](#can-bus-communication)
   - [CAN ID Format](#can-id-format)
   - [CAN_PACKET_ID Reference](#can_packet_id-reference)
   - [CAN Motor Control Commands](#can-motor-control-commands)
   - [CAN Status Messages](#can-status-messages)
   - [Multi-Frame (Buffer) Protocol](#multi-frame-buffer-protocol)
   - [CAN Forwarding over UART](#can-forwarding-over-uart)
5. [Fault Codes](#fault-codes)
6. [Interfacing from External MCU](#interfacing-from-external-mcu)
   - [Arduino (VescUart Library)](#arduino-vescuart-library)
   - [STM32 (Official Reference)](#stm32-official-reference)
   - [CAN Bus with Arduino (MCP2515)](#can-bus-with-arduino-mcp2515)
   - [ESP32](#esp32)
7. [Dual/Multi-VESC Setup](#dualmulti-vesc-setup)
8. [Timing and Timeout](#timing-and-timeout)
9. [Libraries and Resources](#libraries-and-resources)

---

## Architecture Overview

The VESC communication stack is layered:

```
+---------------------------------------------+
| Application Layer (commands.c)               |
| - Assembles/parses payloads for all commands |
| - Same code for USB, UART, CAN              |
+---------------------------------------------+
| Packet Layer (packet.c)                      |
| - Adds start/stop bytes, length, CRC        |
| - State machine for receiving bytes          |
+---------------------------------------------+
| Transport Layer                              |
| - UART (comm_uart.c)                        |
| - USB (comm_usb_serial.c)                   |
| - CAN (comm_can.c)                          |
+---------------------------------------------+
| Helper Utilities                             |
| - buffer.c: type <-> byte array conversion  |
| - crc.c: CRC-16/CCITT checksum              |
+---------------------------------------------+
```

The key insight is that the **higher-level protocol is identical** regardless of physical transport (USB, UART, CAN). The first byte of every payload is a `COMM_PACKET_ID` that identifies the command.

---

## UART Communication

### UART Packet Format

All UART communication uses framed packets. There are two formats depending on payload length:

#### Short Packet (payload <= 256 bytes)

```
+--------+--------+------------------+----------+----------+--------+
| 0x02   | LEN    | PAYLOAD          | CRC_H    | CRC_L    | 0x03   |
| 1 byte | 1 byte | LEN bytes        | 1 byte   | 1 byte   | 1 byte |
+--------+--------+------------------+----------+----------+--------+
```

#### Long Packet (payload > 256 bytes)

```
+--------+--------+--------+------------------+----------+----------+--------+
| 0x03   | LEN_H  | LEN_L  | PAYLOAD          | CRC_H    | CRC_L    | 0x03   |
| 1 byte | 1 byte | 1 byte | LEN bytes        | 1 byte   | 1 byte   | 1 byte |
+--------+--------+--------+------------------+----------+----------+--------+
```

Field descriptions:
- **Start byte**: `0x02` for short packets, `0x03` for long packets
- **Length**: 1 byte (short) or 2 bytes big-endian (long) -- specifies payload length only
- **Payload**: The actual command data. First byte is always a `COMM_PACKET_ID`
- **CRC**: 16-bit CRC-CCITT over the payload only (not start/length/stop bytes), big-endian
- **Stop byte**: Always `0x03`

**Total packet overhead**: 5 bytes (short) or 6 bytes (long).

#### Example: Sending COMM_GET_VALUES (command 0x04)

```
Payload: [0x04]  (1 byte)
CRC16([0x04]) = 0x4084

Packet: 02 01 04 40 84 03
        ^  ^  ^  ^--^  ^
        |  |  |  CRC   stop
        |  |  payload
        |  length=1
        start (short)
```

### CRC-16 Checksum

The VESC uses **CRC-16/CCITT** with polynomial `0x1021` and initial value `0x0000`.

Implementation (table-driven):

```c
static const unsigned short crc16_tab[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x54a5,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x4864, 0x5845, 0x6826, 0x7807, 0x08e0, 0x18c1, 0x28a2, 0x38a3,
    0xc96c, 0xd94d, 0xe92e, 0xf90f, 0x89e8, 0x99c9, 0xa9aa, 0xb98b,
    0x5a55, 0x4a74, 0x7a17, 0x6a36, 0x1ad1, 0x0af0, 0x3a93, 0x2ab2,
    0xdb5d, 0xcb7c, 0xfb1f, 0xeb3e, 0x9bd9, 0x8bf8, 0xbb9b, 0xab9a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x85a9, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd9ec, 0xc9cd, 0xf9ae, 0xe98f, 0x9968, 0x8949, 0xb92a, 0xa90b,
    0x58e4, 0x48c5, 0x78a6, 0x6887, 0x1860, 0x0841, 0x3822, 0x2803,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

unsigned short crc16(unsigned char *buf, unsigned int len) {
    unsigned int i;
    unsigned short cksum = 0;
    for (i = 0; i < len; i++) {
        cksum = crc16_tab[((cksum >> 8) ^ *buf++) & 0xFF] ^ (cksum << 8);
    }
    return cksum;
}
```

### UART Configuration

| Parameter   | Value                        |
|-------------|------------------------------|
| Baud rate   | **115200** (VESC default)    |
| Data bits   | 8                            |
| Stop bits   | 1                            |
| Parity      | None                         |
| Flow control| None                         |

The baud rate is configurable via VESC Tool under App Settings > UART. Some implementations use 250000 baud for better compatibility with 8MHz/16MHz MCUs (Arduino). Whatever you set in VESC Tool, the external MCU must match.

**VESC Tool configuration path**: App Settings > General > App to Use: select "UART" or "PPM and UART".

---

## COMM_PACKET_ID Command Reference

### Full COMM_PACKET_ID Enum

The first byte of every payload identifies the command. From `datatypes.h` in the VESC firmware:

```c
typedef enum {
    COMM_FW_VERSION                         = 0,
    COMM_JUMP_TO_BOOTLOADER                 = 1,
    COMM_ERASE_NEW_APP                      = 2,
    COMM_WRITE_NEW_APP_DATA                 = 3,
    COMM_GET_VALUES                          = 4,
    COMM_SET_DUTY                            = 5,
    COMM_SET_CURRENT                         = 6,
    COMM_SET_CURRENT_BRAKE                   = 7,
    COMM_SET_RPM                             = 8,
    COMM_SET_POS                             = 9,
    COMM_SET_HANDBRAKE                       = 10,
    COMM_SET_DETECT                          = 11,
    COMM_SET_SERVO_POS                       = 12,
    COMM_SET_MCCONF                          = 13,
    COMM_GET_MCCONF                          = 14,
    COMM_GET_MCCONF_DEFAULT                  = 15,
    COMM_SET_APPCONF                         = 16,
    COMM_GET_APPCONF                         = 17,
    COMM_GET_APPCONF_DEFAULT                 = 18,
    COMM_SAMPLE_PRINT                        = 19,
    COMM_TERMINAL_CMD                        = 20,
    COMM_PRINT                               = 21,
    COMM_ROTOR_POSITION                      = 22,
    COMM_EXPERIMENT_SAMPLE                   = 23,
    COMM_DETECT_MOTOR_PARAM                  = 24,
    COMM_DETECT_MOTOR_R_L                    = 25,
    COMM_DETECT_MOTOR_FLUX_LINKAGE           = 26,
    COMM_DETECT_ENCODER                      = 27,
    COMM_DETECT_HALL_FOC                      = 28,
    COMM_REBOOT                              = 29,
    COMM_ALIVE                               = 30,
    COMM_GET_DECODED_PPM                     = 31,
    COMM_GET_DECODED_ADC                     = 32,
    COMM_GET_DECODED_CHUK                    = 33,
    COMM_FORWARD_CAN                         = 34,
    COMM_SET_CHUCK_DATA                      = 35,
    COMM_CUSTOM_APP_DATA                     = 36,
    COMM_NRF_START_PAIRING                   = 37,
    COMM_GPD_SET_FSW                         = 38,
    COMM_GPD_BUFFER_NOTIFY                   = 39,
    COMM_GPD_BUFFER_SIZE_LEFT                = 40,
    COMM_GPD_FILL_BUFFER                     = 41,
    COMM_GPD_OUTPUT_SAMPLE                   = 42,
    COMM_GPD_SET_MODE                        = 43,
    COMM_GPD_FILL_BUFFER_INT8                = 44,
    COMM_GPD_FILL_BUFFER_INT16               = 45,
    COMM_GPD_SET_BUFFER_INT_SCALE            = 46,
    COMM_GET_VALUES_SETUP                    = 47,
    COMM_SET_MCCONF_TEMP                     = 48,
    COMM_SET_MCCONF_TEMP_SETUP               = 49,
    COMM_GET_VALUES_SELECTIVE                = 50,
    COMM_GET_VALUES_SETUP_SELECTIVE          = 51,
    COMM_EXT_NRF_PRESENT                     = 52,
    COMM_EXT_NRF_ESB_SET_CH_ADDR            = 53,
    COMM_EXT_NRF_ESB_SEND_DATA              = 54,
    COMM_EXT_NRF_ESB_RX_DATA                = 55,
    COMM_EXT_NRF_SET_ENABLED                = 56,
    COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP  = 57,
    COMM_DETECT_APPLY_ALL_FOC               = 58,
    COMM_JUMP_TO_BOOTLOADER_ALL_CAN         = 59,
    COMM_ERASE_NEW_APP_ALL_CAN              = 60,
    COMM_WRITE_NEW_APP_DATA_ALL_CAN         = 61,
    COMM_PING_CAN                            = 62,
    COMM_APP_DISABLE_OUTPUT                  = 63,
    COMM_TERMINAL_CMD_SYNC                   = 64,
    COMM_GET_IMU_DATA                        = 65,
    COMM_BM_CONNECT                          = 66,
    COMM_BM_ERASE_FLASH_ALL                 = 67,
    COMM_BM_WRITE_FLASH                     = 68,
    COMM_BM_REBOOT                           = 69,
    COMM_BM_DISCONNECT                       = 70,
    COMM_BM_MAP_PINS_DEFAULT                = 71,
    COMM_BM_MAP_PINS_NRF5X                  = 72,
    COMM_ERASE_BOOTLOADER                   = 73,
    COMM_ERASE_BOOTLOADER_ALL_CAN           = 74,
    COMM_PLOT_INIT                           = 75,
    COMM_PLOT_DATA                           = 76,
    COMM_PLOT_ADD_GRAPH                      = 77,
    COMM_PLOT_SET_GRAPH                      = 78,
    COMM_GET_DECODED_BALANCE                 = 79,
    COMM_BM_MEM_READ                         = 80,
    COMM_WRITE_NEW_APP_DATA_LZO             = 81,
    COMM_WRITE_NEW_APP_DATA_ALL_CAN_LZO     = 82,
    COMM_BM_WRITE_FLASH_LZO                 = 83,
    COMM_SET_CURRENT_REL                     = 84,
    COMM_CAN_FWD_FRAME                       = 85,
    COMM_SET_BATTERY_CUT                     = 86,
    COMM_SET_BLE_NAME                        = 87,
    COMM_SET_BLE_PIN                         = 88,
    COMM_SET_CAN_MODE                        = 89,
    COMM_GET_IMU_CALIBRATION                 = 90,
    COMM_GET_MCCONF_TEMP                     = 91,
    COMM_GET_CUSTOM_CONFIG_XML              = 92,
    COMM_GET_CUSTOM_CONFIG                   = 93,
    COMM_GET_CUSTOM_CONFIG_DEFAULT           = 94,
    COMM_SET_CUSTOM_CONFIG                   = 95,
    COMM_BMS_GET_VALUES                      = 96,
    COMM_BMS_SET_CHARGE_ALLOWED             = 97,
    COMM_BMS_SET_BALANCE_OVERRIDE           = 98,
    COMM_BMS_RESET_COUNTERS                 = 99,
    COMM_BMS_FORCE_BALANCE                  = 100,
    COMM_BMS_ZERO_CURRENT_OFFSET            = 101,
    COMM_JUMP_TO_BOOTLOADER_HW              = 102,
    COMM_ERASE_NEW_APP_HW                   = 103,
    COMM_WRITE_NEW_APP_DATA_HW              = 104,
    COMM_ERASE_BOOTLOADER_HW               = 105,
    COMM_JUMP_TO_BOOTLOADER_ALL_CAN_HW     = 106,
    COMM_ERASE_NEW_APP_ALL_CAN_HW          = 107,
    COMM_WRITE_NEW_APP_DATA_ALL_CAN_HW     = 108,
    COMM_ERASE_BOOTLOADER_ALL_CAN_HW       = 109,
    COMM_SET_ODOMETER                        = 110,
    COMM_PSW_GET_STATUS                      = 111,
    COMM_PSW_SWITCH                          = 112,
    COMM_BMS_FWD_CAN_RX                     = 113,
    COMM_BMS_HW_DATA                         = 114,
    COMM_GET_BATTERY_CUT                     = 115,
    COMM_BM_HALT_REQ                         = 116,
    COMM_GET_QML_UI_HW                      = 117,
    COMM_GET_QML_UI_APP                     = 118,
    COMM_CUSTOM_HW_DATA                     = 119,
    COMM_QMLUI_ERASE                        = 120,
    COMM_QMLUI_WRITE                        = 121,
    COMM_IO_BOARD_GET_ALL                   = 122,
    COMM_IO_BOARD_SET_PWM                   = 123,
    COMM_IO_BOARD_SET_DIGITAL               = 124,
    COMM_BM_MEM_WRITE                        = 125,
    COMM_BMS_BLNC_SELFTEST                  = 126,
    COMM_GET_EXT_HUM_TMP                    = 127,
    COMM_GET_STATS                           = 128,
    COMM_RESET_STATS                         = 129,
    COMM_LISP_READ_CODE                     = 130,
    COMM_LISP_WRITE_CODE                    = 131,
    COMM_LISP_ERASE_CODE                    = 132,
    COMM_LISP_SET_RUNNING                   = 133,
    COMM_LISP_GET_STATS                     = 134,
    COMM_LISP_PRINT                          = 135,
    COMM_BMS_SET_BATT_TYPE                  = 136,
    COMM_BMS_GET_BATT_TYPE                  = 137,
    COMM_LISP_REPL_CMD                      = 138,
    COMM_LISP_STREAM_CODE                   = 139,
    COMM_FILE_LIST                           = 140,
    COMM_FILE_READ                           = 141,
    COMM_FILE_WRITE                          = 142,
    COMM_FILE_MKDIR                          = 143,
    COMM_FILE_REMOVE                         = 144,
    COMM_LOG_START                           = 145,
    COMM_LOG_STOP                            = 146,
    COMM_LOG_CONFIG_FIELD                    = 147,
    COMM_LOG_DATA_F32                        = 148,
    COMM_SET_APPCONF_NO_STORE               = 149,
    COMM_GET_GNSS                            = 150,
    COMM_LOG_DATA_F64                        = 151,
    COMM_LISP_RMSG                           = 152,
    // 153-155 reserved/unused
    COMM_SHUTDOWN                            = 156,
    COMM_FW_INFO                             = 157,
    COMM_CAN_UPDATE_BAUD_ALL                = 158,
    COMM_MOTOR_ESTOP                         = 159,
} COMM_PACKET_ID;
```

### Motor Control Commands

These are the most commonly used commands for controlling the motor from an external MCU. All values are **big-endian** (MSB first).

#### COMM_SET_DUTY (5)

Set motor duty cycle.

```
Payload: [0x05] [int32: duty * 100000]
```

| Field | Type    | Scale    | Range         | Unit |
|-------|---------|----------|---------------|------|
| duty  | int32_t | x100000  | -100000..100000 | ratio (-1.0 to 1.0) |

Example: 50% duty = `0x05 0x00 0x00 0xC3 0x50` (payload = `[5, 50000 as int32 BE]`)

#### COMM_SET_CURRENT (6)

Set motor current.

```
Payload: [0x06] [int32: current_mA]
```

| Field   | Type    | Scale  | Unit |
|---------|---------|--------|------|
| current | int32_t | x1000  | A    |

Example: 10.5A = `0x06 0x00 0x00 0x29 0x04` (payload = `[6, 10500 as int32 BE]`)

#### COMM_SET_CURRENT_BRAKE (7)

Set braking current.

```
Payload: [0x07] [int32: brake_current_mA]
```

Same scaling as COMM_SET_CURRENT (x1000).

#### COMM_SET_RPM (8)

Set target electrical RPM (ERPM).

```
Payload: [0x08] [int32: erpm]
```

| Field | Type    | Scale | Unit |
|-------|---------|-------|------|
| erpm  | int32_t | x1    | ERPM |

Note: ERPM = mechanical RPM * motor pole pairs.

#### COMM_SET_POS (9)

Set target position (requires position control mode in motor config).

```
Payload: [0x09] [int32: position * 1000000]
```

| Field    | Type    | Scale     | Range    | Unit    |
|----------|---------|-----------|----------|---------|
| position | int32_t | x1000000  | 0..360   | degrees |

#### COMM_SET_HANDBRAKE (10)

Set handbrake current.

```
Payload: [0x0A] [float32: current]
```

#### COMM_SET_CURRENT_REL (84)

Set current as a fraction of the configured maximum.

```
Payload: [0x54] [int32: relative * 100000]
```

| Field    | Type    | Scale    | Range            | Unit  |
|----------|---------|----------|------------------|-------|
| relative | int32_t | x100000  | -100000..100000  | ratio |

#### COMM_ALIVE (30)

Keep-alive signal. Send periodically to prevent timeout shutdown when not sending other commands.

```
Payload: [0x1E]
```

### Telemetry: COMM_GET_VALUES

**Request**: Send payload `[0x04]` (single byte).

**Response**: The VESC responds with a payload starting with `[0x04]` followed by telemetry data. The response is approximately 70-80 bytes depending on firmware version.

Response fields (parsed sequentially from the payload, after the command byte):

| # | Field                  | Read Function           | Scale   | Type     | Bytes | Unit    |
|---|------------------------|-------------------------|---------|----------|-------|---------|
| 1 | temp_fet               | buffer_get_float16      | 10.0    | int16_t  | 2     | C       |
| 2 | temp_motor             | buffer_get_float16      | 10.0    | int16_t  | 2     | C       |
| 3 | avg_motor_current      | buffer_get_float32      | 100.0   | int32_t  | 4     | A       |
| 4 | avg_input_current      | buffer_get_float32      | 100.0   | int32_t  | 4     | A       |
| 5 | avg_id (FOC d-axis)    | buffer_get_float32      | 100.0   | int32_t  | 4     | A       |
| 6 | avg_iq (FOC q-axis)    | buffer_get_float32      | 100.0   | int32_t  | 4     | A       |
| 7 | duty_cycle_now         | buffer_get_float16      | 1000.0  | int16_t  | 2     | ratio   |
| 8 | rpm (ERPM)             | buffer_get_float32      | 1.0     | int32_t  | 4     | ERPM    |
| 9 | input_voltage          | buffer_get_float16      | 10.0    | int16_t  | 2     | V       |
| 10| amp_hours              | buffer_get_float32      | 10000.0 | int32_t  | 4     | Ah      |
| 11| amp_hours_charged      | buffer_get_float32      | 10000.0 | int32_t  | 4     | Ah      |
| 12| watt_hours             | buffer_get_float32      | 10000.0 | int32_t  | 4     | Wh      |
| 13| watt_hours_charged     | buffer_get_float32      | 10000.0 | int32_t  | 4     | Wh      |
| 14| tachometer             | buffer_get_int32        | 1       | int32_t  | 4     | steps   |
| 15| tachometer_abs         | buffer_get_int32        | 1       | int32_t  | 4     | steps   |
| 16| fault_code             | (raw byte)              | 1       | uint8_t  | 1     | enum    |
| 17| pid_pos_now            | buffer_get_float32      | 1e6     | int32_t  | 4     | degrees |
| 18| controller_id          | (raw byte)              | 1       | uint8_t  | 1     | ID      |
| 19| temp_mos1              | buffer_get_float16      | 10.0    | int16_t  | 2     | C       |
| 20| temp_mos2              | buffer_get_float16      | 10.0    | int16_t  | 2     | C       |
| 21| temp_mos3              | buffer_get_float16      | 10.0    | int16_t  | 2     | C       |
| 22| vd (FOC d voltage)     | buffer_get_float32      | 1000.0  | int32_t  | 4     | V       |
| 23| vq (FOC q voltage)     | buffer_get_float32      | 1000.0  | int32_t  | 4     | V       |

**Note**: The exact fields depend on firmware version. The authoritative source is `commands.c` in the `vedderb/bldc` repository, in the `COMM_GET_VALUES` handler.

Buffer helper functions decode as follows:
- `buffer_get_float16(buf, scale, &index)`: reads int16_t big-endian, divides by scale
- `buffer_get_float32(buf, scale, &index)`: reads int32_t big-endian, divides by scale
- `buffer_get_int32(buf, &index)`: reads int32_t big-endian

### COMM_GET_VALUES_SELECTIVE

More efficient than COMM_GET_VALUES -- only requests specific fields using a bitmask.

**Request**:
```
Payload: [0x32] [uint32: mask (big-endian)]
```

**Response**: Only the fields corresponding to set bits in the mask are included in the response, in the same order as the full response.

Bitmask field mapping:

| Bit | Field                  |
|-----|------------------------|
| 0   | temp_fet               |
| 1   | temp_motor             |
| 2   | avg_motor_current      |
| 3   | avg_input_current      |
| 4   | avg_id (FOC)           |
| 5   | avg_iq (FOC)           |
| 6   | duty_cycle_now         |
| 7   | erpm                   |
| 8   | input_voltage          |
| 9   | amp_hours              |
| 10  | amp_hours_charged      |
| 11  | watt_hours             |
| 12  | watt_hours_charged     |
| 13  | tachometer             |
| 14  | tachometer_abs         |
| 15  | fault_code             |
| 16  | pid_pos_now            |
| 17  | controller_id          |
| 18  | NTC temps (mos1-3)     |
| 31  | FW version data        |

### Other Useful Commands

#### COMM_FW_VERSION (0)

Request firmware version.

```
Request:  [0x00]
Response: [0x00] [uint8: major] [uint8: minor] [string: hw_name] [bytes: uuid] ...
```

#### COMM_FORWARD_CAN (34)

Forward a command to another VESC on the CAN bus.

```
Payload: [0x22] [uint8: target_can_id] [remaining payload as if sent directly]
```

This allows controlling multiple VESCs through a single UART connection.

#### COMM_TERMINAL_CMD (20)

Send a terminal command string.

```
Payload: [0x14] [null-terminated string]
```

#### COMM_REBOOT (29)

Reboot the VESC.

```
Payload: [0x1D]
```

#### COMM_GET_IMU_DATA (65)

Request IMU sensor data (if IMU is present).

```
Request:  [0x41] [uint16: mask]
Response: [0x41] [IMU data based on mask]
```

---

## CAN Bus Communication

### CAN ID Format

VESC uses **29-bit Extended CAN IDs** (CAN 2.0B). The ID encodes both the command and the target controller:

```
Extended CAN ID (29 bits):
+---------------------------+
| Bits 15-8    | Bits 7-0   |
| CAN_PACKET_ID| VESC_ID    |
+---------------------------+
```

- **Lower 8 bits**: Target VESC controller ID (0-255)
- **Bits 15-8**: `CAN_PACKET_ID` command index
- **Upper bits (28-16)**: Unused (zero)

Example: Sending `CAN_PACKET_SET_CURRENT` (ID=1) to VESC ID 23:
```
CAN Extended ID = (1 << 8) | 23 = 0x0117
```

### CAN_PACKET_ID Reference

```c
typedef enum {
    CAN_PACKET_SET_DUTY                  = 0,
    CAN_PACKET_SET_CURRENT               = 1,
    CAN_PACKET_SET_CURRENT_BRAKE         = 2,
    CAN_PACKET_SET_RPM                   = 3,
    CAN_PACKET_SET_POS                   = 4,
    CAN_PACKET_FILL_RX_BUFFER           = 5,
    CAN_PACKET_FILL_RX_BUFFER_LONG      = 6,
    CAN_PACKET_PROCESS_RX_BUFFER        = 7,
    CAN_PACKET_PROCESS_SHORT_BUFFER     = 8,
    CAN_PACKET_STATUS                    = 9,
    CAN_PACKET_SET_CURRENT_REL          = 10,
    CAN_PACKET_SET_CURRENT_BRAKE_REL    = 11,
    CAN_PACKET_SET_CURRENT_HANDBRAKE    = 12,
    CAN_PACKET_SET_CURRENT_HANDBRAKE_REL = 13,
    CAN_PACKET_STATUS_2                  = 14,
    CAN_PACKET_STATUS_3                  = 15,
    CAN_PACKET_STATUS_4                  = 16,
    CAN_PACKET_PING                      = 17,
    CAN_PACKET_PONG                      = 18,
    CAN_PACKET_DETECT_APPLY_ALL_FOC     = 19,
    CAN_PACKET_DETECT_APPLY_ALL_FOC_RES = 20,
    CAN_PACKET_CONF_CURRENT_LIMITS      = 21,
    CAN_PACKET_CONF_STORE_CURRENT_LIMITS = 22,
    CAN_PACKET_CONF_CURRENT_LIMITS_IN   = 23,
    CAN_PACKET_CONF_STORE_CURRENT_LIMITS_IN = 24,
    CAN_PACKET_CONF_FOC_ERPMS           = 25,
    CAN_PACKET_CONF_STORE_FOC_ERPMS     = 26,
    CAN_PACKET_STATUS_5                  = 27,
    CAN_PACKET_POLL_TS5700N8501_STATUS  = 28,
    CAN_PACKET_CONF_BATTERY_CUT         = 29,
    CAN_PACKET_CONF_STORE_BATTERY_CUT   = 30,
    CAN_PACKET_SHUTDOWN                  = 31,
    CAN_PACKET_IO_BOARD_ADC_1_TO_4      = 32,
    CAN_PACKET_IO_BOARD_ADC_5_TO_8      = 33,
    CAN_PACKET_IO_BOARD_ADC_9_TO_12     = 34,
    CAN_PACKET_IO_BOARD_DIGITAL_IN      = 35,
    CAN_PACKET_IO_BOARD_SET_OUTPUT_DIGITAL = 36,
    CAN_PACKET_IO_BOARD_SET_OUTPUT_PWM  = 37,
    CAN_PACKET_BMS_V_TOT                = 38,
    CAN_PACKET_BMS_I                     = 39,
    CAN_PACKET_BMS_AH_WH                = 40,
    CAN_PACKET_BMS_V_CELL               = 41,
    CAN_PACKET_BMS_BAL                   = 42,
    CAN_PACKET_BMS_TEMPS                 = 43,
    CAN_PACKET_BMS_HUM                   = 44,
    CAN_PACKET_BMS_SOC_SOH_TEMP_STAT    = 45,
    CAN_PACKET_PSW_STAT                  = 46,
    CAN_PACKET_PSW_SWITCH               = 47,
    CAN_PACKET_BMS_HW_DATA_1            = 48,
    CAN_PACKET_BMS_HW_DATA_2            = 49,
    CAN_PACKET_BMS_HW_DATA_3            = 50,
    CAN_PACKET_BMS_HW_DATA_4            = 51,
    CAN_PACKET_BMS_HW_DATA_5            = 52,
    CAN_PACKET_BMS_AH_WH_CHG_TOTAL     = 53,
    CAN_PACKET_BMS_AH_WH_DIS_TOTAL     = 54,
    CAN_PACKET_UPDATE_PID_POS_OFFSET    = 55,
    CAN_PACKET_POLL_ROTOR_POS           = 56,
    CAN_PACKET_NOTIFY_BOOT              = 57,
    CAN_PACKET_STATUS_6                  = 58,
    CAN_PACKET_GNSS_TIME                = 59,
    CAN_PACKET_GNSS_LAT                 = 60,
    CAN_PACKET_GNSS_LON                 = 61,
    CAN_PACKET_GNSS_ALT_SPEED_HDOP     = 62,
    CAN_PACKET_UPDATE_BAUD              = 63,
    CAN_PACKET_BMS_STATUS_1             = 64,
    CAN_PACKET_BMS_STATUS_2             = 65,
    CAN_PACKET_BMS_STATUS_3             = 66,
    CAN_PACKET_BMS_STATUS_4             = 67,
    CAN_PACKET_BMS_STATUS_5             = 68,
} CAN_PACKET_ID;
```

### CAN Motor Control Commands

All simple CAN commands use a **4-byte data payload** containing a **32-bit big-endian signed integer** with a command-specific scaling factor.

| CAN Command                     | ID | Scale     | Unit           | Value Range         |
|---------------------------------|----|-----------|----------------|---------------------|
| CAN_PACKET_SET_DUTY             | 0  | x100000   | ratio          | -1.0 to 1.0        |
| CAN_PACKET_SET_CURRENT          | 1  | x1000     | A              | motor limits        |
| CAN_PACKET_SET_CURRENT_BRAKE    | 2  | x1000     | A              | motor limits        |
| CAN_PACKET_SET_RPM              | 3  | x1        | ERPM           | motor limits        |
| CAN_PACKET_SET_POS              | 4  | x1000000  | degrees        | 0 to 360            |
| CAN_PACKET_SET_CURRENT_REL      | 10 | x100000   | ratio          | -1.0 to 1.0        |
| CAN_PACKET_SET_CURRENT_BRAKE_REL| 11 | x100000   | ratio          | -1.0 to 1.0        |
| CAN_PACKET_SET_CURRENT_HANDBRAKE| 12 | x1000     | A              | motor limits        |
| CAN_PACKET_SET_CURRENT_HANDBRAKE_REL | 13 | x100000 | ratio      | -1.0 to 1.0        |

#### Example: Set 51A current to VESC ID 23

```
CAN Extended ID: (1 << 8) | 23 = 0x0117
Scaled value: 51 * 1000 = 51000 = 0x0000C738
Data bytes: [0x00, 0x00, 0xC7, 0x38]
```

#### Extended Commands with Off-Delay

Some commands support a 6-byte payload adding an off-delay timer:

```
Data: [int32: value (4 bytes)] [int16: off_delay * 1000 (2 bytes)]
```

The off-delay (in seconds, scaled by 1000) tells the VESC how long to maintain the command before timing out. Useful for reducing CAN bus traffic.

### CAN Status Messages

VESCs periodically broadcast status messages on the CAN bus. These are configurable via VESC Tool. Six status message types exist:

#### STATUS (ID 9) - 8 bytes

| Offset | Field      | Type    | Scale  | Unit  |
|--------|------------|---------|--------|-------|
| 0-3    | ERPM       | int32   | x1     | ERPM  |
| 4-5    | Current    | int16   | x10    | A     |
| 6-7    | Duty Cycle | int16   | x1000  | ratio |

#### STATUS_2 (ID 14) - 8 bytes

| Offset | Field              | Type    | Scale   | Unit |
|--------|--------------------|---------|---------|------|
| 0-3    | Amp Hours          | int32   | x10000  | Ah   |
| 4-7    | Amp Hours Charged  | int32   | x10000  | Ah   |

#### STATUS_3 (ID 15) - 8 bytes

| Offset | Field               | Type    | Scale   | Unit |
|--------|---------------------|---------|---------|------|
| 0-3    | Watt Hours          | int32   | x10000  | Wh   |
| 4-7    | Watt Hours Charged  | int32   | x10000  | Wh   |

#### STATUS_4 (ID 16) - 8 bytes

| Offset | Field         | Type    | Scale | Unit    |
|--------|---------------|---------|-------|---------|
| 0-1    | FET Temp      | int16   | x10   | C       |
| 2-3    | Motor Temp    | int16   | x10   | C       |
| 4-5    | Input Current | int16   | x10   | A       |
| 6-7    | PID Position  | int16   | x50   | degrees |

#### STATUS_5 (ID 27) - 8 bytes

| Offset | Field         | Type    | Scale | Unit    |
|--------|---------------|---------|-------|---------|
| 0-3    | Tachometer    | int32   | x6(?) | steps   |
| 4-5    | Input Voltage | int16   | x10   | V       |
| 6-7    | Reserved      | -       | -     | -       |

#### STATUS_6 (ID 58) - 8 bytes

| Offset | Field | Type    | Scale  | Unit |
|--------|-------|---------|--------|------|
| 0-1    | ADC1  | int16   | x1000  | V    |
| 2-3    | ADC2  | int16   | x1000  | V    |
| 4-5    | ADC3  | int16   | x1000  | V    |
| 6-7    | PPM   | int16   | x1000  | ratio|

Status messages transmit at configurable rates (typically 20-50 Hz) via two independent rate groups in VESC Tool.

### Multi-Frame (Buffer) Protocol

Commands that exceed a single CAN frame (8 bytes) use a buffer protocol:

**Short Buffer** (payload <= 6 bytes): Uses `CAN_PACKET_PROCESS_SHORT_BUFFER` (ID 8).

**Long Data**: Split across multiple frames:
1. Fill the receive buffer with `CAN_PACKET_FILL_RX_BUFFER` (ID 5) -- each frame carries 7 bytes of payload data with a 1-byte index prefix
2. Trigger processing with `CAN_PACKET_PROCESS_RX_BUFFER` (ID 7)

This allows sending the full `COMM_PACKET_ID` command set over CAN, not just the simple motor commands.

### CAN Forwarding over UART

When connected to one VESC via UART, you can reach other VESCs on the CAN bus by prepending `COMM_FORWARD_CAN`:

```
UART Payload: [0x22] [target_CAN_ID] [original_command] [command_data...]
               ^                       ^
               COMM_FORWARD_CAN        e.g. COMM_SET_CURRENT
```

This is wrapped in the normal UART packet (start byte, length, CRC, stop byte).

---

## Fault Codes

```c
typedef enum {
    FAULT_CODE_NONE                              = 0,
    FAULT_CODE_OVER_VOLTAGE                      = 1,
    FAULT_CODE_UNDER_VOLTAGE                     = 2,
    FAULT_CODE_DRV                               = 3,
    FAULT_CODE_ABS_OVER_CURRENT                  = 4,
    FAULT_CODE_OVER_TEMP_FET                     = 5,
    FAULT_CODE_OVER_TEMP_MOTOR                   = 6,
    FAULT_CODE_GATE_DRIVER_OVER_VOLTAGE          = 7,
    FAULT_CODE_GATE_DRIVER_UNDER_VOLTAGE         = 8,
    FAULT_CODE_MCU_UNDER_VOLTAGE                 = 9,
    FAULT_CODE_BOOTING_FROM_WATCHDOG_RESET       = 10,
    FAULT_CODE_ENCODER_SPI                       = 11,
    FAULT_CODE_ENCODER_SINCOS_BELOW_MIN_AMPLITUDE = 12,
    FAULT_CODE_ENCODER_SINCOS_ABOVE_MAX_AMPLITUDE = 13,
    FAULT_CODE_FLASH_CORRUPTION                  = 14,
    FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_1      = 15,
    FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_2      = 16,
    FAULT_CODE_HIGH_OFFSET_CURRENT_SENSOR_3      = 17,
    FAULT_CODE_UNBALANCED_CURRENTS               = 18,
    FAULT_CODE_BRK                               = 19,
    FAULT_CODE_RESOLVER_LOT                      = 20,
    FAULT_CODE_RESOLVER_DOS                      = 21,
    FAULT_CODE_RESOLVER_LOS                      = 22,
    FAULT_CODE_FLASH_CORRUPTION_APP_CFG          = 23,
    FAULT_CODE_FLASH_CORRUPTION_MC_CFG           = 24,
    FAULT_CODE_ENCODER_NO_MAGNET                 = 25,
    FAULT_CODE_ENCODER_MAGNET_TOO_STRONG         = 26,
    FAULT_CODE_PHASE_FILTER                      = 27,
    FAULT_CODE_ENCODER_FAULT                     = 28,
    FAULT_CODE_LV_OUTPUT_FAULT                   = 29,
} mc_fault_code;
```

---

## Interfacing from External MCU

### Arduino (VescUart Library)

**Library**: [SolidGeek/VescUart](https://github.com/SolidGeek/VescUart) (recommended, FW5+ compatible)

#### Wiring

| Arduino   | VESC     |
|-----------|----------|
| TX        | RX       |
| RX        | TX       |
| GND       | GND      |
| 5V (opt.) | 5V (opt.)|

Cross-connect TX/RX. Ensure common ground.

#### Setup

```cpp
#include <VescUart.h>

VescUart UART;

void setup() {
    Serial1.begin(115200);      // Hardware serial to VESC
    UART.setSerialPort(&Serial1);

    Serial.begin(115200);       // Debug serial (USB)
    UART.setDebugPort(&Serial);
}
```

#### Reading Telemetry

```cpp
void loop() {
    if (UART.getVescValues()) {
        Serial.print("RPM: ");        Serial.println(UART.data.rpm);
        Serial.print("Voltage: ");    Serial.println(UART.data.inpVoltage);
        Serial.print("Current: ");    Serial.println(UART.data.avgMotorCurrent);
        Serial.print("Duty: ");       Serial.println(UART.data.dutyCycleNow);
        Serial.print("Temp FET: ");   Serial.println(UART.data.tempMosfet);
        Serial.print("Temp Motor: "); Serial.println(UART.data.tempMotor);
        Serial.print("Ah: ");         Serial.println(UART.data.ampHours);
        Serial.print("Wh: ");         Serial.println(UART.data.wattHours);
        Serial.print("Tacho: ");      Serial.println(UART.data.tachometerAbs);
        Serial.print("Fault: ");      Serial.println(UART.data.error);
    }
    delay(50);
}
```

#### Motor Control

```cpp
// Set duty cycle (0.0 to 1.0)
UART.setDuty(0.5);

// Set current in amps
UART.setCurrent(10.0);

// Set brake current in amps
UART.setBrakeCurrent(5.0);

// Set RPM (ERPM)
UART.setRPM(5000);

// Send keepalive (prevents timeout)
UART.sendKeepalive();
```

#### Multi-VESC via CAN Forwarding

All methods accept an optional CAN ID parameter. CAN ID 0 = local VESC.

```cpp
// Read values from VESC with CAN ID 1
UART.getVescValues(1);

// Set current on VESC with CAN ID 2
UART.setCurrent(15.0, 2);

// Set duty on VESC with CAN ID 3
UART.setDuty(0.3, 3);
```

Behind the scenes, the library prepends `COMM_FORWARD_CAN` + CAN ID to the payload:
```
Payload when canId != 0:
[COMM_FORWARD_CAN] [canId] [actual_command] [command_data...]
```

#### Data Structures

```cpp
struct dataPackage {
    float avgMotorCurrent;
    float avgInputCurrent;
    float dutyCycleNow;
    float rpm;
    float inpVoltage;
    float ampHours;
    float ampHoursCharged;
    float wattHours;
    float wattHoursCharged;
    long  tachometer;
    long  tachometerAbs;
    float tempMosfet;
    float tempMotor;
    float pidPos;
    uint8_t id;
    mc_fault_code error;
};
```

#### Full API

| Method                             | Description                              |
|------------------------------------|------------------------------------------|
| `VescUart(timeout_ms = 100)`       | Constructor with configurable timeout    |
| `setSerialPort(Stream* port)`      | Set UART port                            |
| `setDebugPort(Stream* port)`       | Set debug output port                    |
| `getVescValues()`                  | Read telemetry from local VESC           |
| `getVescValues(uint8_t canId)`     | Read telemetry from CAN-connected VESC   |
| `getFWversion()`                   | Get firmware version (local)             |
| `getFWversion(uint8_t canId)`      | Get firmware version (CAN)               |
| `setCurrent(float current)`        | Set motor current (A)                    |
| `setCurrent(float current, uint8_t canId)` | Set current on CAN VESC         |
| `setBrakeCurrent(float current)`   | Set brake current (A)                    |
| `setBrakeCurrent(float current, uint8_t canId)` | Brake on CAN VESC          |
| `setRPM(float rpm)`               | Set target ERPM                          |
| `setRPM(float rpm, uint8_t canId)` | Set ERPM on CAN VESC                    |
| `setDuty(float duty)`             | Set duty cycle (0.0-1.0)                 |
| `setDuty(float duty, uint8_t canId)` | Set duty on CAN VESC                  |
| `setNunchuckValues()`              | Send nunchuck data to local VESC         |
| `setNunchuckValues(uint8_t canId)` | Send nunchuck data to CAN VESC           |
| `sendKeepalive()`                  | Send COMM_ALIVE to local VESC            |
| `sendKeepalive(uint8_t canId)`     | Send COMM_ALIVE to CAN VESC              |
| `printVescValues()`                | Print last values to debug port          |

### STM32 (Official Reference)

**Repository**: [vedderb/bldc_uart_comm_stm32f4_discovery](https://github.com/vedderb/bldc_uart_comm_stm32f4_discovery)

This is Vedder's official reference implementation for UART communication from an STM32F4.

#### Key Files

| File                       | Purpose                                            |
|----------------------------|----------------------------------------------------|
| `bldc_interface.c/h`      | Assembles payloads, parses responses for all commands |
| `bldc_interface_uart.c/h`  | UART abstraction layer                             |
| `packet.c/h`              | Packet framing (start/stop/length/CRC)             |
| `buffer.c/h`              | Type conversion (int/float <-> byte array)         |
| `crc.c/h`                 | CRC-16 CCITT implementation                        |
| `comm_uart.c/h`           | Platform-specific UART driver                      |
| `datatypes.h`             | All VESC data types and enums                      |

#### Wiring (STM32F4 Discovery)

| STM32F4      | VESC |
|--------------|------|
| PB10 (TX)    | RX   |
| PB11 (RX)    | TX   |
| GND          | GND  |

#### Porting to Another Platform

To port the communication code to a different MCU, you need to implement three things in a platform-specific file (replacing `comm_uart.c`):

1. **Initialization**: Set up UART peripheral, register a send callback with the packet layer
2. **Receive handler**: Feed received bytes one at a time to `packet_process_byte()`
3. **Timer**: Provide millisecond timing for timeout detection

The `bldc_interface`, `packet`, `buffer`, and `crc` modules are platform-independent and can be used as-is.

#### Usage Pattern

```c
// Set callback for receiving values
bldc_interface_set_rx_value_func(my_value_callback);

// Request telemetry
bldc_interface_get_values();

// Set motor current
bldc_interface_set_current(10.0);

// Set duty cycle
bldc_interface_set_duty_cycle(0.5);

// Must call periodically to process received bytes
bldc_interface_uart_run_timer();
```

### CAN Bus with Arduino (MCP2515)

**Repository**: [craigg96/vesc_can_bus_arduino](https://github.com/craigg96/vesc_can_bus_arduino)

**Dependency**: [MCP_CAN_lib](https://github.com/coryjfowler/MCP_CAN_lib)

#### VESC Tool Configuration

- VESC ID: set to desired value (e.g. 10)
- CAN Status Message Mode: `CAN_STATUS_1_2_3_4_5`
- CAN Baud Rate: `CAN_BAUD_250K` (match your MCP2515 config)

#### Wiring

MCP2515 module connects to Arduino via SPI. CAN_H and CAN_L connect to VESC CAN bus.

### ESP32

ESP32 has built-in CAN (TWAI) peripheral. No external transceiver chip needed for 3.3V CAN, but a CAN transceiver (like SN65HVD230) is required for the physical bus.

For UART: use any hardware serial port. ESP32 has 3 hardware UARTs.

**Library options**:
- VescUart (SolidGeek) works on ESP32 with Arduino framework
- [vesc_can_sdk](https://github.com/waas-rent/vesc_can_sdk) -- pure C SDK, works with ESP-IDF or Zephyr

---

## Dual/Multi-VESC Setup

### CAN Bus Interconnection

Connect CAN_H to CAN_H and CAN_L to CAN_L between all VESCs. Each VESC must have a unique CAN ID (configured in VESC Tool).

### Control Approaches

**Option A: UART to master, CAN between VESCs**

```
MCU --UART--> VESC_0 --CAN--> VESC_1
                       --CAN--> VESC_2
```

Use `COMM_FORWARD_CAN` to route commands through the UART-connected VESC to others on the bus.

**Option B: Direct CAN from MCU**

```
MCU --CAN--> CAN Bus <-- VESC_0
                     <-- VESC_1
                     <-- VESC_2
```

MCU sends CAN frames directly using `CAN_PACKET_ID` commands. No forwarding needed. Each VESC is addressed by its CAN ID in the extended CAN frame ID.

### Status Message Configuration

In VESC Tool, configure each VESC to broadcast status messages:
- App Settings > General > CAN Status Message Mode
- Options: Disabled, 1, 1_2, 1_2_3, 1_2_3_4, 1_2_3_4_5
- Rate 1 and Rate 2 control the broadcast frequency (Hz)

---

## Timing and Timeout

### Command Timeout

By default, the VESC stops the motor if no command is received within **0.5 seconds** (CAN) or **1 second** (general timeout). This is a safety feature.

**Mitigations**:
- Send commands continuously at a fixed rate (e.g. 50 Hz)
- Send `COMM_ALIVE` (0x1E) as a keepalive when not actively commanding
- Timeout value is configurable in VESC Tool (setting to 0 disables it -- not recommended)

### Recommended Update Rates

| Purpose                | Rate     |
|------------------------|----------|
| Motor commands         | 50 Hz    |
| Telemetry polling      | 10-20 Hz |
| Keepalive (if idle)    | 5-10 Hz  |
| CAN status broadcast   | 20-50 Hz |

### UART Receive Timeout

When waiting for a response (e.g. after `COMM_GET_VALUES`), libraries typically use a 100-300ms timeout for the complete response packet.

---

## Libraries and Resources

### Arduino/Embedded Libraries

| Library | Transport | Platform | URL |
|---------|-----------|----------|-----|
| VescUart (SolidGeek) | UART | Arduino/Teensy/ESP32 | [github.com/SolidGeek/VescUart](https://github.com/SolidGeek/VescUart) |
| VescUartControl (RollingGecko) | UART | Arduino | [github.com/RollingGecko/VescUartControl](https://github.com/RollingGecko/VescUartControl) (discontinued) |
| VescUartLite | UART | Arduino/ESP | [github.com/gitcnd/VescUartLite](https://github.com/gitcnd/VescUartLite) |
| ArduinoVESC | UART | Arduino Mega | [github.com/engineerthenet/ArduinoVESC](https://github.com/engineerthenet/ArduinoVESC) |
| vesc_can_bus_arduino | CAN (MCP2515) | Arduino | [github.com/craigg96/vesc_can_bus_arduino](https://github.com/craigg96/vesc_can_bus_arduino) |
| vesc_can_sdk | CAN | C (Zephyr/bare-metal) | [github.com/waas-rent/vesc_can_sdk](https://github.com/waas-rent/vesc_can_sdk) |

### Official References

| Resource | URL |
|----------|-----|
| VESC firmware source (bldc) | [github.com/vedderb/bldc](https://github.com/vedderb/bldc) |
| VESC Tool source | [github.com/vedderb/vesc_tool](https://github.com/vedderb/vesc_tool) |
| STM32 UART reference | [github.com/vedderb/bldc_uart_comm_stm32f4_discovery](https://github.com/vedderb/bldc_uart_comm_stm32f4_discovery) |
| CAN protocol docs | [github.com/vedderb/bldc/blob/master/documentation/comm_can.md](https://github.com/vedderb/bldc/blob/master/documentation/comm_can.md) |
| Vedder's UART tutorial | [vedder.se/2015/10/communicating-with-the-vesc-using-uart/](http://vedder.se/2015/10/communicating-with-the-vesc-using-uart/) |
| VESC Project forum | [vesc-project.com](https://vesc-project.com) |

### Key Source Files in VESC Firmware

For the definitive protocol specification, read these files in the `vedderb/bldc` repository:

- `datatypes.h` -- all enums (`COMM_PACKET_ID`, `CAN_PACKET_ID`, `mc_fault_code`, etc.)
- `comm/commands.c` -- command handler (payload format for every command)
- `comm/comm_can.c` -- CAN protocol implementation
- `comm/packet.c` -- packet framing layer
- `crc.c` -- CRC-16 implementation
- `documentation/comm_can.md` -- CAN protocol documentation
