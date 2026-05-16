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
 * @file      XPowersAXP2101.tpp
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2022-05-07
 *
 */

#if defined(ARDUINO)
#include <Arduino.h>
#endif /*ARDUINO*/

#include "XPowersCommon.tpp"
#include "REG/AXP2101Constants.h"
#include "XPowersLibInterface.hpp"

typedef enum {
    XPOWERS_AXP2101_IRQ_TIME_1S,
    XPOWERS_AXP2101_IRQ_TIME_1S5,
    XPOWERS_AXP2101_IRQ_TIME_2S,
    XPOWERS_AXP2101_PRESSOFF_2S5,
} xpowers_irq_time_t;



typedef enum {
    XPOWERS_AXP2101_PRECHARGE_0MA,
    XPOWERS_AXP2101_PRECHARGE_25MA,
    XPOWERS_AXP2101_PRECHARGE_50MA,
    XPOWERS_AXP2101_PRECHARGE_75MA,
    XPOWERS_AXP2101_PRECHARGE_100MA,
    XPOWERS_AXP2101_PRECHARGE_125MA,
    XPOWERS_AXP2101_PRECHARGE_150MA,
    XPOWERS_AXP2101_PRECHARGE_175MA,
    XPOWERS_AXP2101_PRECHARGE_200MA,
} xpowers_prechg_t;

typedef enum {
    XPOWERS_AXP2101_CHG_ITERM_0MA,
    XPOWERS_AXP2101_CHG_ITERM_25MA,
    XPOWERS_AXP2101_CHG_ITERM_50MA,
    XPOWERS_AXP2101_CHG_ITERM_75MA,
    XPOWERS_AXP2101_CHG_ITERM_100MA,
    XPOWERS_AXP2101_CHG_ITERM_125MA,
    XPOWERS_AXP2101_CHG_ITERM_150MA,
    XPOWERS_AXP2101_CHG_ITERM_175MA,
    XPOWERS_AXP2101_CHG_ITERM_200MA,
} xpowers_axp2101_chg_iterm_t;


typedef enum {
    XPOWERS_AXP2101_THREMAL_60DEG,
    XPOWERS_AXP2101_THREMAL_80DEG,
    XPOWERS_AXP2101_THREMAL_100DEG,
    XPOWERS_AXP2101_THREMAL_120DEG,
} xpowers_thermal_t;

typedef enum {
    XPOWERS_AXP2101_CHG_TRI_STATE,   //tri_charge
    XPOWERS_AXP2101_CHG_PRE_STATE,   //pre_charge
    XPOWERS_AXP2101_CHG_CC_STATE,    //constant charge
    XPOWERS_AXP2101_CHG_CV_STATE,    //constant voltage
    XPOWERS_AXP2101_CHG_DONE_STATE,  //charge done
    XPOWERS_AXP2101_CHG_STOP_STATE,  //not charge
} xpowers_chg_status_t;

typedef enum {
    XPOWERS_AXP2101_WAKEUP_IRQ_PIN_TO_LOW = _BV(4),
    XPOWERS_AXP2101_WAKEUP_PWROK_TO_LOW   = _BV(3),
    XPOWERS_AXP2101_WAKEUP_DC_DLO_SELECT  = _BV(2),
} xpowers_wakeup_t;

typedef enum {
    XPOWERS_AXP2101_FAST_DCDC1,
    XPOWERS_AXP2101_FAST_DCDC2,
    XPOWERS_AXP2101_FAST_DCDC3,
    XPOWERS_AXP2101_FAST_DCDC4,
    XPOWERS_AXP2101_FAST_DCDC5,
    XPOWERS_AXP2101_FAST_ALDO1,
    XPOWERS_AXP2101_FAST_ALDO2,
    XPOWERS_AXP2101_FAST_ALDO3,
    XPOWERS_AXP2101_FAST_ALDO4,
    XPOWERS_AXP2101_FAST_BLDO1,
    XPOWERS_AXP2101_FAST_BLDO2,
    XPOWERS_AXP2101_FAST_CPUSLDO,
    XPOWERS_AXP2101_FAST_DLDO1,
    XPOWERS_AXP2101_FAST_DLDO2,
} xpowers_fast_on_opt_t;


typedef enum {
    XPOWERS_AXP2101_SEQUENCE_LEVEL_0,
    XPOWERS_AXP2101_SEQUENCE_LEVEL_1,
    XPOWERS_AXP2101_SEQUENCE_LEVEL_2,
    XPOWERS_AXP2101_SEQUENCE_DISABLE,
} xpower_start_sequence_t;

typedef enum {
    XPOWERS_AXP2101_WDT_IRQ_TO_PIN,             //Just interrupt to pin
    XPOWERS_AXP2101_WDT_IRQ_AND_RSET,           //IRQ to pin and reset pmu system
    XPOWERS_AXP2101_WDT_IRQ_AND_RSET_PD_PWROK,  //IRQ to pin and reset pmu system,pull down pwrok
    XPOWERS_AXP2101_WDT_IRQ_AND_RSET_ALL_OFF,   //IRQ to pin and reset pmu system,turn off dcdc & ldo ,pull down pwrok
} xpowers_wdt_config_t;

typedef enum {
    XPOWERS_AXP2101_WDT_TIMEOUT_1S,
    XPOWERS_AXP2101_WDT_TIMEOUT_2S,
    XPOWERS_AXP2101_WDT_TIMEOUT_4S,
    XPOWERS_AXP2101_WDT_TIMEOUT_8S,
    XPOWERS_AXP2101_WDT_TIMEOUT_16S,
    XPOWERS_AXP2101_WDT_TIMEOUT_32S,
    XPOWERS_AXP2101_WDT_TIMEOUT_64S,
    XPOWERS_AXP2101_WDT_TIMEOUT_128S,
} xpowers_wdt_timeout_t;

typedef enum {
    XPOWERS_AXP2101_VSYS_VOL_4V1,
    XPOWERS_AXP2101_VSYS_VOL_4V2,
    XPOWERS_AXP2101_VSYS_VOL_4V3,
    XPOWERS_AXP2101_VSYS_VOL_4V4,
    XPOWERS_AXP2101_VSYS_VOL_4V5,
    XPOWERS_AXP2101_VSYS_VOL_4V6,
    XPOWERS_AXP2101_VSYS_VOL_4V7,
    XPOWERS_AXP2101_VSYS_VOL_4V8,
} xpower_chg_dpm_t;

typedef enum {
    XPOWER_POWERON_SRC_POWERON_LOW,                     //POWERON low for on level when POWERON Mode as POWERON Source
    XPOWER_POWERON_SRC_IRQ_LOW,                         //IRQ PIN Pull-down as POWERON Source
    XPOWER_POWERON_SRC_VBUS_INSERT,                     //Vbus Insert and Good as POWERON Source
    XPOWER_POWERON_SRC_BAT_CHARGE,                      //Vbus Insert and Good as POWERON Source
    XPOWER_POWERON_SRC_BAT_INSERT,                      //Battery Insert and Good as POWERON Source
    XPOWER_POWERON_SRC_ENMODE,                          //POWERON always high when EN Mode as POWERON Source
    XPOWER_POWERON_SRC_UNKONW,                          //Unkonw
} xpower_power_on_source_t;

typedef enum {
    XPOWER_POWEROFF_SRC_PWEKEY_PULLDOWN,            //POWERON Pull down for off level when POWERON Mode as POWEROFF Source
    XPOWER_POWEROFF_SRC_SOFT_OFF,                   //Software configuration as POWEROFF Source
    XPOWER_POWEROFF_SRC_PWEKEY_LOW,                 //POWERON always low when EN Mode as POWEROFF Source
    XPOWER_POWEROFF_SRC_UNDER_VSYS,                 //Vsys Under Voltage as POWEROFF Source
    XPOWER_POWEROFF_SRC_OVER_VBUS,                  //VBUS Over Voltage as POWEROFF Source
    XPOWER_POWEROFF_SRC_UNDER_VOL,                  //DCDC Under Voltage as POWEROFF Source
    XPOWER_POWEROFF_SRC_OVER_VOL,                   //DCDC Over Voltage as POWEROFF Source
    XPOWER_POWEROFF_SRC_OVER_TEMP,                  //Die Over Temperature as POWEROFF Source
    XPOWER_POWEROFF_SRC_UNKONW,                     //Unkonw
} xpower_power_off_source_t;

typedef enum {
    XPOWER_PWROK_DELAY_8MS,
    XPOWER_PWROK_DELAY_16MS,
    XPOWER_PWROK_DELAY_32MS,
    XPOWER_PWROK_DELAY_64MS,
} xpower_pwrok_delay_t;

