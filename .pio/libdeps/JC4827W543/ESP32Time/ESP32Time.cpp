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

#include "ESP32Time.h"
#include "time.h"
#include <sys/time.h>

#ifdef RTC_DATA_ATTR
RTC_DATA_ATTR static bool overflow;
#else
static bool overflow;
#endif


/*!
    @brief  Constructor for ESP32Time
*/
ESP32Time::ESP32Time(){
}

/*!
    @brief  Constructor for ESP32Time
	@param  offest
            gmt offset in seconds
*/
ESP32Time::ESP32Time(long offset){
	this->offset = offset;
}

/*!
    @brief  set the internal RTC time
    @param  sc
            second (0-59)
    @param  mn
            minute (0-59)
    @param  hr
            hour of day (0-23)
    @param  dy
            day of month (1-31)
    @param  mt
            month (1-12)
    @param  yr
            year ie 2021
    @param  ms
            microseconds (optional)
*/
void ESP32Time::setTime(int sc, int mn, int hr, int dy, int mt, int yr, int ms) {
  // seconds, minute, hour, day, month, year $ microseconds(optional)
  // ie setTime(20, 34, 8, 1, 4, 2021) = 8:34:20 1/4/2021
  struct tm t = {0, 0, 0, 0, 0, 0, 0, 0, 0};      // Initalize to all 0's
  t.tm_year = yr - 1900;    // This is year-1900, so 121 = 2021
  t.tm_mon = mt - 1;
  t.tm_mday = dy;
  t.tm_hour = hr;
  t.tm_min = mn;
  t.tm_sec = sc;
  time_t timeSinceEpoch = mktime(&t);
  setTime(timeSinceEpoch, ms);
}

/*!
    @brief  set time from struct
	@param	tm
			time struct
*/
void ESP32Time::setTimeStruct(tm t) { 
	time_t timeSinceEpoch = mktime(&t); 
	setTime(timeSinceEpoch, 0); 
}

/*!
    @brief  set the internal RTC time
    @param  epoch
            epoch time in seconds
    @param  ms
            microseconds (optional)
*/
void ESP32Time::setTime(unsigned long epoch, int ms) {
  struct timeval tv;
  if (epoch > 2082758399){
	  overflow = true;
	  tv.tv_sec = epoch - 2082758399;  // epoch time (seconds)
  } else {
	  overflow = false;
	  tv.tv_sec = epoch;  // epoch time (seconds)
  }
  tv.tv_usec = ms;    // microseconds
  settimeofday(&tv, NULL);
}

/*!
    @brief  get the internal RTC time as a tm struct
*/
tm ESP32Time::getTimeStruct(){
  struct tm timeinfo;
  time_t now;
  time(&now);
  localtime_r(&now, &timeinfo);
  time_t tt = mktime (&timeinfo);
    
  if (overflow){
	  tt += 63071999;
  }
  if (offset > 0){
	tt += (unsigned long) offset;
  } else {
	tt -= (unsigned long) (offset * -1);
  }
  struct tm * tn = localtime(&tt);
  if (overflow){
	  tn->tm_year += 64;
  }
  return *tn;
}

/*!
    @brief  get the time and date as an Arduino String object
    @param  mode
            true = Long date format
			false = Short date format
*/
String ESP32Time::getDateTime(bool mode){
	struct tm timeinfo = getTimeStruct();
	char s[51];
	if (mode)
	{
		strftime(s, 50, "%A, %B %d %Y %H:%M:%S", &timeinfo);
	}
	else
	{
		strftime(s, 50, "%a, %b %d %Y %H:%M:%S", &timeinfo);
	}
	return String(s);
}

/*!
    @brief  get the time and date as an Arduino String object
    @param  mode
            true = Long date format
			false = Short date format
*/
String ESP32Time::getTimeDate(bool mode){
	struct tm timeinfo = getTimeStruct();
	char s[51];
	if (mode)
	{
		strftime(s, 50, "%H:%M:%S %A, %B %d %Y", &timeinfo);
	}
	else
	{
		strftime(s, 50, "%H:%M:%S %a, %b %d %Y", &timeinfo);
	}
	return String(s);
}

