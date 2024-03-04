#pragma once
#include <ArduinoJson.h>

enum StateData {
  STATE_LOOP_COUNTER,
  STEERING_STARTUP_CHECK_OK,
  STEERING_DEAD_MAN_SWITCH_PRESSED,
  STEERING_ACCELERATION_COMMAND,
  STEERING_BRAKE_COMMAND,
  STEERING_PANIC,
  STEERING_WATCHDOG_INDEX,
  STEERING_TIME_SINCE_LAST_WATCHDOG_MESSAGE,
  RELAY_MC_EN,
  RELAY_PRECHARGE,
  RELAY_SC_OK,
  VEHICLE_STATE,
  VD_SPEED_REQUEST_LEFT,
  VD_SPEED_REQUEST_RIGHT,
  VD_SPEED_LEFT,
  VD_SPEED_RIGHT,
  VD_BATTERY_VOLTAGE,
};

JsonDocument getStateData();
void stateSetup();

String getKeyString(StateData key);
void setState(StateData key, uint8_t value);
void setState(StateData key, uint32_t value);
void setState(StateData key, int value);
void setState(StateData key, float value);
void setState(StateData key, bool value);