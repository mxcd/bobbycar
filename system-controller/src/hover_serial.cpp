#include "hover_serial.h"

#define HOVERSERIAL Serial2
// #define SERIAL_BAUD         115200      // [baud] Serial baud rate
#define HOVER_SERIAL_BAUD   115200      // [baud] HoverSerial baud rate

#define START_FRAME         0xABCD     	// [-] Start frme definition for reliable serial communication
#define TIME_SEND           100         // [ms] Sending time interval
#define SPEED_MAX_TEST      500         // [-] Maximum speed for testing

#define POTI_OFFSET         550         // [-] Offset for the potentiometer
#define POTI_DEAD_POINT     15          // [-] Dead point for the potentiometer
#define POTI_MAX_VALUE      475         // [-] Maximum value for the potentiometer
#define POTI_MIN_VALUE      550         // [-] Minimum value for the potentiometer
// #define DEBUG_RX                        // [-] Debug received data. Prints all bytes to serial (comment-out to disable)

HardwareSerial HOVERSERIAL(PD6, PD5); // RX, TX

// Global variables
uint8_t idx = 0;                        // Index for new data pointer
uint16_t bufStartFrame;                 // Buffer Start Frame
byte *p;                                // Pointer declaration for the new received data
byte incomingByte;
byte incomingBytePrev;

typedef struct{
   uint16_t start;
   int16_t  steer;
   int16_t  speed;
   uint16_t checksum;
} SerialCommand;
SerialCommand Command;

typedef struct{
   uint16_t start;
   int16_t  cmd1;
   int16_t  cmd2;
   int16_t  speedR_meas;
   int16_t  speedL_meas;
   int16_t  batVoltage;
   int16_t  boardTemp;
   uint16_t cmdLed;
   uint16_t checksum;
} SerialFeedback;
SerialFeedback Feedback;
SerialFeedback NewFeedback;

HoverSerial::HoverSerial()
{
  powerLeft = 0;
  powerRight = 0;
  speedLeft = 0;
  speedRight = 0;
  batteryVoltage = 0;
}

HoverSerial::~HoverSerial()
{
}

void HoverSerial::setPower(uint16_t powerLeft, uint16_t powerRight)
{
  this->powerLeft = powerLeft;
  this->powerRight = powerRight;
}

void HoverSerial::setup() {
  // Serial.begin(SERIAL_BAUD);
  Serial.println("Setting up HoverSerial...");
  HOVERSERIAL.begin(HOVER_SERIAL_BAUD);
}

// ########################## SEND ##########################
void Send(int16_t uSteer, int16_t uSpeed)
{
  // Serial.print("uSteer: ");
  // Serial.print(uSteer);
  // Serial.print(" uSpeed: ");
  // Serial.println(uSpeed);
  // Create command
  Command.start    = (uint16_t)START_FRAME;
  Command.steer    = (int16_t)uSteer;
  Command.speed    = (int16_t)uSpeed;
  Command.checksum = (uint16_t)(Command.start ^ Command.steer ^ Command.speed);

  // Write to Serial
  HOVERSERIAL.write((uint8_t *) &Command, sizeof(Command)); 
}

// ########################## RECEIVE ##########################
void HoverSerial::Receive()
{
    // Check for new data availability in the Serial buffer
    if (HOVERSERIAL.available()) {
        incomingByte 	  = HOVERSERIAL.read();                                   // Read the incoming byte
        bufStartFrame	= ((uint16_t)(incomingByte) << 8) | incomingBytePrev;       // Construct the start frame
    }
    else {
        return;
    }

  // If DEBUG_RX is defined print all incoming bytes
  #ifdef DEBUG_RX
        Serial.print(incomingByte);
        return;
  #endif

    // Copy received data
    if (bufStartFrame == START_FRAME) {	                    // Initialize if new data is detected
        p       = (byte *)&NewFeedback;
        *p++    = incomingBytePrev;
        *p++    = incomingByte;
        idx     = 2;	
    } else if (idx >= 2 && idx < sizeof(SerialFeedback)) {  // Save the new received data
        *p++    = incomingByte; 
        idx++;
    }	
    
    // Check if we reached the end of the package
    if (idx == sizeof(SerialFeedback)) {
        uint16_t checksum;
        checksum = (uint16_t)(NewFeedback.start ^ NewFeedback.cmd1 ^ NewFeedback.cmd2 ^ NewFeedback.speedR_meas ^ NewFeedback.speedL_meas
                            ^ NewFeedback.batVoltage ^ NewFeedback.boardTemp ^ NewFeedback.cmdLed);

        // Check validity of the new data
        if (NewFeedback.start == START_FRAME && checksum == NewFeedback.checksum) {
            // Copy the new data
            memcpy(&Feedback, &NewFeedback, sizeof(SerialFeedback));

            this->speedLeft = Feedback.speedL_meas;
            setState(VD_SPEED_LEFT, this->speedLeft);
            this->speedRight = Feedback.speedR_meas;
            setState(VD_SPEED_RIGHT, this->speedRight);
            this->batteryVoltage = Feedback.batVoltage;
            setState(VD_BATTERY_VOLTAGE, this->batteryVoltage);

            // Print data to built-in Serial
            Serial.print("1: ");   Serial.print(Feedback.cmd1);
            Serial.print(" 2: ");  Serial.print(Feedback.cmd2);
            Serial.print(" 3: ");  Serial.print(Feedback.speedR_meas);
            Serial.print(" 4: ");  Serial.print(Feedback.speedL_meas);
            Serial.print(" 5: ");  Serial.print(Feedback.batVoltage);
            Serial.print(" 6: ");  Serial.print(Feedback.boardTemp);
            Serial.print(" 7: ");  Serial.println(Feedback.cmdLed);
        } else {
          Serial.println("Non-valid data skipped");
        }
        idx = 0;    // Reset the index (it prevents to enter in this if condition in the next cycle)
    }

    // Update previous states
    incomingBytePrev = incomingByte;
}

// ########################## LOOP ##########################

void HoverSerial::loop() {
  // Receive();
  Send(this->powerLeft, this->powerRight);
}