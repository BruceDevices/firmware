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
 * @file      XPowersParams.hpp
 * @author    Lewis He (lewishe@outlook.com)
 * @date      2022-08-28
 *
 */

#pragma once

#ifdef _BV
#undef _BV
#endif
#define _BV(b)                          (1ULL << (uint64_t)(b))


//TODO:Unified interface functions and parameters

/**
 * @brief PMU PEKEY Press off time parameters.
 */
typedef enum __xpowers_press_off_time {
    XPOWERS_POWEROFF_4S,
    XPOWERS_POWEROFF_6S,
    XPOWERS_POWEROFF_8S,
    XPOWERS_POWEROFF_10S,
} xpowers_press_off_time_t;

/**
 * @brief PMU PEKEY Press on time parameters.
 */
typedef enum __xpowers_press_on_time {
    XPOWERS_POWERON_128MS,
    XPOWERS_POWERON_512MS,
    XPOWERS_POWERON_1S,
    XPOWERS_POWERON_2S,
} xpowers_press_on_time_t;


/**
 * @brief Charging led mode parameters.
 */
typedef enum __xpowers_chg_led_mode {
    XPOWERS_CHG_LED_OFF,
    XPOWERS_CHG_LED_BLINK_1HZ,
    XPOWERS_CHG_LED_BLINK_4HZ,
    XPOWERS_CHG_LED_ON,
    XPOWERS_CHG_LED_CTRL_CHG,    // The charging indicator is controlled by the charger
} xpowers_chg_led_mode_t;

/**
 * @brief axp2101 charge target voltage parameters.
 */
typedef enum __xpowers_axp2101_chg_vol {
    XPOWERS_AXP2101_CHG_VOL_4V = 1,
    XPOWERS_AXP2101_CHG_VOL_4V1,
    XPOWERS_AXP2101_CHG_VOL_4V2,
    XPOWERS_AXP2101_CHG_VOL_4V35,
    XPOWERS_AXP2101_CHG_VOL_4V4,
    XPOWERS_AXP2101_CHG_VOL_MAX
} xpowers_axp2101_chg_vol_t;

/**
 * @brief axp2101 charge currnet voltage parameters.
 */
typedef enum __xpowers_axp2101_chg_curr {
    XPOWERS_AXP2101_CHG_CUR_0MA,
    XPOWERS_AXP2101_CHG_CUR_100MA = 4,
    XPOWERS_AXP2101_CHG_CUR_125MA,
    XPOWERS_AXP2101_CHG_CUR_150MA,
    XPOWERS_AXP2101_CHG_CUR_175MA,
    XPOWERS_AXP2101_CHG_CUR_200MA,
    XPOWERS_AXP2101_CHG_CUR_300MA,
    XPOWERS_AXP2101_CHG_CUR_400MA,
    XPOWERS_AXP2101_CHG_CUR_500MA,
    XPOWERS_AXP2101_CHG_CUR_600MA,
    XPOWERS_AXP2101_CHG_CUR_700MA,
    XPOWERS_AXP2101_CHG_CUR_800MA,
    XPOWERS_AXP2101_CHG_CUR_900MA,
    XPOWERS_AXP2101_CHG_CUR_1000MA,
} xpowers_axp2101_chg_curr_t;


/**
 * @brief axp192 charge target voltage parameters.
 */
typedef enum __xpowers_axp192_chg_vol {
    XPOWERS_AXP192_CHG_VOL_4V1,
    XPOWERS_AXP192_CHG_VOL_4V15,
    XPOWERS_AXP192_CHG_VOL_4V2,
    XPOWERS_AXP192_CHG_VOL_4V36,
    XPOWERS_AXP192_CHG_VOL_MAX,
} xpowers_axp192_chg_vol_t;

/**
 * @brief axp202 charge target voltage parameters.
 */
typedef enum __xpowers_axp202_chg_vol {
    XPOWERS_AXP202_CHG_VOL_4V1,
    XPOWERS_AXP202_CHG_VOL_4V15,
    XPOWERS_AXP202_CHG_VOL_4V2,
    XPOWERS_AXP202_CHG_VOL_4V36,
    XPOWERS_AXP202_CHG_VOL_MAX,
} xpowers_axp202_chg_vol_t;

/**
 * @brief axp192 charge currnet voltage parameters.
 */
