#include "vd.h"
#include <elapsedMillis.h>

#define MOTOR1_ENABLED 1
#define MOTOR2_ENABLED 1

// Maximum motor current at full throttle (mA)
#define MAX_CURRENT_MA 35000

// Brake current when brake button pressed (mA)
#define BRAKE_CURRENT_MA 20000

// Brake current when coasting — DMS held, throttle released (mA)
#define COAST_CURRENT_MA 5000

// Brake current for safe stop — DMS released, driver off vehicle (mA)
#define SAFE_STOP_CURRENT_MA 10000

// Throttle calibration (raw ADC range from steering controller)
#define THROTTLE_CAL_LOW  15
#define THROTTLE_CAL_HIGH 250

// COMM_GET_VALUES_SELECTIVE field mask: fetTemp, motorTemp, motorCurrent,
// inputCurrent, dutyCycle, erpm, inputVoltage, faultCode, controllerId.
// Full bit table: docs/logging.md
#define VESC_TELEMETRY_MASK ((1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) | \
                             (1u << 6) | (1u << 7) | (1u << 8) | (1u << 15) | (1u << 17))

VescSerial vescSerial;

// One UART packet per 10ms tick, 4-tick cycle:
//   0: command motor 1   1: command motor 2
//   2: telemetry motor 1 3: telemetry motor 2
// → 25Hz commands and 25Hz telemetry per motor, no packet collisions.
static uint8_t tick = 0;

// Throttle interlock: prevents torque on DMS re-engage until throttle is zero for 500ms
static bool dmsWasPressed = false;
static bool throttleInterlock = true; // active at startup
static elapsedMillis throttleZeroTimer;

void vdSetup() {
  vescSerial.setup();
  setState(VD_BATTERY_VOLTAGE, 0.0f);
  setState(VD_CURRENT_REQUEST_LEFT, int(0));
  setState(VD_CURRENT_REQUEST_RIGHT, int(0));
}

// Compute motor current from throttle input.
static int32_t throttleToCurrentMa() {
  uint8_t raw = getThrottle();
  if (raw <= THROTTLE_CAL_LOW) return 0;
  int32_t clamped = min((int32_t)raw, (int32_t)THROTTLE_CAL_HIGH);
  return (clamped - THROTTLE_CAL_LOW) * MAX_CURRENT_MA / (THROTTLE_CAL_HIGH - THROTTLE_CAL_LOW);
}

void vdLoop() {
  vescSerial.receive();

  // --- Throttle interlock: track DMS transitions and throttle zero duration ---
  bool dmsPressed = hasDeadManSwitch();
  if (dmsPressed && !dmsWasPressed) {
    throttleInterlock = true;
    throttleZeroTimer = 0;
  }
  dmsWasPressed = dmsPressed;

  if (getThrottle() <= THROTTLE_CAL_LOW) {
    if (throttleInterlock && throttleZeroTimer > 500) {
      throttleInterlock = false;
    }
  } else {
    throttleZeroTimer = 0;
  }

  // --- Compute unified current command ---
  // Positive torque is only ever commanded in the dmsPressed branch —
  // every other path brakes or coasts.
  int32_t commandMa = 0;
  bool useBrake = false;

  if (getVehicleState() == STATE::MC_ACTIVE) {
    useBrake = true;
    if (hasBrakeCommand()) {
      // Braking: brake button pressed in any state
      commandMa = BRAKE_CURRENT_MA;
    } else if (dmsPressed) {
      int32_t throttle = throttleToCurrentMa();
      if (throttle > 0) {
        // Driving: DMS held, throttle applied
        commandMa = throttle;
        useBrake = false;
      } else {
        // Coasting: DMS held, throttle released
        commandMa = COAST_CURRENT_MA;
      }
    } else {
      // Safe stop: DMS released (driver fell off)
      commandMa = SAFE_STOP_CURRENT_MA;
    }

    // Undervoltage protection: zero current while battery is low
    if (!isBatteryVoltageOk()) {
      commandMa = 0;
      useBrake = false;
    }

    // Throttle interlock: zero current until throttle has been zero for 500ms after DMS engage
    if (throttleInterlock) {
      commandMa = 0;
      useBrake = false;
    }
  }
  // Outside MC_ACTIVE: commandMa stays 0 with useBrake false → plain zero current

  // --- One packet per tick ---
  switch (tick) {
    case 0:
#if MOTOR1_ENABLED
      if (useBrake) {
        vescSerial.setBrakeCurrent(commandMa);
      } else {
        vescSerial.setCurrent(commandMa);
      }
#endif
      break;
    case 1:
#if MOTOR2_ENABLED
      if (useBrake) {
        vescSerial.setBrakeCurrentCan(commandMa);
      } else {
        vescSerial.setCurrentCan(commandMa);
      }
#endif
      break;
    case 2:
      vescSerial.requestValuesSelective(VESC_TELEMETRY_MASK);
      break;
    case 3:
      vescSerial.requestValuesSelectiveCan(VESC_TELEMETRY_MASK);
      break;
  }
  tick = (tick + 1) % 4;

  // Update state for telemetry
  int32_t displayMa = useBrake ? -commandMa : commandMa;
  setState(VD_CURRENT_REQUEST_LEFT, (int)displayMa);
  setState(VD_CURRENT_REQUEST_RIGHT, (int)displayMa);
  setState(VD_VESC_RX_BYTES, vescSerial.rxByteCount);
  setState(VD_VESC_TX_BYTES, vescSerial.txByteCount);
  setState(VD_VESC_RX_PACKETS_OK, (int)vescSerial.rxPacketsOk);
  setState(VD_VESC_RX_CRC_FAILS, (int)vescSerial.rxCrcFails);
  setState(VD_VESC_RX_END_BYTE_FAILS, (int)vescSerial.rxEndByteFails);
  setState(VD_VESC_RX_GET_VALUES_OK, (int)vescSerial.rxGetValuesOk);
  setState(VD_VESC_RX_LAST_LEN, (int)vescSerial.rxLastLen);
  setState(VD_VESC_RX_LAST_CMD, vescSerial.rxLastCmd);
  setState(VD_VESC_RX_COMPUTED_CRC, (int)vescSerial.rxLastComputedCrc);
  setState(VD_VESC_RX_RECEIVED_CRC, (int)vescSerial.rxLastReceivedCrc);
}