class XPowersAXP2101 :
    public XPowersCommon<XPowersAXP2101>, public XPowersLibInterface
{
    friend class XPowersCommon<XPowersAXP2101>;

public:


#if defined(ARDUINO)
    XPowersAXP2101(TwoWire &w, int sda = SDA, int scl = SCL, uint8_t addr = AXP2101_SLAVE_ADDRESS)
    {
        __wire = &w;
        __sda = sda;
        __scl = scl;
        __addr = addr;
    }

#endif

    XPowersAXP2101(uint8_t addr, iic_fptr_t readRegCallback, iic_fptr_t writeRegCallback)
    {
        thisReadRegCallback = readRegCallback;
        thisWriteRegCallback = writeRegCallback;
        __addr = addr;
    }

    XPowersAXP2101()
    {
#if defined(ARDUINO)
        __wire = &Wire;
        __sda = SDA;
        __scl = SCL;
#endif
        __addr = AXP2101_SLAVE_ADDRESS;
    }

    ~XPowersAXP2101()
    {
        log_i("~XPowersAXP2101");
        deinit();
    }

#if defined(ARDUINO)
    bool init(TwoWire &w, int sda = SDA, int scl = SCL, uint8_t addr = AXP2101_SLAVE_ADDRESS)
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

    /*
     * PMU status functions
     */
    uint16_t status()
    {
        uint16_t status1 = readRegister(XPOWERS_AXP2101_STATUS1) & 0x1F;
        uint16_t status2 = readRegister(XPOWERS_AXP2101_STATUS2) & 0x1F;;
        return (status1 << 8) | (status2);
    }

    bool isVbusGood(void)
    {
        return  getRegisterBit(XPOWERS_AXP2101_STATUS1, 5);
    }

    bool getBatfetState(void)
    {
        return  getRegisterBit(XPOWERS_AXP2101_STATUS1, 4);
    }

    // getBatPresentState
    bool isBatteryConnect(void)
    {
        return  getRegisterBit(XPOWERS_AXP2101_STATUS1, 3);
    }

    bool isBatInActiveModeState(void)
    {
        return  getRegisterBit(XPOWERS_AXP2101_STATUS1, 2);
    }

    bool getThermalRegulationStatus(void)
    {
        return  getRegisterBit(XPOWERS_AXP2101_STATUS1, 1);
    }

    bool getCurrentLimitStatus(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_STATUS1, 0);
    }

    bool isCharging(void)
    {
        return (readRegister(XPOWERS_AXP2101_STATUS2) >> 5) == 0x01;
    }

    bool isDischarge(void)
    {
        return (readRegister(XPOWERS_AXP2101_STATUS2) >> 5) == 0x02;
    }

    bool isStandby(void)
    {
        return (readRegister(XPOWERS_AXP2101_STATUS2) >> 5) == 0x00;
    }

    bool isPowerOn(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_STATUS2, 4);
    }

    bool isPowerOff(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_STATUS2, 4);
    }

    bool isVbusIn(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_STATUS2, 3) == 0 && isVbusGood();
    }

    xpowers_chg_status_t getChargerStatus(void)
    {
        int val = readRegister(XPOWERS_AXP2101_STATUS2);
        if (val == -1)return XPOWERS_AXP2101_CHG_STOP_STATE;
        val &= 0x07;
        return (xpowers_chg_status_t)val;
    }

    /*
     * Data Buffer
     */

    bool writeDataBuffer(uint8_t *data, uint8_t size)
    {
        if (size > XPOWERS_AXP2101_DATA_BUFFER_SIZE)return false;
        return writeRegister(XPOWERS_AXP2101_DATA_BUFFER1, data, size);
    }

    bool readDataBuffer(uint8_t *data, uint8_t size)
    {
        if (size > XPOWERS_AXP2101_DATA_BUFFER_SIZE)return false;
        return readRegister(XPOWERS_AXP2101_DATA_BUFFER1, data, size);
    }

    /*
     * PMU common configuration
     */

    /**
     * @brief   Internal off-discharge enable for DCDC & LDO & SWITCH
     */

    void enableInternalDischarge(void)
    {
        setRegisterBit(XPOWERS_AXP2101_COMMON_CONFIG, 5);
    }

    void disableInternalDischarge(void)
    {
        clrRegisterBit(XPOWERS_AXP2101_COMMON_CONFIG, 5);
    }


    /**
     * @brief   PWROK PIN pull low to Restart
     */
    void enablePwrOkPinPullLow(void)
    {
        setRegisterBit(XPOWERS_AXP2101_COMMON_CONFIG, 3);
    }

    void disablePwrOkPinPullLow(void)
    {
        clrRegisterBit(XPOWERS_AXP2101_COMMON_CONFIG, 3);
    }

    void enablePwronShutPMIC(void)
    {
        setRegisterBit(XPOWERS_AXP2101_COMMON_CONFIG, 2);
    }

    void disablePwronShutPMIC(void)
    {
        clrRegisterBit(XPOWERS_AXP2101_COMMON_CONFIG, 2);
    }


    /**
     * @brief  Restart the SoC System, POWOFF/POWON and reset the related registers
     * @retval None
     */
    void reset(void)
    {
        setRegisterBit(XPOWERS_AXP2101_COMMON_CONFIG, 1);
    }

    /**
     * @brief  Set shutdown, calling shutdown will turn off all power channels,
     *         only VRTC belongs to normal power supply
     * @retval None
     */
    void shutdown(void)
    {
        setRegisterBit(XPOWERS_AXP2101_COMMON_CONFIG, 0);
    }

    /**
     * @brief  BATFET control / REG 12H
     * @note   DIE Over Temperature Protection Level1 Configuration
     * @param  opt: 0:115 , 1:125 , 2:135
     * @retval None
     */
    void setBatfetDieOverTempLevel1(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_BATFET_CTRL);
        if (val == -1)return;
        val &= 0xF9;
        writeRegister(XPOWERS_AXP2101_BATFET_CTRL, val | (opt << 1));
    }

    uint8_t getBatfetDieOverTempLevel1(void)
    {
        return (readRegister(XPOWERS_AXP2101_BATFET_CTRL) & 0x06);
    }

    void enableBatfetDieOverTempDetect(void)
    {
        setRegisterBit(XPOWERS_AXP2101_BATFET_CTRL, 0);
    }

    void disableBatfetDieOverTempDetect(void)
    {
        clrRegisterBit(XPOWERS_AXP2101_BATFET_CTRL, 0);
    }

    /**
     * @param  opt: 0:115 , 1:125 , 2:135
     */
    void setDieOverTempLevel1(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_DIE_TEMP_CTRL);
        if (val == -1)return;
        val &= 0xF9;
        writeRegister(XPOWERS_AXP2101_DIE_TEMP_CTRL, val | (opt << 1));
    }

    uint8_t getDieOverTempLevel1(void)
    {
        return (readRegister(XPOWERS_AXP2101_DIE_TEMP_CTRL) & 0x06);
    }

    void enableDieOverTempDetect(void)
    {
        setRegisterBit(XPOWERS_AXP2101_DIE_TEMP_CTRL, 0);
    }

    void disableDieOverTempDetect(void)
    {
        clrRegisterBit(XPOWERS_AXP2101_DIE_TEMP_CTRL, 0);
    }

    // Linear Charger Vsys voltage dpm
    void setLinearChargerVsysDpm(xpower_chg_dpm_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_MIN_SYS_VOL_CTRL);
        if (val == -1)return;
        val &= 0x8F;
        writeRegister(XPOWERS_AXP2101_MIN_SYS_VOL_CTRL, val | (opt << 4));
    }

    uint8_t getLinearChargerVsysDpm(void)
    {
        int val = readRegister(XPOWERS_AXP2101_MIN_SYS_VOL_CTRL);
        if (val == -1)return 0;
        val &= 0x70;
        return (val & 0x70) >> 4;
    }


    /**
     * @brief  Set VBUS Voltage Input Limit.
     * @param  opt: View the related chip type xpowers_axp2101_vbus_vol_limit_t enumeration
     *              parameters in "XPowersParams.hpp"
     */
    void setVbusVoltageLimit(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_INPUT_VOL_LIMIT_CTRL);
        if (val == -1)return;
        val &= 0xF0;
        writeRegister(XPOWERS_AXP2101_INPUT_VOL_LIMIT_CTRL, val | (opt & 0x0F));
    }

    /**
    * @brief  Get VBUS Voltage Input Limit.
    * @retval View the related chip type xpowers_axp2101_vbus_vol_limit_t enumeration
    *              parameters in "XPowersParams.hpp"
    */
    uint8_t getVbusVoltageLimit(void)
    {
        return (readRegister(XPOWERS_AXP2101_INPUT_VOL_LIMIT_CTRL) & 0x0F);
    }

    /**
    * @brief  Set VBUS Current Input Limit.
    * @param  opt: View the related chip type xpowers_axp2101_vbus_cur_limit_t enumeration
    *              parameters in "XPowersParams.hpp"
    * @retval true valid false invalid
    */
    bool setVbusCurrentLimit(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_INPUT_CUR_LIMIT_CTRL);
        if (val == -1)return false;
        val &= 0xF8;
        return 0 == writeRegister(XPOWERS_AXP2101_INPUT_CUR_LIMIT_CTRL, val | (opt & 0x07));
    }

    /**
    * @brief  Get VBUS Current Input Limit.
    * @retval View the related chip type xpowers_axp2101_vbus_cur_limit_t enumeration
    *              parameters in "XPowersParams.hpp"
    */
    uint8_t getVbusCurrentLimit(void)
    {
        return (readRegister(XPOWERS_AXP2101_INPUT_CUR_LIMIT_CTRL) & 0x07);
    }

    /**
     * @brief  Reset the fuel gauge
     */
    void resetGauge(void)
    {
        setRegisterBit(XPOWERS_AXP2101_RESET_FUEL_GAUGE, 3);
    }

    /**
     * @brief   reset the gauge besides reset
     */
    void resetGaugeBesides(void)
    {
        setRegisterBit(XPOWERS_AXP2101_RESET_FUEL_GAUGE, 2);
    }

    /**
     * @brief Gauge Module
     */
    void enableGauge(void)
    {
        setRegisterBit(XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL, 3);
    }

    void disableGauge(void)
    {
        clrRegisterBit(XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL, 3);
    }

    /**
     * @brief  Button Battery charge
     */
    bool enableButtonBatteryCharge(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL, 2);
    }

    bool disableButtonBatteryCharge(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL, 2);
    }

    bool isEnableButtonBatteryCharge()
    {
        return getRegisterBit(XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL, 2);
    }


    //Button battery charge termination voltage setting
    bool setButtonBatteryChargeVoltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_BTN_VOL_STEPS) {
            log_e("Mistake ! Button battery charging step voltage is %u mV", XPOWERS_AXP2101_BTN_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_BTN_VOL_MIN) {
            log_e("Mistake ! The minimum charge termination voltage of the coin cell battery is %u mV", XPOWERS_AXP2101_BTN_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_BTN_VOL_MAX) {
            log_e("Mistake ! The minimum charge termination voltage of the coin cell battery is %u mV", XPOWERS_AXP2101_BTN_VOL_MAX);
            return false;
        }
        int val =  readRegister(XPOWERS_AXP2101_BTN_BAT_CHG_VOL_SET);
        if (val == -1)return 0;
        val  &= 0xF8;
        val |= (millivolt - XPOWERS_AXP2101_BTN_VOL_MIN) / XPOWERS_AXP2101_BTN_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_BTN_BAT_CHG_VOL_SET, val);
    }

    uint16_t getButtonBatteryVoltage(void)
    {
        int val =  readRegister(XPOWERS_AXP2101_BTN_BAT_CHG_VOL_SET);
        if (val == -1)return 0;
        return (val & 0x07) * XPOWERS_AXP2101_BTN_VOL_STEPS + XPOWERS_AXP2101_BTN_VOL_MIN;
    }


    /**
     * @brief Cell Battery charge
     */
    void enableCellbatteryCharge(void)
    {
        setRegisterBit(XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL, 1);
    }

    void disableCellbatteryCharge(void)
    {
        clrRegisterBit(XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL, 1);
    }

    /**
     * @brief  Watchdog Module
     */
    void enableWatchdog(void)
    {
        setRegisterBit(XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL, 0);
        enableIRQ(XPOWERS_AXP2101_WDT_EXPIRE_IRQ);
    }

    void disableWatchdog(void)
    {
        disableIRQ(XPOWERS_AXP2101_WDT_EXPIRE_IRQ);
        clrRegisterBit(XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL, 0);
    }

    /**
     * @brief Watchdog Config
     * @note
     * @param  opt: 0: IRQ Only 1: IRQ and System reset  2: IRQ, System Reset and Pull down PWROK 1s  3: IRQ, System Reset, DCDC/LDO PWROFF & PWRON
     * @retval None
     */
    void setWatchdogConfig(xpowers_wdt_config_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_WDT_CTRL);
        if (val == -1)return;
        val &= 0xCF;
        writeRegister(XPOWERS_AXP2101_WDT_CTRL, val | (opt << 4));
    }

    uint8_t getWatchConfig(void)
    {
        return (readRegister(XPOWERS_AXP2101_WDT_CTRL) & 0x30) >> 4;
    }

    void clrWatchdog(void)
    {
        setRegisterBit(XPOWERS_AXP2101_WDT_CTRL, 3);
    }


    void setWatchdogTimeout(xpowers_wdt_timeout_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_WDT_CTRL);
        if (val == -1)return;
        val &= 0xF8;
        writeRegister(XPOWERS_AXP2101_WDT_CTRL, val | opt);
    }

    uint8_t getWatchdogTimerout(void)
    {
        return readRegister(XPOWERS_AXP2101_WDT_CTRL) & 0x07;
    }

    /**
     * @brief  Low battery warning threshold 5-20%, 1% per step
     * @param  percentage:   5 ~ 20
     * @retval None
     */
    void setLowBatWarnThreshold(uint8_t percentage)
    {
        if (percentage < 5 || percentage > 20)return;
        int val = readRegister(XPOWERS_AXP2101_LOW_BAT_WARN_SET);
        if (val == -1)return;
        val &= 0x0F;
        writeRegister(XPOWERS_AXP2101_LOW_BAT_WARN_SET, val | ((percentage - 5) << 4));
    }

    uint8_t getLowBatWarnThreshold(void)
    {
        int val = readRegister(XPOWERS_AXP2101_LOW_BAT_WARN_SET);
        if (val == -1)return 0;
        val &= 0xF0;
        val >>= 4;
        return val;
    }

    /**
     * @brief  Low battery shutdown threshold 0-15%, 1% per step
     * @param  opt:   0 ~ 15
     * @retval None
     */
    void setLowBatShutdownThreshold(uint8_t opt)
    {
        if (opt > 15) {
            opt = 15;
        }
        int val = readRegister(XPOWERS_AXP2101_LOW_BAT_WARN_SET);
        if (val == -1)return;
        val &= 0xF0;
        writeRegister(XPOWERS_AXP2101_LOW_BAT_WARN_SET, val | opt);
    }

    uint8_t getLowBatShutdownThreshold(void)
    {
        return (readRegister(XPOWERS_AXP2101_LOW_BAT_WARN_SET) & 0x0F);
    }

    //!  PWRON statu  20
    // POWERON always high when EN Mode as POWERON Source
    bool isPoweronAlwaysHighSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWRON_STATUS, 5);
    }

    // Battery Insert and Good as POWERON Source
    bool isBattInsertOnSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWRON_STATUS, 4);
    }

    // Battery Voltage > 3.3V when Charged as Source
    bool isBattNormalOnSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWRON_STATUS, 3);
    }

    // Vbus Insert and Good as POWERON Source
    bool isVbusInsertOnSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWRON_STATUS, 2);
    }

    // IRQ PIN Pull-down as POWERON Source
    bool isIrqLowOnSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWRON_STATUS, 1);
    }

    // POWERON low for on level when POWERON Mode as POWERON Source
    bool isPwronLowOnSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWRON_STATUS, 0);
    }

    xpower_power_on_source_t getPowerOnSource()
    {
        int val = readRegister(XPOWERS_AXP2101_PWRON_STATUS);
        if (val == -1) return XPOWER_POWERON_SRC_UNKONW;
        return (xpower_power_on_source_t)val;
    }

    //!  PWROFF status  21
    // Die Over Temperature as POWEROFF Source
    bool isOverTemperatureOffSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWROFF_STATUS, 7);
    }

    // DCDC Over Voltage as POWEROFF Source
    bool isDcOverVoltageOffSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWROFF_STATUS, 6);
    }

    // DCDC Under Voltage as POWEROFF Source
    bool isDcUnderVoltageOffSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWROFF_STATUS, 5);
    }

    // VBUS Over Voltage as POWEROFF Source
    bool isVbusOverVoltageOffSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWROFF_STATUS, 4);
    }

    // Vsys Under Voltage as POWEROFF Source
    bool isVsysUnderVoltageOffSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWROFF_STATUS, 3);
    }

    // POWERON always low when EN Mode as POWEROFF Source
    bool isPwronAlwaysLowOffSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWROFF_STATUS, 2);
    }

    // Software configuration as POWEROFF Source
    bool isSwConfigOffSource()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWROFF_STATUS, 1);
    }

    // POWERON Pull down for off level when POWERON Mode as POWEROFF Source
    bool isPwrSourcePullDown()
    {
        return getRegisterBit(XPOWERS_AXP2101_PWROFF_STATUS, 0);
    }

    xpower_power_off_source_t getPowerOffSource()
    {
        int val = readRegister(XPOWERS_AXP2101_PWROFF_STATUS);
        if (val == -1) return XPOWER_POWEROFF_SRC_UNKONW;
        return (xpower_power_off_source_t)val;
    }

    //!REG 22H
    void enableOverTemperatureLevel2PowerOff()
    {
        setRegisterBit(XPOWERS_AXP2101_PWROFF_EN, 2);
    }

    void disableOverTemperaturePowerOff()
    {
        clrRegisterBit(XPOWERS_AXP2101_PWROFF_EN, 2);
    }

    // CHANGE:  void enablePwrOnOverVolOffLevelPowerOff()
    void enableLongPressShutdown()
    {
        setRegisterBit(XPOWERS_AXP2101_PWROFF_EN, 1);
    }

    // CHANGE:  void disablePwrOnOverVolOffLevelPowerOff()
    void disableLongPressShutdown()
    {
        clrRegisterBit(XPOWERS_AXP2101_PWROFF_EN, 1);
    }

    //CHANGE: void enablePwrOffSelectFunction()
    void setLongPressRestart()
    {
        setRegisterBit(XPOWERS_AXP2101_PWROFF_EN, 0);
    }

    //CHANGE: void disablePwrOffSelectFunction()
    void setLongPressPowerOFF()
    {
        clrRegisterBit(XPOWERS_AXP2101_PWROFF_EN, 0);
    }

    //!REG 23H
    // DCDC 120%(130%) high voltage turn off PMIC function
    void enableDCHighVoltageTurnOff()
    {
        setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 5);
    }

    void disableDCHighVoltageTurnOff()
    {
        clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 5);
    }

    // DCDC5 85% low voltage turn Off PMIC function
    void enableDC5LowVoltageTurnOff()
    {
        setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 4);
    }

    void disableDC5LowVoltageTurnOff()
    {
        clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 4);
    }

    // DCDC4 85% low voltage turn Off PMIC function
    void enableDC4LowVoltageTurnOff()
    {
        setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 3);
    }

    void disableDC4LowVoltageTurnOff()
    {
        clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 3);
    }

    // DCDC3 85% low voltage turn Off PMIC function
    void enableDC3LowVoltageTurnOff()
    {
        setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 2);
    }

    void disableDC3LowVoltageTurnOff()
    {
        clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 2);
    }

    // DCDC2 85% low voltage turn Off PMIC function
    void enableDC2LowVoltageTurnOff()
    {
        setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 1);
    }

    void disableDC2LowVoltageTurnOff()
    {
        clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 1);
    }

    // DCDC1 85% low voltage turn Off PMIC function
    void enableDC1LowVoltageTurnOff()
    {
        setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 0);
    }

    void disableDC1LowVoltageTurnOff()
    {
        clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 0);
    }


    // Set the minimum system operating voltage inside the PMU,
    // below this value will shut down the PMU,Adjustment range 2600mV~3300mV
    bool setSysPowerDownVoltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_MIN) {
            log_e("Mistake ! The minimum settable voltage of VSYS is %u mV", XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_MAX) {
            log_e("Mistake ! The maximum settable voltage of VSYS is %u mV", XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_MAX);
            return false;
        }
        int val = readRegister(XPOWERS_AXP2101_VOFF_SET);
        if (val == -1)return false;
        val &= 0xF8;
        return 0 == writeRegister(XPOWERS_AXP2101_VOFF_SET, val | ((millivolt - XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_MIN) / XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_STEPS));
    }

    uint16_t getSysPowerDownVoltage(void)
    {
        int val = readRegister(XPOWERS_AXP2101_VOFF_SET);
        if (val == -1)return false;
        return (val & 0x07) * XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_STEPS + XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_MIN;
    }

    //  PWROK setting and PWROFF sequence control 25.
    // Check the PWROK Pin enable after all dcdc/ldo output valid 128ms
    void enablePwrOk()
    {
        setRegisterBit(XPOWERS_AXP2101_PWROK_SEQU_CTRL, 4);
    }

    void disablePwrOk()
    {
        clrRegisterBit(XPOWERS_AXP2101_PWROK_SEQU_CTRL, 4);
    }

    // POWEROFF Delay 4ms after PWROK enable
    void enablePowerOffDelay()
    {
        setRegisterBit(XPOWERS_AXP2101_PWROK_SEQU_CTRL, 3);
    }

    // POWEROFF Delay 4ms after PWROK disable
    void disablePowerOffDelay()
    {
        clrRegisterBit(XPOWERS_AXP2101_PWROK_SEQU_CTRL, 3);
    }

    // POWEROFF Sequence Control the reverse of the Startup
    void enablePowerSequence()
    {
        setRegisterBit(XPOWERS_AXP2101_PWROK_SEQU_CTRL, 2);
    }

    // POWEROFF Sequence Control at the same time
    void disablePowerSequence()
    {
        clrRegisterBit(XPOWERS_AXP2101_PWROK_SEQU_CTRL, 2);
    }

    // Delay of PWROK after all power output good
    bool setPwrOkDelay(xpower_pwrok_delay_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_PWROK_SEQU_CTRL);
        if (val == -1)return false;
        val &= 0xFC;
        return 0 == writeRegister(XPOWERS_AXP2101_PWROK_SEQU_CTRL, val | opt);
    }

    xpower_pwrok_delay_t getPwrOkDelay()
    {
        int val = readRegister(XPOWERS_AXP2101_PWROK_SEQU_CTRL);
        if (val == -1)return XPOWER_PWROK_DELAY_8MS;
        return (xpower_pwrok_delay_t)(val & 0x03);
    }

    //  Sleep and 26
    void wakeupControl(xpowers_wakeup_t opt, bool enable)
    {
        int val = readRegister(XPOWERS_AXP2101_SLEEP_WAKEUP_CTRL);
        if (val == -1)return;
        enable ? (val |= opt) : (val &= (~opt));
        writeRegister(XPOWERS_AXP2101_SLEEP_WAKEUP_CTRL, val);
    }

    bool enableWakeup(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_SLEEP_WAKEUP_CTRL, 1);
    }

    bool disableWakeup(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_SLEEP_WAKEUP_CTRL, 1);
    }

    bool enableSleep(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_SLEEP_WAKEUP_CTRL, 0);
    }

    bool disableSleep(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_SLEEP_WAKEUP_CTRL, 0);
    }


    //  RQLEVEL/OFFLEVEL/ONLEVEL setting 27
    /**
     * @brief  IRQLEVEL configur
     * @param  opt: 0:1s  1:1.5s  2:2s 3:2.5s
     */
    void setIrqLevel(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL);
        if (val == -1)return;
        val &= 0xFC;
        writeRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL, val | (opt << 4));
    }

    /**
     * @brief  OFFLEVEL configuration
     * @param  opt:  0:4s 1:6s 2:8s 3:10s
     */
    void setOffLevel(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL, val | (opt << 2));
    }

    /**
     * @brief  ONLEVEL configuration
     * @param  opt: 0:128ms 1:512ms 2:1s  3:2s
     */
    void setOnLevel(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL, val | opt);
    }

    // Fast pwron setting 0  28
    // Fast Power On Start Sequence
    void setDc4FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val | ((opt & 0x3) << 6));
    }

    void setDc3FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val | ((opt & 0x3) << 4));
    }
    void setDc2FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val | ((opt & 0x3) << 2));
    }
    void setDc1FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val | (opt & 0x3));
    }

    //  Fast pwron setting 1  29
    void setAldo3FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val | ((opt & 0x3) << 6));
    }
    void setAldo2FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val | ((opt & 0x3) << 4));
    }
    void setAldo1FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val | ((opt & 0x3) << 2));
    }

    void setDc5FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val | (opt & 0x3));
    }

    //  Fast pwron setting 2  2A
    void setCpuldoFastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val | ((opt & 0x3) << 6));
    }

    void setBldo2FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val | ((opt & 0x3) << 4));
    }

    void setBldo1FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val | ((opt & 0x3) << 2));
    }

    void setAldo4FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val | (opt & 0x3));
    }

    //  Fast pwron setting 3  2B
    void setDldo2FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL, val | ((opt & 0x3) << 2));
    }

    void setDldo1FastStartSequence(xpower_start_sequence_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL, val | (opt & 0x3));
    }

    /**
     * @brief   Setting Fast Power On Start Sequence
     */
    void setFastPowerOnLevel(xpowers_fast_on_opt_t opt, xpower_start_sequence_t seq_level)
    {
        uint8_t val = 0;
        switch (opt) {
        case XPOWERS_AXP2101_FAST_DCDC1:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val | seq_level);
            break;
        case XPOWERS_AXP2101_FAST_DCDC2:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val | (seq_level << 2));
            break;
        case XPOWERS_AXP2101_FAST_DCDC3:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val | (seq_level << 4));
            break;
        case XPOWERS_AXP2101_FAST_DCDC4:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val | (seq_level << 6));
            break;
        case XPOWERS_AXP2101_FAST_DCDC5:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val | seq_level);
            break;
        case XPOWERS_AXP2101_FAST_ALDO1:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val | (seq_level << 2));
            break;
        case XPOWERS_AXP2101_FAST_ALDO2:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val | (seq_level << 4));
            break;
        case XPOWERS_AXP2101_FAST_ALDO3:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val | (seq_level << 6));
            break;
        case XPOWERS_AXP2101_FAST_ALDO4:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val | seq_level);
            break;
        case XPOWERS_AXP2101_FAST_BLDO1:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val | (seq_level << 2));
            break;
        case XPOWERS_AXP2101_FAST_BLDO2:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val | (seq_level << 4));
            break;
        case XPOWERS_AXP2101_FAST_CPUSLDO:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val | (seq_level << 6));
            break;
        case XPOWERS_AXP2101_FAST_DLDO1:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL, val | seq_level);
            break;
        case XPOWERS_AXP2101_FAST_DLDO2:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL, val | (seq_level << 2));
            break;
        default:
            break;
        }
    }

    void disableFastPowerOn(xpowers_fast_on_opt_t opt)
    {
        uint8_t val = 0;
        switch (opt) {
        case XPOWERS_AXP2101_FAST_DCDC1:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val & 0xFC);
            break;
        case XPOWERS_AXP2101_FAST_DCDC2:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val & 0xF3);
            break;
        case XPOWERS_AXP2101_FAST_DCDC3:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val & 0xCF);
            break;
        case XPOWERS_AXP2101_FAST_DCDC4:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET0);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET0, val & 0x3F);
            break;
        case XPOWERS_AXP2101_FAST_DCDC5:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val & 0xFC);
            break;
        case XPOWERS_AXP2101_FAST_ALDO1:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val & 0xF3);
            break;
        case XPOWERS_AXP2101_FAST_ALDO2:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val & 0xCF);
            break;
        case XPOWERS_AXP2101_FAST_ALDO3:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET1);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET1, val & 0x3F);
            break;
        case XPOWERS_AXP2101_FAST_ALDO4:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val & 0xFC);
            break;
        case XPOWERS_AXP2101_FAST_BLDO1:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val & 0xF3);
            break;
        case XPOWERS_AXP2101_FAST_BLDO2:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val & 0xCF);
            break;
        case XPOWERS_AXP2101_FAST_CPUSLDO:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_SET2);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_SET2, val & 0x3F);
            break;
        case XPOWERS_AXP2101_FAST_DLDO1:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL, val & 0xFC);
            break;
        case XPOWERS_AXP2101_FAST_DLDO2:
            val = readRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL);
            writeRegister(XPOWERS_AXP2101_FAST_PWRON_CTRL, val & 0xF3);
            break;
        default:
            break;
        }
    }

    void enableFastPowerOn(void)
    {
        setRegisterBit(XPOWERS_AXP2101_FAST_PWRON_CTRL, 7);
    }

    void disableFastPowerOn(void)
    {
        clrRegisterBit(XPOWERS_AXP2101_FAST_PWRON_CTRL, 7);
    }

    void enableFastWakeup(void)
    {
        setRegisterBit(XPOWERS_AXP2101_FAST_PWRON_CTRL, 6);
    }

    void disableFastWakeup(void)
    {
        clrRegisterBit(XPOWERS_AXP2101_FAST_PWRON_CTRL, 6);
    }

    // DCDC 120%(130%) high voltage turn off PMIC function
    void setDCHighVoltagePowerDown(bool en)
    {
        en ? setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 5) : clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 5);
    }

    bool getDCHighVoltagePowerDownEn()
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 5);
    }

    // DCDCS force PWM control
    void setDcUVPDebounceTime(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL);
        val &= 0xFC;
        writeRegister(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, val | opt);
    }

    void settDC1WorkModeToPwm(uint8_t enable)
    {
        enable ?
        setRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 2)
        : clrRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 2);
    }

    void settDC2WorkModeToPwm(uint8_t enable)
    {
        enable ? setRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 3)
        : clrRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 3);
    }

    void settDC3WorkModeToPwm(uint8_t enable)
    {
        enable ?
        setRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 4)
        : clrRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 4);
    }

    void settDC4WorkModeToPwm( uint8_t enable)
    {
        enable ?
        setRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 5)
        :  clrRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 5);
    }

    //1 = 100khz 0=50khz
    void setDCFreqSpreadRange(uint8_t opt)
    {
        opt ?
        setRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 6)
        :  clrRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 6);
    }

    void setDCFreqSpreadRangeEn(bool en)
    {
        en ?
        setRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 7)
        :  clrRegisterBit(XPOWERS_AXP2101_DC_FORCE_PWM_CTRL, 7);
    }

    void enableCCM()
    {
        setRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 6);
    }

    void disableCCM()
    {
        clrRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 6);
    }

    bool isenableCCM()
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 6);
    }

    enum DVMRamp {
        XPOWERS_AXP2101_DVM_RAMP_15_625US,
        XPOWERS_AXP2101_DVM_RAMP_31_250US,
    };

    //args:enum DVMRamp
    void setDVMRamp(uint8_t opt)
    {
        if (opt > 2)return;
        opt == 0 ? clrRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 5) : setRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 5);
    }



    /*
     * Power control DCDC1 functions
     */
    bool isEnableDC1(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 0);
    }

    bool enableDC1(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 0);
    }

    bool disableDC1(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 0);
    }

    bool setDC1Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_DCDC1_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_DCDC1_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_DCDC1_VOL_MIN) {
            log_e("Mistake ! DC1 minimum voltage is %u mV", XPOWERS_AXP2101_DCDC1_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_DCDC1_VOL_MAX) {
            log_e("Mistake ! DC1 maximum voltage is %u mV", XPOWERS_AXP2101_DCDC1_VOL_MAX);
            return false;
        }
        return 0 == writeRegister(XPOWERS_AXP2101_DC_VOL0_CTRL, (millivolt - XPOWERS_AXP2101_DCDC1_VOL_MIN) / XPOWERS_AXP2101_DCDC1_VOL_STEPS);
    }

    uint16_t getDC1Voltage(void)
    {
        return (readRegister(XPOWERS_AXP2101_DC_VOL0_CTRL) & 0x1F) * XPOWERS_AXP2101_DCDC1_VOL_STEPS + XPOWERS_AXP2101_DCDC1_VOL_MIN;
    }



    // DCDC1 85% low voltage turn off PMIC function
    void setDC1LowVoltagePowerDown(bool en)
    {
        en ? setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 0) : clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 0);
    }

    bool getDC1LowVoltagePowerDownEn()
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 0);
    }

    /*
     * Power control DCDC2 functions
     */
    bool isEnableDC2(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 1);
    }

    bool enableDC2(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 1);
    }

    bool disableDC2(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 1);
    }

    bool setDC2Voltage(uint16_t millivolt)
    {
        int val = readRegister(XPOWERS_AXP2101_DC_VOL1_CTRL);
        if (val == -1)return 0;
        val &= 0x80;
        if (millivolt >= XPOWERS_AXP2101_DCDC2_VOL1_MIN && millivolt <= XPOWERS_AXP2101_DCDC2_VOL1_MAX) {
            if (millivolt % XPOWERS_AXP2101_DCDC2_VOL_STEPS1) {
                log_e("Mistake !  The steps is must %umV", XPOWERS_AXP2101_DCDC2_VOL_STEPS1);
                return false;
            }
            return  0 == writeRegister(XPOWERS_AXP2101_DC_VOL1_CTRL, val | (millivolt - XPOWERS_AXP2101_DCDC2_VOL1_MIN) / XPOWERS_AXP2101_DCDC2_VOL_STEPS1);
        } else if (millivolt >= XPOWERS_AXP2101_DCDC2_VOL2_MIN && millivolt <= XPOWERS_AXP2101_DCDC2_VOL2_MAX) {
            if (millivolt % XPOWERS_AXP2101_DCDC2_VOL_STEPS2) {
                log_e("Mistake !  The steps is must %umV", XPOWERS_AXP2101_DCDC2_VOL_STEPS2);
                return false;
            }
            val |= (((millivolt - XPOWERS_AXP2101_DCDC2_VOL2_MIN) / XPOWERS_AXP2101_DCDC2_VOL_STEPS2) + XPOWERS_AXP2101_DCDC2_VOL_STEPS2_BASE);
            return  0 == writeRegister(XPOWERS_AXP2101_DC_VOL1_CTRL, val);
        }
        return false;
    }

    uint16_t getDC2Voltage(void)
    {
        int val = readRegister(XPOWERS_AXP2101_DC_VOL1_CTRL);
        if (val ==  -1)return 0;
        val &= 0x7F;
        if (val < XPOWERS_AXP2101_DCDC2_VOL_STEPS2_BASE) {
            return (val  * XPOWERS_AXP2101_DCDC2_VOL_STEPS1) +  XPOWERS_AXP2101_DCDC2_VOL1_MIN;
        } else  {
            return (val  * XPOWERS_AXP2101_DCDC2_VOL_STEPS2) - 200;
        }
        return 0;
    }

    uint8_t getDC2WorkMode(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_DCDC2_VOL_STEPS2, 7);
    }

    void setDC2LowVoltagePowerDown(bool en)
    {
        en ? setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 1) : clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 1);
    }

    bool getDC2LowVoltagePowerDownEn()
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 1);
    }

    /*
     * Power control DCDC3 functions
     */

    bool isEnableDC3(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 2);
    }

    bool enableDC3(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 2);
    }

    bool disableDC3(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 2);
    }

    /**
        0.5~1.2V,10mV/step,71steps
        1.22~1.54V,20mV/step,17steps
        1.6~3.4V,100mV/step,19steps
     */
    bool setDC3Voltage(uint16_t millivolt)
    {
        int val = readRegister(XPOWERS_AXP2101_DC_VOL2_CTRL);
        if (val == -1)return false;
        val &= 0x80;
        if (millivolt >= XPOWERS_AXP2101_DCDC3_VOL1_MIN && millivolt <= XPOWERS_AXP2101_DCDC3_VOL1_MAX) {
            if (millivolt % XPOWERS_AXP2101_DCDC3_VOL_STEPS1) {
                log_e("Mistake ! The steps is must %umV", XPOWERS_AXP2101_DCDC3_VOL_STEPS1);
                return false;
            }
            return  0 == writeRegister(XPOWERS_AXP2101_DC_VOL2_CTRL, val | (millivolt - XPOWERS_AXP2101_DCDC3_VOL_MIN) / XPOWERS_AXP2101_DCDC3_VOL_STEPS1);
        } else if (millivolt >= XPOWERS_AXP2101_DCDC3_VOL2_MIN && millivolt <= XPOWERS_AXP2101_DCDC3_VOL2_MAX) {
            if (millivolt % XPOWERS_AXP2101_DCDC3_VOL_STEPS2) {
                log_e("Mistake ! The steps is must %umV", XPOWERS_AXP2101_DCDC3_VOL_STEPS2);
                return false;
            }
            val |= (((millivolt - XPOWERS_AXP2101_DCDC3_VOL2_MIN) / XPOWERS_AXP2101_DCDC3_VOL_STEPS2) + XPOWERS_AXP2101_DCDC3_VOL_STEPS2_BASE);
            return  0 == writeRegister(XPOWERS_AXP2101_DC_VOL2_CTRL, val);
        } else if (millivolt >= XPOWERS_AXP2101_DCDC3_VOL3_MIN && millivolt <= XPOWERS_AXP2101_DCDC3_VOL3_MAX) {
            if (millivolt % XPOWERS_AXP2101_DCDC3_VOL_STEPS3) {
                log_e("Mistake ! The steps is must %umV", XPOWERS_AXP2101_DCDC3_VOL_STEPS3);
                return false;
            }
            val |= (((millivolt - XPOWERS_AXP2101_DCDC3_VOL3_MIN) / XPOWERS_AXP2101_DCDC3_VOL_STEPS3) + XPOWERS_AXP2101_DCDC3_VOL_STEPS3_BASE);
            return  0 == writeRegister(XPOWERS_AXP2101_DC_VOL2_CTRL, val);
        }
        return false;
    }


    uint16_t getDC3Voltage(void)
    {
        int val = readRegister(XPOWERS_AXP2101_DC_VOL2_CTRL) & 0x7F;
        if (val == -1)
            return 0;
        if (val < XPOWERS_AXP2101_DCDC3_VOL_STEPS2_BASE) {
            return (val  * XPOWERS_AXP2101_DCDC3_VOL_STEPS1) +  XPOWERS_AXP2101_DCDC3_VOL_MIN;
        } else if (val >= XPOWERS_AXP2101_DCDC3_VOL_STEPS2_BASE && val < XPOWERS_AXP2101_DCDC3_VOL_STEPS3_BASE) {
            return (val  * XPOWERS_AXP2101_DCDC3_VOL_STEPS2) - 200;
        } else  {
            return (val  * XPOWERS_AXP2101_DCDC3_VOL_STEPS3)  - 7200;
        }
        return 0;
    }

    uint8_t getDC3WorkMode(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_VOL2_CTRL, 7);
    }

    // DCDC3 85% low voltage turn off PMIC function
    void setDC3LowVoltagePowerDown(bool en)
    {
        en ? setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 2) : clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 2);
    }

    bool getDC3LowVoltagePowerDownEn()
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 2);
    }


    /*
    * Power control DCDC4 functions
    */
    /**
        0.5~1.2V,10mV/step,71steps
        1.22~1.84V,20mV/step,32steps
     */
    bool isEnableDC4(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 3);
    }

    bool enableDC4(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 3);
    }

    bool disableDC4(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 3);
    }

    bool setDC4Voltage(uint16_t millivolt)
    {
        int val = readRegister(XPOWERS_AXP2101_DC_VOL3_CTRL);
        if (val == -1)return false;
        val &= 0x80;
        if (millivolt >= XPOWERS_AXP2101_DCDC4_VOL1_MIN && millivolt <= XPOWERS_AXP2101_DCDC4_VOL1_MAX) {
            if (millivolt % XPOWERS_AXP2101_DCDC4_VOL_STEPS1) {
                log_e("Mistake ! The steps is must %umV", XPOWERS_AXP2101_DCDC4_VOL_STEPS1);
                return false;
            }
            return  0 == writeRegister(XPOWERS_AXP2101_DC_VOL3_CTRL, val | (millivolt - XPOWERS_AXP2101_DCDC4_VOL1_MIN) / XPOWERS_AXP2101_DCDC4_VOL_STEPS1);

        } else if (millivolt >= XPOWERS_AXP2101_DCDC4_VOL2_MIN && millivolt <= XPOWERS_AXP2101_DCDC4_VOL2_MAX) {
            if (millivolt % XPOWERS_AXP2101_DCDC4_VOL_STEPS2) {
                log_e("Mistake ! The steps is must %umV", XPOWERS_AXP2101_DCDC4_VOL_STEPS2);
                return false;
            }
            val |= (((millivolt - XPOWERS_AXP2101_DCDC4_VOL2_MIN) / XPOWERS_AXP2101_DCDC4_VOL_STEPS2) + XPOWERS_AXP2101_DCDC4_VOL_STEPS2_BASE);
            return  0 == writeRegister(XPOWERS_AXP2101_DC_VOL3_CTRL, val);

        }
        return false;
    }

    uint16_t getDC4Voltage(void)
    {
        int val = readRegister(XPOWERS_AXP2101_DC_VOL3_CTRL);
        if (val == -1)return 0;
        val &= 0x7F;
        if (val < XPOWERS_AXP2101_DCDC4_VOL_STEPS2_BASE) {
            return (val  * XPOWERS_AXP2101_DCDC4_VOL_STEPS1) +  XPOWERS_AXP2101_DCDC4_VOL1_MIN;
        } else  {
            return (val  * XPOWERS_AXP2101_DCDC4_VOL_STEPS2) - 200;
        }
        return 0;
    }

    // DCDC4 85% low voltage turn off PMIC function
    void setDC4LowVoltagePowerDown(bool en)
    {
        en ? setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 3) : clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 3);
    }

    bool getDC4LowVoltagePowerDownEn()
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 3);
    }

    /*
    * Power control DCDC5 functions,Output to gpio pin
    */
    bool isEnableDC5(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 4);
    }

    bool enableDC5(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 4);
    }

    bool disableDC5(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL, 4);
    }

    bool setDC5Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_DCDC5_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_DCDC5_VOL_STEPS);
            return false;
        }
        if (millivolt != XPOWERS_AXP2101_DCDC5_VOL_1200MV && millivolt < XPOWERS_AXP2101_DCDC5_VOL_MIN) {
            log_e("Mistake ! DC5 minimum voltage is %umV ,%umV", XPOWERS_AXP2101_DCDC5_VOL_1200MV, XPOWERS_AXP2101_DCDC5_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_DCDC5_VOL_MAX) {
            log_e("Mistake ! DC5 maximum voltage is %umV", XPOWERS_AXP2101_DCDC5_VOL_MAX);
            return false;
        }

        int val =  readRegister(XPOWERS_AXP2101_DC_VOL4_CTRL);
        if (val == -1)return false;
        val &= 0xE0;
        if (millivolt == XPOWERS_AXP2101_DCDC5_VOL_1200MV) {
            return 0 == writeRegister(XPOWERS_AXP2101_DC_VOL4_CTRL, val | XPOWERS_AXP2101_DCDC5_VOL_VAL);
        }
        val |= (millivolt - XPOWERS_AXP2101_DCDC5_VOL_MIN) / XPOWERS_AXP2101_DCDC5_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_DC_VOL4_CTRL, val);
    }

    uint16_t getDC5Voltage(void)
    {
        int val = readRegister(XPOWERS_AXP2101_DC_VOL4_CTRL) ;
        if (val == -1)return 0;
        val &= 0x1F;
        if (val == XPOWERS_AXP2101_DCDC5_VOL_VAL)return XPOWERS_AXP2101_DCDC5_VOL_1200MV;
        return  (val * XPOWERS_AXP2101_DCDC5_VOL_STEPS) + XPOWERS_AXP2101_DCDC5_VOL_MIN;
    }

    bool isDC5FreqCompensationEn(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_VOL4_CTRL, 5);
    }

    void enableDC5FreqCompensation()
    {
        setRegisterBit(XPOWERS_AXP2101_DC_VOL4_CTRL, 5);
    }

    void disableFreqCompensation()
    {
        clrRegisterBit(XPOWERS_AXP2101_DC_VOL4_CTRL, 5);
    }

    // DCDC4 85% low voltage turn off PMIC function
    void setDC5LowVoltagePowerDown(bool en)
    {
        en ? setRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 4) : clrRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 4);
    }

    bool getDC5LowVoltagePowerDownEn()
    {
        return getRegisterBit(XPOWERS_AXP2101_DC_OVP_UVP_CTRL, 4);
    }

    /*
    * Power control ALDO1 functions
    */
    bool isEnableALDO1(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 0);
    }

    bool enableALDO1(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 0);
    }

    bool disableALDO1(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 0);
    }

    bool setALDO1Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_ALDO1_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_ALDO1_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_ALDO1_VOL_MIN) {
            log_e("Mistake ! ALDO1 minimum output voltage is  %umV", XPOWERS_AXP2101_ALDO1_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_ALDO1_VOL_MAX) {
            log_e("Mistake ! ALDO1 maximum output voltage is  %umV", XPOWERS_AXP2101_ALDO1_VOL_MAX);
            return false;
        }
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL0_CTRL) & 0xE0;
        val |= (millivolt - XPOWERS_AXP2101_ALDO1_VOL_MIN) / XPOWERS_AXP2101_ALDO1_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_LDO_VOL0_CTRL, val);
    }

    uint16_t getALDO1Voltage(void)
    {
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL0_CTRL) & 0x1F;
        return val * XPOWERS_AXP2101_ALDO1_VOL_STEPS + XPOWERS_AXP2101_ALDO1_VOL_MIN;
    }

    /*
    * Power control ALDO2 functions
    */
    bool isEnableALDO2(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 1);
    }

    bool enableALDO2(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 1);
    }

    bool disableALDO2(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 1);
    }

    bool setALDO2Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_ALDO2_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_ALDO2_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_ALDO2_VOL_MIN) {
            log_e("Mistake ! ALDO2 minimum output voltage is  %umV", XPOWERS_AXP2101_ALDO2_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_ALDO2_VOL_MAX) {
            log_e("Mistake ! ALDO2 maximum output voltage is  %umV", XPOWERS_AXP2101_ALDO2_VOL_MAX);
            return false;
        }
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL1_CTRL) & 0xE0;
        val |= (millivolt - XPOWERS_AXP2101_ALDO2_VOL_MIN) / XPOWERS_AXP2101_ALDO2_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_LDO_VOL1_CTRL, val);
    }

    uint16_t getALDO2Voltage(void)
    {
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL1_CTRL) & 0x1F;
        return val * XPOWERS_AXP2101_ALDO2_VOL_STEPS + XPOWERS_AXP2101_ALDO2_VOL_MIN;
    }

    /*
     * Power control ALDO3 functions
     */
    bool isEnableALDO3(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 2);
    }

    bool enableALDO3(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 2);
    }

    bool disableALDO3(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 2);
    }

    bool setALDO3Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_ALDO3_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_ALDO3_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_ALDO3_VOL_MIN) {
            log_e("Mistake ! ALDO3 minimum output voltage is  %umV", XPOWERS_AXP2101_ALDO3_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_ALDO3_VOL_MAX) {
            log_e("Mistake ! ALDO3 maximum output voltage is  %umV", XPOWERS_AXP2101_ALDO3_VOL_MAX);
            return false;
        }
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL2_CTRL) & 0xE0;
        val |= (millivolt - XPOWERS_AXP2101_ALDO3_VOL_MIN) / XPOWERS_AXP2101_ALDO3_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_LDO_VOL2_CTRL, val);
    }

    uint16_t getALDO3Voltage(void)
    {
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL2_CTRL) & 0x1F;
        return val * XPOWERS_AXP2101_ALDO3_VOL_STEPS + XPOWERS_AXP2101_ALDO3_VOL_MIN;
    }

    /*
     * Power control ALDO4 functions
     */
    bool isEnableALDO4(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 3);
    }

    bool enableALDO4(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 3);
    }

    bool disableALDO4(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 3);
    }

    bool setALDO4Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_ALDO4_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_ALDO4_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_ALDO4_VOL_MIN) {
            log_e("Mistake ! ALDO4 minimum output voltage is  %umV", XPOWERS_AXP2101_ALDO4_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_ALDO4_VOL_MAX) {
            log_e("Mistake ! ALDO4 maximum output voltage is  %umV", XPOWERS_AXP2101_ALDO4_VOL_MAX);
            return false;
        }
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL3_CTRL) & 0xE0;
        val |= (millivolt - XPOWERS_AXP2101_ALDO4_VOL_MIN) / XPOWERS_AXP2101_ALDO4_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_LDO_VOL3_CTRL, val);
    }

    uint16_t getALDO4Voltage(void)
    {
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL3_CTRL) & 0x1F;
        return val * XPOWERS_AXP2101_ALDO4_VOL_STEPS + XPOWERS_AXP2101_ALDO4_VOL_MIN;
    }

    /*
    * Power control BLDO1 functions
    */
    bool isEnableBLDO1(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 4);
    }

    bool enableBLDO1(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 4);
    }

    bool disableBLDO1(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 4);
    }

    bool setBLDO1Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_BLDO1_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_BLDO1_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_BLDO1_VOL_MIN) {
            log_e("Mistake ! BLDO1 minimum output voltage is  %umV", XPOWERS_AXP2101_BLDO1_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_BLDO1_VOL_MAX) {
            log_e("Mistake ! BLDO1 maximum output voltage is  %umV", XPOWERS_AXP2101_BLDO1_VOL_MAX);
            return false;
        }
        int val =  readRegister(XPOWERS_AXP2101_LDO_VOL4_CTRL);
        if (val == -1)return  false;
        val &= 0xE0;
        val |= (millivolt - XPOWERS_AXP2101_BLDO1_VOL_MIN) / XPOWERS_AXP2101_BLDO1_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_LDO_VOL4_CTRL, val);
    }

    uint16_t getBLDO1Voltage(void)
    {
        int val =  readRegister(XPOWERS_AXP2101_LDO_VOL4_CTRL);
        if (val == -1)return 0;
        val &= 0x1F;
        return val * XPOWERS_AXP2101_BLDO1_VOL_STEPS + XPOWERS_AXP2101_BLDO1_VOL_MIN;
    }

    /*
    * Power control BLDO2 functions
    */
    bool isEnableBLDO2(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 5);
    }

    bool enableBLDO2(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 5);
    }

    bool disableBLDO2(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 5);
    }

    bool setBLDO2Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_BLDO2_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_BLDO2_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_BLDO2_VOL_MIN) {
            log_e("Mistake ! BLDO2 minimum output voltage is  %umV", XPOWERS_AXP2101_BLDO2_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_BLDO2_VOL_MAX) {
            log_e("Mistake ! BLDO2 maximum output voltage is  %umV", XPOWERS_AXP2101_BLDO2_VOL_MAX);
            return false;
        }
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL5_CTRL) & 0xE0;
        val |= (millivolt - XPOWERS_AXP2101_BLDO2_VOL_MIN) / XPOWERS_AXP2101_BLDO2_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_LDO_VOL5_CTRL, val);
    }

    uint16_t getBLDO2Voltage(void)
    {
        int val =  readRegister(XPOWERS_AXP2101_LDO_VOL5_CTRL);
        if (val == -1)return 0;
        val &= 0x1F;
        return val * XPOWERS_AXP2101_BLDO2_VOL_STEPS + XPOWERS_AXP2101_BLDO2_VOL_MIN;
    }

    /*
    * Power control CPUSLDO functions
    */
    bool isEnableCPUSLDO(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 6);
    }

    bool enableCPUSLDO(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 6);
    }

    bool disableCPUSLDO(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 6);
    }

    bool setCPUSLDOVoltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_CPUSLDO_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_CPUSLDO_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_CPUSLDO_VOL_MIN) {
            log_e("Mistake ! CPULDO minimum output voltage is  %umV", XPOWERS_AXP2101_CPUSLDO_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_CPUSLDO_VOL_MAX) {
            log_e("Mistake ! CPULDO maximum output voltage is  %umV", XPOWERS_AXP2101_CPUSLDO_VOL_MAX);
            return false;
        }
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL6_CTRL) & 0xE0;
        val |= (millivolt - XPOWERS_AXP2101_CPUSLDO_VOL_MIN) / XPOWERS_AXP2101_CPUSLDO_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_LDO_VOL6_CTRL, val);
    }

    uint16_t getCPUSLDOVoltage(void)
    {
        int val =  readRegister(XPOWERS_AXP2101_LDO_VOL6_CTRL);
        if (val == -1)return 0;
        val &= 0x1F;
        return val * XPOWERS_AXP2101_CPUSLDO_VOL_STEPS + XPOWERS_AXP2101_CPUSLDO_VOL_MIN;
    }


    /*
    * Power control DLDO1 functions
    */
    bool isEnableDLDO1(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 7);
    }

    bool enableDLDO1(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 7);
    }

    bool disableDLDO1(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL0, 7);
    }

    bool setDLDO1Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_DLDO1_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_DLDO1_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_DLDO1_VOL_MIN) {
            log_e("Mistake ! DLDO1 minimum output voltage is  %umV", XPOWERS_AXP2101_DLDO1_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_DLDO1_VOL_MAX) {
            log_e("Mistake ! DLDO1 maximum output voltage is  %umV", XPOWERS_AXP2101_DLDO1_VOL_MAX);
            return false;
        }
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL7_CTRL) & 0xE0;
        val |= (millivolt - XPOWERS_AXP2101_DLDO1_VOL_MIN) / XPOWERS_AXP2101_DLDO1_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_LDO_VOL7_CTRL, val);
    }

    uint16_t getDLDO1Voltage(void)
    {
        int val =  readRegister(XPOWERS_AXP2101_LDO_VOL7_CTRL);
        if (val == -1)return 0;
        val &= 0x1F;
        return val * XPOWERS_AXP2101_DLDO1_VOL_STEPS + XPOWERS_AXP2101_DLDO1_VOL_MIN;
    }

    /*
    * Power control DLDO2 functions
    */
    bool isEnableDLDO2(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL1, 0);
    }

    bool enableDLDO2(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL1, 0);
    }

    bool disableDLDO2(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_LDO_ONOFF_CTRL1, 0);
    }

    bool setDLDO2Voltage(uint16_t millivolt)
    {
        if (millivolt % XPOWERS_AXP2101_DLDO2_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", XPOWERS_AXP2101_DLDO2_VOL_STEPS);
            return false;
        }
        if (millivolt < XPOWERS_AXP2101_DLDO2_VOL_MIN) {
            log_e("Mistake ! DLDO2 minimum output voltage is  %umV", XPOWERS_AXP2101_DLDO2_VOL_MIN);
            return false;
        } else if (millivolt > XPOWERS_AXP2101_DLDO2_VOL_MAX) {
            log_e("Mistake ! DLDO2 maximum output voltage is  %umV", XPOWERS_AXP2101_DLDO2_VOL_MAX);
            return false;
        }
        uint16_t val =  readRegister(XPOWERS_AXP2101_LDO_VOL8_CTRL) & 0xE0;
        val |= (millivolt - XPOWERS_AXP2101_DLDO2_VOL_MIN) / XPOWERS_AXP2101_DLDO2_VOL_STEPS;
        return 0 == writeRegister(XPOWERS_AXP2101_LDO_VOL8_CTRL, val);
    }

    uint16_t getDLDO2Voltage(void)
    {
        int val =  readRegister(XPOWERS_AXP2101_LDO_VOL8_CTRL);
        if (val == -1)return 0;
        val &= 0x1F;
        return val * XPOWERS_AXP2101_DLDO2_VOL_STEPS + XPOWERS_AXP2101_DLDO2_VOL_MIN;
    }


    /*
     * Power ON OFF IRQ TIMMING Control method
     */

    void setIrqLevelTime(xpowers_irq_time_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL);
        if (val == -1)return;
        val &= 0xCF;
        writeRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL, val | (opt << 4));
    }

    xpowers_irq_time_t getIrqLevelTime(void)
    {
        return (xpowers_irq_time_t)((readRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL) & 0x30) >> 4);
    }

    /**
    * @brief Set the PEKEY power-on long press time.
    * @param opt: See xpowers_press_on_time_t enum for details.
    * @retval
    */
    bool setPowerKeyPressOnTime(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL);
        if (val == -1)return false;
        val  &= 0xFC;
        return 0 ==  writeRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL, val | opt);
    }

    /**
    * @brief Get the PEKEY power-on long press time.
    * @retval See xpowers_press_on_time_t enum for details.
    */
    uint8_t getPowerKeyPressOnTime(void)
    {
        int val =  readRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL);
        if (val == -1)return 0;
        return (val & 0x03) ;
    }

    /**
    * @brief Set the PEKEY power-off long press time.
    * @param opt: See xpowers_press_off_time_t enum for details.
    * @retval
    */
    bool setPowerKeyPressOffTime(uint8_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL);
        if (val == -1)return false;
        val  &= 0xF3;
        return 0 == writeRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL, val | (opt << 2));
    }

    /**
    * @brief Get the PEKEY power-off long press time.
    * @retval See xpowers_press_off_time_t enum for details.
    */
    uint8_t getPowerKeyPressOffTime(void)
    {
        return ((readRegister(XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL) & 0x0C) >> 2);
    }

    /*
     * ADC Control method
     */
    bool enableGeneralAdcChannel(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 5);
    }

    bool disableGeneralAdcChannel(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 5);
    }

    bool enableTemperatureMeasure(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 4);
    }

    bool disableTemperatureMeasure(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 4);
    }

    float getTemperature(void)
    {
        uint16_t raw = readRegisterH6L8(XPOWERS_AXP2101_ADC_DATA_RELUST8, XPOWERS_AXP2101_ADC_DATA_RELUST9);
        return XPOWERS_AXP2101_CONVERSION(raw);
    }

    bool enableSystemVoltageMeasure(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 3);
    }

    bool disableSystemVoltageMeasure(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 3);
    }

    uint16_t getSystemVoltage(void)
    {
        return readRegisterH6L8(XPOWERS_AXP2101_ADC_DATA_RELUST6, XPOWERS_AXP2101_ADC_DATA_RELUST7);
    }

    bool enableVbusVoltageMeasure(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 2);
    }

    bool disableVbusVoltageMeasure(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 2);
    }

    uint16_t getVbusVoltage(void)
    {
        if (!isVbusIn()) {
            return 0;
        }
        return readRegisterH6L8(XPOWERS_AXP2101_ADC_DATA_RELUST4, XPOWERS_AXP2101_ADC_DATA_RELUST5);
    }

    bool enableTSPinMeasure(void)
    {
        // TS pin is the battery temperature sensor input and will affect the charger
        uint8_t value =  readRegister(XPOWERS_AXP2101_TS_PIN_CTRL);
        value &= 0xE0;
        writeRegister(XPOWERS_AXP2101_TS_PIN_CTRL, value | 0x07);
        return setRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 1);
    }

    bool disableTSPinMeasure(void)
    {
        // TS pin is the external fixed input and doesn't affect the charger
        uint8_t value =  readRegister(XPOWERS_AXP2101_TS_PIN_CTRL);
        value &= 0xF0;
        writeRegister(XPOWERS_AXP2101_TS_PIN_CTRL, value | 0x10);
        return clrRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 1);
    }


    bool enableTSPinLowFreqSample(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 7);
    }

    bool disableTSPinLowFreqSample(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_ADC_DATA_RELUST2, 7);
    }


    /**
     * Calculate temperature from TS pin ADC value using Steinhart-Hart equation.
     * 
     * @param SteinhartA Steinhart-Hart coefficient A (default: 1.126e-3)
     * @param SteinhartB Steinhart-Hart coefficient B (default: 2.38e-4)
     * @param SteinhartC Steinhart-Hart coefficient C (default: 8.5e-8)
     * @return Temperature in Celsius. Returns 0 if ADC value is 0x2000 (invalid measurement).
     *
     * @details
     * This function converts the ADC reading from the TS pin to temperature using:
     * 1. Voltage calculation: V = ADC_raw × 0.0005 (V)
     * 2. Resistance calculation: R = V / I (Ω), where I = 50μA
     * 3. Temperature calculation: T = 1/(A+B*ln(R)+C*(ln(R))^3) - 273.15 (℃)
     *
     * @note
     * The calculation parameters are from the AXP2101 Datasheet, using the TH11-3H103F NTC resistor 
     *     as the Steinhart-Hart equation calculation parameters
     * 1. Coefficients A, B, C should be calibrated for specific NTC thermistor.
     * 2. ADC value 0x2000 indicates sensor fault (e.g., open circuit).
     * 3. Valid temperature range: typically -20℃ to 60℃. Accuracy may degrade outside this range.
     */
    float getTsTemperature(float SteinhartA = 1.126e-3,
                           float SteinhartB = 2.38e-4,
                           float SteinhartC = 8.5e-8)
    {
        uint16_t  adc_raw =  getTsPinValue();  // Read raw ADC value from TS pin

        // Check for invalid measurement (0x2000 indicates sensor disconnection)
        if (adc_raw == 0x2000) {
            return 0;
        }
        float current_ma = 0.05f;  // Current source: 50μA
        float voltage = adc_raw * 0.0005f;  // Convert ADC value to voltage (V)
        float resistance = voltage / (current_ma / 1000.0f);  // Calculate resistance (Ω)
        // Convert resistance to temperature using Steinhart-Hart equation
        return resistance_to_temperature(resistance, SteinhartA, SteinhartB, SteinhartC);
    }

    // raw value
    uint16_t getTsPinValue(void)
    {
        return readRegisterH6L8(XPOWERS_AXP2101_ADC_DATA_RELUST2, XPOWERS_AXP2101_ADC_DATA_RELUST3);
    }

    bool enableBattVoltageMeasure(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 0);
    }

    bool disableBattVoltageMeasure(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_ADC_CHANNEL_CTRL, 0);
    }

    bool enableBattDetection(void)
    {
        return setRegisterBit(XPOWERS_AXP2101_BAT_DET_CTRL, 0);
    }

    bool disableBattDetection(void)
    {
        return clrRegisterBit(XPOWERS_AXP2101_BAT_DET_CTRL, 0);
    }

    uint16_t getBattVoltage(void)
    {
        if (!isBatteryConnect()) {
            return 0;
        }
        return readRegisterH5L8(XPOWERS_AXP2101_ADC_DATA_RELUST0, XPOWERS_AXP2101_ADC_DATA_RELUST1);
    }

    int getBatteryPercent(void)
    {
        if (!isBatteryConnect()) {
            return -1;
        }
        return readRegister(XPOWERS_AXP2101_BAT_PERCENT_DATA);
    }

    /*
    * CHG LED setting and control
    */
    // void enableChargingLed(void)
    // {
    //     setRegisterBit(XPOWERS_AXP2101_CHGLED_SET_CTRL, 0);
    // }

    // void disableChargingLed(void)
    // {
    //     clrRegisterBit(XPOWERS_AXP2101_CHGLED_SET_CTRL, 0);
    // }

    /**
    * @brief Set charging led mode.
    * @retval See xpowers_chg_led_mode_t enum for details.
    */
    void setChargingLedMode(uint8_t mode)
    {
        int val;
        switch (mode) {
        case XPOWERS_CHG_LED_OFF:
        // clrRegisterBit(XPOWERS_AXP2101_CHGLED_SET_CTRL, 0);
        // break;
        case XPOWERS_CHG_LED_BLINK_1HZ:
        case XPOWERS_CHG_LED_BLINK_4HZ:
        case XPOWERS_CHG_LED_ON:
            val = readRegister(XPOWERS_AXP2101_CHGLED_SET_CTRL);
            if (val == -1)return;
            val &= 0xC8;
            val |= 0x05;    //use manual ctrl
            val |= (mode << 4);
            writeRegister(XPOWERS_AXP2101_CHGLED_SET_CTRL, val);
            break;
        case XPOWERS_CHG_LED_CTRL_CHG:
            val = readRegister(XPOWERS_AXP2101_CHGLED_SET_CTRL);
            if (val == -1)return;
            val &= 0xF9;
            writeRegister(XPOWERS_AXP2101_CHGLED_SET_CTRL, val | 0x01); // use type A mode
            // writeRegister(XPOWERS_AXP2101_CHGLED_SET_CTRL, val | 0x02); // use type B mode
            break;
        default:
            break;
        }
    }

    uint8_t getChargingLedMode()
    {
        int val = readRegister(XPOWERS_AXP2101_CHGLED_SET_CTRL);
        if (val == -1)return XPOWERS_CHG_LED_OFF;
        val >>= 1;
        if ((val & 0x02) == 0x02) {
            val >>= 4;
            return val & 0x03;
        }
        return XPOWERS_CHG_LED_CTRL_CHG;
    }

    /**
     * @brief 预充电充电电流限制
     * @note  Precharge current limit 25*N mA
     * @param  opt: 25 * opt
     * @retval None
     */
    void setPrechargeCurr(xpowers_prechg_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_IPRECHG_SET);
        if (val == -1)return;
        val &= 0xFC;
        writeRegister(XPOWERS_AXP2101_IPRECHG_SET, val | opt);
    }

    xpowers_prechg_t getPrechargeCurr(void)
    {
        return (xpowers_prechg_t)(readRegister(XPOWERS_AXP2101_IPRECHG_SET) & 0x03);
    }


    /**
    * @brief Set charge current.
    * @param  opt: See xpowers_axp2101_chg_curr_t enum for details.
    * @retval
    */
    bool setChargerConstantCurr(uint8_t opt)
    {
        if (opt > XPOWERS_AXP2101_CHG_CUR_1000MA)return false;
        int val = readRegister(XPOWERS_AXP2101_ICC_CHG_SET);
        if (val == -1)return false;
        val &= 0xE0;
        return 0 == writeRegister(XPOWERS_AXP2101_ICC_CHG_SET, val | opt);
    }

    /**
     * @brief Get charge current settings.
    *  @retval See xpowers_axp2101_chg_curr_t enum for details.
     */
    uint8_t getChargerConstantCurr(void)
    {
        int val = readRegister(XPOWERS_AXP2101_ICC_CHG_SET);
        if (val == -1)return 0;
        return val & 0x1F;
    }

    /**
     * @brief  充电终止电流限制
     * @note   Charging termination of current limit
     * @retval
     */
    void setChargerTerminationCurr(xpowers_axp2101_chg_iterm_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_ITERM_CHG_SET_CTRL);
        if (val == -1)return;
        val &= 0xF0;
        writeRegister(XPOWERS_AXP2101_ITERM_CHG_SET_CTRL, val | opt);
    }

    xpowers_axp2101_chg_iterm_t getChargerTerminationCurr(void)
    {
        return (xpowers_axp2101_chg_iterm_t)(readRegister(XPOWERS_AXP2101_ITERM_CHG_SET_CTRL) & 0x0F);
    }

    void enableChargerTerminationLimit(void)
    {
        int val = readRegister(XPOWERS_AXP2101_ITERM_CHG_SET_CTRL);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_ITERM_CHG_SET_CTRL, val | 0x10);
    }

    void disableChargerTerminationLimit(void)
    {
        int val = readRegister(XPOWERS_AXP2101_ITERM_CHG_SET_CTRL);
        if (val == -1)return;
        writeRegister(XPOWERS_AXP2101_ITERM_CHG_SET_CTRL, val & 0xEF);
    }

    bool isChargerTerminationLimit(void)
    {
        return getRegisterBit(XPOWERS_AXP2101_ITERM_CHG_SET_CTRL, 4);
    }


    /**
    * @brief Set charge target voltage.
    * @param  opt: See xpowers_axp2101_chg_vol_t enum for details.
    * @retval
    */
    bool setChargeTargetVoltage(uint8_t opt)
    {
        if (opt >= XPOWERS_AXP2101_CHG_VOL_MAX)return false;
        int val = readRegister(XPOWERS_AXP2101_CV_CHG_VOL_SET);
        if (val == -1)return false;
        val &= 0xF8;
        return 0 == writeRegister(XPOWERS_AXP2101_CV_CHG_VOL_SET, val | opt);
    }

    /**
     * @brief Get charge target voltage settings.
     * @retval See xpowers_axp2101_chg_vol_t enum for details.
     */
    uint8_t getChargeTargetVoltage(void)
    {
        return (readRegister(XPOWERS_AXP2101_CV_CHG_VOL_SET) & 0x07);
    }


    /**
     * @brief  设定热阈值
     * @note   Thermal regulation threshold setting
     */
    void setThermaThreshold(xpowers_thermal_t opt)
    {
        int val = readRegister(XPOWERS_AXP2101_THE_REGU_THRES_SET);
        if (val == -1)return;
        val &= 0xFC;
        writeRegister(XPOWERS_AXP2101_THE_REGU_THRES_SET, val | opt);
    }

    xpowers_thermal_t getThermaThreshold(void)
    {
        return (xpowers_thermal_t)(readRegister(XPOWERS_AXP2101_THE_REGU_THRES_SET) & 0x03);
    }

    uint8_t getBatteryParameter()
    {
        return  readRegister(XPOWERS_AXP2101_BAT_PARAME);
    }

    void fuelGaugeControl(bool writeROM, bool enable)
    {
        if (writeROM) {
            clrRegisterBit(XPOWERS_AXP2101_FUEL_GAUGE_CTRL, 4);
        } else {
            setRegisterBit(XPOWERS_AXP2101_FUEL_GAUGE_CTRL, 4);
        }
        if (enable) {
            setRegisterBit(XPOWERS_AXP2101_FUEL_GAUGE_CTRL, 0);
        } else {
            clrRegisterBit(XPOWERS_AXP2101_FUEL_GAUGE_CTRL, 0);
        }
    }

    /*
     * Interrupt status/control functions
     */

    /**
    * @brief  Get the interrupt controller mask value.
    * @retval   Mask value corresponds to xpowers_axp2101_irq_t ,
    */
    uint64_t getIrqStatus(void)
    {
        statusRegister[0] = readRegister(XPOWERS_AXP2101_INTSTS1);
        statusRegister[1] = readRegister(XPOWERS_AXP2101_INTSTS2);
        statusRegister[2] = readRegister(XPOWERS_AXP2101_INTSTS3);
        return (uint32_t)(statusRegister[0] << 16) | (uint32_t)(statusRegister[1] << 8) | (uint32_t)(statusRegister[2]);
    }


    /**
     * @brief  Clear interrupt controller state.
     */
    void clearIrqStatus()
    {
        for (int i = 0; i < XPOWERS_AXP2101_INTSTS_CNT; i++) {
            writeRegister(XPOWERS_AXP2101_INTSTS1 + i, 0xFF);
            statusRegister[i] = 0;
        }
    }

    /*
    *  @brief  Debug interrupt setting register
    * */