typedef enum __xpowers_axp192_chg_curr {
    XPOWERS_AXP192_CHG_CUR_100MA,
    XPOWERS_AXP192_CHG_CUR_190MA,
    XPOWERS_AXP192_CHG_CUR_280MA,
    XPOWERS_AXP192_CHG_CUR_360MA,
    XPOWERS_AXP192_CHG_CUR_450MA,
    XPOWERS_AXP192_CHG_CUR_550MA,
    XPOWERS_AXP192_CHG_CUR_630MA,
    XPOWERS_AXP192_CHG_CUR_700MA,
    XPOWERS_AXP192_CHG_CUR_780MA,
    XPOWERS_AXP192_CHG_CUR_880MA,
    XPOWERS_AXP192_CHG_CUR_960MA,
    XPOWERS_AXP192_CHG_CUR_1000MA,
    XPOWERS_AXP192_CHG_CUR_1080MA,
    XPOWERS_AXP192_CHG_CUR_1160MA,
    XPOWERS_AXP192_CHG_CUR_1240MA,
    XPOWERS_AXP192_CHG_CUR_1320MA,
} xpowers_axp192_chg_curr_t;

/**
 * @brief axp202 charge currnet voltage parameters.
 */
typedef enum __xpowers_axp202_chg_curr {
    XPOWERS_AXP202_CHG_CUR_100MA,
    XPOWERS_AXP202_CHG_CUR_190MA,
    XPOWERS_AXP202_CHG_CUR_280MA,
    XPOWERS_AXP202_CHG_CUR_360MA,
    XPOWERS_AXP202_CHG_CUR_450MA,
    XPOWERS_AXP202_CHG_CUR_550MA,
    XPOWERS_AXP202_CHG_CUR_630MA,
    XPOWERS_AXP202_CHG_CUR_700MA,
    XPOWERS_AXP202_CHG_CUR_780MA,
    XPOWERS_AXP202_CHG_CUR_880MA,
    XPOWERS_AXP202_CHG_CUR_960MA,
    XPOWERS_AXP202_CHG_CUR_1000MA,
    XPOWERS_AXP202_CHG_CUR_1080MA,
    XPOWERS_AXP202_CHG_CUR_1160MA,
    XPOWERS_AXP202_CHG_CUR_1240MA,
    XPOWERS_AXP202_CHG_CUR_1320MA,
} xpowers_axp202_chg_curr_t;


/**
 * @brief axp2101 vbus currnet limit parameters.
 */
typedef enum {
    XPOWERS_AXP2101_VBUS_CUR_LIM_100MA,
    XPOWERS_AXP2101_VBUS_CUR_LIM_500MA,
    XPOWERS_AXP2101_VBUS_CUR_LIM_900MA,
    XPOWERS_AXP2101_VBUS_CUR_LIM_1000MA,
    XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA,
    XPOWERS_AXP2101_VBUS_CUR_LIM_2000MA,
} xpowers_axp2101_vbus_cur_limit_t;

/**
 * @brief axp192 vbus currnet limit parameters.
 */
typedef enum {
    XPOWERS_AXP192_VBUS_CUR_LIM_500MA,
    XPOWERS_AXP192_VBUS_CUR_LIM_100MA,
    XPOWERS_AXP192_VBUS_CUR_LIM_OFF,
} xpowers_axp192_vbus_cur_limit_t;


/**
 * @brief axp202 vbus currnet limit parameters.
 */
typedef enum {
    XPOWERS_AXP202_VBUS_CUR_LIM_900MA,
    XPOWERS_AXP202_VBUS_CUR_LIM_500MA,
    XPOWERS_AXP202_VBUS_CUR_LIM_100MA,
    XPOWERS_AXP202_VBUS_CUR_LIM_OFF,
} xpowers_axp202_vbus_cur_limit_t;


typedef enum {
    XPOWERS_AXP192_VBUS_VOL_LIM_4V,
    XPOWERS_AXP192_VBUS_VOL_LIM_4V1,
    XPOWERS_AXP192_VBUS_VOL_LIM_4V2,
    XPOWERS_AXP192_VBUS_VOL_LIM_4V3,
    XPOWERS_AXP192_VBUS_VOL_LIM_4V4,
    XPOWERS_AXP192_VBUS_VOL_LIM_4V5,
    XPOWERS_AXP192_VBUS_VOL_LIM_4V6,
    XPOWERS_AXP192_VBUS_VOL_LIM_4V7,
} xpowers_axp192_vbus_vol_limit_t;

