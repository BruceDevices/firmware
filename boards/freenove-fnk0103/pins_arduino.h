#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

static const uint8_t TX = 1;
static const uint8_t RX = 3;

static const uint8_t TXD2 = 17;
static const uint8_t RXD2 = 16;

static const uint8_t SDA = 21;
static const uint8_t SCL = 22;

static const uint8_t SS = -1;
static const uint8_t MOSI = -1;
static const uint8_t MISO = -1;
static const uint8_t SCK = -1;

#define BAD_TX GROVE_SDA
#define BAD_RX GROVE_SCL

#define SERIAL_TX 1
#define SERIAL_RX 3
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX

#define HAS_BTN 0
#define BTN_ALIAS "\"Ok\""
#define BTN_PIN 0
#define BTN_ACT LOW

#define TXLED -1
#define LED_ON HIGH
#define LED_OFF LOW

#define FP 1
#define FM 2
#define FG 3

#define HAS_SCREEN 1
#define HAS_TOUCH 1
#define ROTATION 1
#define MINBRIGHT 160

#define USER_SETUP_LOADED 1
#define USE_HSPI_PORT 1
#define ST7789_DRIVER 1
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO 39
#define TFT_MOSI 32
#define TFT_SCLK 25
#define TFT_CS 26
#define TFT_DC 27
#define TFT_RST 14
#define TFT_BL 21
#define TFT_BACKLIGHT_ON HIGH
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000
#define TOUCH_CS 33

#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

#define GROVE_SDA 22
#define GROVE_SCL 21

#define SPI_SCK_PIN 25
#define SPI_MISO_PIN 39
#define SPI_MOSI_PIN 32
#define SPI_SS_PIN 33

#endif /* Pins_Arduino_h */
