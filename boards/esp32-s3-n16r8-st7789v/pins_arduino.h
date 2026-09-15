#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME "ESP32-S3 N16R8 ST7789V"
#endif

static const uint8_t TX = 43;
static const uint8_t RX = 44;
static const uint8_t SDA = 4;
static const uint8_t SCL = 5;

static const uint8_t SS = 15;
static const uint8_t MOSI = 18;
static const uint8_t MISO = 38;
static const uint8_t SCK = 21;

#endif /* Pins_Arduino_h */
