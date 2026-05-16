#if !defined(_RADIOLIB_SX126X_REGISTERS_H)
#define _RADIOLIB_SX126X_REGISTERS_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_SX126X

// SX126X register map
#define RADIOLIB_SX126X_REG_BPSK_PACKET_PARAMS                  0x00F0
#define RADIOLIB_SX126X_REG_RX_GAIN_RETENTION_0                 0x029F // SX1268 datasheet v1.1, section 9.6
#define RADIOLIB_SX126X_REG_RX_GAIN_RETENTION_1                 0x02A0 // SX1268 datasheet v1.1, section 9.6
#define RADIOLIB_SX126X_REG_RX_GAIN_RETENTION_2                 0x02A1 // SX1268 datasheet v1.1, section 9.6
#define RADIOLIB_SX126X_REG_VERSION_STRING                      0x0320
#define RADIOLIB_SX126X_REG_HOPPING_ENABLE                      0x0385
#define RADIOLIB_SX126X_REG_LR_FHSS_PACKET_LENGTH               0x0386
#define RADIOLIB_SX126X_REG_LR_FHSS_NUM_HOPPING_BLOCKS          0x0387
#define RADIOLIB_SX126X_REG_LR_FHSS_NUM_SYMBOLS_FREQX_MSB(X)    (0x0388 + (X)*6)
#define RADIOLIB_SX126X_REG_LR_FHSS_NUM_SYMBOLS_FREQX_LSB(X)    (0x0389 + (X)*6)
#define RADIOLIB_SX126X_REG_LR_FHSS_FREQX_0(X)                  (0x038A + (X)*6)
#define RADIOLIB_SX126X_REG_LR_FHSS_FREQX_1(X)                  (0x038B + (X)*6)
#define RADIOLIB_SX126X_REG_LR_FHSS_FREQX_2(X)                  (0x038C + (X)*6)
#define RADIOLIB_SX126X_REG_LR_FHSS_FREQX_3(X)                  (0x038D + (X)*6)
#define RADIOLIB_SX126X_REG_SPECTRAL_SCAN_RESULT                0x0401
#define RADIOLIB_SX126X_REG_DIOX_OUT_ENABLE                     0x0580
#define RADIOLIB_SX126X_REG_DIOX_DRIVE_STRENGTH                 0x0582
#define RADIOLIB_SX126X_REG_DIOX_IN_ENABLE                      0x0583
#define RADIOLIB_SX126X_REG_DIOX_PULL_UP_CTRL                   0x0584
#define RADIOLIB_SX126X_REG_DIOX_PULL_DOWN_CTRL                 0x0585
#define RADIOLIB_SX126X_REG_TX_BITBANG_ENABLE_0                 0x0587
#define RADIOLIB_SX126X_REG_PATCH_UPDATE_ENABLE                 0x0610
#define RADIOLIB_SX126X_REG_TX_BITBANG_ENABLE_1                 0x0680
#define RADIOLIB_SX126X_REG_GFSK_FIX_4                          0x06AC
#define RADIOLIB_SX126X_REG_WHITENING_INITIAL_MSB               0x06B8
#define RADIOLIB_SX126X_REG_WHITENING_INITIAL_LSB               0x06B9
#define RADIOLIB_SX126X_REG_RX_TX_PLD_LEN                       0x06BB
#define RADIOLIB_SX126X_REG_CRC_INITIAL_MSB                     0x06BC
#define RADIOLIB_SX126X_REG_CRC_INITIAL_LSB                     0x06BD
#define RADIOLIB_SX126X_REG_CRC_POLYNOMIAL_MSB                  0x06BE
#define RADIOLIB_SX126X_REG_CRC_POLYNOMIAL_LSB                  0x06BF
#define RADIOLIB_SX126X_REG_SYNC_WORD_0                         0x06C0
#define RADIOLIB_SX126X_REG_SYNC_WORD_1                         0x06C1
#define RADIOLIB_SX126X_REG_SYNC_WORD_2                         0x06C2
#define RADIOLIB_SX126X_REG_SYNC_WORD_3                         0x06C3
#define RADIOLIB_SX126X_REG_SYNC_WORD_4                         0x06C4
#define RADIOLIB_SX126X_REG_SYNC_WORD_5                         0x06C5
#define RADIOLIB_SX126X_REG_SYNC_WORD_6                         0x06C6
#define RADIOLIB_SX126X_REG_SYNC_WORD_7                         0x06C7
#define RADIOLIB_SX126X_REG_NODE_ADDRESS                        0x06CD
#define RADIOLIB_SX126X_REG_BROADCAST_ADDRESS                   0x06CE
#define RADIOLIB_SX126X_REG_GFSK_FIX_1                          0x06D1
#define RADIOLIB_SX126X_REG_PAYLOAD_LENGTH                      0x0702
#define RADIOLIB_SX126X_REG_PACKET_PARAMS                       0x0704
#define RADIOLIB_SX126X_REG_LORA_SYNC_TIMEOUT                   0x0706
#define RADIOLIB_SX126X_REG_IQ_CONFIG                           0x0736
#define RADIOLIB_SX126X_REG_LORA_SYNC_WORD_MSB                  0x0740
#define RADIOLIB_SX126X_REG_LORA_SYNC_WORD_LSB                  0x0741
#define RADIOLIB_SX126X_REG_LORA_RX_CODING_RATE                 0x0749
#define RADIOLIB_SX126X_REG_FREQ_ERROR_RX_CRC                   0x076B
#define RADIOLIB_SX126X_REG_SPECTRAL_SCAN_STATUS                0x07CD
#define RADIOLIB_SX126X_REG_RX_ADDR_PTR                         0x0803
#define RADIOLIB_SX126X_REG_RANDOM_NUMBER_0                     0x0819
#define RADIOLIB_SX126X_REG_RANDOM_NUMBER_1                     0x081A
#define RADIOLIB_SX126X_REG_RANDOM_NUMBER_2                     0x081B
#define RADIOLIB_SX126X_REG_RANDOM_NUMBER_3                     0x081C
#define RADIOLIB_SX126X_REG_SENSITIVITY_CONFIG                  0x0889 // SX1268 datasheet v1.1, section 15.1
#define RADIOLIB_SX126X_REG_RF_FREQUENCY_0                      0x088B
#define RADIOLIB_SX126X_REG_RF_FREQUENCY_1                      0x088C
#define RADIOLIB_SX126X_REG_RF_FREQUENCY_2                      0x088D
#define RADIOLIB_SX126X_REG_RF_FREQUENCY_3                      0x088E
#define RADIOLIB_SX126X_REG_RSSI_AVG_WINDOW                     0x089B
#define RADIOLIB_SX126X_REG_RX_GAIN                             0x08AC
#define RADIOLIB_SX126X_REG_GFSK_FIX_3                          0x08B8
#define RADIOLIB_SX126X_REG_TX_CLAMP_CONFIG                     0x08D8
#define RADIOLIB_SX126X_REG_ANA_LNA                             0x08E2
#define RADIOLIB_SX126X_REG_LNA_CAP_TUNE_N                      0x08E3
#define RADIOLIB_SX126X_REG_LNA_CAP_TUNE_P                      0x08E4
#define RADIOLIB_SX126X_REG_ANA_MIXER                           0x08E5
#define RADIOLIB_SX126X_REG_OCP_CONFIGURATION                   0x08E7
#define RADIOLIB_SX126X_REG_RTC_CTRL                            0x0902
#define RADIOLIB_SX126X_REG_XTA_TRIM                            0x0911
#define RADIOLIB_SX126X_REG_XTB_TRIM                            0x0912
#define RADIOLIB_SX126X_REG_DIO3_OUT_VOLTAGE_CTRL               0x0920
#define RADIOLIB_SX126X_REG_EVENT_MASK                          0x0944
#define RADIOLIB_SX126X_REG_PATCH_MEMORY_BASE                   0x8000

