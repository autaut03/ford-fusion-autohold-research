#include <SPI.h>
#include <mcp2515.h>

#define BUTTON_PIN 7
#define LED_PIN 6
#define CAN_HS2_PIN 9
#define CAN_MS_PIN 10

#define DEBOUNCE_DURATION 150

#define BRAKE_SYS_FEATURES_3_CAN_ID 1054
#define CLIMATE_CONTROL_DATA_FD1_CAN_ID 1050
#define ABS_DATA_CAN_ID 1073

struct can_frame canIncomingMsg;
struct can_frame canOutgoingMsg;
struct can_frame climateControlData = {
  CLIMATE_CONTROL_DATA_FD1_CAN_ID,
  8,
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};
struct can_frame absControlData = {
  ABS_DATA_CAN_ID,
  8,
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};
MCP2515 hs2Bus(CAN_HS2_PIN);
MCP2515 msBus(CAN_MS_PIN);

byte autoHoldIndicatorState = LOW;
byte lastButtonState = LOW;
unsigned long lastTimeButtonStateChanged = 0;


void setup() {
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  while (!Serial)
    ;
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

  msBus.reset();
  msBus.setBitrate(CAN_125KBPS, MCP_8MHZ);
  msBus.setNormalMode();
  // can2.setFilter(MCP2515::RXF0, false, BRAKE_SYS_FEATURES_3_CAN_ID);
}

void loop() {
  readButtonPress();
  readMsCanMessages();
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
  Serial.println("");
  Serial.println(String(">>>>>> Emitting button state as ") + String(state == HIGH ? "ON" : "OFF"));
  Serial.println("");

  // canOutgoingMsg.can_id = CLIMATE_CONTROL_DATA_FD1_CAN_ID;
  // canOutgoingMsg.can_dlc = 8;
  // canOutgoingMsg.data[0] = 0x00;
  // canOutgoingMsg.data[1] = 0x00;
  // canOutgoingMsg.data[2] = state == HIGH ? 0x08 : 0x00;
  // canOutgoingMsg.data[3] = 0x00;
  // canOutgoingMsg.data[4] = 0x00;
  // canOutgoingMsg.data[5] = 0x00;
  // canOutgoingMsg.data[6] = 0x00;
  // canOutgoingMsg.data[7] = 0x00;

  // msBus.sendMessage(&canOutgoingMsg);

  // canOutgoingMsg.can_id = CLIMATE_CONTROL_DATA_FD1_CAN_ID;
  // canOutgoingMsg.can_dlc = 8;

  // for (int i = 0; i < 8; i++) {
  //   canOutgoingMsg.data[i] = climateControlData.data[i];
  // }

  // bitWrite(canOutgoingMsg.data[2], 3, state == HIGH ? 1 : 0);

  // msBus.sendMessage(&canOutgoingMsg);

  if (state != HIGH) {
    return;
  }

  canOutgoingMsg.can_id = ABS_DATA_CAN_ID;
  canOutgoingMsg.can_dlc = 8;
  
  for (int i = 0; i < 8; i++) {
    canOutgoingMsg.data[i] = absControlData.data[i];
  }

  bitWrite(canOutgoingMsg.data[7], 2, 1);

  hs2Bus.sendMessage(&canOutgoingMsg);

  // bitWrite(canOutgoingMsg.data[0], 0, 0);
  // bitWrite(canOutgoingMsg.data[1], 3, 0);
  // bitWrite(canOutgoingMsg.data[4], 0, 0);
  // bitWrite(canOutgoingMsg.data[5], 7, 0);
  bitWrite(canOutgoingMsg.data[7], 2, 0);

  delay(10);

  hs2Bus.sendMessage(&canOutgoingMsg);
}

void readMsCanMessages() {
  if (msBus.readMessage(&canIncomingMsg) != MCP2515::ERROR_OK) {
    return;
  }

  if (canIncomingMsg.can_id != CLIMATE_CONTROL_DATA_FD1_CAN_ID) {
    return;
  }

  for (int i = 0; i < 8; i++) {
    climateControlData.data[i] = canIncomingMsg.data[i];
  }
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