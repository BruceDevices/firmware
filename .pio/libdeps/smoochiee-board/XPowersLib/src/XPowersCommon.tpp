/**
 *
 * @license MIT License
 *
 * Copyright (c) 2022 lewis he
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file      XPowersCommon.h
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2022-05-07
 *
 */


#pragma once

#include <stdint.h>

#if defined(ARDUINO)
#include <Wire.h>
#elif defined(ESP_PLATFORM)
#include "esp_log.h"
#include "esp_err.h"
#include <cstring>
#include "esp_idf_version.h"
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)
#include "driver/i2c_master.h"
#else
#include "driver/i2c.h"
#define XPOWERSLIB_I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define XPOWERSLIB_I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define XPOWERSLIB_I2C_MASTER_TIMEOUT_MS       1000
#endif //ESP_IDF_VERSION


#endif //ESP_PLATFORM

#define XPOWERSLIB_I2C_MASTER_SPEED            400000


#ifdef _BV
#undef _BV
#endif
#define _BV(b)                          (1ULL << (uint64_t)(b))


#ifndef constrain
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif



#define XPOWERS_ATTR_NOT_IMPLEMENTED    __attribute__((error("Not implemented")))
#define IS_BIT_SET(val,mask)            (((val)&(mask)) == (mask))

#if !defined(ARDUINO)
#ifdef linux
#include <stdio.h>
#include <string.h>
#include <errno.h>
#define log_e(__info,...)          printf("error :"  __info,##__VA_ARGS__)
#define log_i(__info,...)          printf("info  :"  __info,##__VA_ARGS__)
#define log_d(__info,...)          printf("debug :"  __info,##__VA_ARGS__)
#else
#define log_e(...)
#define log_i(...)
#define log_d(...)
#endif

#define LOW                 0x0
#define HIGH                0x1

//GPIO FUNCTIONS
#define INPUT               0x01
#define OUTPUT              0x03
#define PULLUP              0x04
#define INPUT_PULLUP        0x05
#define PULLDOWN            0x08
#define INPUT_PULLDOWN      0x09

#define RISING              0x01
#define FALLING             0x02
#endif

#if defined(ARDUINO_ARCH_MBED) || defined(ARDUINO_ARCH_ZEPHYR)
#define log_e(...)
#define log_i(...)
#define log_d(...)
#endif

#ifndef ESP32
#ifdef LOG_FILE_LINE_INFO
#undef LOG_FILE_LINE_INFO
#endif
#define LOG_FILE_LINE_INFO __FILE__, __LINE__
#ifndef log_e
#define log_e(fmt, ...)     Serial.printf("[E][%s:%d] " fmt "\n", LOG_FILE_LINE_INFO, ##__VA_ARGS__)
#endif
#ifndef log_i
#define log_i(fmt, ...)     Serial.printf("[I][%s:%d] " fmt "\n", LOG_FILE_LINE_INFO, ##__VA_ARGS__)
#endif
#ifndef log_d
#define log_d(fmt, ...)     Serial.printf("[D][%s:%d] " fmt "\n", LOG_FILE_LINE_INFO, ##__VA_ARGS__)
#endif
#endif


#if defined(NRF52840_XXAA) || defined(NRF52832_XXAA)
#ifndef SDA
#define SDA     (0xFF)
#endif

#ifndef SCL
#define SCL     (0xFF)
#endif
#endif


typedef int (*iic_fptr_t)(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);

template <class chipType>
class XPowersCommon
{

public:

#if defined(ARDUINO)
    bool begin(TwoWire &w, uint8_t addr, int sda, int scl)
    {
        if (__has_init)return thisChip().initImpl();
        __has_init = true;
        __sda = sda;
        __scl = scl;
        __wire = &w;

#if defined(NRF52840_XXAA) || defined(NRF52832_XXAA)
        if (__sda != 0xFF && __scl != 0xFF) {
  #if !defined(ARDUINO_ARCH_MBED) && !defined(ARDUINO_ARCH_ZEPHYR)
            __wire->setPins(__sda, __scl);
  #endif /* ARDUINO_ARCH_MBED || ZEPHYR */
        }
        __wire->begin();
#elif defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_STM32)
        if (__sda != 0xFF && __scl != 0xFF) {
            __wire->end();
            __wire->setSDA(__sda);
            __wire->setSCL(__scl);
        }
        __wire->begin();
#elif defined(ARDUINO_ARCH_ESP32)
        __wire->begin(__sda, __scl);
#else
        __wire->begin();
#endif
        __addr = addr;
        return thisChip().initImpl();
    }
#elif defined(ESP_PLATFORM)


#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)

