#include <Arduino.h>
#include <SoftwareSerial.h>
#include "hover_serial.h"
#include "relay.h"
#include "steering.h"
#include "telemetry.h"
#include "state.h"
#include "stateflow.h"
#include "vd.h"

#define SAMPLE_TIME 10 // ms

elapsedMillis sampleTimer = 0;

void setup() {
  Serial.begin(9600);
  
  Serial.println("Setup start");
  stateflowSetup();
  telemetrySetup();
  relaySetup();
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
  vdLoop();
  telemetryLoop();
}

