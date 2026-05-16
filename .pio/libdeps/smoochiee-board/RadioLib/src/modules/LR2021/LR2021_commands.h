#if !defined(RADIOLIB_LR2021_COMMANDS_H)
#define RADIOLIB_LR2021_COMMANDS_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR2021

// LR2021 SPI commands
#define RADIOLIB_LR2021_CMD_NOP                                 (0x0000)
#define RADIOLIB_LR2021_CMD_READ_RX_FIFO                        (0x0001)
#define RADIOLIB_LR2021_CMD_WRITE_TX_FIFO                       (0x0002)
#define RADIOLIB_LR2021_CMD_WRITE_REG_MEM_32                    (0x0104)
#define RADIOLIB_LR2021_CMD_WRITE_REG_MEM_MASK_32               (0x0105)
#define RADIOLIB_LR2021_CMD_READ_REG_MEM_32                     (0x0106)
#define RADIOLIB_LR2021_CMD_SET_SLEEP                           (0x0127)
#define RADIOLIB_LR2021_CMD_SET_STANDBY                         (0x0128)
#define RADIOLIB_LR2021_CMD_SET_FS                              (0x0129)
#define RADIOLIB_LR2021_CMD_SET_ADDITIONAL_REG_TO_RETAIN        (0x012A)
#define RADIOLIB_LR2021_CMD_SET_RX                              (0x020C)
#define RADIOLIB_LR2021_CMD_SET_TX                              (0x020D)
#define RADIOLIB_LR2021_CMD_SET_RX_TX_FALLBACK_MODE             (0x0206)
#define RADIOLIB_LR2021_CMD_SET_RX_DUTY_CYCLE                   (0x0210)
#define RADIOLIB_LR2021_CMD_SET_AUTO_RX_TX                      (0x0211)
#define RADIOLIB_LR2021_CMD_GET_RX_PKT_LENGTH                   (0x0212)
#define RADIOLIB_LR2021_CMD_STOP_TIMEOUT_ON_PREAMBLE            (0x0209)
#define RADIOLIB_LR2021_CMD_RESET_RX_STATS                      (0x020A)
#define RADIOLIB_LR2021_CMD_SET_DEFAULT_RX_TX_TIMEOUT           (0x0215)
#define RADIOLIB_LR2021_CMD_SET_REG_MODE                        (0x0121)
#define RADIOLIB_LR2021_CMD_CALIBRATE                           (0x0122)
#define RADIOLIB_LR2021_CMD_CALIB_FRONT_END                     (0x0123)
#define RADIOLIB_LR2021_CMD_GET_V_BAT                           (0x0124)
#define RADIOLIB_LR2021_CMD_GET_TEMP                            (0x0125)
#define RADIOLIB_LR2021_CMD_SET_EOL_CONFIG                      (0x0130)
#define RADIOLIB_LR2021_CMD_GET_RANDOM_NUMBER                   (0x0126)
#define RADIOLIB_LR2021_CMD_GET_STATUS                          (0x0100)
#define RADIOLIB_LR2021_CMD_GET_VERSION                         (0x0101)
#define RADIOLIB_LR2021_CMD_CLEAR_ERRORS                        (0x0111)
#define RADIOLIB_LR2021_CMD_GET_ERRORS                          (0x0110)
#define RADIOLIB_LR2021_CMD_SET_DIO_FUNCTION                    (0x0112)
#define RADIOLIB_LR2021_CMD_SET_DIO_RF_SWITCH_CONFIG            (0x0113)
#define RADIOLIB_LR2021_CMD_SET_DIO_IRQ_CONFIG                  (0x0115)
#define RADIOLIB_LR2021_CMD_CLEAR_IRQ                           (0x0116)
#define RADIOLIB_LR2021_CMD_GET_AND_CLEAR_IRQ_STATUS            (0x0117)
#define RADIOLIB_LR2021_CMD_CONFIG_FIFO_IRQ                     (0x011A)
#define RADIOLIB_LR2021_CMD_GET_FIFO_IRQ_FLAGS                  (0x011B)
#define RADIOLIB_LR2021_CMD_CLEAR_FIFO_IRQ_FLAGS                (0x0114)
#define RADIOLIB_LR2021_CMD_GET_AND_CLEAR_FIFO_IRQ_FLAGS        (0x012E)
#define RADIOLIB_LR2021_CMD_GET_RX_FIFO_LEVEL                   (0x011C)
#define RADIOLIB_LR2021_CMD_GET_TX_FIFO_LEVEL                   (0x011D)
#define RADIOLIB_LR2021_CMD_CLEAR_RX_FIFO                       (0x011E)
#define RADIOLIB_LR2021_CMD_CLEAR_TX_FIFO                       (0x011F)
#define RADIOLIB_LR2021_CMD_CONFIG_LF_CLOCK                     (0x0118)
#define RADIOLIB_LR2021_CMD_CONFIG_CLK_OUTPUTS                  (0x0119)
#define RADIOLIB_LR2021_CMD_SET_TCXO_MODE                       (0x0120)
#define RADIOLIB_LR2021_CMD_SET_XOSC_CP_TRIM                    (0x0131)
#define RADIOLIB_LR2021_CMD_SET_RF_FREQUENCY                    (0x0200)
#define RADIOLIB_LR2021_CMD_SET_RX_PATH                         (0x0201)
#define RADIOLIB_LR2021_CMD_GET_RSSI_INST                       (0x020B)
#define RADIOLIB_LR2021_CMD_SET_RSSI_CALIBRATION                (0x0205)
#define RADIOLIB_LR2021_CMD_SET_TIMESTAMP_SOURCE                (0x0216)
#define RADIOLIB_LR2021_CMD_GET_TIMESTAMP_VALUE                 (0x0217)
#define RADIOLIB_LR2021_CMD_SET_CCA                             (0x0218)
#define RADIOLIB_LR2021_CMD_GET_CCA_RESULT                      (0x0219)
#define RADIOLIB_LR2021_CMD_SET_AGC_GAIN_MANUAL                 (0x021A)
#define RADIOLIB_LR2021_CMD_SET_CAD_PARAMS                      (0x021B)
#define RADIOLIB_LR2021_CMD_SET_CAD                             (0x021C)
#define RADIOLIB_LR2021_CMD_SEL_PA                              (0x020F)
#define RADIOLIB_LR2021_CMD_SET_PA_CONFIG                       (0x0202)
#define RADIOLIB_LR2021_CMD_SET_TX_PARAMS                       (0x0203)
#define RADIOLIB_LR2021_CMD_SET_PACKET_TYPE                     (0x0207)
#define RADIOLIB_LR2021_CMD_GET_PACKET_TYPE                     (0x0208)
#define RADIOLIB_LR2021_CMD_SET_LORA_MODULATION_PARAMS          (0x0220)
#define RADIOLIB_LR2021_CMD_SET_LORA_PACKET_PARAMS              (0x0221)
#define RADIOLIB_LR2021_CMD_SET_LORA_SYNCH_TIMEOUT              (0x0222)
#define RADIOLIB_LR2021_CMD_SET_LORA_SYNCWORD                   (0x0223)
#define RADIOLIB_LR2021_CMD_SET_LORA_SIDE_DET_CONFIG            (0x0224)
#define RADIOLIB_LR2021_CMD_SET_LORA_SIDE_DET_SYNCWORD          (0x0225)
#define RADIOLIB_LR2021_CMD_SET_LORA_CAD_PARAMS                 (0x0227)
#define RADIOLIB_LR2021_CMD_SET_LORA_CAD                        (0x0228)
#define RADIOLIB_LR2021_CMD_GET_LORA_RX_STATS                   (0x0229)
#define RADIOLIB_LR2021_CMD_GET_LORA_PACKET_STATUS              (0x022A)
#define RADIOLIB_LR2021_CMD_SET_LORA_ADDRESS                    (0x022B)
#define RADIOLIB_LR2021_CMD_SET_LORA_HOPPING                    (0x022C)
#define RADIOLIB_LR2021_CMD_SET_LORA_TX_SYNC                    (0x021D)
#define RADIOLIB_LR2021_CMD_SET_LORA_SIDE_DET_CAD               (0x021E)
#define RADIOLIB_LR2021_CMD_SET_RANGING_ADDR                    (0x0278)
#define RADIOLIB_LR2021_CMD_SET_RANGING_REQ_ADDR                (0x0279)
#define RADIOLIB_LR2021_CMD_GET_RANGING_RESULT                  (0x027A)
#define RADIOLIB_LR2021_CMD_GET_RANGING_STATS                   (0x027D)
#define RADIOLIB_LR2021_CMD_SET_RANGING_TX_RX_DELAY             (0x027B)
#define RADIOLIB_LR2021_CMD_SET_RANGING_PARAMS                  (0x027C)
#define RADIOLIB_LR2021_CMD_SET_GFSK_MODULATION_PARAMS          (0x0240)
#define RADIOLIB_LR2021_CMD_SET_GFSK_PACKET_PARAMS              (0x0241)
#define RADIOLIB_LR2021_CMD_SET_GFSK_WHITENING_PARAMS           (0x0242)
#define RADIOLIB_LR2021_CMD_SET_GFSK_CRC_PARAMS                 (0x0243)
#define RADIOLIB_LR2021_CMD_SET_GFSK_SYNCWORD                   (0x0244)
#define RADIOLIB_LR2021_CMD_SET_GFSK_ADDRESS                    (0x0245)
#define RADIOLIB_LR2021_CMD_GET_GFSK_RX_STATS                   (0x0246)
#define RADIOLIB_LR2021_CMD_GET_GFSK_PACKET_STATUS              (0x0247)
#define RADIOLIB_LR2021_CMD_SET_WMBUS_PARAMS                    (0x026A)
#define RADIOLIB_LR2021_CMD_GET_WMBUS_RX_STATS                  (0x026C)
#define RADIOLIB_LR2021_CMD_GET_WMBUS_PACKET_STATUS             (0x026D)
#define RADIOLIB_LR2021_CMD_SET_WMBUS_FILTERING_ADDRESS         (0x026E)
#define RADIOLIB_LR2021_CMD_SET_WISUN_MODE                      (0x0270)
#define RADIOLIB_LR2021_CMD_SET_WISUN_PACKET_PARAMS             (0x0271)
#define RADIOLIB_LR2021_CMD_GET_WISUN_RX_STATS                  (0x0272)
#define RADIOLIB_LR2021_CMD_GET_WISUN_PACKET_STATUS             (0x0273)
#define RADIOLIB_LR2021_CMD_SET_WISUN_PACKET_LEN                (0x0274)
#define RADIOLIB_LR2021_CMD_SET_ZWAVE_PARAMS                    (0x0297)
#define RADIOLIB_LR2021_CMD_SET_ZWAVE_HOME_ID_FILTERING         (0x0298)
#define RADIOLIB_LR2021_CMD_GET_ZWAVE_RX_STATS                  (0x0299)
#define RADIOLIB_LR2021_CMD_GET_ZWAVE_PACKET_STATUS             (0x029A)
#define RADIOLIB_LR2021_CMD_SET_ZWAVE_BEAM_FILTERING            (0x029B)
#define RADIOLIB_LR2021_CMD_SET_ZWAVE_SCAN_CONFIG               (0x029C)
#define RADIOLIB_LR2021_CMD_SET_ZWAVE_SCAN                      (0x029D)
#define RADIOLIB_LR2021_CMD_SET_BLE_MODULATION_PARAMS           (0x0260)
#define RADIOLIB_LR2021_CMD_SET_BLE_CHANNEL_PARAMS              (0x0261)
#define RADIOLIB_LR2021_CMD_SET_BLE_PDU_LEN                     (0x0266)
#define RADIOLIB_LR2021_CMD_SET_BLE_TX                          (0x0262)
#define RADIOLIB_LR2021_CMD_GET_BLE_RX_STATS                    (0x0264)
#define RADIOLIB_LR2021_CMD_GET_BLE_PACKET_STATUS               (0x0265)
#define RADIOLIB_LR2021_CMD_SET_OQPSK_PARAMS                    (0x029F)
#define RADIOLIB_LR2021_CMD_GET_OQPSK_RX_STATS                  (0x02A0)
#define RADIOLIB_LR2021_CMD_GET_OQPSK_PACKET_STATUS             (0x02A1)
#define RADIOLIB_LR2021_CMD_SET_OQPSK_PACKET_LEN                (0x02A2)
#define RADIOLIB_LR2021_CMD_SET_OQPSK_ADDRESS                   (0x02A3)
#define RADIOLIB_LR2021_CMD_SET_BPSK_MODULATION_PARAMS          (0x0250)
#define RADIOLIB_LR2021_CMD_SET_BPSK_PACKET_PARAMS              (0x0251)
#define RADIOLIB_LR2021_CMD_SET_FLRC_MODULATION_PARAMS          (0x0248)
#define RADIOLIB_LR2021_CMD_SET_FLRC_PACKET_PARAMS              (0x0249)
#define RADIOLIB_LR2021_CMD_GET_FLRC_RX_STATS                   (0x024A)
#define RADIOLIB_LR2021_CMD_GET_FLRC_PACKET_STATUS              (0x024B)
#define RADIOLIB_LR2021_CMD_SET_FLRC_SYNCWORD                   (0x024C)
#define RADIOLIB_LR2021_CMD_LR_FHSS_BUILD_FRAME                 (0x0256)
#define RADIOLIB_LR2021_CMD_LR_FHSS_SET_SYNCWORD                (0x0257)
#define RADIOLIB_LR2021_CMD_SET_OOK_MODULATION_PARAMS           (0x0281)
#define RADIOLIB_LR2021_CMD_SET_OOK_PACKET_PARAMS               (0x0282)
#define RADIOLIB_LR2021_CMD_SET_OOK_CRC_PARAMS                  (0x0283)
#define RADIOLIB_LR2021_CMD_SET_OOK_SYNCWORD                    (0x0284)
#define RADIOLIB_LR2021_CMD_SET_OOK_ADDRESS                     (0x0285)
#define RADIOLIB_LR2021_CMD_GET_OOK_RX_STATS                    (0x0286)
#define RADIOLIB_LR2021_CMD_GET_OOK_PACKET_STATUS               (0x0287)
#define RADIOLIB_LR2021_CMD_SET_OOK_DETECTOR                    (0x0288)
#define RADIOLIB_LR2021_CMD_SET_OOK_WHITENING_PARAMS            (0x0289)
#define RADIOLIB_LR2021_CMD_SET_TX_TEST_MODE                    (0x020E)

