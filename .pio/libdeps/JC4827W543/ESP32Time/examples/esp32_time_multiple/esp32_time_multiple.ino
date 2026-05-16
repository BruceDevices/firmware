/*
   MIT License

  Copyright (c) 2021 Felix Biego

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <ESP32Time.h>

ESP32Time rtc;
ESP32Time rtc1(-3600);  // offset GMT-1
ESP32Time rtc2(7200);   // offset GMT+2

void setup() {
  Serial.begin(115200);
  rtc.setTime(30, 24, 15, 17, 1, 2021);  // 17th Jan 2021 15:24:30
  //rtc1.setTime(1609459200);  // 1st Jan 2021 00:00:00
  // time can be set on one instance
  // no need for rtc1.setTime() or rtc2.setTime()
  

}

void loop() {
//  Serial.println(rtc.getTime());          //  (String) 15:24:38
//  Serial.println(rtc.getDate());          //  (String) Sun, Jan 17 2021
//  Serial.println(rtc.getDate(true));      //  (String) Sunday, January 17 2021
//  Serial.println(rtc.getDateTime());      //  (String) Sun, Jan 17 2021 15:24:38
//  Serial.println(rtc.getDateTime(true));  //  (String) Sunday, January 17 2021 15:24:38
//  Serial.println(rtc.getTimeDate());      //  (String) 15:24:38 Sun, Jan 17 2021
//  Serial.println(rtc.getTimeDate(true));  //  (String) 15:24:38 Sunday, January 17 2021
//
//  Serial.println(rtc.getMicros());        //  (unsigned long)    723546
//  Serial.println(rtc.getMillis());        //  (unsigned long)    723
//  Serial.println(rtc.getEpoch());         //  (unsigned long)    1609459200
//  Serial.println(rtc.getSecond());        //  (int)     38    (0-59)
//  Serial.println(rtc.getMinute());        //  (int)     24    (0-59)
//  Serial.println(rtc.getHour());          //  (int)     3     (1-12)
//  Serial.println(rtc.getHour(true));      //  (int)     15    (0-23)
//  Serial.println(rtc.getAmPm());          //  (String)  pm
//  Serial.println(rtc.getAmPm(true));      //  (String)  PM
//  Serial.println(rtc.getDay());           //  (int)     17    (1-31)
//  Serial.println(rtc.getDayofWeek());     //  (int)     0     (0-6)
//  Serial.println(rtc.getDayofYear());     //  (int)     16    (0-365)
//  Serial.println(rtc.getMonth());         //  (int)     0     (0-11)
//  Serial.println(rtc.getYear());          //  (int)     2021

  Serial.println(rtc.getTime("RTC0: %A, %B %d %Y %H:%M:%S"));   // (String) returns time with specified format 
  Serial.println(rtc1.getTime("RTC1: %A, %B %d %Y %H:%M:%S"));   // (String) returns time with specified format 
  Serial.println(rtc2.getTime("RTC2: %A, %B %d %Y %H:%M:%S"));   // (String) returns time with specified format 
  
  // formating options  http://www.cplusplus.com/reference/ctime/strftime/

  Serial.println(rtc.getEpoch());         //  (unsigned long)    
  Serial.println(rtc1.getEpoch());         //  (unsigned long)   
  Serial.println(rtc2.getEpoch());         //  (unsigned long)    

  Serial.println(rtc.getLocalEpoch());         //  (unsigned long) epoch without offset, same for all instances
  delay(1000);
}
