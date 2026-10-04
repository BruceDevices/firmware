#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// SERIAL (UART1)
// GPIO43/44 are used by microSD CS and the RGB LED, so UART0 pins are not
// available: the console runs over USB CDC (ARDUINO_USB_CDC_ON_BOOT).
// These are the default pins for the serial/GPS bus and for PN532 in UART
// mode, and they stay user-configurable at runtime through /brucePins.conf.
#define SERIAL_RX 46
#define SERIAL_TX 45
static const uint8_t RX = SERIAL_RX;
static const uint8_t TX = SERIAL_TX;
#define RX1 RX
#define TX1 TX

// USB as HID (BadUSB)
#define USB_as_HID 1

// Main I2C Bus
#define GROVE_SDA 47
#define GROVE_SCL 48
#define SYS_I2C_SDA 47
#define SYS_I2C_SCL 48

static const uint8_t SDA = 47;
static const uint8_t SCL = 48;

// MAIN SPI BUS (shared by TFT, CC1101, nRF24, ST25R3916 and microSD)
#define SPI_SCK_PIN 17
#define SPI_MOSI_PIN 18
#define SPI_MISO_PIN 8
#define SPI_SS_PIN 11

static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK = SPI_SCK_PIN;

// BUTTONS
#define BTN_ALIAS "\"OK\""
#define HAS_5_BUTTONS
#define SEL_BTN 0
#define UP_BTN 41
#define DW_BTN 40
#define R_BTN 38
#define L_BTN 39
#define ESC_BTN 21
#define BTN_ACT LOW

// Deep sleep wakeup (Escape/Power)
#define DEEPSLEEP_WAKEUP_PIN 21
#define DEEPSLEEP_PIN_ACT LOW

// IR
#define RXLED 1
#define TXLED 2
#define LED 2
#define LED_ON HIGH
#define LED_OFF LOW

// CC1101
#define USE_CC1101_VIA_SPI
#define CC1101_SS_PIN 9
#define CC1101_GDO0_PIN 42
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

// NRF24
#define USE_NRF24_VIA_SPI
#define NRF24_SS_PIN 13
#define NRF24_CE_PIN 14
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// NFC ST25R3916
#define HAS_ST25R3916
#define ST25R_SCLK SPI_SCK_PIN
#define ST25R_MISO SPI_MISO_PIN
#define ST25R_MOSI SPI_MOSI_PIN
#define ST25R_CS 11
#define ST25R_IRQ 12

// iButton (1-wire)
#define IBUTTON_PIN 10

// FONT SIZE
#define FP 1
#define FM 2
#define FG 3

// TFT_eSPI display
#define HAS_SCREEN 1
#define ROTATION 1
#define MINBRIGHT (uint8_t)1

#define USER_SETUP_LOADED 1
#define ST7789_DRIVER 1
#define TFT_RGB_ORDER 0
#define TFT_WIDTH 170
#define TFT_HEIGHT 320
#define TFT_BACKLIGHT_ON 1
#define TFT_BL 6
#define TFT_RST 16
#define TFT_DC 15
#define TFT_MISO SPI_MISO_PIN
#define TFT_MOSI SPI_MOSI_PIN
#define TFT_SCLK SPI_SCK_PIN
#define TFT_CS 7
#define TOUCH_CS -1
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000

// SD CARD (microSD, shared SPI)
#define SDCARD_CS 43
#define SDCARD_SCK SPI_SCK_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN

// LoRa (external module, SX1261/SX1262)
// Shares the main SPI bus like every other radio on this board.
// GPIO4 doubles as IRQ: DIO1 for SX1262, DIO0 for SX1276/77/78.
// RST (NRST) is intentionally not wired: RadioLib skips the reset pulse when
// the pin is NC, so the radio must be powered up before the board boots, and a
// module stuck in power-down cannot be recovered in software.
#define LORA_SCK SPI_SCK_PIN
#define LORA_MISO SPI_MISO_PIN
#define LORA_MOSI SPI_MOSI_PIN
#define LORA_CS 5
#define LORA_IRQ 4
#define LORA_BUSY 3

// W5500 Ethernet (shares the module socket with the LoRa footprint above:
// same CS, same control lines. Only one of the two modules can be fitted and
// used at a time, they also contend for the single AUX SPI controller.)
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SS_PIN 5
#define W5500_INT_PIN 4
#define W5500_RST_PIN 3

// RGB LED (WS2812B)
#define HAS_RGB_LED 1
#define RGB_LED 44
#define LED_TYPE WS2812B
#define LED_ORDER RGB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT 1

#define LED_COLOR_STEP 5

// Battery / current sense: INA219 on the main I2C bus (0x40)
#define INA219_I2C_ADDRESS 0x40

#endif /* Pins_Arduino_h */