// RADIOLIB_LR2021_CMD_SET_DIO_IRQ_CONFIG
#define RADIOLIB_LR2021_IRQ_RX_FIFO                             (0x01UL << 0)   //  31    0     interrupt: Rx FIFO threshold reached
#define RADIOLIB_LR2021_IRQ_TX_FIFO                             (0x01UL << 1)   //  31    0                Tx FIFO threshold reached
#define RADIOLIB_LR2021_IRQ_RNG_REQ_VALID                       (0x01UL << 2)   //  31    0                ranging slave received valid request
#define RADIOLIB_LR2021_IRQ_TX_TIMESTAMP                        (0x01UL << 3)   //  31    0                end of packet Tx timestamp
#define RADIOLIB_LR2021_IRQ_RX_TIMESTAMP                        (0x01UL << 4)   //  31    0                end of packet Rx timestamp
#define RADIOLIB_LR2021_IRQ_PREAMBLE_DETECTED                   (0x01UL << 5)   //  31    0                preamble detected
#define RADIOLIB_LR2021_IRQ_LORA_HEADER_VALID                   (0x01UL << 6)   //  31    0                LoRa header received and valid
#define RADIOLIB_LR2021_IRQ_SYNCWORD_VALID                      (0x01UL << 6)   //  31    0                sync word valid
#define RADIOLIB_LR2021_IRQ_CAD_DETECTED                        (0x01UL << 7)   //  31    0                channel activity detected
#define RADIOLIB_LR2021_IRQ_LORA_HDR_TIMESTAMP                  (0x01UL << 8)   //  31    0                LoRa header timestamp
#define RADIOLIB_LR2021_IRQ_LORA_HDR_CRC_ERROR                  (0x01UL << 9)   //  31    0                LoRa header CRC error
#define RADIOLIB_LR2021_IRQ_EOL                                 (0x01UL << 10)  //  31    0                end of life
#define RADIOLIB_LR2021_IRQ_PA_OCP_OVP                          (0x01UL << 11)  //  31    0                PA overcurrent/overvoltage triggered
#define RADIOLIB_LR2021_IRQ_LORA_TX_RX_HOP                      (0x01UL << 12)  //  31    0                LoRa intra-packet hopping
#define RADIOLIB_LR2021_IRQ_SYNC_FAIL                           (0x01UL << 13)  //  31    0                sync word match detection failed
#define RADIOLIB_LR2021_IRQ_LORA_SYMBOL_END                     (0x01UL << 14)  //  31    0                symbol end
#define RADIOLIB_LR2021_IRQ_LORA_TIMESTAMP_STAT                 (0x01UL << 15)  //  31    0                new stats available
#define RADIOLIB_LR2021_IRQ_ERROR                               (0x01UL << 16)  //  31    0                error other than command error
#define RADIOLIB_LR2021_IRQ_CMD_ERROR                           (0x01UL << 17)  //  31    0                command error
#define RADIOLIB_LR2021_IRQ_RX_DONE                             (0x01UL << 18)  //  31    0                packet received
#define RADIOLIB_LR2021_IRQ_TX_DONE                             (0x01UL << 19)  //  31    0                packet transmitted
#define RADIOLIB_LR2021_IRQ_CAD_DONE                            (0x01UL << 20)  //  31    0                CAD finished
#define RADIOLIB_LR2021_IRQ_TIMEOUT                             (0x01UL << 21)  //  31    0                Rx or Tx timeout
#define RADIOLIB_LR2021_IRQ_CRC_ERROR                           (0x01UL << 22)  //  31    0                CRC error
#define RADIOLIB_LR2021_IRQ_LEN_ERROR                           (0x01UL << 23)  //  31    0                length error on received packet
#define RADIOLIB_LR2021_IRQ_ADDR_ERROR                          (0x01UL << 24)  //  31    0                packet with incorrect address received
#define RADIOLIB_LR2021_IRQ_FHSS                                (0x01UL << 25)  //  31    0                FHSS intra-packet hopping
#define RADIOLIB_LR2021_IRQ_INTER_PACKET_FREQ                   (0x01UL << 26)  //  31    0                inter packet hopping can load new frequency table
#define RADIOLIB_LR2021_IRQ_INTER_NEW_PAYLOAD                   (0x01UL << 27)  //  31    0                inter packet hopping can load new payload
#define RADIOLIB_LR2021_IRQ_RNG_RESP_DONE                       (0x01UL << 28)  //  31    0                slave ranging response sent
#define RADIOLIB_LR2021_IRQ_RNG_REQ_DIS                         (0x01UL << 29)  //  31    0                ranging request discarded
#define RADIOLIB_LR2021_IRQ_RNG_EXCH_VALID                      (0x01UL << 30)  //  31    0                master receive valid ranging response
#define RADIOLIB_LR2021_IRQ_RNG_TIMEOUT                         (0x01UL << 31)  //  31    0                ranging timeout
#define RADIOLIB_LR2021_IRQ_ALL                                 (0xFFFFFFFFUL)  //  31    0                all interrupts

