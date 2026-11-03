#ifndef Pins_Arduino_h
#define Pins_Arduino_h

// -------------------------------------------------------------------------------------------------
// QRP Labs LightTracker Plus v1.0 — Bruce board pinout
// ESP32-S3 (8MB flash) + SX1268 (EBYTE E22-400M30S, 433 MHz) + Quectel L80 GPS
// Pin map: lightaprs/LightTracker-Plus-1.0 + richonguzman/LoRa_APRS_Tracker variant
// -------------------------------------------------------------------------------------------------

#include "soc/soc_caps.h"
#include <stdint.h>

#define USB_VID 0x303A
#define USB_PID 0x81E0

// UART0 (USB-UART header)
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// I2C (Wire)
static const uint8_t SDA = 3;
static const uint8_t SCL = 4;

// SPI (LoRa)
static const uint8_t SS = 10;
static const uint8_t MOSI = 11;
static const uint8_t MISO = 13;
static const uint8_t SCK = 12;

static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;
static const uint8_t A6 = 7;
static const uint8_t A7 = 8;
static const uint8_t A8 = 9;
static const uint8_t A9 = 10;
static const uint8_t A10 = 11;
static const uint8_t A11 = 12;
static const uint8_t A12 = 13;
static const uint8_t A13 = 14;
static const uint8_t A14 = 15;
static const uint8_t A15 = 16;
static const uint8_t A16 = 17;
static const uint8_t A17 = 18;
static const uint8_t A18 = 19;
static const uint8_t A19 = 20;

static const uint8_t T1 = 1;
static const uint8_t T2 = 2;
static const uint8_t T3 = 3;
static const uint8_t T4 = 4;
static const uint8_t T5 = 5;
static const uint8_t T6 = 6;
static const uint8_t T7 = 7;
static const uint8_t T8 = 8;
static const uint8_t T9 = 9;
static const uint8_t T10 = 10;
static const uint8_t T11 = 11;
static const uint8_t T12 = 12;
static const uint8_t T13 = 13;
static const uint8_t T14 = 14;

// ---------------- Board specific -----------------
// LoRa SX1268 (E22-400M30S) — RadioLib SX126x driver
#define LT_LORA_SCK 12
#define LT_LORA_MISO 13
#define LT_LORA_MOSI 11
#define LT_LORA_CS 10
#define LT_LORA_RST 9
#define LT_LORA_DIO1 5
#define LT_LORA_BUSY 6
#define LT_LORA_RXEN 42       // E22 RF switch, RX enable
#define LT_LORA_TXEN 14       // E22 RF switch, TX enable
#define LT_LORA_POWER_CTRL 21 // E22 module power

// GPS Quectel L80RE-M37
#define LT_GPS_RX 18 // ESP32 RX  <- GPS TX
#define LT_GPS_TX 17 // ESP32 TX  -> GPS RX
#define LT_GPS_VCC 33 // PNP high-side switch: LOW = on
#define LT_GPS_BAUD 9600

// OLED SSD1306 128x64 (I2C, addr 0x3C)
#define LT_OLED_SDA 3
#define LT_OLED_SCL 4

// Misc
#define LT_BUTTON 0  // BOOT button
#define LT_LED 16    // user LED
#define LT_VBAT_ADC 1 // battery sense, divider 1:2

#endif /* Pins_Arduino_h */
