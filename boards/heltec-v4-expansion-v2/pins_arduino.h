#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#define DEVICE_NAME "Heltec V4 Expansion V2"
#define USB_as_HID 1

// The expansion kit occupies both SPI controllers. Display and SD share HSPI;
// the onboard SX1262 uses FSPI. Do not offer these pins to optional peripherals.
#define SPI_SCK_PIN 16
#define SPI_MISO_PIN 45
#define SPI_MOSI_PIN 15
#define SPI_SS_PIN 3
static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK = SPI_SCK_PIN;

#define SDCARD_CS 3
#define SDCARD_SCK SPI_SCK_PIN
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN

#define USER_SETUP_LOADED
#define USE_HSPI_PORT
#define SUPPORT_TRANSACTIONS
#define ST7789_DRIVER 1
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_ON
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MOSI SPI_MOSI_PIN
#define TFT_MISO SPI_MISO_PIN
#define TFT_SCLK SPI_SCK_PIN
#define TFT_CS 47
#define TFT_DC 48
#define TFT_RST 21 // Also resets the CHSC6xxx touchscreen.
#define TFT_BL 44
#define TFT_BACKLIGHT_ON HIGH
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 8000000
#define SMOOTH_FONT 1
#define HAS_SCREEN
#define HAS_TOUCH
#define HAS_CAPACITIVE_TOUCH
#define ROTATION 1
#define MINBRIGHT 5

#define SYS_I2C_SDA 17
#define SYS_I2C_SCL 18
#define HELTEC_TOUCH_ADDRESS 0x2E
// No external bus is free on this fixed kit; the system bus is touch-only.
#define GROVE_SDA -1
#define GROVE_SCL -1
// Wire.begin(-1, -1) falls back to these defaults. Keep Wire0 disconnected so
// an absent accessory cannot steal Wire1's fixed touchscreen pins.
static const uint8_t SDA = 255;
static const uint8_t SCL = 255;
#define WIRE1_PIN_DEFINED
static const uint8_t SDA1 = SYS_I2C_SDA;
static const uint8_t SCL1 = SYS_I2C_SCL;

#define LORA_SCK 9
#define LORA_MISO 11
#define LORA_MOSI 10
#define LORA_CS 8
#define LORA_RST 12
#define LORA_DIO0 14 // Existing Bruce configuration names SX1262 DIO1 as io2.
#define LORA_IRQ 14
#define LORA_BUSY 13
#define LORA_FEM_POWER 7
#define LORA_FEM_ENABLE 2
#define LORA_FEM_CTX 5
#define LORA_TCXO_VOLTAGE 1.8f
#define LORA_FIXED_SX1262
#define LORA_MIN_FREQ_MHZ 863.0f
#define LORA_MAX_FREQ_MHZ 928.0f
#define LORA_DEFAULT_FREQ_HZ "915000000.00"
// Conservative raw SX1262 drive until the external PA is measured/calibrated.
#define LORA_MAX_TX_POWER 0

#define GPS_SERIAL_RX 39
#define GPS_SERIAL_TX 38
#define HELTEC_GNSS_ENABLE 42 // Active low; L76K backup supply is separate.
#define HELTEC_GNSS_PPS 41
#define HELTEC_VEXT_ENABLE 40 // Active low: display, touch and SD supply.
#define HELTEC_BATTERY_ADC 1

// USB CDC is the console. UART0's usual RX44 conflicts with the backlight.
#define SERIAL_RX -1
#define SERIAL_TX -1
static const uint8_t TX = 255;
static const uint8_t RX = 255;

#define BUZZ_PIN 4
#define HELTEC_USER_BUTTON 0
#define HELTEC_NEXT_BUTTON 46 // Active high, externally biased; input-only pin.
#define BTN_ALIAS "USER"
#define DEEPSLEEP_WAKEUP_PIN HELTEC_USER_BUTTON
#define DEEPSLEEP_PIN_ACT LOW

#define TXLED -1
#define RXLED -1
#define IR_TX_PINS {{"Disabled", -1}, {"GPIO 6", 6}}
#define IR_RX_PINS {{"Disabled", -1}, {"GPIO 6", 6}}
#define RF_TX_PINS {{"Disabled", -1}, {"GPIO 6", 6}}
#define RF_RX_PINS {{"Disabled", -1}, {"GPIO 6", 6}}

#endif
