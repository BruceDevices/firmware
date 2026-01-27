#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// SERIAL
#define SERIAL_TX 43
#define SERIAL_RX 44
static const uint8_t TX = SERIAL_TX;
static const uint8_t RX = SERIAL_RX;
#define TX1 TX
#define RX1 RX

// BAD USB
#define USB_as_HID 1

// Main I2C Bus
#define GROVE_SDA 47
#define GROVE_SCL 48

static const uint8_t SDA = 47;
static const uint8_t SCL = 48;

// MAIN SPI BUS
#define SPI_SCK_PIN 17
#define SPI_MOSI_PIN 18
#define SPI_MISO_PIN 8
#define SPI_SS_PIN 11

static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK = SPI_SCK_PIN;

// BUTTONS
#define BTN_ALIAS "\"OK\""
#define HAS_5_BUTTONS
#define SEL_BTN 0
#define UP_BTN 41
#define DW_BTN 40
#define R_BTN 38
#define L_BTN 39
#define ESC_BTN 21
#define BTN_ACT LOW

#define RXLED 1
#define LED 2
#define LED_ON HIGH
#define LED_OFF LOW

// CC1101
#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 46
#define CC1101_SS_PIN 9
// #define cc1101_GDO2_PIN 42
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

// NRF24
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 14
#define NRF24_SS_PIN 13
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// FONT SIZE
#define FP 1
#define FM 2
#define FG 3

// TFT_eSPI display
#define HAS_SCREEN 1
#define ROTATION 1
#define MINBRIGHT (uint8_t)1

#define USER_SETUP_LOADED 1
#define ST7789_DRIVER 1
#define TFT_RGB_ORDER 0
#define TFT_WIDTH 170
#define TFT_HEIGHT 320
#define TFT_BACKLIGHT_ON 1
#define TFT_BL 6
#define TFT_RST 16
#define TFT_DC 15
#define TFT_MISO SPI_MISO_PIN
#define TFT_MOSI SPI_MOSI_PIN
#define TFT_SCLK SPI_SCK_PIN
#define TFT_CS 7
#define TOUCH_CS -1
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000

// SD CARD
#define SDCARD_CS 3
#define SDCARD_SCK SPI_SCK_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN

// RGB LED

#define HAS_RGB_LED 1
#define RGB_LED 45
#define LED_TYPE WS2812B
#define LED_ORDER RGB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 16

#define LED_COLOR_STEP 5

// PMIC
// #define XPOWERS_CHIP_BQ25896

// BOOST ENABLE PMIC 5V OUTPUT
// #define USE_BOOST

#define XPOWERS_CHIP_BQ25896



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

// IO EXPANDER
#define USE_IO_EXPANDER
#define IO_EXPANDER_AW9523
#define IO_EXP_GPS 5
#define IO_EXP_VIBRO 15
#define IO_EXP_CC_RX 9
#define IO_EXP_CC_TX 10
#define IO_EXP_LOGO 0

// BOTTOM SWITCH ONLY PIN 7
// BOTTOM UART PIN SWICH 12
// BRUCE LOGO PIN 0
// NET5   PIN 13

// LoRa

#define LORA_SCK SPI_SCK_PIN
#define LORA_MISO SPI_MISO_PIN
#define LORA_MOSI SPI_MOSI_PIN
#define LORA_CS 4
#define LORA_RST 43///OR 44
#define LORA_BUSY 5

#define LORA_IRQ 42



#endif /* Pins_Arduino_h */
