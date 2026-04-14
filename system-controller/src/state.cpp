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
    case BATTERY_OK:
      return "batteryOk";
      break;
    case STEERING_THROTTLE:
      return "steeringThrottle";
      break;
    case VEHICLE_STATE:
      return "vehicleState";
      break;
    case VD_BATTERY_VOLTAGE:
      return "vdBatteryVoltage";
      break;
    case VD_DERATING_FACTOR:
      return "vdDeratingFactor";
      break;
    case VD_FET_TEMP:
      return "vdFetTemp";
      break;
    case VD_MOTOR_TEMP:
      return "vdMotorTemp";
      break;
    case VD_FAULT_CODE:
      return "vdFaultCode";
      break;
    case VD_ERPM_LEFT:
      return "vdErpmLeft";
      break;
    case VD_ERPM_RIGHT:
      return "vdErpmRight";
      break;
    case VD_MOTOR_CURRENT_LEFT:
      return "vdMotorCurrentLeft";
      break;
    case VD_MOTOR_CURRENT_RIGHT:
      return "vdMotorCurrentRight";
      break;
    case VD_INPUT_CURRENT:
      return "vdInputCurrent";
      break;
    case VD_DUTY_CYCLE:
      return "vdDutyCycle";
      break;
    case VD_CURRENT_REQUEST_LEFT:
      return "vdCurrentRequestLeft";
      break;
    case VD_CURRENT_REQUEST_RIGHT:
      return "vdCurrentRequestRight";
      break;
    case VD_VESC_RX_BYTES:
      return "vdVescRxBytes";
      break;
    case VD_VESC_TX_BYTES:
      return "vdVescTxBytes";
      break;
    case VD_VESC_RX_PACKETS_OK:
      return "vdVescRxPacketsOk";
      break;
    case VD_VESC_RX_CRC_FAILS:
      return "vdVescRxCrcFails";
      break;
    case VD_VESC_RX_END_BYTE_FAILS:
      return "vdVescRxEndByteFails";
      break;
    case VD_VESC_RX_GET_VALUES_OK:
      return "vdVescRxGetValuesOk";
      break;
    case VD_VESC_RX_LAST_LEN:
      return "vdVescRxLastLen";
      break;
    case VD_VESC_RX_LAST_CMD:
      return "vdVescRxLastCmd";
      break;
    case VD_VESC_RX_COMPUTED_CRC:
      return "vdVescRxComputedCrc";
      break;
    case VD_VESC_RX_RECEIVED_CRC:
      return "vdVescRxReceivedCrc";
      break;
  }
  return "foo";
}

JsonDocument getStateData() {
  return stateData;
}
