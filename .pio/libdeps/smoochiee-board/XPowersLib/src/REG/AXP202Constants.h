#pragma once

#define AXP202_SLAVE_ADDRESS                            (0x35)

#define XPOWERS_AXP202_CHIP_ID                          (0x41)

#define XPOWERS_AXP202_STATUS                           (0x00)
#define XPOWERS_AXP202_MODE_CHGSTATUS                   (0x01)
#define XPOWERS_AXP202_OTG_STATUS                       (0x02)
#define XPOWERS_AXP202_IC_TYPE                          (0x03)
#define XPOWERS_AXP202_DATA_BUFFER1                     (0x04)
#define XPOWERS_AXP202_DATA_BUFFER2                     (0x05)
#define XPOWERS_AXP202_DATA_BUFFER3                     (0x06)
#define XPOWERS_AXP202_DATA_BUFFER4                     (0x07)
#define XPOWERS_AXP202_DATA_BUFFER5                     (0x08)
#define XPOWERS_AXP202_DATA_BUFFER6                     (0x09)
#define XPOWERS_AXP202_DATA_BUFFER7                     (0x0A)
#define XPOWERS_AXP202_DATA_BUFFER8                     (0x0B)
#define XPOWERS_AXP202_DATA_BUFFER9                     (0x0C)
#define XPOWERS_AXP202_DATA_BUFFERA                     (0x0D)
#define XPOWERS_AXP202_DATA_BUFFERB                     (0x0E)
#define XPOWERS_AXP202_DATA_BUFFERC                     (0x0F)
#define XPOWERS_AXP202_LDO234_DC23_CTL                  (0x12)
#define XPOWERS_AXP202_DC2OUT_VOL                       (0x23)
#define XPOWERS_AXP202_LDO3_DC2_DVM                     (0x25)
#define XPOWERS_AXP202_DC3OUT_VOL                       (0x27)
#define XPOWERS_AXP202_LDO24OUT_VOL                     (0x28)
#define XPOWERS_AXP202_LDO3OUT_VOL                      (0x29)
#define XPOWERS_AXP202_IPS_SET                          (0x30)
#define XPOWERS_AXP202_VOFF_SET                         (0x31)
#define XPOWERS_AXP202_OFF_CTL                          (0x32)
#define XPOWERS_AXP202_CHARGE1                          (0x33)
#define XPOWERS_AXP202_CHARGE2                          (0x34)
#define XPOWERS_AXP202_BACKUP_CHG                       (0x35)
#define XPOWERS_AXP202_POK_SET                          (0x36)
#define XPOWERS_AXP202_DCDC_FREQSET                     (0x37)
#define XPOWERS_AXP202_VLTF_CHGSET                      (0x38)
#define XPOWERS_AXP202_VHTF_CHGSET                      (0x39)
#define XPOWERS_AXP202_APS_WARNING1                     (0x3A)
#define XPOWERS_AXP202_APS_WARNING2                     (0x3B)
#define XPOWERS_AXP202_TLTF_DISCHGSET                   (0x3C)
#define XPOWERS_AXP202_THTF_DISCHGSET                   (0x3D)
#define XPOWERS_AXP202_DCDC_MODESET                     (0x80)
#define XPOWERS_AXP202_ADC_EN1                          (0x82)
#define XPOWERS_AXP202_ADC_EN2                          (0x83)
#define XPOWERS_AXP202_ADC_SPEED                        (0x84)
#define XPOWERS_AXP202_ADC_INPUTRANGE                   (0x85)
#define XPOWERS_AXP202_ADC_IRQ_RETFSET                  (0x86)
#define XPOWERS_AXP202_ADC_IRQ_FETFSET                  (0x87)
#define XPOWERS_AXP202_TIMER_CTL                        (0x8A)
#define XPOWERS_AXP202_VBUS_DET_SRP                     (0x8B)
#define XPOWERS_AXP202_HOTOVER_CTL                      (0x8F)

#define XPOWERS_AXP202_DATA_BUFFER_SIZE                 (12)
#define XPOWERS_AXP202_GPIO0_CTL                        (0x90)
#define XPOWERS_AXP202_GPIO0_VOL                        (0x91)
#define XPOWERS_AXP202_GPIO1_CTL                        (0x92)
#define XPOWERS_AXP202_GPIO2_CTL                        (0x93)
#define XPOWERS_AXP202_GPIO012_SIGNAL                   (0x94)
#define XPOWERS_AXP202_GPIO3_CTL                        (0x95)

