#include "steering.h"

#define STEERING_TX_PIN 6
#define STEERING_RX_PIN 5
SoftwareSerial steeringSerial(STEERING_RX_PIN, STEERING_TX_PIN);

#define STEERING_WATCHDOG_MESSAGE_INDEX_MAX_SKIP 5
#define STEERING_WATCHDOG_TIMEOUT 200

bool _steeringStartupCheckOk = false;
bool _hasDeadManSwitchPressed = false;
bool _hasAccelerationCommand = false;
bool _hasBrakeCommand = false;

bool hasSteeringPanic = false;

int lastSteeringWatchdogMessageIndex = -1;
elapsedMillis timeSinceLastSteeringWatchdogMessage = 0;
uint32_t lastTimeSinceLastSteeringWatchdogMessage = 0;

String buffer = "";

uint32_t steeringChecksumErrors = 0;

void steeringSetup() {
  steeringSerial.begin(9600);
}

void steeringLoop() {
  _readSteeringSerial();

  if(timeSinceLastSteeringWatchdogMessage > STEERING_WATCHDOG_TIMEOUT && lastSteeringWatchdogMessageIndex != -1) {
    _steeringPanic();
  }

  _updateSteeringState();
}

void _readSteeringSerial() {
  while(steeringSerial.available()) {
    char c = steeringSerial.read();

    if(c != '\n' && c != '\r') {
      buffer += c;
      continue;
    }

    if(hasSteeringPanic) {
      buffer = "";
      continue;
    }

    if(buffer.length() == 0) {
      continue;
    }

    if(buffer.length() != 3) {
      buffer = "";
      continue;
    }
    
    _processSteeringMessage(buffer);
    buffer = "";
  }
}

void _processSteeringMessage(String message) {

  const char *data = message.c_str();
  uint8_t checksum = 0;
  checksum += data[0];
  checksum += data[1];

  if(checksum != data[2]) {
    Serial.println("Invalid steering checksum");
    ++steeringChecksumErrors;
    return;
  }

  uint8_t index = data[1];

  // INIT case - just accept the message
  if(lastSteeringWatchdogMessageIndex == -1) {
    lastSteeringWatchdogMessageIndex = index;
    timeSinceLastSteeringWatchdogMessage = 0;
    return;
  }

  if(((uint8_t)(index - lastSteeringWatchdogMessageIndex))%128 > STEERING_WATCHDOG_MESSAGE_INDEX_MAX_SKIP || index == lastSteeringWatchdogMessageIndex) {
    _steeringPanic();
    return;
  }
    
  lastSteeringWatchdogMessageIndex = index;
  lastTimeSinceLastSteeringWatchdogMessage = timeSinceLastSteeringWatchdogMessage;
  timeSinceLastSteeringWatchdogMessage = 0;

  Serial.println(data[0], HEX);

  _steeringStartupCheckOk = (data[0] >> 0) & 0x01;
  _hasDeadManSwitchPressed = (data[0] >> 1) & 0x01;
  _hasAccelerationCommand = (data[0] >> 2) & 0x01;
  _hasBrakeCommand = (data[0] >> 3) & 0x01;
}

void _steeringPanic() {
  hasSteeringPanic = true;
  _hasDeadManSwitchPressed = false;
  _hasAccelerationCommand = false;
  _hasBrakeCommand = false;
}

void _updateSteeringState() {
  setState(StateData::STEERING_STARTUP_CHECK_OK, _steeringStartupCheckOk);
  setState(StateData::STEERING_DEAD_MAN_SWITCH_PRESSED, _hasDeadManSwitchPressed);
  setState(StateData::STEERING_ACCELERATION_COMMAND, _hasAccelerationCommand);
  setState(StateData::STEERING_BRAKE_COMMAND, _hasBrakeCommand);
  setState(StateData::STEERING_PANIC, hasSteeringPanic);
  setState(StateData::STEERING_WATCHDOG_INDEX, lastSteeringWatchdogMessageIndex);
  setState(StateData::STEERING_TIME_SINCE_LAST_WATCHDOG_MESSAGE, lastTimeSinceLastSteeringWatchdogMessage);
}

bool isSteeringSafetyOk() {
  return _hasDeadManSwitchPressed && !hasSteeringPanic;
};

bool hasAccelerationCommand() {
  return _hasAccelerationCommand;
};

bool hasBrakeCommand() {
  return _hasBrakeCommand;
};