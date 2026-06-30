#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME "Waveshare 4.3B"
#endif

// =============================================================================
//  Waveshare ESP32-S3-Touch-LCD-4.3B
//  800x480 16-bit RGB parallel panel (ST7262-class) via ESP32-S3 LCD_CAM
//  Driven through Bruce's Arduino_GFX backend (USE_ARDUINO_GFX).
//  Backlight + touch reset are behind a CH422G I2C IO-expander (SDA8/SCL9).
// =============================================================================

// ---- Display: Arduino_GFX RGB panel selection -------------------------------
#define RGB_PANEL                 // -> TFT_DATABUS_TYPE = Arduino_ESP32RGBPanel
#define TFT_DATABUS_N        3    // Arduino_ESP32RGBPanel (16-bit parallel RGB)
#define TFT_DISPLAY_DRIVER_N 49   // Arduino_RGB_Display

#define TFT_WIDTH   800
#define TFT_HEIGHT  480

// Core sync pins
#define TFT_DE      5
#define TFT_VSYNC   3
#define TFT_HSYNC   46
#define TFT_PCLK    7

// 16-bit data pins. Mapped to match the verified ESP-IDF data_gpio_nums order:
//   data[0..4]  = Blue  -> Arduino_GFX b0..b4  (TFT_B*)
//   data[5..10] = Green -> Arduino_GFX g0..g5  (TFT_G*)
//   data[11..15]= Red   -> Arduino_GFX r0..r4  (TFT_R*)
// NOTE: if red/blue appear swapped on screen, swap the TFT_R* and TFT_B* blocks.
#define TFT_R0  1
#define TFT_R1  2
#define TFT_R2  42
#define TFT_R3  41
#define TFT_R4  40

#define TFT_G0  39
#define TFT_G1  0
#define TFT_G2  45
#define TFT_G3  48
#define TFT_G4  47
#define TFT_G5  21

#define TFT_B0  14
#define TFT_B1  38
#define TFT_B2  18
#define TFT_B3  17
#define TFT_B4  10

// Timings (verified for the 4.3B)
#define TFT_HSYNC_POL          0
#define TFT_HSYNC_FRONT_PORCH  8
#define TFT_HSYNC_PULSE_WIDTH  4
#define TFT_HSYNC_BACK_PORCH   8
#define TFT_VSYNC_POL          0
#define TFT_VSYNC_FRONT_PORCH  8
#define TFT_VSYNC_PULSE_WIDTH  4
#define TFT_VSYNC_BACK_PORCH   8
#define TFT_PCLK_ACTIVE_NEG    1
#define TFT_PREF_SPEED         (14 * 1000 * 1000)   // 14 MHz — margin below 16 MHz max to reduce glitching under bus load
// Bounce buffer: scan lines of SRAM cache between PSRAM framebuffer and LCD DMA.
// Prevents the double/tiled image glitch when PSRAM bus is stressed (WiFi+SD),
// and the transient vertical slip during heavy full-screen redraws (theme
// change / gradient settings screens). More lines = more slack to ride out a
// CPU/PSRAM burst before the panel underruns. Must stay an even divisor of 480.
#define TFT_BOUNCE_LINES       24

// ---- Bruce display behaviour ------------------------------------------------
#define HAS_SCREEN
#define ROTATION    0
#define MINBRIGHT   (uint8_t)1

// Font sizes — scaled up for 800×480 (CYD default is 1/2/3 for a 240px screen).
// At the Adafruit GFX base glyph (6×8 px): FP=2→12×16, FM=3→18×24, FG=5→30×40.
#define FP 2
#define FM 3
#define FG 5

// ---- I2C bus (CH422G expander + GT911 touch share this bus) -----------------
#define GROVE_SDA  8
#define GROVE_SCL  9
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// CH422G IO-expander output-bit assignments (set as outputs, push-pull)
#define CH422G_MODE_ADDR  0x24
#define CH422G_OUT_ADDR   0x38
#define CH422G_BIT_TP_RST  1   // EXIO1 - touch reset
#define CH422G_BIT_DISP    2   // EXIO2 - LCD backlight (LCD_BL)
#define CH422G_BIT_LCD_RST 3   // EXIO3 - LCD panel reset (MUST be high to display!)
#define CH422G_BIT_SD_CS   4   // EXIO4 - SD card chip select (active low)
#define CH422G_BIT_USB_SEL 5   // EXIO5 - USB select (leave low = native USB)

// ---- Touch (GT911 capacitive, on the I2C bus) -------------------------------
// HAS_TOUCH / HAS_CAPACITIVE_TOUCH / TOUCH_GT911_I2C are set in the board .ini
#define TOUCH_INT  4          // GT911 INT (direct GPIO)
// TP_RST is on the CH422G expander (CH422G_BIT_TP_RST), not a direct GPIO.

// ---- SD card ----------------------------------------------------------------
// SCK/MISO/MOSI are direct GPIOs.  CS is routed through the CH422G I2C
// expander (EXIO4, active low) — there is no direct GPIO CS line.
//
// Workaround: GPIO6 is an unused pin wired to nothing on the 4.3B; it is
// used here purely to satisfy the Arduino SPI/SD library, which requires a
// real GPIO number for CS bookkeeping.  The real hardware CS is held LOW
// permanently by the expander in _setup_gpio() (see interface.cpp).  Because
// the SD SPI bus is dedicated (no other devices share it), keeping CS
// asserted throughout is safe and the SD card responds normally.
#define SDCARD_CS    6    // dummy GPIO — not connected to SD card HW
#define SDCARD_SCK   12
#define SDCARD_MISO  13
#define SDCARD_MOSI  11

// ---- Misc / radios: not populated on a bare 4.3B; disable to avoid conflicts -
#define SPI_SS_PIN    -1
#define SPI_MOSI_PIN  -1
#define SPI_MISO_PIN  -1
#define SPI_SCK_PIN   -1

// Standard Arduino SPI pin aliases (framework SPI.cpp expects these to exist)
static const uint8_t SS   = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK  = SPI_SCK_PIN;

#define TXLED  -1
#define RXLED  -1

#define SERIAL_TX 43
#define SERIAL_RX 44

// ---- Buttons: only the BOOT button is available on a bare board -------------
#define BTN_ALIAS  "\"OK\""
#define SEL_BTN    0
#define UP_BTN     -1
#define DW_BTN     -1
#define BTN_ACT    LOW

#define LED_ON   HIGH
#define LED_OFF  LOW

// USB HID
#define USB_as_HID 1

#endif /* Pins_Arduino_h */
