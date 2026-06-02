#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#define SPI_SCK_PIN 17
#define SPI_MOSI_PIN 18
#define SPI_MISO_PIN 8
#define SPI_SS_PIN 5
// Modified elsewhere
static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK = SPI_SCK_PIN;

#define SERIAL_RX 44
#define SERIAL_TX 43
#define BAD_RX SERIAL_RX
#define BAD_TX SERIAL_TX
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX
#define USB_as_HID 1
static const uint8_t TX = 43;
static const uint8_t RX = 44;

#define GROVE_SDA 47
#define GROVE_SCL 48
static const uint8_t SDA = 47;
static const uint8_t SCL = 48;

#define BTN_ALIAS "\"OK\""
#define HAS_5_BUTTONS
#define SEL_BTN 0
#define UP_BTN 39
#define DW_BTN 38
#define R_BTN 11
#define L_BTN 10
#define BTN_ACT LOW

#define RXLED 1
#define TXLED 2
#define LED_ON HIGH
#define LED_OFF LOW

#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 9
// #define CC1101_GDO2_PIN 10
#define CC1101_SS_PIN 46
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 13
#define NRF24_SS_PIN 12
#define NRF24_MOSI_PIN SDCARD_MOSI
#define NRF24_SCK_PIN SDCARD_SCK
#define NRF24_MISO_PIN SDCARD_MISO

#define FP 1
#define FM 2
#define FG 3

#define HAS_SCREEN 1
#define ROTATION 3
#define MINBRIGHT (uint8_t)1

#define USER_SETUP_LOADED 1
#define ST7789_DRIVER 1
#define TFT_RGB_ORDER 0
#define TFT_WIDTH 240
#define TFT_HEIGHT 280
#define TFT_BACKLIGHT_ON 1
#define TFT_BL 6
#define TFT_RST 16
#define TFT_DC 15
#define TFT_MISO SPI_MISO_PIN
#define TFT_MOSI SPI_MOSI_PIN
#define TFT_SCLK SPI_SCK_PIN
#define TFT_CS 7
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 80000000
#define SPI_READ_FREQUENCY 20000000

#define SDCARD_CS 3
#define SDCARD_SCK SPI_SCK_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN

#define USE_BOOST
#define XPOWERS_CHIP_BQ25896

#endif