// INTERRUPT REGISTER
#define XPOWERS_AXP202_INTEN1                           (0x40)
#define XPOWERS_AXP202_INTEN2                           (0x41)
#define XPOWERS_AXP202_INTEN3                           (0x42)
#define XPOWERS_AXP202_INTEN4                           (0x43)
#define XPOWERS_AXP202_INTEN5                           (0x44)

//INTERRUPT STATUS REGISTER
#define XPOWERS_AXP202_INTSTS1                          (0x48)
#define XPOWERS_AXP202_INTSTS2                          (0x49)
#define XPOWERS_AXP202_INTSTS3                          (0x4A)
#define XPOWERS_AXP202_INTSTS4                          (0x4B)
#define XPOWERS_AXP202_INTSTS5                          (0x4C)
#define XPOWERS_AXP202_INTSTS_CNT                       (5)


//AXP ADC DATA REGISTER
#define XPOWERS_AXP202_GPIO0_VOL_ADC_H8                 (0x64)
#define XPOWERS_AXP202_GPIO0_VOL_ADC_L4                 (0x65)
#define XPOWERS_AXP202_GPIO1_VOL_ADC_H8                 (0x66)
#define XPOWERS_AXP202_GPIO1_VOL_ADC_L4                 (0x67)


#define XPOWERS_AXP202_GPIO0_STEP                       (0.5F)
#define XPOWERS_AXP202_GPIO1_STEP                       (0.5F)

#define XPOWERS_AXP202_BAT_AVERVOL_H8                   (0x78)
#define XPOWERS_AXP202_BAT_AVERVOL_L4                   (0x79)

#define XPOWERS_AXP202_BAT_AVERCHGCUR_H8                (0x7A)
#define XPOWERS_AXP202_BAT_AVERCHGCUR_L4                (0x7B)

#define XPOWERS_AXP202_BAT_AVERCHGCUR_L5                (0x7B)
#define XPOWERS_AXP202_ACIN_VOL_H8                      (0x56)
#define XPOWERS_AXP202_ACIN_VOL_L4                      (0x57)
#define XPOWERS_AXP202_ACIN_CUR_H8                      (0x58)
#define XPOWERS_AXP202_ACIN_CUR_L4                      (0x59)
#define XPOWERS_AXP202_VBUS_VOL_H8                      (0x5A)
#define XPOWERS_AXP202_VBUS_VOL_L4                      (0x5B)
#define XPOWERS_AXP202_VBUS_CUR_H8                      (0x5C)
#define XPOWERS_AXP202_VBUS_CUR_L4                      (0x5D)
#define XPOWERS_AXP202_INTERNAL_TEMP_H8                 (0x5E)
#define XPOWERS_AXP202_INTERNAL_TEMP_L4                 (0x5F)
#define XPOWERS_AXP202_TS_IN_H8                         (0x62)
#define XPOWERS_AXP202_TS_IN_L4                         (0x63)
#define XPOWERS_AXP202_GPIO0_VOL_ADC_H8                 (0x64)
#define XPOWERS_AXP202_GPIO0_VOL_ADC_L4                 (0x65)
#define XPOWERS_AXP202_GPIO1_VOL_ADC_H8                 (0x66)
#define XPOWERS_AXP202_GPIO1_VOL_ADC_L4                 (0x67)

