#ifndef RF24_UTILITY_TEENSY_RF24_ARCH_CONFIG_H_
#define RF24_UTILITY_TEENSY_RF24_ARCH_CONFIG_H_
#if ARDUINO < 100

    #include <WProgram.h>

#else
    #include <Arduino.h>
#endif

#include <stddef.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <SPI.h>

#define _SPI SPIClass
#define RF24_SPI_PTR
typedef uint8_t rf24_gpio_pin_t;
#define RF24_PIN_INVALID 0xFF

#define printf Serial.printf

#ifdef RF24_DEBUG
    #define IF_RF24_DEBUG(x) ({ x; })
#else
    #define IF_RF24_DEBUG(x)
#endif

#define PRIPSTR "%s"

#endif // RF24_UTILITY_TEENSY_RF24_ARCH_CONFIG_H_
