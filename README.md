# retro-rtc-alarm-clock

![image](https://user-images.githubusercontent.com/42916559/114238083-54a89800-9984-11eb-942f-cc3b6eb8380a.png)
Image: An Arduino-driven LCD display built into a 1970s alarm clock radio

Overview:

This design enables a modern 16x2 LCD display and Arduino-driven clock to replace the clock and driver circuits of a traditional LED clock radio.
The original radio used a neon 7-segment display which had reached the end of its lifetime.
The design involves an Arduino being used in place of the clock logic chip, driver and display circuits of the original device.
The radio circuitry and the physical control buttons for the clock are retained. Externally, other than the display, the device is unchanged.
The Arduino runs an autonomous clock which is periodically updated with an internal RTC time feed via I2C.

Main Components (all built into a 1970s clock radio):

 a. Arduino Nano
 
 b. RTC real-time clock module and button cell battery
 
 c. 5v power supply - a charger from a mobile phone was used.
 
 d. 16x2 LCD Display and driver hardware

(external buttons and a radio unit were provided by the surrounding device)

Operation:

The Arduino runs a sketch with a continuous loop which runs an autonomous clock, drives the display and waits for button inputs.
The clock is updated periodically with the accurate time from the RTC unit.
The display shows the time, date, alarm time and sleep countdown on the 16x2 LCD unit. 
The sketch controls an output pin which outputs 5v or 0v in order to turn a radio on or off according to the the alarm state.
The radio itself can be switched to activate radio or alarm tone, but the electronics and switching for this is part of the original radio unit, not part of the sketch.
Because the radio has no buttons for controlling the date display (original clock did not have date display), button combinations are needed to set the date.

Software (sketch) operation:

The main part of the sketch is a loop. Within this loop is a clock counter which runs in 1000ms intervals (i.e. it 'ticks' every second). 
Every minute the time display is updated with the time from the RTC module to keep the clock accurate.
The rest of the loop updates the display and checks the button presses using "if" trees to activate code e.g. to activate a function or scroll the display. 
No interrupts are used and no other loops are used other than the main loop. This main loop is also used to spin the time display forwards or backwards depending if a button is pressed.

Control Buttons:

The following are the seven buttons and switches which control the display. All buttons and switches set 
respective Arduino input pins to 0v (low state). These were existing buttons on the radio:
1. "time" - Latching pushbutton. Activates the time set mode. Usually the clock is in this mode. Another button is needed to change the time.
2. "signal" - Latching pushbutton. Activates alarm-time set mode which is indicated in the display.
3. "sleep" - Latching pushbutton. Activates a 60 minute countdown. Once the countdown is triggered, an output is set to high to activate the radio.
4. "illum" - Latching pushbutton. Activates a display dimming function. Also used to set the date in conjuction with "Alarm Off" button (because there is no date-set button).
5. "fast" and "slow" - Non-Latching pushbuttons. Used to scroll the time, signal and date depending on what other button is active
6. "Alarm off" - Large, non-latching button on top of device. Used to turn off alarm and also to enable reverse time scrolling when held down.
