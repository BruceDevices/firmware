#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// LilyGO TTGO T4 v1.3 (ESP32-WROVER, 2.4" ILI9341 320x240, microSD, 3 buttons)
// Display and SD live on two separate SPI buses:
//   * TFT  -> VSPI  (MOSI 23, MISO 12, SCLK 18)
//   * SD   -> HSPI  (MOSI 15, MISO  2, SCLK 14)

// ------------------------------------------------------------------ Battery
// The T4 v1.3 has no dedicated on-board battery ADC divider, so getBattery()
// falls back to the shared default implementation in the core.

// ------------------------------------------------------------------ Main I2C (Grove)
#define GROVE_SDA 21
#define GROVE_SCL 22
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// ------------------------------------------------------------------ Secondary SPI (external RF modules)
// No RF module is fitted on the T4; these are sane defaults on free GPIOs so
// CC1101 / NRF24 modules can be wired manually. ALLOW_ALL_GPIO_FOR_IR_RF lets
// the user re-map any of these at runtime.
#define SPI_SCK_PIN 25
#define SPI_MOSI_PIN 26
#define SPI_MISO_PIN 33
#define SPI_SS_PIN 17

// Arduino default SPI globals (used by a bare SPI.begin())
static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK = SPI_SCK_PIN;

#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 34 // input-only, fine for GDO0 (input)
#define CC1101_SS_PIN SPI_SS_PIN
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 35 // input-only, fine for CE (input)
#define NRF24_SS_PIN SPI_SS_PIN
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// ------------------------------------------------------------------ SD card (HSPI)
#define SDCARD_CS 13
#define SDCARD_SCK 14
#define SDCARD_MISO 2
#define SDCARD_MOSI 15

// ------------------------------------------------------------------ TFT_eSPI display (ILI9341, VSPI)
#define USER_SETUP_LOADED
#define ILI9341_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO 12
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 27
#define TFT_DC 32
#define TFT_RST 5
#define TFT_BL 4              // Display backlight control pin
#define TFT_BACKLIGHT_ON HIGH // HIGH or LOW are options
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 40000000     // Maximum for ILI9341
#define SPI_READ_FREQUENCY 16000000

// ------------------------------------------------------------------ Display setup
#define HAS_SCREEN
#define ROTATION 1 // Landscape, buttons on the right edge
#define MINBRIGHT (uint8_t)1

// Font sizes
#define FP 1
#define FM 2
#define FG 3

// ------------------------------------------------------------------ Serial / GPS / BadUSB
// Default to the Grove pins; can be re-mapped from Bruce's settings.
#define SERIAL_TX GROVE_SDA
#define SERIAL_RX GROVE_SCL
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX
#define BAD_TX GROVE_SDA
#define BAD_RX GROVE_SCL

// ------------------------------------------------------------------ Buttons & navigation
// The three front buttons sit on ESP32 input-only GPIOs (34-39), so they rely
// on the board's external pull-ups and are active LOW. See interface.cpp.
//   LEFT   (38) -> Previous / Up
//   CENTER (37) -> Select (click) / Escape (double-click or hold)
//   RIGHT  (39) -> Next / Down
#define BTN_ALIAS "\"Sel\""
#define HAS_3_BUTTONS
#define UP_BTN 38
#define SEL_BTN 37
#define DW_BTN 39
#define BTN_ACT LOW

// ------------------------------------------------------------------ IR default pins
#define TXLED 26
#define RXLED 25
#define LED_ON HIGH
#define LED_OFF LOW

#endif /* Pins_Arduino_h */
