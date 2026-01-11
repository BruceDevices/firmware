#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>
#include "soc/soc_caps.h"

#define USB_VID 0x303a
#define USB_PID 0x1001

// T-Deck Pro does not have NeoPixel LED
#define PIN_NEOPIXEL        -1
#define LED_BUILTIN         -1
#define BUILTIN_LED         -1
#define RGB_BUILTIN         -1
#define RGB_BRIGHTNESS      0

#define HAS_KEYBOARD        // Has TCA8418 keyboard controller
#define HAS_KEYBOARD_HID    // Has keyboard HID support
#define KB_HID_EXIT_MSG "Fn + Esc to exit"

// UART pins - GPS MIA-M10Q
static const uint8_t TX = 16;
static const uint8_t RX = 17;

// I2C pins - Touch CST328 & Keyboard TCA8418
static const uint8_t SDA = 13;
static const uint8_t SCL = 14;

// SPI pins - E-Paper display, SD Card, LoRa
static const uint8_t SS    = 34;   // E-Paper CS
static const uint8_t MOSI  = 33;
static const uint8_t MISO  = -1;   // No MISO needed for E-Paper
static const uint8_t SCK   = 36;

// ADC pins
static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;   // Battery ADC
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;
static const uint8_t A6 = 7;
static const uint8_t A7 = 8;
static const uint8_t A8 = 9;
static const uint8_t A9 = 10;
static const uint8_t A10 = 11;
static const uint8_t A11 = 12;
static const uint8_t A12 = 13;
static const uint8_t A13 = 14;
static const uint8_t A14 = 15;
static const uint8_t A15 = 16;
static const uint8_t A16 = 17;
static const uint8_t A17 = 18;
static const uint8_t A18 = 19;
static const uint8_t A19 = 20;

// Touch capacitive pins (ESP32-S3 has touch support)
static const uint8_t T1 = 1;
static const uint8_t T2 = 2;
static const uint8_t T3 = 3;
static const uint8_t T4 = 4;
static const uint8_t T5 = 5;
static const uint8_t T6 = 6;
static const uint8_t T7 = 7;
static const uint8_t T8 = 8;
static const uint8_t T9 = 9;
static const uint8_t T10 = 10;
static const uint8_t T11 = 11;
static const uint8_t T12 = 12;
static const uint8_t T13 = 13;
static const uint8_t T14 = 14;

// Deepsleep - Keyboard interrupt can wake
#define DEEPSLEEP_WAKEUP_PIN 15  // Keyboard INT pin
#define DEEPSLEEP_PIN_ACT LOW

#endif /* Pins_Arduino_h */