/*!
    @brief  get the time as an Arduino String object
*/
String ESP32Time::getTime(){
	struct tm timeinfo = getTimeStruct();
	char s[51];
	strftime(s, 50, "%H:%M:%S", &timeinfo);
	return String(s);
}

/*!
    @brief  get the time as an Arduino String object with the specified format
	@param	format
			time format 
			http://www.cplusplus.com/reference/ctime/strftime/
*/
String ESP32Time::getTime(String format){
	struct tm timeinfo = getTimeStruct();
	char s[128];
	char c[128];
	format.toCharArray(c, 127);
	strftime(s, 127, c, &timeinfo);
	return String(s);
}

/*!
    @brief  get the date as an Arduino String object
    @param  mode
            true = Long date format
			false = Short date format
*/
String ESP32Time::getDate(bool mode){
	struct tm timeinfo = getTimeStruct();
	char s[51];
	if (mode)
	{
		strftime(s, 50, "%A, %B %d %Y", &timeinfo);
	}
	else
	{
		strftime(s, 50, "%a, %b %d %Y", &timeinfo);
	}
	return String(s);
}

/*!
    @brief  get the current milliseconds as unsigned long
*/
unsigned long ESP32Time::getMillis(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec/1000;
}

/*!
    @brief  get the current microseconds as unsigned long
*/
unsigned long ESP32Time::getMicros(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_usec;
}

/*!
    @brief  get the current epoch seconds as unsigned long
*/
unsigned long ESP32Time::getEpoch(){
	struct tm timeinfo = getTimeStruct();
	return mktime(&timeinfo);
}

/*!
    @brief  get the current epoch seconds as unsigned long from the rtc without offset
*/
unsigned long ESP32Time::getLocalEpoch(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long epoch = tv.tv_sec;
	if (overflow){
		epoch += 63071999 + 2019686400;
	}
	return epoch;
}

/*!
    @brief  get the current seconds as int
*/
int ESP32Time::getSecond(){
	struct tm timeinfo = getTimeStruct();
	return timeinfo.tm_sec;
}

/*!
    @brief  get the current minutes as int
*/
int ESP32Time::getMinute(){
	struct tm timeinfo = getTimeStruct();
	return timeinfo.tm_min;
}

/*!
    @brief  get the current hour as int
	@param	mode
			true = 24 hour mode (0-23)
			false = 12 hour mode (1-12)
*/
int ESP32Time::getHour(bool mode){
	struct tm timeinfo = getTimeStruct();
	if (mode)
	{
		return timeinfo.tm_hour;
	}
	else
	{
		int hour = timeinfo.tm_hour;
		if (hour > 12)
		{
			return timeinfo.tm_hour-12;
		}
		else if (hour == 0)
		{
			return 12; // 12 am
		}
		else
		{
			return timeinfo.tm_hour;
		}
		
	}
}

/*!
    @brief  return current hour am or pm
	@param	lowercase
			true = lowercase
			false = uppercase
*/
String ESP32Time::getAmPm(bool lowercase){
	struct tm timeinfo = getTimeStruct();
	if (timeinfo.tm_hour >= 12)
	{
		if (lowercase)
		{
			return "pm";
		}
		else
		{
			return "PM";
		}
	}
	else
	{
		if (lowercase)
		{
			return "am";
		}
		else
		{
			return "AM";
		}
	}
}

/*!
    @brief  get the current day as int (1-31)
*/
int ESP32Time::getDay(){
	struct tm timeinfo = getTimeStruct();
	return timeinfo.tm_mday;
}

/*!
    @brief  get the current day of week as int (0-6)
*/
int ESP32Time::getDayofWeek(){
	struct tm timeinfo = getTimeStruct();
	return timeinfo.tm_wday;
}

/*!
    @brief  get the current day of year as int (0-365)
*/
int ESP32Time::getDayofYear(){
	struct tm timeinfo = getTimeStruct();
	return timeinfo.tm_yday;
}

/*!
    @brief  get the current month as int (0-11)
*/
int ESP32Time::getMonth(){
	struct tm timeinfo = getTimeStruct();
	return timeinfo.tm_mon;
}

/*!
    @brief  get the current year as int
*/
int ESP32Time::getYear(){
	struct tm timeinfo = getTimeStruct();
	return timeinfo.tm_year+1900;
}
