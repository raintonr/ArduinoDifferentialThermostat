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

In this mode (and only this mode), the relay will turn on and off automatically based on the sensors and settings values configured.

From 'Running mode' an extra long press (default of 1.5s) on the plus button moves to the 'Settings' screen, and on the minus button moves to 'Factory Reset' screen.

### Settings screen

Use this screen to adjust the available settings or manually turn the relay on or off.

Settings are only committed to the EEPROM on exit from this screen, so if settings are adjusted and then a power fail occurs they will have not have been saved.

The following settings are available:

- tOn - the temperature differential (equal to or above) at which the relay will turn on.
- tOff - the temperature differential (less than or equal to) at which the relay will turn off.
- tMax - a maximum temperature allowed at tLow. If tLow is at or above this temperature the relay will be inhibited (constantly off).
- Run - Not really a setting, but this allows allows toggling of the relay state for manual control.

An angle bracket (">") shows the current setting being adjusted with the buttons (plus and minus) adjusting the value accordingly. tOn & tOff are adjusted in 0.1 steps and tMax in steps of 1.0.

Long press plus/minus buttons to move to the next/previous parameter. A long press on the top or bottom parameter in the direction away from the others will move back to the 'Running' mode or on to the 'Factory Reset' screen.

### Factory Reset screen

From this screen, long press the minus button to return to 'Running' mode or the plus button to move to 'Settings' screen.

Factory Reset shows 3 options: "NO", "NO" and "YES". The selected option is highlighed with angle brakcets (eg: "< NO >"). Move between options with the plus/minus buttons.

If either of the "NO" options are highlighted when using long press to change away from this mode then no action will be taken. If "YES" is highlighed then all settings will be wiped (set to zero) and sensor addresses will also be cleared. This is basically the same as power on for the first time.

### Sensors screen

If the 1-Wire bus does not have 2 sensors preset, or they both cannot be read, or a sensor is swapped out for one that was previously present (ie. a DS18B20 failed and was replaced) then this screen with the status of tLow & tHi is shown.

On sensor removal or failure this 'Sensors' screen is only shown from the 'Running' screen. Ie. if a sensor fails when adjusting settings or on the 'Factory Reset' screen then nothing will happen.

This screen shows for each sensor:

- The current temperature or "err!" if the value cannot be read (or is considered spurious - below -100 or above 200 degees C).
- The sensor's 1-Wire address if setup, or a message requesting it is connected.

#### First run

Also applicable after performing a factory reset.

If the board is first run with no sensors connected then this 'Sensors' screen will appear with the message "Connect tLow" (or tHi) showing.

In this case, the next sensor detected on the 1-Wire bus will be assigned to the missing role. Eg. from no sensors found, connecting a sensor will assign this to tLow. Connecting a second sensor will assign that to tHi.

It is perfectly acceptable to connect VCC & GND of sensors before powering on the board, and then connect the data wires when it is powered on. In fact, this is the only way to assign roles to each sensor in a known order.

In practice this means that if 2 (or more) sensors are connected to the 1-Wire bus on first run (or factory reset) they will be instantly detected and assigned to tLow & tHi in the order of that detection, which may appear random. The only way to avoid 'random' assignment of roles to each sensor is to connect tLow then tHi in sequence as mentioned above.

#### Replacing a sensor

If a sensor fails then this 'Sensors' screen will be shown with an "err!" message showing which sensor is bad.

As sensors are assigned roles based on their address, and these are unique to every sensor then it is not possible to simply swap a defective sensor for a new one.

In this case (a sensor has failed), perform a 'Factory Reset' and follow the 'First run' procedure above to re-assign sensor roles as applicable.

## Hardware

Connect the following hardware:

### Dallas 1-Wire Bus

This sketch uses the OneWire and DallasTemperature libraries with the 1-Wire bus on pin 7.

It has provision for 2 x DS18B20 temperature probes. Connect these to VCC, GND & data to pin 7 with a 4.7k resistor between VCC & data.

These sensors are known as tHi (the 'hot' sensor) and tLow (the 'cold' sensor). Because these sensors have specific usage then their addresses are stored in settings 

### I2C display

Connect a 128x64 SSD1306 compatible I2C display to the I2C bus - pins 18 & 19.

### Buttons

2 buttons are used for configuring settings. Connect these to pins 8 & 9 either using a resistor network or optoisolator.

### Relay

A relay is controlled based on temperature difference between the probes (and has a manual override) and should be connected to pin 6.
