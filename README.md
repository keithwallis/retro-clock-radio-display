# retro-clock-radio-display
Arduino-driven LED display controller with basic clock-radio functions
Hardware-related Features: This sketch is used to control an Arduino Uno or Nano to enable it to emulate the sequential logic controller chips found in alarm clock radios of the 1970s. 
The sketch programs input pins, three of which are intended for radio buttons used to set the display to time, alarm, sleep-countdown respectively. These pins are activated by 0v. Another pin is used by a toggle button to select the brightness level to bright/dim.
The sketch programs an output pin which outputs 5v or 0v in order to turn a radio on or off according to the the alarm state.
