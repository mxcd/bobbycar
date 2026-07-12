#pragma once
#include <Arduino.h>
#include "telemetry.pb.h"

enum StateData {
  STATE_LOOP_COUNTER,
  STEERING_STARTUP_CHECK_OK,
  STEERING_DEAD_MAN_SWITCH_PRESSED,
  STEERING_ACCELERATION_COMMAND,
  STEERING_BRAKE_COMMAND,
  STEERING_PANIC,
  STEERING_WATCHDOG_INDEX,
  STEERING_TIME_SINCE_LAST_WATCHDOG_MESSAGE,
  STEERING_THROTTLE,
  RELAY_MC_EN,
  RELAY_PRECHARGE,
  RELAY_SC_OK,
  BATTERY_OK,
  VEHICLE_STATE,
  VD_BATTERY_VOLTAGE,
  VD_DERATING_FACTOR,
  VD_FET_TEMP,
  VD_MOTOR_TEMP,
  VD_FAULT_CODE,
  VD_ERPM_LEFT,
  VD_ERPM_RIGHT,
  VD_MOTOR_CURRENT_LEFT,
  VD_MOTOR_CURRENT_RIGHT,
  VD_INPUT_CURRENT,
  VD_DUTY_CYCLE,
  VD_CURRENT_REQUEST_LEFT,
  VD_CURRENT_REQUEST_RIGHT,
  VD_VESC_RX_BYTES,
  VD_VESC_TX_BYTES,
  VD_VESC_RX_PACKETS_OK,
  VD_VESC_RX_CRC_FAILS,
  VD_VESC_RX_END_BYTE_FAILS,
  VD_VESC_RX_GET_VALUES_OK,
  VD_VESC_RX_LAST_LEN,
  VD_VESC_RX_LAST_CMD,
  VD_VESC_RX_COMPUTED_CRC,
  VD_VESC_RX_RECEIVED_CRC
};

// The telemetry struct IS the vehicle state store — encoded as-is onto the wire.
bob_Telemetry* getTelemetry();

void stateSetup();
void setState(StateData key, uint8_t value);
void setState(StateData key, uint32_t value);
void setState(StateData key, int value);
void setState(StateData key, float value);
void setState(StateData key, bool value);
