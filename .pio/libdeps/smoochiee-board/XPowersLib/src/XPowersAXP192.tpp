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
 * @file      XPowersAXP192.tpp
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2022-05-07
 *
 */
#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <math.h>
#endif /*ARDUINO*/
#include "XPowersCommon.tpp"
#include "REG/AXP192Constants.h"
#include "XPowersLibInterface.hpp"

typedef enum {
    XPOWERS_AXP192_BOOT_TIME_128MS,
    XPOWERS_AXP192_BOOT_TIME_512MS,
    XPOWERS_AXP192_BOOT_TIME_1S,
    XPOWERS_AXP192_BOOT_TIME_2S,
} xpowers_axp192_boot_time_t;




typedef enum {
    XPOWERS_AXP192_CHG_ITERM_LESS_10_PERCENT,
    XPOWERS_AXP192_CHG_ITERM_LESS_15_PERCENT,
} xpowers_axp192_chg_iterm_t;


typedef enum {
    XPOWERS_AXP192_PRECHG_TIMEOUT_30MIN,
    XPOWERS_AXP192_PRECHG_TIMEOUT_40MIN,
    XPOWERS_AXP192_PRECHG_TIMEOUT_50MIN,
    XPOWERS_AXP192_PRECHG_TIMEOUT_60MIN,
} xpoers_axp192_prechg_to_t;

typedef enum {
    XPOWERS_AXP192_POWEROFF_4S,
    XPOWERS_AXP192_POWEROFF_65,
    XPOWERS_AXP192_POWEROFF_8S,
    XPOWERS_AXP192_POWEROFF_10S,
} xpowers_axp192_pekey_poweroff_arg_t;


typedef enum {
    XPOWERS_AXP192_LONGPRESS_1000MS,
    XPOWERS_AXP192_LONGPRESS_1500MS,
    XPOWERS_AXP192_LONGPRESS_2000MS,
    XPOWERS_AXP192_LONGPRESS_2500MS,
} xpowers_axp192_pekey_long_press_t;

typedef enum {
    XPOWERS_AXP192_CHG_CONS_TIMEOUT_7H,
    XPOWERS_AXP192_CHG_CONS_TIMEOUT_8H,
    XPOWERS_AXP192_CHG_CONS_TIMEOUT_9H,
    XPOWERS_AXP192_CHG_CONS_TIMEOUT_10H,
} xpowers_axp192_chg_cons_to_t;


typedef enum {
    XPOWERS_AXP192_BACKUP_BAT_VOL_3V1,
    XPOWERS_AXP192_BACKUP_BAT_VOL_3V,
    XPOWERS_AXP192_BACKUP_BAT_VOL_3V0, //!NEED FIX,DATASHEET ERROR!
    XPOWERS_AXP192_BACKUP_BAT_VOL_2V5,
} xpowers_axp192_backup_batt_vol_t;

typedef enum {
    XPOWERS_AXP192_BACKUP_BAT_CUR_50UA,
    XPOWERS_AXP192_BACKUP_BAT_CUR_100UA,
    XPOWERS_AXP192_BACKUP_BAT_CUR_200UA,
    XPOWERS_AXP192_BACKUP_BAT_CUR_400UA,
} xpowers_axp192_backup_batt_curr_t;

typedef struct {
    uint8_t mode;
} xpowers_axp192_gpio_t;


