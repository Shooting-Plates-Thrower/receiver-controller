#include <Arduino.h>
#include <TM1637Display.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <EEPROM.h>
#include <GyverButton.h>

#define BTN 3
#define CLK 4
#define DIO 5
#define CE  9
#define CSN 10

const unsigned int MENU_TIMEOUT = 15000;
const unsigned int PRINT_DELAY = 5000;
const unsigned int BTN_MENU_STEP_DELAY = 2000;
const unsigned int BTN_HOLD_DELAY = 5000;
const unsigned int MENU_BLINK_DELAY = 500;

const byte DISP_BRIGHT = 7;

byte zoneNum = 0; // Field number
byte lchrNum = 0; // Launcher number
byte receiverAddr[6] = "00000";

const uint8_t SEG_DONE[] = {
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	SEG_C | SEG_E | SEG_G,                           // n
	SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};

RF24 radio(CE, CSN);
TM1637Display display(CLK, DIO);
GButton btn(BTN);

void getSettingOnStartUp();
void saveSettings(byte zoneVal, byte lchrVal);
void printSettings();
void changeSettings();

void setup() {
  radio.begin();
  radio.openReadingPipe(0, receiverAddr);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.startListening();
  display.setBrightness(DISP_BRIGHT);
  btn.setTimeout(BTN_HOLD_DELAY);
  btn.setTickMode(AUTO);
  getSettingOnStartUp();
  printSettings();
}

void loop() {
  if (btn.isClick())
    printSettings();

  if (btn.isHold()) {
    btn.setTimeout(BTN_MENU_STEP_DELAY);
    changeSettings();
    btn.setTimeout(BTN_HOLD_DELAY);
  }

  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));

    uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
    data[0] = display.encodeDigit(1);
    data[1] = display.encodeDigit(2);
    data[2] = display.encodeDigit(3);
    data[3] = display.encodeDigit(4);
    display.setSegments(data);
    display.clear();
  }
}

void getSettingOnStartUp() {
  int address = 0;
  byte value = 0;
  EEPROM.get(address, value);
  if (value > 0) {
    zoneNum = value;
    receiverAddr[0] = byte(zoneNum / 10);
    receiverAddr[1] = zoneNum % 10;
  }
  receiverAddr[2] = 0;
  address += sizeof(zoneNum);
  EEPROM.get(address, value);
  if (value > 0) {
    lchrNum = value;
    receiverAddr[3] = byte(lchrNum / 10);
    receiverAddr[4] = lchrNum % 10;
  }
  receiverAddr[5] = '\0';
}

void saveSettings(byte zoneVal, byte lchrVal) {
  int address = 0;
  zoneNum = zoneVal;
  lchrNum = lchrVal;

  EEPROM.write(address, zoneNum);
  address += sizeof(zoneNum);
  EEPROM.write(address, lchrNum);

  receiverAddr[0] = byte(zoneNum / 10);
  receiverAddr[1] = zoneNum % 10;
  receiverAddr[2] = 0;
  receiverAddr[3] = byte(lchrNum / 10);
  receiverAddr[4] = lchrNum % 10;
  receiverAddr[5] = '\0';
}

void printSettings() {
  display.showNumberDecEx(zoneNum * 100 + lchrNum, 0b01000000, true);
  delay(PRINT_DELAY);
  display.clear();
}

void changeSettings() {
  unsigned long lastActionTime = millis();
  byte tempZoneNum = zoneNum > 0 ? zoneNum : 1;
  byte tempLchrNum = lchrNum > 0 ? lchrNum : 1;
  boolean isSecondStep = false;
  btn.resetStates();

  while (millis() - lastActionTime < MENU_TIMEOUT) {
    if (btn.isClick()) {
      if (isSecondStep) {
        if (tempLchrNum < 100)
          tempLchrNum++;
        else
          tempLchrNum = 1;
      } else {
        if (tempZoneNum < 100)
          tempZoneNum++;
        else
          tempZoneNum = 1;
      }
      lastActionTime = millis();
    }
    if (btn.isHold()) {
      if (isSecondStep) {
        saveSettings(tempZoneNum, tempLchrNum);
        display.setSegments(SEG_DONE);
        delay(PRINT_DELAY);
        display.clear();
        break;
      }
      isSecondStep = true;
      lastActionTime = millis();
      btn.resetStates();
    }

    uint8_t data[] = {0x00, 0x00, 0x00, 0x00};
    if (millis() % (MENU_BLINK_DELAY * 2) < MENU_BLINK_DELAY) {
      display.showNumberDecEx(tempZoneNum * 100 + tempLchrNum, 0b01000000, true);
    } else if (isSecondStep) {
      data[0] = display.encodeDigit(tempZoneNum / 10);
      data[1] = display.encodeDigit(tempZoneNum % 10) | SEG_DP;
      display.setSegments(data);
    } else {
      data[1] = SEG_DP;
      data[2] = display.encodeDigit(tempLchrNum / 10);
      data[3] = display.encodeDigit(tempLchrNum % 10);
      display.setSegments(data);
    }
  }
  display.clear();
}
