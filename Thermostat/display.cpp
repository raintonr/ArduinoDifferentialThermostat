#include <Adafruit_SSD1306.h>

#include "display.h"
#include "globals.h"

// OLED display TWI address
#define OLED_ADDR 0x3C
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
Adafruit_SSD1306 display(-1);

#define FONT_SIZE 2
#define FONT_SIZE_SUP 1
#define FONT_WIDTH 12
#define FONT_HEIGHT 16

void setupDisplay() {
  // initialize and clear display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  // We always use this size & colours
  // (and if not, we always put it back)
  display.setTextSize(FONT_SIZE);
  display.setTextColor(WHITE, BLACK);
  drawRunBack();
}

//////////////////////////////////////////////////////////////////////
// Our display is 128x64 (wide/high).
// 0,0 is top left
// In text size 2 this gives 4 rows of 10 characters
// Text size 1 gives 8 rows of 21 characters

void printTemp(int x, int y, float temp, int dps) {
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
    display.setCursor((x+2) * FONT_WIDTH, y * FONT_HEIGHT);
    display.setTextSize(FONT_SIZE_SUP);
    sprintf(buff, ".%02d", decimals);
    display.print(buff);
  
    display.setCursor(x * FONT_WIDTH, y * FONT_HEIGHT);
    display.setTextSize(FONT_SIZE);
    sprintf(buff, "%2d", integer);
    display.print(buff);
  } else {
    display.setCursor(x * FONT_WIDTH, y * FONT_HEIGHT);
    sprintf(buff, "%2d.%01d", integer, decimals);
    display.print(buff);
  }

  display.setCursor((x+4) * FONT_WIDTH, y * FONT_HEIGHT);
  display.print(sign);
}

// Draw the setup screen:
//   1234567890
// 1  tOn:-99.9
// 2 tOff:-99.9
// 3 
// 4 

void drawSetupBack() {
  Serial.println("drawSetupBack");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("tOn ");
  if (currentMode == MODE_SETON) {
    display.print(">");
  } else {
    display.print(":");
  }

  display.setCursor(0, 1 * FONT_HEIGHT);
  display.print("tOff");
  if (currentMode == MODE_SETOFF) {
    display.print(">");
  } else {
    display.print(":");
  }

  display.display();
  drawSetupVars();
}

void drawSetupVars() {
  Serial.println("drawSetupVars");

  display.setCursor(5 * FONT_WIDTH, 0);
  printTemp(5, 0, settings.dtOn, 1);
  display.setCursor(5 * FONT_WIDTH, 1 * FONT_HEIGHT);
  printTemp(5, 1, settings.dtOff, 1);
  display.display();
}

// Draw the running screen:
//   1234567890
// 1 tLow:-99.9
// 2  tHi:-99.9
// 3   dT:
// 4  Run: OFF

void drawRunBack() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("tLow:");

  display.setCursor(1 * FONT_WIDTH, 1 * FONT_HEIGHT);
  display.print("tHi:");

  display.setCursor(2 * FONT_WIDTH, 2 * FONT_HEIGHT);
  display.print("dT:");

  display.setCursor(1 * FONT_WIDTH, 3 * FONT_HEIGHT);
  display.print("Run:");

  display.display();
  drawRunVars();
}

void drawRunVars() {
  Serial.println("drawRunVars");

  display.setCursor(5 * FONT_WIDTH, 0);
  printTemp(5, 0, tempLow, 2);
  printTemp(5, 1, tempHigh, 2);
  printTemp(5, 2, dT, 2);

  display.setCursor(5 * FONT_WIDTH, 3 * 16);
  if (pumpRunning) {
    display.print("ON ");
  } else {
    display.print("off ");
  }

  display.display();
}

// Draw the reset screen:
//   1234567890
// 1 !!RESET!!
// 2   < NO >
// 3   < NO >
// 4  < YES >

void drawResetBack() {
  display.clearDisplay();
  display.setCursor(0.5 * FONT_WIDTH, 0);
  display.print("!!RESET!!");
  display.setCursor(4 * FONT_WIDTH, 1 * FONT_HEIGHT);
  display.print("NO");
  display.setCursor(4 * FONT_WIDTH, 2 * FONT_HEIGHT);
  display.print("NO");
  display.setCursor(3.5 * FONT_WIDTH, 3 * FONT_HEIGHT);
  display.print("YES");
  display.display();
  drawResetVars();
}

void drawResetVars() {
  for (int lp = 0; lp < 3; lp++) {
    display.setCursor(2.5 * FONT_WIDTH, (lp + 1) * FONT_HEIGHT);
    display.print(resetOption == lp ? "<" : " ");
    display.setCursor(6.5 * FONT_WIDTH, (lp + 1) * FONT_HEIGHT);
    display.print(resetOption == lp ? ">" : " ");
  }
  display.display();
}
