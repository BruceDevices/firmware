// LilyGo T-LoRa-Pager Documentation:
// https://github.com/Xinyuan-LilyGO/LilyGoLib/blob/master/docs/hardware/lilygo-t-lora-pager.md

#ifdef T_LORA_PAGER
#ifndef __T_LORA_PAGER_H__
#define __T_LORA_PAGER_H__

#include "soc/soc_caps.h"
#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x82D4
#define USB_MANUFACTURER "LILYGO"
#define USB_PRODUCT "T-LoRa-Pager"
#define DEVICE_NAME "Lilygo T-Lora Pager"

#define HAS_KEYBOARD     // has keyboard to use
#define HAS_KEYBOARD_HID // has keyboard to use
#define KB_HID_EXIT_MSG "fn + Ok to exit"
#define KB_I2C_ADDRESS 0x34
#define KEYBOARD_BL 46

// Battery charger
#define BQ25896_I2C_ADDRESS 0x6B

// IO Expander
#define EXPANDS_DRV_EN 0
#define EXPANDS_AMP_EN 1
#define EXPANDS_KB_RST 2
#define EXPANDS_LORA_EN 3
#define EXPANDS_GPS_EN 4
#define EXPANDS_NFC_EN 5
#define EXPANDS_GPS_RST 7
#define EXPANDS_KB_EN 8
#define EXPANDS_GPIO_EN 9
#define EXPANDS_SD_DET 10
#define EXPANDS_SD_PULLEN 11
#define EXPANDS_SD_EN 12

#define IO_EXPANDER_PCA9555
#define IO_EXP_GPS EXPANDS_GPS_EN
// Main I2C Bus
#define SPI_SS_PIN 21
#define SPI_MOSI_PIN 34
#define SPI_MISO_PIN 33
#define SPI_SCK_PIN 35
static const uint8_t SS = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t SCK = SPI_MISO_PIN;
static const uint8_t MISO = SPI_SCK_PIN;

// Set Main I2C Bus
#define GROVE_SDA 3
#define GROVE_SCL 2
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// TFT_eSPI display
#define ST7796_DRIVER 1
#define USER_SETUP_LOADED
#define USE_HSPI_PORT
#define SMOOTH_FONT 1
#define TFT_INVERSION_ON
#define TFT_WIDTH 222
#define TFT_HEIGHT 480
#define TFT_BL 42
#define TFT_MISO SPI_MISO_PIN
#define TFT_MOSI SPI_MOSI_PIN
#define TFT_SCLK SPI_SCK_PIN
#define TFT_CS 38
#define TFT_DC 37
#define SPI_FREQUENCY 80000000
#define SPI_READ_FREQUENCY 20000000

// Display Setup
#define HAS_SCREEN
#define ROTATION 3
#define MINBRIGHT (uint8_t)1

// Font Sizes
#define FP 1
#define FM 2
#define FG 3

// SD Card
#define SDCARD_CS 21
#define SDCARD_MISO SPI_MISO_PIN
#define SDCARD_MOSI SPI_MOSI_PIN
#define SDCARD_SCK SPI_SCK_PIN

// NRF24 - Over GPIO expansion header
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN 43
#define NRF24_SS_PIN 44
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// CC1101 - Over GPIO expansion header
#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN 43
#define CC1101_GDO2_PIN 9
#define CC1101_SS_PIN 44
#define CC1101_MISO_PIN SPI_MISO_PIN
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN

// W5500 over QWIIC port
#define USE_W5500_VIA_SPI
#define W5500_SS_PIN 44
#define W5500_MOSI_PIN SPI_MOSI_PIN
#define W5500_SCK_PIN SPI_SCK_PIN
#define W5500_MISO_PIN SPI_MISO_PIN
#define W5500_INT_PIN 43

// Lora setup pins
#define LORA_SCK SPI_SCK_PIN
#define LORA_MISO SPI_MISO_PIN
#define LORA_MOSI SPI_MOSI_PIN
#define LORA_CS 36
#define LORA_RST 47
#define LORA_DIO0 14

// Serial
#define SERIAL_TX 43
#define SERIAL_RX 44
static const uint8_t TX = SERIAL_TX;
static const uint8_t RX = SERIAL_RX;
#define TX1 TX
#define RX1 RX

// Fuel Gauge
#define USE_BQ27220_VIA_I2C
#define BQ27220_I2C_ADDRESS 0x55
#define BQ27220_GROVE_SDA GROVE_SDA
#define BQ27220_GROVE_SCL GROVE_SCL

// Encoder
#define HAS_ENCODER
#define ENCODER_INA 40
#define ENCODER_INB 41
#define ENCODER_KEY 7
#define HAS_BTN 1
#define BTN_ALIAS "\"Mid\""
#define SEL_BTN ENCODER_KEY
#define UP_BTN -1
#define DW_BTN -1
#define BK_BTN 0
#define BTN_ACT LOW

// IR
#define TXLED -1
#define TXLED -1
#define RXLED -1
#define LED_ON HIGH
#define LED_OFF LOW

// FM Radio
#define FM_SI4713

// RFID
#define HAS_ST25R3916
#define RFID125_RX_PIN SERIAL_RX
#define RFID125_TX_PIN SERIAL_TX

// PN532
// TODO:
// #define PN532_RF_REST 45
// #define PN532_IRQ 17

// BadUSB
#define USB_as_HID 1

// Deepsleep
#define DEEPSLEEP_WAKEUP_PIN 0
#define DEEPSLEEP_PIN_ACT LOW

#define CAPS_LOCK 0x00
#define KEY_SHIFT 0x1c
#define KEY_FN 0x14
#define KEY_BACKSPACE 0x08
#define KEY_ENTER 0x0D

// Interrupt IO
#define RTC_INT 1
#define NFC_INT 5
#define SENSOR_INT 8
#define NFC_CS 39

// Audio ES8311
#define AUDIO_I2S_WS 18
#define AUDIO_I2S_SCK 11
#define AUDIO_I2S_MCLK 10
#define AUDIO_I2S_SDOUT 45
#define AUDIO_I2S_SDIN 17

// Speaker
// TODO: Need to implement ES8311
#define HAS_NS4168_SPKR
#define HAS_ES8311
#define ES8311_CODEC 1
#define ES8311_ADDR 0x18
#define BCLK 11
#define WCLK 18
#define DOUT 45
#define MCLK 10

// Mic
// TODO: Need to implement ES8311
#define PIN_BCLK 18
#define PIN_CLK 11
#define PIN_DATA 17

// GPS
#define GPS_SERIAL_TX 12
#define GPS_SERIAL_RX 4
#define GPS_SERIAL_PPS 13

// LoRa
#define LORA_SCK SPI_SCK_PIN
#define LORA_MISO SPI_MISO_PIN
#define LORA_MOSI SPI_MOSI_PIN
#define LORA_CS 36
#define LORA_RST 47
#define LORA_BUSY 48
#define LORA_IRQ 14

#endif /* Pins_Arduino_h */

#endif
