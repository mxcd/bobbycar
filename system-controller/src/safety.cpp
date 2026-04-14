#include "safety.h"
#include "state.h"

#define CELL_COUNT 12 // Number of cells in the battery pack
#define UNDERVOLTAGE_THRESHOLD 45.0f // Absolute cutoff — zeroes current, opens main relay after 1s
#define LOWER_BATTERY_DERATING_VOLTAGE (3.65f * CELL_COUNT)
#define UPPER_BATTERY_DERATING_VOLTAGE (3.7f * CELL_COUNT)

float deratingFactor = 1.0;

bool isBatteryVoltageOk() {
  ArduinoJson::V703PB2::JsonDocument state = getStateData();
  float batteryVoltage = state[getKeyString(VD_BATTERY_VOLTAGE)].as<float>();
  // Allow startup before first telemetry reading
  if (batteryVoltage == 0.0f) {
    setState(BATTERY_OK, true);
    return true;
  }
  bool batteryOk = batteryVoltage > UNDERVOLTAGE_THRESHOLD;
  setState(BATTERY_OK, batteryOk);
  return batteryOk;
}

void safetySetup() {
  // Initialize safety checks
}

void safetyLoop() {
  ArduinoJson::V703PB2::JsonDocument state = getStateData();
  int batteryVoltage = state[getKeyString(VD_BATTERY_VOLTAGE)].as<int>();
  if (batteryVoltage > UPPER_BATTERY_DERATING_VOLTAGE) {
    deratingFactor = 1.0; // no derating
    setState(VD_DERATING_FACTOR, deratingFactor);
  } else {
    float factor = (float)(batteryVoltage - LOWER_BATTERY_DERATING_VOLTAGE) /
                    (UPPER_BATTERY_DERATING_VOLTAGE - LOWER_BATTERY_DERATING_VOLTAGE);
    if (factor < 0.0) {
      factor = 0.0; // no derating below lower limit
    } else if (factor > 1.0) {
      factor = 1.0; // no derating above upper limit
    }
    deratingFactor = factor;
    setState(VD_DERATING_FACTOR, deratingFactor);
  }
}

float getDeratingFactor() {
  return deratingFactor;
}