class XPowersAXP192 :
    public XPowersCommon<XPowersAXP192>, public XPowersLibInterface
{
    friend class XPowersCommon<XPowersAXP192>;


    typedef enum {
        PMU_GPIO0,
        PMU_GPIO1,
        PMU_GPIO2,
        PMU_GPIO3,
        PMU_GPIO4,
        PMU_GPIO5,
        PMU_TS_PIN
    } xpowers_axp192_num_t;

    typedef enum {
        MONITOR_TS_PIN      = _BV(0),
        MONITOR_APS_VOLTAGE = _BV(1),
        MONITOR_USB_CURRENT = _BV(2),
        MONITOR_USB_VOLTAGE = _BV(3),
        MONITOR_AC_CURRENT  = _BV(4),
        MONITOR_AC_VOLTAGE  = _BV(5),
        MONITOR_BAT_CURRENT = _BV(6),
        MONITOR_BAT_VOLTAGE = _BV(7),
        MONITOR_ADC_IO3     = _BV(8),
        MONITOR_ADC_IO2     = _BV(9),
        MONITOR_ADC_IO1     = _BV(10),
        MONITOR_ADC_IO0     = _BV(11),
        MONITOR_TEMPERATURE = _BV(16),
    } axp192_adc_func_t;

    typedef struct {
        uint8_t mode;
    } xpowers_gpio_t;


public:


#if defined(ARDUINO)
    XPowersAXP192(TwoWire &w, int sda = SDA, int scl = SCL, uint8_t addr = AXP192_SLAVE_ADDRESS)
    {
        __wire = &w;
        __sda = sda;
        __scl = scl;
        __addr = addr;
    }
#endif

    XPowersAXP192(uint8_t addr, iic_fptr_t readRegCallback, iic_fptr_t writeRegCallback)
    {
        thisReadRegCallback = readRegCallback;
        thisWriteRegCallback = writeRegCallback;
        __addr = addr;
    }

    XPowersAXP192()
    {
#if defined(ARDUINO)
        __wire = &Wire;
        __sda = SDA;
        __scl = SCL;
#endif
        __addr = AXP192_SLAVE_ADDRESS;
    }

    ~XPowersAXP192()
    {
        log_i("~XPowersAXP192");
        deinit();
    }

#if defined(ARDUINO)
    bool init(TwoWire &w, int sda = SDA, int scl = SCL, uint8_t addr = AXP192_SLAVE_ADDRESS)
    {
        __wire = &w;
        __sda = sda;
        __scl = scl;
        __addr = addr;
        return begin();
    }
#endif

    bool init()
    {
        return begin();
    }

    void deinit()
    {
        end();
    }

    uint16_t status()
    {
        return readRegister(XPOWERS_AXP192_STATUS);
    }

    bool isAcinVbusStart()
    {
        return getRegisterBit(XPOWERS_AXP192_STATUS, 0);
    }

    bool isDischarge()
    {
        return !getRegisterBit(XPOWERS_AXP192_STATUS, 2);
    }

    bool isVbusIn(void)
    {
        return getRegisterBit(XPOWERS_AXP192_STATUS, 5);
    }

    bool isAcinEfficient()
    {
        return getRegisterBit(XPOWERS_AXP192_STATUS, 6);
    }

    bool isAcinIn()
    {
        return getRegisterBit(XPOWERS_AXP192_STATUS, 7);
    }

    bool isOverTemperature()
    {
        return getRegisterBit(XPOWERS_AXP192_MODE_CHGSTATUS, 7);
    }

    bool isCharging(void)
    {
        return getRegisterBit(XPOWERS_AXP192_MODE_CHGSTATUS, 6);
    }

    bool isBatteryConnect(void)
    {
        return getRegisterBit(XPOWERS_AXP192_MODE_CHGSTATUS, 5);
    }

    bool isBattInActiveMode()
    {
        return getRegisterBit(XPOWERS_AXP192_MODE_CHGSTATUS, 3);
    }

    bool isChargeCurrLessPreset()
    {
        return getRegisterBit(XPOWERS_AXP192_MODE_CHGSTATUS, 2);
    }

    void enableVbusVoltageLimit()
    {
        setRegisterBit(XPOWERS_AXP192_IPS_SET, 6);
    }

    void disableVbusVoltageLimit()
    {
        clrRegisterBit(XPOWERS_AXP192_IPS_SET, 6);
    }

    /**
     * @brief  Set VBUS Voltage Input Limit.
     * @param  opt: View the related chip type xpowers_axp192_vbus_vol_limit_t enumeration
     *              parameters in "XPowersParams.hpp"
     */
    void setVbusVoltageLimit(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP192_IPS_SET);
        if (val == -1)return;
        val &= 0xC7;
        writeRegister(XPOWERS_AXP192_IPS_SET, val | (opt << 3));
    }

    /**
    * @brief  Get VBUS Voltage Input Limit.
    * @retval View the related chip type xpowers_axp192_vbus_vol_limit_t enumeration
    *              parameters in "XPowersParams.hpp"
    */
    uint8_t getVbusVoltageLimit(void)
    {
        return (readRegister(XPOWERS_AXP192_IPS_SET) >> 3 ) & 0x7;
    }

    /**
    * @brief  Set VBUS Current Input Limit.
    * @param  opt: View the related chip type xpowers_axp192_vbus_cur_limit_t enumeration
    *              parameters in "XPowersParams.hpp"
    * @retval true valid false invalid
    */
    bool setVbusCurrentLimit(uint8_t opt)
    {
        switch (opt) {
        case XPOWERS_AXP192_VBUS_CUR_LIM_500MA:
            setRegisterBit(XPOWERS_AXP192_IPS_SET, 1);
            return clrRegisterBit(XPOWERS_AXP192_IPS_SET, 0);
        case XPOWERS_AXP192_VBUS_CUR_LIM_100MA:
            setRegisterBit(XPOWERS_AXP192_IPS_SET, 1);
            return setRegisterBit(XPOWERS_AXP192_IPS_SET, 0);
        case XPOWERS_AXP192_VBUS_CUR_LIM_OFF:
            return clrRegisterBit(XPOWERS_AXP192_IPS_SET, 1);
        default:
            break;
        }
        return false;
    }


    /**
    * @brief  Get VBUS Current Input Limit.
    * @retval View the related chip type xpowers_axp192_vbus_cur_limit_t enumeration
    *              parameters in "XPowersParams.hpp"
    */
    uint8_t getVbusCurrentLimit(void)
    {
        if (getRegisterBit(XPOWERS_AXP192_IPS_SET, 1) == 0) {
            return XPOWERS_AXP192_VBUS_CUR_LIM_OFF;
        }
        if (getRegisterBit(XPOWERS_AXP192_IPS_SET, 0)) {
            return XPOWERS_AXP192_VBUS_CUR_LIM_100MA;
        }
        return XPOWERS_AXP192_VBUS_CUR_LIM_500MA;
    }


    // Set the minimum system operating voltage inside the PMU,
    // below this value will shut down the PMU,Adjustment range 2600mV ~ 3300mV
    bool setSysPowerDownVoltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP192_SYS_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP192_SYS_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP192_VOFF_VOL_MIN) {
            log_e("Mistake ! SYS minimum output voltage is  %umV", XPOWERS_AXP192_VOFF_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP192_VOFF_VOL_MAX) {
            log_e("Mistake ! SYS maximum output voltage is  %umV", XPOWERS_AXP192_VOFF_VOL_MAX);
            return false;
        }

        int val = readRegister(XPOWERS_AXP192_VOFF_SET);
        if (val == -1)return false;
        val &= 0xF8;
        val |= (millivolt - XPOWERS_AXP192_VOFF_VOL_MIN) / XPOWERS_AXP192_SYS_VOL_STEPS;
        return 0 ==  writeRegister(XPOWERS_AXP192_VOFF_SET, val);
    }

    uint16_t getSysPowerDownVoltage()
    {
        int val = readRegister(XPOWERS_AXP192_VOFF_SET);
        if (val == -1)return 0;
        val &= 0x07;
        return (val * XPOWERS_AXP192_SYS_VOL_STEPS) + XPOWERS_AXP192_VOFF_VOL_MIN;
    }


    /**
     * @brief  Set shutdown, calling shutdown will turn off all power channels,
     *         only VRTC belongs to normal power supply
     * @retval None
     */
    void shutdown()
    {
        setRegisterBit(XPOWERS_AXP192_OFF_CTL, 7);
    }


    /*
    * Charge setting
    */
    void enableCharge()
    {
        setRegisterBit(XPOWERS_AXP192_CHARGE1, 7);
    }

    void disableCharge()
    {
        clrRegisterBit(XPOWERS_AXP192_CHARGE1, 7);
    }





    /**
    * @brief Set charge target voltage.
    * @param  opt: See xpowers_axp192_chg_vol_t enum for details.
    * @retval
    */
    bool setChargeTargetVoltage(uint8_t opt)
    {
        if (opt >= XPOWERS_AXP192_CHG_VOL_MAX)return false;
        int val = readRegister(XPOWERS_AXP192_CHARGE1);
        if (val == -1) return false;
        val &= 0x9F;
        return 0 == writeRegister(XPOWERS_AXP192_CHARGE1, val | (opt << 5));
    }

    /**
     * @brief Get charge target voltage settings.
     * @retval See xpowers_axp192_chg_vol_t enum for details.
     */
    uint8_t getChargeTargetVoltage()
    {
        int val = readRegister(XPOWERS_AXP192_CHARGE1);
        if (val == -1) return 0;
        return (val & 0x60) >> 5;
    }

    /**
    * @brief Set charge current settings.
    * @retval See xpowers_axp192_chg_curr_t enum for details.
    */
    bool setChargerConstantCurr(uint8_t opt)
    {
        if (opt > 0x0F)return false;
        int val = readRegister(XPOWERS_AXP192_CHARGE1);
        if (val == -1) {
            return false;
        }
        val &= 0xF0;
        return 0 == writeRegister(XPOWERS_AXP192_CHARGE1, val | opt);
    }

    /**
    * @brief Get charge current settings.
    * @retval See xpowers_axp192_chg_curr_t enum for details.
    */
    uint8_t getChargerConstantCurr(void)
    {
        int val = readRegister(XPOWERS_AXP192_CHARGE1) & 0x0F;
        if (val == -1)return XPOWERS_AXP192_CHG_CUR_780MA;
        return val;
    }

    void setChargerTerminationCurr(xpowers_axp192_chg_iterm_t opt)
    {
        switch (opt) {
        case XPOWERS_AXP192_CHG_ITERM_LESS_10_PERCENT:
            clrRegisterBit(XPOWERS_AXP192_CHARGE1, 0);
            break;
        case XPOWERS_AXP192_CHG_ITERM_LESS_15_PERCENT:
            setRegisterBit(XPOWERS_AXP192_CHARGE1, 0);
            break;
        default:
            break;
        }
    }

    uint8_t getChargerTerminationCurr()
    {
        return getRegisterBit(XPOWERS_AXP192_CHARGE1, 4);
    }

    bool setPrechargeTimeout(xpoers_axp192_prechg_to_t opt)
    {
        int val = readRegister(XPOWERS_AXP192_CHARGE2);
        if (val == -1)return false;
        val &= 0x3F;
        return 0 == writeRegister(XPOWERS_AXP192_CHARGE2, val | (opt << 6));
    }

    // External channel charge current setting,Range:300~1000mA
    bool setChargerExternChannelCurr(uint16_t milliampere)
    {
        if (milliampere % XPOWERS_AXP192_CHG_EXT_CURR_STEP) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP192_CHG_EXT_CURR_STEP);
            return false;
        }
        if (milliampere < XPOWERS_AXP192_CHG_EXT_CURR_MIN) {
            log_e("Mistake ! The minimum external path charge current setting is:  %umA", XPOWERS_AXP192_CHG_EXT_CURR_MIN);
            return false;
        } else if (milliampere > XPOWERS_AXP192_CHG_EXT_CURR_MAX) {
            log_e("Mistake ! The maximum external channel charge current setting is:  %umA", XPOWERS_AXP192_CHG_EXT_CURR_MAX);
            return false;
        }
        int val = readRegister(XPOWERS_AXP192_CHARGE2);
        if (val == -1)return false;
        val &= 0xC7;
        val |= ((milliampere - XPOWERS_AXP192_CHG_EXT_CURR_MIN ) / XPOWERS_AXP192_CHG_EXT_CURR_STEP);
        return 0 == writeRegister(XPOWERS_AXP192_CHARGE2, val);
    }

    bool enableChargerExternChannel()
    {
        return setRegisterBit(XPOWERS_AXP192_CHARGE2, 2);
    }

    bool disableChargerExternChannel()
    {
        return clrRegisterBit(XPOWERS_AXP192_CHARGE2, 2);
    }

    // Timeout setting in constant current mode
    bool setChargerConstantTimeout(xpowers_axp192_chg_cons_to_t opt)
    {
        int val = readRegister(XPOWERS_AXP192_CHARGE2);
        if (val == -1)return false;
        val &= 0xFC;
        return 0 == writeRegister(XPOWERS_AXP192_CHARGE2, val | opt);
    }

    bool enableBackupBattCharger()
    {
        return setRegisterBit(XPOWERS_AXP192_BACKUP_CHG, 7);
    }


    bool disableBackupBattCharger()
    {
        return clrRegisterBit(XPOWERS_AXP192_BACKUP_CHG, 7);
    }

    bool isEnableBackupCharger()
    {
        return getRegisterBit(XPOWERS_AXP192_BACKUP_CHG, 7);
    }

    bool setBackupBattChargerVoltage(xpowers_axp192_backup_batt_vol_t opt)
    {
        int val = readRegister(XPOWERS_AXP192_BACKUP_CHG);
        if (val == -1)return false;
        val &= 0x9F;
        return 0 == writeRegister(XPOWERS_AXP192_BACKUP_CHG, val | (opt << 5));
    }

    bool setBackupBattChargerCurr(xpowers_axp192_backup_batt_curr_t opt)
    {
        int val = readRegister(XPOWERS_AXP192_BACKUP_CHG);
        if (val == -1)return false;
        val &= 0xFC;
        return 0 == writeRegister(XPOWERS_AXP192_BACKUP_CHG, val | opt);
    }

    /*
    * Temperature
    */
    float getTemperature()
    {
        return readRegisterH8L4(XPOWERS_AXP192_INTERNAL_TEMP_H8, XPOWERS_AXP192_INTERNAL_TEMP_L4)
               * XPOWERS_AXP192_INTERNAL_TEMP_STEP - XPOWERS_AXP192_INERNAL_TEMP_OFFSET;
    }

    bool enableTemperatureMeasure()
    {
        return  setRegisterBit(XPOWERS_AXP192_ADC_EN2, 7);
    }

    bool disableTemperatureMeasure()
    {
        return  clrRegisterBit(XPOWERS_AXP192_ADC_EN2, 7);
    }

    /*
    * Power control LDOio functions
    */
    bool isEnableLDOio(void)
    {
        int val = readRegister(XPOWERS_AXP192_GPIO0_CTL);
        return (val & 0x02);
    }

    bool enableLDOio(void)
    {
        int val = readRegister(XPOWERS_AXP192_GPIO0_CTL) & 0xF8;
        return 0 == writeRegister(XPOWERS_AXP192_GPIO0_CTL, val | 0x02);
    }

    bool disableLDOio(void)
    {
        int val = readRegister(XPOWERS_AXP192_GPIO0_CTL) & 0xF8;
        return 0 == writeRegister(XPOWERS_AXP192_GPIO0_CTL, val);
    }

    bool setLDOioVoltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP192_LDOIO_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP192_LDOIO_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP192_LDOIO_VOL_MIN) {
            log_e("Mistake ! LDOIO minimum output voltage is  %umV", XPOWERS_AXP192_LDOIO_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP192_LDOIO_VOL_MAX) {
            log_e("Mistake ! LDOIO maximum output voltage is  %umV", XPOWERS_AXP192_LDOIO_VOL_MAX);
            return false;
        }
        int val = readRegister(XPOWERS_AXP192_GPIO0_VOL);
        if (val == -1)return false;
        val |=  (((millivolt - XPOWERS_AXP192_LDOIO_VOL_MIN) / XPOWERS_AXP192_LDOIO_VOL_STEPS) << 4);
        return 0 == writeRegister(XPOWERS_AXP192_GPIO0_VOL, val);
    }

    uint16_t getLDOioVoltage(void)
    {
        int val = readRegister(XPOWERS_AXP192_GPIO0_VOL);
        if (val == -1)return 0;
        val >>= 4;
        val *= XPOWERS_AXP192_LDOIO_VOL_STEPS;
        val += XPOWERS_AXP192_LDOIO_VOL_MIN;
        return val;
    }

    /*
    * Power control LDO2 functions
    */
    bool isEnableLDO2(void)
    {
        return getRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 2);
    }

    bool enableLDO2(void)
    {
        return setRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 2);
    }

    bool disableLDO2(void)
    {
        return clrRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 2);
    }

    bool setLDO2Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP192_LDO2_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP192_LDO2_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP192_LDO2_VOL_MIN) {
            log_e("Mistake ! LDO2 minimum output voltage is  %umV", XPOWERS_AXP192_LDO2_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP192_LDO2_VOL_MAX) {
            log_e("Mistake ! LDO2 maximum output voltage is  %umV", XPOWERS_AXP192_LDO2_VOL_MAX);
            return false;
        }

        int val = readRegister(XPOWERS_AXP192_LDO23OUT_VOL);
        if (val == -1) return false;
        val  &= 0x0F;
        return 0 == writeRegister(XPOWERS_AXP192_LDO23OUT_VOL, val | (((millivolt - XPOWERS_AXP192_LDO2_VOL_MIN) / XPOWERS_AXP192_LDO2_VOL_STEPS) << XPOWERS_AXP192_LDO2_VOL_BIT_MASK));
    }

    uint16_t getLDO2Voltage(void)
    {
        int val = readRegister(XPOWERS_AXP192_LDO23OUT_VOL) & 0xF0;
        return (val >> XPOWERS_AXP192_LDO2_VOL_BIT_MASK) * XPOWERS_AXP192_LDO2_VOL_STEPS + XPOWERS_AXP192_LDO2_VOL_MIN;
    }

    /*
     * Power control LDO3 functions
     */
    bool isEnableLDO3(void)
    {
        return getRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 3);
    }

    bool enableLDO3(void)
    {
        return setRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 3);
    }

    bool disableLDO3(void)
    {
        return clrRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 3);
    }


    bool setLDO3Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP192_LDO3_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP192_LDO3_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP192_LDO3_VOL_MIN) {
            log_e("Mistake ! LDO3 minimum output voltage is  %umV", XPOWERS_AXP192_LDO3_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP192_LDO3_VOL_MAX) {
            log_e("Mistake ! LDO3 maximum output voltage is  %umV", XPOWERS_AXP192_LDO3_VOL_MAX);
            return false;
        }

        int val = readRegister(XPOWERS_AXP192_LDO23OUT_VOL) & 0xF0;
        return 0 == writeRegister(XPOWERS_AXP192_LDO23OUT_VOL, val | ((millivolt - XPOWERS_AXP192_LDO3_VOL_MIN) / XPOWERS_AXP192_LDO3_VOL_STEPS));
    }

    uint16_t getLDO3Voltage(void)
    {
        int val = readRegister(XPOWERS_AXP192_LDO23OUT_VOL);
        if (val == -1)return 0;
        val &= 0x0F;
        return (val * XPOWERS_AXP192_LDO3_VOL_STEPS) + XPOWERS_AXP192_LDO3_VOL_MIN;
    }

    /*
     * Power control DCDC1 functions
     */
    void setDC1PwmMode(void)
    {
        int val = readRegister(XPOWERS_AXP192_DCDC_MODESET) & 0xF7;
        writeRegister(XPOWERS_AXP192_DCDC_MODESET, val | 0x08);
    }

    void setDC1AutoMode(void)
    {
        int val = readRegister(XPOWERS_AXP192_DCDC_MODESET) & 0xF7;
        writeRegister(XPOWERS_AXP192_DCDC_MODESET, val);
    }

    bool isEnableDC1(void)
    {
        return getRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 0);
    }

    bool enableDC1(void)
    {
        return setRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 0);
    }

    bool disableDC1(void)
    {
        return clrRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 0);
    }

    bool setDC1Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP192_DC1_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP192_DC1_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP192_DC1_VOL_STEPS) {
            log_e("Mistake ! DCDC1 minimum output voltage is  %umV", XPOWERS_AXP192_DC1_VOL_STEPS);
            return false;
        } else if (millivolt > XPOWERS_AXP192_DC1_VOL_MAX) {
            log_e("Mistake ! DCDC1 maximum output voltage is  %umV", XPOWERS_AXP192_DC1_VOL_MAX);
            return false;
        }

        int val = readRegister(XPOWERS_AXP192_DC1_VLOTAGE);
        if (val == -1)return false;
        val &= 0x80;
        val |= (millivolt - XPOWERS_AXP192_DC1_VOL_MIN) / XPOWERS_AXP192_DC1_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP192_DC1_VLOTAGE, val);
    }

    uint16_t getDC1Voltage(void)
    {
        int val = readRegister(XPOWERS_AXP192_DC1_VLOTAGE) & 0x7F;
        return val * XPOWERS_AXP192_DC1_VOL_STEPS + XPOWERS_AXP192_DC1_VOL_MIN;
    }

    /*
     * Power control DCDC2 functions
     */
    void setDC2PwmMode(void)
    {
        int val = readRegister(XPOWERS_AXP192_DCDC_MODESET) & 0xFB;
        writeRegister(XPOWERS_AXP192_DCDC_MODESET, val | 0x04);
    }

    void setDC2AutoMode(void)
    {
        int val = readRegister(XPOWERS_AXP192_DCDC_MODESET) & 0xFB;
        writeRegister(XPOWERS_AXP192_DCDC_MODESET, val);
    }

    void enableDC2VRC(void)
    {
        int val =  readRegister(XPOWERS_AXP192_DC2_DVM);
        writeRegister(XPOWERS_AXP192_DC2_DVM, val | 0x04);
    }

    void disableDC2VRC(void)
    {
        int val =  readRegister(XPOWERS_AXP192_DC2_DVM);
        writeRegister(XPOWERS_AXP192_DC2_DVM, val & 0xFB);
    }

    bool setDC2VRC(uint8_t opts)
    {
        if (opts > 1) {
            return false;
        }
        int val =  readRegister(XPOWERS_AXP192_DC2_DVM) & 0xFE;
        writeRegister(XPOWERS_AXP192_DC2_DVM, val | opts);
    }

    bool isEnableDC2VRC(void)
    {
        return (readRegister(XPOWERS_AXP192_DC2_DVM) & 0x04) == 0x04;
    }

    bool isEnableDC2(void)
    {
        return getRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 4);
    }

    bool enableDC2(void)
    {
        return setRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 4);
    }

    bool disableDC2(void)
    {
        return clrRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 4);
    }

    bool setDC2Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP192_DC2_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP192_DC2_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP192_DC2_VOL_MIN) {
            log_e("Mistake ! DCDC2 minimum output voltage is  %umV", XPOWERS_AXP192_DC2_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP192_DC2_VOL_MAX) {
            log_e("Mistake ! DCDC2 maximum output voltage is  %umV", XPOWERS_AXP192_DC2_VOL_MAX);
            return false;
        }
        int val = readRegister(XPOWERS_AXP192_DC2OUT_VOL);
        if (val == -1)return false;
        val  &= 0x80;
        val |= (millivolt - XPOWERS_AXP192_DC2_VOL_MIN) / XPOWERS_AXP192_DC2_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP192_DC2OUT_VOL, val);
    }

    uint16_t getDC2Voltage(void)
    {
        int val = readRegister(XPOWERS_AXP192_DC2OUT_VOL);
        if (val == -1)return 0;
        return (val * XPOWERS_AXP192_DC2_VOL_STEPS) + XPOWERS_AXP192_DC2_VOL_MIN;
    }

    /*
     * Power control DCDC3 functions
     */
    void setDC3PwmMode(void)
    {
        int val = readRegister(XPOWERS_AXP192_DCDC_MODESET) & 0xFD;
        writeRegister(XPOWERS_AXP192_DCDC_MODESET, val | 0x02);
    }

    void setDC3AutoMode(void)
    {
        int val = readRegister(XPOWERS_AXP192_DCDC_MODESET) & 0xFD;
        writeRegister(XPOWERS_AXP192_DCDC_MODESET, val);
    }

    bool isEnableDC3(void)
    {
        return getRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 1);
    }

    bool enableDC3(void)
    {
        return setRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 1);
    }

    bool disableDC3(void)
    {
        return clrRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 1);
    }

    bool setDC3Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP192_DC3_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP192_DC3_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP192_DC3_VOL_MIN) {
            log_e("Mistake ! DCDC3 minimum output voltage is  %umV", XPOWERS_AXP192_DC3_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP192_DC3_VOL_MAX) {
            log_e("Mistake ! DCDC3 maximum output voltage is  %umV", XPOWERS_AXP192_DC3_VOL_MAX);
            return false;
        }
        return 0 == writeRegister(XPOWERS_AXP192_DC3OUT_VOL, (millivolt - XPOWERS_AXP192_DC3_VOL_MIN) / XPOWERS_AXP192_DC3_VOL_STEPS);
    }

    uint16_t getDC3Voltage(void)
    {
        int val = readRegister(XPOWERS_AXP192_DC3OUT_VOL);
        if (val == -1)return 0;
        return (val * XPOWERS_AXP192_DC3_VOL_STEPS) + XPOWERS_AXP192_DC3_VOL_MIN;
    }

    /*
     * Power control EXTEN functions
     */
    bool enableExternalPin(void)
    {
        return setRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 6);
    }

    bool disableExternalPin(void)
    {
        return clrRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 6);
    }

    bool isEnableExternalPin(void)
    {
        return getRegisterBit(XPOWERS_AXP192_LDO23_DC123_EXT_CTL, 6);
    }

    /*
    * Interrupt status functions
    */

    /**
    * @brief  Get the interrupt controller mask value.
    * @retval   Mask value corresponds to xpowers_axp192_irq_t ,
    */
    uint64_t getIrqStatus(void)
    {
        statusRegister[0] = readRegister(XPOWERS_AXP192_INTSTS1);
        statusRegister[1] = readRegister(XPOWERS_AXP192_INTSTS2);
        statusRegister[2] = readRegister(XPOWERS_AXP192_INTSTS3);
        statusRegister[3] = readRegister(XPOWERS_AXP192_INTSTS4);
        statusRegister[4] = readRegister(XPOWERS_AXP192_INTSTS5);
        return ((uint64_t)statusRegister[4]) << 32 |
               ((uint64_t)statusRegister[3]) << 24 |
               ((uint64_t)statusRegister[2]) << 16 |
               ((uint64_t)statusRegister[1]) << 8  |
               ((uint64_t)statusRegister[0]);
    }

    /**
     * @brief  Clear interrupt controller state.
     */
    void clearIrqStatus(void)
    {
        for (int i = 0; i < 4; i++) {
            writeRegister(XPOWERS_AXP192_INTSTS1 + i, 0xFF);
        }
        writeRegister(XPOWERS_AXP192_INTSTS5, 0xFF);
    }

    /**
     * @brief  Enable PMU interrupt control mask .
     * @param  opt: View the related chip type xpowers_axp192_irq_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval
     */
    bool enableIRQ(uint64_t opt)
    {
        return setInterruptImpl(opt, true);
    }

    /**
     * @brief  Disable PMU interrupt control mask .
     * @param  opt: View the related chip type xpowers_axp192_irq_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval
     */
    bool disableIRQ(uint64_t opt)
    {
        return setInterruptImpl(opt, false);
    }

    bool isAcinOverVoltageIrq(void)
    {
        return (bool)(statusRegister[0] & _BV(7));
    }

    bool isAcinInserIrq(void)
    {
        return (bool)(statusRegister[0] & _BV(6));
    }

    bool isAcinRemoveIrq(void)
    {
        return (bool)(statusRegister[0] & _BV(5));
    }

    bool isVbusOverVoltageIrq(void)
    {
        return (bool)(statusRegister[0] & _BV(4));
    }

    bool isVbusInsertIrq(void)
    {
        return (bool)(statusRegister[0] & _BV(3));
    }

    bool isVbusRemoveIrq(void)
    {
        return (bool)(statusRegister[0] & _BV(2));
    }

    bool isVbusLowVholdIrq(void)
    {
        return (bool)(statusRegister[0] & _BV(1));
    }

    bool isBatInsertIrq(void)
    {
        return (bool)(statusRegister[1] & _BV(7));
    }

    bool isBatRemoveIrq(void)
    {
        return (bool)(statusRegister[1] & _BV(6));
    }

    bool isBattEnterActivateIrq(void)
    {
        return (bool)(statusRegister[1] & _BV(5));
    }

    bool isBattExitActivateIrq(void)
    {
        return (bool)(statusRegister[1] & _BV(4));
    }

    bool isBatChargeStartIrq(void)
    {
        return (bool)(statusRegister[1] & _BV(3));
    }

    bool isBatChargeDoneIrq(void)
    {
        return (bool)(statusRegister[1] & _BV(2));
    }

    bool isBattTempHighIrq(void)
    {
        return (bool)(statusRegister[1] & _BV(1));
    }

    bool isBattTempLowIrq(void)
    {
        return (bool)(statusRegister[1] & _BV(0));
    }

    bool isChipOverTemperatureIrq(void)
    {
        return (bool)(statusRegister[2] & _BV(7));
    }

    bool isChargingCurrentLessIrq(void)
    {
        return (bool)(statusRegister[2] & _BV(6));
    }

    bool isDC1VoltageLessIrq(void)
    {
        return (bool)(statusRegister[2] & _BV(5));
    }

    bool isDC2VoltageLessIrq(void)
    {
        return (bool)(statusRegister[2] & _BV(4));
    }

    bool isDC3VoltageLessIrq(void)
    {
        return (bool)(statusRegister[2] & _BV(3));
    }

    bool isPekeyShortPressIrq(void)
    {
        return (bool)(statusRegister[2] & _BV(1));
    }

    bool isPekeyLongPressIrq(void)
    {
        return (bool)(statusRegister[2] & _BV(0));
    }

    bool isNOEPowerOnIrq(void)
    {
        return (bool)(statusRegister[3] & _BV(7));
    }

    bool isNOEPowerDownIrq(void)
    {
        return (bool)(statusRegister[3] & _BV(6));
    }

    bool isVbusEffectiveIrq(void)
    {
        return (bool)(statusRegister[3] & _BV(5));
    }

    bool isVbusInvalidIrq(void)
    {
        return (bool)(statusRegister[3] & _BV(4));
    }

    bool isVbusSessionIrq(void)
    {
        return (bool)(statusRegister[3] & _BV(3));
    }

    bool isVbusSessionEndIrq(void)
    {
        return (bool)(statusRegister[3] & _BV(2));
    }

    bool isLowVoltageLevel2Irq(void)
    {
        return (bool)(statusRegister[3] & _BV(0));
    }

    //IRQ5 REGISTER :
    bool isWdtExpireIrq(void)
    {
        return (bool)(statusRegister[4] & _BV(7));
    }

    bool isGpio2EdgeTriggerIrq(void)
    {
        return (bool)(statusRegister[4] & _BV(2));
    }

    bool isGpio1EdgeTriggerIrq(void)
    {
        return (bool)(statusRegister[4] & _BV(1));
    }

    bool isGpio0EdgeTriggerIrq(void)
    {
        return (bool)(statusRegister[4] & _BV(0));
    }

    /*
     *   ADC Functions
     */

    bool enableBattDetection()
    {
        return setRegisterBit(XPOWERS_AXP192_OFF_CTL, 6);
    }

    bool disableBattDetection()
    {
        return clrRegisterBit(XPOWERS_AXP192_OFF_CTL, 6);
    }

    bool enableVbusVoltageMeasure()
    {
        return setSignalCaptureImpl(MONITOR_USB_CURRENT | MONITOR_USB_VOLTAGE, true);
    }

    bool disableVbusVoltageMeasure()
    {
        return setSignalCaptureImpl(MONITOR_USB_CURRENT | MONITOR_USB_VOLTAGE, false);
    }

    bool enableBattVoltageMeasure()
    {
        return setSignalCaptureImpl(MONITOR_BAT_CURRENT | MONITOR_BAT_VOLTAGE, true);
    }

    bool disableBattVoltageMeasure()
    {
        return setSignalCaptureImpl(MONITOR_BAT_CURRENT | MONITOR_BAT_VOLTAGE, false);
    }

    bool enableSystemVoltageMeasure()
    {
        return setSignalCaptureImpl(MONITOR_APS_VOLTAGE, true);
    }

    bool disableSystemVoltageMeasure()
    {
        return setSignalCaptureImpl(MONITOR_APS_VOLTAGE, false);
    }

    bool enableTSPinMeasure()
    {
        return setSignalCaptureImpl(MONITOR_TS_PIN, true);
    }

    bool disableTSPinMeasure()
    {
        return setSignalCaptureImpl(MONITOR_TS_PIN, false);
    }

    bool enableAdcChannel(uint32_t opts)
    {
        return setSignalCaptureImpl(opts, true);
    }

    bool disableAdcChannel(uint32_t opts)
    {
        return setSignalCaptureImpl(opts, false);
    }

    uint16_t getVbusVoltage()
    {
        if (!isVbusIn()) {
            return 0;
        }
        return readRegisterH8L4(XPOWERS_AXP192_VBUS_VOL_H8,
                                XPOWERS_AXP192_VBUS_VOL_L4
                               ) * XPOWERS_AXP192_VBUS_VOLTAGE_STEP;
    }

    float getVbusCurrent()
    {
        if (!isVbusIn()) {
            return 0;
        }
        return readRegisterH8L4(XPOWERS_AXP192_VBUS_CUR_H8,
                                XPOWERS_AXP192_VBUS_CUR_L4
                               ) * XPOWERS_AXP192_VBUS_CUR_STEP;
    }

    uint16_t getBattVoltage()
    {
        if (!isBatteryConnect()) {
            return 0;
        }
        return readRegisterH8L4(XPOWERS_AXP192_BAT_AVERVOL_H8,
                                XPOWERS_AXP192_BAT_AVERVOL_L4
                               ) * XPOWERS_AXP192_BATT_VOLTAGE_STEP;
    }

    float getBattDischargeCurrent()
    {
        if (!isBatteryConnect()) {
            return 0;
        }
        return readRegisterH8L5(XPOWERS_AXP192_BAT_AVERDISCHGCUR_H8,
                                XPOWERS_AXP192_BAT_AVERDISCHGCUR_L5) * XPOWERS_AXP192_BATT_DISCHARGE_CUR_STEP;
    }

    uint16_t getAcinVoltage()
    {
        if (!isAcinIn()) {
            return 0;
        }
        return readRegisterH8L4(XPOWERS_AXP192_ACIN_VOL_H8, XPOWERS_AXP192_ACIN_VOL_L4) * XPOWERS_AXP192_ACIN_VOLTAGE_STEP;
    }

    float getAcinCurrent()
    {
        if (!isAcinIn()) {
            return 0;
        }
        return readRegisterH8L4(XPOWERS_AXP192_ACIN_CUR_H8, XPOWERS_AXP192_ACIN_CUR_L4) * XPOWERS_AXP192_ACIN_CUR_STEP;
    }

    uint16_t getSystemVoltage()
    {
        return readRegisterH8L4(XPOWERS_AXP192_APS_AVERVOL_H8, XPOWERS_AXP192_APS_AVERVOL_L4) * XPOWERS_AXP192_APS_VOLTAGE_STEP;
    }

    /*
    * Timer Control
    */
    void setTimerout(uint8_t minute)
    {
        writeRegister(XPOWERS_AXP192_TIMER_CTL, 0x80 | minute);
    }

    void disableTimer()
    {
        writeRegister(XPOWERS_AXP192_TIMER_CTL, 0x80);
    }

    void clearTimerFlag()
    {
        setRegisterBit(XPOWERS_AXP192_TIMER_CTL, 7);
    }

    /*
    * Data Buffer
    */
    bool writeDataBuffer(uint8_t *data, uint8_t size)
    {
        if (size > XPOWERS_AXP192_DATA_BUFFER_SIZE)return false;
        for (int i = 0; i < size; ++i) {
            writeRegister(XPOWERS_AXP192_DATA_BUFFER1 + i, data[i]);
        }
        return true;
    }

    bool readDataBuffer(uint8_t *data, uint8_t size)
    {
        if (size > XPOWERS_AXP192_DATA_BUFFER_SIZE)return false;
        for (int i = 0; i < size; ++i) {
            data[i] = readRegister(XPOWERS_AXP192_DATA_BUFFER1 + i);
        }
        return true;
    }

    /*
     * Charge led functions
     */

    /**
    * @brief Set charging led mode.
    * @retval See xpowers_chg_led_mode_t enum for details.
    */
    void setChargingLedMode(uint8_t mode)
    {
        int val;
        switch (mode) {
        case XPOWERS_CHG_LED_OFF:
        case XPOWERS_CHG_LED_BLINK_1HZ:
        case XPOWERS_CHG_LED_BLINK_4HZ:
        case XPOWERS_CHG_LED_ON:
            val = readRegister(XPOWERS_AXP192_OFF_CTL);
            if (val == -1)return;
            val &= 0xC7;
            val |= 0x08;      //use manual ctrl
            val |= (mode << 4);
            writeRegister(XPOWERS_AXP192_OFF_CTL, val);
            break;
        case XPOWERS_CHG_LED_CTRL_CHG:
            clrRegisterBit(XPOWERS_AXP192_OFF_CTL, 3);
            break;
        default:
            break;
        }
    }

    uint8_t getChargingLedMode()
    {
        if (!getRegisterBit(XPOWERS_AXP192_OFF_CTL, 3)) {
            return XPOWERS_CHG_LED_CTRL_CHG;
        }
        int val = readRegister(XPOWERS_AXP192_OFF_CTL);
        if (val == -1)return XPOWERS_CHG_LED_OFF;
        val &= 0x30;
        return val >> 4;
    }

    /*
     * Coulomb counter control
     */
    void enableCoulomb()
    {
        setRegisterBit(XPOWERS_AXP192_COULOMB_CTL, 7);
    }

    void disableCoulomb()
    {
        clrRegisterBit(XPOWERS_AXP192_COULOMB_CTL, 7);
    }

    void stopCoulomb()
    {
        setRegisterBit(XPOWERS_AXP192_COULOMB_CTL, 6);
    }

    void clearCoulomb()
    {
        setRegisterBit(XPOWERS_AXP192_COULOMB_CTL, 5);
    }

    uint32_t getBattChargeCoulomb()
    {
        int data[4];
        data[0] = readRegister(XPOWERS_AXP192_BAT_CHGCOULOMB3);
        data[1] = readRegister(XPOWERS_AXP192_BAT_CHGCOULOMB2);
        data[2] = readRegister(XPOWERS_AXP192_BAT_CHGCOULOMB1);
        data[3] = readRegister(XPOWERS_AXP192_BAT_CHGCOULOMB0);
        for (int i = 0; i < 4; ++i) {
            if (data[i] == -1)return 0;
        }
        return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];
    }

    uint32_t getBattDischargeCoulomb()
    {
        int data[4];
        data[0] = readRegister(XPOWERS_AXP192_BAT_DISCHGCOULOMB3);
        data[1] = readRegister(XPOWERS_AXP192_BAT_DISCHGCOULOMB2);
        data[2] = readRegister(XPOWERS_AXP192_BAT_DISCHGCOULOMB1);
        data[3] = readRegister(XPOWERS_AXP192_BAT_DISCHGCOULOMB0);
        for (int i = 0; i < 4; ++i) {
            if (data[i] == -1)return 0;
        }
        return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];
    }

    uint8_t getAdcSamplingRate(void)
    {
        int val = readRegister(XPOWERS_AXP192_ADC_SPEED);
        if (val == -1)return 0;
        return 25 * (int)pow(2, (val & 0xC0) >> 6);
    }

    float getCoulombData(void)
    {
        uint32_t charge = getBattChargeCoulomb(), discharge = getBattDischargeCoulomb();
        uint8_t rate = getAdcSamplingRate();
        float result = 65536.0 * 0.5 * ((float)charge - (float)discharge) / 3600.0 / rate;
        return result;
    }

    /*
     * GPIO control functions
     */
    float getBatteryChargeCurrent(void)
    {
        return readRegisterH8L5(
                   XPOWERS_AXP192_BAT_AVERCHGCUR_H8,
                   XPOWERS_AXP192_BAT_AVERCHGCUR_L5
               ) * XPOWERS_AXP192_BATT_CHARGE_CUR_STEP;
    }

    uint16_t getGpio0Voltage()
    {
        return readRegisterH8L4(XPOWERS_AXP192_GPIO0_VOL_ADC_H8, XPOWERS_AXP192_GPIO0_VOL_ADC_L4) * XPOWERS_AXP192_GPIO0_STEP * 1000;
    }

    uint16_t getGpio1Voltage()
    {
        return readRegisterH8L4(XPOWERS_AXP192_GPIO1_VOL_ADC_H8, XPOWERS_AXP192_GPIO1_VOL_ADC_L4) * XPOWERS_AXP192_GPIO1_STEP * 1000;
    }

    int8_t pwmSetup(uint8_t channel, uint8_t freq, uint16_t duty)
    {
        // PWM输出频率 = 2.25MHz / (X+1) / Y1
        // PWM输出占空比 = Y2 / Y1
        switch (channel) {
        case 0:
            writeRegister(XPOWERS_AXP192_PWM1_FREQ_SET,  freq);
            writeRegister(XPOWERS_AXP192_PWM1_DUTY_SET1, duty >> 8);
            writeRegister(XPOWERS_AXP192_PWM1_DUTY_SET2, duty & 0xFF);
            break;
        case 1:
            writeRegister(XPOWERS_AXP192_PWM2_FREQ_SET,  freq);
            writeRegister(XPOWERS_AXP192_PWM2_DUTY_SET1, duty >> 8);
            writeRegister(XPOWERS_AXP192_PWM2_DUTY_SET2, duty & 0xFF);
            break;
        default:
            return -1;
            break;
        }
        return 0;
    }

    int8_t pwmEnable(uint8_t channel)
    {
        int val = 0;
        switch (channel) {
        case 0:
            val = readRegister(XPOWERS_AXP192_GPIO1_CTL) & 0xF8;
            writeRegister(XPOWERS_AXP192_GPIO1_CTL, val | 0x02);
            return 0;
        case 1:
            val = readRegister(XPOWERS_AXP192_GPIO2_CTL) & 0xF8;
            writeRegister(XPOWERS_AXP192_GPIO2_CTL, val | 0x02);
            return 0;
        default:
            break;
        }
        return -1;
    }

    int getBatteryPercent(void)
    {
        if (!isBatteryConnect()) {
            return -1;
        }
        const static int table[11] = {
            3000, 3650, 3700, 3740, 3760, 3795,
            3840, 3910, 3980, 4070, 4150
        };
        uint16_t voltage = getBattVoltage();
        if (voltage < table[0])
            return 0;
        for (int i = 0; i < 11; i++) {
            if (voltage < table[i])
                return i * 10 - (10UL * (int)(table[i] - voltage)) /
                       (int)(table[i] - table[i - 1]);;
        }
        return 100;
    }

    uint8_t getChipID(void)
    {
        return readRegister(XPOWERS_AXP192_IC_TYPE);
    }





    /*
    * GPIO setting
    */
    int8_t pinMode(uint8_t pin, uint8_t mode)
    {
        int val = 0;
        switch (pin) {
        case PMU_GPIO0:
            /*
            * 000: NMOS open-drain output
            * 001: Universal input function
            * 010: Low noise LDO
            * 011: reserved
            * 100: ADC input
            * 101: Low output
            * 11X: Floating
            * * */
            if (mode == INPUT || mode == INPUT_PULLDOWN) {
                if (gpio[pin].mode != INPUT) {
                    gpio[pin].mode = INPUT;
                }
                val = readRegister(XPOWERS_AXP192_GPIO0_CTL) & 0xF8;
                writeRegister(XPOWERS_AXP192_GPIO0_CTL, val | 0x01);
                //Set pull-down mode
                val = readRegister(XPOWERS_AXP192_GPIO012_PULLDOWN) & 0xFE;
                if (mode == INPUT_PULLDOWN ) {
                    writeRegister(XPOWERS_AXP192_GPIO012_PULLDOWN, val | 0x01);
                } else {
                    writeRegister(XPOWERS_AXP192_GPIO012_PULLDOWN, val);
                }
            }
            break;

        case PMU_GPIO1:
            /*
            * 000: NMOS open-drain output
            * 001: Universal input function
            * 010: PWM1 output, high level is VINT, not Can add less than 100K pull-down resistance
            * 011: reserved
            * 100: ADC input
            * 101: Low output
            * 11X: Floating
            * * */
            if (mode == INPUT || mode == INPUT_PULLDOWN) {
                if (gpio[pin].mode != INPUT) {
                    gpio[pin].mode = INPUT;
                }
                val = readRegister(XPOWERS_AXP192_GPIO1_CTL) & 0xF8;
                writeRegister(XPOWERS_AXP192_GPIO1_CTL, val | 0x01);

                //Set pull-down mode
                val = readRegister(XPOWERS_AXP192_GPIO012_PULLDOWN) & 0xFD;
                if (mode == INPUT_PULLDOWN ) {
                    writeRegister(XPOWERS_AXP192_GPIO012_PULLDOWN, val | 0x02);
                } else {
                    writeRegister(XPOWERS_AXP192_GPIO012_PULLDOWN, val);
                }
            }
            break;

        case PMU_GPIO2:
            /*
            * 000: NMOS open-drain output
            * 001: Universal input function
            * 010: PWM2 output, high level is VINT, not Can add less than 100K pull-down resistance
            * 011: reserved
            * 100: ADC input
            * 101: Low output
            * 11X: Floating
            * */
            if (mode == INPUT || mode == INPUT_PULLDOWN) {
                if (gpio[pin].mode != INPUT) {
                    gpio[pin].mode = INPUT;
                }
                val = readRegister(XPOWERS_AXP192_GPIO2_CTL) & 0xF8;
                writeRegister(XPOWERS_AXP192_GPIO2_CTL, val | 0x01);

                //Set pull-down mode
                val = readRegister(XPOWERS_AXP192_GPIO012_PULLDOWN) & 0xFB;
                if (mode == INPUT_PULLDOWN ) {
                    writeRegister(XPOWERS_AXP192_GPIO012_PULLDOWN, val | 0x04);
                } else {
                    writeRegister(XPOWERS_AXP192_GPIO012_PULLDOWN, val);
                }
            }
            break;

        case PMU_GPIO3:
            /*
            * 00: External charging control
            * 01: NMOS open-drain output port 3
            * 10: Universal input port 3
            * 11: ADC input
            * * */
            if (mode == INPUT) {
                if (gpio[pin].mode != INPUT) {
                    gpio[pin].mode = INPUT;
                }
                val = readRegister(XPOWERS_AXP192_GPIO34_CTL) & 0xFC;
                writeRegister(XPOWERS_AXP192_GPIO34_CTL, val | 0x82 );
            }
            break;

        case PMU_GPIO4:
            /*
            * 00: External charging control
            * 01: NMOS open-drain output port 4
            * 10: Universal input port 4
            * 11: undefined
            * * */
            if (mode == INPUT) {
                if (gpio[pin].mode != INPUT) {
                    gpio[pin].mode = INPUT;
                }
                val = readRegister(XPOWERS_AXP192_GPIO34_CTL) & 0xF3;
                writeRegister(XPOWERS_AXP192_GPIO34_CTL, val | 0x88 );
            }
            break;
        case PMU_GPIO5:
            if (mode == INPUT) {
                if (gpio[pin].mode != INPUT) {
                    gpio[pin].mode = INPUT;
                }
                val = readRegister(XPOWERS_AXP192_GPIO5_CTL) & 0xBF;
                writeRegister(XPOWERS_AXP192_GPIO5_CTL, val | 0x40);
            }
            break;
        default:
            break;
        }
        return 0;
    }

    uint8_t digitalRead(uint8_t pin)
    {
        switch (pin) {
        case PMU_GPIO0:
            return getRegisterBit(XPOWERS_AXP192_GPIO012_SIGNAL, 4);
        case PMU_GPIO1:
            return getRegisterBit(XPOWERS_AXP192_GPIO012_SIGNAL, 5);
        case PMU_GPIO2:
            return getRegisterBit(XPOWERS_AXP192_GPIO012_SIGNAL, 6);
        case PMU_GPIO3:
            return getRegisterBit(XPOWERS_AXP192_GPIO34_SIGNAL, 4);
        case PMU_GPIO4:
            return getRegisterBit(XPOWERS_AXP192_GPIO34_SIGNAL, 5);
        case PMU_GPIO5:
            return getRegisterBit(XPOWERS_AXP192_GPIO5_CTL, 4);
        default:
            break;
        }
        return 0;
    }

    void digitalWrite(uint8_t pin, uint8_t val)
    {
        int reg = 0;
        switch (pin) {
        case PMU_GPIO0:
            if (gpio[pin].mode != OUTPUT) {
                gpio[pin].mode = OUTPUT;
            }
            reg = readRegister(XPOWERS_AXP192_GPIO0_CTL) & 0xFE;
            writeRegister(XPOWERS_AXP192_GPIO0_CTL,  val ? (reg | 0x01) : reg);
            break;
        case PMU_GPIO1:
            if (gpio[pin].mode != OUTPUT) {
                gpio[pin].mode = OUTPUT;
            }
            reg = readRegister(XPOWERS_AXP192_GPIO1_CTL) & 0xFD;
            writeRegister(XPOWERS_AXP192_GPIO1_CTL,  val ? (reg | 0x01) : reg);
            break;
        case PMU_GPIO2:
            if (gpio[pin].mode != OUTPUT) {
                gpio[pin].mode = OUTPUT;
            }
            reg = readRegister(XPOWERS_AXP192_GPIO2_CTL) & 0xFB;
            writeRegister(XPOWERS_AXP192_GPIO2_CTL,  val ? (reg | 0x01) : reg);
            break;
        case PMU_GPIO3:
            if (gpio[pin].mode != OUTPUT) {
                gpio[pin].mode = OUTPUT;
                reg = readRegister(XPOWERS_AXP192_GPIO34_CTL) & 0xFC;
                writeRegister(XPOWERS_AXP192_GPIO34_CTL,   reg | 0x01);
            }
            reg = readRegister(XPOWERS_AXP192_GPIO34_SIGNAL) & 0xF7;
            writeRegister(XPOWERS_AXP192_GPIO34_SIGNAL,   val ? (val | 0x08) : reg);
            break;
        case PMU_GPIO4:
            if (gpio[pin].mode != OUTPUT) {
                gpio[pin].mode = OUTPUT;
                reg = readRegister(XPOWERS_AXP192_GPIO34_CTL) & 0xF3;
                writeRegister(XPOWERS_AXP192_GPIO34_CTL,  reg | 0x04);
            }
            reg = readRegister(XPOWERS_AXP192_GPIO34_SIGNAL) & 0xEF;
            writeRegister(XPOWERS_AXP192_GPIO34_SIGNAL,   val ? (val | 0x10) : reg);
            break;
        case PMU_GPIO5:
            if (gpio[pin].mode != OUTPUT) {
                gpio[pin].mode = OUTPUT;
                reg = readRegister(XPOWERS_AXP192_GPIO5_CTL) & 0xBF;
                writeRegister(XPOWERS_AXP192_GPIO5_CTL,  reg);
            }
            reg = readRegister(XPOWERS_AXP192_GPIO5_CTL) & 0xDF;
            writeRegister(XPOWERS_AXP192_GPIO5_CTL, val ? (reg | 0x20) : reg);
            break;
        default:
            break;
        }
    }

