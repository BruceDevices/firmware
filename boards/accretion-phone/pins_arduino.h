#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME "Accretion Phone"
#endif

// =============================================
// USB (native USB on GPIO19/20)
// =============================================
#define USB_VID 0x303a
#define USB_PID 0x1001

// =============================================
// UART0
// =============================================
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// =============================================
// I2C - shared bus with the MPU6050 (0x68)
// =============================================
#define GROVE_SDA 15
#define GROVE_SCL 7
#define SYS_I2C_SDA GROVE_SDA
#define SYS_I2C_SCL GROVE_SCL
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// =============================================
// External-module SPI bus (CC1101 / NRF24 / W5500 ...)
// Uses pins that are NOT used by the display, touch, SD, mic, IMU or PSRAM.
// (41/42/17 were the MAX98357A speaker pins on the original Accretion Phone wiring.)
// =============================================
#define SPI_SCK_PIN 41
#define SPI_MOSI_PIN 42
#define SPI_MISO_PIN 16
#define SPI_SS_PIN 1

static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK = SPI_SCK_PIN;

// =============================================
// SD Card - SDIO 1-bit (SD_MMC). CLK=39 CMD=38 D0=40 (set in interface.cpp)
// =============================================
#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

// =============================================
// Optional external radios on the module bus above
// =============================================
#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 17
#define CC1101_SS_PIN SPI_SS_PIN
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 17
#define NRF24_SS_PIN SPI_SS_PIN
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

#define USE_W5500_VIA_SPI
#define W5500_SS_PIN SPI_SS_PIN
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_INT_PIN 17

// =============================================
// TFT display - ILI9341 240x320 (SPI, TFT_eSPI)
// =============================================
#define USER_SETUP_LOADED
#define ILI9341_DRIVER 1
#define TFT_INVERSION_OFF 1
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS 10
#define TFT_DC 2
#define TFT_RST 14
#define TFT_BL 21
#define TFT_BACKLIGHT_ON HIGH
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 16000000
#define SPI_TOUCH_FREQUENCY 2000000
// Touch is read on its own bus (see accretion-phone.ini), not through TFT_eSPI
#define TOUCH_CS -1

#define HAS_SCREEN 1
#define ROTATION 1 // landscape (same as Accretion Phone's own landscape mode)
#define MINBRIGHT 1
#define BACKLIGHT 21

// =============================================
// Touch - XPT2046 (pins are in accretion-phone.ini)
// =============================================
#define HAS_TOUCH 1

// =============================================
// Font sizes
// =============================================
// Intentionally NOT defined here: the Bruce font-size constants (small/medium/
// large = 1/2/3) come from include/precompiler_flags.h. This header is pulled
// into every library through Arduino.h, and generic global macros with those
// short names collide with library code (FastLED uses `FP` as a type alias).

// =============================================
// RGB LED - WS2812 on GPIO48
// =============================================
#define HAS_RGB_LED 1
#define RGB_LED 48
#define LED_TYPE WS2812B
#define LED_ORDER GRB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 1
#define LED_COLOR_STEP 5

// =============================================
// Battery - ADC on GPIO8, 1:2 divider (default multiplier is 2.0)
// =============================================
#define ANALOG_BAT_PIN 8

// =============================================
// Microphone - INMP441 (I2S). L/R tied to GND (left channel)
// =============================================
#define PIN_CLK 47  // SCK / BCLK
#define PIN_WS 46   // WS / LRCLK
#define PIN_DATA 45 // SD

// =============================================
// Vibration motor (Bruce has no driver for it; the pin is just kept low)
// =============================================
#define ACCRETION_VIB_PIN 18

// =============================================
// Boot button (GPIO0) - used only for wake-up from deep sleep
// =============================================
#define HAS_BTN 0
#define BTN_ALIAS "\"Boot\""
#define BTN_PIN 0
#define BTN_ACT LOW
#define DEEPSLEEP_WAKEUP_PIN 0
#define DEEPSLEEP_PIN_ACT LOW

// =============================================
// IR / RF default pins (external modules on free GPIOs)
// NOTE: in a header these lists must NOT be wrapped in single quotes
// (the quotes are only stripped when passed as -D flags in an .ini file).
// =============================================
#define TXLED 17
#define RXLED 16
#define LED_ON HIGH
#define LED_OFF LOW

#define IR_TX_PINS {{"GPIO17", 17}, {"GPIO16", 16}, {"GPIO1", 1}}
#define IR_RX_PINS {{"GPIO16", 16}, {"GPIO17", 17}, {"GPIO1", 1}}
#define RF_TX_PINS {{"GPIO17", 17}, {"GPIO16", 16}, {"GPIO1", 1}}
#define RF_RX_PINS {{"GPIO16", 16}, {"GPIO17", 17}, {"GPIO1", 1}}

// =============================================
// Serial (GPS) on UART0 pins
// =============================================
#define SERIAL_TX 43
#define SERIAL_RX 44
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX

// =============================================
// BadUSB (USB HID)
// =============================================
#define USB_as_HID 1
#define BAD_TX GROVE_SDA
#define BAD_RX GROVE_SCL

#endif /* Pins_Arduino_h */
