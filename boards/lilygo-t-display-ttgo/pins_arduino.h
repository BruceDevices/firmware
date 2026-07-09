#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#define ADC_EN 14 // ADC_EN is the ADC detection enable port
#define ANALOG_BAT_PIN 34

// Lite Version
// #define LITE_VERSION 1

#define SPI_SS_PIN 33
#define SPI_MOSI_PIN 26
#define SPI_MISO_PIN 27
#define SPI_SCK_PIN 25

#define SDCARD_CS SPI_SS_PIN
#define SDCARD_SCK SPI_SCK_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN

static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t SCK = SPI_MISO_PIN;
static const uint8_t MISO = SPI_SCK_PIN;

// Set Main I2C Bus
#define GROVE_SDA 21
#define GROVE_SCL 22
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// TFT_eSPI display
#define USER_SETUP_LOADED
#define ST7735_DRIVER
#define TFT_WIDTH 80
#define TFT_HEIGHT 160
#define CGRAM_OFFSET
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23
#define TFT_BL 4              // Display backlight control pin
#define TFT_BACKLIGHT_ON HIGH // HIGH or LOW are options
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 27000000     // Maximum for ILI9341
#define SPI_READ_FREQUENCY 6000000 // 6 MHz is the maximum SPI read speed for the ST7789V
#define ST7735_GREENTAB160x80
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON
// Display Setup#
#define HAS_SCREEN
#define ROTATION 3
#define MINBRIGHT (uint8_t)1

// Font Sizes#
#define FP 1
#define FM 1
#define FG 1

// Serial
#define SERIAL_TX 12
#define SERIAL_RX 13



#define BAD_TX 12
#define BAD_RX 13

// Buttons & Navigation
#define BTN_ALIAS "\"OK\""
#define HAS_3_BUTTONS
#define UP_BTN 0
#define DW_BTN 35
#define BTN_ACT LOW

#define LED_ON HIGH
#define LED_OFF LOW

#endif /* Pins_Arduino_h */
