// clang-format off
#pragma once
/*     This file has the purpose to verify the existence of some default flags      */
/*                Only PreCompiler code must be set in this file                    */

#include <pins_arduino.h>

#ifdef BOARD_HAS_PSRAM
#ifndef SERIAL_CMDS_TASK_STACK_SIZE
  #define SERIAL_CMDS_TASK_STACK_SIZE 8192
#endif
#ifndef INTERPRETER_TASK_STACK_SIZE
  #define INTERPRETER_TASK_STACK_SIZE 16384
#endif
#ifndef INPUT_HANDLER_TASK_STACK_SIZE
  #define INPUT_HANDLER_TASK_STACK_SIZE 4096
#endif
#ifndef SSH_TASK_STACK_SIZE
  #define SSH_TASK_STACK_SIZE 1024*10
#endif
#ifndef SAFE_STACK_BUFFER_SIZE
  #define SAFE_STACK_BUFFER_SIZE 4096
#endif
#else
// Boards without PSRAM need smaller stack sizes to be able to run properly
#ifndef SERIAL_CMDS_TASK_STACK_SIZE
  #define SERIAL_CMDS_TASK_STACK_SIZE 8192
#endif
#ifndef INTERPRETER_TASK_STACK_SIZE
  #define INTERPRETER_TASK_STACK_SIZE 16384
#endif
#ifndef INPUT_HANDLER_TASK_STACK_SIZE
  #define INPUT_HANDLER_TASK_STACK_SIZE 2048
#endif
#ifndef SSH_TASK_STACK_SIZE
  #define SSH_TASK_STACK_SIZE 1024*8
#endif
#ifndef SAFE_STACK_BUFFER_SIZE
  #define SAFE_STACK_BUFFER_SIZE 1024
#endif

#endif


#ifndef GROVE_SDA           // Pin to be used in I2C communications
  #define GROVE_SDA -1
#endif
#ifndef GROVE_SCL           // Pin to be used in I2C communications
  #define GROVE_SCL -1
#endif
#ifndef RXLED               // Default RX Infrared LED
  #define RXLED GROVE_SCL
#endif
#ifndef TXLED                 // Default TX Infrared Led
  #define TXLED GROVE_SDA
#endif
#ifndef LED_ON              // Infrared TXLED On state
  #define LED_ON 1
#endif
#ifndef LED_OFF             // Infrared LED Off state
  #define LED_OFF 0
#endif
#ifndef FP                  // Small Font -> Font Pequena
  #define FP 1
#endif
#ifndef FM                  // Medium Font -> Fonte Media
  #define FM 2
#endif
#ifndef FG                  // LArge Font -> Fonte Grande
  #define FG 3
#endif
#ifndef ROTATION
  #define ROTATION 1
#endif
#ifndef	TFT_WIDTH
  #define TFT_WIDTH 240
#endif
#ifndef TFT_HEIGHT
  #define TFT_HEIGHT 135
#endif

#ifndef LORA_SCK
  #define LORA_SCK -1
#endif
#ifndef LORA_MISO
  #define LORA_MISO -1
#endif
#ifndef LORA_MOSI
  #define LORA_MOSI -1
#endif
#ifndef LORA_CS
  #define LORA_CS -1
#endif
#ifndef LORA_RST
  #define LORA_RST -1
#endif
#ifndef LORA_DIO0
  #define LORA_DIO0 -1
#endif

// Default initializers
#ifndef TFT_CS
  #define TFT_CS -1
#endif
#ifndef TFT_RST
  #define TFT_RST -1
#endif
#ifndef TFT_SCLK
  #define TFT_SCLK -1
#endif
#ifndef TFT_MOSI
  #define TFT_MOSI -1
#endif
#ifndef TFT_BL
  #define TFT_BL -1
#endif
#ifndef TOUCH_CS
  #define TOUCH_CS -1
#endif
#ifndef SDCARD_MOSI
  #define SDCARD_MOSI -1
#endif
#ifndef SDCARD_MISO
  #define SDCARD_MISO  -1
#endif
#ifndef SDCARD_CS
  #define SDCARD_CS -1
#endif
#ifndef SDCARD_SCK
  #define SDCARD_SCK -1
#endif

#ifndef SERIAL_TX         // Serial TX used for GPS communications -> To use in other boards
  #define SERIAL_TX GROVE_SDA
#endif

#ifndef SERIAL_RX         // Serial RX used for GPS communications -> To use in other boards
  #define SERIAL_RX GROVE_SCL
#endif

#ifndef GPS_SERIAL_TX // Serial TX used just for GPS communications
  #define GPS_SERIAL_TX GROVE_SDA
#endif

#ifndef GPS_SERIAL_RX // Serial RX used just for GPS communications
  #define GPS_SERIAL_RX GROVE_SCL
#endif

