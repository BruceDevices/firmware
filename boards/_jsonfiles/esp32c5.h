#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

// LP UART Pins are fixed on ESP32-C5
static const uint8_t LP_RX = 12;
static const uint8_t LP_TX = 11;

static const uint8_t USB_DM = 13;
static const uint8_t USB_DP = 14;

static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;

// UART (CH340 USB-to-UART bridge)
static const uint8_t TX = 4;
static const uint8_t RX = 5;

// I2C (CN1 connector)
static const uint8_t SDA = 9;
static const uint8_t SCL = 8;
#define WIRE1_PIN_DEFINED
static const uint8_t SDA1 = 9;
static const uint8_t SCL1 = 8;
// SPI2 (FSPI) - shared by Display (ST7789), Touch (XPT2046) and SD card
static const uint8_t SS = 10; // SD card CS
static const uint8_t MOSI = 7;
static const uint8_t MISO = 2;
static const uint8_t SCK = 6;

#endif /* Pins_Arduino_h */
