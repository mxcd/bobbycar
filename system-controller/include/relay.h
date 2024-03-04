#pragma once

#include <Arduino.h>
#include "state.h"

void relaySetup();
void relayLoop();

void enablePrechargeRelay();
void disablePrechargeRelay();

void enableMotorControllerButton();
void disableMotorControllerButton();

void enableScOk();
void disableScOk();

void _updateRelayState();