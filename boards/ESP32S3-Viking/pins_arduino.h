#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// =============================================================
// ESP32S3-Viking: ESP32-S3 N16R8 + ILI9341 2.8" SPI + XPT2046
// =============================================================
// Hardware:
//   - ESP32-S3-WROOM-1 N16R8 (16MB Flash, 8MB OPI PSRAM)
//   - ESP32-S3 GPIO Extension Board KIT B
//   - ILI9341 2.8" SPI TFT 240x320 (TENSTAR ROBOT)
//   - XPT2046 Resistive Touch (shared SPI bus)
//   - PN532 NFC/RFID (SPI - osobna magistrala)
//   - 433MHz ASK RF TX+RX modules (prawa strona KIT B)
//
// WIRING TFT (lewa strona KIT B, od gory do dolu):
// KIT B pin | GPIO  | -> | Screen pin
// ----------+-------+----+------------------
// GND       |  -    | -> | GND
// 3V3       |  -    | -> | VCC
// IO4       |  4    | -> | LCD_RST
// IO5       |  5    | -> | LCD_CS
// IO6       |  6    | -> | LCD_DC (RS)
// IO7       |  7    | -> | LCD_MOSI (SDI)
// IO15      | 15    | -> | LCD_SCK (CLK)
// IO16      | 16    | -> | LCD_MISO (SDO) [optional]
// IO17      | 17    | -> | LCD_BL (LED)
// IO18      | 18    | -> | T_CS
// IO8       |  8    | -> | T_IRQ
// (shared)  |  7    | -> | T_DIN  = LCD_MOSI
// (shared)  | 15    | -> | T_CLK  = LCD_SCK
// (shared)  | 16    | -> | T_DO   = LCD_MISO
//
// WIRING PN532 SPI (lewa strona KIT B, dolny blok):
// IO9       |  9    | -> | PN532 SCK
// IO10      | 10    | -> | PN532 MISO
// IO11      | 11    | -> | PN532 MOSI
// IO12      | 12    | -> | PN532 SS (CS)
// IO13      | 13    | -> | PN532 IRQ
// IO14      | 14    | -> | PN532 RSTO
//
// WIRING RF 433MHz (prawa strona KIT B):
// IO19      | 19    | -> | RF TX DATA (nadajnik)
// IO20      | 20    | -> | RF RX DATA (odbiornik)
// =============================================================

#ifndef DEVICE_NAME
#define DEVICE_NAME "ESP32S3-Viking"
#endif

// =============================================
// USB
// =============================================
#define USB_VID 0x303a
#define USB_PID 0x1001

// =============================================
// UART0
// =============================================
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// =============================================
// SPI Bus (shared: ILI9341 display + XPT2046 touch)
// Using sequential GPIO block on left side of KIT B
// =============================================
#define TFT_MOSI_PIN  7   // IO7  -> LCD_MOSI / T_DIN
#define TFT_SCLK_PIN 15   // IO15 -> LCD_SCK  / T_CLK
#define TFT_MISO_PIN 16   // IO16 -> LCD_MISO / T_DO

static const uint8_t SS   = 5;
static const uint8_t MOSI = TFT_MOSI_PIN;
static const uint8_t SCK  = TFT_SCLK_PIN;
static const uint8_t MISO = TFT_MISO_PIN;

// =============================================
// PN532 NFC/RFID - SPI (osobna magistrala)
// Lewa strona KIT B, dolny blok GPIO 9-14
// =============================================
#define PN532_SCK_PIN   9   // IO9  -> PN532 SCK
#define PN532_MISO_PIN 10   // IO10 -> PN532 MISO
#define PN532_MOSI_PIN 11   // IO11 -> PN532 MOSI
#define PN532_SS_PIN   12   // IO12 -> PN532 SS (CS)
#define PN532_IRQ_PIN  13   // IO13 -> PN532 IRQ
#define PN532_RST_PIN  14   // IO14 -> PN532 RSTO

// =============================================
// SD Card - not used
// =============================================
#define SDCARD_CS   -1
#define SDCARD_SCK  -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

// =============================================
// External SPI modules (CC1101, NRF24, W5500)
// Using right side of KIT B: IO40/IO41/IO42
// =============================================
#define SPI_SCK_PIN  15
#define SPI_MOSI_PIN  7
#define SPI_MISO_PIN 16
#define SPI_SS_PIN   40

#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN   41
#define CC1101_SS_PIN     40
#define CC1101_MOSI_PIN   SPI_MOSI_PIN
#define CC1101_SCK_PIN    SPI_SCK_PIN
#define CC1101_MISO_PIN   SPI_MISO_PIN