typedef enum {
    XPOWERS_AXP202_VBUS_VOL_LIM_4V,
    XPOWERS_AXP202_VBUS_VOL_LIM_4V1,
    XPOWERS_AXP202_VBUS_VOL_LIM_4V2,
    XPOWERS_AXP202_VBUS_VOL_LIM_4V3,
    XPOWERS_AXP202_VBUS_VOL_LIM_4V4,
    XPOWERS_AXP202_VBUS_VOL_LIM_4V5,
    XPOWERS_AXP202_VBUS_VOL_LIM_4V6,
    XPOWERS_AXP202_VBUS_VOL_LIM_4V7,
} xpowers_axp202_vbus_vol_limit_t;

typedef enum {
    XPOWERS_AXP2101_VBUS_VOL_LIM_3V88,
    XPOWERS_AXP2101_VBUS_VOL_LIM_3V96,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V04,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V12,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V20,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V28,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V36,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V44,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V52,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V60,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V68,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V76,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V84,
    XPOWERS_AXP2101_VBUS_VOL_LIM_4V92,
    XPOWERS_AXP2101_VBUS_VOL_LIM_5V,
    XPOWERS_AXP2101_VBUS_VOL_LIM_5V08,
} xpower_apx2101_vbus_vol_limit_t;




/**
 * @brief XPowersLibInterface interrupt control mask parameters.
 *          Common interrupt interfaces
 * @Example: enableInterrupt(XPOWERS_USB_INSERT_INT|XPOWERS_USB_REMOVE_INT);
 *           disableInterrupt(XPOWERS_CHARGE_START_INT|XPOWERS_PWR_BTN_CLICK_INT);
 */
typedef enum __xpowers_interrupt_enum {
    XPOWERS_USB_INSERT_INT          = _BV(0),
    XPOWERS_USB_REMOVE_INT          = _BV(1),
    XPOWERS_BATTERY_INSERT_INT      = _BV(2),
    XPOWERS_BATTERY_REMOVE_INT      = _BV(3),
    XPOWERS_CHARGE_START_INT        = _BV(4),
    XPOWERS_CHARGE_DONE_INT         = _BV(5),
    XPOWERS_PWR_BTN_CLICK_INT       = _BV(6),
    XPOWERS_PWR_BTN_LONGPRESSED_INT = _BV(7),
    XPOWERS_ALL_INT                 = _BV(8),
} xpowers_interrupt_enum_t;


/**
 * @brief axp192 interrupt control mask parameters.
 */
