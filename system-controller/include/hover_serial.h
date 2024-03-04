#pragma once

#include <Arduino.h>
#include <elapsedMillis.h>
#include "state.h"

class HoverSerial
{
  private:
    uint16_t powerLeft;
    uint16_t powerRight;
  public:
    HoverSerial();
    ~HoverSerial();
    void setup();
    void loop();
    void Receive();
    void setPower(uint16_t powerLeft, uint16_t powerRight);

    int16_t speedLeft;
    int16_t speedRight;
    int16_t batteryVoltage;
};