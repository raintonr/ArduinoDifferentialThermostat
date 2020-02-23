#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

#include "display.h"
#include "globals.h"

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

#define T_FONT_WIDTH 12
#define T_FONT_HEIGHT 2

SSD1306AsciiWire oled;

// Shared buffer for formatting
char buff[8];

void setupDisplay() {
  Wire.begin();
  Wire.setClock(400000L);

  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setScrollMode(SCROLL_MODE_OFF);
  oled.setFont(Adafruit5x7);
  oled.set2X();
}

//////////////////////////////////////////////////////////////////////
// Our display is 128x64 (wide/high).
// 0,0 is top left
// In text size 2 this gives 4 rows of 10 characters
// Text size 1 gives 8 rows of 21 characters

void printTemp(int x, int y, float temp, int dps) {
  // Get the sign to display later while making temp positive 
  const char *sign;
  if (temp < 0) {
    temp = -temp;
    sign = "-";
  } else {
    sign = " ";
  }

  // We only cater for 1 or 2 DPs
  float mult = dps > 1 ? 100 : 10;

  // Multiply up...
  temp *= mult;
  // ... and add 0.5 to properly round up when applicable
  temp += 0.5;

  // Convert to integer and pull out decimals
  int value = temp;
  int integer = int(value / mult);
  int decimals = value - (integer * mult);

  // If dps > 1 we will write those in a small font so...
  if (dps > 1) {
    oled.setCursor((x+2) * T_FONT_WIDTH, y * T_FONT_HEIGHT);
    oled.set1X();
    sprintf(buff, ".%02d", decimals);
    oled.print(buff);
  
    oled.setCursor(x * T_FONT_WIDTH, y * T_FONT_HEIGHT);
    oled.set2X();
    sprintf(buff, "%2d", integer);
    oled.print(buff);
  } else {
    oled.setCursor(x * T_FONT_WIDTH, y * T_FONT_HEIGHT);
    sprintf(buff, "%2d.%01d", integer, decimals);
    oled.print(buff);
  }

  oled.setCursor((x+4) * T_FONT_WIDTH, y * T_FONT_HEIGHT);
  oled.print(sign);
}

// Draw the setup screen:
//   1234567890
// 1  tOn:99.9-
// 2 tOff:99.9-
// 3 tMax:99.9-
// 4 

void drawSetupBack() {
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("tOn ");
  if (currentMode == MODE_SETON) {
    oled.print(">");
  } else {
    oled.print(":");
  }

  oled.setCursor(0, 1 * T_FONT_HEIGHT);
  oled.print("tOff");
  if (currentMode == MODE_SETOFF) {
    oled.print(">");
  } else {
    oled.print(":");
  }

  oled.setCursor(0, 2 * T_FONT_HEIGHT);
  oled.print("tMax");
  if (currentMode == MODE_SETMAX) {
    oled.print(">");
  } else {
    oled.print(":");
  }

  drawSetupVars(true);
}

void drawSetupVars(boolean drawAll) {
  if (drawAll || currentMode == MODE_SETON) {
    oled.setCursor(5 * T_FONT_WIDTH, 0);
    printTemp(5, 0, settings.dtOn, 1);
  }
  if (drawAll || currentMode == MODE_SETOFF) {
    oled.setCursor(5 * T_FONT_WIDTH, 1 * T_FONT_HEIGHT);
    printTemp(5, 1, settings.dtOff, 1);
  }
  if (drawAll || currentMode == MODE_SETMAX) {
    oled.setCursor(5 * T_FONT_WIDTH, 2 * T_FONT_HEIGHT);
    printTemp(5, 2, settings.tMax, 1);
  }
}

// Draw the sensors screen:
//   1234567890
// 1 tLow:
// 2 12345678
// 3 tHi:
// 4 12345678
//
// There isn't a separate back/vals for this

void printHex(DeviceAddress address) {
  for (int lp = 0; lp < sizeof(DeviceAddress); lp++) {
    sprintf(buff, "%02X", address[lp]);
    oled.print(buff);
  }
}

