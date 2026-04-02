#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

static const uint8_t TX = 4;
static const uint8_t RX = 5;

static const uint8_t SDA = 4;
static const uint8_t SCL = 5;

static const uint8_t SS = 6;
static const uint8_t MOSI = 11;
static const uint8_t MISO = 8;
static const uint8_t SCK = 10;

#define SPI_SCK_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 8

#define SERIAL_RX 5
#define SERIAL_TX 4
#define BAD_RX SERIAL_RX
#define BAD_TX SERIAL_TX
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX
#define USB_as_HID 1

#define BTN_ALIAS "\"OK\""
#define HAS_3_BUTTONS
#define SEL_BTN 1
#define DW_BTN 2
#define UP_BTN 3
#define BTN_ACT LOW
#define DEEPSLEEP_WAKEUP_PIN SEL_BTN

#define RXLED 4
#define TXLED 5
#define LED_ON HIGH
#define LED_OFF LOW

#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 16
#define CC1101_SS_PIN 17
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 21
#define NRF24_SS_PIN 18
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

#define FP 1
#define FM 2
#define FG 3

#define HAS_SCREEN 1
#define ROTATION 1
#define MINBRIGHT (uint8_t)1

#define USER_SETUP_LOADED 1
#define ST7789_DRIVER 1
#define TFT_RGB_ORDER 0
#define TFT_WIDTH 240
#define TFT_HEIGHT 280
#define TFT_BACKLIGHT_ON 1
#define TFT_BL 43
#define TFT_RST 12
#define TFT_DC 13
#define TFT_MISO 8
#define TFT_MOSI 11
#define TFT_SCLK 10
#define TFT_CS 44
#define TOUCH_CS -1 
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 20000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

#define SDCARD_CS 7
#define SDCARD_SCK 10
#define SDCARD_MISO 8
#define SDCARD_MOSI 11

#define GROVE_SDA 4
#define GROVE_SCL 5

#define SPI_SCK_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 8
#define SPI_SS_PIN 6

// RGB LED

#define HAS_RGB_LED 1
#define RGB_LED 48
#define LED_TYPE WS2812B
#define LED_ORDER GRB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 16

#define LED_COLOR_STEP 15

#endif /* Pins_Arduino_h */
