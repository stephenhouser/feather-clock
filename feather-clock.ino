// Adafruit feather 32u4 Digital Clock
// using a seven segment display and DS3231 real-time clock.
//
// Adapted by Stephen Houser for himself
// Written by Tony DiCola for Adafruit Industries.
// Released under a MIT license: https://opensource.org/licenses/MIT

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <RTClib.h>
#include "Adafruit_LEDBackpack.h"

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      false

// I2C address of the display.  Stick with the default address of 0x70
// unless you've changed the address jumpers on the back of the display.
#define DISPLAY_ADDRESS   0x70

// Create display and DS1307 objects.  These are global variables that
// can be accessed from both the setup and loop function below.
Adafruit_7segment clockDisplay = Adafruit_7segment();
RTC_DS3231 rtc;

// Keep track of the hours, minutes, seconds displayed by the clock.
// Start off at 0:00:00 as a signal that the time should be read from
// the DS3231 to initialize it.
int hours = 0;
int minutes = 0;
int seconds = 0;

static const uint8_t numbertable[] = {
	0x3F, /* 0 */
	0x06, /* 1 */
	0x5B, /* 2 */
	0x4F, /* 3 */
	0x66, /* 4 */
	0x6D, /* 5 */
	0x7D, /* 6 */
	0x07, /* 7 */
	0x7F, /* 8 */
	0x6F, /* 9 */
};

uint8_t digit_encode(int digit) {
  return numbertable[digit % 10];
}

uint8_t digits[5];
char dot_animation[] = {B00000, B10000, B01000, B00010, B00001, 
                        B11011, B00001, B00010, B01000, B10000};
int dot_animation_step = 0;

// Remember if the colon was drawn on the display so it can be blinked
// on and off every second.
bool blinkColon = false;

void setup() {
  // Setup Serial port to print debug output.
  Serial.begin(9600);
  Serial.println("Clock starting!");

  // Setup the display.
  clockDisplay.begin(DISPLAY_ADDRESS);
  clockDisplay.setBrightness(5);
  for (int d = 0; d < 5; d++) {
      clockDisplay.writeDigitRaw(d, 0);
  }

  // Setup the DS3231 real-time clock.
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

void loop() {
  // Loop function runs over and over again to implement the clock logic.
    
  // Every few minutes get a new time reading from the DS3231.  
  // This helps keep the clock accurate by fixing any drift.  
  if ((minutes % 5) == 0) {
    DateTime now = rtc.now();

    // Print out the time for debug purposes:
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

    // Now set the hours and minutes.
    hours = now.hour();
    minutes = now.minute();
    seconds = now.second();
  }

  // 12 hour clock
  if (hours == 0) {
    hours = 12;
  } else if (hours > 12) {
    hours -= 12;
  } 

  digits[0] = digit_encode(hours / 10);
  digits[1] = digit_encode(hours % 10);
  digits[2] = !digits[2];
  digits[3] = digit_encode(minutes / 10);
  digits[4] = digit_encode(minutes % 10);
  
  dot_animation_step = (dot_animation_step + 1) % (sizeof(dot_animation) / sizeof(char));

  // Digits; 2 is the colon
  // 0 1 (2) 3 4
  for (int d = 0; d < 5; d++) {
    if (d == 2) {
      clockDisplay.drawColon(digits[d]);
    } else {
      uint8_t encoded_digit = digits[d];
      if (d == 0 && (hours / 10) == 0) {
        encoded_digit = 0;
      }

      bool show_dot = dot_animation[dot_animation_step] & (B10000 >> d);
      uint8_t raw_digit = encoded_digit | (show_dot ? B10000000 : 0);
      clockDisplay.writeDigitRaw(d, raw_digit);
    }
  }

  // Now push out to the display the new values that were set above.
  clockDisplay.writeDisplay();

  // Pause for a second for time to elapse.  This value is in milliseconds
  // so 1000 milliseconds = 1 second.
  delay(1000);
  
  // If the seconds go above 59 then the minutes should increase and
  // the seconds should wrap back to 0.
  // Now increase the seconds by one.
  seconds += 1;
  if (seconds > 59) {
    seconds = 0;
    minutes += 1;
    // When minutes go above 59 then the hour should increase and
    // the minutes should wrap back to 0.
    if (minutes > 59) {
      minutes = 0;
      hours += 1;
      if (hours > 23) {
        hours = 0;
      }
    }
  }
}