#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN      42
#define NRF24_SS_PIN      40
#define NRF24_MOSI_PIN    SPI_MOSI_PIN
#define NRF24_SCK_PIN     SPI_SCK_PIN
#define NRF24_MISO_PIN    SPI_MISO_PIN

#define USE_W5500_VIA_SPI
#define W5500_SS_PIN      -1
#define W5500_MOSI_PIN    SPI_MOSI_PIN
#define W5500_SCK_PIN     SPI_SCK_PIN
#define W5500_MISO_PIN    SPI_MISO_PIN
#define W5500_INT_PIN     -1

// =============================================
// TFT Display: ILI9341 2.8" SPI 240x320
// Wired to left side of KIT B (sequential GPIOs)
// =============================================
#define USER_SETUP_LOADED
#define ILI9341_2_DRIVER  1
#define TFT_INVERSION_ON  0
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MISO  TFT_MISO_PIN  // IO16
#define TFT_MOSI  TFT_MOSI_PIN  // IO7
#define TFT_SCLK  TFT_SCLK_PIN  // IO15
#define TFT_CS    5              // IO5 -> LCD_CS
#define TFT_DC    6              // IO6 -> LCD_DC
#define TFT_RST   4              // IO4 -> LCD_RST
#define TFT_BL   17              // IO17 -> LCD_BL (backlight)
#define TFT_BACKLIGHT_ON HIGH
#define SMOOTH_FONT 1

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// =============================================
// Display Setup
// =============================================
#define HAS_SCREEN  1
#define ROTATION    1     // Landscape 320x240
#define MINBRIGHT   1
#define BACKLIGHT  17

// =============================================
// Touch Screen: XPT2046 Resistive (shared SPI)
// =============================================
#define HAS_TOUCH  1
#define TOUCH_CS  18   // IO18 -> T_CS
#define TOUCH_IRQ  8   // IO8  -> T_IRQ

#define XPT2046_X_MIN  200
#define XPT2046_X_MAX 3900
#define XPT2046_Y_MIN  200
#define XPT2046_Y_MAX 3900

// =============================================
// Font Sizes
// =============================================
#define FP 1
#define FM 2
#define FG 3

// =============================================
// RGB LED - WS2812 on IO48
// =============================================
#define HAS_RGB_LED        1
#define RGB_LED           48
#define LED_TYPE      WS2812B
#define LED_ORDER         GRB
#define LED_TYPE_IS_RGBW    0
#define LED_COUNT           1
#define LED_COLOR_STEP      5

// =============================================
// Buttons (BOOT button = IO0)
// =============================================
#define HAS_BTN    1
#define BTN_ALIAS  '"Boot"'
#define BTN_PIN    0
#define BTN_ACT    LOW
#define SEL_BTN    0

// =============================================
// Audio - not available
// =============================================

// =============================================
// Battery ADC - not available
// =============================================
#define ANALOG_BAT_PIN        -1
#define ANALOG_BAT_MULTIPLIER 2.0f

// =============================================
// Infrared TX/RX (right side of KIT B)
// IO38 = TX, IO39 = RX
// =============================================
#define TXLED  38
#define RXLED  39
#define LED_ON   HIGH
#define LED_OFF  LOW
#define IR_TX_PINS {{"GPIO38", 38}, {"GPIO39", 39}}
#define IR_RX_PINS {{"GPIO38", 38}, {"GPIO39", 39}}

// =============================================
// RF 433MHz ASK (prawa strona KIT B)
// IO19 = TX DATA -> nadajnik
// IO20 = RX DATA -> odbiornik
// Zasilanie modulow: VCC -> 5V (lub 3.3V), GND -> GND
// =============================================
#define RF_TX_PINS {{"GPIO19", 19}, {"GPIO20", 20}}
#define RF_RX_PINS {{"GPIO19", 19}, {"GPIO20", 20}}
#define RF_TX_DEFAULT_PIN 19  // IO19 = TX DATA (nadajnik)
#define RF_RX_DEFAULT_PIN 20  // IO20 = RX DATA (odbiornik)

// =============================================
// Serial / GPS
// =============================================
#define SERIAL_TX  43
#define SERIAL_RX  44
#define GPS_SERIAL_TX  SERIAL_TX
#define GPS_SERIAL_RX  SERIAL_RX

// =============================================
// BadUSB (USB HID via native USB)
// =============================================
#define USB_as_HID  1
#define BAD_TX  3
#define BAD_RX  46

// =============================================
// Deep Sleep wakeup on BOOT button
// =============================================
#define DEEPSLEEP_WAKEUP_PIN  0
#define DEEPSLEEP_PIN_ACT     LOW

#endif /* Pins_Arduino_h */
