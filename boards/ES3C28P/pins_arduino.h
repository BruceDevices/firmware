#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME "ES3C28P"
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
// I2C Bus (shared between touch, audio codec, expansion)
// Touch FT6336G @ 0x38, Audio ES8311 @ 0x18
// =============================================
#define GROVE_SDA 16
#define GROVE_SCL 15
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// =============================================
// Main SPI Bus (exposed on expansion pins)
// Used for CC1101, NRF24, W5500 external modules
// =============================================
#define SPI_SCK_PIN 14
#define SPI_MOSI_PIN 3
#define SPI_MISO_PIN 2
#define SPI_SS_PIN 21

static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t SCK = SPI_SCK_PIN;
static const uint8_t MISO = SPI_MISO_PIN;

// =============================================
// SD Card - Uses SDIO mode (not SPI)
// SD_CLK=38, SD_CMD=40, SD_D0=39, SD_D1=41, SD_D2=48, SD_D3=47
// =============================================
#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

// =============================================
// CC1101 Sub-GHz Radio (external module via expansion pins)
// =============================================
#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 2
#define CC1101_SS_PIN 21
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

// =============================================
// NRF24L01 2.4GHz Radio (external module via expansion pins)
// =============================================
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 3
#define NRF24_SS_PIN 21
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// =============================================
// W5500 Ethernet (external module via expansion pins)
// =============================================
#define USE_W5500_VIA_SPI
#define W5500_SS_PIN -1
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_INT_PIN -1

// =============================================
// TFT Display (ILI9341V via SPI)
// =============================================
#define USER_SETUP_LOADED
#define ILI9341_2_DRIVER 1
#define TFT_INVERSION_ON 1 // Fix inverted colors
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO 13
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS 10
#define TFT_DC 46
#define TFT_RST -1 // Connected to CHIP_PU (shared with board reset)
#define TFT_BL 45  // Backlight control (HIGH = on)
#define TFT_BACKLIGHT_ON HIGH
#define SMOOTH_FONT 1
#define TOUCH_CS -1 // No SPI touch, using I2C capacitive touch
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000

// =============================================
// Display Setup
// =============================================
#define HAS_SCREEN 1
#define ROTATION 1 // Landscape mode (320x240)
#define MINBRIGHT 1
#define BACKLIGHT 45

// =============================================
// Touch Screen (FT6336G via I2C @ 0x38)
// =============================================
#define HAS_TOUCH 1
#define HAS_CAPACITIVE_TOUCH 1
#define TOUCH_FT6336_I2C 1
#define FT6336_I2C_ADDR 0x38
#define FT6336_I2C_CONFIG_SDA 16
#define FT6336_I2C_CONFIG_SCL 15
#define FT6336_TOUCH_CONFIG_RST 18
#define FT6336_TOUCH_CONFIG_INT 17

// =============================================
// Font Sizes
// =============================================
#define FP 1
#define FM 2
#define FG 3

// =============================================
// RGB LED (WS2812 NeoPixel)
// =============================================
#define HAS_RGB_LED 1
#define RGB_LED 42
#define LED_TYPE WS2812B
#define LED_ORDER GRB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 1
#define LED_COLOR_STEP 5

// =============================================
// Buttons
// =============================================
#define HAS_BTN 1
#define BTN_ALIAS "\"Boot\""
#define BTN_PIN 0 // BOOT button
#define BTN_ACT LOW
#define SEL_BTN 0 // BOOT button used as Select

// =============================================
// Audio System (ES8311 codec + I2S + FM8002E amplifier)
// ES8311 I2C: SDA=16, SCL=15 (shared with touch)
// I2S: MCLK=4, BCLK=5, LRC=7, DOUT=8, DIN=6
// Per LCD Wiki example: GPIO8=DOUT (ESP32→ES8311), GPIO6=DIN (ES8311→ESP32)
// =============================================
#define HAS_NS4168_SPKR 1 // Compatible I2S speaker interface
#define ES8311_CODEC 1
#define ES8311_ADDR 0x18
#define I2S_MCLK_PIN 4 // Master clock
#define BCLK 5         // Bit clock
#define WCLK 7         // Word clock (LRC)
#define DOUT 8         // Data out (ESP32 → ES8311 SDIN for playback)
#define MCLK 4         // Alias for I2S_MCLK_PIN
#define AMP_EN_PIN 1   // FM8002E amplifier enable (active LOW)

// Microphone (via ES8311 ADC)
#define MIC_SPM1423 1
#define PIN_CLK 7 // I2S WS/LRC for mic (used as ws pin in std I2S mode)
#define I2S_SCLK_PIN 5
#define I2S_DATA_PIN 6 // I2S DIN (ES8311 SDOUT → ESP32 for recording)
#define PIN_DATA 6

// =============================================
// Battery ADC
// =============================================
#define ANALOG_BAT_PIN 9
#define ANALOG_BAT_MULTIPLIER 2.0f // Voltage divider: multiply ADC by 2

// =============================================
// Infrared (external, via expansion pins)
// =============================================
#define TXLED 2 // IR TX default (expansion GPIO2)
#define RXLED 3 // IR RX default (expansion GPIO3)
#define LED_ON HIGH
#define LED_OFF LOW

#define IR_TX_PINS '{{"GPIO2", 2}, {"GPIO3", 3}, {"GPIO14", 14}, {"GPIO21", 21}}'
#define IR_RX_PINS '{{"GPIO2", 2}, {"GPIO3", 3}, {"GPIO14", 14}, {"GPIO21", 21}}'

// =============================================
// RF (external, via expansion pins)
// =============================================
#define RF_TX_PINS '{{"GPIO2", 2}, {"GPIO3", 3}, {"GPIO14", 14}, {"GPIO21", 21}}'
#define RF_RX_PINS '{{"GPIO2", 2}, {"GPIO3", 3}, {"GPIO14", 14}, {"GPIO21", 21}}'

// =============================================
// Serial (GPS) dedicated pins
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

// =============================================
// Deep Sleep
// =============================================
#define DEEPSLEEP_WAKEUP_PIN 0 // BOOT button
#define DEEPSLEEP_PIN_ACT LOW

#endif /* Pins_Arduino_h */
