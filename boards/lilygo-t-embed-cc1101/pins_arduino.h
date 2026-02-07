#ifdef T_EMBED_1101
#ifndef __T_EMBED_H__
#define __T_EMBED_H__
#define T_EMBED

#include "soc/soc_caps.h"
#include <stdint.h>

// Main I2C Bus
#define SPI_SS_PIN 8
#define SPI_MOSI_PIN 9
#define SPI_MISO_PIN 10
#define SPI_SCK_PIN 11
static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t SCK = SPI_MISO_PIN;
static const uint8_t MISO = SPI_SCK_PIN;

// Set Main I2C Bus
#define GROVE_SDA 8
#define GROVE_SCL 18
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// TFT_eSPI display
#define ST7789_DRIVER 1
#define USER_SETUP_LOADED
#define USE_HSPI_PORT
#define SMOOTH_FONT 1
#define TFT_INVERSION_ON
#define TFT_WIDTH 170
#define TFT_HEIGHT 320
#define TFT_BL 21
#define TFT_MISO SPI_MISO_PIN
#define TFT_MOSI SPI_MOSI_PIN
#define TFT_SCLK SPI_SCK_PIN
#define TFT_CS 41
#define TFT_DC 16
#define TFT_RST 40
#define SPI_FREQUENCY 80000000
#define SPI_READ_FREQUENCY 20000000

// Display Setup
#define HAS_SCREEN
#define ROTATION 3
#define MINBRIGHT (uint8_t)1
#define PIN_POWER_ON 15

// Font Sizes
#define FP 1
#define FM 2
#define FG 3

// SD Card
#define SDCARD_CS 13
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN
#define SDCARD_SCK SPI_SCK_PIN

// NRF24 - Over QWIIC Port #2
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 43
#define NRF24_SS_PIN 44
#define NRF24_MOSI_PIN SDCARD_MOSI
#define NRF24_SCK_PIN SDCARD_SCK
#define NRF24_MISO_PIN SDCARD_MISO

// CC1101
#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 3
#define CC1101_GDO2_PIN 38
#define CC1101_SW1_PIN 47
#define CC1101_SW0_PIN 48
#define CC1101_SS_PIN 12
#define CC1101_MISO_PIN SPI_MISO_PIN
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN

// W5500 over QWIIC port
#define USE_W5500_VIA_SPI
#define W5500_SS_PIN 44
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_INT_PIN 43
// PN532
#define PN532_RF_REST 45
#define PN532_IRQ 17

// Mic
#define PIN_CLK 39
#define PIN_DATA 42

// Speaker
#define HAS_NS4168_SPKR
#define BCLK 46
#define WCLK 40
#define DOUT 7
#define MCLK PIN_CLK

// Serial
#define SERIAL_TX 43
#define SERIAL_RX 44
static const uint8_t TX = SERIAL_TX;
static const uint8_t RX = SERIAL_RX;
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX
#define TX1 TX
#define RX1 RX

// Fuel Gauge
#define USE_BQ27220_VIA_I2C
#define BQ27220_I2C_ADDRESS 0x55
#ifdef BQ27220_I2C_SDA
#undef BQ27220_I2C_SDA
#endif
#ifdef BQ27220_I2C_SCL
#undef BQ27220_I2C_SCL
#endif
#define BQ27220_I2C_SDA GROVE_SDA
#define BQ27220_I2C_SCL GROVE_SCL

// Encoder
#define HAS_ENCODER
#define ENCODER_INA 4
#define ENCODER_INB 5
#define ENCODER_KEY 0
#define HAS_BTN 1
#define BTN_ALIAS "\"Mid\""
#define SEL_BTN ENCODER_KEY
#define UP_BTN -1
#define DW_BTN -1
#define BK_BTN 6
#define BTN_ACT LOW

// IR
#define TXLED 2
#define RXLED 1
#define LED_ON HIGH
#define LED_OFF LOW

// FM Radio
#define FM_SI4713
// RFID

#define RFID125_RX_PIN SERIAL_RX
#define RFID125_TX_PIN SERIAL_TX

