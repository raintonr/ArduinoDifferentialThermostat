#include <DallasTemperature.h>
#include <EEPROM.h>
#include <OneWire.h>

// Our headers
#include "display.h"
#include "globals.h"

// Uncomment for a bit of debug
// #define DEBUG 1

// How many milliseonds between 1-wire polling
#define TEMPERATURE_POLL 2000

// How many milliseonds after 1-wire polling should we read temperatures
#define TEMPERATURE_READ_DELAY 750

// Heartbeat display interval in ms
#define HEARTBEAT 500

// Debounce & long press
#define DEBOUNCE_DELAY 25
#define LONG_PRESS 250

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
// Increment this when changing code so old settings are discarded.

#define EEPROM_MAGIC 8386

void saveSettings() {
#ifdef DEBUG    
  Serial.println("Saving settings.");
#endif
  EEPROM.put(SETTINGS_ADDRESS, settings);
}

void factoryReset() {
#ifdef DEBUG    
  Serial.println("Factory reset.");
#endif
  settings.setup = 0;
  settings.dtOn = 0;
  settings.dtOff = 0;
  settings.tMax = 0;
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
#ifdef DEBUG    
    Serial.println("Loaded good settings.");
#endif
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

  // Start up the 1-wire library
  sensors.begin();
}

// Check to make sure the passed address is not stored as a known
// sensor.

int addressCmp(DeviceAddress a1, DeviceAddress a2) {
  int cmp = 0;
  for (int lp = 0; lp < sizeof(DeviceAddress); lp++) {
    if ((a1[lp]) != a2[lp]) {
      cmp = a1[lp] - a2[lp];
      break;
    }
  }
  return cmp;
}

boolean isKnownSensor(DeviceAddress address) {
  return ((addressCmp(address, settings.sensorLow) == 0 || addressCmp(address, settings.sensorLow) == 0) ? true : false);
}

void poll1Wire() {
  // If we don't have both sensors defined, poll the wire and
  // put any new addresses found in our empty settings.
  if (!allSensorsDefined()) {
#ifdef DEBUG    
    Serial.println("Searching for sensors...");
#endif
    DeviceAddress address;
    int found = 0;
    boolean updates = false;
    while (oneWire.search(address)) {
      found++;
      if (isZeroAddress(settings.sensorLow) && !isKnownSensor(address)) {
        memcpy(settings.sensorLow, address, sizeof(DeviceAddress));
        updates = true;
      } else if (isZeroAddress(settings.sensorHigh) && !isKnownSensor(address)) {
        memcpy(settings.sensorHigh, address, sizeof(DeviceAddress));
        updates = true;
      }
    }
    if (updates) {
      saveSettings();
    }
#ifdef DEBUG    
    Serial.print("Found ");
    Serial.print(found);
    Serial.println(" sensors");
#endif    
  }
  // We have our sensors defined, get their values
  sensors.setWaitForConversion(false);  // makes it async
  sensors.requestTemperatures();
  sensors.setWaitForConversion(true);
}

void read1Wire() {
  tempLow = sensors.getTempC(settings.sensorLow);
  tempHigh = sensors.getTempC(settings.sensorHigh);
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
      drawSetupMode();
      break;
    
    case MODE_SETOFF:
      currentMode = MODE_SETMAX;
      drawSetupMode();
      break;

    case MODE_SETMAX:
      currentMode = MODE_SETRUN;
      drawSetupMode();
      break;

    case MODE_SETRUN:
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
            drawSetupVars(false);
            break;
          case MODE_SETOFF:
            settings.dtOff += button->stepDirection * 0.1;
            drawSetupVars(false);
            break;
          case MODE_SETMAX:
            settings.tMax += button->stepDirection;
            drawSetupVars(false);
            break;
          case MODE_SETRUN:
            // I guess we could call drawRunState here but don't
            // do that...just for consistency.
            pumpRunning = !pumpRunning;
            drawSetupVars(false);
            setRelayState();
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
unsigned long lastHeartbeat = 0;
boolean wasInvalid = true;
boolean doneSensorRead = false;

void setRelayState() {
  digitalWrite(RELAY_PIN, pumpRunning ? HIGH : LOW);
}

void loop() {
  if (currentMode == MODE_RUNNING) {
    if (millis() - lastPoll >= TEMPERATURE_POLL) {
      // Poll requests temperatures but doesn't wait to read, we do that below
      lastPoll = millis();
      doneSensorRead = false;
      poll1Wire();
    } else if (millis() - lastPoll >= TEMPERATURE_READ_DELAY && !doneSensorRead) {
      // Poll should have a result, go read them
      read1Wire();
      doneSensorRead = true;

      // Force display of sensors if they aren't both good
      // We only do this in the running mode so that settings
      // can be reached when sensors are bad.
      // Regular running mode

      if (!allSensorsDefined() || !isValidTemp(tempLow) || !isValidTemp(tempHigh)) {
        // Force pump off on faulty sensors
        pumpRunning = false;

        // Draw background if we're just going bad
        if (!wasInvalid) {
          drawSensorsBack();
          wasInvalid = true;
        }
        // Always draw vars
        drawSensorsVars();
      } else {
        if (wasInvalid) {
          wasInvalid = false;
          drawRunBack();
        }
        dT = tempHigh - tempLow;
        // Check for maximum temperature (on tempLow)
        if (tempLow >= settings.tMax) {
          pumpRunning = false;
        } else {
          // Temp below max, so pump based on dT
          if (pumpRunning && dT <= settings.dtOff) {
            pumpRunning = false;
          } else if (!pumpRunning && dT >= settings.dtOn) {
            pumpRunning = true;
          }
        }
        drawRunVars();
      }
      setRelayState();
    }
  }

  // Always show heartbeat
  if (millis() - lastHeartbeat >= HEARTBEAT) {
    drawHeartbeat();
    lastHeartbeat = millis();
  }

  // Button stuff with debounce
  handleButton(&b1);
  handleButton(&b2);
}
