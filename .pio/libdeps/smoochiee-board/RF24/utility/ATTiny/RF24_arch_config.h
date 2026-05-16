/*
 TMRh20 2015
 ATTiny Configuration File
*/

#ifndef RF24_UTILITY_ATTINY_RF24_ARCH_CONFIG_H_
#define RF24_UTILITY_ATTINY_RF24_ARCH_CONFIG_H_

/*** USER DEFINES:  ***/
//#define FAILURE_HANDLING
//#define MINIMAL
/**********************/

#define rf24_max(a, b) (a > b ? a : b)
#define rf24_min(a, b) (a < b ? a : b)

#if ARDUINO < 100
    #include <WProgram.h>
#else
    #include <Arduino.h>
#endif

#include <stddef.h>

#include <SPI.h>

#define _SPI SPI
typedef uint8_t rf24_gpio_pin_t;
#define RF24_PIN_INVALID 0xFF

#if !defined(RF24_CSN_SETTLE_LOW_DELAY)
    #define RF24_CSN_SETTLE_LOW_DELAY 11
#endif

#if !defined(RF24_CSN_SETTLE_HIGH_DELAY)
    #define RF24_CSN_SETTLE_HIGH_DELAY 100
#endif

#ifdef RF24_DEBUG
    #define IF_RF24_DEBUG(x) ({ x; })
#else
    #define IF_RF24_DEBUG(x)

    #if defined(RF24_TINY)
        #define printf_P(...)
    #endif
#endif // !defined(RF24_DEBUG)

#include <avr/pgmspace.h>

#define PRIPSTR "%S"

#endif // RF24_UTILITY_ATTINY_RF24_ARCH_CONFIG_H_
