#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

#include "display.h"
#include "globals.h"

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

#define T_FONT_WIDTH 12
#define T_FONT_HEIGHT 2

SSD1306AsciiWire oled;

void setupDisplay() {
  Serial.println("setupDisplay");

  Wire.begin();
  Wire.setClock(400000L);

  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setScrollMode(SCROLL_MODE_OFF);
  oled.setFont(Adafruit5x7);
  oled.set2X();

  drawRunBack();
  Serial.println("setupDisplay done");
}

//////////////////////////////////////////////////////////////////////
// Our display is 128x64 (wide/high).
// 0,0 is top left
// In text size 2 this gives 4 rows of 10 characters
// Text size 1 gives 8 rows of 21 characters

void printTemp(int x, int y, float temp, int dps) {
  Serial.print("printTemp: ");
  Serial.println(temp);

  char buff[24];

  // Get the sign to display later
  const char *sign = (temp < 0 ? "-" : " ");

  // Now do everything in the positive domain
  if (temp < 0) {
    temp = -temp;
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
// 1  tOn:-99.9
// 2 tOff:-99.9
// 3 
// 4 

void drawSetupBack() {
  Serial.println("drawSetupBack");

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

  drawSetupVars();
}

void drawSetupVars() {
  Serial.println("drawSetupVars");

  oled.setCursor(5 * T_FONT_WIDTH, 0);
  printTemp(5, 0, settings.dtOn, 1);
  oled.setCursor(5 * T_FONT_WIDTH, 1 * T_FONT_HEIGHT);
  printTemp(5, 1, settings.dtOff, 1);
}

// Draw the running screen:
//   1234567890
// 1 tLow:-99.9
// 2  tHi:-99.9
// 3   dT:
// 4  Run: OFF

void drawRunBack() {
  Serial.println("drawRunBack");

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
  Serial.println("drawRunVars");

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
  Serial.println("drawResetBack");

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
  Serial.println("drawResetVars");

  for (int lp = 0; lp < 3; lp++) {
    oled.setCursor(2.5 * T_FONT_WIDTH, (lp + 1) * T_FONT_HEIGHT);
    oled.print(resetOption == lp ? "<" : " ");
    oled.setCursor(6.5 * T_FONT_WIDTH, (lp + 1) * T_FONT_HEIGHT);
    oled.print(resetOption == lp ? ">" : " ");
  }
}
