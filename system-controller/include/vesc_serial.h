#pragma once

#include <Arduino.h>
#include "state.h"

// VESC command IDs (from vedderb/bldc datatypes.h)
#define COMM_GET_VALUES            4
#define COMM_SET_CURRENT           6
#define COMM_SET_CURRENT_BRAKE     7
#define COMM_SET_HANDBRAKE        10
#define COMM_ALIVE                30
#define COMM_FORWARD_CAN          34
#define COMM_GET_VALUES_SELECTIVE 50

// Configuration
#define VESC_SERIAL_BAUD      115200
#define VESC_CAN_ID_MOTOR1    23      // CAN ID of first motor on Duet XS
#define VESC_CAN_ID_MOTOR2    24      // CAN ID of second motor on Duet XS
#define VESC_RX_BUFFER_SIZE   128
#define VESC_TX_BUFFER_SIZE   16      // largest outbound packet: FORWARD_CAN + SET_CURRENT = 12 bytes framed

class VescSerial {
public:
  VescSerial();
  void setup();

  // Receive and parse incoming VESC packets. Returns true if a complete
  // COMM_GET_VALUES response was successfully parsed this call.
  bool receive();

  // --- Motor 1 (UART-connected, local VESC on Duet XS) ---
  void setCurrent(int32_t currentMa);
  void setBrakeCurrent(int32_t brakeMa);

  // --- Motor 2 (CAN-forwarded to second VESC on Duet XS) ---
  void setCurrentCan(int32_t currentMa, uint8_t canId = VESC_CAN_ID_MOTOR2);
  void setBrakeCurrentCan(int32_t brakeMa, uint8_t canId = VESC_CAN_ID_MOTOR2);
  void setHandbrakeCan(int32_t brakeMa, uint8_t canId = VESC_CAN_ID_MOTOR2);

  // Keepalive — resets VESC timeout
  void sendAlive();

  // Request telemetry from motor 1 (local) and motor 2 (CAN)
  void requestValues();
  void requestValuesCan(uint8_t canId = VESC_CAN_ID_MOTOR2);

  // Selective telemetry (COMM_GET_VALUES_SELECTIVE) — smaller responses,
  // field mask bits documented in docs/logging.md
  void requestValuesSelective(uint32_t mask);
  void requestValuesSelectiveCan(uint32_t mask, uint8_t canId = VESC_CAN_ID_MOTOR2);

  // Parsed telemetry (updated by receive())
  uint32_t rxByteCount;
  uint32_t txByteCount;
  uint16_t rxPacketsOk;
  uint16_t rxCrcFails;
  uint16_t rxEndByteFails;
  uint16_t rxGetValuesOk;
  uint16_t rxLastLen;
  uint8_t  rxLastCmd;
  uint16_t rxLastComputedCrc;
  uint16_t rxLastReceivedCrc;
  float inputVoltage;
  float fetTemp;
  float motorTemp;
  float inputCurrent;
  float dutyCycle;
  uint8_t faultCode;
  int32_t erpmMotor1;
  int32_t erpmMotor2;

private:
  enum RxState {
    WAIT_START,
    READ_LEN_SHORT,
    READ_LEN_LONG_H,
    READ_LEN_LONG_L,
    READ_PAYLOAD,
    READ_CRC_H,
    READ_CRC_L,
    READ_STOP
  };

  RxState  _rxState;
  uint8_t  _rxBuf[VESC_RX_BUFFER_SIZE];
  uint16_t _rxLen;
  uint16_t _rxIdx;
  uint16_t _rxCrc;

  static const uint16_t crc16_tab[256];
  static uint16_t crc16(const uint8_t *buf, uint16_t len);

  void sendPacket(const uint8_t *payload, uint8_t len);
  void processGetValues(const uint8_t *payload, uint16_t len);
  void processGetValuesSelective(const uint8_t *payload, uint16_t len);

  static int16_t  getInt16(const uint8_t *buf, int idx);
  static int32_t  getInt32(const uint8_t *buf, int idx);
  static void     putInt32(uint8_t *buf, int idx, int32_t val);
};
