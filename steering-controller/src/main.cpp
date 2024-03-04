#include <Arduino.h>


#define ACC_BTN 2
#define BRK_BTN 3
#define DMS_BTN 6

bool accelerate = false;
bool brake = false;
bool deadManSwitch = false;
bool startupCheckOk = false;
uint8_t counter = 0;


bool isButtonPressed(int pin) {
  return digitalRead(pin) == LOW;
}

void checkButtons() {
  accelerate = isButtonPressed(ACC_BTN);
  brake = isButtonPressed(BRK_BTN);
  deadManSwitch = isButtonPressed(DMS_BTN);
}

void encodeMessage(char *data) {
  data[0] = 0;
  data[1] = 0;
  data[2] = 0;

  data[0] = startupCheckOk | deadManSwitch << 1 | accelerate << 2 | brake << 3;
  data[1] = counter%128;

  uint8_t checksum = 0;
  checksum += data[0];
  checksum += data[1];

  data[2] = checksum;
}

void sendMessage() {
  char data[3];
  encodeMessage(data);
  Serial.println(data);
}

void sendVerboseMessage() {
  Serial.print("Startup check: ");
  Serial.println(startupCheckOk);
  Serial.print("ACC: ");
  Serial.println(accelerate);
  Serial.print("BRK: ");
  Serial.println(brake);
  Serial.print("DMS: ");
  Serial.println(deadManSwitch);
}

void setup() {
  // init led pin
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  Serial.begin(9600);

  delay(500);
  startupCheckOk = false;
  while(!startupCheckOk) {
    checkButtons();

    if(!deadManSwitch) {
      startupCheckOk = true;
      continue;
    }

    sendMessage();
    delay(200);
  }
}

void loop() {
  ++counter;
  checkButtons();
  sendMessage();
  // sendVerboseMessage();
  delay(50);
}