// RADIOLIB_LR2021_CMD_SET_SLEEP
#define RADIOLIB_LR2021_SLEEP_32K_CLK_DISABLED                  (0x00UL << 0)   //  0     0     32 kHz clock: disabled
#define RADIOLIB_LR2021_SLEEP_32K_CLK_ENABLED                   (0x01UL << 0)   //  0     0                   enabled
#define RADIOLIB_LR2021_SLEEP_RETENTION_DISABLED                (0x00UL << 1)   //  1     1     configuration retention in sleep mode: disabled
#define RADIOLIB_LR2021_SLEEP_RETENTION_ENABLED                 (0x01UL << 1)   //  1     1                                            enabled

// RADIOLIB_LR2021_CMD_SET_STANDBY
#define RADIOLIB_LR2021_STANDBY_RC                              (0x00UL << 0)   //  7     0     standby mode: RC oscillator
#define RADIOLIB_LR2021_STANDBY_XOSC                            (0x01UL << 0)   //  7     0                   XOSC oscillator

// RADIOLIB_LR2021_CMD_SET_RX
#define RADIOLIB_LR2021_RX_TIMEOUT_NONE                         (0x000000UL)    //  23    0     Rx timeout duration: no timeout (Rx single mode)
#define RADIOLIB_LR2021_RX_TIMEOUT_INF                          (0xFFFFFFUL)    //  23    0                          infinite (Rx continuous mode)

// RADIOLIB_LR2021_CMD_SET_TX
#define RADIOLIB_LR2021_TX_TIMEOUT_NONE                         (0x000000UL)    //  23    0     disable Tx timeout

// RADIOLIB_LR2021_CMD_SET_RX_TX_FALLBACK_MODE
#define RADIOLIB_LR2021_FALLBACK_MODE_STBY_RC                   (0x01UL << 0)   //  1     0     fallback mode after Rx/Tx: standby with RC
#define RADIOLIB_LR2021_FALLBACK_MODE_STBY_XOSC                 (0x02UL << 0)   //  1     0                                standby with XOSC
#define RADIOLIB_LR2021_FALLBACK_MODE_FS                        (0x03UL << 0)   //  1     0                                frequency synthesis

