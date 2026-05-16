#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME "JC4827W543 4.3\" ESP32-S3"
#endif

// =============================================
// UART0
// =============================================
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// =============================================
// I2C Bus (for future expansion - GT911 capacitive touch alternative)
// =============================================
#define GROVE_SDA 8
#define GROVE_SCL 4
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// =============================================
// Main SPI Bus (for CC1101, NRF24, W5500 external modules)
// =============================================
#define SPI_SCK_PIN 47
#define SPI_MOSI_PIN 21
#define SPI_MISO_PIN 48
#define SPI_SS_PIN 45

static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t SCK = SPI_SCK_PIN;
static const uint8_t MISO = SPI_MISO_PIN;

// =============================================
// SD Card - Using SDIO mode
// SD_CLK=38, SD_CMD=40, SD_D0=39, SD_D1=41, SD_D2=48, SD_D3=47
// =============================================
#define SDCARD_CS -1
#define SDCARD_SCK 38
#define SDCARD_MISO 40
#define SDCARD_MOSI 39
#define SDCARD_DETECT -1

#define USE_SDIO

// =============================================
// CC1101 SPI Radio
// =============================================
#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 10
#define CC1101_SS_PIN 9
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

// =============================================
// NRF24L01 Radio
// =============================================
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 6
#define NRF24_SS_PIN 7
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// =============================================
// W5500 Ethernet
// =============================================
#define USE_W5500_VIA_SPI
#define W5500_SS_PIN -1
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_INT_PIN -1

// =============================================
// TFT Display (NV3041A driver)
// =============================================
#define USER_SETUP_LOADED
#define NV3041A_DRIVER 1
#define TFT_RGB_ORDER TFT_RGB
#define TFT_INVERSION_ON
#define TFT_PARALLEL_8_BIT
#define SMOOTH_FONT 1

#define TFT_WIDTH 480
#define TFT_HEIGHT 272
#define TFT_BL 1
#define TFT_CS 45
#define TFT_RST -1  // Not connected, using software reset
#define TFT_DC 2
#define TFT_D0 39
#define TFT_D1 40
#define TFT_D2 41
#define TFT_D3 48
#define TFT_D4 21
#define TFT_D5 47
#define TFT_D6 46
#define TFT_D7 6
#define TFT_WR 7
#define TFT_RD 8

#define TFT_BACKLIGHT_ON 1
#define INIT_SEQUENCE_3
#define CGRAM_OFFSET

#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000

// Display Setup
#define HAS_SCREEN
#define ROTATION 0
#define MINBRIGHT (uint8_t)1

// Font Sizes
#define FP 1
#define FM 2
#define FG 3

// =============================================
// XPT2046 Resistive Touch (original pins from vendor)
// =============================================
#define HAS_TOUCH
#define USE_XPT2046_TOUCH
#define TOUCH_XPT2046
#define TOUCH_XPT2046_SCK 12  // Original vendor pin
#define TOUCH_XPT2046_MISO 13  // Original vendor pin  
#define TOUCH_XPT2046_MOSI 11  // Original vendor pin
#define TOUCH_XPT2046_CS 38   // Original vendor pin
#define TOUCH_XPT2046_INT 3   // Original vendor pin
#define TOUCH_XPT2046_ROTATION 0
#define TOUCH_XPT2046_SAMPLES 10

// =============================================
// Power & Battery
// =============================================
#define PIN_POWER_ON 5

// =============================================
// LED / Status
// =============================================
#define LED_ON HIGH
#define LED_OFF LOW

// =============================================
// Buttons - Board has touchscreen, buttons optional
// Using safest free GPIO (14 only as it doesn't conflict with anything critical)
// =============================================
#define HAS_3_BUTTONS
#define BTN_ALIAS "\"OK\""
#define SEL_BTN 14     // GPIO 14 - only button (avoid strapping pins 0,3)
#define UP_BTN -1      // Not used (touchscreen available)
#define DW_BTN -1      // Not used (touchscreen available)
#define BTN_ACT LOW

// =============================================
// USB HID (BadUSB)
// =============================================
#define USB_as_HID 1

// =============================================
// IR/RF Additional GPIO (for IR RX LED)
// =============================================
#define ALLOW_ALL_GPIO_FOR_IR_RF 1

#endif /* Pins_Arduino_h */