#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>
#include "soc/soc_caps.h"

// USB identifiers (optional)
#define USB_VID 0x303a
#define USB_PID 0x1001

// UART pins (keep defaults if present)
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// I2C pins for OLED display (4-wire I2C: VCC, GND, SDA, SCL)
#define SYS_I2C_SDA 8
#define SYS_I2C_SCL 9

static const uint8_t SDA = SYS_I2C_SDA;
static const uint8_t SCL = SYS_I2C_SCL;

// SPI pins (default)
static const uint8_t SS = 10;
static const uint8_t MOSI = 11;
static const uint8_t MISO = 13;
static const uint8_t SCK = 12;

// Navigation Button Pins (safe for ESP32-S3)
#define UP_BTN   35
#define DOWN_BTN 36
#define LEFT_BTN 37
#define RIGHT_BTN 38
#define SEL_BTN  39
#define BACK_BTN 40

// GPIO compatibility aliases
static const uint8_t G0 = 0;
static const uint8_t G1 = 1;
static const uint8_t G2 = 2;
static const uint8_t G3 = 3;
static const uint8_t G4 = 4;
static const uint8_t G5 = 5;
static const uint8_t G6 = 6;
static const uint8_t G7 = 7;
static const uint8_t G8 = 8;
static const uint8_t G9 = 9;
static const uint8_t G10 = 10;
static const uint8_t G11 = 11;
static const uint8_t G12 = 12;
static const uint8_t G13 = 13;
static const uint8_t G14 = 14;
static const uint8_t G15 = 15;
static const uint8_t G16 = 16;
static const uint8_t G17 = 17;
static const uint8_t G18 = 18;
static const uint8_t G19 = 19;
static const uint8_t G20 = 20;
static const uint8_t G21 = 21;
static const uint8_t G26 = 26;
static const uint8_t G33 = 33;
static const uint8_t G34 = 34;
static const uint8_t G35 = 35;
static const uint8_t G36 = 36;
static const uint8_t G37 = 37;
static const uint8_t G38 = 38;
static const uint8_t G39 = 39;
static const uint8_t G40 = 40;
static const uint8_t G41 = 41;
static const uint8_t G42 = 42;
static const uint8_t G43 = 43;
static const uint8_t G44 = 44;
static const uint8_t G45 = 45;
static const uint8_t G46 = 46;
static const uint8_t G47 = 47;
static const uint8_t G48 = 48;

// ADC pins (kept for compatibility)
static const uint8_t ADC1 = 1;
static const uint8_t ADC2 = 2;
static const uint8_t ADC3 = 3;
static const uint8_t ADC4 = 4;

#endif /* Pins_Arduino_h */