// RGB LED
#define HAS_RGB_LED 1
#define RGB_LED 14
#define LED_TYPE WS2812B
#define LED_ORDER GRB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 8
#define LED_COLOR_STEP 5
#define HAS_ENCODER_LED 1

// BadUSB
#define USB_as_HID 1

// Deepsleep
#define DEEPSLEEP_WAKEUP_PIN 6
#define DEEPSLEEP_PIN_ACT LOW
#endif /* Pins_Arduino_h */

#else // pins_arduino.h for T-Embed NON CC1101
// ********************************************************************
// ********************************************************************
// ********************************************************************
// ********************************************************************
#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// Lite Version
// #define LITE_VERSION 1

// Main I2C Bus
#define SPI_SS_PIN 16 // NFC_CS
#define SPI_MOSI_PIN 41
#define SPI_MISO_PIN 38 // https://github.com/pr3y/Bruce/issues/1130
#define SPI_SCK_PIN 40
static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t SCK = SPI_MISO_PIN;
static const uint8_t MISO = SPI_SCK_PIN;

// Set Main I2C Bus
#define GROVE_SDA 44
#define GROVE_SCL 43
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// TFT_eSPI display
#define ST7789_DRIVER 1
#define USER_SETUP_LOADED
#define TFT_BACKLIGHT_ON 1
#define USE_HSPI_PORT
#define SMOOTH_FONT 1
#define TFT_WIDTH 170
#define TFT_HEIGHT 320
#define TFT_BL 15
#define TFT_MISO -1
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS 10
#define TFT_DC 13
#define TFT_RST 9
#define TOUCH_CS -1
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

// Display Setup#
#define HAS_SCREEN
#define ROTATION 3
#define MINBRIGHT (uint8_t)1
#define PIN_POWER_ON 46

// Font Sizes#
#define FP 1
#define FM 2
#define FG 3

// SD Card#
#define SDCARD_CS 39
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN
#define SDCARD_SCK SPI_SCK_PIN

// NRF24 - Over QWIIC Port #
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 44
#define NRF24_SS_PIN 43
#define NRF24_MOSI_PIN SDCARD_MOSI
#define NRF24_SCK_PIN SDCARD_SCK
#define NRF24_MISO_PIN SDCARD_MISO

// CC1101#
#define USE_CC1101_VIA_SPI
#define CC1101_SW1_PIN 43
#define CC1101_SW0_PIN 44
#define CC1101_GDO0_PIN 44
#define CC1101_SS_PIN 43
#define CC1101_MISO_PIN SPI_MISO_PIN
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN

// W5500 over QWIIC port
#define USE_W5500_VIA_SPI
#define W5500_SS_PIN 43
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_INT_PIN 44

// Mic#
#define PIN_CLK 21
#define PIN_DATA 14

// Speaker#
#define HAS_NS4168_SPKR
#define BCLK 7
#define WCLK 5
#define DOUT 6
#define MCLK 39

// Serial
#define SERIAL_TX 43
#define SERIAL_RX 44
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX
#define ANALOG_BAT_PIN 4

// Encoder
#define HAS_ENCODER
#define ENCODER_INA 2
#define ENCODER_INB 1
#define ENCODER_KEY 0
#define HAS_BTN 1
#define BTN_ALIAS "\"Mid\""
#define SEL_BTN ENCODER_KEY
#define UP_BTN 1
#define DW_BTN 1
#define BTN_ACT LOW

// IR
#define TXLED 44
#define RXLED 43
#define LED_ON HIGH
#define LED_OFF LOW

// RF Module

// FM
#define FM_SI4713
// #define FM_RSTPIN 40

// RGB LED
#define HAS_RGB_LED 1
#define RGB_LED 42
#define RGB_LED_CLK 45
#define LED_TYPE APA102
#define LED_ORDER BGR
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 8
#define LED_COLOR_STEP 5
#define HAS_ENCODER_LED 1

// BadUSB
#define USB_as_HID 1

// Deepsleep
#define DEEPSLEEP_WAKEUP_PIN 0
#define DEEPSLEEP_PIN_ACT LOW

#endif /* Pins_Arduino_h */

#endif