#if 0
    // ! Need FIX
    uint16_t analogRead(uint8_t pin)
    {
        switch (pin) {
        case PMU_GPIO0:
            if (gpio[pin].mode != ANALOG) {
                // Enable GPIO ADC Function
                uint8_t val = readRegister(XPOWERS_AXP192_GPIO0_CTL) & 0xF8;
                writeRegister(XPOWERS_AXP192_GPIO0_CTL, val | 0x04);

                //Enable ADC2 / GPIO0
                // val = readRegister(XPOWERS_AXP192_ADC_EN2) | 0x08;
                // writeRegister(XPOWERS_AXP192_ADC_EN2, val );
                setRegisterBit(XPOWERS_AXP192_ADC_EN2, 3);

                // Set adc input range 0~2.0475V
                val = readRegister(XPOWERS_AXP192_ADC_INPUTRANGE) & 0xFE;
                writeRegister(XPOWERS_AXP192_ADC_INPUTRANGE, val);
                gpio[pin].mode = ANALOG;
            }
            return readRegisterH8L4(XPOWERS_AXP192_GPIO0_VOL_ADC_H8, XPOWERS_AXP192_GPIO0_VOL_ADC_L4);

        case PMU_GPIO1:
            if (gpio[pin].mode != ANALOG) {
                // Enable GPIO ADC Function
                uint8_t val = readRegister(XPOWERS_AXP192_GPIO1_CTL) & 0xF8;
                writeRegister(XPOWERS_AXP192_GPIO1_CTL, val | 0x04);

                //Enable ADC2 / GPIO1
                // val = readRegister(XPOWERS_AXP192_ADC_EN2) | 0x04;
                // writeRegister(XPOWERS_AXP192_ADC_EN2, val );
                setRegisterBit(XPOWERS_AXP192_ADC_EN2, 2);


                // Set adc input range 0~2.0475V
                val = readRegister(XPOWERS_AXP192_ADC_INPUTRANGE) & 0xFD;
                writeRegister(XPOWERS_AXP192_ADC_INPUTRANGE, val);
                gpio[pin].mode = ANALOG;
            }
            return readRegisterH8L4(XPOWERS_AXP192_GPIO1_VOL_ADC_H8, XPOWERS_AXP192_GPIO1_VOL_ADC_L4);

        case PMU_GPIO2:
            if (gpio[pin].mode != ANALOG) {
                // Enable GPIO ADC Function
                uint8_t val = readRegister(XPOWERS_AXP192_GPIO1_CTL) & 0xF8;
                writeRegister(XPOWERS_AXP192_GPIO1_CTL, val | 0x04);
                //Enable ADC2 / GPIO1
                // val = readRegister(XPOWERS_AXP192_ADC_EN2) | 0x02;
                // writeRegister(XPOWERS_AXP192_ADC_EN2, val );
                setRegisterBit(XPOWERS_AXP192_ADC_EN2, 1);

                // Set adc input range 0~2.0475V
                val = readRegister(XPOWERS_AXP192_ADC_INPUTRANGE) & 0xFB;
                writeRegister(XPOWERS_AXP192_ADC_INPUTRANGE, val);
                gpio[pin].mode = ANALOG;
            }
            return readRegisterH8L4(XPOWERS_AXP192_GPIO2_VOL_ADC_H8, XPOWERS_AXP192_GPIO2_VOL_ADC_L4);


        case PMU_GPIO3:
            if (gpio[pin].mode != ANALOG) {
                // Enable GPIO ADC Function
                uint8_t val = readRegister(XPOWERS_AXP192_GPIO1_CTL) & 0xF8;
                writeRegister(XPOWERS_AXP192_GPIO1_CTL, val | 0x04);

                //Enable ADC2 / GPIO1
                // val = readRegister(XPOWERS_AXP192_ADC_EN2) | 0X01;
                // writeRegister(XPOWERS_AXP192_ADC_EN2, val );
                setRegisterBit(XPOWERS_AXP192_ADC_EN2, 0);

                // Set adc input range 0~2.0475V
                val = readRegister(XPOWERS_AXP192_ADC_INPUTRANGE) & 0xF7;
                writeRegister(XPOWERS_AXP192_ADC_INPUTRANGE, val);
                gpio[pin].mode = ANALOG;
            }
            return readRegisterH8L4(XPOWERS_AXP192_GPIO3_VOL_ADC_H8, XPOWERS_AXP192_GPIO3_VOL_ADC_L4);

        case PMU_TS_PIN:
            if (gpio[pin].mode != ANALOG) {
                // Enable TS PIN ADC Function
                setRegisterBit(XPOWERS_AXP192_ADC_SPEED, 2);
                // uint8_t val = readRegister(XPOWERS_AXP192_ADC_SPEED) & 0xFB;
                // writeRegister(XPOWERS_AXP192_ADC_SPEED, val | 0x04);
                gpio[pin].mode = ANALOG;
            }
            return readRegisterH8L4(XPOWERS_AXP192_TS_IN_H8, XPOWERS_AXP192_TS_IN_L4);
            break;
        default:
            break;
        }
        return 0;
    }
