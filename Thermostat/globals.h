#include <DallasTemperature.h> // For DeviceAddress

// We can have several modes of operation that are cycled through by
// holding either button.
// 
// - Running
// - Setting tOn
// - Setting tOff
// - Setting tMax
// - Perform reset
enum mode {MODE_RUNNING, MODE_SETON, MODE_SETOFF, MODE_SETMAX, MODE_RESET};

// Settings that will be stored in EEPROM
struct Settings {
  int setup;
  float dtOn;
  float dtOff;
  float tMax;
  DeviceAddress sensorLow;
  DeviceAddress sensorHigh;
};

// Extern references - all declared in main code

extern float tempLow, tempHigh, dT;
extern boolean pumpRunning;
extern int resetOption;
extern enum mode currentMode;
extern struct Settings settings;
