#pragma once

#define AXP2101_SLAVE_ADDRESS                            (0x34)

#define XPOWERS_AXP2101_CHIP_ID                          (0x4A)

#define XPOWERS_AXP2101_STATUS1                          (0x00)
#define XPOWERS_AXP2101_STATUS2                          (0x01)
#define XPOWERS_AXP2101_IC_TYPE                          (0x03)


#define XPOWERS_AXP2101_DATA_BUFFER1                     (0x04)
#define XPOWERS_AXP2101_DATA_BUFFER2                     (0x05)
#define XPOWERS_AXP2101_DATA_BUFFER3                     (0x06)
#define XPOWERS_AXP2101_DATA_BUFFER4                     (0x07)
#define XPOWERS_AXP2101_DATA_BUFFER_SIZE                 (4u)

#define XPOWERS_AXP2101_COMMON_CONFIG                    (0x10)
#define XPOWERS_AXP2101_BATFET_CTRL                      (0x12)
#define XPOWERS_AXP2101_DIE_TEMP_CTRL                    (0x13)
#define XPOWERS_AXP2101_MIN_SYS_VOL_CTRL                 (0x14)
#define XPOWERS_AXP2101_INPUT_VOL_LIMIT_CTRL             (0x15)
#define XPOWERS_AXP2101_INPUT_CUR_LIMIT_CTRL             (0x16)
#define XPOWERS_AXP2101_RESET_FUEL_GAUGE                 (0x17)
#define XPOWERS_AXP2101_CHARGE_GAUGE_WDT_CTRL            (0x18)


#define XPOWERS_AXP2101_WDT_CTRL                         (0x19)
#define XPOWERS_AXP2101_LOW_BAT_WARN_SET                 (0x1A)


#define XPOWERS_AXP2101_PWRON_STATUS                     (0x20)
#define XPOWERS_AXP2101_PWROFF_STATUS                    (0x21)
#define XPOWERS_AXP2101_PWROFF_EN                        (0x22)
#define XPOWERS_AXP2101_DC_OVP_UVP_CTRL                  (0x23)
#define XPOWERS_AXP2101_VOFF_SET                         (0x24)
#define XPOWERS_AXP2101_PWROK_SEQU_CTRL                  (0x25)
#define XPOWERS_AXP2101_SLEEP_WAKEUP_CTRL                (0x26)
#define XPOWERS_AXP2101_IRQ_OFF_ON_LEVEL_CTRL            (0x27)

#define XPOWERS_AXP2101_FAST_PWRON_SET0                  (0x28)
#define XPOWERS_AXP2101_FAST_PWRON_SET1                  (0x29)
#define XPOWERS_AXP2101_FAST_PWRON_SET2                  (0x2A)
#define XPOWERS_AXP2101_FAST_PWRON_CTRL                  (0x2B)

#define XPOWERS_AXP2101_ADC_CHANNEL_CTRL                 (0x30)
#define XPOWERS_AXP2101_ADC_DATA_RELUST0                 (0x34)
#define XPOWERS_AXP2101_ADC_DATA_RELUST1                 (0x35)
#define XPOWERS_AXP2101_ADC_DATA_RELUST2                 (0x36)
#define XPOWERS_AXP2101_ADC_DATA_RELUST3                 (0x37)
#define XPOWERS_AXP2101_ADC_DATA_RELUST4                 (0x38)
#define XPOWERS_AXP2101_ADC_DATA_RELUST5                 (0x39)
#define XPOWERS_AXP2101_ADC_DATA_RELUST6                 (0x3A)
#define XPOWERS_AXP2101_ADC_DATA_RELUST7                 (0x3B)
#define XPOWERS_AXP2101_ADC_DATA_RELUST8                 (0x3C)
#define XPOWERS_AXP2101_ADC_DATA_RELUST9                 (0x3D)


//XPOWERS INTERRUPT REGISTER
#define XPOWERS_AXP2101_INTEN1                           (0x40)
#define XPOWERS_AXP2101_INTEN2                           (0x41)
#define XPOWERS_AXP2101_INTEN3                           (0x42)


//XPOWERS INTERRUPT STATUS REGISTER
#define XPOWERS_AXP2101_INTSTS1                          (0x48)
#define XPOWERS_AXP2101_INTSTS2                          (0x49)
#define XPOWERS_AXP2101_INTSTS3                          (0x4A)
#define XPOWERS_AXP2101_INTSTS_CNT                       (3)

#define XPOWERS_AXP2101_TS_PIN_CTRL                      (0x50)
#define XPOWERS_AXP2101_TS_HYSL2H_SET                    (0x52)
#define XPOWERS_AXP2101_TS_LYSL2H_SET                    (0x53)


#define XPOWERS_AXP2101_VLTF_CHG_SET                     (0x54)
#define XPOWERS_AXP2101_VHLTF_CHG_SET                    (0x55)
#define XPOWERS_AXP2101_VLTF_WORK_SET                    (0x56)
#define XPOWERS_AXP2101_VHLTF_WORK_SET                   (0x57)


