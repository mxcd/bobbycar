#include "state.h"

JsonDocument stateData;

void stateSetup() {
  stateData[STATE_LOOP_COUNTER] = (uint32_t)0;
}

void setState(StateData key, uint8_t value) {
  stateData[getKeyString(key)] = value;
}
void setState(StateData key, uint32_t value) {
  stateData[getKeyString(key)] = value;
}
void setState(StateData key, int value) {
  stateData[getKeyString(key)] = value;
}
void setState(StateData key, float value) {
  stateData[getKeyString(key)] = value;
}

void setState(StateData key, bool value) {
  stateData[getKeyString(key)] = value;
}

String getKeyString(StateData key) {
  switch(key) {
    case STATE_LOOP_COUNTER:
      return "loopCounter";
      break;
    case STEERING_STARTUP_CHECK_OK:
      return "steeringStartupCheckOk";
      break;
    case STEERING_DEAD_MAN_SWITCH_PRESSED:
      return "steeringDeadManSwitchPressed";
      break;
    case STEERING_ACCELERATION_COMMAND:
      return "steeringAccelerationCommand";
      break;
    case STEERING_BRAKE_COMMAND:
      return "steeringBrakeCommand";
      break;
    case STEERING_PANIC:
      return "steeringPanic";
      break;
    case STEERING_WATCHDOG_INDEX:
      return "steeringWatchdogIndex";
      break;
    case STEERING_TIME_SINCE_LAST_WATCHDOG_MESSAGE:
      return "steeringTimeSinceLastWatchdogMessage";
      break;
    case RELAY_MC_EN:
      return "relayMcEn";
      break;
    case RELAY_PRECHARGE:
      return "relayPrecharge";
      break;
    case RELAY_SC_OK:
      return "relayScOk";
      break;
    case VD_SPEED_REQUEST_LEFT:
      return "vdSpeedRequestLeft";
      break;
    case VD_SPEED_REQUEST_RIGHT:
      return "vdSpeedRequestRight";
      break;
    case VEHICLE_STATE:
      return "vehicleState";
      break;
    case VD_SPEED_LEFT:
      return "vdSpeedLeft";
      break;
    case VD_SPEED_RIGHT:
      return "vdSpeedRight";
      break;
    case VD_BATTERY_VOLTAGE:
      return "vdBatteryVoltage";
      break;
  }
  return "foo";
}

JsonDocument getStateData() {
  return stateData;
}