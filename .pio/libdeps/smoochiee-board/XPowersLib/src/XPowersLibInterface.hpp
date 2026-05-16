/**
 *
 * @license MIT License
 *
 * Copyright (c) 2022 lewis he
 *
 * Permission is hereby granted,free of charge,to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),to deal
 * in the Software without restriction,including without limitation the rights
 * to use,copy,modify,merge,publish,distribute,sublicense,and/or sell
 * copies of the Software,and to permit persons to whom the Software is
 * furnished to do so,subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS",WITHOUT WARRANTY OF ANY KIND,EXPRESS OR
 * IMPLIED,INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,DAMAGES OR OTHER
 * LIABILITY,WHETHER IN AN ACTION OF CONTRACT,TORT OR OTHERWISE,ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file      XPowersLibInterface.hpp
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2022-08-28
 *
 */
#pragma once

#include <stdint.h>
#include "XPowersParams.hpp"

/*
| CHIP       | AXP173            | AXP192            | AXP202            | AXP2101                                |
| ---------- | ----------------- | ----------------- | ----------------- | -------------------------------------- |
| DC1        | 0.7V-3.5V /1.2A   | 0.7V-3.5V  /1.2A  | X                 | 1.5-3.4V                        /2A    |
| DC2        | 0.7-2.275V/0.6A   | 0.7-2.275V /1.6A  | 0.7-2.275V /1.6A  | 0.5-1.2V,1.22-1.54V             /2A    |
| DC3        | X                 | 0.7-3.5V   /0.7A  | 0.7-3.5V   /1.2A  | 0.5-1.2V,1.22-1.54V,1.6-3.4V    /2A    |
| DC4        | X                 | x                 | x                 | 0.5-1.2V,1.22-1.84V             /1.5A   |
| DC5        | X                 | x                 | x                 | 1.2V,1.4-3.7V                   /1A    |
| LDO1(VRTC) | 3.3V       /30mA  | 3.3V       /30mA  | 3.3V       /30mA  | 1.8V                            /30mA  |
| LDO2       | 1.8V-3.3V  /200mA | 1.8V-3.3V  /200mA | 1.8V-3.3V  /200mA | x                                      |
| LDO3       | 1.8V-3.3V  /200mA | 1.8-3.3V   /200mA | 0.7-3.5V   /200mA | x                                      |
| LDO4       | 0.7-3.5V   /500mA | X                 | 1.8V-3.3V  /200mA | x                                      |
| LDO5/IO0   | X                 | 1.8-3.3V   /50mA  | 1.8-3.3V   /50mA  | x                                      |
| ALDO1      | x                 | x                 | x                 | 0.5-3.5V                        /300mA |
| ALDO2      | x                 | x                 | x                 | 0.5-3.5V                        /300mA |
| ALDO3      | x                 | x                 | x                 | 0.5-3.5V                        /300mA |
| ALDO4      | x                 | x                 | x                 | 0.5-3.5V                        /300mA |
| BLDO1      | x                 | x                 | x                 | 0.5-3.5V                        /300mA |
| BLDO2      | x                 | x                 | x                 | 0.5-3.5V                        /300mA |
| DLDO1      | x                 | x                 | x                 | 0.5-3.3V/ 0.5-1.4V              /300mA |
| DLDO1      | x                 | x                 | x                 | 0.5-3.3V/ 0.5-1.4V              /300mA |
| CPUSLDO    | x                 | x                 | x                 | 0.5-1.4V                        /30mA  |
|            |                   |                   |                   |                                        |
*/



// @brief Each chip resource is different,please refer to the table above
typedef enum __XPowersPowerChannel {

    XPOWERS_DCDC1,
    XPOWERS_DCDC2,
    XPOWERS_DCDC3,
    XPOWERS_DCDC4,
    XPOWERS_DCDC5,

    XPOWERS_LDO1,
    XPOWERS_LDO2,
    XPOWERS_LDO3,
    XPOWERS_LDO4,
    XPOWERS_LDO5,

    XPOWERS_LDOIO,

    XPOWERS_ALDO1,
    XPOWERS_ALDO2,
    XPOWERS_ALDO3,
    XPOWERS_ALDO4,

    XPOWERS_BLDO1,
    XPOWERS_BLDO2,

    XPOWERS_DLDO1,
    XPOWERS_DLDO2,

    XPOWERS_VBACKUP,

    XPOWERS_CPULDO,

} XPowersPowerChannel_t;

