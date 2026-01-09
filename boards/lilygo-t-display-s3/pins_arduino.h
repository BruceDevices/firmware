#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME "Lilygo T-Display S3"
#endif
// Lite Version
// #define LITE_VERSION 1

// Main SPI Bus
#ifdef USE_SD_MMC

// Willy board definitions
#define SPI_SS_PIN 1
#define SPI_MOSI_PIN 3
#define SPI_MISO_PIN 2
#define SPI_SCK_PIN 43

#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 44
#define CC1101_SS_PIN 1
#define CC1101_MOSI_PIN 3
#define CC1101_SCK_PIN 43
#define CC1101_MISO_PIN 2

#define USE_NRF24_VIA_SPI
#ifndef NRF24_SS_PIN    // if touch, set on lilygo-t-display-s3.ini
#define NRF24_CE_PIN 17 // if touch, set on lilygo-t-display-s3.ini
#define NRF24_SS_PIN 18 // if touch, set on lilygo-t-display-s3.ini
#endif
#define NRF24_MOSI_PIN 3
#define NRF24_SCK_PIN 43
#define NRF24_MISO_PIN 2

#define USE_W5500_VIA_SPI
#define W5500_SS_PIN -1
#define W5500_MOSI_PIN 3
#define W5500_SCK_PIN 43
#define W5500_MISO_PIN 2
#define W5500_INT_PIN -1

// Set Main I2C Bus
#ifndef GROVE_SDA    // if touch, set on lilygo-t-display-s3.ini
#define GROVE_SDA 16 // if touch, set on lilygo-t-display-s3.ini
#define GROVE_SCL 21 // if touch, set on lilygo-t-display-s3.ini
#endif
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// Serial
#define SERIAL_TX 21
#define SERIAL_RX 16

// Infrared
#define TXLED 10
#define RXLED 44

#else

#define SPI_SS_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SCK_PIN 12

#define SDCARD_CS 1
#define SDCARD_SCK SPI_SCK_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN

#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 21
#define CC1101_SS_PIN 2
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 3
#define NRF24_SS_PIN 10
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

#define USE_W5500_VIA_SPI
#define W5500_SS_PIN -1
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_INT_PIN -1

// Set Main I2C Bus
#ifndef GROVE_SDA    // if touch, set on lilygo-t-display-s3.ini
#define GROVE_SDA 44 // if touch, set on lilygo-t-display-s3.ini
#define GROVE_SCL 43 // if touch, set on lilygo-t-display-s3.ini
// InfraRed pis
#define TXLED 17     // if touch, set on lilygo-t-display-s3.ini
#define RXLED 18     // if touch, set on lilygo-t-display-s3.ini
#endif

static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// Serial
#define SERIAL_TX 44
#define SERIAL_RX 43

#endif

static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t SCK = SPI_MISO_PIN;
static const uint8_t MISO = SPI_SCK_PIN;

// TFT_eSPI display
#define USER_SETUP_LOADED
#define ST7789_DRIVER 1
#define TFT_BACKLIGHT_ON 1
#define INIT_SEQUENCE_3
#define CGRAM_OFFSET
#define TFT_RGB_ORDER TFT_RGB
#define TFT_INVERSION_ON
#define TFT_PARALLEL_8_BIT
#define SMOOTH_FONT 1
#define TFT_WIDTH 170
#define TFT_HEIGHT 320
#define TFT_BL 38
#define TFT_CS 6
#define TFT_DC 7
#define TFT_RST 5
#define TFT_D0 39
#define TFT_D1 40
#define TFT_D2 41
#define TFT_D3 42
#define TFT_D4 45
#define TFT_D5 46
#define TFT_D6 47
#define TFT_D7 48
#define TFT_WR 8
#define TFT_RD 9

#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000

// Display Setup#
#define HAS_SCREEN
#define ROTATION 3
#define MINBRIGHT (uint8_t)1

// Font Sizes#
#define FP 1
#define FM 2
#define FG 3

// Battery PIN
#define PIN_POWER_ON 15
#define ANALOG_BAT_PIN 4

// Mic
#define PIN_CLK 39
#define PIN_DATA 42

// Buttons & Navigation
#define BTN_ALIAS "\"OK\""
#define HAS_3_BUTTONS
#define SEL_BTN 16
#define UP_BTN 0
#define DW_BTN 14
#define BTN_ACT LOW

// IR pins

#define LED_ON HIGH
#define LED_OFF LOW

// BadUSB
#define USB_as_HID 1

#endif /* Pins_Arduino_h */
