#include <Arduino.h>
#include <SoftwareSerial.h>
#include "vesc_serial.h"
#include "relay.h"
#include "steering.h"
#include "telemetry.h"
#include "safety.h"
#include "state.h"
#include "stateflow.h"
#include "vd.h"

#define SAMPLE_TIME 10 // ms

// Motor 1: UART | VESC ID 23
// Motor 2: CAN  | VESC ID 24

elapsedMillis sampleTimer = 0;

void setup() {
  Serial.begin(9600);
  
  Serial.println("Setup start");
  stateflowSetup();
  telemetrySetup();
  relaySetup();
  safetySetup();
  steeringSetup();
  vdSetup();
}



uint32_t loopCounter = 0;
void loop() {
  if(sampleTimer < SAMPLE_TIME) return;
  sampleTimer -= SAMPLE_TIME;
  
  ++loopCounter;
  setState(STATE_LOOP_COUNTER, loopCounter);

  stateflowLoop();
  steeringLoop();
  relayLoop();
  safetyLoop();
  vdLoop();
  telemetryLoop();
}

