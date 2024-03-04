#include "stateflow.h"

elapsedMillis stateTimer = 0;

enum STATE {
  IDLE,
  PRECHARGING,
  TS_ACTIVE,
  MC_ACTIVE,
  DISCHARGING
};

STATE vehicleState = IDLE;

bool safetyCheck() {
  if(!isSteeringSafetyOk()) {
    return false;
  }
  return true;
}

void enterIdleState() {
  stateTimer = 0;
  vehicleState = IDLE;
  setState(VEHICLE_STATE, vehicleState);
  disablePrechargeRelay();
  disableMotorControllerButton();
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
  vehicleState = MC_ACTIVE;
  setState(VEHICLE_STATE, vehicleState);
  disableMotorControllerButton();
}

void enterDischargeState() {
  stateTimer = 0;
  vehicleState = DISCHARGING;
  setState(VEHICLE_STATE, vehicleState);
  disablePrechargeRelay();
  disableScOk();
}

void stateflowSetup() {
  enterIdleState();
}

void stateflowLoop() {
  switch(vehicleState) {
    case IDLE:
      if(safetyCheck()) {
        enterPrechargeState();
      }
      break;
    case PRECHARGING:
      if(!safetyCheck()) {
        enterDischargeState();
        break;
      } 
      if(stateTimer > 2000) {
        enterTsActiveState();
      }
      break;
    case TS_ACTIVE:
      if(!safetyCheck()) {
        enterDischargeState();
        break;
      }
      if (stateTimer > 500) {
        enterMcActiveState();
      } else if(stateTimer > 250) {
        enableMotorControllerButton();
      }
      break;
    case MC_ACTIVE:
      if(!safetyCheck()) {
        enterDischargeState();
        break;
      }
      break;
    case DISCHARGING:
      if(stateTimer > 5000) {
        enterIdleState();
      }
      break;
  }
}