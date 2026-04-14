#include "steering.h"

#define STEERING_TX_PIN 6
#define STEERING_RX_PIN 5
SoftwareSerial steeringSerial(STEERING_RX_PIN, STEERING_TX_PIN);

#define STEERING_WATCHDOG_MESSAGE_INDEX_MAX_SKIP 10
#define STEERING_WATCHDOG_TIMEOUT 500
#define DMS_TOLERANCE_MS 500
#define START_BYTE 0xAA
#define STEERING_MSG_LEN 4

// CRC-8/AUTOSAR: poly=0x2F, init=0xFF, xorout=0xFF
static const uint8_t crc8_table[256] = {
  0x00, 0x2F, 0x5E, 0x71, 0xBC, 0x93, 0xE2, 0xCD,
  0x57, 0x78, 0x09, 0x26, 0xEB, 0xC4, 0xB5, 0x9A,
  0xAE, 0x81, 0xF0, 0xDF, 0x12, 0x3D, 0x4C, 0x63,
  0xF9, 0xD6, 0xA7, 0x88, 0x45, 0x6A, 0x1B, 0x34,
  0x73, 0x5C, 0x2D, 0x02, 0xCF, 0xE0, 0x91, 0xBE,
  0x24, 0x0B, 0x7A, 0x55, 0x98, 0xB7, 0xC6, 0xE9,
  0xDD, 0xF2, 0x83, 0xAC, 0x61, 0x4E, 0x3F, 0x10,
  0x8A, 0xA5, 0xD4, 0xFB, 0x36, 0x19, 0x68, 0x47,
  0xE6, 0xC9, 0xB8, 0x97, 0x5A, 0x75, 0x04, 0x2B,
  0xB1, 0x9E, 0xEF, 0xC0, 0x0D, 0x22, 0x53, 0x7C,
  0x48, 0x67, 0x16, 0x39, 0xF4, 0xDB, 0xAA, 0x85,
  0x1F, 0x30, 0x41, 0x6E, 0xA3, 0x8C, 0xFD, 0xD2,
  0x95, 0xBA, 0xCB, 0xE4, 0x29, 0x06, 0x77, 0x58,
  0xC2, 0xED, 0x9C, 0xB3, 0x7E, 0x51, 0x20, 0x0F,
  0x3B, 0x14, 0x65, 0x4A, 0x87, 0xA8, 0xD9, 0xF6,
  0x6C, 0x43, 0x32, 0x1D, 0xD0, 0xFF, 0x8E, 0xA1,
  0xE3, 0xCC, 0xBD, 0x92, 0x5F, 0x70, 0x01, 0x2E,
  0xB4, 0x9B, 0xEA, 0xC5, 0x08, 0x27, 0x56, 0x79,
  0x4D, 0x62, 0x13, 0x3C, 0xF1, 0xDE, 0xAF, 0x80,
  0x1A, 0x35, 0x44, 0x6B, 0xA6, 0x89, 0xF8, 0xD7,
  0x90, 0xBF, 0xCE, 0xE1, 0x2C, 0x03, 0x72, 0x5D,
  0xC7, 0xE8, 0x99, 0xB6, 0x7B, 0x54, 0x25, 0x0A,
  0x3E, 0x11, 0x60, 0x4F, 0x82, 0xAD, 0xDC, 0xF3,
  0x69, 0x46, 0x37, 0x18, 0xD5, 0xFA, 0x8B, 0xA4,
  0x05, 0x2A, 0x5B, 0x74, 0xB9, 0x96, 0xE7, 0xC8,
  0x52, 0x7D, 0x0C, 0x23, 0xEE, 0xC1, 0xB0, 0x9F,
  0xAB, 0x84, 0xF5, 0xDA, 0x17, 0x38, 0x49, 0x66,
  0xFC, 0xD3, 0xA2, 0x8D, 0x40, 0x6F, 0x1E, 0x31,
  0x76, 0x59, 0x28, 0x07, 0xCA, 0xE5, 0x94, 0xBB,
  0x21, 0x0E, 0x7F, 0x50, 0x9D, 0xB2, 0xC3, 0xEC,
  0xD8, 0xF7, 0x86, 0xA9, 0x64, 0x4B, 0x3A, 0x15,
  0x8F, 0xA0, 0xD1, 0xFE, 0x33, 0x1C, 0x6D, 0x42
};