#endif


    /**
     * Sleep function
     */
    bool enableSleep()
    {
        return setRegisterBit(XPOWERS_AXP192_VOFF_SET, 3);
    }

    /*
     * Pekey function
     */

    /**
    * @brief Set the PEKEY power-on long press time.
    * @param opt: See xpowers_press_on_time_t enum for details.
    * @retval
    */
    bool setPowerKeyPressOnTime(uint8_t opt)
    {
        int val =  readRegister(XPOWERS_AXP192_POK_SET);
        if (val == -1)return false;
        return 0 == writeRegister(XPOWERS_AXP192_POK_SET, (val & 0x3F) | (opt << 6));
    }

    /**
    * @brief Get the PEKEY power-on long press time.
    * @retval See xpowers_press_on_time_t enum for details.
    */
    uint8_t getPowerKeyPressOnTime()
    {
        int val =  readRegister(XPOWERS_AXP192_POK_SET);
        if (val == -1)return 0;
        return (val & 0xC0) >> 6;
    }

    /**
    * @brief Set the PEKEY power-off long press time.
    * @param opt: See xpowers_press_off_time_t enum for details.
    * @retval
    */
    bool setPowerKeyPressOffTime(uint8_t opt)
    {
        int val =  readRegister(XPOWERS_AXP192_POK_SET);
        if (val == -1)return false;
        return 0 == writeRegister(XPOWERS_AXP192_POK_SET, (val & 0xFC)  | opt);
    }

    /**
    * @brief Get the PEKEY power-off long press time.
    * @retval See xpowers_press_off_time_t enum for details.
    */
    uint8_t getPowerKeyPressOffTime()
    {
        int val =  readRegister(XPOWERS_AXP192_POK_SET);
        if (val == -1)return 0;
        return (val & 0x03);
    }


    void setPowerKeyLongPressOnTime(xpowers_axp192_pekey_long_press_t opt)
    {
        int val =  readRegister(XPOWERS_AXP192_POK_SET);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP192_POK_SET, (val & 0xCF) | (opt << 4));
    }

    void enablePowerKeyLongPressPowerOff()
    {
        setRegisterBit(XPOWERS_AXP192_POK_SET, 3);
    }

    void disablePowerKeyLongPressPowerOff()
    {
        clrRegisterBit(XPOWERS_AXP192_POK_SET, 3);
    }


