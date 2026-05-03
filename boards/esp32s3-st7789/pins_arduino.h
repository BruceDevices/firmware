#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

static const uint8_t TX = 43;
static const uint8_t RX = 44;

static const uint8_t SDA = 8;
static const uint8_t SCL = 9;

static const uint8_t SS = 10;
static const uint8_t MOSI = 11;
static const uint8_t MISO = 13;
static const uint8_t SCK = 12;

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

// Boot button as OK (active-low)
#ifndef HAS_BTN
#define HAS_BTN 1
#endif
#ifndef BTN_ALIAS
#define BTN_ALIAS "\"OK\""
#endif
#ifndef BTN_PIN
#define BTN_PIN 0
#endif
#ifndef BTN_ACT
#define BTN_ACT LOW
#endif

#define HAS_SCREEN 1
#define ROTATION 1
#define MINBRIGHT 1

// TFT_eSPI Setup for ST7789 2.8" display (HSPI bus)
#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED 1
#endif
#ifndef ST7789_DRIVER
#define ST7789_DRIVER 1
#endif
#ifndef TFT_RGB_ORDER
#define TFT_RGB_ORDER TFT_BGR
#endif
#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 320
#endif
#ifndef TFT_INVERSION_ON
#define TFT_INVERSION_ON
#endif
#ifndef TFT_MISO
#define TFT_MISO 13
#endif
#ifndef TFT_MOSI
#define TFT_MOSI 11
#endif
#ifndef TFT_SCLK
#define TFT_SCLK 12
#endif
#ifndef TFT_CS
#define TFT_CS 10
#endif
#ifndef TFT_DC
#define TFT_DC 7
#endif
#ifndef TFT_RST
#define TFT_RST 5
#endif
#ifndef TFT_BL
#define TFT_BL 38
#endif
#ifndef TFT_BACKLIGHT_ON
#define TFT_BACKLIGHT_ON HIGH
#endif
#ifndef SMOOTH_FONT
#define SMOOTH_FONT 1
#endif
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

// Touchscreen — XPT2046 resistive (shares HSPI bus with display)
#define HAS_TOUCH 1
#define TOUCH_XPT2046_SPI 1
#define XPT2046_SPI_BUS_MOSI_IO_NUM 11
#define XPT2046_SPI_BUS_MISO_IO_NUM 13
#define XPT2046_SPI_BUS_SCLK_IO_NUM 12
#define XPT2046_SPI_CONFIG_CS_GPIO_NUM 3
#define XPT2046_TOUCH_CONFIG_INT_GPIO_NUM -1

// Status LED
#define TXLED 40
#define LED_ON HIGH
#define LED_OFF LOW

// SD Card (on HSPI bus with display/touch)
#define SDCARD_CS 4
#define SDCARD_SCK 12
#define SDCARD_MISO 13
#define SDCARD_MOSI 11

// Default I2C
#define GROVE_SDA 8
#define GROVE_SCL 9

// Shared SPI bus for CC1101/NRF24 (main SPI)
#define SPI_SCK_PIN 12
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SS_PIN 10

#endif /* Pins_Arduino_h */
