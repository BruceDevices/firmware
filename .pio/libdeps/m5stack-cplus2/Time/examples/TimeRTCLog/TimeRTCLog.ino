/*
 * TimeRTCLogger.ino
 * example code illustrating adding and subtracting Time.
 *
 * this sketch logs pin state change events
 * the time of the event and time since the previous event is calculated and sent to the serial port.
 */

#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t

const int nbrInputPins  = 6;             // monitor 6 digital pins
const int inputPins[nbrInputPins] = {2,3,4,5,6,7};  // pins to monitor
boolean state[nbrInputPins] ;            // the state of the monitored pins
time_t  prevEventTime[nbrInputPins] ;    // the time of the previous event

void setup()  {
  Serial.begin(9600);
  setSyncProvider(RTC.get);   // the function to sync the time from the RTC
  for (int i=0; i < nbrInputPins; i++) {
     pinMode( inputPins[i], INPUT);
     // uncomment these lines if pull-up resistors are wanted
     // pinMode( inputPins[i], INPUT_PULLUP);
     // state[i] = HIGH;
  }
}

void loop()
{
   for (int i=0; i < nbrInputPins; i++) {
     boolean val = digitalRead(inputPins[i]);
     if (val != state[i]) {
        time_t duration = 0; // the time since the previous event
        state[i] = val;
        time_t timeNow = now();
        if (prevEventTime[i] > 0) {
           // if this was not the first state change, calculate the time from the previous change
           duration = timeNow - prevEventTime[i];
        }
        logEvent(inputPins[i], val, timeNow, duration );  // log the event
        prevEventTime[i] = timeNow;                       // store the time for this event
     }
   }
}

void logEvent( int pin, boolean state, time_t timeNow, time_t duration)
{
   Serial.print("Pin ");
   Serial.print(pin);
   if (state == HIGH) {
      Serial.print(" went High at ");
   } else {
     Serial.print(" went  Low at ");
   }
   showTime(timeNow);
   if (duration > 0) {
     // only display duration if greater than 0
     Serial.print(", Duration was ");
     showDuration(duration);
   }
   Serial.println();
}


void showTime(time_t t)
{
  // display the given time
  Serial.print(hour(t));
  printDigits(minute(t));
  printDigits(second(t));
  Serial.print(" ");
  Serial.print(day(t));
  Serial.print(" ");
  Serial.print(month(t));
  Serial.print(" ");
  Serial.print(year(t));
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void showDuration(time_t duration)
{
  // prints the duration in days, hours, minutes and seconds
  if (duration >= SECS_PER_DAY) {
     Serial.print(duration / SECS_PER_DAY);
     Serial.print(" day(s) ");
     duration = duration % SECS_PER_DAY;
  }
  if (duration >= SECS_PER_HOUR) {
     Serial.print(duration / SECS_PER_HOUR);
     Serial.print(" hour(s) ");
     duration = duration % SECS_PER_HOUR;
  }
  if (duration >= SECS_PER_MIN) {
     Serial.print(duration / SECS_PER_MIN);
     Serial.print(" minute(s) ");
     duration = duration % SECS_PER_MIN;
  }
  Serial.print(duration);
  Serial.print(" second(s) ");
}

