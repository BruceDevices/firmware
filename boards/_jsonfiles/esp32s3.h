#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef USB_VID
#define USB_VID 0x303a
#endif
#ifndef USB_PID
#define USB_PID 0x1001
#endif

#ifndef ARDUINO_DEFAULT_TX
#define ARDUINO_DEFAULT_TX 43
#endif
static const uint8_t TX = ARDUINO_DEFAULT_TX;
#ifndef ARDUINO_DEFAULT_RX
#define ARDUINO_DEFAULT_RX 44
#endif
static const uint8_t RX = ARDUINO_DEFAULT_RX;

#ifndef ARDUINO_DEFAULT_SDA
#define ARDUINO_DEFAULT_SDA 8
#endif
static const uint8_t SDA = ARDUINO_DEFAULT_SDA;
#ifndef ARDUINO_DEFAULT_SCL
#define ARDUINO_DEFAULT_SCL 9
#endif
static const uint8_t SCL = ARDUINO_DEFAULT_SCL;

#ifndef ARDUINO_DEFAULT_SS
#define ARDUINO_DEFAULT_SS 10
#endif
static const uint8_t SS = ARDUINO_DEFAULT_SS;
#ifndef ARDUINO_DEFAULT_MOSI
#define ARDUINO_DEFAULT_MOSI 11
#endif
static const uint8_t MOSI = ARDUINO_DEFAULT_MOSI;
#ifndef ARDUINO_DEFAULT_MISO
#define ARDUINO_DEFAULT_MISO 13
#endif
static const uint8_t MISO = ARDUINO_DEFAULT_MISO;
#ifndef ARDUINO_DEFAULT_SCK
#define ARDUINO_DEFAULT_SCK 12
#endif
static const uint8_t SCK = ARDUINO_DEFAULT_SCK;

// Boards with fixed peripherals can override defaults in their build flags.
// Wire1 defaults are only supplied when a board explicitly declares both pins.
#if defined(ARDUINO_DEFAULT_SDA1) && defined(ARDUINO_DEFAULT_SCL1)
#define WIRE1_PIN_DEFINED
static const uint8_t SDA1 = ARDUINO_DEFAULT_SDA1;
static const uint8_t SCL1 = ARDUINO_DEFAULT_SCL1;
#endif

static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
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

#endif /* Pins_Arduino_h */