// @brief Chip type
typedef enum __XPowersChipModel {
    XPOWERS_AXP173,
    XPOWERS_AXP192,
    XPOWERS_AXP202,
    XPOWERS_AXP216,
    XPOWERS_AXP2101,
    XPOWERS_UNDEFINED,
} XPowersChipModel_t;


/**
 * @brief  Compatible with subclasses of the Meshtastic-devic project
 */
class HasBatteryLevel
{
public:
    /**
    * @brief  Get battery percentage
    * @retval 0~100% , -1 no battery is connected
    */
    virtual int getBatteryPercent()
    {
        return -1;
    }

    /**
    * @brief  Get battery Voltage
    * @retval Voltage unit: millivolt , 0 is no battery is connected
    */
    virtual uint16_t getBattVoltage()
    {
        return 0;
    }

    /**
     * @brief Query whether the current battery is connected
     * @retval true to access,false to not access
     */
    virtual bool isBatteryConnect()
    {
        return false;
    }

    /**
     * @brief Query whether the current USB is connected
     * @retval true to access,false to not access
     */
    virtual bool isVbusIn()
    {
        return false;
    }

    /**
    * @brief Query whether it is currently in charging state
    * @retval true to charge,false to not charge
    */
    virtual bool isCharging()
    {
        return false;
    }
};

// @brief Power resource interface class
class XPowersLibInterface : public HasBatteryLevel
{
public:

    XPowersLibInterface() : __chipModel(XPOWERS_UNDEFINED), __protectedMask(0) {};

    virtual ~XPowersLibInterface() {}

    /**
     * @brief  Calling the XPowersLibInterface interface class
     *         requires calling init for initialization
     * @retval
     */
    virtual bool init() = 0;

    /**
     * @brief  When calling the XPowersLibInterface interface class,
     *         calling deinit releases the Wire handle
     * @retval None
     */
    virtual void deinit() = 0;


    /**
     * @brief  Set the PMU sleep flag,
     *         need to manually close the power channel after setting
     * @retval true success false failed
     */
    virtual bool enableSleep() = 0;


    /**
     * @brief  Set shutdown, calling shutdown will turn off all power channels,
     *         only VRTC belongs to normal power supply
     * @retval None
     */
    virtual void shutdown() = 0;


    /**
     * @brief Get PMU status register
     * @note
     * @retval register value
     */
    virtual uint16_t status() = 0;


    /**
     * @brief Query chip ID
     * @retval  Chip ID
     */
    virtual uint8_t getChipID() = 0;

    //Status function
    /**
    * @brief Query whether it is currently in charging state
    * @retval true to charge,false to not charge
    */
    // virtual bool isCharging() = 0;

    /**
     * @brief Query whether the current USB is connected
     * @retval true to access,false to not access
     */
    // virtual bool isVbusIn() = 0;

    /**
     * @brief Query whether the current battery is connected
     * @retval true to access,false to not access
     */
    // virtual bool isBatteryConnect() = 0;

    /**
     * @brief Query whether the current is in the discharge state
     * @retval true the battery is discharged,false is not discharged
     */
    virtual bool isDischarge() = 0;

    //Power Channel Control

    /**
     * @brief  Turn on the power channel
     * @param  channel: Parameters See XPowersPowerChannel_t enumeration
     * @retval true success false failed
     */
    virtual bool enablePowerOutput(uint8_t channel) = 0;

    /**
     * @brief  Turn off the power channel
     * @param  channel: Parameters See XPowersPowerChannel_t enumeration
     * @retval true success false failed
     */
    virtual bool disablePowerOutput(uint8_t channel) = 0;

