#pragma once

#define AXP192_SLAVE_ADDRESS                            (0x34)

#define XPOWERS_AXP192_CHIP_ID                          (0x03)

#define XPOWERS_AXP192_STATUS                           (0x00)
#define XPOWERS_AXP192_MODE_CHGSTATUS                   (0x01)
#define XPOWERS_AXP192_OTG_STATUS                       (0x02)
#define XPOWERS_AXP192_IC_TYPE                          (0x03)

#define XPOWERS_AXP192_DATA_BUFFER1                     (0x06)
#define XPOWERS_AXP192_DATA_BUFFER2                     (0x07)
#define XPOWERS_AXP192_DATA_BUFFER3                     (0x08)
#define XPOWERS_AXP192_DATA_BUFFER4                     (0x09)
#define XPOWERS_AXP192_DATA_BUFFER5                     (0x0A)
#define XPOWERS_AXP192_DATA_BUFFER6                     (0x0B)
#define XPOWERS_AXP192_DATA_BUFFER_SIZE                 (6)


#define XPOWERS_AXP192_LDO23_DC123_EXT_CTL              (0x12)
#define XPOWERS_AXP192_DC2OUT_VOL                       (0x23)
#define XPOWERS_AXP192_DC2_DVM                          (0x25)
#define XPOWERS_AXP192_DC3OUT_VOL                       (0x27)
#define XPOWERS_AXP192_LDO24OUT_VOL                     (0x28)
#define XPOWERS_AXP192_LDO3OUT_VOL                      (0x29)
#define XPOWERS_AXP192_IPS_SET                          (0x30)
#define XPOWERS_AXP192_VOFF_SET                         (0x31)
#define XPOWERS_AXP192_OFF_CTL                          (0x32)
#define XPOWERS_AXP192_CHARGE1                          (0x33)
#define XPOWERS_AXP192_CHARGE2                          (0x34)
#define XPOWERS_AXP192_BACKUP_CHG                       (0x35)
#define XPOWERS_AXP192_POK_SET                          (0x36)
#define XPOWERS_AXP192_DCDC_FREQSET                     (0x37)
#define XPOWERS_AXP192_VLTF_CHGSET                      (0x38)
#define XPOWERS_AXP192_VHTF_CHGSET                      (0x39)
#define XPOWERS_AXP192_APS_WARNING1                     (0x3A)
#define XPOWERS_AXP192_APS_WARNING2                     (0x3B)
#define XPOWERS_AXP192_TLTF_DISCHGSET                   (0x3C)
#define XPOWERS_AXP192_THTF_DISCHGSET                   (0x3D)
#define XPOWERS_AXP192_DCDC_MODESET                     (0x80)
#define XPOWERS_AXP192_ADC_EN1                          (0x82)
#define XPOWERS_AXP192_ADC_EN2                          (0x83)
#define XPOWERS_AXP192_ADC_SPEED                        (0x84)
#define XPOWERS_AXP192_ADC_INPUTRANGE                   (0x85)
#define XPOWERS_AXP192_ADC_IRQ_RETFSET                  (0x86)
#define XPOWERS_AXP192_ADC_IRQ_FETFSET                  (0x87)
#define XPOWERS_AXP192_TIMER_CTL                        (0x8A)
#define XPOWERS_AXP192_VBUS_DET_SRP                     (0x8B)
#define XPOWERS_AXP192_HOTOVER_CTL                      (0x8F)

#define XPOWERS_AXP192_PWM1_FREQ_SET                    (0x98)
#define XPOWERS_AXP192_PWM1_DUTY_SET1                   (0x99)
#define XPOWERS_AXP192_PWM1_DUTY_SET2                   (0x9A)

#define XPOWERS_AXP192_PWM2_FREQ_SET                    (0x9B)
#define XPOWERS_AXP192_PWM2_DUTY_SET1                   (0x9C)
#define XPOWERS_AXP192_PWM2_DUTY_SET2                   (0x9D)


