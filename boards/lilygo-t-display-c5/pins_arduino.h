#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

/* =====================================================================
 * LILYGO T-Display-C5  (ESP32-C5, 1.9" ST7789 170x320, AXP2602 PMU)
 * Bruce board definition — forked from ESP32-C5-tft
 *
 * Physical map (from the LILYGO pinmap):
 *   Display : CS=26  SCK=7  MOSI=9  DC=8  RST=23  BL=25   (write-only, no MISO)
 *   Buttons : BOOT=IO28   IO0=IO0
 *   I2C/QWIIC + PMU : SDA=2  SCL=3   (AXP2602 INT on GPIO10)
 *   UART    : TX=11  RX=12
 * ===================================================================== */

/* ---- Arduino-core RGB LED (devkit default; T-Display-C5 has none — harmless) ---- */
#define PIN_RGB_LED 27
static const uint8_t LED_BUILTIN = SOC_GPIO_PIN_COUNT + PIN_RGB_LED;
#define BUILTIN_LED  LED_BUILTIN
#define RGB_BUILTIN  LED_BUILTIN
#define RGB_BRIGHTNESS 64

static const uint8_t TX = 11;
static const uint8_t RX = 12;

/* I2C — QWIIC connector + AXP2602 PMU */
static const uint8_t SDA = 2;
static const uint8_t SCL = 3;

/* Shared SPI bus (display + radios). Display is write-only, so MISO (6)
 * is only read by the radio modules. */
static const uint8_t SCK  = 7;
static const uint8_t MOSI = 9;
static const uint8_t MISO = 6;
static const uint8_t SS   = 4;   // radio / peripheral CS default

static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;

// LP I2C / LP UART pins are fixed on ESP32-C5
static const uint8_t LP_SDA = 4;
static const uint8_t LP_SCL = 5;
#define WIRE1_PIN_DEFINED
#define SDA1 LP_SDA
#define SCL1 LP_SCL
static const uint8_t LP_RX = 12;
static const uint8_t LP_TX = 11;

/* RGB LED — Bruce's led_control.cpp compiles unconditionally and needs
 * these symbols defined even if the board has no addressable LED. The
 * RGB_LED pin (27) is only driven if a WS2812 is actually present, so
 * leaving this defined is harmless on a board without one. */
#define HAS_RGB_LED 1
#define LED_ORDER GRB
#define LED_TYPE WS2812
#define LED_COUNT 1
#define LED_COLOR_STEP 15
#define RGB_LED 27

/* Communication buses */
#define SERIAL_TX 11
#define SERIAL_RX 12
#define GROVE_SDA 2
#define GROVE_SCL 3
#define SPI_SCK_PIN  7
#define SPI_MOSI_PIN 9
#define SPI_MISO_PIN 6
#define SPI_SS_PIN   4

/* =========================  TFT (ST7789 170x320)  ==================== */
#define HAS_SCREEN 1
#define ROTATION 1              // landscape 320x170; use 0 for portrait
#define MINBRIGHT (uint8_t)1
#define USER_SETUP_LOADED 1

#define ST7789_DRIVER 1
#define TFT_WIDTH  170
#define TFT_HEIGHT 320
#define TFT_RGB_ORDER TFT_BGR   // panel is BGR: without this red<->blue swap
                                // (red shows blue, purple shows pink; green OK)
#define TFT_INVERSION_ON        // confirmed: factory lcd_init() calls invert_color(true)
// Confirmed from the factory sketch: the 170-wide panel needs a 35px gap on
// its short axis (esp_lcd set_gap(0,35)). Recent TFT_eSPI auto-applies this
// for a 170x320 ST7789; if the image is shifted, that's the value to nudge.
#define CGRAM_OFFSET

#define TFT_BACKLIGHT_ON 1
#define TFT_BL   25
#define TFT_RST  23
#define TFT_DC   8
#define TFT_CS   26
#define TFT_MOSI 9
#define TFT_SCLK 7
#define TFT_MISO 6              // display is write-only, but the C5 SPI bus
                                // init wants a real MISO GPIO assigned (matches
                                // the working nm-cyd-c5); shares pin 6 with radios
#define SMOOTH_FONT 1
// 20MHz to match the known-good C5+ST7789 SPI board (nm-cyd-c5). TFT_eSPI's
// C5 SPI path is unreliable at 40MHz (backlight on but no image); the factory
// sketch only reached 40MHz via the native esp_lcd driver, not TFT_eSPI.
#define SPI_FREQUENCY 20000000
#define SPI_READ_FREQUENCY 20000000

/* =====================  Buttons — two-button nav  ==================
 * Only two readable buttons exist; the side "Reset" just reboots the
 * chip and cannot be read as a GPIO. Navigation is done by the custom
 * InputHandler in this board's interface.cpp:
 *     BTN_A short = Next    BTN_A long = Prev
 *     BTN_B short = Select  BTN_B long = Back / Esc                    */
#define HAS_2_BUTTONS
#define BTN_A 0                 // IO0 button
#define BTN_B 28                // BOOT button
#define BTN_ACT LOW
#define DEEPSLEEP_WAKEUP_PIN 0  // IO0 is an RTC/wake-capable pin

/* =========================  InfraRed (optional)  ==================== */
/* IR is optional and this board exposes almost no free GPIO. GPIO27 is
 * taken by RGB_LED above, so IR shares GPIO1 (also the freed nav pin).
 * Rewire these to dedicated pins if you actually fit an IR TX/RX. */
#define TXLED 1                 // IR TX (placeholder — no dedicated free pin)
#define RXLED 1                 // IR RX
#define LED_ON  HIGH
#define LED_OFF LOW

/* =========================  SD Card  ================================ */
/* T-Display-C5 has no microSD slot */
#define SDCARD_CS   -1
#define SDCARD_SCK  -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

/* =========================  External radios  ========================
 * All share the display SPI bus (SCK 7 / MOSI 9 / MISO 6).
 * CC1101, NRF24 and W5500 reuse the SAME CS (4) and control pin (5),
 * so only ONE can be installed at a time unless you add a CS switch.
 * These pins also collide with GPS (below) — pick radio OR gps. */
// CC1101 (SubGHz)
#define CC1101_GDO0_PIN 5
#define CC1101_SS_PIN   4
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN  SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN
// NRF24 (2.4GHz)
#define NRF24_CE_PIN    5
#define NRF24_SS_PIN    4
#define NRF24_MOSI_PIN  SPI_MOSI_PIN
#define NRF24_SCK_PIN   SPI_SCK_PIN
#define NRF24_MISO_PIN  SPI_MISO_PIN
// W5500 (Ethernet)
#define W5500_INT_PIN   5
#define W5500_SS_PIN    4
#define W5500_MOSI_PIN  SPI_MOSI_PIN
#define W5500_SCK_PIN   SPI_SCK_PIN
#define W5500_MISO_PIN  SPI_MISO_PIN

/* =========================  Other peripherals  ===================== */
// GPS — shares GPIO 4/5 with the radios; use one or the other
#define GPS_SERIAL_TX 5
#define GPS_SERIAL_RX 4
// Bad-USB via CH9329 (ESP32-C5 has no USB-OTG) — on the I2C/serial pins
#define BAD_RX 2
#define BAD_TX 3

#endif /* Pins_Arduino_h */