    /**
     * @brief  Get whether the power channel is enabled
     * @param  channel: Parameters See XPowersPowerChannel_t enumeration
     * @retval true success false failed
     */
    virtual bool isPowerChannelEnable(uint8_t channel) = 0;

    /**
     * @brief  Get the set voltage of the power channel
     * @param  channel: Parameters See XPowersPowerChannel_t enumeration
     * @retval true success false failed
     */
    virtual uint16_t getPowerChannelVoltage(uint8_t channel) = 0;

    /**
     * @brief  Set the output voltage of a channel power supply
     * @param  channel: Parameters See XPowersPowerChannel_t enumeration
     * @retval true success false failed
     */
    virtual bool setPowerChannelVoltage(uint8_t channel, uint16_t millivolt) = 0;

    /**
     * @brief  Set a channel power protection,after setting this channel
     *         will not be able to be set and closed
     * @param  channel: Parameters See XPowersPowerChannel_t enumeration
     */
    virtual void setProtectedChannel(uint8_t channel);

    /**
     * @brief  Unprotect the channel, call this to unprotect the channel lock
     * @param  channel: Parameters See XPowersPowerChannel_t enumeration
     */
    virtual void setUnprotectChannel(uint8_t channel);

    /**
     * * @brief  Get whether a channel power supply has been protected
     * @param  channel: Parameters See XPowersPowerChannel_t enumeration
     * @retval true is set,false is not set
     */
    virtual bool getProtectedChannel(uint8_t channel);


    /**
     * @brief  Query whether the PMU input parameter channel is valid
     * @param  channel: Parameters See XPowersPowerChannel_t enumeration
     * @retval true valid false invalid
     */
    virtual bool isChannelAvailable(uint8_t channel);


    //battery
    /**
    * @brief  Get battery Voltage
    * @retval Voltage unit: millivolt , 0 is no battery is connected
    */
    // virtual uint16_t getBattVoltage() = 0;

    /**
    * @brief  Get battery percentage
    * @retval 0~100% , -1 no battery is connected
    */
    // virtual int getBatteryPercent(void);

    // Vbus
    /**
    * @brief  Get PMU VBUS/USB Voltage
    * @retval Voltage unit: millivolt , 0 is no vbus is connected
    */
    virtual uint16_t getVbusVoltage();


    /**
     * @brief  Set VBUS Current Input Limit.
     * @param  opt: View the related chip type xpowers_axpxxx_vbus_cur_limit_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval true valid false invalid
     */
    virtual bool setVbusCurrentLimit(uint8_t opt) = 0;

    /**
    * @brief  Get VBUS Current Input Limit.
    * @retval View the related chip type xpowers_axpxxx_vbus_cur_limit_t enumeration
    *              parameters in "XPowersParams.hpp"
    */
    virtual uint8_t getVbusCurrentLimit(void) = 0;


    /**
     * @brief  Set VBUS Voltage Input Limit.
     * @param  opt: View the related chip type xpowers_axpxxx_vbus_vol_limit_t enumeration
     *              parameters in "XPowersParams.hpp"
     * 
     */
    virtual void setVbusVoltageLimit(uint8_t opt) = 0;

    /**
    * @brief  Get VBUS Voltage Input Limit.
    * @retval View the related chip type xpowers_axpxxx_vbus_vol_limit_t enumeration
    *              parameters in "XPowersParams.hpp"
    */
    virtual uint8_t getVbusVoltageLimit(void) = 0;


    // SYS
    /**
    * @brief  Get PMU SYS main Voltage
    * @retval Voltage unit: millivolt
    */
    virtual uint16_t getSystemVoltage() = 0;

    /**
     * @brief  Set PMU Low Voltage Shutdown Threshold
     * @param  millivolt: 2600mV ~ 3300mV
     * @retval true valid false invalid
     */
    virtual bool setSysPowerDownVoltage(uint16_t millivolt) = 0;

    /**
     * @brief  Get PMU Low Voltage Shutdown Threshold
     * @retval Voltage unit: millivolt
     */
    virtual uint16_t getSysPowerDownVoltage() = 0;

