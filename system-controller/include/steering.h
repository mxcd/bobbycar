#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <elapsedMillis.h>
#include "state.h"



void steeringSetup();
void steeringLoop();


bool isSteeringSafetyOk();
bool isSteeringStartupCheckOk();
bool isSteeringPanic();
bool hasDeadManSwitch();
bool hasAccelerationCommand();
bool hasBrakeCommand();
uint8_t getThrottle();

void _steeringPanic();
void _readSteeringSerial();
void _processSteeringMessage(const uint8_t *data);
void _updateSteeringState();