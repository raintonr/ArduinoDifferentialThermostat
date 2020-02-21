#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// How many milliseonds between 1-wire polling
#define TEMPERATURE_POLL 5000

// Debounce & long press
#define DEBOUNCE_DELAY 50
#define LONG_PRESS 500

// Which pins are our buttons on
#define B1_PIN 8
#define B2_PIN 9

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 7

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);  

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

// OLED display TWI address
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(-1);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// We can have several modes of operation that are cycled through by
// holding either button.
// 
// - Setting tOn
// - Setting tOff
// - Running

#define MODE_RUNNING 0
#define MODE_SETON 1
#define MODE_SETOFF 2

unsigned long currentMode = MODE_RUNNING;

// Button structures so we can share debounce code, etc

struct Button {
  unsigned long pin;
  unsigned long lastState = LOW;
  unsigned long debounceTime = 0;
  unsigned long pressed = false;
  unsigned long released = true; /* Assume starting state of released */
  float setupAdjust;
};

struct Button b1, b2;

void setup() {
  // Input buttons
  pinMode(B1_PIN, INPUT);
  b1.pin = B1_PIN;
  b1.setupAdjust = 0.1;

  pinMode(B2_PIN, INPUT);
  b2.pin = B2_PIN;
  b2.setupAdjust = -0.1;
  
  // initialize and clear display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  drawRunBack();

  sensors.begin();  // Start up the library
  Serial.begin(9600);
  
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  int deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");
}

// Declare some globals for read temperatures & on/off thresholds

float dtOn, dtOff;
float tempHigh, tempLow, dT;
unsigned long pumpRunning = false;

// Our display is 128x64 (wide/high).
// 0,0 is top left
// In text size 2 this gives 4 rows of 10 characters
// Text size 1 gives 8 rows of 21 characters

#define FONT_SIZE 2
#define FONT_WIDTH 12
#define FONT_HEIGHT 16

void printTemp(float temp) {
  char buff[32];
  int value = int(temp * 10);
  sprintf(buff, "%c%2d.%01d", value < 0 ? '-' : ' ', value / 10, abs(value) % 10);
  Serial.println(buff);
  display.print(buff);
}

// Draw the setup screen:
//   1234567890
// 1  tOn:-99.9
// 2 tOff:-99.9
// 3 
// 4 

void drawSetupBack() {
  display.clearDisplay();
  display.setTextSize(FONT_SIZE);
  display.setTextColor(WHITE);
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

  display.setTextSize(FONT_SIZE);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(5 * FONT_WIDTH, 0);
  printTemp(dtOn);
  display.setCursor(5 * FONT_WIDTH, 1 * FONT_HEIGHT);
  printTemp(dtOff);
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
  display.setTextSize(FONT_SIZE);
  display.setTextColor(WHITE);
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

  display.setTextSize(FONT_SIZE);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(5 * FONT_WIDTH, 0);
  printTemp(tempLow);

  display.setCursor(5 * FONT_WIDTH, 1 * 16);
  printTemp(tempHigh);
 
  display.setCursor(5 * FONT_WIDTH, 2 * 16);
  printTemp(dT);

  display.setCursor(5 * FONT_WIDTH, 3 * 16);
  if (pumpRunning) {
    display.print("ON ");
  } else {
    display.print("off ");
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
      currentMode = MODE_RUNNING;
      drawRunBack();
      break;
  }
}

void handleButton(Button *button) {
  unsigned long state = digitalRead(button->pin);
  if (state != button->lastState) {
    button->debounceTime = millis();
    button->lastState = state;
  } else if ((millis() - button->debounceTime) > DEBOUNCE_DELAY) {
    if (state == HIGH) {
      if ((millis() - button->debounceTime) > LONG_PRESS && !button->pressed) {
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
            dtOn += button->setupAdjust;
            drawSetupVars();
            break;
          case MODE_SETOFF:
            dtOff += button->setupAdjust;
            drawSetupVars();
            break;
        }
      }
      button->released = true;
    }
  }
}

// In main loop we're only going to read temperatures every x ms so keep an eye on
// last time we read.
// Note that millis() function loops back to zero every 50 days or so.

unsigned long lastPoll = 0;

void loop() {
  if (currentMode == MODE_RUNNING) {
    if (millis() - lastPoll > TEMPERATURE_POLL) {
      poll1Wire();
      dT = tempHigh - tempLow;
      if (pumpRunning && dtOff <= dT) {
        pumpRunning = false;
      } else if (!pumpRunning && dtOn >= dT) {
        pumpRunning = true;
      }
      // TODO: set relay
      drawRunVars();
      lastPoll = millis();
    }
  }

  // Button stuff with debounce
  handleButton(&b1);
  handleButton(&b2);
}
