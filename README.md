# retro-clock-radio-display
Arduino-driven LCD display controller with basic clock-radio functions.
The Arduino runs an autonomous clock which is periodically updated with a RTC time feed via I2C.
The display shows the time, date, alarm time and sleep countdown on the 16x2 LCD unit.
The sketch is used to control an Arduino Uno or Nano which in this case was built into a 1970s alarm clock radio.  
The sketch programs an output pin which outputs 5v or 0v in order to turn a radio on or off according to the the alarm state.
The radio itself can be switched to activate radio or alarm tone, but the electronics and switching for this is part of the original radio unit, not part of the sketch.

The following are the buttons and switches which control the display. All buttons and switches set 
respective Arduino input pins to 0v (low state):
1. "time" - Latching pushbutton. Activates the time set mode
2. "signal" - Latching pushbutton. Activates alarm-time set mode
3. "sleep" - Latching pushbutton. Activates a 60 minute countdown with the radio active
4. "illum" - Latching pushbutton. Activates the display dimming function. Also used to set the date in conjuction with "Alarm Off" button.
5. "fast" and "slow" - Non-Latching pushbuttons. Used to scroll the time, signal and date depending on what other button is active
6. "Alarm off" - Large, non-latching button on top of device. Used to turn off alarm and also to enable reverse time scrolling.
