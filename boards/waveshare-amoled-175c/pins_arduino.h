#ifndef Pins_Arduino_h
#define Pins_Arduino_h

/*
 * Waveshare ESP32-S3-Touch-AMOLED-1.75C
 *   - ESP32-S3R8 (8MB OPI PSRAM), 16/32MB QSPI flash
 *   - 1.75" round AMOLED 466x466, CO5300 driver over QSPI
 *   - CST9217 capacitive touch (I2C 0x5A)
 *   - AXP2101 PMU (I2C 0x34) - PWR key read through PMU IRQ
 *   - ES8311 codec (I2C 0x18) + NS4150B amp (PA = GPIO46, active HIGH)
 *   - ES7210 mic ADC (0x40), QMI8658 IMU (0x6B), no RTC
 *   - BOOT button on GPIO0
 *
 * UI note: the panel is round. Bruce's UI is rectangular, so we expose a
 * centred 340x340 window (fits inside the circle) as the logical screen.
 * The window is created with column/row offsets in the CO5300 driver,
 * so nothing gets drawn in the invisible corners.
 */

#include "soc/soc_caps.h"
#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

static const uint8_t TX = 43;
static const uint8_t RX = 44;

// =============================================
// System I2C bus (touch, PMU, codec, RTC, IMU)
// =============================================
#define SYS_I2C_SDA 15
#define SYS_I2C_SCL 14
#define GROVE_SDA 15
#define GROVE_SCL 14
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// =============================================
// SPI for external modules (no SPI bus broken out on this board)
// GPIO17/18 + 43/44 are on the expansion header
// =============================================
#define SPI_SCK_PIN -1
#define SPI_MOSI_PIN -1
#define SPI_MISO_PIN -1
#define SPI_SS_PIN -1
static const uint8_t SS = -1;
static const uint8_t MOSI = -1;
static const uint8_t MISO = -1;
static const uint8_t SCK = -1;

#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

#define CC1101_GDO0_PIN -1
#define CC1101_SS_PIN -1
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define NRF24_CE_PIN -1
#define NRF24_SS_PIN -1
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

#define W5500_SS_PIN -1
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_INT_PIN -1

// =============================================
// Display: CO5300 AMOLED via Arduino_GFX (QSPI)
// =============================================
#define USE_ARDUINO_GFX 1
#define TFT_DATABUS_N 1 // Arduino_ESP32QSPI
#define TFT_DISPLAY_DRIVER_N 41 // Arduino_CO5300
// This panel drops 1px-wide/tall RAM writes (lines, outlines, icons vanish):
// widen them to 2px in the driver (see ardgfx.h)
#define PANEL_MIN_2PX 1
#define TFT_CS 12
#define TFT_SCLK 38
#define TFT_D0 4
#define TFT_D1 5
#define TFT_D2 6
#define TFT_D3 7
#define TFT_RST 1
#define TFT_IPS 0
// Bruce defaults colorInverted=1 (meant for IPS LCDs). The CO5300 driver ignores
// TFT_IPS, so flip the invert logic in the HAL instead (black background).
#define TFT_INVERT_FLIP 1
#define TFT_ROTATION 0

// Physical panel is 466x466 with a 6px column offset in controller RAM.
// We use the centred square that fits in the circle.
#define AMOLED_PHYS_SIZE 466
#define AMOLED_COL_BASE 6
#define AMOLED_WINDOW 340
#define AMOLED_WIN_OFS ((AMOLED_PHYS_SIZE - AMOLED_WINDOW) / 2) // 63
#define TFT_WIDTH AMOLED_WINDOW
#define TFT_HEIGHT AMOLED_WINDOW
#define TFT_COL_OFS1 (AMOLED_COL_BASE + AMOLED_WIN_OFS)
#define TFT_ROW_OFS1 (AMOLED_WIN_OFS)
// Offsets used when the image is flipped 180 deg (controller RAM is 480x480)
#define TFT_COL_OFS2 (480 - AMOLED_PHYS_SIZE - AMOLED_COL_BASE + AMOLED_WIN_OFS)
#define TFT_ROW_OFS2 (480 - AMOLED_PHYS_SIZE + AMOLED_WIN_OFS)
// NOTE: CO5300 cannot swap X/Y, so only rotation 0 and 2 are meaningful.

