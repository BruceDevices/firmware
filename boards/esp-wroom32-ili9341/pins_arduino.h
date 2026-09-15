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

// 5-way tactile switch
#define HAS_5_BUTTONS
#define SEL_BTN 35
#define UP_BTN 34
#define DW_BTN 26
#define R_BTN 27
#define L_BTN 33
#define BTN_ACT LOW

#define BTN_ALIAS "\"OK\""

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
#define MINBRIGHT 160

// TFT_eSPI Setup for ILI9341 2.4" display
#define USER_SETUP_LOADED 1
#define ILI9341_2_DRIVER 1
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 17
#define TFT_DC 16
#define TFT_RST 5
#define TFT_BL 32
#define TFT_BACKLIGHT_ON HIGH
#define TOUCH_CS -1
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000

// SD Card (directly on main SPI bus)
#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

// Default I2C
#define GROVE_SDA 21
#define GROVE_SCL 22

// Shared SPI bus for CC1101/NRF24 (main VSPI)
#define SPI_SCK_PIN 18
#define SPI_MOSI_PIN 23
#define SPI_MISO_PIN 19
#define SPI_SS_PIN 15

#endif /* Pins_Arduino_h */
