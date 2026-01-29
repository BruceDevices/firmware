#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

/*
 * ESP-WROOM-32 U.FL (External Antenna) Board
 * 
 * This is a generic ESP-WROOM-32 development board with U.FL antenna connector
 * for external antenna connection. Uses standard ESP-WROOM-32 pinout.
 * 
 * Features:
 * - ESP32-WROOM-32 module
 * - External U.FL antenna connector (WiFi/BT)
 * - 4MB Flash
 * - 240MHz Dual Core
 * - WiFi + Bluetooth Classic + BLE
 * 
 * Pin assignments follow the ESP-WROOM-32 reference design.
 */

// ============================================================================
// SPI Bus Configuration (VSPI - Default SPI for peripherals)
// ============================================================================
#define SPI_SS_PIN   5
#define SPI_MOSI_PIN 23
#define SPI_MISO_PIN 19
#define SPI_SCK_PIN  18

static const uint8_t SS   = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK  = SPI_SCK_PIN;

// ============================================================================
// SD Card Configuration (No onboard SD - uses SPI pins for external SD)
// ============================================================================
#define SDCARD_CS   SPI_SS_PIN
#define SDCARD_SCK  SPI_SCK_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN

// ============================================================================
// CC1101 Sub-GHz Radio Configuration
// ============================================================================
#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN  4
#define CC1101_SS_PIN    15
#define CC1101_MOSI_PIN  SPI_MOSI_PIN
#define CC1101_SCK_PIN   SPI_SCK_PIN
#define CC1101_MISO_PIN  SPI_MISO_PIN

// ============================================================================
// NRF24L01 2.4GHz Radio Configuration
// ============================================================================
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN     16
#define NRF24_SS_PIN     17
#define NRF24_MOSI_PIN   SPI_MOSI_PIN
#define NRF24_SCK_PIN    SPI_SCK_PIN
#define NRF24_MISO_PIN   SPI_MISO_PIN

// ============================================================================
// W5500 Ethernet Configuration
// ============================================================================
#define USE_W5500_VIA_SPI
#define W5500_SS_PIN     17
#define W5500_MOSI_PIN   SPI_MOSI_PIN
#define W5500_SCK_PIN    SPI_SCK_PIN
#define W5500_MISO_PIN   SPI_MISO_PIN
#define W5500_INT_PIN    16

// ============================================================================
// I2C Bus Configuration (Grove connector compatible)
// ============================================================================
#define GROVE_SDA 21
#define GROVE_SCL 22
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// ============================================================================
// Serial/UART Configuration
// ============================================================================
static const uint8_t TX = 1;
static const uint8_t RX = 3;

#define SERIAL_TX 12
#define SERIAL_RX 13

#define BAD_TX 12
#define BAD_RX 13

// ============================================================================
// This board has NO built-in TFT display
// The firmware will use WebUI mode by default
// ============================================================================
// TFT pins set to -1 to indicate no display present
#define TFT_CS    -1
#define TFT_DC    -1
#define TFT_RST   -1
#define TFT_BL    -1
#define TFT_MOSI  -1
#define TFT_SCLK  -1
#define TOUCH_CS  -1
#define BACKLIGHT -1

// Display settings (WebUI mode - no physical screen)
#define HAS_SCREEN    0
#define ROTATION      0
#define MINBRIGHT     0

// Font Sizes (unused but defined for compatibility)
#define FP 1
#define FM 2
#define FG 3

// ============================================================================
// Button Configuration (No built-in buttons - using GPIO0 for boot button)
// ============================================================================
#define BTN_ALIAS   "\"Boot\""
#define HAS_BTN     1
#define BTN_PIN     0    // Boot button on most ESP32 dev boards
#define BTN_ACT     LOW

// ============================================================================
// IR LED Configuration (External IR module)
// ============================================================================
#define TXLED 2         // GPIO2 - onboard LED can be used as IR TX
#define RXLED 14        // GPIO14 - IR receiver input

#define LED_ON  HIGH
#define LED_OFF LOW

// ============================================================================
// Onboard LED
// ============================================================================
#define LED_BUILTIN 2   // GPIO2 - most ESP32 boards have LED on GPIO2

#endif /* Pins_Arduino_h */
