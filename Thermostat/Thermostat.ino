#include <Adafruit_SSD1306.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <Wire.h> // I2C

#define DEBUG 1

// How many milliseonds between 1-wire polling
#define TEMPERATURE_POLL 2500

// Debounce & long press
#define DEBOUNCE_DELAY 25
#define LONG_PRESS 500

// Which pins are our buttons on
#define B1_PIN 8
#define B2_PIN 9

// ... and the relay
#define RELAY_PIN 6

// What address in the EEPROM are our settings?
#define SETTINGS_ADDRESS 0

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 7

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);  
// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

// OLED display TWI address
#define OLED_ADDR 0x3C
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif
Adafruit_SSD1306 display(-1);

// We can have several modes of operation that are cycled through by
// holding either button.
// 
// - Running
// - Setting tOn
// - Setting tOff
// - Perform reset

enum mode {MODE_RUNNING, MODE_SETON, MODE_SETOFF, MODE_RESET};

// Button structures so we can share debounce code, etc

struct Button {
  int pin;
  int lastState = LOW;
  unsigned long debounceTime = 0;
  boolean pressed = false;
  boolean released = true; /* Assume starting state of released */
  int stepDirection;
};
struct Button b1, b2;

//////////////////////////////////////////////////////////////////////
// Declare some globals for read temperatures & pump state, etc.

float tempHigh, tempLow, dT;
boolean pumpRunning = false;
int resetOption = 0;
enum mode currentMode;

//////////////////////////////////////////////////////////////////////
// Settings that we will store in EEPROM.
// We will store a magic number in the first element to check settings
// are in ther EEPROM and not junk.

#define EEPROM_MAGIC 8385

struct Settings {
  int setup;
  float dtOn;
  float dtOff;
  DeviceAddress sensorLow;
  DeviceAddress sensorHigh;
};
struct Settings settings;

void saveSettings() {
  Serial.println("Saving settings.");
  EEPROM.put(SETTINGS_ADDRESS, settings);
}

void factoryReset() {
  Serial.println("Factory reset.");
  settings.setup = 0;
  settings.dtOn = 0;
  settings.dtOff = 0;
  memset(settings.sensorLow, 0, sizeof(DeviceAddress));
  memset(settings.sensorHigh, 0, sizeof(DeviceAddress));
}

void loadSettings() {
  // Read settings from EEPROM
  EEPROM.get(SETTINGS_ADDRESS, settings);
  
  // Check EEPROM was setup and wipe if not
  if (settings.setup != EEPROM_MAGIC) {
    factoryReset();
    settings.setup = EEPROM_MAGIC;
  } else {
    Serial.println("Loaded good settings.");
  }
}

//////////////////////////////////////////////////////////////////////

#define FONT_SIZE 2
#define FONT_SIZE_SUP 1
#define FONT_WIDTH 12
#define FONT_HEIGHT 16

//////////////////////////////////////////////////////////////////////
void setup() {
#ifdef DEBUG  
  Serial.begin(57600);
#endif

  // Input buttons
  pinMode(B1_PIN, INPUT);
  b1.pin = B1_PIN;
  b1.stepDirection = 1;

  pinMode(B2_PIN, INPUT);
  b2.pin = B2_PIN;
  b2.stepDirection = -1;

  // Relay
  pinMode(RELAY_PIN, OUTPUT);

  // Load settings from EEPROM
  loadSettings();
  
  // initialize and clear display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  // We always use this size & colours
  // (and if not, we always put it back)
  display.setTextSize(FONT_SIZE);
  display.setTextColor(WHITE, BLACK);
  drawRunBack();

  sensors.begin();  // Start up the library
  
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  int deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");
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

  temp = abs(temp);

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

void poll1Wire() {
  // Send command to all the sensors for temperature conversion
  sensors.requestTemperatures(); 

  // TODO: Setup for sensors
  tempLow = sensors.getTempCByIndex(0);
  tempHigh = sensors.getTempCByIndex(1);
}

//////////////////////////////////////////////////////////////////////
// Mode change

void modeChange() {
  switch (currentMode) {
    case MODE_RUNNING:
      currentMode = MODE_SETON;
      drawSetupBack();
      break;

    case MODE_SETON:
      currentMode = MODE_SETOFF;
      drawSetupBack();
      break;
    
    case MODE_SETOFF:
      currentMode = MODE_RESET;
      saveSettings();
      resetOption = 0;
      drawResetBack();
      break;

    case MODE_RESET:
      if (resetOption == 2) {
        // Yes, they really selected factory reset!
        factoryReset();
        saveSettings();
      }        
      currentMode = MODE_RUNNING;
      drawRunBack();
      break;
  }
}

//////////////////////////////////////////////////////////////////////
// Button handling
// This is why we have button objects defined and setup above. So we
// can use a function for the debounce/long press detection.

void handleButton(Button *button) {
  int state = digitalRead(button->pin);
  if (state != button->lastState) {
    button->debounceTime = millis();
    button->lastState = state;
  } else if ((millis() - button->debounceTime) >= DEBOUNCE_DELAY) {
    if (state == HIGH) {
      if ((millis() - button->debounceTime) >= LONG_PRESS && !button->pressed) {
        // This is a long press, which always triggers mode change
        button->pressed = true;
        modeChange();
      }
      button->released = false;
    } else if (state == LOW && !button->released) {
      // Successfully transitioned low so if this wasn't pressed register
      if (button->pressed) {
        // Long press release, just reset that
        button->pressed = false;
      } else {
        // Button was just released but long press wasn't triggered so must be a click
        // Action depends on current mode
        switch (currentMode) {
          case MODE_RUNNING:
            // Do nothing
            break;
          case MODE_SETON:
            settings.dtOn += button->stepDirection * 0.1;
            drawSetupVars();
            break;
          case MODE_SETOFF:
            settings.dtOff += button->stepDirection * 0.1;
            drawSetupVars();
            break;
          case MODE_RESET:
            resetOption += button->stepDirection;
            if (resetOption > 2) {
              resetOption = 2;
            } else if (resetOption < 0) {
              resetOption = 0;
            }
            drawResetVars();
        }
      }
      button->released = true;
    }
  }
}

//////////////////////////////////////////////////////////////////////
// In main loop we're only going to read temperatures every x ms so
// keep an eye on last time we read.
//
// Note that millis() function loops back to zero every 49 days or so but as
// it's an unsigned long don't have to worry about this. Same for the above.

unsigned long lastPoll = 0;

void loop() {
  if (currentMode == MODE_RUNNING) {
    if (millis() - lastPoll >= TEMPERATURE_POLL) {
      poll1Wire();
      dT = tempHigh - tempLow;
      if (pumpRunning && dT <= settings.dtOff) {
        pumpRunning = false;
      } else if (!pumpRunning && dT >= settings.dtOn) {
        pumpRunning = true;
      }
      digitalWrite(RELAY_PIN, pumpRunning ? HIGH : LOW);
      drawRunVars();
      lastPoll = millis();
    }
  }

  // Button stuff with debounce
  handleButton(&b1);
  handleButton(&b2);
}
