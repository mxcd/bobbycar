#pragma once

#include <Arduino.h>
#include "state.h"

void safetySetup();
void safetyLoop();

bool isBatteryVoltageOk();
float getDeratingFactor();