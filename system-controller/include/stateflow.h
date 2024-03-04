#pragma once

#include "state.h"
#include <elapsedMillis.h>
#include "relay.h"
#include "steering.h"

enum STATE {
  IDLE,
  PRECHARGING,
  TS_ACTIVE,
  MC_ACTIVE,
  DISCHARGING
};

void stateflowSetup();
void stateflowLoop();
int getVehicleState();