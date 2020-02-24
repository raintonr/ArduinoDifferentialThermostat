#include <DallasTemperature.h> // For DeviceAddress

void setupDisplay();

boolean allSensorsDefined();
boolean isZeroAddress(DeviceAddress address);
boolean isValidTemp(float temp);

void drawSetupBack();
void drawSetupMode();
void drawSetupVars(boolean);
void drawRunBack();
void drawRunVars();
void drawRunState();
void drawHeartbeat();
void drawResetBack();
void drawResetVars();
void drawSensorsBack();
void drawSensorsVars();
