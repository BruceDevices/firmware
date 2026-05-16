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
 * @file      XPowersLibInterface.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2022-08-28
 *
 */
#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include "XPowersLibInterface.hpp"


bool XPowersLibInterface::isChannelAvailable(uint8_t channel)
{
    if (__chipModel == XPOWERS_AXP192) {
        switch (channel) {
        case XPOWERS_DCDC1:
        case XPOWERS_DCDC2:
        case XPOWERS_DCDC3:
        case XPOWERS_LDO2:
        case XPOWERS_LDO3:
        case XPOWERS_LDOIO:
            return true;
        default:
            return false;
        }
    } else if (__chipModel == XPOWERS_AXP202) {

        switch (channel) {
        case XPOWERS_DCDC2:
        case XPOWERS_DCDC3:
        case XPOWERS_LDO2:
        case XPOWERS_LDO3:
        case XPOWERS_LDO4:
        case XPOWERS_LDO5:
            return true;
        default:
            return false;
        }

    } else if (__chipModel == XPOWERS_AXP2101) {
        switch (channel) {
        case XPOWERS_DCDC1:
        case XPOWERS_DCDC2:
        case XPOWERS_DCDC3:
        case XPOWERS_DCDC4:
        case XPOWERS_DCDC5:
        case XPOWERS_ALDO1:
        case XPOWERS_ALDO2:
        case XPOWERS_ALDO3:
        case XPOWERS_ALDO4:
        case XPOWERS_BLDO1:
        case XPOWERS_BLDO2:
        case XPOWERS_VBACKUP:
        case XPOWERS_CPULDO:
            return true;
        default:
            // DLDO is not available, will also return false
            return false;
        }
    }
    return false;
}

void XPowersLibInterface::setProtectedChannel(uint8_t channel)
{
    __protectedMask |= _BV(channel);
}

void XPowersLibInterface::setUnprotectChannel(uint8_t channel)
{
    __protectedMask &= (~_BV(channel));
}

bool XPowersLibInterface::getProtectedChannel(uint8_t channel)
{
    return __protectedMask & _BV(channel);
}


uint16_t XPowersLibInterface::getVbusVoltage()
{
    return 0;
}

static uint64_t inline check_params(uint32_t opt, uint32_t params, uint64_t mask)
{
    return ((opt & params) == params) ? mask : 0;
}

bool XPowersLibInterface::enableInterrupt(uint32_t option)
{
    return setInterruptMask(option, true);
}

bool XPowersLibInterface::disableInterrupt(uint32_t option)
{
    return setInterruptMask(option, false);
}

bool XPowersLibInterface::setInterruptMask(uint32_t option, bool enable)
{
    uint64_t params = 0;
    switch (__chipModel) {
    case XPOWERS_AXP173:
        break;
    case XPOWERS_AXP192:
        params |=  check_params(option, XPOWERS_USB_INSERT_INT, XPOWERS_AXP192_VBUS_INSERT_IRQ);
        params |=  check_params(option, XPOWERS_USB_REMOVE_INT, XPOWERS_AXP192_VBUS_REMOVE_IRQ);
        params |=  check_params(option, XPOWERS_BATTERY_INSERT_INT, XPOWERS_AXP192_BAT_INSERT_IRQ);
        params |=  check_params(option, XPOWERS_BATTERY_REMOVE_INT, XPOWERS_AXP192_BAT_REMOVE_IRQ);
        params |=  check_params(option, XPOWERS_CHARGE_START_INT, XPOWERS_AXP192_BAT_CHG_START_IRQ);
        params |=  check_params(option, XPOWERS_CHARGE_DONE_INT, XPOWERS_AXP192_BAT_CHG_DONE_IRQ);
        params |=  check_params(option, XPOWERS_PWR_BTN_CLICK_INT, XPOWERS_AXP192_PKEY_SHORT_IRQ);
        params |=  check_params(option, XPOWERS_PWR_BTN_LONGPRESSED_INT, XPOWERS_AXP192_PKEY_LONG_IRQ);
        params |=  check_params(option, XPOWERS_ALL_INT, XPOWERS_AXP192_ALL_IRQ);
        return enable ? enableIRQ(params) : disableIRQ(params);
        break;
    case XPOWERS_AXP202:
        params |=  check_params(option, XPOWERS_USB_INSERT_INT, XPOWERS_AXP202_VBUS_INSERT_IRQ);
        params |=  check_params(option, XPOWERS_USB_REMOVE_INT, XPOWERS_AXP202_VBUS_REMOVE_IRQ);
        params |=  check_params(option, XPOWERS_BATTERY_INSERT_INT, XPOWERS_AXP202_BAT_INSERT_IRQ);
        params |=  check_params(option, XPOWERS_BATTERY_REMOVE_INT, XPOWERS_AXP202_BAT_REMOVE_IRQ);
        params |=  check_params(option, XPOWERS_CHARGE_START_INT, XPOWERS_AXP202_BAT_CHG_START_IRQ);
        params |=  check_params(option, XPOWERS_CHARGE_DONE_INT, XPOWERS_AXP202_BAT_CHG_DONE_IRQ);
        params |=  check_params(option, XPOWERS_PWR_BTN_CLICK_INT, XPOWERS_AXP202_PKEY_SHORT_IRQ);
        params |=  check_params(option, XPOWERS_PWR_BTN_LONGPRESSED_INT, XPOWERS_AXP202_PKEY_LONG_IRQ);
        params |=  check_params(option, XPOWERS_ALL_INT, XPOWERS_AXP202_ALL_IRQ);
        return enable ? enableIRQ(params) : disableIRQ(params);
        break;
    case XPOWERS_AXP216:
        break;
    case XPOWERS_AXP2101:
        params |=  check_params(option, XPOWERS_USB_INSERT_INT, XPOWERS_AXP2101_VBUS_INSERT_IRQ);
        params |=  check_params(option, XPOWERS_USB_REMOVE_INT, XPOWERS_AXP2101_VBUS_REMOVE_IRQ);
        params |=  check_params(option, XPOWERS_BATTERY_INSERT_INT, XPOWERS_AXP2101_BAT_INSERT_IRQ);
        params |=  check_params(option, XPOWERS_BATTERY_REMOVE_INT, XPOWERS_AXP2101_BAT_REMOVE_IRQ);
        params |=  check_params(option, XPOWERS_CHARGE_START_INT, XPOWERS_AXP2101_BAT_CHG_START_IRQ);
        params |=  check_params(option, XPOWERS_CHARGE_DONE_INT, XPOWERS_AXP2101_BAT_CHG_DONE_IRQ);
        params |=  check_params(option, XPOWERS_PWR_BTN_CLICK_INT, XPOWERS_AXP2101_PKEY_SHORT_IRQ);
        params |=  check_params(option, XPOWERS_PWR_BTN_LONGPRESSED_INT, XPOWERS_AXP2101_PKEY_LONG_IRQ);
        params |=  check_params(option, XPOWERS_ALL_INT, XPOWERS_AXP2101_ALL_IRQ);
        return enable ? enableIRQ(params) : disableIRQ(params);
        break;
    default:
        break;
    }
    return false;
}