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
 * @file      PowersBQ25896.tpp
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2024-10-29
 *
 */
#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <math.h>
#endif /*ARDUINO*/
#include "XPowersCommon.tpp"
#include "REG/GeneralPPMConstants.h"
#include "REG/BQ25896Constants.h"

class PowersBQ25896 :
    public XPowersCommon<PowersBQ25896>
{
    friend class XPowersCommon<PowersBQ25896>;
public:

    enum BusStatus {
        BUS_STATE_NOINPUT,
        BUS_STATE_USB_SDP,
        BUS_STATE_ADAPTER,
        BUS_STATE_OTG
    } ;

    enum ChargeStatus {
        CHARGE_STATE_NO_CHARGE,
        CHARGE_STATE_PRE_CHARGE,
        CHARGE_STATE_FAST_CHARGE,
        CHARGE_STATE_DONE,
        CHARGE_STATE_UNKOWN,
    } ;

    enum NTCStatus {
        BUCK_NTC_NORMAL = 0,
        BUCK_NTC_WARM = 2,
        BUCK_NTC_COOL = 3,
        BUCK_NTC_COLD = 5,
        BUCK_NTC_HOT = 6,
    };

    enum BoostNTCStatus {
        BOOST_NTC_NORMAL = 0,
        BOOST_NTC_COLD = 5,
        BOOST_NTC_HOT = 6,
    };

    enum WatchdogConfig {
        TIMER_OUT_40SEC,      //40 Second
        TIMER_OUT_80SEC,      //80 Second
        TIMER_OUT_160SEC,     //160 Second
    } ;

    enum MeasureMode {
        ONE_SHORT,
        CONTINUOUS,
    };

    enum BoostFreq {
        BOOST_FREQ_1500KHZ,
        BOOST_FREQ_500KHZ,
    };

    enum ExitBoostModeVolt {
        MINI_VOLT_2V9,
        MINI_VOLT_2V5
    };

    enum FastChargeThreshold {
        FAST_CHG_THR_2V8,
        FAST_CHG_THR_3V0
    };

    enum RechargeThresholdOffset {
        RECHARGE_OFFSET_100MV,
        RECHARGE_OFFSET_200MV
    };

    enum FastChargeTimer {
        FAST_CHARGE_TIMER_5H,
        FAST_CHARGE_TIMER_8H,
        FAST_CHARGE_TIMER_12H,
        FAST_CHARGE_TIMER_20H,
    };

    enum JeitaLowTemperatureCurrent {
        JEITA_LOW_TEMP_50,  //50% of ICHG (REG04[6:0])
        JEITA_LOW_TEMP_20,  //20% of ICHG (REG04[6:0])
    };

    enum BoostCurrentLimit {
        BOOST_CUR_LIMIT_500MA,
        BOOST_CUR_LIMIT_750MA,
        BOOST_CUR_LIMIT_1200MA,
        BOOST_CUR_LIMIT_1400MA,
        BOOST_CUR_LIMIT_1650MA,
        BOOST_CUR_LIMIT_1875MA,
        BOOST_CUR_LIMIT_2150MA,
    } ;

#if defined(ARDUINO)
    PowersBQ25896(TwoWire &w, int sda = SDA, int scl = SCL, uint8_t addr = BQ25896_SLAVE_ADDRESS)
    {
        __wire = &w;
        __sda = sda;
        __scl = scl;
        __addr = addr;
    }
#endif

    PowersBQ25896(uint8_t addr, iic_fptr_t readRegCallback, iic_fptr_t writeRegCallback)
    {
        thisReadRegCallback = readRegCallback;
        thisWriteRegCallback = writeRegCallback;
        __addr = addr;
    }

    PowersBQ25896()
    {
#if defined(ARDUINO)
        __wire = &Wire;
        __sda = SDA;
        __scl = SCL;
#endif
        __addr = BQ25896_SLAVE_ADDRESS;
    }

    ~PowersBQ25896()
    {
        log_d("~PowersBQ25896");
        deinit();
    }

#if defined(ARDUINO)
    bool init(TwoWire &w, int sda = SDA, int scl = SCL, uint8_t addr = BQ25896_SLAVE_ADDRESS)
    {
        __wire = &w;
        __sda = sda;
        __scl = scl;
        __addr = addr;
        __irq_mask = 0;
        return begin();
    }
#endif

    const char *getChipName()
    {
        return getChipID() == BQ25896_DEV_REV ? "BQ25896" : "Unknown";
    }

    bool init()
    {
        return begin();
    }

    void deinit()
    {
        end();
    }

    /***************************************************
     * POWERS_PPM_REG_00H ✅
     **************************************************/
    // USB input path is disabled and can only be reset by disconnecting
    // the power supply, otherwise the power cannot be turned on
    void enterHizMode()
    {
        setRegisterBit(POWERS_PPM_REG_00H, 7);
    }

    void exitHizMode()
    {
        clrRegisterBit(POWERS_PPM_REG_00H, 7);
    }

    bool isHizMode()
    {
        return getRegisterBit(POWERS_PPM_REG_00H, 7);
    }

    // Enable ILIM Pin
    void enableCurrentLimitPin()
    {
        setRegisterBit(POWERS_PPM_REG_00H, 6);
    }

    void disableCurrentLimitPin()
    {
        clrRegisterBit(POWERS_PPM_REG_00H, 6);
    }

    bool isEnableCurrentLimitPin()
    {
        return getRegisterBit(POWERS_PPM_REG_00H, 6);
    }

    // Input Current Limit
    // Offset: 100mA
    // Range: 100mA (000000) – 3.25A (111111)
    // Default:0001000 (500mA)
    // (Actual input current limit is the lower of I2C or ILIM pin)
    // IINLIM bits are changed automaticallly after input source
    // type detection is completed
    // bq25896
    // PSEL = Hi (USB500) = 500mA
    // PSEL = Lo = 3.25A
    bool setInputCurrentLimit(uint16_t milliampere)
    {
        if (milliampere % POWERS_BQ25896_IN_CURRENT_STEP) {
            log_e("Mistake ! The steps is must %u mA", POWERS_BQ25896_IN_CURRENT_STEP);
            return false;
        }
        if (milliampere < POWERS_BQ25896_IN_CURRENT_MIN) {
            milliampere = POWERS_BQ25896_IN_CURRENT_MIN;
        }
        if (milliampere > POWERS_BQ25896_IN_CURRENT_MAX) {
            milliampere = POWERS_BQ25896_IN_CURRENT_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_00H);
        if (val == -1)
            return false;
        val &= 0xC0;
        milliampere = ((milliampere - POWERS_BQ25896_IN_CURRENT_MIN) / POWERS_BQ25896_IN_CURRENT_STEP);
        val |=  milliampere;
        return writeRegister(POWERS_PPM_REG_00H, val) != -1;
    }

    uint32_t getInputCurrentLimit()
    {
        int val = readRegister(POWERS_PPM_REG_00H);
        if (val == -1)
            return false;
        val &= 0x3F;
        return (val * POWERS_BQ25896_IN_CURRENT_STEP) + POWERS_BQ25896_IN_CURRENT_MIN;
    }

    /***************************************************
     * POWERS_PPM_REG_01H ✅
     **************************************************/

    // Boost Mode Hot Temperature Monitor Threshold
    // 0x0 – VBHOT1 Threshold (34.75%) (default)
    // 0x01 – VBHOT0 Threshold (Typ. 37.75%)
    // 0x02 – VBHOT2 Threshold (Typ. 31.25%)
    // 0x03 – Disable boost mode thermal protection
    void setBoostModeHotTemperatureMonitorThreshold(uint8_t params)
    {
        int val = readRegister(POWERS_PPM_REG_01H);
        if (val == -1)return;
        val &= 0x3F;
        val |=  (params << 6);
        writeRegister(POWERS_PPM_REG_01H, val);
    }

    // Boost Mode Cold Temperature Monitor Threshold
    // 0 – VBCOLD0 Threshold (Typ. 77%) (default)
    // 1 – VBCOLD1 Threshold (Typ. 80%)
    void setBoostModeColdTemperatureMonitorThreshold(uint8_t params)
    {
        int val = readRegister(POWERS_PPM_REG_01H);
        if (val == -1)return;
        val &= 0xDF;
        val |=  ((params & 0x01) << 5);
        writeRegister(POWERS_PPM_REG_01H, val);
    }

    // Input Voltage Limit Offset
    // Default: 600mV (00110)
    // Range: 0mV – 3100mV
    // Minimum VINDPM threshold is clamped at 3.9V
    // Maximum VINDPM threshold is clamped at 15.3V
    // When VBUS at noLoad is ≤ 6V, the VINDPM_OS is used to calculate VINDPM threhold
    // When VBUS at noLoad is > 6V, the VINDPM_OS multiple by 2 is used to calculate VINDPM threshold.
    void setInputVoltageLimitOffset(uint16_t millivolt)
    {
        if (millivolt % POWERS_BQ25896_IN_CURRENT_OFFSET_STEP) {
            log_e("Mistake ! The steps is must %u mA", POWERS_BQ25896_IN_CURRENT_OFFSET_STEP);
            return;
        }
        if (millivolt > POWERS_BQ25896_IN_CURRENT_OFFSET_MAX) {
            millivolt = POWERS_BQ25896_IN_CURRENT_OFFSET_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_01H);
        val &= 0xE0;
        millivolt = (millivolt  / POWERS_BQ25896_IN_CURRENT_OFFSET_STEP);
        val |=  millivolt;
        writeRegister(POWERS_PPM_REG_01H, val);
    }

    /***************************************************
     * POWERS_PPM_REG_02H  ✅
     **************************************************/
    bool enableMeasure(MeasureMode mode = CONTINUOUS)
    {
        int val = readRegister(POWERS_PPM_REG_02H);
        switch (mode) {
        case CONTINUOUS:
            val |= _BV(6);
            break;
        case ONE_SHORT:
            val &= (~_BV(6));
        default:
            break;
        }
        val |= _BV(7);
        return writeRegister(POWERS_PPM_REG_02H, val) != -1;
    }

    bool disableMeasure()
    {
        int val = readRegister(POWERS_PPM_REG_02H);
        if (val == -1) {
            return false;
        }
        val &= (~_BV(7));
        val &= (~_BV(6));
        return writeRegister(POWERS_PPM_REG_02H, val) != 1;
    }

    bool setBoostFreq(BoostFreq freq)
    {
        switch (freq) {
        case BOOST_FREQ_500KHZ:
            return setRegisterBit(POWERS_PPM_REG_02H, 5);
        case BOOST_FREQ_1500KHZ:
            return clrRegisterBit(POWERS_PPM_REG_02H, 5);
        default:
            break;
        }
        return false;
    }

    BoostFreq getBoostFreq()
    {
        return getRegisterBit(POWERS_PPM_REG_02H, 5) ?
               BOOST_FREQ_500KHZ :
               BOOST_FREQ_1500KHZ;
    }

    // Input Current Optimizer (ICO) Enable
    void enableInputCurrentOptimizer()
    {
        setRegisterBit(POWERS_PPM_REG_02H, 4);
    }

    // Input Current Optimizer (ICO) Disable
    void disableInputCurrentOptimizer()
    {
        clrRegisterBit(POWERS_PPM_REG_02H, 4);
    }

    // Enable Force Input Detection , Force PSEL detection
    void enableInputDetection()
    {
        setRegisterBit(POWERS_PPM_REG_02H, 1);
    }

    // Disable Force Input Detection ,  Not in PSEL detection (default)
    void disableInputDetection()
    {
        clrRegisterBit(POWERS_PPM_REG_02H, 1);
    }

    // Get Force DP/DM detection
    bool isEnableInputDetection()
    {
        return getRegisterBit(POWERS_PPM_REG_02H, 1);
    }

    // Enable PSEL detection when VBUS is plugged-in (default)
    void enableAutomaticInputDetection()
    {
        setRegisterBit(POWERS_PPM_REG_02H, 0);
    }

    // Disable PSEL detection when VBUS is plugged-in
    void disableAutomaticInputDetection()
    {
        clrRegisterBit(POWERS_PPM_REG_02H, 0);
    }

    // Get DPDM detection when BUS is plugged-in.
    bool isEnableAutomaticInputDetection()
    {
        return getRegisterBit(POWERS_PPM_REG_02H, 0);
    }


    /***************************************************
     * POWERS_PPM_REG_03H ✅
     **************************************************/

    // Return  Battery Load status
    bool isEnableBatLoad()
    {
        return getRegisterBit(POWERS_PPM_REG_03H, 7);
    }

    // Battery Load Disable
    void disableBatLoad()
    {
        clrRegisterBit(POWERS_PPM_REG_03H, 7);
    }

    // Battery Load Enable
    void enableBatLoad()
    {
        setRegisterBit(POWERS_PPM_REG_03H, 7);
    }

    void feedWatchdog()
    {
        setRegisterBit(POWERS_PPM_REG_03H, 6);
    }

    bool isEnableOTG()
    {
        return getRegisterBit(POWERS_PPM_REG_03H, 5);
    }

    void disableOTG()
    {
        clrRegisterBit(POWERS_PPM_REG_03H, 5);
        /*
        * After turning on the OTG function, the charging function will
        * be automatically disabled. If the user does not disable the charging
        * function, the charging function will be automatically enabled after
        * turning off the OTG output.
        * */
        if (!__user_disable_charge) {
            setRegisterBit(POWERS_PPM_REG_03H, 4);
        }
    }

    bool enableOTG()
    {
        if (isVbusIn())
            return false;
        return setRegisterBit(POWERS_PPM_REG_03H, 5);
    }

    bool isEnableCharge()
    {
        return getRegisterBit(POWERS_PPM_REG_03H, 4);
    }

    void disableCharge()
    {
        __user_disable_charge = true;
        clrRegisterBit(POWERS_PPM_REG_03H, 4);
    }

    void enableCharge()
    {
        __user_disable_charge = false;
        setRegisterBit(POWERS_PPM_REG_03H, 4);
    }

    bool setSysPowerDownVoltage(uint16_t millivolt)
    {
        if (millivolt % POWERS_BQ25896_SYS_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", POWERS_BQ25896_SYS_VOL_STEPS);
            return false;
        }
        if (millivolt < POWERS_BQ25896_SYS_VOFF_VOL_MIN) {
            log_e("Mistake ! SYS minimum output voltage is  %umV", POWERS_BQ25896_SYS_VOFF_VOL_MIN);
            return false;
        } else if (millivolt > POWERS_BQ25896_SYS_VOFF_VOL_MAX) {
            log_e("Mistake ! SYS maximum output voltage is  %umV", POWERS_BQ25896_SYS_VOFF_VOL_MAX);
            return false;
        }
        int val = readRegister(POWERS_PPM_REG_03H);
        if (val == -1)return false;
        val &= 0xF1;
        val |= (millivolt - POWERS_BQ25896_SYS_VOFF_VOL_MIN) / POWERS_BQ25896_SYS_VOL_STEPS;
        val <<= 1;
        return 0 ==  writeRegister(POWERS_PPM_REG_03H, val);

    }

    uint16_t getSysPowerDownVoltage()
    {
        int val = readRegister(POWERS_PPM_REG_03H);
        if (val == -1)return 0;
        val &= 0x0E;
        val >>= 1;
        return (val * POWERS_BQ25896_SYS_VOL_STEPS) + POWERS_BQ25896_SYS_VOFF_VOL_MIN;
    }

    // Minimum Battery Voltage (falling) to exit boost mode
    void setExitBoostModeVoltage(enum ExitBoostModeVolt params)
    {
        switch (params) {
        case MINI_VOLT_2V9:
            clrRegisterBit(POWERS_PPM_REG_03H, 0);
            break;
        case MINI_VOLT_2V5:
            setRegisterBit(POWERS_PPM_REG_03H, 0);
            break;
        default:
            break;
        }
    }

    /***************************************************
     * POWERS_PPM_REG_04H ✅
     **************************************************/

    void enableCurrentPulseControl()
    {
        setRegisterBit(POWERS_PPM_REG_04H, 7);
    }

    void disableCurrentPulseControl()
    {
        clrRegisterBit(POWERS_PPM_REG_04H, 7);
    }

    uint16_t getChargerConstantCurr()
    {
        int val = readRegister(POWERS_PPM_REG_04H);
        val &= 0x7F;
        return val * POWERS_BQ25896_FAST_CHG_CUR_STEP;
    }

    /**
     * @brief  setChargerConstantCurr
     * @note
     * @param  milliampere: Range:0~3008 mA / step:64mA
     * @retval true : success false : failed
     */
    bool setChargerConstantCurr(uint16_t milliampere)
    {
        if (milliampere % POWERS_BQ25896_FAST_CHG_CUR_STEP) {
            log_e("Mistake ! The steps is must %u mA", POWERS_BQ25896_FAST_CHG_CUR_STEP);
            return false;
        }
        if (milliampere > POWERS_BQ25896_FAST_CHG_CURRENT_MAX) {
            milliampere = POWERS_BQ25896_FAST_CHG_CURRENT_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_04H);
        val &= 0x80;
        val |= (milliampere / POWERS_BQ25896_FAST_CHG_CUR_STEP);
        return  writeRegister(POWERS_PPM_REG_04H, val) != -1;
    }

    /***************************************************
     * POWERS_PPM_REG_05H ✅
     **************************************************/

    //Precharge Current Limit Range: 64mA ~ 1024mA ,step:64mA
    bool setPrechargeCurr(uint16_t milliampere)
    {
        if (milliampere % POWERS_BQ25896_PRE_CHG_CUR_STEP) {
            log_e("Mistake ! The steps is must %u mA", POWERS_BQ25896_PRE_CHG_CUR_STEP);
            return false;
        }
        if (milliampere < POWERS_BQ25896_PRE_CHG_CURRENT_MIN) {
            milliampere = POWERS_BQ25896_PRE_CHG_CURRENT_MIN;
        }
        if (milliampere > POWERS_BQ25896_PRE_CHG_CURRENT_MAX) {
            milliampere = POWERS_BQ25896_PRE_CHG_CURRENT_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_05H);
        val &= 0x0F;
        milliampere = ((milliampere - POWERS_BQ25896_PRE_CHG_CUR_BASE) / POWERS_BQ25896_PRE_CHG_CUR_STEP);
        val |=  milliampere << 4;
        return writeRegister(POWERS_PPM_REG_05H, val) != -1;
    }

    uint16_t getPrechargeCurr(void)
    {
        int val = readRegister(POWERS_PPM_REG_05H);
        val &= 0xF0;
        val >>= 4;
        return POWERS_BQ25896_PRE_CHG_CUR_STEP + (val * POWERS_BQ25896_PRE_CHG_CUR_STEP);
    }

    //Precharge Current Limit Range: 64mA ~ 1024mA ,step:64mA
    bool setTerminationCurr(uint16_t milliampere)
    {
        if (milliampere % POWERS_BQ25896_TERM_CHG_CUR_STEP) {
            log_e("Mistake ! The steps is must %u mA", POWERS_BQ25896_TERM_CHG_CUR_STEP);
            return false;
        }
        if (milliampere < POWERS_BQ25896_TERM_CHG_CURRENT_MIN) {
            milliampere = POWERS_BQ25896_TERM_CHG_CURRENT_MIN;
        }
        if (milliampere > POWERS_BQ25896_TERM_CHG_CURRENT_MAX) {
            milliampere = POWERS_BQ25896_TERM_CHG_CURRENT_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_05H);
        val &= 0xF0;
        milliampere = ((milliampere - POWERS_BQ25896_TERM_CHG_CUR_BASE) / POWERS_BQ25896_TERM_CHG_CUR_STEP);
        val |=  milliampere;
        return writeRegister(POWERS_PPM_REG_05H, val) != -1;
    }

    uint16_t getTerminationCurr(void)
    {
        int val = readRegister(POWERS_PPM_REG_05H);
        val &= 0x0F;
        return POWERS_BQ25896_TERM_CHG_CUR_STEP + (val * POWERS_BQ25896_TERM_CHG_CUR_STEP);
    }


    /***************************************************
     * POWERS_PPM_REG_06H ✅
     **************************************************/
    uint16_t getChargeTargetVoltage()
    {
        int val = readRegister(POWERS_PPM_REG_06H);
        val = (val & 0xFC) >> 2;
        if (val > 0x30) {
            return POWERS_BQ25896_FAST_CHG_VOL_MAX;
        }
        return val * POWERS_BQ25896_CHG_VOL_STEP + POWERS_BQ25896_CHG_VOL_BASE;
    }

    // Charge Voltage Limit Range:3840 ~ 4608mV ,step:16 mV
    bool setChargeTargetVoltage(uint16_t millivolt)
    {
        if (millivolt % POWERS_BQ25896_CHG_VOL_STEP) {
            log_e("Mistake ! The steps is must %u mV", POWERS_BQ25896_CHG_VOL_STEP);
            return false;
        }
        if (millivolt < POWERS_BQ25896_FAST_CHG_VOL_MIN) {
            millivolt = POWERS_BQ25896_FAST_CHG_VOL_MIN;
        }
        if (millivolt > POWERS_BQ25896_FAST_CHG_VOL_MAX) {
            millivolt = POWERS_BQ25896_FAST_CHG_VOL_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_06H);
        val &= 0x03;
        val |= (((millivolt - POWERS_BQ25896_CHG_VOL_BASE) / POWERS_BQ25896_CHG_VOL_STEP) << 2);
        return  writeRegister(POWERS_PPM_REG_06H, val) != -1;
    }

    // Battery Precharge to Fast Charge Threshold
    void setFastChargeThreshold(enum FastChargeThreshold  threshold)
    {
        switch (threshold) {
        case FAST_CHG_THR_2V8:
            clrRegisterBit(POWERS_PPM_REG_06H, 1);
            break;
        case FAST_CHG_THR_3V0:
            setRegisterBit(POWERS_PPM_REG_06H, 1);
            break;
        default:
            break;
        }
    }

    // Battery Recharge Threshold Offset(below Charge Voltage Limit)
    void setBatteryRechargeThresholdOffset(enum RechargeThresholdOffset offset)
    {
        switch (offset) {
        case RECHARGE_OFFSET_100MV:
            clrRegisterBit(POWERS_PPM_REG_06H, 0);
            break;
        case RECHARGE_OFFSET_200MV:
            setRegisterBit(POWERS_PPM_REG_06H, 0);
            break;
        default:
            break;
        }
    }

    /***************************************************
     * POWERS_PPM_REG_07H ✅
     **************************************************/

    // Charging Termination Enable
    void enableChargingTermination()
    {
        setRegisterBit(POWERS_PPM_REG_07H, 7);
    }

    // Charging Termination Enable
    void disableChargingTermination()
    {
        clrRegisterBit(POWERS_PPM_REG_07H, 7);
    }

    // Charging Termination Enable
    bool isEnableChargingTermination()
    {
        return getRegisterBit(POWERS_PPM_REG_07H, 7);
    }

    // STAT Pin function
    void disableStatPin()
    {
        setRegisterBit(POWERS_PPM_REG_07H, 6);
    }

    void enableStatPin()
    {
        clrRegisterBit(POWERS_PPM_REG_07H, 6);
    }

    bool isEnableStatPin()
    {
        return getRegisterBit(POWERS_PPM_REG_07H, 6) == false;
    }

    // I2C Watchdog Timer Setting
    bool isEnableWatchdog()
    {
        int regVal = readRegister(POWERS_PPM_REG_07H);
        if (regVal == -1) {
            log_e("Config watch dog failed!");
            return false;
        }
        regVal >>= 4;
        return regVal & 0x03;
    }

    void disableWatchdog()
    {
        int regVal = readRegister(POWERS_PPM_REG_07H);
        regVal  &= 0xCF;
        writeRegister(POWERS_PPM_REG_07H, regVal);
    }

    void enableWatchdog(enum WatchdogConfig val)
    {
        int regVal = readRegister(POWERS_PPM_REG_07H);
        regVal  &= 0xCF;
        switch (val) {
        case TIMER_OUT_40SEC:
            writeRegister(POWERS_PPM_REG_07H, regVal | 0x10);
            break;
        case TIMER_OUT_80SEC:
            writeRegister(POWERS_PPM_REG_07H, regVal | 0x20);
            break;
        case TIMER_OUT_160SEC:
            writeRegister(POWERS_PPM_REG_07H, regVal | 0x30);
            break;
        default:
            break;
        }
    }

    void disableChargingSafetyTimer()
    {
        clrRegisterBit(POWERS_PPM_REG_07H, 3);
    }

    void enableChargingSafetyTimer()
    {
        setRegisterBit(POWERS_PPM_REG_07H, 3);
    }

    bool isEnableChargingSafetyTimer()
    {
        return getRegisterBit(POWERS_PPM_REG_07H, 3);
    }

    void setFastChargeTimer(FastChargeTimer timer)
    {
        int val;
        switch (timer) {
        case FAST_CHARGE_TIMER_5H:
        case FAST_CHARGE_TIMER_8H:
        case FAST_CHARGE_TIMER_12H:
        case FAST_CHARGE_TIMER_20H:
            val = readRegister(POWERS_PPM_REG_07H);
            if (val == -1)
                return;
            val &= 0xF1;
            val |= (timer << 1);
            writeRegister(POWERS_PPM_REG_07H, val);
            break;
        default:
            break;
        }
    }

    FastChargeTimer getFastChargeTimer()
    {
        int val = readRegister(POWERS_PPM_REG_07H);
        return static_cast<FastChargeTimer>((val & 0x0E) >> 1);
    }

    // JEITA Low Temperature Current Setting
    // JEITA（Japan Electronics and Information Technology Industries Association）
    // https://en.wikipedia.org/wiki/Japan_Electronics_and_Information_Technology_Industries_Association
    void setJeitaLowTemperatureCurrent(enum JeitaLowTemperatureCurrent params)
    {
        switch (params) {
        case JEITA_LOW_TEMP_50:
            clrRegisterBit(POWERS_PPM_REG_07H, 0);
            break;
        case JEITA_LOW_TEMP_20:
            setRegisterBit(POWERS_PPM_REG_07H, 0);
            break;
        default:
            break;
        }
    }

    /***************************************************
     * POWERS_PPM_REG_08H ✅
     **************************************************/
    // IR Compensation Resistor Setting
    // Range: 0 – 140mΩ
    // Default: 0Ω (000) (i.e. Disable IRComp)
    void setIRCompensationResistor(uint16_t params)
    {
        if (params % POWERS_BQ25896_BAT_COMP_STEPS) {
            log_e("Mistake ! The steps is must %u mA", POWERS_BQ25896_BAT_COMP_STEPS);
            return;
        }
        if (params > POWERS_BQ25896_TERM_CHG_CURRENT_MAX) {
            params = POWERS_BQ25896_TERM_CHG_CURRENT_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_08H);
        if (val == -1)return;
        val &= 0x1F;
        params = (params  / POWERS_BQ25896_BAT_COMP_STEPS);
        val |=  (params << 5);
        writeRegister(POWERS_PPM_REG_08H, val);
    }

    // IR Compensation Voltage Clamp
    // above VREG (REG06[7:2])
    // Offset: 0mV
    // Range: 0-224mV
    // Default: 0mV (000)
    void setIRCompensationVoltageClamp(uint16_t params)
    {
        if (params % POWERS_BQ25896_VCLAMP_STEPS) {
            log_e("Mistake ! The steps is must %u mA", POWERS_BQ25896_VCLAMP_STEPS);
            return;
        }
        if (params > POWERS_BQ25896_TERM_CHG_CURRENT_MAX) {
            params = POWERS_BQ25896_TERM_CHG_CURRENT_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_08H);
        if (val == -1)return;
        val &= 0xE3;
        params = (params  / POWERS_BQ25896_VCLAMP_STEPS);
        val |=  (params << 2);
        writeRegister(POWERS_PPM_REG_08H, val);
    }

    // Thermal Regulation Threshold
    // 0x0 – 60°C
    // 0x1 – 80°C
    // 0x2 – 100°C
    // 0x3 – 120°C (default)
    void setThermalRegulationThreshold(uint8_t params)
    {
        int val = readRegister(POWERS_PPM_REG_08H);
        if (val == -1)return;
        val &= 0xE3;
        val |=  (params);
        writeRegister(POWERS_PPM_REG_08H, val);
    }

    /***************************************************
     * POWERS_PPM_REG_09H
     **************************************************/
    // Force Start Input Current Optimizer (ICO)
    // 0 – Do not force ICO (default)
    // 1 – Force ICO
    // Note: This bit is can only be set only and always returns to 0 after ICO starts
    void forceInputCurrentOptimizer(bool force)
    {
        force ? setRegisterBit(POWERS_PPM_REG_09H, 7) : clrRegisterBit(POWERS_PPM_REG_09H, 7);
    }

    // Safety Timer Setting during DPM or Thermal Regulation
    // 0 – Safety timer not slowed by 2X during input DPM or thermal regulation
    // 1 – Safety timer slowed by 2X during input DPM or thermal regulation (default)
    void setThermalRegulation(uint8_t params)
    {
        params ? setRegisterBit(POWERS_PPM_REG_09H, 6) : clrRegisterBit(POWERS_PPM_REG_09H, 6) ;
    }


    // Turn off the battery power supply path. It can only be turned off when the
    // battery is powered. It cannot be turned off when USB is connected.
    // The device can only be powered on by pressing the PWR button or by connecting the power supply.
    void shutdown()
    {
        disableBatterPowerPath();
    }

    // Close battery power path
    void disableBatterPowerPath()
    {
        setRegisterBit(POWERS_PPM_REG_09H, 5);       //Force BATFET Off : BATFET_DIS
    }

    // Enable battery power path
    void enableBatterPowerPath()
    {
        clrRegisterBit(POWERS_PPM_REG_09H, 5);       //Force BATFET Off : BATFET_DIS
    }

    // JEITA High Temperature Voltage Setting
    // JEITA（Japan Electronics and Information Technology Industries Association）
    // https://en.wikipedia.org/wiki/Japan_Electronics_and_Information_Technology_Industries_Association
    // 0 – Set Charge Voltage to VREG-200mV during JEITA hig temperature(default)
    // 1 – Set Charge Voltage to VREG during JEITA high temperature
    void setJeitaHighTemperature(uint8_t params)
    {
        params ? setRegisterBit(POWERS_PPM_REG_09H, 4) : clrRegisterBit(POWERS_PPM_REG_09H, 4) ;
    }

    // BATFET turn off delay control
    // 0 – BATFET turn off immediately when BATFET_DIS bit is set (default)
    // 1 – BATFET turn off delay by tSM_DLY when BATFET_DIS bit is set
    void setTurnOffDelay(uint8_t params)
    {
        params ? setRegisterBit(POWERS_PPM_REG_09H, 3) : clrRegisterBit(POWERS_PPM_REG_09H, 3) ;
    }

    // BATFET full system reset enable
    // 0 – Disable BATFET full system reset
    // 1 – Enable BATFET full system reset (default)
    void setFullSystemReset(uint8_t params)
    {
        params ? setRegisterBit(POWERS_PPM_REG_09H, 2) : clrRegisterBit(POWERS_PPM_REG_09H, 2) ;
    }

    // Current pulse control voltage up enable
    // 0 – Disable (default)
    // 1 – Enable
    // Note: This bit is can only be set when EN_PUMPX bit is set and returns to 0 after current pulse control sequence is completed
    void setCurrentPulseControlVoltageUp(uint8_t params)
    {
        params ? setRegisterBit(POWERS_PPM_REG_09H, 1) : clrRegisterBit(POWERS_PPM_REG_09H, 1) ;
    }

    // Current pulse control voltage down enable
    // 0 – Disable (default)
    // 1 – Enable
    // Note: This bit is can only be set when EN_PUMPX bit is set and returns to 0 after current pulse control sequence is completed
    void setCurrentPulseControlVoltageDown(uint8_t params)
    {
        params ? setRegisterBit(POWERS_PPM_REG_09H, 0) : clrRegisterBit(POWERS_PPM_REG_09H, 0) ;
    }

    /***************************************************
     * POWERS_PPM_REG_0AH ✅
     **************************************************/

    // Boost Mode Voltage Regulation: 4550mV ~ 5510mV
    bool setBoostVoltage(uint16_t millivolt)
    {
        if (millivolt % POWERS_BQ25896_BOOTS_VOL_STEP) {
            log_e("Mistake ! The steps is must %u mV", POWERS_BQ25896_BOOTS_VOL_STEP);
            return false;
        }
        if (millivolt < POWERS_BQ25896_BOOST_VOL_MIN) {
            millivolt = POWERS_BQ25896_BOOST_VOL_MIN;
        }
        if (millivolt > POWERS_BQ25896_BOOST_VOL_MAX) {
            millivolt = POWERS_BQ25896_BOOST_VOL_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_0AH);
        val &= 0xF0;
        val |= (((millivolt - POWERS_BQ25896_BOOTS_VOL_BASE) / POWERS_BQ25896_BOOTS_VOL_STEP) << 4);
        return  writeRegister(POWERS_PPM_REG_0AH, val) != -1;
    }

    // Boost Current Limit: 500mA ~ 150 mA
    bool setBoostCurrentLimit(BoostCurrentLimit milliampere)
    {
        if (milliampere > BOOST_CUR_LIMIT_2150MA) {
            return false;
        }
        int val = readRegister(POWERS_PPM_REG_0AH);
        val &= 0x03;
        val |= milliampere;
        return  writeRegister(POWERS_PPM_REG_0AH, val) != -1;
    }

    // PFM mode allowed in boost mode
    // 0 – Allow PFM in boost mode (default)
    // 1 – Disable PFM in boost mode
    void setBoostModeUsePFM(bool enable)
    {
        enable ? clrRegisterBit(POWERS_PPM_REG_0AH, 3) : setRegisterBit(POWERS_PPM_REG_0AH, 3);
    }


    /***************************************************
     * POWERS_PPM_REG_0BH ✅
     **************************************************/

    bool isOTG()
    {
        return getBusStatus() == BUS_STATE_OTG;
    }

    bool isCharging(void)
    {
        return chargeStatus() != CHARGE_STATE_NO_CHARGE;
    }

    bool isChargeDone()
    {
        return chargeStatus() == CHARGE_STATE_DONE;
    }

    bool isPowerGood()
    {
        return getRegisterBit(POWERS_PPM_REG_0BH, 2);
    }

    BusStatus getBusStatus()
    {
        int val =  readRegister(POWERS_PPM_REG_0BH);
        return static_cast<BusStatus>((val >> 5) & 0x07);
    }

    const char *getBusStatusString()
    {
        BusStatus status = getBusStatus();
        switch (status) {
        case BUS_STATE_NOINPUT:
            return "No input";
        case BUS_STATE_USB_SDP:
            return "USB Host SDP";
        case BUS_STATE_ADAPTER:
            return "Adapter";
        case BUS_STATE_OTG:
            return "OTG";
        default:
            return "Unknown";
        }
    }

    ChargeStatus chargeStatus()
    {
        int val =  readRegister(POWERS_PPM_REG_0BH);
        if (val == -1)return CHARGE_STATE_UNKOWN;
        return static_cast<ChargeStatus>((val >> 3) & 0x03);
    }

    const char *getChargeStatusString()
    {
        ChargeStatus status = chargeStatus();
        switch (status) {
        case CHARGE_STATE_NO_CHARGE:
            return "Not Charging";
        case CHARGE_STATE_PRE_CHARGE:
            return "Pre-charge";
        case CHARGE_STATE_FAST_CHARGE:
            return "Fast Charging";
        case CHARGE_STATE_DONE:
            return "Charge Termination Done";
        default:
            return "Unknown";
        }
    }

    // VSYS Regulation Status
    // 0 – Not in VSYSMIN regulation (BAT > VSYSMIN)
    // 1 – In VSYSMIN regulation (BAT < VSYSMIN)
    bool getVsysRegulationStatus()
    {
        return getRegisterBit(POWERS_PPM_REG_0BH, 0);
    }

    const char *getVsysRegulationStatusString()
    {
        if (getVsysRegulationStatus()) {
            return "BAT < VSYSMIN";
        }
        return "BAT > VSYSMIN";
    }

    /***************************************************
     * POWERS_PPM_REG_0CH ✅
     **************************************************/

    // After reading the register, all will be cleared
    uint8_t getFaultStatus(void)
    {
        int val = readRegister(POWERS_PPM_REG_0CH);
        if (val == -1) {
            return 0;
        }
        __irq_mask = val;
        return __irq_mask;
    }

    // Watchdog Fault Status
    // 0 – Normal
    // 1- Watchdog timer expiration
    bool isWatchdogFault()
    {
        return POWERS_BQ25896_IRQ_WTD_FAULT(__irq_mask);
    }

    // Boost Mode Fault Status
    // 0 – Normal
    // 1 – VBUS overloaded in OTG, or VBUS OVP, or battery is too low in boost mode
    bool isBoostFault()
    {
        return POWERS_BQ25896_IRQ_BOOST_FAULT(__irq_mask);
    }

    // Charge Fault Status
    // 00 – Normal
    // 01 – Input fault (VBUS > VACOV or VBAT < VBUS < VVBUSMIN(typical 3.8V)
    // 10 - Thermal shutdown
    // 11 – Charge Safety Timer Expiration
    uint8_t isChargeFault()
    {
        return POWERS_BQ25896_IRQ_CHG_FAULT(__irq_mask);
    }

    // Battery Fault Status
    // 0 – Normal
    // 1 – BATOVP (VBAT > VBATOVP)
    bool isBatteryFault()
    {
        return POWERS_BQ25896_IRQ_BAT_FAULT(__irq_mask);
    }

    // NTC Fault Status
    bool isNTCFault()
    {
        return POWERS_BQ25896_IRQ_NTC_FAULT(__irq_mask);
    }

    // NTC Fault Status string
    uint8_t getNTCStatus()
    {
        return (__irq_mask & 0x07);
    }

    const char *getNTCStatusString()
    {
        uint8_t status = getNTCStatus();
        if (isOTG()) {
            // Boost mode
            switch (status) {
            case BOOST_NTC_NORMAL:
                return "Boost mode NTC normal";
            case BOOST_NTC_COLD:
                return "Boost mode NTC cold";
            case BOOST_NTC_HOT:
                return "Boost mode NTC hot";
            default:
                break;
            }
        } else {
            // Buck mode
            switch (status) {
            case BUCK_NTC_NORMAL:
                return "Buck mode NTC normal";
            case BUCK_NTC_WARM:
                return "Buck mode NTC warm";
            case BUCK_NTC_COOL:
            case BUCK_NTC_COLD:
                return "Buck mode NTC cold";
            case BUCK_NTC_HOT:
                return "Buck mode NTC hot";
            default:
                break;
            }
        }
        return "Unknown";
    }

    // Debug
    void getReadOnlyRegisterValue()
    {
#ifdef ARDUINO //debug ..
        static uint8_t last_val[8] = {0};
        const uint8_t regis[] = {
            POWERS_PPM_REG_0BH,
            POWERS_PPM_REG_0CH,
            // POWERS_PPM_REG_0EH, //BATTERY VOLTAGE
            // POWERS_PPM_REG_0FH, //SYSTEM VOLTAGE
            // POWERS_PPM_REG_10H, //NTC PERCENTAGE
            // POWERS_PPM_REG_11H, //VBUS VOLTAGE
            POWERS_PPM_REG_12H,
            POWERS_PPM_REG_13H
        };
        Serial.println();
        Serial.println("-------------------------");
        for (uint32_t i = 0; i < sizeof(regis) / sizeof(regis[0]); ++i) {
            int val = readRegister(regis[i]);
            if (val == -1) {
                continue;
            }
            if (last_val[i] != val) {
                Serial.printf("\t---> REG%02X Prev:0x%02X ", regis[i], last_val[i]);
                Serial.print(" BIN:"); Serial.print(last_val[i], BIN);
                Serial.printf(" Curr: 0x%02X", val);
                Serial.print(" BIN:"); Serial.println(val, BIN);
                last_val[i] = val;
            }
            Serial.printf("\tREG%02XH:0x%X BIN:0b", regis[i], val);
            Serial.println(val, BIN);
        }
        Serial.println("-------------------------");
#endif
    }

    /***************************************************
     * POWERS_PPM_REG_0DH ✅
     **************************************************/
    // VINDPM Threshold Setting Method
    // 0 – Run Relative VINDPM Threshold (default)
    // 1 – Run Absolute VINDPM Threshold
    // Note: Register is reset to default value when input source is plugged-in
    void setVinDpmThresholdSetting(bool relative)
    {
        relative ? clrRegisterBit(POWERS_PPM_REG_0DH, 7) : setRegisterBit(POWERS_PPM_REG_0DH, 7);
    }

    // Absolute VINDPM Threshold
    bool setVinDpmThreshold(uint16_t millivolt)
    {
        if (millivolt % POWERS_BQ25896_VINDPM_VOL_STEPS) {
            log_e("Mistake ! The steps is must %u mV", POWERS_BQ25896_VINDPM_VOL_STEPS);
            return false;
        }
        if (millivolt < POWERS_BQ25896_VINDPM_VOL_MIN) {
            millivolt = POWERS_BQ25896_VINDPM_VOL_MIN;
        }
        if (millivolt > POWERS_BQ25896_VINDPM_VOL_MAX) {
            millivolt = POWERS_BQ25896_VINDPM_VOL_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_0DH);
        val &= 0x80;
        val |= (((millivolt - POWERS_BQ25896_VINDPM_VOL_BASE) / POWERS_BQ25896_VINDPM_VOL_STEPS));
        return  writeRegister(POWERS_PPM_REG_0DH, val) != -1;
    }

    /***************************************************
     * POWERS_PPM_REG_0EH ✅
     **************************************************/
    // Thermal Regulation Status
    // true – Normal
    // false – In Thermal Regulation
    bool isThermalRegulationNormal()
    {
        return getRegisterBit(POWERS_PPM_REG_0EH, 7) == false;
    }

    // ADC conversion of Battery Voltage /mv
    uint16_t getBattVoltage()
    {
        int val = readRegister(POWERS_PPM_REG_0EH);
        if (val == -1)return 0;
        val = POWERS_BQ25896_VBAT_MASK_VAL(val);
        if (val == 0)return 0;
        return (val * POWERS_BQ25896_VBAT_VOL_STEP) + POWERS_BQ25896_VBAT_BASE_VAL;
    }

    /***************************************************
     * POWERS_PPM_REG_0FH ✅
     **************************************************/

    // ADC conversion of System Voltage (VSYS)
    uint16_t getSystemVoltage()
    {
        int val = readRegister(POWERS_PPM_REG_0FH);
        if (val == -1 || val == 0)return 0;
        return (POWERS_BQ25896_VSYS_MASK_VAL(val) * POWERS_BQ25896_VSYS_VOL_STEP) + POWERS_BQ25896_VSYS_BASE_VAL;
    }

    /***************************************************
    * POWERS_PPM_REG_10H ✅
    **************************************************/

    // ADC conversion of TS Voltage (TS) as percentage of REGN
    float getNTCPercentage()
    {
        int val = readRegister(POWERS_PPM_REG_10H);
        if (val == -1)return 0;
        return (POWERS_BQ25896_NTC_MASK_VAL(val) * POWERS_BQ25896_NTC_VOL_STEP) + POWERS_BQ25896_NTC_BASE_VAL;
    }

    /***************************************************
     * POWERS_PPM_REG_11H ✅
     **************************************************/

    // VBUS Good Status
    bool isVbusIn()
    {
        return getRegisterBit(POWERS_PPM_REG_11H, 7);
    }

    // ADC conversion of VBUS voltage (VBUS)
    uint16_t getVbusVoltage()
    {
        if (!isVbusIn()) {
            return 0;
        }
        int val = readRegister(POWERS_PPM_REG_11H);
        return (POWERS_BQ25896_VBUS_MASK_VAL(val) * POWERS_BQ25896_VBUS_VOL_STEP) + POWERS_BQ25896_VBUS_BASE_VAL;
    }

    /***************************************************
     * POWERS_PPM_REG_12H ✅
     **************************************************/

    // ADC conversion of Charge Current (IBAT) when VBAT > VBATSHORT
    //* If the charger is disconnected, the value in the register
    //* will remain the last value and will not be updated to 0.
    uint16_t getChargeCurrent()
    {
        ChargeStatus status = chargeStatus();
        if (status == CHARGE_STATE_NO_CHARGE) {
            log_e("CHARGE_STATE_NO_CHARGE...");
            return 0;
        }
        int val = readRegister(POWERS_PPM_REG_12H);
        if (val == 0 || val == -1) {
            log_e("read reg failed !...");
            return 0;
        }
        val = (val & 0x7F);
        return (val * POWERS_BQ25896_CHG_STEP_VAL) ;
    }

    /***************************************************
     * POWERS_PPM_REG_13H ✅
     **************************************************/
    // VINDPM Status : DynamicPower-Path Management and Dynamic Power Management
    bool isDynamicPowerManagement()
    {
        return getRegisterBit(POWERS_PPM_REG_13H, 7);
    }

    // IINDPM Status
    bool isInputCurrentLimit()
    {
        return getRegisterBit(POWERS_PPM_REG_13H, 6);
    }

    // Input Current Limit in effect while Input Current Optimizer (ICO) is enabled
    // Range: 100 ~ 3250 mA
    bool setInputCurrentLimitOptimizer(uint16_t milliampere)
    {
        if (milliampere % POWERS_BQ25896_IN_CURRENT_OPT_STEP) {
            log_e("Mistake ! The steps is must %u mA", POWERS_BQ25896_IN_CURRENT_OPT_STEP);
            return false;
        }
        if (milliampere < POWERS_BQ25896_IN_CURRENT_OPT_MIN) {
            milliampere = POWERS_BQ25896_IN_CURRENT_OPT_MIN;
        }
        if (milliampere > POWERS_BQ25896_IN_CURRENT_OPT_MAX) {
            milliampere = POWERS_BQ25896_IN_CURRENT_OPT_MAX;
        }
        int val = readRegister(POWERS_PPM_REG_13H);
        if (val == -1)
            return false;
        val &= 0x3F;
        milliampere = ((milliampere - POWERS_BQ25896_IN_CURRENT_OPT_MIN) / POWERS_BQ25896_IN_CURRENT_STEP);
        val |=  milliampere;
        return writeRegister(POWERS_PPM_REG_13H, val) != -1;
    }

    /***************************************************
     * POWERS_PPM_REG_14H ✅
     **************************************************/
    void resetDefault()
    {
        setRegisterBit(POWERS_PPM_REG_14H, 7);
    }

    // Input Current Optimizer (ICO) Status
    // true – Optimization is in progress
    // false – Maximum Input Current Detected
    bool isInputCurrentOptimizer()
    {
        return getRegisterBit(POWERS_PPM_REG_14H, 6) ;
    }

    // Device Revision:Default 10
    uint8_t getChipID()
    {
        int val = readRegister(POWERS_PPM_REG_14H);
        if (val == -1)return 0;
        return (val & 0x03);
    }

    // Device Configuration
    uint8_t getDeviceConfig()
    {
        int val = readRegister(POWERS_PPM_REG_14H);
        if (val == -1)return 0;
        return (val >> 3) & 0x03;
    }

private:

    bool initImpl()
    {
        __user_disable_charge = false;

        uint8_t rev = getChipID();
        if (rev != BQ25896_DEV_REV) {
            return false;
        }
        // Set the minimum operating voltage. Below this voltage, the PMU will protect
        // setSysPowerDownVoltage(3300);

        //Default disable Watchdog
        disableWatchdog();

        return true;
    }

    bool __user_disable_charge;
    uint32_t __irq_mask;
};

