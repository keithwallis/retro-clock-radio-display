#include <Adafruit_LEDBackpack.h>

#include <Boards.h>
#include <Firmata.h>
#include <FirmataConstants.h>
#include <FirmataDefines.h>
#include <FirmataMarshaller.h>
#include <FirmataParser.h>
#include <elapsedMillis.h> //load the library

// Clock example using a seven segment display & DS1307 real-time clock.
//
// Must have the Adafruit RTClib library installed too!  See:
//   https://github.com/adafruit/RTClib
//
// Core features are designed to work with the Adafruit LED 7-Segment backpacks
// and DS1307 real-time clock breakout:
// ----> http://www.adafruit.com/products/881
// ----> http://www.adafruit.com/products/880
// ----> http://www.adafruit.com/products/879
// ----> http://www.adafruit.com/products/878
// ----> https://www.adafruit.com/products/264
//
// Adafruit invests time and resources providing this open source code,
// please support Adafruit and open-source hardware by purchasing
// products from Adafruit!
//
// Core code (particularly in set-up and clock loop) by Tony DiCola for Adafruit Industries.
// This code was extended.
// Released under a MIT license: https://opensource.org/licenses/MIT

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <RTClib.h>
#include "Adafruit_LEDBackpack.h"
#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      true

// I2C address of the display.  Stick with the default address of 0x70
// unless you've changed the address jumpers on the back of the display.
#define DISPLAY_ADDRESS   0x70


// Create display and DS1307 objects.  These are global and 
// are available from both the setup and loop function below.
Adafruit_7segment clockDisplay = Adafruit_7segment();
RTC_DS1307 rtc = RTC_DS1307();

//Create instances of the elapsed millisecond object for tracking absolute time in ms. 
elapsedMillis timeElapsed; // For the clock loop
elapsedMillis AlarmTimerMillis; // For the alarm timeout - not used



// Keep track of the hours, minutes, seconds displayed by the clock.
// Start off at 0:00:00 as a signal that the time should be read from
// the DS1307 to initialize it.

int years = 0;
int months = 0;
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

// Initialise the settable values for the auxilliary clock functions.
int alarm_time = 0; /* an integer holding a 4 digit test-display number*/
int alm_hrs = 0; /* Initialise alarm hours default */
int alm_mins = 0; /* Initialise alarm mins default */
const int sleep_timer_max = 60; /* Set sleep timer minutes default maximum */
int sleep_timer = sleep_timer_max; /* Initialise sleep timer to default maximum */

// Brightness level settings
const int bright = 5;
const int dim = 0;

// Define constants for the control pin numbers
const int alarm_arm = 2; /* Define pin number for arming the alarm/radio driver */
const int dim_in = 4; /* Define pin number for dimming display */
const int sleep_out = 5; /* Define sleep out */
const int alarm_off_in = 6; /* Define pin number for turning off the radio/alarm */
const int sleep_in = 7; /* Define sleep pin number */
const int signal_out = 8; /* Define radio-alarm out */
const int set_time_in = 9; /* Define set-time pin number */
const int slow_set_in = 10; /* Define mins increment pin number */
const int fast_set_in = 11; /* Define hours increment pin number */
const int set_alarm_in = 12; /* Define set-alarm pin number */

// Remember if the colon was drawn on the display so it can be blinked
// on and off every second.
bool blinkColon = false;
bool signalActive = false;

//////////////////////////// Set Up /////////////////////////////////////