typedef enum __xpowers_axp192_irq {
    //! IRQ1 REG 40H
    XPOWERS_AXP192_VBUS_VHOLD_LOW_IRQ       = _BV(1),   //VBUS is available, but lower than V HOLD, IRQ enable
    XPOWERS_AXP192_VBUS_REMOVE_IRQ          = _BV(2),   //VBUS removed, IRQ enable
    XPOWERS_AXP192_VBUS_INSERT_IRQ          = _BV(3),   //VBUS connected, IRQ enable
    XPOWERS_AXP192_VBUS_OVER_VOL_IRQ        = _BV(4),   //VBUS over-voltage, IRQ enable
    XPOWERS_AXP192_ACIN_REMOVED_IRQ         = _BV(5),   //ACIN removed, IRQ enable
    XPOWERS_AXP192_ACIN_CONNECT_IRQ         = _BV(6),   //ACIN connected, IRQ enable
    XPOWERS_AXP192_ACIN_OVER_VOL_IRQ        = _BV(7),   //ACIN over-voltage, IRQ enable

    //! IRQ2 REG 41H
    XPOWERS_AXP192_BATT_LOW_TEMP_IRQ        = _BV(8),   //Battery low-temperature, IRQ enable
    XPOWERS_AXP192_BATT_OVER_TEMP_IRQ       = _BV(9),   //Battery over-temperature, IRQ enable
    XPOWERS_AXP192_BAT_CHG_DONE_IRQ         = _BV(10),  //Charge finished, IRQ enable
    XPOWERS_AXP192_BAT_CHG_START_IRQ        = _BV(11),  //Be charging, IRQ enable
    XPOWERS_AXP192_BATT_EXIT_ACTIVATE_IRQ   = _BV(12),  //Exit battery activate mode, IRQ enable
    XPOWERS_AXP192_BATT_ACTIVATE_IRQ        = _BV(13),  //Battery activate mode, IRQ enable
    XPOWERS_AXP192_BAT_REMOVE_IRQ           = _BV(14),  //Battery removed, IRQ enable
    XPOWERS_AXP192_BAT_INSERT_IRQ           = _BV(15),  //Battery connected, IRQ enable

    //! IRQ3 REG 42H
    XPOWERS_AXP192_PKEY_LONG_IRQ            = _BV(16),  //PEK long press, IRQ enable
    XPOWERS_AXP192_PKEY_SHORT_IRQ           = _BV(17),  //PEK short press, IRQ enable
    //**Reserved and unchangeable BIT 2
    XPOWERS_AXP192_DC3_LOW_VOL_IRQ          = _BV(19),  //DC-DC3output voltage is lower than the set value, IRQ enable
    XPOWERS_AXP192_DC2_LOW_VOL_IRQ          = _BV(20),  //DC-DC2 output voltage is lower than the set value, IRQ enable
    XPOWERS_AXP192_DC1_LOW_VOL_IRQ          = _BV(21),  //DC-DC1 output voltage is lower than the set value, IRQ enable
    XPOWERS_AXP192_CHARGE_LOW_CUR_IRQ       = _BV(22),  //Charge current is lower than the set current, IRQ enable
    XPOWERS_AXP192_CHIP_TEMP_HIGH_IRQ       = _BV(23),  //XPOWERS internal over-temperature, IRQ enable

    //! IRQ4 REG 43H
    XPOWERS_AXP192_APS_LOW_VOL_LEVEL_IRQ    = _BV(24),  //APS low-voltage, IRQ enable
    //**Reserved and unchangeable BIT 1
    XPOWERS_AXP192_VBUS_SESSION_END_IRQ     = _BV(26),  //VBUS Session End IRQ enable
    XPOWERS_AXP192_VBUS_SESSION_AB_IRQ      = _BV(27),  //VBUS Session A/B IRQ enable
    XPOWERS_AXP192_VBUS_INVALID_IRQ         = _BV(28),  //VBUS invalid, IRQ enable
    XPOWERS_AXP192_VBUS_VAILD_IRQ           = _BV(29),  //VBUS valid, IRQ enable
    XPOWERS_AXP192_NOE_OFF_IRQ              = _BV(30),  //N_OE shutdown, IRQ enable
    XPOWERS_AXP192_NOE_ON_IRQ               = _BV(31),  //N_OE startup, IRQ enable

    //! IRQ5 REG 4AH
    XPOWERS_AXP192_GPIO0_EDGE_TRIGGER_IRQ   = _BV(32),  //GPIO0 input edge trigger, IRQ enable
    XPOWERS_AXP192_GPIO1_EDGE_TRIGGER_IRQ   = _BV(33),  //GPIO1input edge trigger or ADC input, IRQ enable
    XPOWERS_AXP192_GPIO2_EDGE_TRIGGER_IRQ   = _BV(34),  //GPIO2input edge trigger, IRQ enable
    //**Reserved and unchangeable BIT 3
    //**Reserved and unchangeable BIT 4
    //**Reserved and unchangeable BIT 5
    //**Reserved and unchangeable BIT 6
    XPOWERS_AXP192_TIMER_TIMEOUT_IRQ        = _BV(39),  //Timer timeout, IRQ enable

    XPOWERS_AXP192_ALL_IRQ                  = (0xFFFFFFFFFFULL)
} xpowers_axp192_irq_t;



/**
 * @brief axp2101 interrupt control mask parameters.
 */
