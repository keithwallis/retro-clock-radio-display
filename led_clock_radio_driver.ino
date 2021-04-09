#include <LiquidCrystal_I2C.h> // Seems to use Adafruit version
#include <Boards.h>
#include <Firmata.h>
#include <FirmataConstants.h>
#include <FirmataDefines.h>
#include <FirmataMarshaller.h>
#include <FirmataParser.h>
#include <elapsedMillis.h>  // Needs the elapsedMillis library
#include <Wire.h>
#include <RTClib.h>
#include <Time.h>
#include <TimeLib.h> //Needs the Time library
#include <TimeAlarms.h> // Needs the TimeAlarms library
#include <EEPROM.h>


//FOR ARDUINO NANO NOTE THE FOLLOWING//
//Under "Tools" use the following settings:
//Board: Arduino Nano
//Processor: ATmega328P (Old Bootloader)
//Programmer: Atmel STK500 development board


// Must have the Adafruit RTClib library installed too!  See:
//   https://github.com/adafruit/RTClib
//
// Core features are designed to work with DS1307 real-time clock breakout:
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


// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      true

// I2C address of the display.  Stick with the default address of 0x70
// unless you've changed the address jumpers on the back of the display.
#define DISPLAY_ADDRESS   0x70


// Create display and DS1307 objects.  These are global variables that
// can be accessed from both the setup and loop function below.
/*Adafruit_7segment clockDisplay = Adafruit_7segment();*/
LiquidCrystal_I2C lcd(0x27, 16, 2);
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
int alm_hrs = 0; /* Initialise alarm hours default */
int alm_mins = 0; /* Initialise alarm mins default */



const int sleep_timer_max = 60; /* Set sleep timer minutes default countdown start */
const int alarm_timer_limit = 600; /* Duration of alarm/signal in sec. Must be > 60 to ensure the alarm works beyond hr/min parity of time trigger.*/
int sleep_timer = sleep_timer_max; /* Initialise sleep timer to default maximum */
int alarm_timer = alarm_timer_limit; /* Initialise alarm countdown to default maximum */

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

// Default setting of signal-active flag.
bool AlmCountdownRunning = false;

// Create a bit matrix for custom characters formed from the 5x8 array
byte bell[8] = {
  B00000,
  B00100,
  B01110,
  B01110,
  B11111,
  B11111,
  B00100,
};

byte moon[8] = {
  B00110,
  B01100,
  B11000,
  B11000,
  B11000,
  B01100,
  B00110,
};

byte battery[8] = {
  B00100,
  B11111,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111,
};


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

  // Setup the static display texts or symbols. 
  lcd.begin(); 
  //lcd.setCursor(0,0); // Position the static Time label
  //lcd.print("Time"); // Write the static Time label
  lcd.setCursor(12,0); // Position the static sleep symbol
  lcd.write(byte(1));  // Write the static sleep symbol   
 

 // Create a custom character, acc. to setup definition, of the 0 to 7 which are supported;   
  lcd.createChar(0, bell);
  lcd.createChar(1, moon);
  lcd.createChar(2, battery);



// Reset in case out-of-range values are loaded from EEPROM - i.e. virgin board case
  if ((EEPROM.read(0) > 24) ^ (EEPROM.read(0) < 0)) {EEPROM.write(0, 0);}
  if ((EEPROM.read(1) > 59) ^ (EEPROM.read(1) < 0)) {EEPROM.write(1, 0);}


  // Get the stored alarm time from EEPROM memory on power-on.
  // Can only do this within a function, which is "setup()" in this case
  alm_hrs = EEPROM.read(0); 
  alm_mins = EEPROM.read(1); 

  // Setup the DS1307 real-time clock and start it.
  rtc.begin();
  // This line sets the DS1307 time to the given date and time
