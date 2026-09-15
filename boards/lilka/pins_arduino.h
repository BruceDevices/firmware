#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// =====================================================================
//  Lilka v2.3  —  ESP32-S3-WROOM-1-N16R8
//  Verified against the v2.3 schematic + lilka-dev/sdk + docs.lilka.dev
//  No onboard: RGB LED, microphone, PMIC/fuel-gauge, IO expander, touchscreen.
//  (simple TLV1117 LDO + TP4056, battery sensed via ADC.)
//  External modules (CC1101 / NRF24 / LoRa / RC522 / PN532 / IR-RF) attach
//  to the 12-pin extension header.
// =====================================================================

// ===== USB =====
#define USB_VID 0x303A
#define USB_PID 0x1001

// ===== SERIAL (on the extension header) + BadUSB + GPS =====
#define SERIAL_TX 43
#define SERIAL_RX 44
static const uint8_t TX = SERIAL_TX;
static const uint8_t RX = SERIAL_RX;
#define TX1 TX
#define RX1 RX
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX
#define USB_as_HID 1
#define BAD_TX SERIAL_TX
#define BAD_RX SERIAL_RX

// ===== I2C =====
// GROVE_SDA/SCL is just Bruce's macro name for the "default I2C pins"
// (Lilka has no Grove connector). Values are a pair from the extension header.
// NOTE: overlaps the module bus below (only 6 GPIO on the header).
#define GROVE_SDA 13
#define GROVE_SCL 14
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// ===== MAIN SPI bus (internal): ST7789 display + SD card =====
// Arduino-default SPI. Display is write-only; MISO is only for the SD card.
static const uint8_t SCK  = 18;
static const uint8_t MOSI = 17;
static const uint8_t MISO = 8;
static const uint8_t SS   = 16; // SD CS

// ===== SECOND SPI bus: modules on the extension header (HSPI) =====
// Requires -DUSE_HSPI_PORT=1 in lilka.ini
#define SPI_SCK_PIN  12
#define SPI_MOSI_PIN 14
#define SPI_MISO_PIN 13
#define SPI_SS_PIN   47

// ===== BUTTONS (active LOW, internal pull-ups) =====
#define BTN_ALIAS "\"A\""
#define HAS_5_BUTTONS
#define UP_BTN  38
#define DW_BTN  41
#define L_BTN   39
#define R_BTN   40
#define SEL_BTN 5  // A = OK   (alternative: Select=0)
#define ESC_BTN 6  // B = back (alternative: Start=4)
#define BTN_ACT LOW
// Spare Lilka buttons: Start 4, Select 0, C 10, D 9 — bind in interface.cpp

// ===== BATTERY (via ADC, no PMIC) =====
#define ANALOG_BAT_PIN 3 // ADC1_CH2

// ===== BUZZER =====
// Piezo on GPIO11 (schematic). BUZZ_PIN macro is set in lilka.ini: -DBUZZ_PIN=11

// ===== FONTS =====
#define FP 1
#define FM 2
#define FG 3

// ===== TFT_eSPI (ST7789, 1.69" IPS, 280x240) =====
#define HAS_SCREEN 1
#define ROTATION 3             // landscape (try 1 if image ends up flipped 180°)
#define MINBRIGHT (uint8_t)1
#define USER_SETUP_LOADED 1
#define ST7789_DRIVER 1
#define TFT_WIDTH 240          // init width (portrait)
#define TFT_HEIGHT 280         // init height -> 20px CGRAM offset applied AUTOMATICALLY (driver)
#define TFT_INVERSION_ON       // confirmed by working ESPHome (invert_colors: true)
#define TFT_RGB_ORDER 0        // 0 = BGR (confirmed by ESPHome color_order: BGR); 1 = RGB
#define TFT_BACKLIGHT_ON 1
#define TFT_BL 46              // SLEEP net — backlight, active HIGH
#define TFT_RST -1             // tied to system reset
#define TFT_DC 15
#define TFT_CS 7
#define TFT_MOSI 17
#define TFT_SCLK 18
#define TFT_MISO -1            // display is write-only
#define TOUCH_CS -1
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000

// ===== SD CARD (main bus) =====
#define SDCARD_CS 16
#define SDCARD_SCK 18
#define SDCARD_MISO 8
#define SDCARD_MOSI 17

// ===== IR (extension header) =====
// RXLED is the default IR-RX pin (used by interface.cpp irRx); the lists let
// the user pick the pin from the Bruce menu.
#define RXLED 21
#define TXLED 21
#define LED_ON  HIGH
#define LED_OFF LOW
#define IR_TX_PINS {{"GPIO 12", 12}, {"GPIO 13", 13}, {"GPIO 14", 14}, {"GPIO 21", 21}, {"GPIO 47", 47}, {"GPIO 48", 48}}
#define IR_RX_PINS {{"GPIO 12", 12}, {"GPIO 13", 13}, {"GPIO 14", 14}, {"GPIO 21", 21}, {"GPIO 47", 47}, {"GPIO 48", 48}}

// ===== RF (sub-GHz one-pin modules) — same pin choices as IR =====
#define RF_TX_PINS IR_TX_PINS
#define RF_RX_PINS IR_RX_PINS

// ===== CC1101 (sub-GHz, module bus) =====
#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 21
#define CC1101_GDO2_PIN 48
#define CC1101_SS_PIN 47
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

// ===== NRF24 (module bus) =====
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 21
#define NRF24_SS_PIN 47
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// ===== LoRa (external module on the extension header) =====
// Enabled simply by defining LORA_SCK. Pins for an SX126x-class module
// (note: BUSY/IRQ reuse the UART pins 43/44).
#define LORA_SCK  14
#define LORA_MISO 47
#define LORA_MOSI 21
#define LORA_CS   13
#define LORA_RST  48
#define LORA_BUSY 43
#define LORA_IRQ  44
// #define LORA_DIO0 -1   // only for SX127x-class radios; leave out for SX126x

// ===== RFID: RC522 / PN532 =====
// Nothing to define here — both are chosen at runtime in the RFID menu:
//   * "RC522 on SPI" -> reuses the module SPI bus (CS = SPI_SS_PIN + SPI_SCK/MISO/MOSI_PIN)
//   * "PN532 on I2C" -> reuses the I2C bus (GROVE_SDA / GROVE_SCL)

// ===== Deep sleep: wake on Select button (GPIO0) =====
#define DEEPSLEEP_WAKEUP_PIN 0
#define DEEPSLEEP_PIN_ACT LOW

#endif /* Pins_Arduino_h */
