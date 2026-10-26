/*
  rtl_433_ESP - 433.92 MHz protocols library for ESP32

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with library. If not, see <http://www.gnu.org/licenses/>


  Project Structure

  rtl_433_ESP - Main Class
  decoder.cpp - Wrapper and interface for the rtl_433 classes
  receiver.cpp - Wrapper and interface for RadioLib
  rtl_433 - subset of rtl_433 package

*/

#ifndef _LOG_H_
#define _LOG_H_

#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7
#define LOG_TRACE   8
#define LOG_VERBOSE 9

#define LOG_STACK 255

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
unsigned long rtl_433_millis(void);
#ifdef __cplusplus
}
#endif

// ArduinoLog levels used by the example application. Defining them here keeps
// LOG_LEVEL usable in library translation units that do not include ArduinoLog.
#ifndef LOG_LEVEL_SILENT
#  define LOG_LEVEL_SILENT  0
#  define LOG_LEVEL_FATAL   1
#  define LOG_LEVEL_ERROR   2
#  define LOG_LEVEL_WARNING 3
#  define LOG_LEVEL_NOTICE  4
#  define LOG_LEVEL_TRACE   5
#  define LOG_LEVEL_VERBOSE 6
#endif

// Translate ArduinoLog's 0-6 scale to the syslog-style priorities above. If
// LOG_LEVEL is omitted, retain the historical behaviour and print all levels.
#ifndef LOG_LEVEL
#  define RTL_433_LOG_PRIORITY LOG_VERBOSE
#elif LOG_LEVEL == LOG_LEVEL_SILENT
#  define RTL_433_LOG_PRIORITY (-1)
#elif LOG_LEVEL == LOG_LEVEL_FATAL
#  define RTL_433_LOG_PRIORITY LOG_CRIT
#elif LOG_LEVEL == LOG_LEVEL_ERROR
#  define RTL_433_LOG_PRIORITY LOG_ERR
#elif LOG_LEVEL == LOG_LEVEL_WARNING
#  define RTL_433_LOG_PRIORITY LOG_WARNING
#elif LOG_LEVEL == LOG_LEVEL_NOTICE
#  define RTL_433_LOG_PRIORITY LOG_NOTICE
#elif LOG_LEVEL == LOG_LEVEL_TRACE
#  define RTL_433_LOG_PRIORITY LOG_TRACE
#else
#  define RTL_433_LOG_PRIORITY LOG_VERBOSE
#endif

#define logprintf(prio, args...)                                  \
  do {                                                            \
    if ((prio) <= RTL_433_LOG_PRIORITY) {                         \
      printf("rtl_433_ESP(%d): %lu, ", (prio), rtl_433_millis()); \
      printf(args);                                               \
    }                                                             \
  } while (0)
#define logprintfLn(prio, args...)                                \
  do {                                                            \
    if ((prio) <= RTL_433_LOG_PRIORITY) {                         \
      printf("rtl_433_ESP(%d): %lu, ", (prio), rtl_433_millis()); \
      printf(args);                                               \
      printf("\n");                                               \
    }                                                             \
  } while (0)
#define alogprintf(prio, args...)         \
  do {                                    \
    if ((prio) <= RTL_433_LOG_PRIORITY) { \
      printf(args);                       \
    }                                     \
  } while (0)
#define alogprintfLn(prio, args...)       \
  do {                                    \
    if ((prio) <= RTL_433_LOG_PRIORITY) { \
      printf(args);                       \
      printf("\n");                       \
    }                                     \
  } while (0)

#endif
