#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// =============================================================================
//  Lilka V2 - ESP32-S3 handheld reference mapping for Bruce
// =============================================================================
#define LILKA_V2_BOARD_NAME "Lilka V2"
#define LILKA_V2_HAS_PSRAM 1

// =============================================================================
//  USB / Serial identifiers
// =============================================================================
#define USB_VID 0x303a
#define USB_PID 0x1001

static const uint8_t TX = 1;
static const uint8_t RX = 2;

#define SERIAL_RX 2
#define SERIAL_TX 1
#define BAD_RX SERIAL_RX
#define BAD_TX SERIAL_TX
#define USB_as_HID 1

// =============================================================================
//  System power control
// =============================================================================
#define SLEEP_PIN 46      // HIGH = awake, LOW = deep sleep
#define DISPLAY_EN_PIN 45 // Display power enable rail

// =============================================================================
//  Navigation + action buttons (active LOW)
// =============================================================================
#define HAS_5_BUTTONS
#define BTN_ALIAS "\"A\""
#define BTN_ACT LOW

#define UP_BTN 38
#define DW_BTN 41
#define L_BTN 39
#define R_BTN 40
#define SEL_BTN 0 // Boot button (also SELECT)

#define START_BTN 4
#define A_BTN 5
#define B_BTN 6 // Shared with ESC/back
#define C_BTN 10
#define D_BTN 9

// File browser helpers
#define FP 1
#define FM 2
#define FG 3

// =============================================================================
//  Peripherals
// =============================================================================
#define BAT_PIN 3   // ADC via 100K/33K divider
#define BUZZ_PIN 11 // Piezo output

// =============================================================================
//  Display (ST7789 240×280)
// =============================================================================
#define HAS_SCREEN 1
#define ROTATION 3
#define MINBRIGHT (uint8_t)1

#define USER_SETUP_LOADED 1
#define ST7789_DRIVER 1
#define USE_HSPI_PORT
#define CGRAM_OFFSET
#define TFT_INVERSION_ON
#define TFT_RGB_ORDER TFT_BGR
#define SMOOTH_FONT 1

#define TFT_WIDTH 240
#define TFT_HEIGHT 280

#define TFT_DC 15
#define TFT_CS 7
#define TFT_MOSI 17
#define TFT_MISO 8
#define TFT_SCLK 18
#define TFT_RST -1
#define TOUCH_CS -1

// =============================================================================
//  Shared SPI / SD interface
// =============================================================================
#define SS 3
#define MOSI 17
#define MISO 8
#define SCK 18

#define SDCARD_CS 16
#define SDCARD_SCK 18
#define SDCARD_MISO 8
#define SDCARD_MOSI 17

// =============================================================================
//  Grove / I2C connector
// =============================================================================
#define GROVE_SDA 47
#define GROVE_SCL 48

static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// =============================================================================
//  SPI2 expansion header (CC1101, NRF24, etc.)
// =============================================================================
#define SPI_SCK_PIN 21
#define SPI_MOSI_PIN 47
#define SPI_MISO_PIN 14
#define SPI_SS_PIN 48

// =============================================================================
//  Optional audio output (I2S)
// =============================================================================
#define I2S_BCLK_PIN 42
#define I2S_DOUT_PIN 2
#define I2S_LRCK_PIN 1

#endif /* Pins_Arduino_h */