#define XPOWERS_AXP202_BAT_AVERDISCHGCUR_H8             (0x7C)
#define XPOWERS_AXP202_BAT_AVERDISCHGCUR_L5             (0x7D)
#define XPOWERS_AXP202_APS_AVERVOL_H8                   (0x7E)
#define XPOWERS_AXP202_APS_AVERVOL_L4                   (0x7F)
#define XPOWERS_AXP202_INT_BAT_CHGCUR_H8                (0xA0)
#define XPOWERS_AXP202_INT_BAT_CHGCUR_L4                (0xA1)
#define XPOWERS_AXP202_EXT_BAT_CHGCUR_H8                (0xA2)
#define XPOWERS_AXP202_EXT_BAT_CHGCUR_L4                (0xA3)
#define XPOWERS_AXP202_INT_BAT_DISCHGCUR_H8             (0xA4)
#define XPOWERS_AXP202_INT_BAT_DISCHGCUR_L4             (0xA5)
#define XPOWERS_AXP202_EXT_BAT_DISCHGCUR_H8             (0xA6)
#define XPOWERS_AXP202_EXT_BAT_DISCHGCUR_L4             (0xA7)
#define XPOWERS_AXP202_BAT_CHGCOULOMB3                  (0xB0)
#define XPOWERS_AXP202_BAT_CHGCOULOMB2                  (0xB1)
#define XPOWERS_AXP202_BAT_CHGCOULOMB1                  (0xB2)
#define XPOWERS_AXP202_BAT_CHGCOULOMB0                  (0xB3)
#define XPOWERS_AXP202_BAT_DISCHGCOULOMB3               (0xB4)
#define XPOWERS_AXP202_BAT_DISCHGCOULOMB2               (0xB5)
#define XPOWERS_AXP202_BAT_DISCHGCOULOMB1               (0xB6)
#define XPOWERS_AXP202_BAT_DISCHGCOULOMB0               (0xB7)
#define XPOWERS_AXP202_COULOMB_CTL                      (0xB8)
#define XPOWERS_AXP202_BATT_PERCENTAGE                  (0xB9)

#define XPOWERS_AXP202_BAT_POWERH8                      (0x70)
#define XPOWERS_AXP202_BAT_POWERM8                      (0x71)
#define XPOWERS_AXP202_BAT_POWERL8                      (0x72)

#define XPOWERS_AXP202_BATT_VOLTAGE_STEP                (1.1F)
#define XPOWERS_AXP202_BATT_DISCHARGE_CUR_STEP          (0.5F)
#define XPOWERS_AXP202_BATT_CHARGE_CUR_STEP             (0.5F)
#define XPOWERS_AXP202_ACIN_VOLTAGE_STEP                (1.7F)
#define XPOWERS_AXP202_ACIN_CUR_STEP                    (0.625F)
#define XPOWERS_AXP202_VBUS_VOLTAGE_STEP                (1.7F)
#define XPOWERS_AXP202_VBUS_CUR_STEP                    (0.375F)
#define XPOWERS_AXP202_INTERNAL_TEMP_STEP               (0.1F)
#define XPOWERS_AXP202_APS_VOLTAGE_STEP                 (1.4F)
#define XPOWERS_AXP202_TS_PIN_OUT_STEP                  (0.8F)

#define XPOWERS_AXP202_LDO2_VOL_MIN                     (1800u)
#define XPOWERS_AXP202_LDO2_VOL_MAX                     (3300u)
#define XPOWERS_AXP202_LDO2_VOL_STEPS                   (100u)
#define XPOWERS_AXP202_LDO2_VOL_BIT_MASK                (4u)

#define XPOWERS_AXP202_LDO3_VOL_MIN                     (700u)
#define XPOWERS_AXP202_LDO3_VOL_MAX                     (3500u)
#define XPOWERS_AXP202_LDO3_VOL_STEPS                   (25u)

#define XPOWERS_AXP202_DC2_VOL_STEPS                    (25u)
#define XPOWERS_AXP202_DC2_VOL_MIN                      (700u)
#define XPOWERS_AXP202_DC2_VOL_MAX                      (2275u)

#define XPOWERS_AXP202_DC3_VOL_STEPS                    (25u)
#define XPOWERS_AXP202_DC3_VOL_MIN                      (700u)
#define XPOWERS_AXP202_DC3_VOL_MAX                      (3500u)

#define XPOWERS_AXP202_LDOIO_VOL_STEPS                  (100)
#define XPOWERS_AXP202_LDOIO_VOL_MIN                    (1800)
#define XPOWERS_AXP202_LDOIO_VOL_MAX                    (3300)


#define XPOWERS_AXP202_SYS_VOL_STEPS                    (100)
#define XPOWERS_AXP202_VOFF_VOL_MIN                     (2600)
#define XPOWERS_AXP202_VOFF_VOL_MAX                     (3300)

#define XPOWERS_AXP202_CHG_EXT_CURR_MIN                 (300)
#define XPOWERS_AXP202_CHG_EXT_CURR_MAX                 (1000)
#define XPOWERS_AXP202_CHG_EXT_CURR_STEP                (100)

#define XPOWERS_AXP202_INERNAL_TEMP_OFFSET              (144.7)