typedef enum __xpowers_axp2101_irq {
    //! IRQ1 REG 40H
    XPOWERS_AXP2101_BAT_NOR_UNDER_TEMP_IRQ   = _BV(0),   // Battery Under Temperature in Work
    XPOWERS_AXP2101_BAT_NOR_OVER_TEMP_IRQ    = _BV(1),   // Battery Over Temperature in Work mode
    XPOWERS_AXP2101_BAT_CHG_UNDER_TEMP_IRQ   = _BV(2),   // Battery Under Temperature in Charge mode IRQ(bcut_irq)
    XPOWERS_AXP2101_BAT_CHG_OVER_TEMP_IRQ    = _BV(3),   // Battery Over Temperature in Charge mode IRQ(bcot_irq) enable
    XPOWERS_AXP2101_GAUGE_NEW_SOC_IRQ        = _BV(4),   // Gauge New SOC IRQ(lowsoc_irq) enable ???
    XPOWERS_AXP2101_WDT_TIMEOUT_IRQ          = _BV(5),   // Gauge Watchdog Timeout IRQ(gwdt_irq) enable
    XPOWERS_AXP2101_WARNING_LEVEL1_IRQ       = _BV(6),   // SOC drop to Warning Level1 IRQ(socwl1_irq) enable
    XPOWERS_AXP2101_WARNING_LEVEL2_IRQ       = _BV(7),   // SOC drop to Warning Level2 IRQ(socwl2_irq) enable

    //! IRQ2 REG 41H
    XPOWERS_AXP2101_PKEY_POSITIVE_IRQ        = _BV(8),   // POWERON Positive Edge IRQ(ponpe_irq_en) enable
    XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ        = _BV(9),   // POWERON Negative Edge IRQ(ponne_irq_en) enable
    XPOWERS_AXP2101_PKEY_LONG_IRQ            = _BV(10),  // POWERON Long PRESS IRQ(ponlp_irq) enable
    XPOWERS_AXP2101_PKEY_SHORT_IRQ           = _BV(11),  // POWERON Short PRESS IRQ(ponsp_irq_en) enable
    XPOWERS_AXP2101_BAT_REMOVE_IRQ           = _BV(12),  // Battery Remove IRQ(bremove_irq) enable
    XPOWERS_AXP2101_BAT_INSERT_IRQ           = _BV(13),  // Battery Insert IRQ(binsert_irq) enabl
    XPOWERS_AXP2101_VBUS_REMOVE_IRQ          = _BV(14),  // VBUS Remove IRQ(vremove_irq) enabl
    XPOWERS_AXP2101_VBUS_INSERT_IRQ          = _BV(15),  // VBUS Insert IRQ(vinsert_irq) enable

    //! IRQ3 REG 42H
    XPOWERS_AXP2101_BAT_OVER_VOL_IRQ         = _BV(16),  // Battery Over Voltage Protection IRQ(bovp_irq) enable
    XPOWERS_AXP2101_CHAGER_TIMER_IRQ         = _BV(17),  // Charger Safety Timer1/2 expire IRQ(chgte_irq) enable
    XPOWERS_AXP2101_DIE_OVER_TEMP_IRQ        = _BV(18),  // DIE Over Temperature level1 IRQ(dotl1_irq) enable
    XPOWERS_AXP2101_BAT_CHG_START_IRQ        = _BV(19),  // Charger start IRQ(chgst_irq) enable
    XPOWERS_AXP2101_BAT_CHG_DONE_IRQ         = _BV(20),  // Battery charge done IRQ(chgdn_irq) enable
    XPOWERS_AXP2101_BATFET_OVER_CURR_IRQ     = _BV(21),  // BATFET Over Current Protection IRQ(bocp_irq) enable
    XPOWERS_AXP2101_LDO_OVER_CURR_IRQ        = _BV(22),  // LDO Over Current IRQ(ldooc_irq) enable
    XPOWERS_AXP2101_WDT_EXPIRE_IRQ           = _BV(23),  // Watchdog Expire IRQ(wdexp_irq) enable

    XPOWERS_AXP2101_ALL_IRQ                  = (0xFFFFFFFFUL)
} xpowers_axp2101_irq_t;


/**
 * @brief axp202 interrupt control mask parameters.
 */