// RADIOLIB_LR2021_CMD_SET_RX_DUTY_CYCLE
#define RADIOLIB_LR2021_RX_DUTY_CYCLE_MODE_RX                   (0x00UL << 0)   //  0     0     mode in Rx windows: Rx (default)
#define RADIOLIB_LR2021_RX_DUTY_CYCLE_MODE_CAD                  (0x01UL << 0)   //  0     0                         CAD

// RADIOLIB_LR20210_CMD_AUTO_TX_RX
#define RADIOLIB_LR2021_AUTO_MODE_NONE                          (0x00UL << 0)   //  1     0     auto rx-tx mode: never enable auto rx-tx
#define RADIOLIB_LR2021_AUTO_MODE_ALWAYS                        (0x01UL << 0)   //  1     0                      auto rx-tx on every RxDone or TxDone event
#define RADIOLIB_LR2021_AUTO_MODE_OK                            (0x02UL << 0)   //  1     0                      auto rx-tx on valid Rx packet only (Tx always)
#define RADIOLIB_LR2021_AUTO_MODE_CLEAR_DISABLED                (0x00UL << 7)   //  7     7     automatically disable auto rx-tx on timeout: disabled
#define RADIOLIB_LR2021_AUTO_MODE_CLEAR_ENABLED                 (0x01UL << 7)   //  7     7                                                  enabled

// RADIOLIB_LR2021_CMD_SET_REG_MODE
#define RADIOLIB_LR2021_REG_MODE_SIMO_OFF                       (0x00UL << 0)   //  7     0     SIMO mode: disabled
#define RADIOLIB_LR2021_REG_MODE_SIMO_NORMAL                    (0x02UL << 0)   //  7     0                normal
#define RADIOLIB_LR2021_REG_MODE_RAMP_RES_2_US                  (0x00UL << 5)   //  6     5     ramp timing resolution: 2 us
#define RADIOLIB_LR2021_REG_MODE_RAMP_RES_4_US                  (0x01UL << 5)   //  6     5                             4 us
#define RADIOLIB_LR2021_REG_MODE_RAMP_RES_8_US                  (0x02UL << 5)   //  6     5                             8 us
#define RADIOLIB_LR2021_REG_MODE_RAMP_RES_16_US                 (0x03UL << 5)   //  6     5                             16 us
#define RADIOLIB_LR2021_REG_MODE_RAMP_INDEX_RC2RU               (0)
#define RADIOLIB_LR2021_REG_MODE_RAMP_INDEX_TX2RU               (1)
#define RADIOLIB_LR2021_REG_MODE_RAMP_INDEX_RU2RC               (2)
#define RADIOLIB_LR2021_REG_MODE_RAMP_INDEX_RAMP_DOWN           (3)

// RADIOLIB_LR2021_CMD_CALIBRATE
#define RADIOLIB_LR2021_CALIBRATE_LF_RC                         (0x01UL << 0)   //  0     0     blocks to calibrate: low-frequency RC
#define RADIOLIB_LR2021_CALIBRATE_HF_RC                         (0x01UL << 1)   //  1     1                          high-frequency RC
#define RADIOLIB_LR2021_CALIBRATE_PLL                           (0x01UL << 2)   //  2     2                          phase-locked loop
#define RADIOLIB_LR2021_CALIBRATE_AAF                           (0x01UL << 3)   //  3     3                          anti-aliasing filter
#define RADIOLIB_LR2021_CALIBRATE_MU                            (0x01UL << 5)   //  4     4                          measurement unit
#define RADIOLIB_LR2021_CALIBRATE_PA_OFF                        (0x01UL << 6)   //  5     5                          power amplifier offset
#define RADIOLIB_LR2021_CALIBRATE_ALL                           (0x6FUL << 0)   //  7     0                          everything

// RADIOLIB_LR2021_CMD_CALIB_FRONT_END
#define RADIOLIB_LR2021_CALIBRATE_FE_LF_PATH                    (0x00UL << 15)  // 15    15     calibration path: low-frequency
#define RADIOLIB_LR2021_CALIBRATE_FE_HF_PATH                    (0x01UL << 15)  // 15    15                       high-frequency
#define RADIOLIB_LR2021_CAL_IMG_FREQ_TRIG_MHZ                   (20.0f)
#define RADIOLIB_LR2021_LF_CUTOFF_FREQ                          (1500.0f)

// RADIOLIB_LR2021_CMD_GET_V_BAT
#define RADIOLIB_LR2021_VBAT_FORMAT_RAW                         (0x00UL << 3)   //  3     3     readout format: raw
#define RADIOLIB_LR2021_VBAT_FORMAT_MV                          (0x01UL << 3)   //  3     3                     millivolts
#define RADIOLIB_LR2021_MEAS_RESOLUTION_OFFSET                  (8)

// RADIOLIB_LR2021_CMD_GET_TEMP
#define RADIOLIB_LR2021_TEMP_SOURCE_VBE                         (0x00UL << 4)   //  4     4     temperature source: sensor near Vbe junction
#define RADIOLIB_LR2021_TEMP_SOURCE_XOSC                        (0x01UL << 4)   //  4     4                         sensor near XOSC
#define RADIOLIB_LR2021_TEMP_FORMAT_RAW                         (0x00UL << 3)   //  3     3     readout format: raw
#define RADIOLIB_LR2021_TEMP_FORMAT_DEG_C                       (0x01UL << 3)   //  3     3                     degrees Celsius

// RADIOLIB_LR2021_CMD_SET_EOL_CONFIG
#define RADIOLIB_LR2021_EOL_TRIM_1V6                            (0x00UL << 1)   //  3     1     EoL trigger threshold: 1.60 V
#define RADIOLIB_LR2021_EOL_TRIM_1V67                           (0x01UL << 1)   //  3     1                            1.67 V
#define RADIOLIB_LR2021_EOL_TRIM_1V74                           (0x02UL << 1)   //  3     1                            1.74 V
#define RADIOLIB_LR2021_EOL_TRIM_1V8                            (0x03UL << 1)   //  3     1                            1.80 V
#define RADIOLIB_LR2021_EOL_TRIM_1V88                           (0x04UL << 1)   //  3     1                            1.88 V (default)
#define RADIOLIB_LR2021_EOL_TRIM_1V95                           (0x05UL << 1)   //  3     1                            1.95 V
#define RADIOLIB_LR2021_EOL_TRIM_2V0                            (0x06UL << 1)   //  3     1                            2.00 V
#define RADIOLIB_LR2021_EOL_TRIM_2V1                            (0x07UL << 1)   //  3     1                            2.10 V

