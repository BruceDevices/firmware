#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>
#include "soc/soc_caps.h"

// I2C — override standard esp32s3 defaults (SDA=8, SCL=9) for this board
static const uint8_t SDA = 38;
static const uint8_t SCL = 39;

// Grove connector aliases
#define GROVE_SDA 38
#define GROVE_SCL 39

// Serial / GPS
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// SPI defaults. The board's SPI2_HOST is wired to the QSPI display;
// peripheral SPI (RFID, CC1101, NRF24, etc.) uses these same GPIO
// numbers unless overridden at runtime with begin(sck, miso, mosi, ss).
#define SPI_SS_PIN   10
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SCK_PIN  12

static const uint8_t SS   = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK  = SPI_SCK_PIN;

// Peripheral aliases expected by Bruce modules
#define SDCARD_SCK   SPI_SCK_PIN
#define SDCARD_MISO  SPI_MISO_PIN
#define SDCARD_MOSI  SPI_MOSI_PIN
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN  SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN
#define NRF24_MOSI_PIN  SPI_MOSI_PIN
#define NRF24_SCK_PIN   SPI_SCK_PIN
#define NRF24_MISO_PIN  SPI_MISO_PIN
#define W5500_MOSI_PIN  SPI_MOSI_PIN
#define W5500_SCK_PIN   SPI_SCK_PIN
#define W5500_MISO_PIN  SPI_MISO_PIN

// Font sizes (used by Bruce display code).
// Deliberately static const (NOT #define) so that FastLED's internal
// `using FP = fl::s16x16;` type-alias is not broken by macro substitution.
static const uint8_t FP = 1;
static const uint8_t FM = 2;
static const uint8_t FG = 3;

// USB HID capable
// On-board WS2812B (3-channel, GRB) on RGB_LED
#define LED_TYPE WS2812B
#define LED_ORDER GRB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 1
#define LED_COLOR_STEP 15

#define USB_as_HID 1

#endif /* Pins_Arduino_h */
