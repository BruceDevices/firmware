#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

static const uint8_t TX = 1;
static const uint8_t RX = 3;

static const uint8_t SDA = 21;
static const uint8_t SCL = 22;

static const uint8_t SS = 5;
static const uint8_t MOSI = 23;
static const uint8_t MISO = 19;
static const uint8_t SCK = 18;

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

#define SERIAL_TX 1
#define SERIAL_RX 3
#define BAD_TX SERIAL_TX
#define BAD_RX SERIAL_RX
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX

#define TXLED 2
#define LED_ON HIGH
#define LED_OFF LOW

#define FP 1
#define FM 2
#define FG 3

#define HAS_SCREEN 1
#define ROTATION 1
#define MINBRIGHT 1

// TFT_eSPI Setup for ST7789 2.8" display (VSPI bus)
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
#define TFT_MISO 19
#endif
#ifndef TFT_MOSI
#define TFT_MOSI 23
#endif
#ifndef TFT_SCLK
#define TFT_SCLK 18
#endif
#ifndef TFT_CS
#define TFT_CS 17
#endif
#ifndef TFT_DC
#define TFT_DC 16
#endif
#ifndef TFT_RST
#define TFT_RST 5
#endif
#ifndef TFT_BL
#define TFT_BL 32
#endif
#ifndef TFT_BACKLIGHT_ON
#define TFT_BACKLIGHT_ON HIGH
#endif
#ifndef TOUCH_CS
#define TOUCH_CS -1
#endif
#ifndef SMOOTH_FONT
#define SMOOTH_FONT 1
#endif
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

// Touchscreen — XPT2046 resistive (shares VSPI bus with display)
#define HAS_TOUCH 1
#define TOUCH_XPT2046_SPI 1
#define XPT2046_SPI_BUS_MOSI_IO_NUM 23
#define XPT2046_SPI_BUS_MISO_IO_NUM 19
#define XPT2046_SPI_BUS_SCLK_IO_NUM 18
#define XPT2046_SPI_CONFIG_CS_GPIO_NUM 21
#define XPT2046_TOUCH_CONFIG_INT_GPIO_NUM -1

// SD Card (on VSPI bus)
#define SDCARD_CS 12
#define SDCARD_SCK 18
#define SDCARD_MISO 19
#define SDCARD_MOSI 23

// Default I2C
#define GROVE_SDA 21
#define GROVE_SCL 22

// Shared SPI bus for CC1101/NRF24 (main VSPI)
#define SPI_SCK_PIN 18
#define SPI_MOSI_PIN 23
#define SPI_MISO_PIN 19
#define SPI_SS_PIN 15

#endif /* Pins_Arduino_h */