void setup() {
  // Setup function runs once at startup to initialize the display
  // and DS1307 clock. This runs on power-up.

  // Setup Serial port to print debug output.
  Serial.begin(115200);
  Serial.println("Clock starting!");

  // Setup control pins and states
  pinMode(dim_in, INPUT); /* Configures pin for input of alarm activation switch*/
  pinMode(alarm_arm, INPUT); /* Configures pin for arming the alarm*/
  pinMode(sleep_out, OUTPUT); /* Configures the pin for radio activation for sleep*/
  pinMode(signal_out, OUTPUT); /* Configures the pin for buzzer or radio activation*/
  pinMode(set_time_in, INPUT); /* Configures pin for input from set time switch*/
  pinMode(fast_set_in, INPUT);
  pinMode(slow_set_in, INPUT);
  pinMode(set_alarm_in, INPUT);
  pinMode(sleep_in, INPUT);
  pinMode(alarm_off_in, INPUT);
  digitalWrite(signal_out, LOW); /* Set the alarm output pin to low as default*/
  digitalWrite(sleep_out, LOW); /* Set the sleep output pin to low as default*/  
  digitalWrite(fast_set_in, HIGH); /* Enables the internal pullup on the input pin to keep it high, unless grounded*/
  digitalWrite(set_time_in, HIGH);
  digitalWrite(slow_set_in, HIGH);
  digitalWrite(set_alarm_in, HIGH);
  digitalWrite(sleep_in, HIGH);
  digitalWrite(alarm_off_in, HIGH); /* For sensing the alarm off button */
  digitalWrite(alarm_arm, HIGH); /* For sensing the switch to activate alarm function */
  digitalWrite(dim_in, HIGH); /* For sensing the dimming switch to dim the display */

  // Setup the display.
  clockDisplay.begin(DISPLAY_ADDRESS);

  // Setup the DS1307 real-time clock and start it.
  rtc.begin();

  // Set the DS1307 clock if not set before, i.e. is not running.
  bool ClockStopped = !rtc.isrunning();
  // Alternatively you can force the clock to be set again by
  // uncommenting this line:
  // ClockStopped = true;
  if (ClockStopped) {
    // Serial.println("Setting DS1307 time!");
    // This line sets the DS1307 time to the exact date and time the
    // sketch was compiled:
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Alternatively you can set the RTC with an explicit date & time,
    // for example to set January 31, 2010 at 3am you would uncomment:
    //rtc.adjust(DateTime(2010, 1, 31, 0, 0, 0));
  }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////Main Loop///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
void loop() {
  // Loop function runs over and over again to implement the clock logic.
  
  // Turn the display of hours and minutes  into a numeric
  // value, like 3:30 becomes 330, by multiplying the hour by
  // 100 and then adding the minutes.
  int displayValue = hours * 100 + minutes;//
  
  ////////////////RTC Clock Mode///////////////////////////////
  // Check if it's the top of the hour and get a new time reading
  // from the DS1307.  This helps keep the clock accurate by fixing
  // any drift.
  if( rtc.isrunning()){
    if (minutes == 0) {  
      DateTime now = rtc.now();
      // Print out the time for debug purposes:
      Serial.print("Read date & time from DS1307: ");
      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print(' ');
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.println();

      // Set the time variables to actual values retrieved from the DS1307.
      years = now.year();
      months = now.month();
      days = now.day();
      hours = now.hour();
      minutes = now.minute();
      seconds = now.second();

    } // End of the rtc time-setting
  } // End of test for rtc running

  // Carry out activities is one-second cycle like tick the clock
  // or turn off alarm or decrement sleep timer
  if (timeElapsed > 1000) {

  ////////////////Arduino Internal Clock///////////////////////////////
    // This enables the clock to work independently of the rtc.
    // It is based on incrementing the seconds.
    seconds += 1;
    // If the seconds go above 59 then the minutes should increase and
    // the seconds should wrap back to 0.
    if (seconds > 59) {
      seconds = 0;
      minutes += 1;
      // Again if the minutes go above 59 then the hour should increase and
      // the minutes should wrap back to 0.
      if (minutes > 59) {
        minutes = 0;
        hours += 1;
        // Just to be safe, wrap
        // back to 0 if it goes above 23 (i.e. past midnight).
        if (hours > 23) {
          hours = 0;
        }
      }
    } ///////// End the Arduino own clock code////////////
  
  // Blink the colon by flipping its value every loop iteration
  // (which happens every time-elapsed iteration).
  blinkColon = !blinkColon;

  // Alarm triggering sequence senses for given hour/minute if alarm
  // is armed. 
  if ((hours == alm_hrs) && (minutes == alm_mins) && (seconds == 0)) { // If conditions fulfilled
    digitalWrite(signal_out, HIGH); // Activate the signal
    signalActive = true; // Set the signal flag on
  }
  
  if ((hours == alm_hrs) && (minutes == alm_mins) && (seconds == 59)) { // If conditions fulfilled
    digitalWrite(signal_out, LOW); // Deactivate the signal after 59 seconds or if alarm_off is pressed
    signalActive = false; // Set the signal flag off
  }
  
  // Sleep timer countdown per minute if the sleep function is active
  if (digitalRead(sleep_in) == LOW){
      // Decrement the sleep timer every minute as long as sleep button is active. 
    if ((seconds == 0) && (sleep_timer > 0)) {
      sleep_timer -= 1;
    }
  }

  // Following line resets the time elapsed counter
  timeElapsed = 0;
    
 }  // End of the 1-second clock and activity  cycle.
 
//////////////////////Display Time///////////////////////////
if ((digitalRead(sleep_in) == HIGH) && (digitalRead(set_time_in)) == LOW) { //Display the time only if the sleep display is off
  digitalWrite(sleep_out, LOW); //Make sure sleep output is off when sleep not activated
  // Do 24 hour to 12 hour format conversion when required. 
  if (!TIME_24_HOUR) {
    // Handle when hours are past 12 by subtracting 12 hours (1200 value).
    if (hours > 12) {
      displayValue -= 1200;
    }
    // Handle hour 0 (midnight) being shown as 12.
    else if (hours == 0) {
      displayValue += 1200;
    }
  }
  //Print data to a buffer, pad if necessary, then give a command to write the buffer to the display.  
  //So use the order print(to the buffer) ->  pad(zeros if needed) -> write (to the display).
  clockDisplay.print(displayValue, DEC);

  // Pad with leading zeros when in 24hr mode, so _3:_3 would become 03:03
    if (TIME_24_HOUR) {
      padHHmm(hours, minutes);
    }

   //Draw the colon (which will be blinked alternately in the time call iteration)
   clockDisplay.drawColon(blinkColon);

   if ((digitalRead(fast_set_in) == LOW) ^ (digitalRead(slow_set_in) == LOW)) {
     SpinHHmm(hours, minutes);
     //Set clock values individually to the rtc unit - this will overwrite some values with values set above.
     rtc.adjust(DateTime(years, months, days, hours, minutes, seconds));
   }

   ////////////////////Signal Active Indicator//////////////////////////////////////
   // Light up indicator LED in position 4 to show alarm/signal is active in time display mode
   if (signalActive) {
     // Turn on alarm-armed LED indication by illuminating DP position 4
     // Uses writeDigitNum to write a digit with a DP using modulo 
     // division of the minutes to get the last digit and add DP
     clockDisplay.writeDigitNum(4, (minutes%10), true); 
//     clockDisplay.writeDisplay();
   } 
   else {
     clockDisplay.writeDigitNum(4, (minutes%10), false);
   }

   /////////////////RTC Battery Fail Indicator/////////////////////////////////////<------------!
if(!rtc.isrunning()){
    // Light up indicator Decimal Point in position 1 (second digit position) to show rtc is not running
    // Uses writeDigitNum to write a digit with a DP using modulo 
    // division of the hours to get the last digit and add DP
    clockDisplay.writeDigitNum(1, (hours%10), true);
    //rtc.begin(); //Try to restart the rtc <-----------------!
   }
   else {
    clockDisplay.writeDigitNum(1, (hours%10), false);  
   }
  
   
    clockDisplay.writeDisplay(); 
 }// End of time display section

 //////////////Set Brightness////////////////////
  if (digitalRead(dim_in) == LOW) { //Dim the display acc. to dim/bright
    clockDisplay.setBrightness(dim);
  }
  else {
    clockDisplay.setBrightness(bright); // Otherwise set it bright
  } // End Set Brightness code


//////////////// Turn Off Alarm////////////////////////////////
  if (digitalRead(alarm_off_in) == LOW) { // If alarm-off button pressed
    digitalWrite(signal_out, LOW);
    signalActive = false; // Set the signal flag off
  }
  
//////////////// SLEEP setting and display function////////////////////////////////
  if ((digitalRead(sleep_in) == LOW) && (digitalRead(set_time_in)) == HIGH) { //Display the sleeptime only if the sleep display is on
    //Push out to the display the value. It will be the maximum sleep value initially.
    //Use the order print->pad->write.
    //Send to display. Use the order print->pad->write.    
    clockDisplay.print(sleep_timer, DEC);
    // Zero padding
      if (sleep_timer < 10) {
        // Pad tens of minutes with 0.
        clockDisplay.writeDigitNum(3, 0);
      }
      clockDisplay.writeDisplay();
      if ((digitalRead(fast_set_in) == LOW) ^ (digitalRead(slow_set_in) == LOW)) {
        sleep_timer -= 1;
        delay(50);
      if (digitalRead(slow_set_in) == LOW) {
        delay(500);
      }
      // Wrap sleep timer back to maximum.
      if (sleep_timer < 00) {
        sleep_timer = sleep_timer_max;
      }
      //Send to display. Use the order print->pad->write.    
      clockDisplay.print(sleep_timer, DEC);
      // Zero padding
      if (sleep_timer < 10) {
        // Pad tens of minutes with 0.
        clockDisplay.writeDigitNum(3, 0);
      }
      clockDisplay.writeDisplay();
    }  

    // Sleep timer activation cycle
    if (sleep_timer > 0 && sleep_timer <= sleep_timer_max){
      digitalWrite(sleep_out, HIGH);  // Turns on radio activation signal
      }
      else {
        digitalWrite(sleep_out, LOW);   // Turns off radio activation signal
        }
      // Code to turn off radio if end of countdown is reached
      if (sleep_timer == 0) {
        digitalWrite(sleep_out, LOW);   // Turns off radio activation signal
      }   // End of sleep timer activation cycle
}

////////////////////////////////////////////////////////////////////////////////////
  // ALARM-SETTING sub loop 
  // with fast/slow incrementing. Executes if set-alarm button is held pressed (and pin grounded).
  if (digitalRead(set_alarm_in) == LOW) {
    int displayAlarm = alm_hrs * 100 + alm_mins;
    //Push out to the display the new values using the print->pad->write order
    //Use the order print->pad->write.
    clockDisplay.print(displayAlarm, DEC);

    // Zero padding where: 0 is tens hours, 1 is 1s hours, 2 is colon, 3 is tens minutes, 4 is 1s minutes
    if (TIME_24_HOUR) {
      padHHmm(alm_hrs, alm_mins);
    }

    // Set-Alarm fast or slow increment code here:
    if ((digitalRead(fast_set_in) == LOW) ^ (digitalRead(slow_set_in) == LOW)) {
      SpinHHmm(alm_hrs, alm_mins);
    }

    //Push out to the display the new values that were set above.
    clockDisplay.writeDisplay();    
  }
  
  // Loop code is finished. It will jump back to the start of the loop
  // function again!
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////Function Definitions////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void padHHmm (int hrs, int mins) // Pads the display when zeros should appear
{
      if (hrs == 0) {
        // Pad 00 at minutes past midnight
        clockDisplay.writeDigitNum(0, 0);
        clockDisplay.writeDigitNum(1, 0);
      }
      if (hrs < 10) {
        // Pad 0 for hrs less than 10
        clockDisplay.writeDigitNum(0, 0);
      }
      if (mins < 10) {
        // Pad 0 for mins less than 10
        clockDisplay.writeDigitNum(3, 0);
      }  
}


void SpinHHmm (int &hrs, int &mins) //References variables to up/down/wrap them
{   
  if (digitalRead(alarm_off_in) == LOW) { // If alarm-off button held pressed
  //Spin backwards 
      mins -= 1;
      delay(10);
      if (digitalRead(slow_set_in) == LOW) {
        delay(200);
      }
      // If the minutes go below 0 then the hour should decrease and
      // the minutes should wrap back to 59.
      if (mins < 0) {
        mins = 59;
        hrs -= 1;
        // Decrement the hour and wrap back to 23 if it goes below 0.
        if (hrs < 0) {
          hrs = 23;
        }
      }
  }
  else {
   //Spin forwards 
    mins += 1;
     delay(10);
     if (digitalRead(slow_set_in) == LOW) {
       delay(200);
     }
     // If the minutes go above 59 then the hour should increase and
     // the minutes should wrap back to 0.
     if (mins > 59) {
       mins = 0;
       hrs += 1;
       // Increment the hour and wrap back to 0 if it goes above 23 (i.e. past midnight).
       if (hrs > 23) {
         hrs = 0;
        }
      } 
  }
}