    // * Using the new API of esp-idf 5.x, you need to pass the I2C BUS handle,
    // * which is useful when the bus shares multiple devices.
    bool begin(i2c_master_bus_handle_t i2c_dev_bus_handle, uint8_t addr)
    {
        log_i("Using ESP-IDF Driver interface.");
        if (i2c_dev_bus_handle == NULL) return false;
        if (__has_init)return thisChip().initImpl();

        thisReadRegCallback = NULL;
        thisWriteRegCallback = NULL;

        /*
            i2c_master_bus_config_t i2c_bus_config;
            memset(&i2c_bus_config, 0, sizeof(i2c_bus_config));
            i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
            i2c_bus_config.i2c_port = port_num;
            i2c_bus_config.scl_io_num = (gpio_num_t)__scl;
            i2c_bus_config.sda_io_num = (gpio_num_t)__sda;
            i2c_bus_config.glitch_ignore_cnt = 7;

            i2c_new_master_bus(&i2c_bus_config, &bus_handle);
        */

        bus_handle = i2c_dev_bus_handle;

        i2c_device_config_t i2c_dev_conf = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = XPOWERSLIB_I2C_MASTER_SPEED,
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,3,0))
            // New fields since esp-idf-v5.3-beta1
            .scl_wait_us = 0,
            .flags = {
                . disable_ack_check = 0
            }
#endif
        };

        if (ESP_OK != i2c_master_bus_add_device(bus_handle,
                                                &i2c_dev_conf,
                                                &__i2c_device)) {
            return false;
        }

        __has_init = thisChip().initImpl();

        if (!__has_init) {
            // Initialization failed, delete device
            i2c_master_bus_rm_device(__i2c_device);
        }
        return __has_init;
    }


#else //ESP 4.X


    bool begin(i2c_port_t port_num, uint8_t addr, int sda, int scl)
    {
        __i2c_num = port_num;
        log_i("Using ESP-IDF Driver interface.");
        if (__has_init)return thisChip().initImpl();
        __sda = sda;
        __scl = scl;
        __addr = addr;
        thisReadRegCallback = NULL;
        thisWriteRegCallback = NULL;

        i2c_config_t i2c_conf;
        memset(&i2c_conf, 0, sizeof(i2c_conf));
        i2c_conf.mode = I2C_MODE_MASTER;
        i2c_conf.sda_io_num = sda;
        i2c_conf.scl_io_num = scl;
        i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
        i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
        i2c_conf.master.clk_speed = XPOWERSLIB_I2C_MASTER_SPEED;

        /**
         * @brief Without checking whether the initialization is successful,
         * I2C may be initialized externally,
         * so just make sure there is an initialization here.
         */
        i2c_param_config(__i2c_num, &i2c_conf);
        i2c_driver_install(__i2c_num,
                           i2c_conf.mode,
                           XPOWERSLIB_I2C_MASTER_RX_BUF_DISABLE,
                           XPOWERSLIB_I2C_MASTER_TX_BUF_DISABLE, 0);
        __has_init = thisChip().initImpl();
        return __has_init;
    }
#endif //ESP 5.X
#endif //ESP_PLATFORM

    bool begin(uint8_t addr, iic_fptr_t readRegCallback, iic_fptr_t writeRegCallback)
    {
        if (__has_init)return thisChip().initImpl();
        __has_init = true;
        thisReadRegCallback = readRegCallback;
        thisWriteRegCallback = writeRegCallback;
        __addr = addr;
        return thisChip().initImpl();
    }

    int readRegister(uint8_t reg)
    {
        uint8_t val = 0;
        return readRegister(reg, &val, 1) == -1 ? -1 : val;
    }

    int writeRegister(uint8_t reg, uint8_t val)
    {
        return writeRegister(reg, &val, 1);
    }

    int readRegister(uint8_t reg, uint8_t *buf, uint8_t length)
    {
        if (thisReadRegCallback) {
            return thisReadRegCallback(__addr, reg, buf, length);
        }
#if defined(ARDUINO)
        if (__wire) {
            __wire->beginTransmission(__addr);
            __wire->write(reg);
            if (__wire->endTransmission() != 0) {
                return -1;
            }
            __wire->requestFrom(__addr, length);
            return __wire->readBytes(buf, length) == length ? 0 : -1;
        }
#elif defined(ESP_PLATFORM)

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)
        if (ESP_OK == i2c_master_transmit_receive(
                    __i2c_device,
                    (const uint8_t *)&reg,
                    1,
                    buf,
                    length,
                    -1)) {
            return 0;
        }
#else //ESP_IDF_VERSION
        if (ESP_OK == i2c_master_write_read_device(__i2c_num,
                __addr,
                (uint8_t *)&reg,
                1,
                buf,
                length,
                XPOWERSLIB_I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS)) {
            return 0;
        }
