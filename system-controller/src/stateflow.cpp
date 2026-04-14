#include "stateflow.h"
#include "safety.h"


elapsedMillis stateTimer = 0;
elapsedMillis undervoltageTimer = 0;

STATE vehicleState = IDLE;

bool safetyCheck() {
  return isSteeringSafetyOk() && isBatteryVoltageOk();
}

void enterIdleState() {
  stateTimer = 0;
  vehicleState = IDLE;
  setState(VEHICLE_STATE, vehicleState);
  disablePrechargeRelay();
  disableScOk();
}

void enterPrechargeState() {
  stateTimer = 0;
  vehicleState = PRECHARGING;
  setState(VEHICLE_STATE, vehicleState);
  enablePrechargeRelay();
}

void enterTsActiveState() {
  stateTimer = 0;
  vehicleState = TS_ACTIVE;
  setState(VEHICLE_STATE, vehicleState);
  enableScOk();
}

void enterMcActiveState() {
  stateTimer = 0;
  undervoltageTimer = 0;
  vehicleState = MC_ACTIVE;
  setState(VEHICLE_STATE, vehicleState);
}

void stateflowSetup() {
  enterIdleState();
}

void stateflowLoop() {
  switch(vehicleState) {
    case IDLE:
      if(isSteeringStartupCheckOk() && !isSteeringPanic() && isBatteryVoltageOk()) {
        enterPrechargeState();
      }
      break;
    case PRECHARGING:
      if(!safetyCheck()) {
        enterIdleState();
        break;
      }
      if(stateTimer > 2000) {
        enterTsActiveState();
      }
      break;
    case TS_ACTIVE:
      if(!safetyCheck()) {
        enterIdleState();
        break;
      }
      if (stateTimer > 500) {
        enterMcActiveState();
      }
      break;
    case MC_ACTIVE:
      // Stay in MC_ACTIVE regardless of steering panic — motor commands
      // are gated by throttle value which holds last known good value.
      // Undervoltage: vd.cpp zeroes current immediately via isBatteryVoltageOk(),
      // then we open the relay after 1 second of continuous undervoltage.
      if(!isBatteryVoltageOk()) {
        if (undervoltageTimer > 1000) {
          enterIdleState();
        }
      } else {
        undervoltageTimer = 0;
      }
      break;
  }
}

int getVehicleState() {
  return vehicleState;
}