    /**
     * @brief  Set charge target voltage.
     * @param  opt: View the related chip type xpowers_axpxxx_chg_vol_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval true valid false invalid
     */
    virtual bool setChargeTargetVoltage(uint8_t opt) = 0;

    /**
     * @brief  Get charge target voltage.
     * @retval View the related chip type xpowers_axpxxx_chg_vol_t enumeration
     *         parameters in "XPowersParams.hpp"
     *   parameters in "XPowersParams.hpp"
     */
    virtual uint8_t getChargeTargetVoltage() = 0;

    /**
     * @brief  Set charge current.
     * @param  opt: View the related chip type xpowers_axpxxx_chg_curr_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval true valid false invalid
     */
    virtual bool setChargerConstantCurr(uint8_t opt) = 0;

    /**
     * @brief  Get charge current.
     * @retval View the related chip type xpowers_axpxxx_chg_curr_t enumeration
     *         parameters in "XPowersParams.hpp"
     */
    virtual uint8_t getChargerConstantCurr() = 0;


    //!PMU Interrupt control
    /*
     *  Example of interrupt usage
     *  if (pmuInterrupt) {
     *      pmuInterrupt = false;
     *
     *      Read interrupt status
     *      uint64_t mask =  PMU->getIrqStatus();
     *      Serial.print("IRQ Mask:0b");
     *      Serial.println(mask,BIN);
     *
     *      if (PMU->isPekeyShortPressIrq()) {
     *          Serial.println("isPekeyShortPressIrq");
     *      }
     *      if (PMU->isBatChargeStartIrq()) {
     *          Serial.println("isBatChargeStart");
     *      }
     *      ......
     *
     *      After reading the interrupt status,you need to manually clear the status register
     *      PMU->clearIrqStatus();
     *  }
     * * * */


    /**
    * @brief  Get the interrupt controller mask value.
    * @retval   Mask value corresponds to xpowers_axpxxx_irq_t ,
    */
    virtual uint64_t getIrqStatus() = 0;


    /**
     * @brief  Clear interrupt controller state.
     */
    virtual void clearIrqStatus() = 0;

    /**
     * @brief  Enable PMU interrupt control mask .
     * @param  opt: View the related chip type xpowers_axpxxx_irq_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval true valid false invalid
     */
    virtual bool enableIRQ(uint64_t opt) = 0;

    /**
     * @brief  Disable PMU interrupt control mask .
     * @param  opt: View the related chip type xpowers_axpxxx_irq_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval true valid false invalid
     */
    virtual bool disableIRQ(uint64_t opt) = 0;

    /**
     * @brief   .
     * @param  opt: View the related chip type xpowers_interrupt_enum_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval true valid false invalid
     */
    bool enableInterrupt(uint32_t opt);

    /**
     * @brief   .
     * @param  opt: View the related chip type xpowers_interrupt_enum_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval true valid false invalid
     */
    bool disableInterrupt(uint32_t opt);

    /**
     * @brief   .
     * @param  opt: View the related chip type xpowers_interrupt_enum_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval true valid false invalid
     */
    bool setInterruptMask(uint32_t option, bool enable);

    /**
     * @brief  Interrupt response when PMU PEKEY is short pressed
     * @retval true valid false invalid
     */
    virtual bool isPekeyShortPressIrq() = 0;

    /**
     * @brief  Interrupt response when PMU PEKEY is long pressed
     * @retval true valid false invalid
     */
    virtual bool isPekeyLongPressIrq() = 0;

    /**
     * @brief  Interrupt response when PMU battery is connected
     * @retval true valid false invalid
     */
    virtual bool isBatInsertIrq() = 0;

    /**
     * @brief  Interrupt response when PMU battery is removed
     * @retval true valid false invalid
     */
    virtual bool isBatRemoveIrq() = 0;

    /**
     * @brief  Interrupt response when PMU USB is plugged in
     * @retval true valid false invalid
     */
    virtual bool isVbusInsertIrq() = 0;

    /**
     * @brief  Interrupt response when PMU USB is removed
     * @retval true valid false invalid
     */
    virtual bool isVbusRemoveIrq() = 0;