// RADIOLIB_LR2021_CMD_GET_ERRORS
#define RADIOLIB_LR2021_HF_XOSC_START_ERR                       (0x01UL << 0)   // 15     0     error: high-frequency XOSC failed to start
#define RADIOLIB_LR2021_LF_XOSC_START_ERR                       (0x01UL << 1)   // 15     0            low-frequency XOSC failed to start
#define RADIOLIB_LR2021_PLL_LOCK_ERR                            (0x01UL << 2)   // 15     0            PLL failed to lock
#define RADIOLIB_LR2021_LF_RC_CALIB_ERR                         (0x01UL << 3)   // 15     0            low-frequency RC calibration failed
#define RADIOLIB_LR2021_HF_RC_CALIB_ERR                         (0x01UL << 4)   // 15     0            high-frequency RC calibration failed
#define RADIOLIB_LR2021_PLL_CALIB_ERR                           (0x01UL << 5)   // 15     0            PLL calibration failed
#define RADIOLIB_LR2021_AAF_CALIB_ERR                           (0x01UL << 6)   // 15     0            anti-aliasing filter calibration failed
#define RADIOLIB_LR2021_IMG_CALIB_ERR                           (0x01UL << 7)   // 15     0            image rejection calibration failed
#define RADIOLIB_LR2021_CHIP_BUSY_ERR                           (0x01UL << 8)   // 15     0            Tx or Rx could not be processed because chips was busy
#define RADIOLIB_LR2021_RXFREQ_NO_FE_CAL_ERR                    (0x01UL << 9)   // 15     0            front-end calibration nto available for this Rx frequency
#define RADIOLIB_LR2021_MEAS_UNIT_ADC_CALIB_ERR                 (0x01UL << 10)  // 15     0            measurement unit ADC calibration failed
#define RADIOLIB_LR2021_PA_OFFSET_CALIB_ERR                     (0x01UL << 11)  // 15     0            PA offset calibration failed
#define RADIOLIB_LR2021_PPF_CALIB_ERR                           (0x01UL << 12)  // 15     0            poly-phase filter calibration failed
#define RADIOLIB_LR2021_SRC_CALIB_ERR                           (0x01UL << 13)  // 15     0            self-reception cancellation calibration failed
#define RADIOLIB_LR2021_SRC_SATURATION_CALIB_ERR                (0x01UL << 14)  // 15     0            RSSI saturation during SRC calibration
#define RADIOLIB_LR2021_SRC_TOLERANCE_CALIB_ERR                 (0x01UL << 15)  // 15     0            self-reception cancellation values out of tolernce

// RADIOLIB_LR2021_CMD_SET_DIO_FUNCTION
#define RADIOLIB_LR2021_DIO_FUNCTION_NONE                       (0x00UL << 4)   //  7     4     DIO function: none
#define RADIOLIB_LR2021_DIO_FUNCTION_IRQ                        (0x01UL << 4)   //  7     4                   interrupt
#define RADIOLIB_LR2021_DIO_FUNCTION_RF_SWITCH                  (0x02UL << 4)   //  7     4                   RF switch
#define RADIOLIB_LR2021_DIO_FUNCTION_GPIO_OUTPUT_LOW            (0x05UL << 4)   //  7     4                   low output
#define RADIOLIB_LR2021_DIO_FUNCTION_GPIO_OUTPUT_HIGH           (0x06UL << 4)   //  7     4                   high output
#define RADIOLIB_LR2021_DIO_FUNCTION_HF_CLK_OUT                 (0x07UL << 4)   //  7     4                   high-frequency clock output
#define RADIOLIB_LR2021_DIO_FUNCTION_LF_CLK_OUT                 (0x08UL << 4)   //  7     4                   low-frequency clock output (DIO7-11 only)
#define RADIOLIB_LR2021_DIO_FUNCTION_TX_TRIGGER                 (0x09UL << 4)   //  7     4                   Tx trigger
#define RADIOLIB_LR2021_DIO_FUNCTION_RX_TRIGGER                 (0x0AUL << 4)   //  7     4                   Rx trigger
#define RADIOLIB_LR2021_DIO_SLEEP_PULL_NONE                     (0x00UL << 0)   //  3     0     pull up/down in sleep mode: none
#define RADIOLIB_LR2021_DIO_SLEEP_PULL_DOWN                     (0x01UL << 0)   //  3     0                                 pull-down
#define RADIOLIB_LR2021_DIO_SLEEP_PULL_UP                       (0x02UL << 0)   //  3     0                                 pull-up
#define RADIOLIB_LR2021_DIO_SLEEP_PULL_AUTO                     (0x03UL << 0)   //  3     0                                 auto

// RADIOLIB_LR2021_CMD_SET_DIO_RF_SWITCH_CONFIG
#define RADIOLIB_LR2021_DIO5                                    (RADIOLIB_LRXXXX_DIOx(0))
#define RADIOLIB_LR2021_DIO6                                    (RADIOLIB_LRXXXX_DIOx(1))
#define RADIOLIB_LR2021_DIO7                                    (RADIOLIB_LRXXXX_DIOx(2))
#define RADIOLIB_LR2021_DIO8                                    (RADIOLIB_LRXXXX_DIOx(3))
#define RADIOLIB_LR2021_DIO9                                    (RADIOLIB_LRXXXX_DIOx(4))
#define RADIOLIB_LR2021_DIO10                                   (RADIOLIB_LRXXXX_DIOx(5))
#define RADIOLIB_LR2021_DIO11                                   (RADIOLIB_LRXXXX_DIOx(6))

// RADIOLIB_LR2021_CMD_CONFIG_FIFO_IRQ
#define RADIOLIB_LR2021_FIFO_IRQ_EMPTY                          (0x01UL << 0)   //  7     0     FIFO interrupt on: empty FIFO
#define RADIOLIB_LR2021_FIFO_IRQ_LOW                            (0x01UL << 1)   //  7     0                        level below threshold
#define RADIOLIB_LR2021_FIFO_IRQ_HIGH                           (0x01UL << 2)   //  7     0                        level above threshold
#define RADIOLIB_LR2021_FIFO_IRQ_FULL                           (0x01UL << 3)   //  7     0                        full FIFO
#define RADIOLIB_LR2021_FIFO_IRQ_OVERFLOW                       (0x01UL << 4)   //  7     0                        overflow
#define RADIOLIB_LR2021_FIFO_IRQ_UNDERFLOW                      (0x01UL << 5)   //  7     0                        underflow

// RADIOLIB_LR2021_CMD_CONFIG_LF_CLOCK
#define RADIOLIB_LR2021_LF_CLOCK_INTERNAL_RC                    (0x00UL << 0)   //  7     0     low-frequency source: internal 32 kHz RC oscillator
#define RADIOLIB_LR2021_LF_CLOCK_EXTERNAL                       (0x02UL << 0)   //  7     0                           external 32.768 kHz signal on DIO11

// RADIOLIB_LR2021_CMD_SET_RX_PATH
#define RADIOLIB_LR2021_RX_PATH_LF                              (0x00UL << 0)   //  7     0     Rx path: low-frequency
#define RADIOLIB_LR2021_RX_PATH_HF                              (0x01UL << 0)   //  7     0              high-frequency
#define RADIOLIB_LR2021_RX_BOOST_LF                             (0x00UL << 0)   //  7     0     Rx boost: low-frequency
#define RADIOLIB_LR2021_RX_BOOST_HF                             (0x04UL << 0)   //  7     0               high-frequency

