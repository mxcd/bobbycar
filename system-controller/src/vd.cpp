#include "vd.h"
#include <elapsedMillis.h>

// === TEST CONFIG ===
#define MOTOR1_ENABLED 1
#define MOTOR2_ENABLED 1

#define SWEEP_AMPLITUDE_MA 10000   // 10A peak
#define SWEEP_HALF_PERIOD  400     // ticks per half-cycle (4s at 10ms/tick)

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

// Telemetry request interval in loop ticks (10ms each → 200ms)
#define TELEM_INTERVAL 20

VescSerial vescSerial;

static uint8_t tick = 0;
static uint16_t sweepTick = 0;
static bool sweepActive = false;

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

// Triangle wave: ramps linearly between -AMPLITUDE and +AMPLITUDE.
static int32_t sweepCurrentMa(uint16_t t) {
  uint16_t fullPeriod = 2 * SWEEP_HALF_PERIOD;
  uint16_t phase = t % fullPeriod;
  if (phase < SWEEP_HALF_PERIOD) {
    return -SWEEP_AMPLITUDE_MA + (int32_t)(2 * SWEEP_AMPLITUDE_MA) * phase / SWEEP_HALF_PERIOD;
  } else {
    return SWEEP_AMPLITUDE_MA - (int32_t)(2 * SWEEP_AMPLITUDE_MA) * (phase - SWEEP_HALF_PERIOD) / SWEEP_HALF_PERIOD;
  }
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

  // Telemetry requests run in all states — VESC is always powered
  bool telemTick = false;
  if (tick == TELEM_INTERVAL - 2) {
    vescSerial.requestValues();
    telemTick = true;
  } else if (tick == TELEM_INTERVAL - 1) {
    vescSerial.requestValuesCan();
    telemTick = true;
  }

  // --- Throttle interlock: track DMS transitions and throttle zero duration ---
  bool dmsNow = hasDeadManSwitch();
  if (dmsNow && !dmsWasPressed) {
    throttleInterlock = true;
    throttleZeroTimer = 0;
  }
  dmsWasPressed = dmsNow;

  if (getThrottle() <= THROTTLE_CAL_LOW) {
    if (throttleInterlock && throttleZeroTimer > 500) {
      throttleInterlock = false;
    }
  } else {
    throttleZeroTimer = 0;
  }

  // Outside MC_ACTIVE: zero enabled motors (skip on telem ticks for UART spacing)
  if (getVehicleState() != STATE::MC_ACTIVE) {
    if (!telemTick) {
#if MOTOR1_ENABLED
      vescSerial.setCurrent(0);
#endif
#if MOTOR2_ENABLED
      vescSerial.setCurrentCan(0);
#endif
    }
    setState(VD_CURRENT_REQUEST_LEFT, int(0));
    setState(VD_CURRENT_REQUEST_RIGHT, int(0));
    sweepTick = 0;
    sweepActive = false;
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
    tick++;
    if (tick >= TELEM_INTERVAL) tick = 0;
    return;
  }

  // --- Compute unified current command ---
  int32_t commandMa = 0;
  bool useBrake = true;

  bool accPressed = hasAccelerationCommand();
  bool dmsPressed = hasDeadManSwitch();
  bool brakePressed = hasBrakeCommand();

  if (accPressed) {
    // Sweep mode: triangle wave while ACC held
    if (!sweepActive) {
      sweepTick = SWEEP_HALF_PERIOD / 2; // start from 0
      sweepActive = true;
    }
    commandMa = sweepCurrentMa(sweepTick);
    useBrake = false;
    sweepTick++;
    if (sweepTick >= 2 * SWEEP_HALF_PERIOD) sweepTick = 0;
  } else {
    sweepTick = 0;
    sweepActive = false;

    if (brakePressed) {
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

  // --- Round-robin motor commands (telemetry requests handled above) ---
  uint8_t phase = tick % 5;
#if MOTOR1_ENABLED
  if (!telemTick && phase == 0) {
    if (useBrake) {
      vescSerial.setBrakeCurrent(commandMa);
    } else {
      vescSerial.setCurrent(commandMa);
    }
  }
#endif
#if MOTOR2_ENABLED
  if (!telemTick && phase == 1) {
    if (useBrake) {
      vescSerial.setBrakeCurrentCan(commandMa);
    } else {
      vescSerial.setCurrentCan(commandMa);
    }
  }
#endif

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

  tick++;
  if (tick >= TELEM_INTERVAL) tick = 0;
}