// SX126X SPI register variables
//RADIOLIB_SX126X_REG_HOPPING_ENABLE                                          MSB   LSB   DESCRIPTION
#define RADIOLIB_SX126X_HOPPING_ENABLED                         0b00000001  //  0     0   intra-packet hopping for LR-FHSS: enabled
#define RADIOLIB_SX126X_HOPPING_DISABLED                        0b00000000  //  0     0                                     (disabled)

//RADIOLIB_SX126X_REG_LORA_SYNC_WORD_MSB + LSB
#define RADIOLIB_SX126X_SYNC_WORD_PUBLIC                        0x34        // actually 0x3444  NOTE: The low nibbles in each byte (0x_4_4) are masked out since apparently, they're reserved.
#define RADIOLIB_SX126X_SYNC_WORD_PRIVATE                       0x12        // actually 0x1424        You couldn't make this up if you tried.

// RADIOLIB_SX126X_REG_TX_BITBANG_ENABLE_1
#define RADIOLIB_SX126X_TX_BITBANG_1_DISABLED                   0b00000000  //  6     4   Tx bitbang: disabled (default)
#define RADIOLIB_SX126X_TX_BITBANG_1_ENABLED                    0b00010000  //  6     4               enabled

// RADIOLIB_SX126X_REG_TX_BITBANG_ENABLE_0
#define RADIOLIB_SX126X_TX_BITBANG_0_DISABLED                   0b00000000  //  3     0   Tx bitbang: disabled (default)
#define RADIOLIB_SX126X_TX_BITBANG_0_ENABLED                    0b00001100  //  3     0               enabled

