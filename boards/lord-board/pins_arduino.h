#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// ---------------- CONFIG --------------------
#define USB_VID 0x303a
#define USB_PID 0x1001
#define USER_SETUP_LOADED 1
#define USE_HSPI_PORT 1
#define HAS_SCREEN 1
#define HAS_TOUCH 1
#define FP 1
#define FM 2
#define FG 3

// ---------------- UART --------------------
// UART0: Programming via CH343 USB-Serial (default)
static const uint8_t TX = 43; // CH343 USB-Serial TX
static const uint8_t RX = 44; // CH343 USB-Serial

// UART1: GPS NEO‑6M
static const uint8_t GPS_TX = 41; // ESP32 TX1 -> GPS RX
static const uint8_t GPS_RX = 40; // ESP32 RX1 <- GPS TX

//------------------BAD USB ----------------
#define SERIAL_RX 44 // CH343 USB-serial RX
#define SERIAL_TX 43 // CH343 USB-serial TX
#define BAD_RX SERIAL_RX
#define BAD_TX SERIAL_TX
#define USB_as_HID 1

// ---------------- I2C -------------------------
// PN532 NFC (I2C)
static const uint8_t SDA = 45;
static const uint8_t SCL = 42; // Safe choice, avoids SPI/Touch conflicts

// ---------------- Shared VSPI Bus ---------------
// VSPI for TFT (ILI9341), Touch (XPT2046), SD card
static const uint8_t SS = 14;
static const uint8_t SCK = 13;
static const uint8_t MOSI = 12;
static const uint8_t MISO = 11;
#define SPI_FREQUENCY 20000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000

// ---------------- ILI9341 TFT ---------------
#define ILI9341_DRIVER 1
#define TFT_RGB_ORDER 0
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_BACKLIGHT_ON 1
#define TFT_BL 10
#define TFT_RST 9
#define TFT_DC 3
#define TFT_MISO MISO
#define TFT_MOSI MOSI
#define TFT_SCLK SCK
#define TFT_CS 8
#define SMOOTH_FONT 1
#define ROTATION 1
#define MINBRIGHT (uint8_t)1

// ---------------- XPT2046 Touch  ---------------
#define TOUCH_XPT2046_SPI
#define TOUCH_CS 38
#define TOUCH_IRQ 39
#define XPT2046_TOUCH_CONFIG_INT_GPIO_NUM 39
#define XPT2046_SPI_BUS_MISO_IO_NUM MISO
#define XPT2046_SPI_BUS_MOSI_IO_NUM MOSI
#define XPT2046_SPI_BUS_SCLK_IO_NUM SCK
#define XPT2046_SPI_CONFIG_CS_GPIO_NUM 38

// ---------------- SD CARD --------------------
#define SDCARD_CS 21
#define SDCARD_SCK SCK
#define SDCARD_MISO MISO
#define SDCARD_MOSI MOSI

// ----------------Onboard RGB LED --------------------
#define LED_BUILTIN 48 // WS2812 data pin
#define HAS_RGB_LED 1
#define RGB_LED 48
#define LED_TYPE WS2812B
#define LED_ORDER GRB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 1
#define LED_COLOR_STEP 15

// ----------------Button --------------------
#define HAS_BTN 0
#define BTN_ALIAS "\"Ok\""
#define BTN_PIN -1

// ---------------- Shared HSPI Bus ---------------
// HSPI for CC1101 , NRF24+
static const uint8_t SPI_SCK_PIN = 18;
static const uint8_t SPI_MOSI_PIN = 17;
static const uint8_t SPI_MISO_PIN = 16;

// ---------------- CC1101 Radio ----------------
#define USE_CC1101_VIA_SPI
#define CC1101_SS_PIN 15
#define CC1101_GDO0_PIN 7 // No longer conflict with TFT_BL
#define CC1101_GDO2_PIN 6 // Moved from GPIO4 → avoids backlight & reserved pins
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

// ---------------- nRF24L01+ Radio --------------
#define USE_NRF24_VIA_SPI
#define NRF24_SS_PIN 5
#define NRF24_CE_PIN 4
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// ---------------- IR Transmitter/Receiver -----
#define GROVE_SDA 45
#define GROVE_SCL 42
#define IR_RX_PIN 1 //  digital input (connect IR receiver OUT here)
#define IR_TX_PIN 2 //  digital output (PWM capable, for IR LED)
#define RXLED 1
#define LED 2 // Renamed from LED to avoid conflicts
#define LED_ON HIGH
#define LED_OFF LOW

// ---------------- ALIAS -----
#define SPI_SS_PIN TFT_CS

#endif /* Pins_Arduino_h */