#define XPOWERS_AXP2101_JIETA_EN_CTRL                    (0x58)
#define XPOWERS_AXP2101_JIETA_SET0                       (0x59)
#define XPOWERS_AXP2101_JIETA_SET1                       (0x5A)
#define XPOWERS_AXP2101_JIETA_SET2                       (0x5B)


#define XPOWERS_AXP2101_IPRECHG_SET                      (0x61)
#define XPOWERS_AXP2101_ICC_CHG_SET                      (0x62)
#define XPOWERS_AXP2101_ITERM_CHG_SET_CTRL               (0x63)

#define XPOWERS_AXP2101_CV_CHG_VOL_SET                   (0x64)

#define XPOWERS_AXP2101_THE_REGU_THRES_SET               (0x65)
#define XPOWERS_AXP2101_CHG_TIMEOUT_SET_CTRL             (0x67)

#define XPOWERS_AXP2101_BAT_DET_CTRL                     (0x68)
#define XPOWERS_AXP2101_CHGLED_SET_CTRL                  (0x69)

#define XPOWERS_AXP2101_BTN_VOL_MIN                      (2600)
#define XPOWERS_AXP2101_BTN_VOL_MAX                      (3300)
#define XPOWERS_AXP2101_BTN_VOL_STEPS                    (100)


#define XPOWERS_AXP2101_BTN_BAT_CHG_VOL_SET              (0x6A)


#define XPOWERS_AXP2101_DC_ONOFF_DVM_CTRL                (0x80)
#define XPOWERS_AXP2101_DC_FORCE_PWM_CTRL                (0x81)
#define XPOWERS_AXP2101_DC_VOL0_CTRL                     (0x82)
#define XPOWERS_AXP2101_DC_VOL1_CTRL                     (0x83)
#define XPOWERS_AXP2101_DC_VOL2_CTRL                     (0x84)
#define XPOWERS_AXP2101_DC_VOL3_CTRL                     (0x85)
#define XPOWERS_AXP2101_DC_VOL4_CTRL                     (0x86)


#define XPOWERS_AXP2101_LDO_ONOFF_CTRL0                  (0x90)
#define XPOWERS_AXP2101_LDO_ONOFF_CTRL1                  (0x91)
#define XPOWERS_AXP2101_LDO_VOL0_CTRL                    (0x92)
#define XPOWERS_AXP2101_LDO_VOL1_CTRL                    (0x93)
#define XPOWERS_AXP2101_LDO_VOL2_CTRL                    (0x94)
#define XPOWERS_AXP2101_LDO_VOL3_CTRL                    (0x95)
#define XPOWERS_AXP2101_LDO_VOL4_CTRL                    (0x96)
#define XPOWERS_AXP2101_LDO_VOL5_CTRL                    (0x97)
#define XPOWERS_AXP2101_LDO_VOL6_CTRL                    (0x98)
#define XPOWERS_AXP2101_LDO_VOL7_CTRL                    (0x99)
#define XPOWERS_AXP2101_LDO_VOL8_CTRL                    (0x9A)


#define XPOWERS_AXP2101_BAT_PARAME                       (0xA1)
#define XPOWERS_AXP2101_FUEL_GAUGE_CTRL                  (0xA2)
#define XPOWERS_AXP2101_BAT_PERCENT_DATA                 (0xA4)

// DCDC 1~5
#define XPOWERS_AXP2101_DCDC1_VOL_MIN                    (1500)
#define XPOWERS_AXP2101_DCDC1_VOL_MAX                    (3400)
#define XPOWERS_AXP2101_DCDC1_VOL_STEPS                  (100u)

#define XPOWERS_AXP2101_DCDC2_VOL1_MIN                  (500u)
#define XPOWERS_AXP2101_DCDC2_VOL1_MAX                  (1200u)
#define XPOWERS_AXP2101_DCDC2_VOL2_MIN                  (1220u)
#define XPOWERS_AXP2101_DCDC2_VOL2_MAX                  (1540u)

#define XPOWERS_AXP2101_DCDC2_VOL_STEPS1                 (10u)
#define XPOWERS_AXP2101_DCDC2_VOL_STEPS2                 (20u)

#define XPOWERS_AXP2101_DCDC2_VOL_STEPS1_BASE            (0u)
#define XPOWERS_AXP2101_DCDC2_VOL_STEPS2_BASE            (71)


#define XPOWERS_AXP2101_DCDC3_VOL1_MIN                  (500u)
#define XPOWERS_AXP2101_DCDC3_VOL1_MAX                  (1200u)
#define XPOWERS_AXP2101_DCDC3_VOL2_MIN                  (1220u)
#define XPOWERS_AXP2101_DCDC3_VOL2_MAX                  (1540u)
#define XPOWERS_AXP2101_DCDC3_VOL3_MIN                  (1600u)
#define XPOWERS_AXP2101_DCDC3_VOL3_MAX                  (3400u)