// RADIOLIB_SX126X_REG_DIOX_OUT_ENABLE
#define RADIOLIB_SX126X_DIO1_OUT_DISABLED                       0b00000010  //  1     1   DIO1 output: disabled
#define RADIOLIB_SX126X_DIO1_OUT_ENABLED                        0b00000000  //  1     1                enabled
#define RADIOLIB_SX126X_DIO2_OUT_DISABLED                       0b00000100  //  2     2   DIO2 output: disabled
#define RADIOLIB_SX126X_DIO2_OUT_ENABLED                        0b00000000  //  2     2                enabled
#define RADIOLIB_SX126X_DIO3_OUT_DISABLED                       0b00001000  //  3     3   DIO3 output: disabled
#define RADIOLIB_SX126X_DIO3_OUT_ENABLED                        0b00000000  //  3     3                enabled

// RADIOLIB_SX126X_REG_DIOX_IN_ENABLE
#define RADIOLIB_SX126X_DIO1_IN_DISABLED                        0b00000000  //  1     1   DIO1 input: disabled
#define RADIOLIB_SX126X_DIO1_IN_ENABLED                         0b00000010  //  1     1               enabled
#define RADIOLIB_SX126X_DIO2_IN_DISABLED                        0b00000000  //  2     2   DIO2 input: disabled
#define RADIOLIB_SX126X_DIO2_IN_ENABLED                         0b00000100  //  2     2               enabled
#define RADIOLIB_SX126X_DIO3_IN_DISABLED                        0b00000000  //  3     3   DIO3 input: disabled
#define RADIOLIB_SX126X_DIO3_IN_ENABLED                         0b00001000  //  3     3               enabled

// RADIOLIB_SX126X_REG_RX_GAIN
#define RADIOLIB_SX126X_RX_GAIN_BOOSTED                         0x96        //  7     0   Rx gain: boosted
#define RADIOLIB_SX126X_RX_GAIN_POWER_SAVING                    0x94        //  7     0            power saving
#define RADIOLIB_SX126X_RX_GAIN_SPECTRAL_SCAN                   0xCB        //  7     0            spectral scan

// RADIOLIB_SX126X_REG_PATCH_UPDATE_ENABLE
#define RADIOLIB_SX126X_PATCH_UPDATE_DISABLED                   0b00000000  //  4     4   patch update: disabled
#define RADIOLIB_SX126X_PATCH_UPDATE_ENABLED                    0b00010000  //  4     4                 enabled

// RADIOLIB_SX126X_REG_SPECTRAL_SCAN_STATUS
#define RADIOLIB_SX126X_SPECTRAL_SCAN_NONE                      0x00        //  7     0   spectral scan status: none
#define RADIOLIB_SX126X_SPECTRAL_SCAN_ONGOING                   0x0F        //  7     0                         ongoing
#define RADIOLIB_SX126X_SPECTRAL_SCAN_ABORTED                   0xF0        //  7     0                         aborted
#define RADIOLIB_SX126X_SPECTRAL_SCAN_COMPLETED                 0xFF        //  7     0                         completed

