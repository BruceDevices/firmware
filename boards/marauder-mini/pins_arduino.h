#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// เปลี่ยนกลับเป็นขา Serial มาตรฐานของ ESP32 Classic
static const uint8_t TX = 1;
static const uint8_t RX = 3;

static const uint8_t TXD2 = 17;
static const uint8_t RXD2 = 16;

static const uint8_t SDA = 33;
static const uint8_t SCL = 26;

// กำหนดขาตำแหน่ง SPI มาตรฐาน (VSPI Bus)
static const uint8_t SS   = 5;
static const uint8_t MOSI = 23;
static const uint8_t MISO = 19;
static const uint8_t SCK  = 18;

// รายชื่อขา GPIO ที่มีอยู่จริงบน ESP32 Classic 30-pin เท่านั้น
static const uint8_t G0 = 0;
static const uint8_t G2 = 2;
static const uint8_t G4 = 4;
static const uint8_t G5 = 5;
static const uint8_t G12 = 12;
static const uint8_t G13 = 13;
static const uint8_t G14 = 14;
static const uint8_t G15 = 15;
static const uint8_t G16 = 16;
static const uint8_t G17 = 17;
static const uint8_t G18 = 18;
static const uint8_t G19 = 19;
static const uint8_t G21 = 21;
static const uint8_t G22 = 22;
static const uint8_t G23 = 23;
static const uint8_t G25 = 25;
static const uint8_t G26 = 26;
static const uint8_t G27 = 27;
static const uint8_t G32 = 32;
static const uint8_t G33 = 33;
static const uint8_t G34 = 34; // Input Only
static const uint8_t G35 = 35; // Input Only
static const uint8_t G36 = 36; // Input Only
static const uint8_t G39 = 39; // Input Only

// SERIAL (GPS) ย้ายมาใช้คู่ขาพอร์ต 2 (Serial2) เพื่อไม่ให้ชนกับระบบอื่น
#define SERIAL_TX 17
#define SERIAL_RX 16
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX

// --- ระบบควบคุม 5 ปุ่ม ---
// (โน้ต: ขา 34, 35, 36, 39 จำเป็นต้องต่อตัวต้านทาน Pull-up 10k ภายนอกลงไฟ 3.3V)
#define HAS_BTN 1
#define HAS_5_BUTTONS
#define SEL_BTN 34
#define UP_BTN 36
#define DW_BTN 35
#define R_BTN 39
#define L_BTN 13
#define BTN_ALIAS "\"Ok\""
#define BTN_ACT LOW

#define TXLED -1
#define LED_ON HIGH
#define LED_OFF LOW

// --- บอร์ดเสริม CC1101 & NRF24 ---
#define CC1101_GDO0_PIN 27
#define CC1101_SS_PIN 15
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define NRF24_CE_PIN 12
#define NRF24_SS_PIN 14
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

#define FP 1
#define FM 1
#define FG 2

#define HAS_SCREEN 1
#define ROTATION 0
#define MINBRIGHT 20

// --- ระบบ SD Card (ย้ายขา CS หนีจอแสดงผล) ---
#define SDCARD_CS 15
#define SDCARD_SCK 18
#define SDCARD_MISO 19
#define SDCARD_MOSI 23

#define GROVE_SDA 33
#define GROVE_SCL 26

// สรุปขาเชื่อมต่อ SPI หลักให้ถูกต้องตามสถาปัตยกรรมชิป
#define SPI_SCK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23
#define SPI_SS_PIN 5  // เปลี่ยนจากขา 1 มาเป็นขา 5 ป้องกัน Serial พัง

#endif /* Pins_Arduino_h */
