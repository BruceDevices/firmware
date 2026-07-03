#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// ลบ USB_VID และ USB_PID ออกเนื่องจาก ESP32 Classic ไม่มี Native USB OTG เหมือน S3

static const uint8_t TX = 1;
static const uint8_t RX = 3;

static const uint8_t SDA = 21;
static const uint8_t SCL = 22;

// กำหนดขาตำแหน่ง SPI มาตรฐาน (VSPI ของ ESP32)
static const uint8_t SS   = 5;
static const uint8_t MOSI = 23;
static const uint8_t MISO = 19;
static const uint8_t SCK  = 18;

// รายชื่อขา GPIO ทั้งหมดที่มีอยู่จริงบน ESP32 Classic (30-pin)
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

// การตั้งค่าระบบเสียง / Buzzer
#define BUZZ_PIN 25

// ลบการตั้งค่า RGB_LED และ I2S ชุดเดิมที่ใช้ขาของ S3 ออก เพื่อป้องกันการคอมไพล์ผิดพลาด
#define LED_ON HIGH
#define LED_OFF LOW

// --- การตั้งค่าปุ่มกดควบคุมระบบของ Bruce ---
#define HAS_BTN 1
#define BTN_UP 32
#define BTN_DOWN 33
#define BTN_SEL 26
#define BTN_BACK 22
#define BTN_ACT LOW

// --- การตั้งค่าโมดูลอินฟราเรด (IR) และวิทยุ (RF) ---
#define IR_TX_PINS '{{"IR_TX", 12}}'
#define IR_RX_PINS '{{"IR_RX", 13}}'
#define RF_TX_PINS '{{"RF_TX", 14}}'
#define RF_RX_PINS '{{"RF_RX", 27}}'

// --- การตั้งค่าสำหรับบอร์ดเสริม CC1101 / NRF24 ---
#define CC1101_GDO0_PIN 34  // ใช้ขา Input Only ที่ปลอดภัย
#define CC1101_SS_PIN 15
#define CC1101_MOSI_PIN SPI_MOSI_PIN
#define CC1101_SCK_PIN SPI_SCK_PIN
#define CC1101_MISO_PIN SPI_MISO_PIN

#define NRF24_CE_PIN 17
#define NRF24_SS_PIN 16
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// --- คอนฟิกหน้าจอ ST7789 ขนาด 1.47 นิ้ว ---
#define HAS_SCREEN 1
#define ROTATION 1
#define MINBRIGHT 20 // ปรับขั้นต่ำลงมาเพื่อให้หรี่ไฟได้จริง

#define USER_SETUP_LOADED 1
#define USE_HSPI_PORT 1
#define ST7789_DRIVER 1 // เปลี่ยนจาก ST7789_2_DRIVER เป็นไดรเวอร์มาตรฐาน
#define TFT_RGB_ORDER 1 // 1: RGB, 0: BGR (หากจอสีเพี้ยนให้ลองสลับตรงนี้)
#define TFT_WIDTH 172   // แก้ไขเป็นความละเอียดของจอ 1.47"
#define TFT_HEIGHT 320  // แก้ไขเป็นความละเอียดของจอ 1.47"
#define TFT_BACKLIGHT_ON 1

// จับคู่ขาหน้าจอให้ตรงกับที่เราเขียนไว้ในไฟล์อินเตอร์เฟซและแพลตฟอร์มไอโอ
#define TFT_BL 21
#define TFT_RST 4
#define TFT_DC 2
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS 5

#define TOUCH_CS -1
#define SMOOTH_FONT 1
#define SPI_FREQUENCY 40000000 // เพิ่มความเร็ว Bus เป็น 40MHz เพื่อความลื่นไหล

// ปิดการใช้งาน SD Card ชั่วคราวเนื่องจากขาบนบอร์ด 30-pin เต็ม (ใส่เป็น -1)
#define SDCARD_CS -1
#define SDCARD_SCK -1
#define SDCARD_MISO -1
#define SDCARD_MOSI -1

#define SPI_SCK_PIN 18
#define SPI_MOSI_PIN 23
#define SPI_MISO_PIN 19
#define SPI_SS_PIN 5

#endif /* Pins_Arduino_h */