#define XPOWERS_AXP2101_DCDC3_VOL_MIN                    (500)
#define XPOWERS_AXP2101_DCDC3_VOL_MAX                    (3400)

#define XPOWERS_AXP2101_DCDC3_VOL_STEPS1                 (10u)
#define XPOWERS_AXP2101_DCDC3_VOL_STEPS2                 (20u)
#define XPOWERS_AXP2101_DCDC3_VOL_STEPS3                 (100u)

#define XPOWERS_AXP2101_DCDC3_VOL_STEPS1_BASE            (0u)
#define XPOWERS_AXP2101_DCDC3_VOL_STEPS2_BASE            (71)
#define XPOWERS_AXP2101_DCDC3_VOL_STEPS3_BASE            (88)



#define XPOWERS_AXP2101_DCDC4_VOL1_MIN                  (500u)
#define XPOWERS_AXP2101_DCDC4_VOL1_MAX                  (1200u)
#define XPOWERS_AXP2101_DCDC4_VOL2_MIN                  (1220u)
#define XPOWERS_AXP2101_DCDC4_VOL2_MAX                  (1840u)

#define XPOWERS_AXP2101_DCDC4_VOL_STEPS1                 (10u)
#define XPOWERS_AXP2101_DCDC4_VOL_STEPS2                 (20u)

#define XPOWERS_AXP2101_DCDC4_VOL_STEPS1_BASE            (0u)
#define XPOWERS_AXP2101_DCDC4_VOL_STEPS2_BASE            (71)



#define XPOWERS_AXP2101_DCDC5_VOL_1200MV                 (1200)
#define XPOWERS_AXP2101_DCDC5_VOL_VAL                    (0x19)
#define XPOWERS_AXP2101_DCDC5_VOL_MIN                    (1400)
#define XPOWERS_AXP2101_DCDC5_VOL_MAX                    (3700)
#define XPOWERS_AXP2101_DCDC5_VOL_STEPS                  (100u)

#define XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_MIN          (2600)
#define XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_MAX          (3300)
#define XPOWERS_AXP2101_VSYS_VOL_THRESHOLD_STEPS        (100)

// ALDO 1~4

#define XPOWERS_AXP2101_ALDO1_VOL_MIN                    (500)
#define XPOWERS_AXP2101_ALDO1_VOL_MAX                    (3500)
#define XPOWERS_AXP2101_ALDO1_VOL_STEPS                  (100u)

#define XPOWERS_AXP2101_ALDO2_VOL_MIN                    (500)
#define XPOWERS_AXP2101_ALDO2_VOL_MAX                    (3500)
#define XPOWERS_AXP2101_ALDO2_VOL_STEPS                  (100u)


#define XPOWERS_AXP2101_ALDO3_VOL_MIN                    (500)
#define XPOWERS_AXP2101_ALDO3_VOL_MAX                    (3500)
#define XPOWERS_AXP2101_ALDO3_VOL_STEPS                  (100u)


#define XPOWERS_AXP2101_ALDO4_VOL_MIN                    (500)
#define XPOWERS_AXP2101_ALDO4_VOL_MAX                    (3500)
#define XPOWERS_AXP2101_ALDO4_VOL_STEPS                  (100u)

// BLDO 1~2

#define XPOWERS_AXP2101_BLDO1_VOL_MIN                    (500)
#define XPOWERS_AXP2101_BLDO1_VOL_MAX                    (3500)
#define XPOWERS_AXP2101_BLDO1_VOL_STEPS                  (100u)

#define XPOWERS_AXP2101_BLDO2_VOL_MIN                    (500)
#define XPOWERS_AXP2101_BLDO2_VOL_MAX                    (3500)
#define XPOWERS_AXP2101_BLDO2_VOL_STEPS                  (100u)

// CPUSLDO

#define XPOWERS_AXP2101_CPUSLDO_VOL_MIN                  (500)
#define XPOWERS_AXP2101_CPUSLDO_VOL_MAX                  (1400)
#define XPOWERS_AXP2101_CPUSLDO_VOL_STEPS                (50)


// DLDO 1~2
#define XPOWERS_AXP2101_DLDO1_VOL_MIN                  (500)
#define XPOWERS_AXP2101_DLDO1_VOL_MAX                  (3400)
#define XPOWERS_AXP2101_DLDO1_VOL_STEPS                (100u)

#define XPOWERS_AXP2101_DLDO2_VOL_MIN                  (500)
#define XPOWERS_AXP2101_DLDO2_VOL_MAX                  (3400)
#define XPOWERS_AXP2101_DLDO2_VOL_STEPS                (100u)


#define XPOWERS_AXP2101_CONVERSION(raw)                 (22.0 + (7274 - raw) / 20.0)

