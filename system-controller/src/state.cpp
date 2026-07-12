#include "state.h"

static bob_Telemetry telemetry = bob_Telemetry_init_zero;

void stateSetup() {}

bob_Telemetry* getTelemetry() {
  return &telemetry;
}

static uint32_t toU32(double v) {
  return v < 0 ? 0 : (uint32_t)v;
}

static void setValue(StateData key, double v) {
  switch (key) {
    case STATE_LOOP_COUNTER:            telemetry.loop_counter = toU32(v); break;
    case STEERING_STARTUP_CHECK_OK:     telemetry.steering_startup_check_ok = v != 0; break;
    case STEERING_DEAD_MAN_SWITCH_PRESSED: telemetry.steering_dead_man_switch_pressed = v != 0; break;
    case STEERING_ACCELERATION_COMMAND: telemetry.steering_acceleration_command = v != 0; break;
    case STEERING_BRAKE_COMMAND:        telemetry.steering_brake_command = v != 0; break;
    case STEERING_PANIC:                telemetry.steering_panic = v != 0; break;
    case STEERING_WATCHDOG_INDEX:       telemetry.steering_watchdog_index = toU32(v); break;
    case STEERING_TIME_SINCE_LAST_WATCHDOG_MESSAGE: telemetry.steering_time_since_last_watchdog_message = toU32(v); break;
    case STEERING_THROTTLE:             telemetry.steering_throttle = toU32(v); break;
    case RELAY_MC_EN:                   telemetry.relay_mc_en = v != 0; break;
    case RELAY_PRECHARGE:               telemetry.relay_precharge = v != 0; break;
    case RELAY_SC_OK:                   telemetry.relay_sc_ok = v != 0; break;
    case BATTERY_OK:                    telemetry.battery_ok = v != 0; break;
    case VEHICLE_STATE:                 telemetry.vehicle_state = toU32(v); break;
    case VD_BATTERY_VOLTAGE:            telemetry.vd_battery_voltage = (float)v; break;
    case VD_DERATING_FACTOR:            telemetry.vd_derating_factor = (float)v; break;
    case VD_FET_TEMP:                   telemetry.vd_fet_temp = (float)v; break;
    case VD_MOTOR_TEMP:                 telemetry.vd_motor_temp = (float)v; break;
    case VD_FAULT_CODE:                 telemetry.vd_fault_code = toU32(v); break;
    case VD_ERPM_LEFT:                  telemetry.vd_erpm_left = (int32_t)v; break;
    case VD_ERPM_RIGHT:                 telemetry.vd_erpm_right = (int32_t)v; break;
    case VD_MOTOR_CURRENT_LEFT:         telemetry.vd_motor_current_left = (float)v; break;
    case VD_MOTOR_CURRENT_RIGHT:        telemetry.vd_motor_current_right = (float)v; break;
    case VD_INPUT_CURRENT:              telemetry.vd_input_current = (float)v; break;
    case VD_DUTY_CYCLE:                 telemetry.vd_duty_cycle = (float)v; break;
    case VD_CURRENT_REQUEST_LEFT:       telemetry.vd_current_request_left = (int32_t)v; break;
    case VD_CURRENT_REQUEST_RIGHT:      telemetry.vd_current_request_right = (int32_t)v; break;
    case VD_VESC_RX_BYTES:              telemetry.vd_vesc_rx_bytes = toU32(v); break;
    case VD_VESC_TX_BYTES:              telemetry.vd_vesc_tx_bytes = toU32(v); break;
    case VD_VESC_RX_PACKETS_OK:         telemetry.vd_vesc_rx_packets_ok = toU32(v); break;
    case VD_VESC_RX_CRC_FAILS:          telemetry.vd_vesc_rx_crc_fails = toU32(v); break;
    case VD_VESC_RX_END_BYTE_FAILS:     telemetry.vd_vesc_rx_end_byte_fails = toU32(v); break;
    case VD_VESC_RX_GET_VALUES_OK:      telemetry.vd_vesc_rx_get_values_ok = toU32(v); break;
    case VD_VESC_RX_LAST_LEN:           telemetry.vd_vesc_rx_last_len = toU32(v); break;
    case VD_VESC_RX_LAST_CMD:           telemetry.vd_vesc_rx_last_cmd = toU32(v); break;
    case VD_VESC_RX_COMPUTED_CRC:       telemetry.vd_vesc_rx_computed_crc = toU32(v); break;
    case VD_VESC_RX_RECEIVED_CRC:       telemetry.vd_vesc_rx_received_crc = toU32(v); break;
  }
}

void setState(StateData key, uint8_t value)  { setValue(key, value); }
void setState(StateData key, uint32_t value) { setValue(key, value); }
void setState(StateData key, int value)      { setValue(key, value); }
void setState(StateData key, float value)    { setValue(key, value); }
void setState(StateData key, bool value)     { setValue(key, value ? 1 : 0); }