// INTERRUPT REGISTER
#define XPOWERS_AXP192_INTEN1                           (0x40)
#define XPOWERS_AXP192_INTEN2                           (0x41)
#define XPOWERS_AXP192_INTEN3                           (0x42)
#define XPOWERS_AXP192_INTEN4                           (0x43)
#define XPOWERS_AXP192_INTEN5                           (0x4A)

// INTERRUPT STATUS REGISTER
#define XPOWERS_AXP192_INTSTS1                          (0x44)
#define XPOWERS_AXP192_INTSTS2                          (0x45)
#define XPOWERS_AXP192_INTSTS3                          (0x46)
#define XPOWERS_AXP192_INTSTS4                          (0x47)
#define XPOWERS_AXP192_INTSTS5                          (0x4D)
#define XPOWERS_AXP192_INTSTS_CNT                       (5)

#define XPOWERS_AXP192_DC1_VLOTAGE                      (0x26)
#define XPOWERS_AXP192_LDO23OUT_VOL                     (0x28)
#define XPOWERS_AXP192_GPIO0_CTL                        (0x90)
#define XPOWERS_AXP192_GPIO0_VOL                        (0x91)
#define XPOWERS_AXP192_GPIO1_CTL                        (0X92)
#define XPOWERS_AXP192_GPIO2_CTL                        (0x93)
#define XPOWERS_AXP192_GPIO012_SIGNAL                   (0x94)
#define XPOWERS_AXP192_GPIO34_CTL                       (0x95)
#define XPOWERS_AXP192_GPIO34_SIGNAL                    (0x96)
#define XPOWERS_AXP192_GPIO012_PULLDOWN                 (0x97)
#define XPOWERS_AXP192_GPIO5_CTL                        (0x9E)

#define XPOWERS_AXP192_GPIO0_VOL_ADC_H8                 (0x64)
#define XPOWERS_AXP192_GPIO0_VOL_ADC_L4                 (0x65)
#define XPOWERS_AXP192_GPIO1_VOL_ADC_H8                 (0x66)
#define XPOWERS_AXP192_GPIO1_VOL_ADC_L4                 (0x67)
#define XPOWERS_AXP192_GPIO2_VOL_ADC_H8                 (0x68)
#define XPOWERS_AXP192_GPIO2_VOL_ADC_L4                 (0x69)
#define XPOWERS_AXP192_GPIO3_VOL_ADC_H8                 (0x6A)
#define XPOWERS_AXP192_GPIO3_VOL_ADC_L4                 (0x6B)

#define XPOWERS_AXP192_GPIO0_STEP                       (0.5F)
#define XPOWERS_AXP192_GPIO1_STEP                       (0.5F)
#define XPOWERS_AXP192_TS_IN_H8                         (0x62)
#define XPOWERS_AXP192_TS_IN_L4                         (0x63)

#define XPOWERS_AXP192_BAT_AVERCHGCUR_H8                (0x7A)
#define XPOWERS_AXP192_BAT_AVERCHGCUR_L5                (0x7B)


#define XPOWERS_AXP192_ACIN_VOL_H8                      (0x56)
#define XPOWERS_AXP192_ACIN_VOL_L4                      (0x57)
#define XPOWERS_AXP192_ACIN_CUR_H8                      (0x58)
#define XPOWERS_AXP192_ACIN_CUR_L4                      (0x59)
#define XPOWERS_AXP192_VBUS_VOL_H8                      (0x5A)
#define XPOWERS_AXP192_VBUS_VOL_L4                      (0x5B)
#define XPOWERS_AXP192_VBUS_CUR_H8                      (0x5C)
#define XPOWERS_AXP192_VBUS_CUR_L4                      (0x5D)

#define XPOWERS_AXP192_BAT_AVERDISCHGCUR_H8             (0x7C)
#define XPOWERS_AXP192_BAT_AVERDISCHGCUR_L5             (0x7D)
#define XPOWERS_AXP192_APS_AVERVOL_H8                   (0x7E)
#define XPOWERS_AXP192_APS_AVERVOL_L4                   (0x7F)
#define XPOWERS_AXP192_BAT_AVERVOL_H8                   (0x78)
#define XPOWERS_AXP192_BAT_AVERVOL_L4                   (0x79)