//rtc.adjust(DateTime(2019, 2, 27, 23, 49, 12));
  // Set the DS1307 clock if not set before, i.e. is not running.
  bool ClockStopped = !rtc.isrunning();//<----"isrunning" seems unreliable; returns true, even if rtc battery was removed.
  // Alternatively you can force the clock to be set again by
  // uncommenting this line:
  //ClockStopped = true;
  if (ClockStopped) { 
    Serial.println("Setting DS1307 time!");
    // This line sets the DS1307 time to the exact date and time the
    // sketch was compiled:
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // <--Command writes compile time to RTC which RTC defaults to after every restart! 
                                                    //But only if ClockStopped is true which is not the case on restart.

  }
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////Main Loop///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
void loop() {
  // Loop function runs over and over again to implement the clock logic.


  ////////////////RTC Clock Mode///////////////////////////////
  // Check every minute and get a new time reading
  // from the DS1307.  This helps keep the clock accurate by fixing
  // any drift.
  if( rtc.isrunning()){
    if (seconds == 0) {  //Update on the minute
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

  /////////////////////////////////////////////////////////////////////
  ///////////////One-Second Cycle commands/////////////////////////////
  /////////////////////////////////////////////////////////////////////
  // Carry out activities in one-second cycle like tick the clock
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

  //Reset the RTC battery fail symbol (which will happen every second)
    if( rtc.isrunning()){ 
     lcd.setCursor(15,0);
     // Blank the battery symbol//
     lcd.print(" ");
    }

//////////////////////Alarm Triggering and Countdown/////////////  


  // Alarm triggering sequence senses for given hour/minute if alarm
  // is armed and trips the alarm countdown.
  // Note: If the alarm triggers, a switch on the radio decides if it actually sounds.
  
  if ((hours == alm_hrs) && (minutes == alm_mins) && (alarm_timer > 60)) { // If time matches and countdown running
    digitalWrite(signal_out, HIGH); // Activate the signal   
  }

  if (alarm_timer <= 0) { // Deactivate signal/alarm and reset countdown when countdown reaches zero
    digitalWrite(signal_out, LOW);
    alarm_timer = alarm_timer_limit; // Reset countdown to max.
  }    
 // Otherwise, i.e. while countdown is running, decrement countdown
  else {  
    alarm_timer -= 1; 
  }

////////////////////End of Alarm Trigger and Countdown///////////////////////

  
  // Sleep timer countdown per minute if the sleep function is active //
  if (digitalRead(sleep_in) == LOW){
      // Decrement the sleep timer every minute as long as sleep button is active. 
    if ((seconds == 0) && (sleep_timer > 0)) {
      sleep_timer -= 1;
    }

  }

////////////////////Store alarm time if changed///////////////////////
//Every second (defined by this "if" loop) check if alarm hrs/mins differs from
//EEPROM values and if yes, write the new values to EEPROM, but only in alarm set mode
    if (digitalRead(set_alarm_in) == LOW) {
      if (alm_hrs != EEPROM.read(0)) {EEPROM.write(0, alm_hrs);} 
      if (alm_mins != EEPROM.read(1)) {EEPROM.write(1, alm_mins);}
    }


  // Following line resets the time elapsed counter
  timeElapsed = 0;
    
  }  // End of the 1-second clock and activity  cycle.
 

//////////////////////Time Display///////////////////////////  
  //Define strings for the displayed variables and pad with zeros
  String s, m, h, dy, mo, yr;
  if (seconds < 10) { s = "0" + String(seconds); } else { s = String(seconds); }
  if (minutes < 10) { m = "0" + String(minutes); } else { m = String(minutes); }
  if (hours < 10) { h = "0" + String(hours); } else { h = String(hours); }  
  if (days < 10) { dy = "0" + String(days); } else { dy = String(days); }
  if (months < 10) { mo = "0" + String(months); } else { mo = String(months); }
  if (years > 2009) { yr = String(years-2000); } else { yr = "0" + String(years-2000); }  
  
  // lcd.clear();
  lcd.setCursor(5,0);
  // Print the time with or without colon //
  if (blinkColon) { lcd.print(h + ":" + m);} else {lcd.print(h + " " + m);}

  // Print the date
  lcd.setCursor(8,1);
  lcd.print(dy + "/" + mo + "/" + yr); 

//////////////////////Alarm Display///////////////////////////  
 //Define strings with zero-padding
  String a_m, a_h;
  if (alm_mins < 10) { a_m = "0" + String(alm_mins); } else { a_m = String(alm_mins); }
  if (alm_hrs < 10) { a_h = "0" + String(alm_hrs); } else { a_h = String(alm_hrs); }    

  // lcd.clear();
  lcd.setCursor(1,1);
  // Print the alarm time with no colon//
  lcd.print(a_h + ":" + a_m);


//////////////////////Time Set///////////////////////////
if ((digitalRead(sleep_in) == HIGH) && (digitalRead(set_time_in)) == LOW) { //Display the time only if the sleep display is off
  digitalWrite(sleep_out, LOW); //Make sure sleep output is off when sleep not activated


   // Time set: Fast or slow
   if ((digitalRead(fast_set_in) == LOW) ^ (digitalRead(slow_set_in) == LOW)) {
     SpinHHmm(hours, minutes);
     //Set clock values individually to the rtc unit - this will overwrite some values with values set above.
     rtc.adjust(DateTime(years, months, days, hours, minutes, seconds));
   }

   // Date set: fixed rate
   if ((digitalRead(fast_set_in) == LOW) && (digitalRead(slow_set_in) == LOW)) {
     SpinDate(years, months, days);
     //Set clock values individually to the rtc unit - this will overwrite some values with values set above.
     rtc.adjust(DateTime(years, months, days, hours, minutes, seconds));
   }


}

//////////////////////Alarm Set///////////////////////////
//Set fast or slow if "Signal" button is depressed to allow alarm setting

  //Blink the display symbol here:
  if (digitalRead(set_alarm_in) == LOW) {
    if (blinkColon) { 
      lcd.setCursor(0,1);
      // Write the bell symbol//
      lcd.write(byte(0));
      } 
    else {
      lcd.setCursor(0,1);
      // Blank the bell symbol//
      lcd.print(" ");
    }

    // Set-Alarm fast or slow increment code here:
    if ((digitalRead(fast_set_in) == LOW) ^ (digitalRead(slow_set_in) == LOW)) {
      SpinHHmm(alm_hrs, alm_mins);
      alarm_timer = alarm_timer_limit; //Reset alarm timer countdown to max.
    } 
  }
  else {
    lcd.setCursor(0,1);
    // Write the bell symbol if not in the alarm-set cycle
    lcd.write(byte(0));
  }
  // End Time/Alarm display set


   ////////////////////Signal Active Indicator//////////////////////////////////////
   // Light up indicator an indicator to show alarm/signal is active in time display mode
   if (AlmCountdownRunning) {
     // Display alarm-sounding alert using exclamation mark after the alarm time
         if (blinkColon) {
          lcd.setCursor(6,1);
          // Blank the alert symbol//
          lcd.print("!");
         }
          else {
            // Blank the alert symbol//
            lcd.setCursor(6,1);
            // Blank the alert symbol//
            lcd.print(" ");
          }
   } 
   else {
     lcd.setCursor(6,1);
     // Blank the alert symbol//
     lcd.print(" ");
   }

   /////////////////RTC Battery Fail Indicator/////////////////////////////////////
if(!rtc.isrunning()){
    Serial.println("Warning: RTC not running!");
    // Light up battery fail indicator to show rtc is not running
    // Uses the custom character made from a byte array
    lcd.setCursor(15,0);
    // Write the battery symbol//
    lcd.write(byte(2));
 }

 //////////////Set Brightness////////////////////
  if (digitalRead(dim_in) == LOW) { //Dim the display acc. to dim/bright
    // Set the backlight off/on. If variable (1-255) is not supported then 0 = off, >0 = on.
    // Note: This assumes a resistor is used to bridge Vss to the LED to ensure it is on and dim when Backlight is off.
    lcd.setBacklight(0);
  }
  else {
    lcd.setBacklight(255);
  } // End Set Brightness code


//////////////// Turn Off Alarm////////////////////////////////
  if (digitalRead(alarm_off_in) == LOW) { // If alarm-off button pressed, and as long as it is held down
    digitalWrite(signal_out, LOW); // Turn off signal/alarm
      // If time matches to the minute, set the countdown to 60 sec. to prevent triggering during that minute. Otherwise reset it.
      if ((hours == alm_hrs) && (minutes == alm_mins)) {
        alarm_timer = 60; 
      }
      else {
      alarm_timer = alarm_timer_limit; // Reset alarm countdown.
      } 
  }
  
//////////////// SLEEP setting and display function////////////////////////////////
  if ((digitalRead(sleep_in) == LOW) && (digitalRead(set_time_in)) == HIGH) { //Display the sleeptime only if the sleep display is on
   
    //Display the sleep symbol and the timer value
    lcd.setCursor(12,0);  
    lcd.write(byte(1)); // Write the symbol
    //Cast the sleep value as string, pad with zero and print 
    String slp_mins = String(sleep_timer);  //Cast the sleep value as string, pad with zero and print
    if (sleep_timer < 10) { slp_mins = "0" + String(sleep_timer); }
    lcd.setCursor(13,0);
    lcd.print(slp_mins); 
 
    //Here is the sleep setting countdown code if the fast/slow buttons are pressed
      if ((digitalRead(fast_set_in) == LOW) ^ (digitalRead(slow_set_in) == LOW)) { //Detect button presses
        sleep_timer -= 1;
        delay(50);
        if (digitalRead(slow_set_in) == LOW) {
          delay(500);
        }
        // Wrap sleep timer back to maximum.
        if (sleep_timer < 00) {
          sleep_timer = sleep_timer_max;
        } 
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
    else { // i.e. if the sleep setting button is not pressed
    // Blank out the sleep display
    lcd.setCursor(12,0);
    lcd.print("   "); 
   }  
} // End of main loop


///////////////////////////////////////////////////////////////////////////////
///////////////////////Function Definitions////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


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
  alarm_timer = alarm_timer_limit; //Reset alarm timer countdown to max. 
}


void SpinDate (int &yrs, int &mths, int &dys) //References variables to up/down/wrap them
  {
  //Spin the date if the dim button is held low
  if (digitalRead(dim_in) == LOW) { 
    if (digitalRead(alarm_off_in) == LOW) { // If alarm-off button held pressed
    //Spin backwards 
        dys -= 1;
        delay(200);
        // If the days go below 0 then the month should decrease and
        // the days should wrap back to 30.
        if (dys < 0) {
          dys = 30;
          mths -= 1;
          // Decrement the hour and wrap back to 23 if it goes below 0.
          if (mths < 0) {
            mths = 12;
          }
        }
    }
    else {
     //Spin forwards 
      dys += 1;
       delay(200);
       // If the days go above 30 then the month should increase and
       // the days should wrap back to 0.
       if (dys > 30) {
         dys = 0;
         mths += 1;
         // Increment the hour and wrap back to 0 if it goes above 23 (i.e. past midnight).
         if (mths > 12) {
           mths = 0;
          }
        } 
      }
    }
    //Otherwise - if the dim button is not held low - spin the year    
    else {
      // Increment the year up to 2100 and loop back 
      //Spin forwards 
      yrs += 1;
      delay(200);
      // If the days go above 30 then the month should increase and
      // the days should wrap back to 0.
      if (yrs > 2099) {
        yrs = 2000;
      }
    }      
  } //End of spinDate function
