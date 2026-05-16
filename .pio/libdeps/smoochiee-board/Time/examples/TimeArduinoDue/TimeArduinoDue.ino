/*
 * TimeRTC.pde
 * example code illustrating Time library with Real Time Clock.
 *
 * This example requires Markus Lange's Arduino Due RTC Library 
 * https://github.com/MarkusLange/Arduino-Due-RTC-Library
 */

#include <TimeLib.h>
#include <rtc_clock.h>

// Select the Slowclock source
//RTC_clock rtc_clock(RC);
RTC_clock rtc_clock(XTAL);

void setup()  {
  Serial.begin(9600);
  rtc_clock.init();
  if (rtc_clock.date_already_set() == 0) {
    // Unfortunately, the Arduino Due hardware does not seem to
    // be designed to maintain the RTC clock state when the
    // board resets.  Markus described it thusly: "Uhh the Due
    // does reset with the NRSTB pin.  This resets the full chip
    // with all backup regions including RTC, RTT and SC.  Only
    // if the reset is done with the NRST pin will these regions
    // stay with their old values."
    rtc_clock.set_time(__TIME__);
    rtc_clock.set_date(__DATE__);
    // However, this might work on other unofficial SAM3X boards
    // with different reset circuitry than Arduino Due?
  }
  setSyncProvider(getArduinoDueTime);
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time");      
}

time_t getArduinoDueTime()
{
  return rtc_clock.unixtime();
}

void loop()
{
   digitalClockDisplay();
   delay(1000);
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