// RADIOLIB_LR2021_CMD_SET_RSSI_CALIBRATION
#define RADIOLIB_LR2021_RSSI_PATH_LF                            (0x01UL << 0)   //  0     0     Rx path for RSSI: low-frequency
#define RADIOLIB_LR2021_RSSI_PATH_HF                            (0x01UL << 1)   //  1     1                       high-frequency
#define RADIOLIB_LR2021_GAIN_TABLE_LENGTH                       (27)

// RADIOLIB_LR2021_CMD_SET_TIMESTAMP_SOURCE
#define RADIOLIB_LR2021_TIMESTAMP_SOURCE_NONE                   (0x00UL << 0)   //  3     0     timestamp source: none
#define RADIOLIB_LR2021_TIMESTAMP_SOURCE_TX_DONE                (0x01UL << 0)   //  3     0                       Tx done
#define RADIOLIB_LR2021_TIMESTAMP_SOURCE_RX_DONE                (0x02UL << 0)   //  3     0                       Rx done
#define RADIOLIB_LR2021_TIMESTAMP_SOURCE_SYNC                   (0x03UL << 0)   //  3     0                       sync
#define RADIOLIB_LR2021_TIMESTAMP_SOURCE_HEADER                 (0x04UL << 0)   //  3     0                       LoRa header

// RADIOLIB_LR2021_CMD_SET_CAD_PARAMS
#define RADIOLIB_LR2021_CAD_EXIT_MODE_FALLBACK                  (0x00UL << 0)   //  1     0     CAD exit mode: the configured fallback mode
#define RADIOLIB_LR2021_CAD_EXIT_MODE_TX                        (0x01UL << 0)   //  1     0                    Tx
#define RADIOLIB_LR2021_CAD_EXIT_MODE_RX                        (0x02UL << 0)   //  1     0                    Rx
#define RADIOLIB_LR2021_CAD_PARAM_DEFAULT                       (0xFFUL << 0)   //  7     0     used by the CAD methods to specify default parameter value

// RADIOLIB_LR2021_CMD_SEL_PA
#define RADIOLIB_LR2021_PA_LOW_POWER                            (0x00UL << 0)   //  1     0     PA to use: low-power
#define RADIOLIB_LR2021_PA_HIGH_POWER                           (0x01UL << 0)   //  1     0                high-power

// RADIOLIB_LR2021_CMD_SET_PA_CONFIG
#define RADIOLIB_LR2021_PA_LF_MODE_FSM                          (0x00UL << 0)   //  1     0     PA LF mode: full single-ended mode
#define RADIOLIB_LR2021_PA_LF_DUTY_CYCLE_UNUSED                 (0x06UL << 4)   //  7     4     PA LF duty cycle: PA not used
#define RADIOLIB_LR2021_PA_LF_SLICES_UNUSED                     (0x07UL << 0)   //  3     0     PA LF slices: PA not used
#define RADIOLIB_LR2021_PA_HF_DUTY_CYCLE_UNUSED                 (0x10UL << 0)   //  4     0     PA HF duty cycle: PA not used

// RADIOLIB_LR2021_CMD_SET_PACKET_TYPE
#define RADIOLIB_LR2021_PACKET_TYPE_LORA                        (0x00UL << 0)   //  7     0     packet type: LoRa
#define RADIOLIB_LR2021_PACKET_TYPE_GFSK                        (0x02UL << 0)   //  7     0                  FSK
#define RADIOLIB_LR2021_PACKET_TYPE_BLE                         (0x03UL << 0)   //  7     0                  BLE
#define RADIOLIB_LR2021_PACKET_TYPE_RTTOF                       (0x04UL << 0)   //  7     0                  RTToF
#define RADIOLIB_LR2021_PACKET_TYPE_FLRC                        (0x05UL << 0)   //  7     0                  FLRC
#define RADIOLIB_LR2021_PACKET_TYPE_BPSK                        (0x06UL << 0)   //  7     0                  BPSK
#define RADIOLIB_LR2021_PACKET_TYPE_LR_FHSS                     (0x07UL << 0)   //  7     0                  LR-FHSS
#define RADIOLIB_LR2021_PACKET_TYPE_WM_BUS                      (0x08UL << 0)   //  7     0                  WM-BUS
#define RADIOLIB_LR2021_PACKET_TYPE_WI_SUN                      (0x09UL << 0)   //  7     0                  WI-SUN
#define RADIOLIB_LR2021_PACKET_TYPE_OOK                         (0x0AUL << 0)   //  7     0                  OOK
#define RADIOLIB_LR2021_PACKET_TYPE_RAW                         (0x0BUL << 0)   //  7     0                  RAW
#define RADIOLIB_LR2021_PACKET_TYPE_Z_WAVE                      (0x0CUL << 0)   //  7     0                  Z-WAVE
#define RADIOLIB_LR2021_PACKET_TYPE_OQPSK                       (0x0DUL << 0)   //  7     0                  OQPSK
#define RADIOLIB_LR2021_PACKET_TYPE_NONE                        (0xFFUL << 0)   //  2     0                  none

// RADIOLIB_LR2021_CMD_SET_LORA_MODULATION_PARAMS
#define RADIOLIB_LR2021_LORA_BW_31                              (0x02UL << 0)   //  3     0     LoRa bandwidth: 31.25 kHz
#define RADIOLIB_LR2021_LORA_BW_41                              (0x0AUL << 0)   //  3     0                     41.67 kHz
#define RADIOLIB_LR2021_LORA_BW_83                              (0x0BUL << 0)   //  3     0                     83.34 kHz
#define RADIOLIB_LR2021_LORA_BW_62                              (0x03UL << 0)   //  3     0                     62.50 kHz
#define RADIOLIB_LR2021_LORA_BW_125                             (0x04UL << 0)   //  3     0                     125 kHz
#define RADIOLIB_LR2021_LORA_BW_250                             (0x05UL << 0)   //  3     0                     250 kHz
#define RADIOLIB_LR2021_LORA_BW_500                             (0x06UL << 0)   //  3     0                     500 kHz
#define RADIOLIB_LR2021_LORA_BW_1000                            (0x07UL << 0)   //  3     0                     1000 kHz
#define RADIOLIB_LR2021_LORA_BW_812                             (0x0FUL << 0)   //  3     0                     812 kHz
#define RADIOLIB_LR2021_LORA_BW_406                             (0x0EUL << 0)   //  3     0                     406 kHz
#define RADIOLIB_LR2021_LORA_BW_203                             (0x0DUL << 0)   //  3     0                     203 kHz
#define RADIOLIB_LR2021_LORA_BW_101                             (0x0CUL << 0)   //  3     0                     101 kHz
#define RADIOLIB_LR2021_LORA_CR_4_5                             (0x01UL << 0)   //  3     0     LoRa coding rate: 4/5
#define RADIOLIB_LR2021_LORA_CR_4_6                             (0x02UL << 0)   //  3     0                       4/6
#define RADIOLIB_LR2021_LORA_CR_4_7                             (0x03UL << 0)   //  3     0                       4/7
#define RADIOLIB_LR2021_LORA_CR_4_8                             (0x04UL << 0)   //  3     0                       4/8
#define RADIOLIB_LR2021_LORA_CR_4_5_LI                          (0x05UL << 0)   //  3     0                       4/5 long interleaver
#define RADIOLIB_LR2021_LORA_CR_4_6_LI                          (0x06UL << 0)   //  3     0                       4/6 long interleaver
#define RADIOLIB_LR2021_LORA_CR_4_7_LI                          (0x07UL << 0)   //  3     0                       4/7 long interleaver
#define RADIOLIB_LR2021_LORA_LDRO_DISABLED                      (0x00UL << 0)   //  1     0     LDRO/PPM configuration: disabled
#define RADIOLIB_LR2021_LORA_LDRO_ENABLED                       (0x01UL << 0)   //  1     0                             enabled

