#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME "Elecrow Advance 3.5 S3"
#endif

// =============================================================================
//  Elecrow CrowPanel Advance 3.5" HMI
//  ESP32-S3-WROOM-1-N16R8, ILI9488 480x320 IPS over SPI (TFT_eSPI backend),
//  GT911 capacitive touch over I2C. Concrete pin assignments live in the build
//  flags (boards/elecrow_advance_s3/elecrow_advance_s3.ini); this header only
//  provides the standard Arduino aliases the core/libraries expect.
// =============================================================================

static const uint8_t TX = 43;
static const uint8_t RX = 44;

// GT911 capacitive touch I2C bus
static const uint8_t SDA = 15;
static const uint8_t SCL = 16;

// Display / SD share SPI signals; chip-selects differ (see board .ini)
static const uint8_t SS = 40;   // TFT_CS
static const uint8_t MOSI = 39; // TFT_MOSI
static const uint8_t MISO = 4;  // SD_MISO (TFT MISO is unused, -1)
static const uint8_t SCK = 42;  // TFT_SCLK

#endif /* Pins_Arduino_h */
