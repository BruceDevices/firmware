#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// -------------------------------------------------------------------------------------------------
// ATS Mini (ESP32-S3-WROOM-1 + SI4732 pocket receiver) - Bruce board pinout
//
// Source of truth: https://github.com/esp32-si4732/ats-mini
//   ats-mini/Common.h        -> power / encoder / I2C / audio pins
//   ats-mini/Display.h       -> display bus and panel setup
//   docs/source/hardware.md  -> ESP32-S3-WROOM-1 pinout table
//
// Hardware summary:
//   MCU      ESP32-S3-WROOM-1, 8MB flash, 8MB OPI PSRAM (some units ship with Quad PSRAM, see ini)
//   Display  1.9" 170x320 ST7789, 8-bit parallel (same wiring as LilyGO T-Display-S3)
//   Input    rotary encoder (A/B + push) and an optional BOOT button (not soldered on every unit)
//   Battery  Li-ion, divider on GPIO4
//   Radio    SI4732 on its own I2C bus; Bruce does not drive it
// -------------------------------------------------------------------------------------------------

#ifndef DEVICE_NAME
#define DEVICE_NAME "ATS Mini"
#endif

#define USB_VID 0x303a
#define USB_PID 0x1001

// UART0 pins, not connected on the PCB
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// I2C: the only bus on the board is shared with the SI4732 (SDA=18, SCL=17).
// GROVE_SDA/GROVE_SCL/SYS_I2C_* are intentionally left unassigned so no Bruce module fights the receiver.
static const uint8_t SDA = 18;
static const uint8_t SCL = 17;

// Default SPI pins on the unused GPIOs (IO11-IO14 are "NC / spare" on the ESP32-S3-WROOM-1).
// IO10 is the audio amplifier enable, so it is NOT used as SS.
#define SPI_SS_PIN 14
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SCK_PIN 12

static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK = SPI_SCK_PIN;

// No SD card slot
#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

// TFT_eSPI display: ST7789 170x320, 8-bit parallel.
// Same settings the original ATS Mini firmware used before moving to LovyanGFX
// (INIT_SEQUENCE_3 / JLX240 table, inverted colors, 35px column offset).
#define USER_SETUP_LOADED
#define ST7789_DRIVER 1
#define TFT_BACKLIGHT_ON 1
#define INIT_SEQUENCE_3
#define CGRAM_OFFSET
#define TFT_RGB_ORDER TFT_RGB
#define TFT_INVERSION_ON
#define TFT_PARALLEL_8_BIT
#define SMOOTH_FONT 1
#define TFT_WIDTH 170
#define TFT_HEIGHT 320
#define TFT_BL 38
#define TFT_CS 6
#define TFT_DC 7 // LovyanGFX: pin_rs
#define TFT_RST 5
#define TFT_D0 39
#define TFT_D1 40
#define TFT_D2 41
#define TFT_D3 42
#define TFT_D4 45
#define TFT_D5 46
#define TFT_D6 47
#define TFT_D7 48
#define TFT_WR 8
#define TFT_RD 9

#define SPI_FREQUENCY 20000000
#define SPI_READ_FREQUENCY 20000000

// Display setup
#define HAS_SCREEN
#define ROTATION 3 // ATS Mini firmware uses setRotation(3) for its landscape layout
#define MINBRIGHT (uint8_t)1

// Font sizes
#define FP 1
#define FM 2
#define FG 3

// Power / audio control pins (SI4732 receiver section)
#define PIN_POWER_ON 15 // External LDO enable (1 = on). Stock firmware drives it HIGH at boot
#define PIN_AMP_EN 10   // Audio amplifier enable (1 = on). Never pulse it, a short pulse selects amp mode D
#define AUDIO_MUTE 3    // Hardware L/R mute (1 = muted)
#define SI4732_RESET 16 // SI4732 reset line (unused by Bruce)

// Battery monitor: GPIO4 behind a resistor divider.
// The ATS Mini power path has a load-sharing circuit, so with USB attached the pin reads about VBUS - 0.35V.
#define ANALOG_BAT_PIN 4
#define ANALOG_BAT_MULTIPLIER 2.0f // Divider ratio, verify against a multimeter on real hardware

// Rotary encoder + buttons
#define HAS_ENCODER
#define ENCODER_INA 2
#define ENCODER_INB 1
#define ENCODER_KEY 21
#define HAS_BTN 1
#define BTN_ALIAS "\"Push\""
#define SEL_BTN ENCODER_KEY
#define ESC_BTN 0 // BOOT button, optional (missing on some units). Long press on the encoder also acts as Esc
#define BTN_ACT LOW

// Deep sleep wake-up: encoder push button (RTC capable pad)
#define DEEPSLEEP_WAKEUP_PIN ENCODER_KEY
#define DEEPSLEEP_PIN_ACT LOW

// Infrared / single pin RF modules: the board has none. Defaults sit on the spare GPIOs
// instead of the I2C pins so nothing toggles the SI4732 bus by accident.
#define TXLED 11
#define RXLED 12
#define LED_ON HIGH
#define LED_OFF LOW

// BadUSB
#define USB_as_HID 1

#endif /* Pins_Arduino_h */
