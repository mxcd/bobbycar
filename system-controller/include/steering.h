#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <elapsedMillis.h>
#include "state.h"



void steeringSetup();
void steeringLoop();


bool isSteeringSafetyOk();
bool hasAccelerationCommand();
bool hasBrakeCommand();

void _steeringPanic();
void _readSteeringSerial();
void _processSteeringMessage(String message);
void _updateSteeringState();