//This one sets an array to set create the options to devices that have all GPIO available to use (except tft and SD Card)
#ifdef CONFIG_IDF_TARGET_ESP32S3
#define GPIO_PIN_LIST { \
    {"GPIO 1",   1}, {"GPIO 2",   2}, {"GPIO 3",   3}, {"GPIO 4",   4}, {"GPIO 5",   5}, {"GPIO 6",   6}, {"GPIO 7",   7}, {"GPIO 8",   8}, {"GPIO 9",   9}, {"GPIO 10", 10}, \
    {"GPIO 11", 11}, {"GPIO 12", 12}, {"GPIO 13", 13}, {"GPIO 14", 14}, {"GPIO 15", 15}, {"GPIO 16", 16}, {"GPIO 17", 17}, {"GPIO 18", 18}, {"GPIO 19", 19}, {"GPIO 20", 20}, \
    {"GPIO 21", 21}, {"GPIO 22", 22}, {"GPIO 23", 23}, {"GPIO 24", 24}, {"GPIO 25", 25}, {"GPIO 26", 26}, {"GPIO 27", 27}, {"GPIO 28", 28}, {"GPIO 29", 29}, {"GPIO 30", 30}, \
    {"GPIO 31", 31}, {"GPIO 32", 32}, {"GPIO 33", 33}, {"GPIO 34", 34}, {"GPIO 35", 35}, {"GPIO 36", 36}, {"GPIO 37", 37}, {"GPIO 38", 38}, {"GPIO 39", 39}, {"GPIO 40", 40}, \
    {"GPIO 41", 41}, {"GPIO 42", 42}, {"GPIO 43", 43}, {"GPIO 44", 44}, {"GPIO 0",   0}, \
}
#elif CONFIG_IDF_TARGET_ESP32C5
#define GPIO_PIN_LIST { \
  {"GPIO 0",   0}, {"GPIO 1",   1}, {"GPIO 2",   2}, {"GPIO 3",   3}, {"GPIO 4",   4}, {"GPIO 5",   5}, {"GPIO 6",   6}, {"GPIO 7",   7}, {"GPIO 8",   8}, {"GPIO 9",   9}, \
  {"GPIO 10", 10}, {"GPIO 11", 11}, {"GPIO 12", 12}, {"GPIO 13", 13}, {"GPIO 14", 14}, {"GPIO 23", 23}, {"GPIO 24", 24}, {"GPIO 25", 25}, {"GPIO 26", 26}, {"GPIO 27", 27}, {"GPIO 28", 28} \
}
#else
#define GPIO_PIN_LIST { \
  {"GPIO 1",   1}, {"GPIO 2",   2}, {"GPIO 3",   3}, {"GPIO 4",   4}, {"GPIO 5",   5}, {"GPIO 6",   6}, {"GPIO 7",   7}, {"GPIO 8",   8}, {"GPIO 9",   9}, {"GPIO 10", 10}, \
  {"GPIO 11", 11}, {"GPIO 12", 12}, {"GPIO 13", 13}, {"GPIO 14", 14}, {"GPIO 15", 15}, {"GPIO 16", 16}, {"GPIO 17", 17}, {"GPIO 18", 18}, {"GPIO 19", 19}, {"GPIO 20", 20}, \
  {"GPIO 21", 21}, {"GPIO 22", 22}, {"GPIO 23", 23}, {"GPIO 24", 24}, {"GPIO 25", 25}, {"GPIO 26", 26}, {"GPIO 27", 27}, {"GPIO 28", 28}, {"GPIO 29", 29}, {"GPIO 30", 30}, \
  {"GPIO 31", 31}, {"GPIO 32", 32}, {"GPIO 33", 33}, {"GPIO 34", 34}, {"GPIO 35", 35}, {"GPIO 36", 36}, {"GPIO 37", 37}, {"GPIO 38", 38}, {"GPIO 39", 39}, {"GPIO 0",   0} \
}
#endif
#ifdef ALLOW_ALL_GPIO_FOR_IR_RF
    #undef IR_TX_PINS
    #undef IR_RX_PINS
    #undef RF_TX_PINS
    #undef RF_RX_PINS

    #define IR_TX_PINS GPIO_PIN_LIST
    #define IR_RX_PINS GPIO_PIN_LIST
    #define RF_TX_PINS GPIO_PIN_LIST
    #define RF_RX_PINS GPIO_PIN_LIST
#endif
#if !defined(IR_TX_PINS) || !defined(IR_RX_PINS) || !defined(RF_TX_PINS) || !defined(RF_RX_PINS)
    #define IR_TX_PINS GPIO_PIN_LIST
    #define IR_RX_PINS GPIO_PIN_LIST
    #define RF_TX_PINS GPIO_PIN_LIST
    #define RF_RX_PINS GPIO_PIN_LIST
#endif

#ifndef NRF24_SCK_PIN
#define NRF24_SCK_PIN -1
#endif

#ifndef NRF24_MISO_PIN
#define NRF24_MISO_PIN -1
#endif

#ifndef NRF24_MOSI_PIN
#define NRF24_MOSI_PIN -1
#endif

#ifndef NRF24_SS_PIN
#define NRF24_SS_PIN -1
#endif

#ifndef NRF24_CE_PIN
#define NRF24_CE_PIN -1
#endif

#ifndef CC1101_SCK_PIN
#define CC1101_SCK_PIN -1
#endif

#ifndef CC1101_MISO_PIN
#define CC1101_MISO_PIN -1
#endif

#ifndef CC1101_MOSI_PIN
#define CC1101_MOSI_PIN -1
#endif

#ifndef CC1101_SS_PIN
#define CC1101_SS_PIN -1
#endif

#ifndef CC1101_GDO0_PIN
#define CC1101_GDO0_PIN -1
#endif

#ifndef W5500_SCK_PIN
#define W5500_SCK_PIN -1
#endif

#ifndef W5500_MISO_PIN
#define W5500_MISO_PIN -1
#endif

#ifndef W5500_MOSI_PIN
#define W5500_MOSI_PIN -1
#endif

#ifndef W5500_SS_PIN
#define W5500_SS_PIN -1
#endif

#ifndef W5500_INT_PIN
#define W5500_INT_PIN -1
#endif

#ifndef W5500_RST_PIN
#define W5500_RST_PIN -1
#endif

// Temporary, delete after finish Interfaces

#ifndef SMOOTH_FONT
#define SMOOTH_FONT
#endif

#ifndef DEEPSLEEP_WAKEUP_PIN
#define DEEPSLEEP_WAKEUP_PIN -1
#endif

#ifndef DEEPSLEEP_PIN_ACT
#define DEEPSLEEP_PIN_ACT LOW
#endif