void drawSensors() {
  oled.clear();
  oled.setCursor(0,0);
  oled.print("tLow:");
  if (isValidTemp(tempLow)) {
    printTemp(5, 0, tempLow, 2);
  } else {
    oled.setCursor(5 * T_FONT_WIDTH, 0);
    oled.print("err!");
  }
  oled.setCursor(0, 2 * T_FONT_HEIGHT);
  oled.print("tHi:");
  if (isValidTemp(tempHigh)) {
    printTemp(4, 2, tempHigh, 2);
  } else {
    oled.setCursor(4 * T_FONT_WIDTH, 2 * T_FONT_HEIGHT);
    oled.print("err!");
  }

  oled.set1X();
  oled.setCursor(0, 2);
  if (isZeroAddress(settings.sensorLow)) {
    oled.print("Connect tLow");
  } else {
    printHex(settings.sensorLow);
  }
  
  oled.setCursor(0, 6);
    if (isZeroAddress(settings.sensorHigh)) {
    oled.print("Connect tHi");
  } else {
    printHex(settings.sensorHigh);
  }
  oled.set2X();
}

// Draw the running screen:
//   1234567890
// 1 tLow:99.9-
// 2  tHi:99.9-
// 3   dT:99.9-
// 4  Run: OFF

void drawRunBack() {
  oled.clear();
  oled.setCursor(0,0);
  oled.print("tLow:");

  oled.setCursor(1 * T_FONT_WIDTH, 1 * T_FONT_HEIGHT);
  oled.print("tHi:");

  oled.setCursor(2 * T_FONT_WIDTH, 2 * T_FONT_HEIGHT);
  oled.print("dT:");

  oled.setCursor(1 * T_FONT_WIDTH, 3 * T_FONT_HEIGHT);
  oled.print("Run:");

  drawRunVars();
}

boolean heartbeat = false;
void drawHeartbeat() {
  heartbeat = !heartbeat;

  oled.set1X();
  oled.setCursor(10 * T_FONT_WIDTH, 0);
  oled.print(heartbeat ? "/" : "\\");
  oled.set2X();
}

void drawRunVars() {
  oled.setCursor(5 * T_FONT_WIDTH, 0);
  printTemp(5, 0, tempLow, 2);
  printTemp(5, 1, tempHigh, 2);
  printTemp(5, 2, dT, 2);

  oled.setCursor(5 * T_FONT_WIDTH, 3 * T_FONT_HEIGHT);
  if (pumpRunning) {
    oled.print("ON ");
  } else {
    oled.print("off ");
  }
}

// Draw the reset screen:
//   1234567890
// 1 !!RESET!!
// 2   < NO >
// 3   < NO >
// 4  < YES >

void drawResetBack() {
  oled.clear();
  oled.setCursor(0.5 * T_FONT_WIDTH, 0);
  oled.print("!!RESET!!");
  oled.setCursor(4 * T_FONT_WIDTH, 1 * T_FONT_HEIGHT);
  oled.print("NO");
  oled.setCursor(4 * T_FONT_WIDTH, 2 * T_FONT_HEIGHT);
  oled.print("NO");
  oled.setCursor(3.5 * T_FONT_WIDTH, 3 * T_FONT_HEIGHT);
  oled.print("YES");
  drawResetVars();
}

void drawResetVars() {
  for (int lp = 0; lp < 3; lp++) {
    oled.setCursor(2.5 * T_FONT_WIDTH, (lp + 1) * T_FONT_HEIGHT);
    oled.print(resetOption == lp ? "<" : " ");
    oled.setCursor(6.5 * T_FONT_WIDTH, (lp + 1) * T_FONT_HEIGHT);
    oled.print(resetOption == lp ? ">" : " ");
  }
}

// Check to see if sensors are setup

boolean isZeroAddress(DeviceAddress address) {
  boolean allZero = true;
  for (int lp = 0; lp < sizeof(DeviceAddress); lp++) {
    if ((address[lp]) != 0) {
      allZero = false;
      break;
    }
  }
  return allZero;
}

boolean isValidTemp(float temp) {
  return (temp > -100 && temp < 200);
}

boolean allSensorsDefined() {
  return (!isZeroAddress(settings.sensorLow) && !isZeroAddress(settings.sensorHigh));
}
