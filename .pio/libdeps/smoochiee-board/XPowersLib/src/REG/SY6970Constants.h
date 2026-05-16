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
 * @file      SY6970Constants.h
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2023-07-20
 *
 */
#pragma once

#define SY6970_SLAVE_ADDRESS                                    (0x6A)

#define SY6970_DEV_REV                                          (0x00)

#define POWERS_SY6970_VBUS_MASK_VAL(val)                        (val & 0x7F)
#define POWERS_SY6970_VBAT_MASK_VAL(val)                        (val & 0x7F)
#define POWERS_SY6970_VSYS_MASK_VAL(val)                        (val & 0x7F)
#define POWERS_SY6970_NTC_MASK_VAL(val)                         (val & 0x7F)

#define POWERS_SY6970_VBUS_BASE_VAL                             (2600)
#define POWERS_SY6970_VBAT_BASE_VAL                             (2304)
#define POWERS_SY6970_VSYS_BASE_VAL                             (2304)
#define POWERS_SY6970_NTC_BASE_VAL                              (21)

#define POWERS_SY6970_VBUS_VOL_STEP                             (100)
#define POWERS_SY6970_VBAT_VOL_STEP                             (20)
#define POWERS_SY6970_VSYS_VOL_STEP                             (20)
#define POWERS_SY6970_NTC_VOL_STEP                              (0.465)
#define POWERS_SY6970_CHG_STEP_VAL                              (50)

#define POWERS_SY6970_PRE_CHG_CUR_BASE                          (64)
#define POWERS_SY6970_FAST_CHG_CUR_STEP                         (64)
#define POWERS_SY6970_PRE_CHG_CUR_STEP                          (64)

#define POWERS_SY6970_FAST_CHG_CURRENT_MAX                      (5056)

#define POWERS_SY6970_PRE_CHG_CURRENT_MIN                       (64)
#define POWERS_SY6970_PRE_CHG_CURRENT_MAX                       (1024)

#define POWERS_SY6970_CHG_VOL_BASE                              (3840)
#define POWERS_SY6970_CHG_VOL_STEP                              (16)
#define POWERS_SY6970_FAST_CHG_VOL_MIN                          (3840)
#define POWERS_SY6970_FAST_CHG_VOL_MAX                          (4608)

#define POWERS_SY6970_SYS_VOL_STEPS                             (100)
#define POWERS_SY6970_SYS_VOFF_VOL_MIN                          (3000)
#define POWERS_SY6970_SYS_VOFF_VOL_MAX                          (3700)

#define POWERS_SY6970_IN_CURRENT_STEP                           (50)
#define POWERS_SY6970_IN_CURRENT_MIN                            (100)
#define POWERS_SY6970_IN_CURRENT_MAX                            (3250)

#define POWERS_SY6970_BOOTS_VOL_BASE                            (4550)
#define POWERS_SY6970_BOOTS_VOL_STEP                            (64)
#define POWERS_SY6970_BOOST_VOL_MIN                             (4550)
#define POWERS_SY6970_BOOST_VOL_MAX                             (5510)

#define POWERS_SY6970_IRQ_WTD_FAULT(x)                          (bool)(( x & 0xFF ) >> 7)
#define POWERS_SY6970_IRQ_BOOST_FAULT(x)                        (bool)(( x & 0xFF ) >> 6)
#define POWERS_SY6970_IRQ_CHG_FAULT(x)                          (bool)(( x & 0xFF ) >> 5)
#define POWERS_SY6970_IRQ_BAT_FAULT(x)                          (bool)(( x & 0xFF ) >> 4)
#define POWERS_SY6970_IRQ_NTC_FAULT(x)                          (bool)(( x & 0xFF ) & 0x03)

#define POWERS_SY6970_VINDPM_VOL_BASE                           (4550)
#define POWERS_SY6970_VINDPM_VOL_STEPS                          (100)
#define POWERS_SY6970_VINDPM_VOL_MIN                            (3900)
#define POWERS_SY6970_VINDPM_VOL_MAX                            (15300)