// RADIOLIB_LR2021_CMD_SET_LORA_PACKET_PARAMS
#define RADIOLIB_LR2021_LORA_HEADER_EXPLICIT                    (0x00UL << 2)   //  2     2     LoRa header mode: explicit
#define RADIOLIB_LR2021_LORA_HEADER_IMPLICIT                    (0x01UL << 2)   //  2     2                       implicit
#define RADIOLIB_LR2021_LORA_CRC_DISABLED                       (0x00UL << 1)   //  1     1     LoRa CRC: disabled
#define RADIOLIB_LR2021_LORA_CRC_ENABLED                        (0x01UL << 1)   //  1     1               enabled
#define RADIOLIB_LR2021_LORA_IQ_STANDARD                        (0x00UL << 0)   //  0     0     LoRa IQ: standard
#define RADIOLIB_LR2021_LORA_IQ_INVERTED                        (0x01UL << 0)   //  0     0              inverted

// RADIOLIB_LR2021_CMD_SET_LORA_SYNCH_TIMEOUT
#define RADIOLIB_LR2021_LORA_SYNCH_TIMEOUT_FORMAT_SYMBOLS       (0x00UL << 0)   //  7     0     LoRa synch timeout format: number of symbols
#define RADIOLIB_LR2021_LORA_SYNCH_TIMEOUT_FORMAT_MANT_EXP      (0x01UL << 0)   //  7     0                                mantissa-exponent

// RADIOLIB_LR2021_CMD_SET_LORA_SYNCWORD
#define RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE                  (0x12UL << 0)   //  7     0     LoRa sync word: 0x12 (private networks)
#define RADIOLIB_LR2021_LORA_SYNC_WORD_LORAWAN                  (0x34UL << 0)   //  7     0                     0x34 (LoRaWAN reserved)

// RADIOLIB_LR2021_CMD_SET_LORA_HOPPING
#define RADIOLIB_LR2021_LORA_HOPPING_DISABLED                   (0x00UL << 6)   //  7     6     LoRa intra-packet hopping: disabled
#define RADIOLIB_LR2021_LORA_HOPPING_ENABLED                    (0x01UL << 6)   //  7     6                                enabled

// RADIOLIB_LR2021_CMD_SET_LORA_TX_SYNC
#define RADIOLIB_LR2021_LORA_TX_SYNC_DISABLED                   (0x00UL << 6)   //  7     6     Tx sync: disabled
#define RADIOLIB_LR2021_LORA_TX_SYNC_MASTER                     (0x01UL << 6)   //  7     6              master (wait for signal to transmit sync frame)
#define RADIOLIB_LR2021_LORA_TX_SYNC_SLAVE                      (0x02UL << 6)   //  7     6              slave (output signal on sync frame)

// RADIOLIB_LR2021_CMD_GET_RANGING_RESULT
#define RADIOLIB_LR2021_RANGING_RESULT_TYPE_RAW                 (0x00UL << 0)   //  7     0     ranging result type: raw
#define RADIOLIB_LR2021_RANGING_RESULT_TYPE_RAW_EXT             (0x01UL << 0)   //  7     0                          extended raw
#define RADIOLIB_LR2021_RANGING_RESULT_TYPE_GAINS               (0x02UL << 0)   //  7     0                          AGC gain steps

// RADIOLIB_LR2021_CMD_SET_GFSK_MODULATION_PARAMS
#define RADIOLIB_LR2021_GFSK_BPSK_OOK_BITRATE_BPS               (0x00UL << 31)  //  7     0     bitrate units: bits per second
#define RADIOLIB_LR2021_GFSK_BPSK_OOK_BITRATE_FRACTIONAL        (0x01UL << 31)  //  7     0                    fractional (1/256 bps)
#define RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_NONE             (0x00UL << 0)   //  7     0     shaping filter: none
#define RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_GAUSS_BT_2_0     (0x02UL << 0)   //  7     0                     Gaussian, BT = 2.0
#define RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_RRC_ROLLOFF_0_4  (0x03UL << 0)   //  7     0                     Root-Raised-Cosine with 0.4 roll-off
#define RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_GAUSS_BT_0_3     (0x04UL << 0)   //  7     0                     Gaussian, BT = 0.3
#define RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_GAUSS_BT_0_5     (0x05UL << 0)   //  7     0                     Gaussian, BT = 0.5
#define RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_GAUSS_BT_0_7     (0x06UL << 0)   //  7     0                     Gaussian, BT = 0.7
#define RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_GAUSS_BT_1_0     (0x07UL << 0)   //  7     0                     Gaussian, BT = 1.0
#define RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_RRC_ROLLOFF_0_3  (0x08UL << 0)   //  7     0                     Root-Raised-Cosine with 0.3 roll-off
#define RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_RRC_ROLLOFF_0_5  (0x09UL << 0)   //  7     0                     Root-Raised-Cosine with 0.5 roll-off
#define RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_RRC_ROLLOFF_0_7  (0x0AUL << 0)   //  7     0                     Root-Raised-Cosine with 0.7 roll-off
// TODO implement the other bandwidths as well (and figure out a way how to calculate it)
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_4_8                      (39)            //  7     0     GFSK Rx bandwidth: 4.8 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_5_8                      (215)           //  7     0                        5.8 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_7_4                      (87)            //  7     0                        7.4 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_9_7                      (38)            //  7     0                        9.6 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_12_0                     (30)            //  7     0                        12.0 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_14_9                     (86)            //  7     0                        14.9 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_19_2                     (37)            //  7     0                        19.2 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_23_1                     (213)           //  7     0                        21.3 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_29_8                     (85)            //  7     0                        29.8 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_38_5                     (36)            //  7     0                        38.5 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_46_3                     (212)           //  7     0                        46.3 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_59_5                     (84)            //  7     0                        59.5 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_76_9                     (35)            //  7     0                        76.9 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_92_6                     (211)           //  7     0                        92.6 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_119_0                    (83)            //  7     0                        119.0 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_153_8                    (34)            //  7     0                        153.8 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_185_2                    (210)           //  7     0                        185.2 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_238_1                    (82)            //  7     0                        238.1 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_307_7                    (33)            //  7     0                        307.7 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_370_4                    (209)           //  7     0                        370.4 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_476_2                    (81)            //  7     0                        476.2 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_555_6                    (216)           //  7     0                        555.6 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_666_7                    (152)           //  7     0                        666.7 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_769_2                    (24)            //  7     0                        769.2 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_1111                     (200)           //  7     0                        1111 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_2222                     (192)           //  7     0                        2222 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_2666                     (128)           //  7     0                        2667 kHz
#define RADIOLIB_LR2021_GFSK_OOK_RX_BW_3076                     (0)             //  7     0                        3077 kHz