#endif //ESP_IDF_VERSION
#endif //ESP_PLATFORM
        return -1;
    }

    int writeRegister(uint8_t reg, uint8_t *buf, uint8_t length)
    {
        if (thisWriteRegCallback) {
            return thisWriteRegCallback(__addr, reg, buf, length);
        }
#if defined(ARDUINO)
        if (__wire) {
            __wire->beginTransmission(__addr);
            __wire->write(reg);
            __wire->write(buf, length);
            return (__wire->endTransmission() == 0) ? 0 : -1;
        }
        return -1;
#elif defined(ESP_PLATFORM)
        uint8_t *write_buffer = (uint8_t *)malloc(sizeof(uint8_t) * (length + 1));
        if (!write_buffer) {
            return -1;
        }
        write_buffer[0] = reg;
        memcpy(write_buffer + 1, buf, length);


#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)
        if (ESP_OK != i2c_master_transmit(
                    __i2c_device,
                    write_buffer,
                    length + 1,
                    -1)) {
            free(write_buffer);
            return -1;
        }
#else //ESP_IDF_VERSION
        if (ESP_OK != i2c_master_write_to_device(__i2c_num,
                __addr,
                write_buffer,
                length + 1,
                XPOWERSLIB_I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS)) {
            free(write_buffer);
            return -1;
        }
#endif //ESP_IDF_VERSION
        free(write_buffer);
        return 0;
#endif //ESP_PLATFORM
    }


    bool inline clrRegisterBit(uint8_t registers, uint8_t bit)
    {
        int val = readRegister(registers);
        if (val == -1) {
            return false;
        }
        return  writeRegister(registers, (val & (~_BV(bit)))) == 0;
    }

    bool inline setRegisterBit(uint8_t registers, uint8_t bit)
    {
        int val = readRegister(registers);
        if (val == -1) {
            return false;
        }
        return  writeRegister(registers, (val | (_BV(bit)))) == 0;
    }

    bool inline getRegisterBit(uint8_t registers, uint8_t bit)
    {
        int val = readRegister(registers);
        if (val == -1) {
            return false;
        }
        return val & _BV(bit);
    }

    uint16_t inline readRegisterH8L4(uint8_t highReg, uint8_t lowReg)
    {
        int h8 = readRegister(highReg);
        int l4 = readRegister(lowReg);
        if (h8 == -1 || l4 == -1)return 0;
        return (h8 << 4) | (l4 & 0x0F);
    }

    uint16_t inline readRegisterH8L5(uint8_t highReg, uint8_t lowReg)
    {
        int h8 = readRegister(highReg);
        int l5 = readRegister(lowReg);
        if (h8 == -1 || l5 == -1)return 0;
        return (h8 << 5) | (l5 & 0x1F);
    }

    uint16_t inline readRegisterH6L8(uint8_t highReg, uint8_t lowReg)
    {
        int h6 = readRegister(highReg);
        int l8 = readRegister(lowReg);
        if (h6 == -1 || l8 == -1)return 0;
        return ((h6 & 0x3F) << 8) | l8;
    }

    uint16_t inline readRegisterH5L8(uint8_t highReg, uint8_t lowReg)
    {
        int h5 = readRegister(highReg);
        int l8 = readRegister(lowReg);
        if (h5 == -1 || l8 == -1)return 0;
        return ((h5 & 0x1F) << 8) | l8;
    }

    /*
     * CRTP Helper
     */
protected:

    bool begin()
    {
#if defined(ARDUINO)
        if (__has_init) return thisChip().initImpl();
        __has_init = true;
        if (__wire) {
            log_i("SDA:%d SCL:%d", __sda, __scl);
#if defined(NRF52840_XXAA) || defined(NRF52832_XXAA)
            if (__sda != 0xFF && __scl != 0xFF) {
  #if !defined(ARDUINO_ARCH_MBED) && !defined(ARDUINO_ARCH_ZEPHYR)
                __wire->setPins(__sda, __scl);
  #endif /* ARDUINO_ARCH_MBED || ZEPHYR */
            }
            __wire->begin();
#elif defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_STM32)
            if (__sda != 0xFF && __scl != 0xFF) {
                __wire->end();
                __wire->setSDA(__sda);
                __wire->setSCL(__scl);
            }
            __wire->begin();
#elif defined(ARDUINO_ARCH_ESP32)
            __wire->begin(__sda, __scl);
#else
            __wire->begin();
#endif
        }
#endif  /*ARDUINO*/
        return thisChip().initImpl();
    }

    void end()
    {
#if defined(ARDUINO)
        if (__wire) {
#if defined(ESP_IDF_VERSION)
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4,4,0)
            __wire->end();
#endif  /*ESP_IDF_VERSION*/
#endif  /*ESP_IDF_VERSION*/
        }
#endif /*ARDUINO*/
    }


    inline const chipType &thisChip() const
    {
        return static_cast<const chipType &>(*this);
    }

    inline chipType &thisChip()
    {
        return static_cast<chipType &>(*this);
    }

protected:
    bool        __has_init              = false;
#if defined(ARDUINO)
    TwoWire     *__wire                 = NULL;
#elif defined(ESP_PLATFORM)
    i2c_port_t  __i2c_num;
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)
    i2c_master_bus_handle_t  bus_handle;
    i2c_master_dev_handle_t  __i2c_device;
#endif //ESP_IDF_VERSION

#endif //ESP_PLATFORM
    int         __sda                   = -1;
    int         __scl                   = -1;
    uint8_t     __addr                  = 0xFF;
    iic_fptr_t  thisReadRegCallback     = NULL;
    iic_fptr_t  thisWriteRegCallback    = NULL;
};
