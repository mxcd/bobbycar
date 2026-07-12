#include "vesc_serial.h"

HardwareSerial VESCSERIAL(PD6, PD5); // RX, TX

// CRC-16/CCITT-FALSE: poly=0x1021, init=0x0000, xorout=0x0000
// Identical to the table in vedderb/bldc crc.c
// CRC-16/CCITT-FALSE: poly=0x1021, init=0x0000, xorout=0x0000
// Source: vedderb/bldc crc.c (verified against SolidGeek/VescUart)
const uint16_t VescSerial::crc16_tab[256] = {
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
  0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
  0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
  0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
  0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
  0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
  0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
  0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
  0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
  0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
  0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
  0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
  0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
  0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
  0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
  0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
  0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
  0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
  0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
  0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
  0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
  0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
  0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
  0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
  0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
  0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
  0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
  0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

VescSerial::VescSerial()
  : _rxState(WAIT_START), _rxLen(0), _rxIdx(0), _rxCrc(0),
    rxByteCount(0), txByteCount(0),
    rxPacketsOk(0), rxCrcFails(0), rxEndByteFails(0), rxGetValuesOk(0),
    rxLastLen(0), rxLastCmd(0), rxLastComputedCrc(0), rxLastReceivedCrc(0),
    inputVoltage(0), fetTemp(0), motorTemp(0),
    inputCurrent(0), dutyCycle(0), faultCode(0),
    erpmMotor1(0), erpmMotor2(0) {}

void VescSerial::setup() {
  VESCSERIAL.begin(VESC_SERIAL_BAUD);
}

// ---------- CRC ----------

uint16_t VescSerial::crc16(const uint8_t *buf, uint16_t len) {
  uint16_t crc = 0;
  for (uint16_t i = 0; i < len; i++) {
    crc = (crc << 8) ^ crc16_tab[((crc >> 8) ^ buf[i]) & 0xFF];
  }
  return crc;
}

// ---------- Helpers ----------

int16_t VescSerial::getInt16(const uint8_t *buf, int idx) {
  return (int16_t)((uint16_t)buf[idx] << 8 | buf[idx + 1]);
}

int32_t VescSerial::getInt32(const uint8_t *buf, int idx) {
  return (int32_t)((uint32_t)buf[idx] << 24 | (uint32_t)buf[idx + 1] << 16 |
                   (uint32_t)buf[idx + 2] << 8 | buf[idx + 3]);
}

void VescSerial::putInt32(uint8_t *buf, int idx, int32_t val) {
  buf[idx]     = (val >> 24) & 0xFF;
  buf[idx + 1] = (val >> 16) & 0xFF;
  buf[idx + 2] = (val >> 8)  & 0xFF;
  buf[idx + 3] = val & 0xFF;
}

// ---------- Packet framing ----------
// Short-packet format (payload <= 255):
//   [0x02] [len] [payload...] [crc_hi] [crc_lo] [0x03]
// CRC is over payload only.

void VescSerial::sendPacket(const uint8_t *payload, uint8_t len) {
  uint8_t frame[VESC_TX_BUFFER_SIZE];
  if (len + 5 > VESC_TX_BUFFER_SIZE) return; // safety guard

  uint16_t crc = crc16(payload, len);
  frame[0] = 0x02;
  frame[1] = len;
  memcpy(&frame[2], payload, len);
  frame[2 + len] = (crc >> 8) & 0xFF;
  frame[3 + len] = crc & 0xFF;
  frame[4 + len] = 0x03;
  VESCSERIAL.write(frame, len + 5);
  txByteCount += len + 5;
}

// ---------- Motor 1 — direct UART ----------

void VescSerial::setCurrent(int32_t currentMa) {
  uint8_t p[5];
  p[0] = COMM_SET_CURRENT;
  putInt32(p, 1, currentMa);
  sendPacket(p, 5);
}

void VescSerial::setBrakeCurrent(int32_t brakeMa) {
  uint8_t p[5];
  p[0] = COMM_SET_CURRENT_BRAKE;
  putInt32(p, 1, brakeMa);
  sendPacket(p, 5);
}

// ---------- Motor 2 — CAN-forwarded ----------

void VescSerial::setCurrentCan(int32_t currentMa, uint8_t canId) {
  uint8_t p[7];
  p[0] = COMM_FORWARD_CAN;
  p[1] = canId;
  p[2] = COMM_SET_CURRENT;
  putInt32(p, 3, currentMa);
  sendPacket(p, 7);
}

void VescSerial::setBrakeCurrentCan(int32_t brakeMa, uint8_t canId) {
  uint8_t p[7];
  p[0] = COMM_FORWARD_CAN;
  p[1] = canId;
  p[2] = COMM_SET_CURRENT_BRAKE;
  putInt32(p, 3, brakeMa);
  sendPacket(p, 7);
}

void VescSerial::setHandbrakeCan(int32_t brakeMa, uint8_t canId) {
  uint8_t p[7];
  p[0] = COMM_FORWARD_CAN;
  p[1] = canId;
  p[2] = COMM_SET_HANDBRAKE;
  putInt32(p, 3, brakeMa);
  sendPacket(p, 7);
}

// ---------- Keepalive & telemetry requests ----------

void VescSerial::sendAlive() {
  uint8_t p[1] = { COMM_ALIVE };
  sendPacket(p, 1);
}

void VescSerial::requestValues() {
  uint8_t p[1] = { COMM_GET_VALUES };
  sendPacket(p, 1);
}

void VescSerial::requestValuesCan(uint8_t canId) {
  uint8_t p[3];
  p[0] = COMM_FORWARD_CAN;
  p[1] = canId;
  p[2] = COMM_GET_VALUES;
  sendPacket(p, 3);
}

void VescSerial::requestValuesSelective(uint32_t mask) {
  uint8_t p[5];
  p[0] = COMM_GET_VALUES_SELECTIVE;
  putInt32(p, 1, (int32_t)mask);
  sendPacket(p, 5);
}

void VescSerial::requestValuesSelectiveCan(uint32_t mask, uint8_t canId) {
  uint8_t p[7];
  p[0] = COMM_FORWARD_CAN;
  p[1] = canId;
  p[2] = COMM_GET_VALUES_SELECTIVE;
  putInt32(p, 3, (int32_t)mask);
  sendPacket(p, 7);
}

// ---------- COMM_GET_VALUES response parsing ----------
// Field offsets within payload (byte 0 = COMM_GET_VALUES):
//   [1..2]   fetTemp           int16 / 10.0   (°C)
//   [3..4]   motorTemp         int16 / 10.0   (°C)
//   [5..8]   avgMotorCurrent   int32 / 100.0  (A)
//   [9..12]  avgInputCurrent   int32 / 100.0  (A)
//   [13..16] avgId             int32 / 100.0  (A)
//   [17..20] avgIq             int32 / 100.0  (A)
//   [21..22] dutyCycle         int16 / 1000.0
//   [23..26] erpm              int32
//   [27..28] inputVoltage      int16 / 10.0   (V)
//   [29..32] ampHours          int32 / 10000.0
//   [33..36] ampHoursCharged   int32 / 10000.0
//   [37..40] wattHours         int32 / 10000.0
//   [41..44] wattHoursCharged  int32 / 10000.0
//   [45..48] tachometer        int32
//   [49..52] tachometerAbs     int32
//   [53]     faultCode         uint8
//   [54..57] pidPos            int32 / 1000000.0
//   [58]     controllerId      uint8
// Minimum usable length: 59 bytes (through controllerId)

void VescSerial::processGetValues(const uint8_t *payload, uint16_t len) {
  if (len < 59) return;

  float motorCurrent = getInt32(payload, 5) / 100.0f;
  int32_t erpm       = getInt32(payload, 23);
  uint8_t ctrlId     = payload[58];

  if (ctrlId == VESC_CAN_ID_MOTOR2) {
    erpmMotor2 = erpm;
    setState(VD_MOTOR_CURRENT_RIGHT, motorCurrent);
    setState(VD_ERPM_RIGHT, (int)erpm);
  } else {
    // Motor 1 (UART-connected) — also carries shared battery/thermal data
    setState(VD_MOTOR_CURRENT_LEFT, motorCurrent);
    setState(VD_ERPM_LEFT, (int)erpm);

    fetTemp      = getInt16(payload, 1)  / 10.0f;
    motorTemp    = getInt16(payload, 3)  / 10.0f;
    inputCurrent = getInt32(payload, 9)  / 100.0f;
    dutyCycle    = getInt16(payload, 21) / 1000.0f;
    inputVoltage = getInt16(payload, 27) / 10.0f;
    faultCode    = payload[53];

    setState(VD_BATTERY_VOLTAGE, inputVoltage);
    setState(VD_FET_TEMP, fetTemp);
    setState(VD_MOTOR_TEMP, motorTemp);
    setState(VD_INPUT_CURRENT, inputCurrent);
    setState(VD_DUTY_CYCLE, dutyCycle);
    setState(VD_FAULT_CODE, faultCode);
  }
}

// ---------- COMM_GET_VALUES_SELECTIVE response parsing ----------
// Response: [50][mask uint32][fields in mask-bit order]. Field widths per
// mask bit follow the COMM_GET_VALUES field order (vedderb/bldc commands.c).
// Parsing is driven by the mask ECHOED by the VESC, not by what we requested,
// so a firmware that serves fewer/more bits stays correctly aligned.
// Full bit table: docs/logging.md

void VescSerial::processGetValuesSelective(const uint8_t *payload, uint16_t len) {
  if (len < 5) return;
  uint32_t mask = (uint32_t)getInt32(payload, 1);
  uint16_t idx = 5;
  static const uint8_t widths[] = {2, 2, 4, 4, 4, 4, 2, 4, 2, 4, 4,
                                   4, 4, 4, 4, 1, 4, 1, 6, 4, 4, 1};

  float fet = 0, motor = 0, current = 0, inCurrent = 0, duty = 0, voltage = 0;
  int32_t erpm = 0;
  uint8_t fault = 0, ctrlId = 0;
  bool hasCtrlId = false;

  for (uint8_t bit = 0; bit < sizeof(widths); bit++) {
    if (!(mask & ((uint32_t)1 << bit))) continue;
    if (idx + widths[bit] > len) return; // truncated response
    switch (bit) {
      case 0:  fet       = getInt16(payload, idx) / 10.0f;   break;
      case 1:  motor     = getInt16(payload, idx) / 10.0f;   break;
      case 2:  current   = getInt32(payload, idx) / 100.0f;  break;
      case 3:  inCurrent = getInt32(payload, idx) / 100.0f;  break;
      case 6:  duty      = getInt16(payload, idx) / 1000.0f; break;
      case 7:  erpm      = getInt32(payload, idx);           break;
      case 8:  voltage   = getInt16(payload, idx) / 10.0f;   break;
      case 15: fault     = payload[idx];                     break;
      case 17: ctrlId    = payload[idx]; hasCtrlId = true;   break;
      default: break; // requested by someone else / not consumed — skip
    }
    idx += widths[bit];
  }
  if (!hasCtrlId) return; // cannot route to a motor without controller id

  if (ctrlId == VESC_CAN_ID_MOTOR2) {
    erpmMotor2 = erpm;
    setState(VD_MOTOR_CURRENT_RIGHT, current);
    setState(VD_ERPM_RIGHT, (int)erpm);
  } else {
    // Motor 1 (UART-connected) — also carries shared battery/thermal data
    erpmMotor1 = erpm;
    setState(VD_MOTOR_CURRENT_LEFT, current);
    setState(VD_ERPM_LEFT, (int)erpm);

    fetTemp      = fet;
    motorTemp    = motor;
    inputCurrent = inCurrent;
    dutyCycle    = duty;
    inputVoltage = voltage;
    faultCode    = fault;

    setState(VD_BATTERY_VOLTAGE, voltage);
    setState(VD_FET_TEMP, fet);
    setState(VD_MOTOR_TEMP, motor);
    setState(VD_INPUT_CURRENT, inCurrent);
    setState(VD_DUTY_CYCLE, duty);
    setState(VD_FAULT_CODE, fault);
  }
}

// ---------- Receive state machine ----------
// Handles both short (0x02) and long (0x03) packet start bytes.

bool VescSerial::receive() {
  bool gotPacket = false;

  while (VESCSERIAL.available()) {
    uint8_t b = VESCSERIAL.read();
    rxByteCount++;

    switch (_rxState) {
      case WAIT_START:
        if (b == 0x02) {
          _rxState = READ_LEN_SHORT;
        } else if (b == 0x03) {
          _rxState = READ_LEN_LONG_H;
        }
        break;

      case READ_LEN_SHORT:
        _rxLen = b;
        _rxIdx = 0;
        _rxState = (_rxLen > 0 && _rxLen <= VESC_RX_BUFFER_SIZE) ? READ_PAYLOAD : WAIT_START;
        break;

      case READ_LEN_LONG_H:
        _rxLen = (uint16_t)b << 8;
        _rxState = READ_LEN_LONG_L;
        break;

      case READ_LEN_LONG_L:
        _rxLen |= b;
        _rxIdx = 0;
        _rxState = (_rxLen > 0 && _rxLen <= VESC_RX_BUFFER_SIZE) ? READ_PAYLOAD : WAIT_START;
        break;

      case READ_PAYLOAD:
        _rxBuf[_rxIdx++] = b;
        if (_rxIdx >= _rxLen) _rxState = READ_CRC_H;
        break;

      case READ_CRC_H:
        _rxCrc = (uint16_t)b << 8;
        _rxState = READ_CRC_L;
        break;

      case READ_CRC_L:
        _rxCrc |= b;
        _rxState = READ_STOP;
        break;

      case READ_STOP:
        rxLastLen = _rxLen;
        rxLastCmd = (_rxLen > 0) ? _rxBuf[0] : 0xFF;
        rxLastComputedCrc = crc16(_rxBuf, _rxLen);
        rxLastReceivedCrc = _rxCrc;
        if (b != 0x03) {
          rxEndByteFails++;
        } else if (rxLastComputedCrc == rxLastReceivedCrc) {
          rxPacketsOk++;
          if (_rxBuf[0] == COMM_GET_VALUES) {
            rxGetValuesOk++;
            processGetValues(_rxBuf, _rxLen);
            gotPacket = true;
          } else if (_rxBuf[0] == COMM_GET_VALUES_SELECTIVE) {
            rxGetValuesOk++;
            processGetValuesSelective(_rxBuf, _rxLen);
            gotPacket = true;
          }
        } else {
          rxCrcFails++;
        }
        _rxState = WAIT_START;
        break;
    }
  }

  return gotPacket;
}
