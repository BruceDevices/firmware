#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

/*#define PIN_RGB_LED 27
// BUILTIN_LED can be used in new Arduino API digitalWrite() like in Blink.ino
static const uint8_t LED_BUILTIN = SOC_GPIO_PIN_COUNT + PIN_RGB_LED;
#define BUILTIN_LED LED_BUILTIN // backward compatibility
#define LED_BUILTIN LED_BUILTIN // allow testing #ifdef LED_BUILTIN
// RGB_BUILTIN and RGB_BRIGHTNESS can be used in new Arduino API rgbLedWrite()
#define RGB_BUILTIN LED_BUILTIN
#define RGB_BRIGHTNESS 64*/

static const uint8_t TX = 11;
static const uint8_t RX = 12;

static const uint8_t USB_DM = 13;
static const uint8_t USB_DP = 14;

static const uint8_t SDA = 4;
static const uint8_t SCL = 5;

static const uint8_t SS = 10;
static const uint8_t MOSI = 7;
static const uint8_t MISO = 2;
static const uint8_t SCK = 6;

static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;

// LP I2C Pins are fixed on ESP32-C5
static const uint8_t LP_SDA = 4;
static const uint8_t LP_SCL = 5;
#define WIRE1_PIN_DEFINED
#define SDA1 LP_SDA
#define SCL1 LP_SCL

// LP UART Pins are fixed on ESP32-C5
static const uint8_t LP_RX = 12;
static const uint8_t LP_TX = 11;

#define HAS_RGB_LED 1
#define RGB_LED 27
#define LED_TYPE WS2812B
#define LED_ORDER RGB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 5
#define LED_COLOR_STEP 5

/* Communication Buses*/
// UART
#define SERIAL_TX 11
#define SERIAL_RX 12
// I2C
#define GROVE_SDA 4
#define GROVE_SCL 5
// SPI
#define SPI_SCK_PIN 6
#define SPI_MOSI_PIN 7
#define SPI_MISO_PIN 2
#define SPI_SS_PIN 8

/* TFT definitions */
#define HAS_SCREEN 1
// #define ROTATION 2
#define MINBRIGHT (uint8_t)1
#define USER_SETUP_LOADED 1
/* ---------------------   */
// Setup for ST7789 170x320

// #define ST7789_DRIVER 1
// #define TFT_RGB_ORDER 0
// #define TFT_WIDTH 240
// #define TFT_HEIGHT 280

/* ---------------------   */
// Setup for ST7789 240x320

#define ST7789_DRIVER 1
#define TFT_WIDTH 135

#define TFT_HEIGHT 240
#define TFT_INVERSION_ON
#define TFT_ORIENTATION 1
#define TFT_RGB_ORDER TFT_BGR

/* ---------------------   */
// Setup for ILI9341 320x240 (no touch)

// #define ILI9341_DRIVER 1
// define TFT_HEIGHT 320
// #define TFT_WIDTH 240

/* ---------------------   */
// Common TFT definitions
#define TFT_BACKLIGHT_ON 1
#define TFT_BL 3
#define TFT_RST -1
#define TFT_DC 23
#define TFT_MISO SPI_MISO_PIN
#define TFT_MOSI SPI_MOSI_PIN
#define TFT_SCLK SPI_SCK_PIN
#define TFT_CS 24
// #define TOUCH_CS 1
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 20000000
#define SPI_READ_FREQUENCY 20000000
// #define SPI_TOUCH_FREQUENCY 2500000

/*  Peripheral settings  */
// Bad USB with CH9329
#define BAD_RX 12
#define BAD_TX 11
// GPS Bus
#define GPS_SERIAL_TX 11
#define GPS_SERIAL_RX 12

// InfraRed
#define RXLED 26
#define TXLED 25
#define LED_ON HIGH
#define LED_OFF LOW
// SDCard
#define SDCARD_CS 0
#define SDCARD_SCK SPI_SCK_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN
// CC1101
// #define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 10
#define CC1101_SS_PIN 9
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN
// NRF24

// #define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 1
#define NRF24_SS_PIN 8
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN
// Ethernet
/*#define W5500_INT_PIN 8
#define W5500_SS_PIN 9
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN*/

#define USE_IO_EXPANDER
#define IO_EXPANDER_AW9523

#define EXPANDER_INT_PIN 28
/// IO BUTTONS
#define HAS_5_BUTTONS
#define IO_EXP_UP 11
#define IO_EXP_DOWN 0
#define IO_EXP_ESC 8
#define IO_EXP_LEFT 1
#define IO_EXP_RIGHT 10
#define IO_EXP_SEL 9

#define IO_EXP_GPS 6

#define BTN_ACT LOW

// #define USE_BOOST
#define XPOWERS_CHIP_BQ25896

#endif /* Pins_Arduino_h */
