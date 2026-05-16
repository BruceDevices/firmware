# ESP32Time
An Arduino library for setting and retrieving internal RTC time on ESP32 boards

[![arduino-library-badge](https://www.ardu-badge.com/badge/ESP32Time.svg?)](https://www.arduinolibraries.info/libraries/esp32-time)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/fbiego/library/ESP32Time.svg)](https://registry.platformio.org/libraries/fbiego/ESP32Time)

## Functions

```
ESP32Time rtc(offset); // create an instance with a specifed offset in seconds
rtc.offset;	// get or modify the current offset
setTime(30, 24, 15, 17, 1, 2021);  // 17th Jan 2021 15:24:30
setTime(1609459200);  // 1st Jan 2021 00:00:00
setTimeStruct(time);	// set with time struct

getTime()          //  (String) 15:24:38
getDate()          //  (String) Sun, Jan 17 2021
getDate(true)      //  (String) Sunday, January 17 2021
getDateTime()      //  (String) Sun, Jan 17 2021 15:24:38
getDateTime(true)  //  (String) Sunday, January 17 2021 15:24:38
getTimeDate()      //  (String) 15:24:38 Sun, Jan 17 2021
getTimeDate(true)  //  (String) 15:24:38 Sunday, January 17 2021

getMicros()        //  (unsigned long)    723546
getMillis()        //  (unsigned long)    723
getEpoch()         //  (unsigned long)    1609459200
getLocalEpoch()    //  (unsigned long)    1609459200 // local epoch without offset
getSecond()        //  (int)     38    (0-59)
getMinute()        //  (int)     24    (0-59)
getHour()          //  (int)     3     (1-12)
getHour(true)      //  (int)     15    (0-23)
getAmPm()          //  (String)  PM
getAmPm(true)     //  (String)  pm
getDay()           //  (int)     17    (1-31)
getDayofWeek()     //  (int)     0     (0-6)
getDayofYear()     //  (int)     16    (0-365)
getMonth()         //  (int)     0     (0-11)
getYear()          //  (int)     2021

getTime("%A, %B %d %Y %H:%M:%S")   // (String) returns time with specified format 
```
[`Formatting options`](http://www.cplusplus.com/reference/ctime/strftime/)
