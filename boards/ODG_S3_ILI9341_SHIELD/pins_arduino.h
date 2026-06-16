#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME "ODG S3 ILI9341 Shield"
#endif

#define USB_VID 0x303a
#define USB_PID 0x1001

// UART0 console follows the existing ESP32-S3 board pattern.
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// GPIO3/GPIO45/GPIO46 are strapping pins used exclusively by TFT.
#define TFT_MOSI 45
#define TFT_MISO 46
#define TFT_SCLK 3
#define TFT_CS 14
#define TFT_DC 47
#define TFT_RST 21
#define TFT_BL -1
#define BACKLIGHT -1

static const uint8_t MOSI = TFT_MOSI;
static const uint8_t MISO = TFT_MISO;
static const uint8_t SCK = TFT_SCLK;
static const uint8_t SS = TFT_CS;

// GPIO1/GPIO2/GPIO41/GPIO42 are touch XPT2046.
#define TOUCH_CS 1
#define TOUCH_CLK 42
#define TOUCH_MOSI 2
#define TOUCH_MISO 41
#define TOUCH_IRQ -1

// GPIO17/GPIO18 reserved for ESP32 Classic UART.
#define ODG_CLASSIC_UART_TX_RESERVED 17
#define ODG_CLASSIC_UART_RX_RESERVED 18

// microSD desactivada hasta confirmar SD_CS por continuidad.
#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

#define USER_SETUP_LOADED 1
#define ILI9341_2_DRIVER 1
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define ROTATION 1
#define HAS_SCREEN 1
#define HAS_TOUCH 1
#define ODG_TOUCH_XPT2046_SEPARATE_SPI 1
#define SMOOTH_FONT 1
#define FP 1
#define FM 2
#define FG 3

#define HAS_BTN 0
#define BTN_ALIAS "\"Touch\""
#define BTN_PIN -1

#define DEEPSLEEP_WAKEUP_PIN -1
#define DEEPSLEEP_PIN_ACT LOW

#endif /* Pins_Arduino_h */