#ifdef ARDUINO
    void printIntRegister(Stream *stream)
    {
        for (int i = 0; i < XPOWERS_AXP2101_INTSTS_CNT; i++) {
            uint8_t val =  readRegister(XPOWERS_AXP2101_INTEN1 + i);
            stream->print("INT["); stream->print(i);
            stream->print(']');
            stream->print("  HEX: "); stream->print(val, HEX);
            stream->print(" BIN:0b"); stream->println(val, BIN);
        }
    }
#else
    void printIntRegister()
    {
        for (int i = 0; i < XPOWERS_AXP2101_INTSTS_CNT; i++) {
            uint8_t val =  readRegister(XPOWERS_AXP2101_INTEN1 + i);
            printf("INT[%d] HEX:0x%X\n", i, val);
        }
    }
#endif

    /**
     * @brief  Enable PMU interrupt control mask .
     * @param  opt: View the related chip type xpowers_axp2101_irq_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval
     */
    bool enableIRQ(uint64_t opt)
    {
        return setInterruptImpl(opt, true);
    }

    /**
     * @brief  Disable PMU interrupt control mask .
     * @param  opt: View the related chip type xpowers_axp2101_irq_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval
     */
    bool disableIRQ(uint64_t opt)
    {
        return setInterruptImpl(opt, false);
    }

    //IRQ STATUS 0
    bool isDropWarningLevel2Irq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_WARNING_LEVEL2_IRQ;
        if (intRegister[0] & mask) {
            return IS_BIT_SET(statusRegister[0], mask);
        }
        return false;
    }

    bool isDropWarningLevel1Irq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_WARNING_LEVEL1_IRQ;
        if (intRegister[0] & mask) {
            return IS_BIT_SET(statusRegister[0], mask);
        }
        return false;
    }

    bool isGaugeWdtTimeoutIrq()
    {
        uint8_t mask = XPOWERS_AXP2101_WDT_TIMEOUT_IRQ;
        if (intRegister[0] & mask) {
            return IS_BIT_SET(statusRegister[0], mask);
        }
        return false;
    }

    bool isBatChargerOverTemperatureIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_BAT_CHG_OVER_TEMP_IRQ;
        if (intRegister[0] & mask) {
            return IS_BIT_SET(statusRegister[0], mask);
        }
        return false;
    }

    bool isBatChargerUnderTemperatureIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_BAT_CHG_UNDER_TEMP_IRQ;
        if (intRegister[0] & mask) {
            return IS_BIT_SET(statusRegister[0], mask);
        }
        return false;
    }

    bool isBatWorkOverTemperatureIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_BAT_NOR_OVER_TEMP_IRQ;
        if (intRegister[0] & mask) {
            return IS_BIT_SET(statusRegister[0], mask);
        }
        return false;
    }

    bool isBatWorkUnderTemperatureIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_BAT_NOR_UNDER_TEMP_IRQ;
        if (intRegister[0] & mask) {
            return IS_BIT_SET(statusRegister[0], mask);
        }
        return false;
    }

    //IRQ STATUS 1
    bool isVbusInsertIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_VBUS_INSERT_IRQ  >> 8;
        if (intRegister[1] & mask) {
            return IS_BIT_SET(statusRegister[1], mask);
        }
        return false;
    }

    bool isVbusRemoveIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_VBUS_REMOVE_IRQ  >> 8;
        if (intRegister[1] & mask) {
            return IS_BIT_SET(statusRegister[1], mask);
        }
        return false;
    }

    bool isBatInsertIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_BAT_INSERT_IRQ  >> 8;
        if (intRegister[1] & mask) {
            return IS_BIT_SET(statusRegister[1], mask);
        }
        return false;
    }

    bool isBatRemoveIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_BAT_REMOVE_IRQ  >> 8;
        if (intRegister[1] & mask) {
            return IS_BIT_SET(statusRegister[1], mask);
        }
        return false;
    }

    bool isPekeyShortPressIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_PKEY_SHORT_IRQ  >> 8;
        if (intRegister[1] & mask) {
            return IS_BIT_SET(statusRegister[1], mask);
        }
        return false;

    }

    bool isPekeyLongPressIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_PKEY_LONG_IRQ  >> 8;
        if (intRegister[1] & mask) {
            return IS_BIT_SET(statusRegister[1], mask);
        }
        return false;
    }

    bool isPekeyNegativeIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ  >> 8;
        if (intRegister[1] & mask) {
            return IS_BIT_SET(statusRegister[1], mask);
        }
        return false;
    }

    bool isPekeyPositiveIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_PKEY_POSITIVE_IRQ  >> 8;
        if (intRegister[1] & mask) {
            return IS_BIT_SET(statusRegister[1], mask);
        }
        return false;
    }

    //IRQ STATUS 2
    bool isWdtExpireIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_WDT_EXPIRE_IRQ  >> 16;
        if (intRegister[2] & mask) {
            return IS_BIT_SET(statusRegister[2], mask);
        }
        return false;
    }

    bool isLdoOverCurrentIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_LDO_OVER_CURR_IRQ  >> 16;
        if (intRegister[2] & mask) {
            return IS_BIT_SET(statusRegister[2], mask);
        }
        return false;
    }

    bool isBatfetOverCurrentIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_BATFET_OVER_CURR_IRQ  >> 16;
        if (intRegister[2] & mask) {
            return IS_BIT_SET(statusRegister[2], mask);
        }
        return false;
    }

    bool isBatChargeDoneIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  >> 16;
        if (intRegister[2] & mask) {
            return IS_BIT_SET(statusRegister[2], mask);
        }
        return false;
    }

    bool isBatChargeStartIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_BAT_CHG_START_IRQ  >> 16;
        if (intRegister[2] & mask) {
            return IS_BIT_SET(statusRegister[2], mask);
        }
        return false;
    }

    bool isBatDieOverTemperatureIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_DIE_OVER_TEMP_IRQ  >> 16;
        if (intRegister[2] & mask) {
            return IS_BIT_SET(statusRegister[2], mask);
        }
        return false;
    }

    bool isChargeOverTimeoutIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_CHAGER_TIMER_IRQ  >> 16;
        if (intRegister[2] & mask) {
            return IS_BIT_SET(statusRegister[2], mask);
        }
        return false;
    }

    bool isBatOverVoltageIrq(void)
    {
        uint8_t mask = XPOWERS_AXP2101_BAT_OVER_VOL_IRQ  >> 16;
        if (intRegister[2] & mask) {
            return IS_BIT_SET(statusRegister[2], mask);
        }
        return false;
    }


    uint8_t getChipID(void)
    {
        return readRegister(XPOWERS_AXP2101_IC_TYPE);
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
        case XPOWERS_DCDC4:
            return getDC4Voltage();
        case XPOWERS_DCDC5:
            return getDC5Voltage();
        case XPOWERS_ALDO1:
            return getALDO1Voltage();
        case XPOWERS_ALDO2:
            return getALDO2Voltage();
        case XPOWERS_ALDO3:
            return getALDO3Voltage();
        case XPOWERS_ALDO4:
            return getALDO4Voltage();
        case XPOWERS_BLDO1:
            return getBLDO1Voltage();
        case XPOWERS_BLDO2:
            return getBLDO2Voltage();
        case XPOWERS_DLDO1:
            return getDLDO1Voltage();
        case XPOWERS_DLDO2:
            return getDLDO2Voltage();
        case XPOWERS_VBACKUP:
            return getButtonBatteryVoltage();
        default:
            break;
        }
        return 0;
    }

    bool inline enablePowerOutput(uint8_t channel)
    {
        switch (channel) {
        case XPOWERS_DCDC1:
            return enableDC1();
        case XPOWERS_DCDC2:
            return enableDC2();
        case XPOWERS_DCDC3:
            return enableDC3();
        case XPOWERS_DCDC4:
            return enableDC4();
        case XPOWERS_DCDC5:
            return enableDC5();
        case XPOWERS_ALDO1:
            return enableALDO1();
        case XPOWERS_ALDO2:
            return enableALDO2();
        case XPOWERS_ALDO3:
            return enableALDO3();
        case XPOWERS_ALDO4:
            return enableALDO4();
        case XPOWERS_BLDO1:
            return enableBLDO1();
        case XPOWERS_BLDO2:
            return enableBLDO2();
        case XPOWERS_DLDO1:
            return enableDLDO1();
        case XPOWERS_DLDO2:
            return enableDLDO2();
        case XPOWERS_VBACKUP:
            return enableButtonBatteryCharge();
        default:
            break;
        }
        return false;
    }

    bool inline disablePowerOutput(uint8_t channel)
    {
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
        case XPOWERS_DCDC4:
            return disableDC4();
        case XPOWERS_DCDC5:
            return disableDC5();
        case XPOWERS_ALDO1:
            return disableALDO1();
        case XPOWERS_ALDO2:
            return disableALDO2();
        case XPOWERS_ALDO3:
            return disableALDO3();
        case XPOWERS_ALDO4:
            return disableALDO4();
        case XPOWERS_BLDO1:
            return disableBLDO1();
        case XPOWERS_BLDO2:
            return disableBLDO2();
        case XPOWERS_DLDO1:
            return disableDLDO1();
        case XPOWERS_DLDO2:
            return disableDLDO2();
        case XPOWERS_VBACKUP:
            return disableButtonBatteryCharge();
        case XPOWERS_CPULDO:
            return disableCPUSLDO();
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
        case XPOWERS_DCDC4:
            return isEnableDC4();
        case XPOWERS_DCDC5:
            return isEnableDC5();
        case XPOWERS_ALDO1:
            return isEnableALDO1();
        case XPOWERS_ALDO2:
            return isEnableALDO2();
        case XPOWERS_ALDO3:
            return isEnableALDO3();
        case XPOWERS_ALDO4:
            return isEnableALDO4();
        case XPOWERS_BLDO1:
            return isEnableBLDO1();
        case XPOWERS_BLDO2:
            return isEnableBLDO2();
        case XPOWERS_DLDO1:
            return isEnableDLDO1();
        case XPOWERS_DLDO2:
            return isEnableDLDO2();
        case XPOWERS_VBACKUP:
            return isEnableButtonBatteryCharge();
        case XPOWERS_CPULDO:
            return isEnableCPUSLDO();
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
        case XPOWERS_DCDC4:
            return setDC4Voltage(millivolt);
        case XPOWERS_DCDC5:
            return setDC5Voltage(millivolt);
        case XPOWERS_ALDO1:
            return setALDO1Voltage(millivolt);
        case XPOWERS_ALDO2:
            return setALDO2Voltage(millivolt);
        case XPOWERS_ALDO3:
            return setALDO3Voltage(millivolt);
        case XPOWERS_ALDO4:
            return setALDO4Voltage(millivolt);
        case XPOWERS_BLDO1:
            return setBLDO1Voltage(millivolt);
        case XPOWERS_BLDO2:
            return setBLDO1Voltage(millivolt);
        case XPOWERS_DLDO1:
            return setDLDO1Voltage(millivolt);
        case XPOWERS_DLDO2:
            return setDLDO1Voltage(millivolt);
        case XPOWERS_VBACKUP:
            return setButtonBatteryChargeVoltage(millivolt);
        case XPOWERS_CPULDO:
            return setCPUSLDOVoltage(millivolt);
        default:
            break;
        }
        return false;
    }

    bool initImpl()
    {
        if (getChipID() == XPOWERS_AXP2101_CHIP_ID) {
            setChipModel(XPOWERS_AXP2101);
            disableTSPinMeasure();      //Disable NTC temperature detection by default
            return true;
        }
        return  false;
    }

    /*
     * Interrupt control functions
     */
    bool setInterruptImpl(uint32_t opts, bool enable)
    {
        int res = 0;
        uint8_t data = 0, value = 0;
        log_d("%s - HEX:0x%x \n", enable ? "ENABLE" : "DISABLE", opts);
        if (opts & 0x0000FF) {
            value = opts & 0xFF;
            // log_d("Write INT0: %x\n", value);
            data = readRegister(XPOWERS_AXP2101_INTEN1);
            intRegister[0] =  enable ? (data | value) : (data & (~value));
            res |= writeRegister(XPOWERS_AXP2101_INTEN1, intRegister[0]);
        }
        if (opts & 0x00FF00) {
            value = opts >> 8;
            // log_d("Write INT1: %x\n", value);
            data = readRegister(XPOWERS_AXP2101_INTEN2);
            intRegister[1] =  enable ? (data | value) : (data & (~value));
            res |= writeRegister(XPOWERS_AXP2101_INTEN2, intRegister[1]);
        }
        if (opts & 0xFF0000) {
            value = opts >> 16;
            // log_d("Write INT2: %x\n", value);
            data = readRegister(XPOWERS_AXP2101_INTEN3);
            intRegister[2] =  enable ? (data | value) : (data & (~value));
            res |= writeRegister(XPOWERS_AXP2101_INTEN3, intRegister[2]);
        }
        return res == 0;
    }

    const char  *getChipNameImpl(void)
    {
        return "AXP2101";
    }

private:

    float resistance_to_temperature(float resistance, float SteinhartA,
                                    float SteinhartB,
                                    float SteinhartC)
    {
        float ln_r = log(resistance);
        float t_inv = SteinhartA + SteinhartB * ln_r + SteinhartC * pow(ln_r, 3);
        return (1.0f / t_inv) - 273.15f;
    }
    uint8_t statusRegister[XPOWERS_AXP2101_INTSTS_CNT];
    uint8_t intRegister[XPOWERS_AXP2101_INTSTS_CNT];
};