    /**
     * @brief  Interrupt response when PMU charging is complete
     * @retval true valid false invalid
     */
    virtual bool isBatChargeDoneIrq() = 0;

    /**
     * @brief  Interrupt response when PMU charging starts
     * @retval true valid false invalid
     */
    virtual bool isBatChargeStartIrq() = 0;


    //Data collection function

    /**
     * @brief  Enable battery detection function,the default is on
     * @retval true success false failed
     */
    virtual bool enableBattDetection() = 0;

    /**
     * @brief  Disable battery detection
     * @retval true success false failed
     */
    virtual bool disableBattDetection() = 0;

    /**
     * @brief  Enable USB input voltage detection
     * @retval true success false failed
     */
    virtual bool enableVbusVoltageMeasure(void) = 0;

    /**
     * @brief  Disable USB input voltage detection
     * @retval true success false failed
     */
    virtual bool disableVbusVoltageMeasure(void) = 0;

    /**
     * @brief  Enable system voltage detection
     * @retval true success false failed
     */
    virtual bool enableSystemVoltageMeasure(void) = 0;

    /**
     * @brief  Disable system voltage detection
     * @retval true success false failed
     */
    virtual bool disableSystemVoltageMeasure(void) = 0;

    /**
     * @brief  Enable PMU internal temperature sensor detection
     * @retval true success false failed
     */
    virtual bool enableTemperatureMeasure(void) = 0;

    /**
     * @brief  Disable PMU internal temperature sensor detection
     * @retval true success false failed
     */
    virtual bool disableTemperatureMeasure(void) = 0;

    /**
     * @brief  Enable battery input voltage detection
     * @retval true success false failed
     */
    virtual bool enableBattVoltageMeasure(void) = 0;

    /**
     * @brief  Disable battery input voltage detection
     * @retval true success false failed
     */
    virtual bool disableBattVoltageMeasure(void) = 0;

    /**
     * @brief  Enable NTC thermistor detection (requires hardware support)
     * @retval true success false failed
     */
    virtual bool enableTSPinMeasure(void) = 0;

    /**
     * @brief  Disable NTC thermistor detection (requires hardware support)
     * @retval true success false failed
     */
    virtual bool disableTSPinMeasure(void) = 0;

    // Charge indicator function
    /**
    * @brief  Set charging led mode
    * @param  opt: View the related chip type xpowers_chg_led_mode_t enumeration
    *              parameters in "XPowersParams.hpp"
    */
    virtual void setChargingLedMode(uint8_t mode) = 0;



    // PMU PEKEY settings
    /**
    * @brief  Set PEKEY press power on time
    * @param  opt: View the related chip type xpowers_press_on_time_t enumeration
    *              parameters in "XPowersParams.hpp"
    * @retval true success false failed
    */
    virtual bool setPowerKeyPressOnTime(uint8_t opt) = 0;

    /**
     * @brief Get PEKEY press power on time
     * @retval View the related chip type xpowers_press_on_time_t enumeration
     *              parameters in "XPowersParams.hpp"
     */
    virtual uint8_t getPowerKeyPressOnTime() = 0;

    /**
     * @brief Set PEKEY press power off time
     * @param  opt: View the related chip type xpowers_press_off_time_t enumeration
     *              parameters in "XPowersParams.hpp"
     * @retval true success false failed
     */
    virtual bool setPowerKeyPressOffTime(uint8_t opt) = 0;

    /**
     * @brief Get PEKEY press power off time
     * @retval View the related chip type xpowers_press_off_time_t enumeration
     *              parameters in "XPowersParams.hpp"
     */
    virtual uint8_t getPowerKeyPressOffTime() = 0;

    /**
     * @brief Get the chip model
     * @retval See XPowersChipModel_t enumeration
     */
    uint8_t getChipModel()
    {
        return __chipModel;
    }

protected:

    void setChipModel(uint8_t m)
    {
        __chipModel = m;
    }

    uint8_t __chipModel;
    uint32_t __protectedMask;

};