typedef enum __xpowers_axp202_irq {
    //! IRQ1 REG 40H
    XPOWERS_AXP202_VBUS_VHOLD_LOW_IRQ       = _BV(1),   //VBUS is available, but lower than V HOLD, IRQ enable
    XPOWERS_AXP202_VBUS_REMOVE_IRQ          = _BV(2),   //VBUS removed, IRQ enable
    XPOWERS_AXP202_VBUS_INSERT_IRQ          = _BV(3),   //VBUS connected, IRQ enable
    XPOWERS_AXP202_VBUS_OVER_VOL_IRQ        = _BV(4),   //VBUS over-voltage, IRQ enable
    XPOWERS_AXP202_ACIN_REMOVED_IRQ         = _BV(5),   //ACIN removed, IRQ enable
    XPOWERS_AXP202_ACIN_CONNECT_IRQ         = _BV(6),   //ACIN connected, IRQ enable
    XPOWERS_AXP202_ACIN_OVER_VOL_IRQ        = _BV(7),   //ACIN over-voltage, IRQ enable

    //! IRQ2 REG 41H
    XPOWERS_AXP202_BATT_LOW_TEMP_IRQ        = _BV(8),   //Battery low-temperature, IRQ enable
    XPOWERS_AXP202_BATT_OVER_TEMP_IRQ       = _BV(9),   //Battery over-temperature, IRQ enable
    XPOWERS_AXP202_BAT_CHG_DONE_IRQ         = _BV(10),  //Charge finished, IRQ enable
    XPOWERS_AXP202_BAT_CHG_START_IRQ        = _BV(11),  //Be charging, IRQ enable
    XPOWERS_AXP202_BATT_EXIT_ACTIVATE_IRQ   = _BV(12),  //Exit battery activate mode, IRQ enable
    XPOWERS_AXP202_BATT_ACTIVATE_IRQ        = _BV(13),  //Battery activate mode, IRQ enable
    XPOWERS_AXP202_BAT_REMOVE_IRQ           = _BV(14),  //Battery removed, IRQ enable
    XPOWERS_AXP202_BAT_INSERT_IRQ           = _BV(15),  //Battery connected, IRQ enable

    //! IRQ3 REG 42H
    XPOWERS_AXP202_PKEY_LONG_IRQ            = _BV(16),  //PEK long press, IRQ enable
    XPOWERS_AXP202_PKEY_SHORT_IRQ           = _BV(17),  //PEK short press, IRQ enable
    XPOWERS_AXP202_LDO3_LOW_VOL_IRQ         = _BV(18),  //LDO3output voltage is lower than the set value, IRQ enable
    XPOWERS_AXP202_DC3_LOW_VOL_IRQ          = _BV(19),  //DC-DC3output voltage is lower than the set value, IRQ enable
    XPOWERS_AXP202_DC2_LOW_VOL_IRQ          = _BV(20),  //DC-DC2 output voltage is lower than the set value, IRQ enable
    //**Reserved and unchangeable BIT 5
    XPOWERS_AXP202_CHARGE_LOW_CUR_IRQ       = _BV(22),  //Charge current is lower than the set current, IRQ enable
    XPOWERS_AXP202_CHIP_TEMP_HIGH_IRQ       = _BV(23),  //AXP202 internal over-temperature, IRQ enable

    //! IRQ4 REG 43H
    XPOWERS_AXP202_APS_LOW_VOL_LEVEL2_IRQ   = _BV(24),  //APS low-voltage, IRQ enable（LEVEL2）
    XPOWERS_APX202_APS_LOW_VOL_LEVEL1_IRQ   = _BV(25),  //APS low-voltage, IRQ enable（LEVEL1）
    XPOWERS_AXP202_VBUS_SESSION_END_IRQ     = _BV(26),  //VBUS Session End IRQ enable
    XPOWERS_AXP202_VBUS_SESSION_AB_IRQ      = _BV(27),  //VBUS Session A/B IRQ enable
    XPOWERS_AXP202_VBUS_INVALID_IRQ         = _BV(28),  //VBUS invalid, IRQ enable
    XPOWERS_AXP202_VBUS_VAILD_IRQ           = _BV(29),  //VBUS valid, IRQ enable
    XPOWERS_AXP202_NOE_OFF_IRQ              = _BV(30),  //N_OE shutdown, IRQ enable
    XPOWERS_AXP202_NOE_ON_IRQ               = _BV(31),  //N_OE startup, IRQ enable

    //! IRQ5 REG 44H
    XPOWERS_AXP202_GPIO0_EDGE_TRIGGER_IRQ   = _BV(32),  //GPIO0 input edge trigger, IRQ enable
    XPOWERS_AXP202_GPIO1_EDGE_TRIGGER_IRQ   = _BV(33),  //GPIO1input edge trigger or ADC input, IRQ enable
    XPOWERS_AXP202_GPIO2_EDGE_TRIGGER_IRQ   = _BV(34),  //GPIO2input edge trigger, IRQ enable
    XPOWERS_AXP202_GPIO3_EDGE_TRIGGER_IRQ   = _BV(35),  //GPIO3 input edge trigger, IRQ enable
    //**Reserved and unchangeable BIT 4
    XPOWERS_AXP202_PKEY_NEGATIVE_IRQ        = _BV(37),  //PEK press falling edge, IRQ enable
    XPOWERS_AXP202_PKEY_POSITIVE_IRQ        = _BV(38),  //PEK press rising edge, IRQ enable
    XPOWERS_AXP202_TIMER_TIMEOUT_IRQ        = _BV(39),  //Timer timeout, IRQ enable

    XPOWERS_AXP202_ALL_IRQ                  = (0xFFFFFFFFFFULL)
} xpowers_axp202_irq_t;

