// RADIOLIB_SX126X_REG_RSSI_AVG_WINDOW
#define RADIOLIB_SX126X_SPECTRAL_SCAN_WINDOW_DEFAULT            (0x05 << 2) //  7     0   default RSSI average window

// RADIOLIB_SX126X_REG_ANA_LNA
#define RADIOLIB_SX126X_LNA_RNG_DISABLED                        0b00000001  //  0     0   random number: disabled
#define RADIOLIB_SX126X_LNA_RNG_ENABLED                         0b00000000  //  0     0                  enabled

// RADIOLIB_SX126X_REG_ANA_MIXER
#define RADIOLIB_SX126X_MIXER_RNG_DISABLED                      0b00000001  //  7     7   random number: disabled
#define RADIOLIB_SX126X_MIXER_RNG_ENABLED                       0b00000000  //  7     7                  enabled

// size of the spectral scan result
#define RADIOLIB_SX126X_SPECTRAL_SCAN_RES_SIZE                  (33)

// LR-FHSS configuration
#define RADIOLIB_SX126X_LR_FHSS_CR_5_6                          (0x00UL << 0)   //  7     0     LR FHSS coding rate: 5/6
#define RADIOLIB_SX126X_LR_FHSS_CR_2_3                          (0x01UL << 0)   //  7     0                          2/3
#define RADIOLIB_SX126X_LR_FHSS_CR_1_2                          (0x02UL << 0)   //  7     0                          1/2
#define RADIOLIB_SX126X_LR_FHSS_CR_1_3                          (0x03UL << 0)   //  7     0                          1/3
#define RADIOLIB_SX126X_LR_FHSS_MOD_TYPE_GMSK                   (0x00UL << 0)   //  7     0     LR FHSS modulation: GMSK
#define RADIOLIB_SX126X_LR_FHSS_GRID_STEP_FCC                   (0x00UL << 0)   //  7     0     LR FHSS step size: 25.390625 kHz (FCC)
#define RADIOLIB_SX126X_LR_FHSS_GRID_STEP_NON_FCC               (0x01UL << 0)   //  7     0                        3.90625 kHz (non-FCC)
#define RADIOLIB_SX126X_LR_FHSS_HOPPING_DISABLED                (0x00UL << 0)   //  7     0     LR FHSS hopping: disabled
#define RADIOLIB_SX126X_LR_FHSS_HOPPING_ENABLED                 (0x01UL << 0)   //  7     0                      enabled
#define RADIOLIB_SX126X_LR_FHSS_BW_39_06                        (0x00UL << 0)   //  7     0     LR FHSS bandwidth: 39.06 kHz
#define RADIOLIB_SX126X_LR_FHSS_BW_85_94                        (0x01UL << 0)   //  7     0                        85.94 kHz
#define RADIOLIB_SX126X_LR_FHSS_BW_136_72                       (0x02UL << 0)   //  7     0                        136.72 kHz
#define RADIOLIB_SX126X_LR_FHSS_BW_183_59                       (0x03UL << 0)   //  7     0                        183.59 kHz
#define RADIOLIB_SX126X_LR_FHSS_BW_335_94                       (0x04UL << 0)   //  7     0                        335.94 kHz
#define RADIOLIB_SX126X_LR_FHSS_BW_386_72                       (0x05UL << 0)   //  7     0                        386.72 kHz
#define RADIOLIB_SX126X_LR_FHSS_BW_722_66                       (0x06UL << 0)   //  7     0                        722.66 kHz
#define RADIOLIB_SX126X_LR_FHSS_BW_773_44                       (0x07UL << 0)   //  7     0                        773.44 kHz
#define RADIOLIB_SX126X_LR_FHSS_BW_1523_4                       (0x08UL << 0)   //  7     0                        1523.4 kHz
#define RADIOLIB_SX126X_LR_FHSS_BW_1574_2                       (0x09UL << 0)   //  7     0                        1574.2 kHz

#endif

#endif