// RADIOLIB_LR2021_CMD_SET_GFSK_PACKET_PARAMS
#define RADIOLIB_LR2021_GFSK_OOK_ADDR_FILT_DISABLED             (0x00UL << 0)   //  7     0     address filtering: disabled
#define RADIOLIB_LR2021_GFSK_OOK_ADDR_FILT_NODE                 (0x01UL << 0)   //  7     0                        node only
#define RADIOLIB_LR2021_GFSK_OOK_ADDR_FILT_NODE_BROADCAST       (0x02UL << 0)   //  7     0                        node and broadcast
#define RADIOLIB_LR2021_GFSK_OOK_PACKET_FORMAT_FIXED            (0x00UL << 0)   //  7     0     packet format: fixed length
#define RADIOLIB_LR2021_GFSK_OOK_PACKET_FORMAT_VARIABLE_8BIT    (0x01UL << 0)   //  7     0                    variable, 8-bit length
#define RADIOLIB_LR2021_GFSK_OOK_PACKET_FORMAT_VARIABLE_9BIT    (0x02UL << 0)   //  7     0                    variable, 9-bit length (for SX128x compatibility)
#define RADIOLIB_LR2021_GFSK_OOK_PACKET_FORMAT_VARIABLE_15BIT   (0x03UL << 0)   //  7     0                    variable, 15-bit length
#define RADIOLIB_LR2021_GFSK_OOK_CRC_OFF                        (0x00UL << 0)   //  7     0     CRC: disabled
#define RADIOLIB_LR2021_GFSK_OOK_CRC8                           (0x01UL << 0)   //  7     0          1-byte
#define RADIOLIB_LR2021_GFSK_OOK_CRC16                          (0x02UL << 0)   //  7     0          2-byte
#define RADIOLIB_LR2021_GFSK_OOK_CRC24                          (0x03UL << 0)   //  7     0          3-byte
#define RADIOLIB_LR2021_GFSK_OOK_CRC32                          (0x04UL << 0)   //  7     0          4-byte
#define RADIOLIB_LR2021_GFSK_OOK_CRC8_INV                       (0x09UL << 0)   //  7     0          1-byte, inverted
#define RADIOLIB_LR2021_GFSK_OOK_CRC16_INV                      (0x0AUL << 0)   //  7     0          2-byte, inverted
#define RADIOLIB_LR2021_GFSK_OOK_CRC24_INV                      (0x0BUL << 0)   //  7     0          3-byte, inverted
#define RADIOLIB_LR2021_GFSK_OOK_CRC32_INV                      (0x0CUL << 0)   //  7     0          4-byte, inverted

// RADIOLIB_LR2021_CMD_SET_GFSK_WHITENING_PARAMS
#define RADIOLIB_LR2021_GFSK_WHITENING_TYPE_SX126X_LR11XX       (0x00UL << 0)   //  7     0     whitening type: compatible with SX126x and LR2021
#define RADIOLIB_LR2021_GFSK_WHITENING_TYPE_SX128X              (0x01UL << 0)   //  7     0                     compatible with SX128x

// RADIOLIB_LR2021_CMD_SET_GFSK_SYNCWORD
#define RADIOLIB_LR2021_GFSK_SYNC_WORD_LEN                      (8)

// RADIOLIB_LR2021_CMD_SET_OQPSK_PARAMS
#define RADIOLIB_LR2021_OQPSK_TYPE_15_4                         (0x00UL << 0)   //  7     0     OQPSK type: 802.15.4 PHY, 250 kbps bit rate

// RADIOLIB_LR2021_CMD_SET_BPSK_PACKET_PARAMS
#define RADIOLIB_LR2021_BPSK_MODE_RAW                           (0x00UL << 0)   //  7     0     encoding mode: raw
#define RADIOLIB_LR2021_BPSK_MODE_SIGFOX                        (0x01UL << 0)   //  7     0                    SigFox PHY

// RADIOLIB_LR2021_CMD_SET_FLRC_MODULATION_PARAMS
#define RADIOLIB_LR2021_FLRC_BR_2600                            (0x00UL << 0)   //  7     0     bitrate/bandwidth: 2600 kbps, 2666 kHz
#define RADIOLIB_LR2021_FLRC_BR_2080                            (0x01UL << 0)   //  7     0                        2080 kbps, 2222 kHz
#define RADIOLIB_LR2021_FLRC_BR_1300                            (0x02UL << 0)   //  7     0                        1300 kbps, 1333 kHz
#define RADIOLIB_LR2021_FLRC_BR_1040                            (0x03UL << 0)   //  7     0                        1040 kbps, 1333 kHz
#define RADIOLIB_LR2021_FLRC_BR_650                             (0x04UL << 0)   //  7     0                        650 kbps, 888 kHz
#define RADIOLIB_LR2021_FLRC_BR_520                             (0x05UL << 0)   //  7     0                        520 kbps, 769 kHz
#define RADIOLIB_LR2021_FLRC_BR_325                             (0x06UL << 0)   //  7     0                        325 kbps, 444 kHz
#define RADIOLIB_LR2021_FLRC_BR_260                             (0x07UL << 0)   //  7     0                        260 kbps, 444 kHz
#define RADIOLIB_LR2021_FLRC_CR_1_2                             (0x00UL << 0)   //  7     0     coding rate: 1/2
#define RADIOLIB_LR2021_FLRC_CR_3_4                             (0x01UL << 0)   //  7     0                  3/4
#define RADIOLIB_LR2021_FLRC_CR_1_0                             (0x02UL << 0)   //  7     0                  1 (uncoded)
#define RADIOLIB_LR2021_FLRC_CR_2_3                             (0x03UL << 0)   //  7     0                  2/3

// RADIOLIB_LR2021_CMD_SET_OOK_MODULATION_PARAMS
#define RADIOLIB_LR2021_OOK_DEPTH_FULL                          (0x00UL << 0)   //  7     0     magnitude depth: limited by the PA
#define RADIOLIB_LR2021_OOK_DEPTH_20_DB                         (0x01UL << 0)   //  7     0                      20 dB maximum

// RADIOLIB_LR2021_CMD_SET_OOK_PACKET_PARAMS
#define RADIOLIB_LR2021_OOK_MANCHESTER_OFF                      (0x00UL << 0)   //  3     0     Manchester encoding: disabled
#define RADIOLIB_LR2021_OOK_MANCHESTER_ON                       (0x01UL << 0)   //  3     0                          enabled
#define RADIOLIB_LR2021_OOK_MANCHESTER_ON_INV                   (0x09UL << 0)   //  3     0                          enabled, inverted

// RADIOLIB_LR2021_CMD_SET_TX_TEST_MODE
#define RADIOLIB_LR2021_TX_TEST_MODE_NORMAL_TX                  (0x00UL << 0)   //  7     0     Tx test mode: normal
#define RADIOLIB_LR2021_TX_TEST_MODE_INF_PREAMBLE               (0x01UL << 0)   //  7     0                   infinite preamble
#define RADIOLIB_LR2021_TX_TEST_MODE_CW                         (0x02UL << 0)   //  7     0                   continuous wave
#define RADIOLIB_LR2021_TX_TEST_MODE_PRBS9                      (0x03UL << 0)   //  7     0                   pseudo-random bits

#endif

#endif
