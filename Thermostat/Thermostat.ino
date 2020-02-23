#include <DallasTemperature.h>
#include <EEPROM.h>
#include <OneWire.h>

// Our headers
#include "display.h"
#include "globals.h"

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

float tempHigh = 0, tempLow = 0, dT = 0;
boolean pumpRunning = false;
int resetOption = 0;
enum mode currentMode = MODE_RUNNING;
struct Settings settings;

//////////////////////////////////////////////////////////////////////
// Settings that we will store in EEPROM.
// We will store a magic number in the first element to check settings
// are in ther EEPROM and not junk.

#define EEPROM_MAGIC 8385

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
void setup() {
#ifdef DEBUG  
  Serial.begin(19200);
  Serial.println("Starting...");
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

  // Initialise display
  setupDisplay();

  sensors.begin();  // Start up the library
  
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  int deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");
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