protected:

    uint16_t getPowerChannelVoltage(uint8_t channel)
    {
        switch (channel) {
        case XPOWERS_DCDC1:
            return getDC1Voltage();
        case XPOWERS_DCDC2:
            return getDC2Voltage();
        case XPOWERS_DCDC3:
            return getDC3Voltage();
        case XPOWERS_LDO2:
            return getLDO2Voltage();
        case XPOWERS_LDO3:
            return getLDO3Voltage();
        case XPOWERS_LDOIO:
            return getLDOioVoltage();
        default:
            break;
        }
        return 0;
    }

    bool inline enablePowerOutput(uint8_t channel)
    {
        /*
        return 0 == writeRegister(XPOWERS_AXP192_LDO23_DC123_EXT_CTL,
                                  val | readRegister(XPOWERS_AXP192_LDO23_DC123_EXT_CTL));
        */
        switch (channel) {
        case XPOWERS_DCDC1:
            return enableDC1();
        case XPOWERS_DCDC2:
            return enableDC2();
        case XPOWERS_DCDC3:
            return enableDC3();
        case XPOWERS_LDO2:
            return enableLDO2();
        case XPOWERS_LDO3:
            return enableLDO3();
        case XPOWERS_LDOIO:
            return enableLDOio();
        case XPOWERS_VBACKUP:
            return enableBackupBattCharger();
        default:
            break;
        }
        return false;
    }

    bool inline disablePowerOutput(uint8_t channel)
    {
        /*
        return 0 == writeRegister(XPOWERS_AXP192_LDO23_DC123_EXT_CTL,
                                  val & readRegister(XPOWERS_AXP192_LDO23_DC123_EXT_CTL));
        */
        if (getProtectedChannel(channel)) {
            log_e("Failed to disable the power channel, the power channel has been protected");
            return false;
        }
        switch (channel) {
        case XPOWERS_DCDC1:
            return disableDC1();
        case XPOWERS_DCDC2:
            return disableDC2();
        case XPOWERS_DCDC3:
            return disableDC3();
        case XPOWERS_LDO2:
            return disableLDO2();
        case XPOWERS_LDO3:
            return disableLDO3();
        case XPOWERS_LDOIO:
            return disableLDOio();
        case XPOWERS_VBACKUP:
            return disableBackupBattCharger();
        default:
            break;
        }
        return false;
    }

    bool inline isPowerChannelEnable(uint8_t channel)
    {
        switch (channel) {
        case XPOWERS_DCDC1:
            return isEnableDC1();
        case XPOWERS_DCDC2:
            return isEnableDC2();
        case XPOWERS_DCDC3:
            return isEnableDC3();
        case XPOWERS_LDO2:
            return isEnableLDO2();
        case XPOWERS_LDO3:
            return isEnableLDO3();
        case XPOWERS_LDOIO:
            return isEnableLDOio();
        case XPOWERS_VBACKUP:
            return isEnableBackupCharger();
        default:
            break;
        }
        return false;
    }


    bool inline setPowerChannelVoltage(uint8_t channel, uint16_t millivolt)
    {
        if (getProtectedChannel(channel)) {
            log_e("Failed to set the power channel, the power channel has been protected");
            return false;
        }
        switch (channel) {
        case XPOWERS_DCDC1:
            return setDC1Voltage(millivolt);
        case XPOWERS_DCDC2:
            return setDC2Voltage(millivolt);
        case XPOWERS_DCDC3:
            return setDC3Voltage(millivolt);
        case XPOWERS_LDO2:
            return setLDO2Voltage(millivolt);
        case XPOWERS_LDO3:
            return setLDO3Voltage(millivolt);
        case XPOWERS_LDOIO:
            return setLDOioVoltage(millivolt);
        case XPOWERS_VBACKUP:
        //TODO:
        // return setBackupBattChargerVoltage(millivolt);
        default:
            break;
        }
        return false;
    }

    bool inline getPowerEnable(uint8_t val)
    {
        return (bool)(readRegister(XPOWERS_AXP192_LDO23_DC123_EXT_CTL) & val);
    }

    bool initImpl()
    {
        if (getChipID() == XPOWERS_AXP192_CHIP_ID) {
            setChipModel(XPOWERS_AXP192);
            return true;
        }
        return false;
    }


    /*
     * Interrupt control functions
     */
    bool setInterruptImpl(uint64_t opts, bool enable)
    {
        int res = 0;
        int data = 0, value = 0;

        log_d("%s %s - 0x%llx\n", __func__, enable ? "ENABLE" : "DISABLE", opts);

        if (opts & 0xFF) {
            value = opts & 0xFF;
            data = readRegister(XPOWERS_AXP192_INTEN1);
            res |= writeRegister(XPOWERS_AXP192_INTEN1, enable ? (data | value) : (data & (~value)));
        }

        if (opts & 0xFF00) {
            value = opts >> 8;
            data = readRegister(XPOWERS_AXP192_INTEN2);
            res |= writeRegister(XPOWERS_AXP192_INTEN2, enable ? (data | value) : (data & (~value)));
        }

        if (opts & 0xFF0000) {
            value = opts >> 16;
            data = readRegister(XPOWERS_AXP192_INTEN3);
            res |= writeRegister(XPOWERS_AXP192_INTEN3, enable ? (data | value) : (data & (~value)));
        }

        if (opts & 0xFF000000) {
            value = opts >> 24;
            data = readRegister(XPOWERS_AXP192_INTEN4);
            res |= writeRegister(XPOWERS_AXP192_INTEN4, enable ? (data | value) : (data & (~value)));
        }

        if (opts & 0xFF00000000) {
            value = opts >> 32;
            data = readRegister(XPOWERS_AXP192_INTEN5);
            res |= writeRegister(XPOWERS_AXP192_INTEN5, enable ? (data | value) : (data & (~value)));
        }
        return res == 0;
    }

    /*
     * Signal Capture control functions
     */
    bool setSignalCaptureImpl(uint32_t opts, bool enable)
    {
        int value = 0;
        if (opts & 0xFF) {
            value = readRegister(XPOWERS_AXP192_ADC_EN1);
            writeRegister(XPOWERS_AXP192_ADC_EN1, enable ? (value | opts) : (value & (~opts)));
        }
        if (opts & 0xFF00) {
            opts >>= 8;
            value = readRegister(XPOWERS_AXP192_ADC_EN2);
            writeRegister(XPOWERS_AXP192_ADC_EN2, enable ? (value | opts) : (value & (~opts)));
        }
        return true;
    }

    const char  *getChipNameImpl(void)
    {
        return "AXP192";
    }


private:
    const uint16_t chargeTargetVol[4] = {4100, 4150, 4200, 4360};
    uint8_t statusRegister[5];
    xpowers_gpio_t gpio[6];
};



