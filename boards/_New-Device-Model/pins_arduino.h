#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

static const uint8_t TX = 43;
static const uint8_t RX = 44;

static const uint8_t TXD2 = 1;
static const uint8_t RXD2 = 2;

static const uint8_t SDA = 13;
static const uint8_t SCL = 15;

// Modified elsewhere
static const uint8_t SS = -1;
static const uint8_t MOSI = -1;
static const uint8_t MISO = -1;
static const uint8_t SCK = -1;

static const uint8_t G0 = 0;
static const uint8_t G1 = 1;
static const uint8_t G2 = 2;
static const uint8_t G3 = 3;
static const uint8_t G4 = 4;
static const uint8_t G5 = 5;
static const uint8_t G6 = 6;
static const uint8_t G7 = 7;
static const uint8_t G8 = 8;
static const uint8_t G9 = 9;
static const uint8_t G10 = 10;
static const uint8_t G11 = 11;
static const uint8_t G12 = 12;
static const uint8_t G13 = 13;
static const uint8_t G14 = 14;
static const uint8_t G15 = 15;
static const uint8_t G39 = 39;
static const uint8_t G40 = 40;
static const uint8_t G41 = 41;
static const uint8_t G42 = 42;
static const uint8_t G43 = 43;
static const uint8_t G44 = 44;
static const uint8_t G46 = 46;

static const uint8_t ADC1 = 7;
static const uint8_t ADC2 = 8;

#define FM_RSTPIN 40
#define PIN_CLK 43
#define I2S_SCLK_PIN 43
#define I2S_DATA_PIN 46
#define PIN_DATA 46

#define RGB_LED 21

#define BCLK 41
#define WCLK 43
#define DOUT 42

#define BAD_TX GROVE_SDA
#define BAD_RX GROVE_SCL

#define HAS_BTN 0
#define BTN_ALIAS "\"Ok\""
#define BTN_PIN 0
#define BTN_ACT LOW

#define IR_TX_PINS '{{"M5 IR Mod", GROVE_SDA}, {"Pin 1", 1}, {"Pin 2", 2}}'
#define IR_RX_PINS '{{"M5 IR Mod", GROVE_SCL}, {"Pin 1", 1}, {"Pin 2", 2}}'
#define TXLED -1
#define LED_ON HIGH
#define LED_OFF LOW

#define RF_TX_PINS '{{"M5 RF433T", GROVE_SDA}, {"Pin 1", 1}, {"Pin 2", 2}}'
#define RF_RX_PINS '{{"M5 FR433R", GROVE_SCL}, {"Pin 1", 1}, {"Pin 2", 2}}'

#define CC1101_GDO0_PIN 9
#define CC1101_SS_PIN 10
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define NRF24_CE_PIN 6
#define NRF24_SS_PIN 7
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

#define FP 1
#define FM 2
#define FG 3

#define HAS_SCREEN 1
#define ROTATION 1
#define MINBRIGHT 160

#define USER_SETUP_LOADED 1
#define USE_HSPI_PORT 1
#define ST7789_2_DRIVER 1
#define TFT_RGB_ORDER 1
#define TFT_WIDTH 135
#define TFT_HEIGHT 240
#define TFT_BACKLIGHT_ON 1
#define TFT_BL 38
#define TFT_RST 33
#define TFT_DC 34
#define TFT_MOSI 35
#define TFT_SCLK 36
#define TFT_CS 37
#define TOUCH_CS -1
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 20000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

#define SDCARD_CS 12
#define SDCARD_SCK 40
#define SDCARD_MISO 39
#define SDCARD_MOSI 14

#define GROVE_SDA 2
#define GROVE_SCL 1

#define SPI_SCK_PIN 12
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SS_PIN 10

#endif /* Pins_Arduino_h */
