
/*
 Copyright (C)
    2011            J. Coliz <maniacbug@ymail.com>
    2015-2019       TMRh20
    2015            spaniakos <spaniakos@gmail.com>
    2015            nerdralph
    2015            zador-blood-stained
    2016            akatran
    2017-2019       Avamander <avamander@gmail.com>
    2019            IkpeohaGodson
    2021            2bndy5

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

#ifndef RF24_CONFIG_H_
#define RF24_CONFIG_H_

/*** USER DEFINES:    ***/
#define FAILURE_HANDLING
//#define RF24_DEBUG
//#define MINIMAL
//#define SPI_UART    // Requires library from https://github.com/TMRh20/Sketches/tree/master/SPI_UART
//#define SOFTSPI     // Requires library from https://github.com/greiman/DigitalIO

/**
 * User access to internally used delay time (in microseconds) during RF24::powerUp()
 * @warning This default value compensates for all supported hardware. Only adjust this if you
 * know your radio's hardware is, in fact, genuine and reliable.
 */
#if !defined(RF24_POWERUP_DELAY)
    #define RF24_POWERUP_DELAY 5000
#endif

/**********************/
#define rf24_max(a, b) ((a) > (b) ? (a) : (b))
#define rf24_min(a, b) ((a) < (b) ? (a) : (b))

/** @brief The default SPI speed (in Hz) */
#ifndef RF24_SPI_SPEED
    #define RF24_SPI_SPEED 10000000
#endif

//ATXMega
#if defined(__AVR_ATxmega64D3__) || defined(__AVR_ATxmega128D3__) || defined(__AVR_ATxmega192D3__) || defined(__AVR_ATxmega256D3__) || defined(__AVR_ATxmega384D3__)
    // In order to be available both in Windows and Linux this should take presence here.
    #define XMEGA
    #define XMEGA_D3
    #include "utility/ATXMegaD3/RF24_arch_config.h"

// RaspberryPi rp2xxx-based devices (e.g. RPi Pico board)
#elif defined(PICO_BUILD) && !defined(ARDUINO)
    #include "utility/rp2/RF24_arch_config.h"
    #define sprintf_P sprintf

#elif (!defined(ARDUINO)) // Any non-arduino device is handled via configure/Makefile
    // The configure script detects device and copies the correct includes.h file to /utility/includes.h
    // This behavior can be overridden by calling configure with respective parameters
    // The includes.h file defines either RF24_RPi, MRAA, LITTLEWIRE or RF24_SPIDEV and includes the correct RF24_arch_config.h file
    #include "utility/includes.h"

    #ifndef sprintf_P
        #define sprintf_P sprintf
    #endif // sprintf_P

//ATTiny
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny4313__) || defined(__AVR_ATtiny861__) || defined(__AVR_ATtinyX5__) || defined(__AVR_ATtinyX4__) || defined(__AVR_ATtinyX313__) || defined(__AVR_ATtinyX61__)
    #define RF24_TINY
    #include "utility/ATTiny/RF24_arch_config.h"

#elif defined(LITTLEWIRE) //LittleWire
    #include "utility/LittleWire/RF24_arch_config.h"

#elif defined(TEENSYDUINO) //Teensy
    #include "utility/Teensy/RF24_arch_config.h"

#else //Everything else
    #include <Arduino.h>

    #ifdef NUM_DIGITAL_PINS
        #if NUM_DIGITAL_PINS < 255
typedef uint8_t rf24_gpio_pin_t;
            #define RF24_PIN_INVALID 0xFF
        #else
typedef uint16_t rf24_gpio_pin_t;
            #define RF24_PIN_INVALID 0xFFFF
        #endif
    #else
typedef uint16_t rf24_gpio_pin_t;
        #define RF24_PIN_INVALID 0xFFFF
    #endif

    #if defined(ARDUINO) && !defined(__arm__) && !defined(__ARDUINO_X86__)
        #if defined SPI_UART
            #include <SPI_UART.h>
            #define _SPI uspi
        #elif defined(SOFTSPI)
            // change these pins to your liking
            //
            #ifndef SOFT_SPI_MISO_PIN
                #define SOFT_SPI_MISO_PIN 9
            #endif // SOFT_SPI_MISO_PIN

            #ifndef SOFT_SPI_MOSI_PIN
                #define SOFT_SPI_MOSI_PIN 8
            #endif // SOFT_SPI_MOSI_PIN

            #ifndef SOFT_SPI_SCK_PIN
                #define SOFT_SPI_SCK_PIN 7
            #endif // SOFT_SPI_SCK_PIN

