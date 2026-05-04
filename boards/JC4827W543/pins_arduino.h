#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

// UART pins
static const uint8_t TX = 1;
static const uint8_t RX = 3;

// I2C pins (Groove)
static const uint8_t SDA = 21;
static const uint8_t SCL = 22;

// SPI pins (for peripherals)
static const uint8_t SS = 5;
static const uint8_t MOSI = 23;
static const uint8_t MISO = 19;
static const uint8_t SCK = 18;

// Analog pins
static const uint8_t A0 = 36;
static const uint8_t A3 = 39;
static const uint8_t A4 = 32;
static const uint8_t A5 = 33;
static const uint8_t A6 = 34;
static const uint8_t A7 = 35;
static const uint8_t A10 = 4;
static const uint8_t A11 = 0;
static const uint8_t A12 = 2;
static const uint8_t A13 = 15;
static const uint8_t A14 = 13;
static const uint8_t A15 = 12;

// Touch controller (XPT2046 or GT911)
// Using GPIO 38 for XPT2046 CS, GPIO 3 for interrupt
#define TOUCH_CS 38
#define TOUCH_INT 3

// FM Radio Si4713 (optional)
#define FM_RSTPIN 40

// Microphone (optional)
#define PIN_CLK 43
#define I2S_SCLK_PIN 43
#define I2S_DATA_PIN 46
#define PIN_DATA 46

// RGB LED (optional)
#define RGB_LED -1

// Speaker NS4168 (optional)
#define BCLK 41
#define WCLK 43
#define DOUT 42

// BadUSB HID (optional)
#define BAD_TX GROVE_SDA
#define BAD_RX GROVE_SCL

// Physical button
#define HAS_BTN 1
#define BTN_ALIAS "Ok"
#define BTN_PIN 0
#define BTN_ACT LOW

// IR/RF disabled for initial build - can enable later
#define TXLED 22
#define LED_ON HIGH
#define LED_OFF LOW

// CC1101, NRF24, W5500 - disabled for initial build
// #define USE_CC1101_VIA_SPI
// #define USE_NRF24_VIA_SPI
// #define USE_W5500_VIA_SPI

// Font sizes
#define FP 1
#define FM 2
#define FG 3

// Screen settings
#define HAS_SCREEN 1
#define ROTATION 1
#define MINBRIGHT 160

// TFT_eSPI setup for ST7789 display via SPI
#define USER_SETUP_LOADED 1
#define ST7789_2_DRIVER 1
#define TFT_WIDTH 480
#define TFT_HEIGHT 270
#define TFT_BACKLIGHT_ON 1

// SPI pins for display
#define TFT_CS 45
#define TFT_DC 48
#define TFT_RST 40
#define TFT_BL 1
#define TFT_MOSI 21
#define TFT_SCLK 47
#define TFT_MISO 39

#define SPI_FREQUENCY 27000000
#define SPI_READ_FREQUENCY 16000000
#define SMOOTH_FONT 1

// Touch controller (XPT2046 for resistive touch)
#define HAS_TOUCH 1
#define TOUCH_CS 38
#define TOUCH_INT 3
#define USE_TFT_eSPI_TOUCH 1
#define XPT2046_CS TOUCH_CS
#define XPT2046_IRQ TOUCH_INT
#define TOUCH_MIN_PRESSURE 200
#define TOUCH_ORIENTATION 0
#define TOUCH_MAP_X1 0
#define TOUCH_MAP_X2 480
#define TOUCH_MAP_Y1 270
#define TOUCH_MAP_Y2 0

// SD Card
#define SDCARD_CS 5
#define SDCARD_SCK 18
#define SDCARD_MISO 19
#define SDCARD_MOSI 23

// I2C Grove port
#define GROVE_SDA 21
#define GROVE_SCL 22

// SPI bus (shared with SD card)
#define SPI_SCK_PIN 18
#define SPI_MOSI_PIN 23
#define SPI_MISO_PIN 19
#define SPI_SS_PIN 5

// Deepsleep wakeup
#define DEEPSLEEP_WAKEUP_PIN 36
#define DEEPSLEEP_PIN_ACT LOW

#endif /* Pins_Arduino_h */