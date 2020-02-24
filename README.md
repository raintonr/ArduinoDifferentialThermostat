# ArduinoDifferentialThermostat

Thermostatic controller based on difference between 2 probes for Arduino (Atmega328P).

This sketch is for a pump (or anything that can be switched with a relay) controller that should be activated based on the temperature difference between 2 sensors.

## Use

The screen always shows a 'heatbeat' top right corner which should be an ASCII based spinner. In addition, the following modes are present:

### Running mode

On first run, if both DS18B20 sensors are connected the screen will display 'Running' mode which consists of:

- tLow - temperature of 'cold' DS18B20 sensor.
- tHi - temperature of 'hot' DS18B20 sensor.
- dT - Difference between the two sensors.
- Run - Status of the relay (on or off).

### Settings screen

From 'Running mode' an extra long press (default of 1.5s) on either button moves to the settings screen. In this screen temperatures are not sensed and the relay will not turn on or off automatically.

Use this screen to adjust the available settings or manually turn the relay on or off.

Settings are only committed to the EEPROM on exit from this screen, so if settings are adjusted and then a power fail occurs they will have not have been saved.

The following settings are available:

- tOn - the temperature differential (equal to or above) at which the relay will turn on.
- tOff - the temperature differential (less than or equal to) at which the relay will turn off.
- tMax - a maximum temperature allowed at tLow. If tLow is at or above this temperature the relay will be inhibited (constantly off).
- Run - Not really a setting, but this allows allows toggling of the relay state for manual control.

An angle bracket (">") shows the current setting being adjusted with the buttons (plus and minus) adjusting the value accordingly. tOn & tOff are adjusted in 0.1 steps and tMax in steps of 1.0.

tOn is being adjusted when the screen is first displayed. Long press either button to move to the next paramter. Long press on 'Run' will move on to the 'Factory Reset' screen (and after back to 'Running' mode).

### Factory Reset screen

The 'Factory Reset' screen is reached after settings (long press from 'Run' adjustment).

From this screen, long press either button to return to 'Running' mode.

Factory Reset shows 3 options: "NO", "NO" and "YES". The selected option is highlighed with angle brakcets (eg: "< NO >"). Move between options with the plus/minus buttons.

If either "NO" is highlighted on using long press to return to 'Running' mode then no action will be taken. If "YES" is highlighed then all settings will be wiped (set to zero) and sensor addresses will also be cleared. This is basically the same as power on for the first time.

## Hardware

Connect the following hardware:

### Dallas 1-Wire Bus

This sketch uses the OneWire and DallasTemperature libraries with the 1-Wire bus on pin 7.

It has provision for 2 x DS18B20 temperature probes. Connect these to VCC, GND & data to pin 7 with a 4.7k resistor between VCC & data.

### I2C display

Connect a 128x64 SSD1306 compatible I2C display to the I2C bus - pins 18 & 19.

### Buttons

2 buttons are used for configuring settings. Connect these to pins 8 & 9 either using a resistor network or optoisolator.

### Relay

A relay is controlled based on temperature difference between the probes (and has a manual override) and should be connected to pin 6.
