
#include "relay.h"

#define MC_EN_RELAY_PIN D43
#define PRECHARGE_RELAY_PIN D44
#define SC_OK_RELAY_PIN D45

bool isMotorControllerEnabled = false;
bool isPrechargeEnabled = false;
bool isScOkEnabled = false;

void relaySetup() {
  pinMode(MC_EN_RELAY_PIN, OUTPUT);
  digitalWrite(MC_EN_RELAY_PIN, HIGH);
  pinMode(PRECHARGE_RELAY_PIN, OUTPUT);
  digitalWrite(PRECHARGE_RELAY_PIN, HIGH);
  pinMode(SC_OK_RELAY_PIN, OUTPUT);
  digitalWrite(SC_OK_RELAY_PIN, HIGH);
}

void relayLoop() {
// Nothing to do here
}


void enablePrechargeRelay() {
  digitalWrite(PRECHARGE_RELAY_PIN, LOW);
  isPrechargeEnabled = true;
  setState(StateData::RELAY_PRECHARGE, isPrechargeEnabled);
}

void disablePrechargeRelay() {
  digitalWrite(PRECHARGE_RELAY_PIN, HIGH);
  isPrechargeEnabled = false;
  setState(StateData::RELAY_PRECHARGE, isPrechargeEnabled);
}

void enableMotorControllerButton() {
  digitalWrite(MC_EN_RELAY_PIN, LOW);
  isMotorControllerEnabled = true;
  setState(StateData::RELAY_MC_EN, isMotorControllerEnabled);
}

void disableMotorControllerButton() {
  digitalWrite(MC_EN_RELAY_PIN, HIGH);
  isMotorControllerEnabled = false;
  setState(StateData::RELAY_MC_EN, isMotorControllerEnabled);
}

void enableScOk() {
  digitalWrite(SC_OK_RELAY_PIN, LOW);
  isScOkEnabled = true;
  setState(StateData::RELAY_SC_OK, isScOkEnabled);
}

void disableScOk() {
  digitalWrite(SC_OK_RELAY_PIN, HIGH);
  isScOkEnabled = false;
  setState(StateData::RELAY_SC_OK, isScOkEnabled);
}