#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// Lite Version
// #define LITE_VERSION 1

#define SPI_SS_PIN 15
#define SPI_MOSI_PIN 3
#define SPI_MISO_PIN 4
#define SPI_SCK_PIN 1

// it is physically grounded, using a NC(33) pin to avoid warnings on Serial during SD Readings
#define SDCARD_CS -1 // it is grounded
#define SDCARD_SCK 12
#define SDCARD_MISO 13
#define SDCARD_MOSI 11

#define CC1101_GDO0_PIN 16
#define CC1101_SS_PIN 15
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define NRF24_CE_PIN 16
#define NRF24_SS_PIN 15
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

#define USE_W5500_VIA_SPI
#define W5500_SS_PIN SPI_SS_PIN
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_INT_PIN 16

// touch screen XPT2046
// Pins set on .ini file
#define HAS_TOUCH 1
// Set Main I2C Bus
#define GROVE_SDA 17
#define GROVE_SCL 18
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// Serial
#define SERIAL_TX 44
#define SERIAL_RX 43
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX
#define TXLED 17
#define RXLED 18

static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t SCK = SPI_MISO_PIN;
static const uint8_t MISO = SPI_SCK_PIN;

// TFT_eSPI display
#define USER_SETUP_LOADED
#define ST7789_DRIVER // Configure all registers
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_RGB_ORDER TFT_BGR // Colour order Blue-Green-Red
#define TFT_INVERSION_OFF
#define TFT_PARALLEL_8_BIT
#define TFT_CS 6   // Chip select control pin (library pulls permanently low
#define TFT_DC 7   // Data Command control pin
#define TFT_RST -1 // Reset pin, toggles on startup
#define TFT_WR 8   // Write strobe control pin
#define TFT_RD -1  // Read strobe control pin
#define TFT_D0 48  // Must use pins in the range 0-31 or alternatively 32-48
#define TFT_D1 47  // so a single register write sets/clears all bits.
#define TFT_D2 39  // Pins can be randomly assigned, this does not affect
#define TFT_D3 40  // TFT screen update performance.
#define TFT_D4 41
#define TFT_D5 42
#define TFT_D6 45
#define TFT_D7 46
#define TFT_BL 38 // LED back-light

#define SMOOTH_FONT 1

// Display Setup#
#define HAS_SCREEN
#define ROTATION 3
#define MINBRIGHT (uint8_t)1

// Font Sizes#
#define FP 1
#define FM 2
#define FG 3

// Battery PIN
#define PWR_EN_PIN 10
#define PWR_ON_PIN 14
#define ANALOG_BAT_PIN 5

// Mic
#define PIN_CLK -1
#define PIN_DATA -1

// Buttons & Navigation
#define BTN_ALIAS "\"OK\""

// IR pins
#define LED_ON HIGH
#define LED_OFF LOW

// BadUSB
#define USB_as_HID 1

#endif /* Pins_Arduino_h */
