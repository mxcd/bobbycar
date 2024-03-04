#include "vd.h"

HoverSerial hoverSerial;

float targetSpeedLeft = 0.0f;
float targetSpeedRight = 0.0f;

float accellerateRamp = 5.0f;
float brakeRamp = 5.0f;
float idleRamp = 2.0f;

#define SPEED_LOWER_LIMIT 0.0f
#define SPEED_UPPER_LIMIT 1000.0f

#define SPEED_DIFF_THREASHOLD 200.0f

float limitSpeed(float speed) {
  if(getVehicleState() != STATE::MC_ACTIVE) {
    return 0.0f;
  }
  if(speed < SPEED_LOWER_LIMIT) {
    return SPEED_LOWER_LIMIT;
  } else if(speed > SPEED_UPPER_LIMIT) {
    return SPEED_UPPER_LIMIT;
  }
  return speed;
}

void vdSetup() {
  hoverSerial.setup();
}

void vdLoop() {
  hoverSerial.Receive();

  bool accelerationCommand = hasAccelerationCommand();
  bool brakeCommand = hasBrakeCommand();

  /*
  if(targetSpeedLeft > hoverSerial.speedLeft + SPEED_DIFF_THREASHOLD) {
    targetSpeedLeft = hoverSerial.speedLeft + SPEED_DIFF_THREASHOLD;
  } else if(targetSpeedLeft < hoverSerial.speedLeft - SPEED_DIFF_THREASHOLD) {
    targetSpeedLeft = hoverSerial.speedLeft - SPEED_DIFF_THREASHOLD;
  }
  
  if(targetSpeedRight > hoverSerial.speedRight + SPEED_DIFF_THREASHOLD) {
    targetSpeedRight = hoverSerial.speedRight + SPEED_DIFF_THREASHOLD;
  } else if(targetSpeedRight < hoverSerial.speedRight - SPEED_DIFF_THREASHOLD) {
    targetSpeedRight = hoverSerial.speedRight - SPEED_DIFF_THREASHOLD;
  }
  */

  if(accelerationCommand) {
    targetSpeedLeft += accellerateRamp;
    targetSpeedRight += accellerateRamp;
  } else if(brakeCommand) {
    targetSpeedLeft -= brakeRamp;
    targetSpeedRight -= brakeRamp;
  } else {
    targetSpeedLeft -= idleRamp;
    targetSpeedRight -= idleRamp;
  }

  targetSpeedLeft = limitSpeed(targetSpeedLeft);
  targetSpeedRight = limitSpeed(targetSpeedRight);
  setState(VD_SPEED_REQUEST_LEFT, int(targetSpeedLeft));
  setState(VD_SPEED_REQUEST_RIGHT, int(targetSpeedRight));

  hoverSerial.setPower(int(targetSpeedLeft), int(targetSpeedRight));
  hoverSerial.loop();
}