#include <SPI.h>
#include <mcp2515.h>

#define BUTTON_PIN 7
#define LED_PIN 6
#define CAN_HS2_PIN 9

#define DEBOUNCE_DURATION 150

#define BRAKE_SYS_FEATURES_3_CAN_ID 1054
#define ABS_DATA_CAN_ID 1073

struct can_frame canIncomingMsg;
struct can_frame canOutgoingMsg;
struct can_frame absControlData = {
  ABS_DATA_CAN_ID,
  8,
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};
MCP2515 hs2Bus(CAN_HS2_PIN);

byte autoHoldIndicatorState = LOW;
byte lastButtonState = LOW;
unsigned long lastTimeButtonStateChanged = 0;


void setup() {
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  while (!Serial);
  Serial.begin(115200);

  SPI.begin();

  hs2Bus.reset();
  hs2Bus.setBitrate(CAN_500KBPS, MCP_8MHZ);
  hs2Bus.setConfigMode();
  hs2Bus.setFilterMask(MCP2515::MASK0, false, 0x7FF);
  hs2Bus.setFilterMask(MCP2515::MASK1, false, 0x7FF);
  hs2Bus.setFilter(MCP2515::RXF0, false, ABS_DATA_CAN_ID);
  hs2Bus.setFilter(MCP2515::RXF1, false, BRAKE_SYS_FEATURES_3_CAN_ID);
  hs2Bus.setFilter(MCP2515::RXF2, false, ABS_DATA_CAN_ID);
  hs2Bus.setFilter(MCP2515::RXF3, false, BRAKE_SYS_FEATURES_3_CAN_ID);
  hs2Bus.setFilter(MCP2515::RXF4, false, ABS_DATA_CAN_ID);
  hs2Bus.setFilter(MCP2515::RXF5, false, BRAKE_SYS_FEATURES_3_CAN_ID);
  hs2Bus.setNormalMode();
}

void loop() {
  readButtonPress();
  readHs2CanMessages();
  digitalWrite(LED_PIN, autoHoldIndicatorState);
}

void readButtonPress() {
  if (millis() - lastTimeButtonStateChanged <= DEBOUNCE_DURATION) {
    return;
  }

  byte buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == lastButtonState) {
    return;
  }

  lastButtonState = buttonState;
  lastTimeButtonStateChanged = millis();

  emitButtonState(buttonState);
}

void emitButtonState(byte state) {
  Serial.println(String(">>>>>> Emitting button state as ") + String(state == HIGH ? "ON" : "OFF"));
  
  if (state != HIGH) {
    return;
  }

  canOutgoingMsg.can_id = ABS_DATA_CAN_ID;
  canOutgoingMsg.can_dlc = 8;
  
  for (int i = 0; i < 8; i++) {
    canOutgoingMsg.data[i] = absControlData.data[i];
  }

  // So the way it's implemented by Ford is it sends an "AutoHold pressed" signal first,
  // then after a short time another "AutoHold NOT pressed" signal.
  // I've found the delay of 10ms to work reliably 99.99% of the time.
  bitWrite(canOutgoingMsg.data[7], 2, 1);
  hs2Bus.sendMessage(&canOutgoingMsg);

  delay(10);

  bitWrite(canOutgoingMsg.data[7], 2, 0);
  hs2Bus.sendMessage(&canOutgoingMsg);
}

void readHs2CanMessages() {
  if (hs2Bus.readMessage(&canIncomingMsg) != MCP2515::ERROR_OK) {
    return;
  }

  if (canIncomingMsg.can_id != ABS_DATA_CAN_ID && canIncomingMsg.can_id != BRAKE_SYS_FEATURES_3_CAN_ID) {
    return;
  }

  if (canIncomingMsg.can_id == ABS_DATA_CAN_ID) {
    for (int i = 0; i < 8; i++) {
      absControlData.data[i] = canIncomingMsg.data[i];
    }
  }

  if (canIncomingMsg.can_id == BRAKE_SYS_FEATURES_3_CAN_ID) {
    autoHoldIndicatorState = bitRead(canIncomingMsg.data[0], 0) ? HIGH : LOW;
    Serial.println(String(autoHoldIndicatorState));
  }

  // https://github.com/commaai/opendbc/blob/master/ford_lincoln_base_pt.dbc#L1816
  Serial.println(String("#") + String(canIncomingMsg.can_id) + String(":") + String(canIncomingMsg.data[0], HEX) + String(",") + String(canIncomingMsg.data[1], HEX) + String(",") + String(canIncomingMsg.data[2], HEX) + String(",") + String(canIncomingMsg.data[3], HEX) + String(",") + String(canIncomingMsg.data[4], HEX) + String(",") + String(canIncomingMsg.data[5], HEX) + String(",") + String(canIncomingMsg.data[6], HEX) + String(",") + String(canIncomingMsg.data[7], HEX));
}