uint8_t crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < len; i++) {
    crc = crc8_table[crc ^ data[i]];
  }
  return crc ^ 0xFF;
}

bool _steeringStartupCheckOk = false;
bool _hasDeadManSwitchPressed = false;
bool _hasAccelerationCommand = false;
bool _hasBrakeCommand = false;
uint8_t _throttle = 0;

bool hasSteeringPanic = false;

int lastSteeringWatchdogMessageIndex = -1;
elapsedMillis timeSinceLastSteeringWatchdogMessage = 0;
uint32_t lastTimeSinceLastSteeringWatchdogMessage = 0;
elapsedMillis timeSinceLastDmsPressed = DMS_TOLERANCE_MS;
enum SteeringRxState { WAIT_START, READ_DATA };
SteeringRxState _rxState = WAIT_START;
uint8_t _rxBuffer[STEERING_MSG_LEN];
uint8_t _rxIdx = 0;

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
    uint8_t b = steeringSerial.read();

    switch(_rxState) {
      case WAIT_START:
        if(b == START_BYTE) {
          _rxIdx = 0;
          _rxState = READ_DATA;
        }
        break;
      case READ_DATA:
        _rxBuffer[_rxIdx++] = b;
        if(_rxIdx >= STEERING_MSG_LEN) {
          _processSteeringMessage(_rxBuffer);
          _rxState = WAIT_START;
        }
        break;
    }
  }
}

void _processSteeringMessage(const uint8_t *data) {
  uint8_t expected_crc = crc8(data, 3);
  if(expected_crc != data[3]) {
    ++steeringChecksumErrors;
    return;
  }

  uint8_t index = data[2];

  // Recovery from panic: accept next valid CRC message as re-init
  if(hasSteeringPanic) {
    hasSteeringPanic = false;
    lastSteeringWatchdogMessageIndex = index;
    timeSinceLastSteeringWatchdogMessage = 0;
    return;
  }

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

  _steeringStartupCheckOk = (data[0] >> 0) & 0x01;
  _hasDeadManSwitchPressed = (data[0] >> 1) & 0x01;
  _hasAccelerationCommand = (data[0] >> 2) & 0x01;
  _hasBrakeCommand = (data[0] >> 3) & 0x01;
  _throttle = (uint8_t)data[1];

  if(_hasDeadManSwitchPressed) {
    timeSinceLastDmsPressed = 0;
  }
}

void _steeringPanic() {
  hasSteeringPanic = true;
  _hasDeadManSwitchPressed = false;
  _hasAccelerationCommand = false;
  _hasBrakeCommand = false;
  // Keep _throttle at last known value — zeroing causes jerky motor response
  // during brief EMI-induced panics. A steering panic should still invalidate
  // the DMS gate immediately instead of waiting for the release tolerance.
  timeSinceLastDmsPressed = DMS_TOLERANCE_MS;
}

void _updateSteeringState() {
  setState(StateData::STEERING_STARTUP_CHECK_OK, _steeringStartupCheckOk);
  setState(StateData::STEERING_DEAD_MAN_SWITCH_PRESSED, _hasDeadManSwitchPressed);
  setState(StateData::STEERING_ACCELERATION_COMMAND, _hasAccelerationCommand);
  setState(StateData::STEERING_BRAKE_COMMAND, _hasBrakeCommand);
  setState(StateData::STEERING_PANIC, hasSteeringPanic);
  setState(StateData::STEERING_WATCHDOG_INDEX, lastSteeringWatchdogMessageIndex);
  setState(StateData::STEERING_TIME_SINCE_LAST_WATCHDOG_MESSAGE, lastTimeSinceLastSteeringWatchdogMessage);
  setState(StateData::STEERING_THROTTLE, _throttle);
}

bool isSteeringSafetyOk() {
  return !hasSteeringPanic;
};

bool isSteeringStartupCheckOk() {
  return _steeringStartupCheckOk;
};

bool isSteeringPanic() {
  return hasSteeringPanic;
};

bool hasDeadManSwitch() {
  return timeSinceLastDmsPressed < DMS_TOLERANCE_MS;
};

bool hasAccelerationCommand() {
  return _hasAccelerationCommand;
};

bool hasBrakeCommand() {
  return _hasBrakeCommand;
};

uint8_t getThrottle() {
  return _throttle;
};
