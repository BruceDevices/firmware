/**
 *
 * @license MIT License
 *
 * Copyright (c) 2024 lewis he
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
 * @file      PowerDeliveryHUSB238.tpp
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2024-07-24
 *
 */
#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <math.h>
#endif /*ARDUINO*/
#include "XPowersCommon.tpp"
#include "REG/HUSB238Constants.h"

class PowerDeliveryHUSB238 :
    public XPowersCommon<PowerDeliveryHUSB238>
{
    friend class XPowersCommon<PowerDeliveryHUSB238>;

public:

    enum PD_Status {
        NO_RESPONSE,
        SUCCESS,
        INVALID_CMD,
        NOT_SUPPORT,
        TRANSACTION_FAIL
    };

    enum PD_Voltage {
        PD_5V = 1,
        PD_9V,
        PD_12V,
        PD_15V,
        PD_18V,
        PD_20V
    };


#if defined(ARDUINO)
    PowerDeliveryHUSB238(TwoWire &w, int sda = SDA, int scl = SCL, uint8_t addr = HUSB238_SLAVE_ADDRESS)
    {
        __wire = &w;
        __sda = sda;
        __scl = scl;
        __addr = addr;
    }
#endif

    PowerDeliveryHUSB238(uint8_t addr, iic_fptr_t readRegCallback, iic_fptr_t writeRegCallback)
    {
        thisReadRegCallback = readRegCallback;
        thisWriteRegCallback = writeRegCallback;
        __addr = addr;
    }

    PowerDeliveryHUSB238()
    {
#if defined(ARDUINO)
        __wire = &Wire;
        __sda = SDA;
        __scl = SCL;
#endif
        __addr = HUSB238_SLAVE_ADDRESS;
    }


    ~PowerDeliveryHUSB238()
    {
        log_i("~PowerDeliveryHUSB238");
        deinit();
    }

#if defined(ARDUINO)
    bool init(TwoWire &w, int sda = SDA, int scl = SCL, uint8_t addr = HUSB238_SLAVE_ADDRESS)
    {
        __wire = &w;
        __sda = sda;
        __scl = scl;
        __addr = addr;
        return begin();
    }
#endif

    void deinit()
    {
        end();
    }

    uint8_t getPdVoltage()
    {
        int val =  readRegister(HUSB238_PD_STATUS0);
        if (val == -1)return 0;
        val &= 0xF0;
        val >>= 4;
        return pd_voltage_list[val];
    }

    float getPdCurrent()
    {
        int val =  readRegister(HUSB238_PD_STATUS0);
        if (val == -1)return 0;
        val &= 0x0F;
        return pd_current_list[val];
    }

    void setPdVoltage(PD_Voltage vol)
    {
        writeRegister(HUSB238_SRC_PDO, vol);
        writeRegister(HUSB238_GO_COMMAND, 0x01);
    }

    void resetPdVoltage()
    {
        writeRegister(HUSB238_GO_COMMAND, 0x10);
    }

    bool attach()
    {
        return getRegisterBit(HUSB238_PD_STATUS1, 6);
    }

    PD_Status status()
    {
        int val = readRegister(HUSB238_PD_STATUS1);
        if (val == -1)return NO_RESPONSE;
        val >>= 3;
        val &= 0x03;
        return static_cast<PD_Status>(val);
    }


private:
    const uint8_t pd_voltage_list[7] = {0, 5, 9, 12, 15, 18, 20};
    const float pd_current_list[16] = {
        0.5, 0.7, 1, 1.25, 1.5, 1.75, 2, 2.25, 2.5,
        2.75, 3, 3.25, 3.5, 4, 4.5, 5
    };

    bool initImpl()
    {
        return readRegister(HUSB238_PD_STATUS0) != -1;
    }

};