#define TFT_BL -1
#define BACKLIGHT -1
#define TFT_BACKLIGHT_ON 1

#define HAS_SCREEN 1
// Round panel: hide Bruce's rectangular screen frame / footer box / divider
#define ROUND_DISPLAY_NO_FRAME 1
// Status bar: bigger clock, refreshed every second on the main menu
#define STATUS_CLOCK_SIZE 2
#define STATUSBAR_REFRESH_MS 1000
// Verified on hardware: rotation 0 (CO5300 default scan) is upright.
// 2 = 180 deg, 3 = vertical flip, 1 = horizontal mirror.
#define ROTATION 0
// CO5300 can't swap X/Y and Bruce defaults to landscape (1/3) - pin it to 0
#define FIXED_ROTATION 0
#define MINBRIGHT 1

// Font sizes
// Bigger than other boards: the AMOLED is ~266 ppi, size 1 text is unreadable
#define FP 2
#define FM 3
#define FG 4

// =============================================
// Touch: CST9217 (I2C 0x5A)
// =============================================
#define HAS_TOUCH 1
#define TOUCH_FOOTER_BACK 1 // footer shows BACK | PREV | SEL | NEXT
#define TOUCH_FOOTER_H 44   // big footer buttons
#define TOUCH_I2C_ADDR 0x5A
#define TOUCH_INT_PIN 11
#define TOUCH_RST_PIN 2

// =============================================
// PMU AXP2101
// =============================================
#define HAS_AXP2101 1

// RTC: this 1.75C board has NO PCF85063 (I2C scan finds only 0x18 0x34 0x40
// 0x5A 0x6B). Enabling HAS_RTC makes Bruce read 0x51, the NACK leaves the
// shared I2C bus stuck and breaks touch/PMU reads. Keep it disabled.

// BadUSB / USB keyboard over native USB
#define USB_as_HID 1

// Deep sleep wake on BOOT button
#define DEEPSLEEP_WAKEUP_PIN 0
#define DEEPSLEEP_PIN_ACT LOW

// =============================================
// Buttons
// =============================================
#define HAS_BTN 1
#define BTN_ALIAS "\"Boot\""
#define BTN_PIN 0
#define BTN_ACT LOW

// =============================================
// Audio: ES8311 codec + NS4150B amplifier
// =============================================
#define HAS_NS4168_SPKR 1
#define ES8311_CODEC 1
#define ES8311_ADDR 0x18
#define AMP_EN_PIN 46 // NS4150B enable, active HIGH
#define I2S_MCLK_PIN 16
#define MCLK 16
#define BCLK 9
#define WCLK 45
#define DOUT 8

// =============================================
// Infrared / RF (external, on expansion header)
// =============================================
#define TXLED 17
#define RXLED 18
#define LED_ON HIGH
#define LED_OFF LOW
#define IR_TX_PINS {{"GPIO17", 17}, {"GPIO18", 18}, {"GPIO43", 43}, {"GPIO44", 44}}
#define IR_RX_PINS {{"GPIO17", 17}, {"GPIO18", 18}, {"GPIO43", 43}, {"GPIO44", 44}}
#define RF_TX_PINS {{"GPIO17", 17}, {"GPIO18", 18}, {"GPIO43", 43}, {"GPIO44", 44}}
#define RF_RX_PINS {{"GPIO17", 17}, {"GPIO18", 18}, {"GPIO43", 43}, {"GPIO44", 44}}

// Serial (GPS) on expansion header UART pins
#define SERIAL_TX 43
#define SERIAL_RX 44
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX

#define BAD_TX SERIAL_TX
#define BAD_RX SERIAL_RX

#define FM_RSTPIN -1
#define RGB_LED -1

#endif /* Pins_Arduino_h */