const uint8_t SPI_MODE = 0;
            #define _SPI spi

        #elif defined(ARDUINO_SAM_DUE)
            #include <SPI.h>
            #define _SPI SPI

        #else // !defined (SPI_UART) && !defined (SOFTSPI)
            #include <SPI.h>
            #define _SPI SPIClass
            #define RF24_SPI_PTR
        #endif // !defined (SPI_UART) && !defined (SOFTSPI)

    #else // !defined(ARDUINO) || defined (__arm__) || defined (__ARDUINO_X86__)
        // Define _BV for non-Arduino platforms and for Arduino DUE
        #include <stdint.h>
        #include <stdio.h>
        #include <string.h>

        #if defined(__arm__) || defined(__ARDUINO_X86__)
            #if defined(__arm__) && defined(SPI_UART)
                #include <SPI_UART.h>
                #define _SPI uspi

            #else // !defined (__arm__) || !defined (SPI_UART)
                #include <SPI.h>
                #define _SPI SPIClass
                #define RF24_SPI_PTR

            #endif // !defined (__arm__) || !defined (SPI_UART)
        #elif !defined(__arm__) && !defined(__ARDUINO_X86__)
// fallback to unofficially supported Hardware (courtesy of ManiacBug)
extern HardwareSPI SPI;
            #define _SPI HardwareSPI
            #define RF24_SPI_PTR

        #endif // !defined(__arm__) && !defined (__ARDUINO_X86__)

        #ifndef _BV
            #define _BV(x) (1 << (x))
        #endif
    #endif // defined (ARDUINO) && !defined (__arm__) && !defined (__ARDUINO_X86__)

    #ifdef RF24_DEBUG
        #define IF_RF24_DEBUG(x) ({ x; })
    #else
        #define IF_RF24_DEBUG(x)
        #if defined(RF24_TINY)
            #define printf_P(...)
        #endif // defined(RF24_TINY)

    #endif // RF24_DEBUG

    #if defined(__ARDUINO_X86__)
        #define printf_P printf
        #define _BV(bit) (1 << (bit))

    #endif // defined (__ARDUINO_X86__)

    // Progmem is Arduino-specific
    #if defined(ARDUINO_ARCH_ESP8266) || defined(ESP32) || (defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED))
        #include <pgmspace.h>
        #define PRIPSTR "%s"
        #ifndef pgm_read_ptr
            #define pgm_read_ptr(p) (*(void* const*)(p))
        #endif
        // Serial.printf() is no longer defined in the unifying Arduino/ArduinoCore-API repo
        // Serial.printf() is defined if using the arduino-pico/esp32/8266 repo
        #if defined(ARDUINO_ARCH_ESP32) // do not `undef` when using the espressif SDK only
            #undef printf_P             // needed for ESP32 core
        #endif
        #define printf_P Serial.printf
    #elif defined(ARDUINO) && !defined(ESP_PLATFORM) && !defined(__arm__) && !defined(__ARDUINO_X86__) || defined(XMEGA)
        #include <avr/pgmspace.h>
        #define PRIPSTR "%S"

    #else                     // !defined (ARDUINO) || defined (ESP_PLATFORM) || defined (__arm__) || defined (__ARDUINO_X86__) && !defined (XMEGA)
        #if !defined(ARDUINO) // This doesn't work on Arduino DUE
typedef char const char;
        #else                 // Fill in pgm_read_byte that is used
            #if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_SAM_DUE)
                #include <avr/pgmspace.h> // added to ArduinoCore-sam (Due core) in 2013
            #endif

            // Since the official arduino/ArduinoCore-samd repo switched to a unified API in 2016,
            // Serial.printf() is no longer defined in the unifying Arduino/ArduinoCore-API repo
            #if defined(ARDUINO_ARCH_SAMD) && defined(ARDUINO_SAMD_ADAFRUIT)
                // it is defined if using the adafruit/ArduinoCore-samd repo
                #define printf_P Serial.printf
            #endif // defined (ARDUINO_ARCH_SAMD)

            #ifndef pgm_read_byte
                #define pgm_read_byte(addr) (*(const unsigned char*)(addr))
            #endif
        #endif // !defined (ARDUINO)

        #ifndef prog_uint16_t
typedef uint16_t prog_uint16_t;
        #endif
        #ifndef PSTR
            #define PSTR(x) (x)
        #endif
        #ifndef printf_P
            #define printf_P printf
        #endif
        #ifndef strlen_P
            #define strlen_P strlen
        #endif
        #ifndef PROGMEM
            #define PROGMEM
        #endif
        #ifndef pgm_read_word
            #define pgm_read_word(p) (*(const unsigned short*)(p))
        #endif
        #if !defined pgm_read_ptr || defined ARDUINO_ARCH_MBED
            #define pgm_read_ptr(p) (*(void* const*)(p))
        #endif
        #ifndef PRIPSTR
            #define PRIPSTR "%s"
        #endif

    #endif // !defined (ARDUINO) || defined (ESP_PLATFORM) || defined (__arm__) || defined (__ARDUINO_X86__) && !defined (XMEGA)

#endif //Everything else

#if defined(SPI_HAS_TRANSACTION) && !defined(SPI_UART) && !defined(SOFTSPI)
    #define RF24_SPI_TRANSACTIONS
#endif // defined (SPI_HAS_TRANSACTION) && !defined (SPI_UART) && !defined (SOFTSPI)

#endif // RF24_CONFIG_H_