#define XPOWERS_AXP192_BAT_CHGCOULOMB3                  (0xB0)
#define XPOWERS_AXP192_BAT_CHGCOULOMB2                  (0xB1)
#define XPOWERS_AXP192_BAT_CHGCOULOMB1                  (0xB2)
#define XPOWERS_AXP192_BAT_CHGCOULOMB0                  (0xB3)
#define XPOWERS_AXP192_BAT_DISCHGCOULOMB3               (0xB4)
#define XPOWERS_AXP192_BAT_DISCHGCOULOMB2               (0xB5)
#define XPOWERS_AXP192_BAT_DISCHGCOULOMB1               (0xB6)
#define XPOWERS_AXP192_BAT_DISCHGCOULOMB0               (0xB7)
#define XPOWERS_AXP192_COULOMB_CTL                      (0xB8)


#define XPOWERS_AXP192_BATT_VOLTAGE_STEP                (1.1F)
#define XPOWERS_AXP192_BATT_DISCHARGE_CUR_STEP          (0.5F)
#define XPOWERS_AXP192_BATT_CHARGE_CUR_STEP             (0.5F)
#define XPOWERS_AXP192_ACIN_VOLTAGE_STEP                (1.7F)
#define XPOWERS_AXP192_ACIN_CUR_STEP                    (0.625F)
#define XPOWERS_AXP192_VBUS_VOLTAGE_STEP                (1.7F)
#define XPOWERS_AXP192_VBUS_CUR_STEP                    (0.375F)
#define XPOWERS_AXP192_APS_VOLTAGE_STEP                 (1.4F)
#define XPOWERS_AXP192_TS_PIN_OUT_STEP                  (0.8F)


#define XPOWERS_AXP192_LDO2_VOL_MIN                     (1800u)
#define XPOWERS_AXP192_LDO2_VOL_MAX                     (3300u)
#define XPOWERS_AXP192_LDO2_VOL_STEPS                   (100u)
#define XPOWERS_AXP192_LDO2_VOL_BIT_MASK                (4u)

#define XPOWERS_AXP192_LDO3_VOL_MIN                     (1800u)
#define XPOWERS_AXP192_LDO3_VOL_MAX                     (3300u)
#define XPOWERS_AXP192_LDO3_VOL_STEPS                   (100u)


#define XPOWERS_AXP192_DC1_VOL_STEPS                    (25u)
#define XPOWERS_AXP192_DC1_VOL_MIN                      (700u)
#define XPOWERS_AXP192_DC1_VOL_MAX                      (3500u)

#define XPOWERS_AXP192_DC2_VOL_STEPS                    (25u)
#define XPOWERS_AXP192_DC2_VOL_MIN                      (700u)
#define XPOWERS_AXP192_DC2_VOL_MAX                      (3500u)

#define XPOWERS_AXP192_DC3_VOL_STEPS                    (25u)
#define XPOWERS_AXP192_DC3_VOL_MIN                      (700u)
#define XPOWERS_AXP192_DC3_VOL_MAX                      (3500u)

#define XPOWERS_AXP192_LDOIO_VOL_STEPS                  (100)
#define XPOWERS_AXP192_LDOIO_VOL_MIN                    (1800)
#define XPOWERS_AXP192_LDOIO_VOL_MAX                    (3300)

#define XPOWERS_AXP192_SYS_VOL_STEPS                    (100)
#define XPOWERS_AXP192_VOFF_VOL_MIN                     (2600)
#define XPOWERS_AXP192_VOFF_VOL_MAX                     (3300)

#define XPOWERS_AXP192_CHG_EXT_CURR_MIN                 (300)
#define XPOWERS_AXP192_CHG_EXT_CURR_MAX                 (1000)
#define XPOWERS_AXP192_CHG_EXT_CURR_STEP                (100)


#define XPOWERS_AXP192_INTERNAL_TEMP_H8                 (0x5E)
#define XPOWERS_AXP192_INTERNAL_TEMP_L4                 (0x5F)
#define XPOWERS_AXP192_INTERNAL_TEMP_STEP               (0.1F)
#define XPOWERS_AXP192_INERNAL_TEMP_OFFSET              (144.7)