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
 * @file      XPowersLib.h
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2024-10-30
 *
 */

#pragma once

#if defined(XPOWERS_CHIP_AXP192)
#include "XPowersAXP192.tpp"
typedef XPowersAXP192 XPowersPMU;
#elif defined(XPOWERS_CHIP_AXP202)
#include "XPowersAXP202.tpp"
typedef XPowersAXP202 XPowersPMU;
#elif defined(XPOWERS_CHIP_AXP2101)
#include "XPowersAXP2101.tpp"
typedef XPowersAXP2101 XPowersPMU;
#elif defined(XPOWERS_CHIP_SY6970)
#include "PowersSY6970.tpp"
typedef PowersSY6970 XPowersPPM;
#elif defined(XPOWERS_CHIP_BQ25896)
#include "PowersBQ25896.tpp"
typedef PowersBQ25896 XPowersPPM;
#else
#include "XPowersAXP192.tpp"
#include "XPowersAXP202.tpp"
#include "XPowersAXP2101.tpp"
#include "PowersSY6970.tpp"
#endif

#include "PowerDeliveryHUSB238.hpp"

#include "XPowersLib_Version.h"
