#pragma once

#include <Arduino.h>
#include <LwIP.h>
#include <STM32Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#include <elapsedMillis.h>
#include <state.h>

void telemetrySetup();
void telemetryLoop();

void _sendTelemetryMessage();