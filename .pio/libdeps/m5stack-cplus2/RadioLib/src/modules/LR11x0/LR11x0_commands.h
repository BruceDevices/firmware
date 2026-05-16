#if !defined(RADIOLIB_LR11X0_COMMANDS_H)
#define RADIOLIB_LR11X0_COMMANDS_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR11X0

// LR11X0 SPI commands
#define RADIOLIB_LR11X0_CMD_NOP                                 (0x0000)
#define RADIOLIB_LR11X0_CMD_WRITE_REG_MEM                       (0x0105)
#define RADIOLIB_LR11X0_CMD_READ_REG_MEM                        (0x0106)
#define RADIOLIB_LR11X0_CMD_WRITE_BUFFER                        (0x0109)
#define RADIOLIB_LR11X0_CMD_READ_BUFFER                         (0x010A)
#define RADIOLIB_LR11X0_CMD_CLEAR_RX_BUFFER                     (0x010B)
#define RADIOLIB_LR11X0_CMD_WRITE_REG_MEM_MASK                  (0x010C)
#define RADIOLIB_LR11X0_CMD_GET_STATUS                          (0x0100)
#define RADIOLIB_LR11X0_CMD_GET_VERSION                         (0x0101)
#define RADIOLIB_LR11X0_CMD_GET_ERRORS                          (0x010D)
#define RADIOLIB_LR11X0_CMD_CLEAR_ERRORS                        (0x010E)
#define RADIOLIB_LR11X0_CMD_CALIBRATE                           (0x010F)
#define RADIOLIB_LR11X0_CMD_SET_REG_MODE                        (0x0110)
#define RADIOLIB_LR11X0_CMD_CALIB_IMAGE                         (0x0111)
#define RADIOLIB_LR11X0_CMD_SET_DIO_AS_RF_SWITCH                (0x0112)
#define RADIOLIB_LR11X0_CMD_SET_DIO_IRQ_PARAMS                  (0x0113)
#define RADIOLIB_LR11X0_CMD_CLEAR_IRQ                           (0x0114)
#define RADIOLIB_LR11X0_CMD_CONFIG_LF_CLOCK                     (0x0116)
#define RADIOLIB_LR11X0_CMD_SET_TCXO_MODE                       (0x0117)
#define RADIOLIB_LR11X0_CMD_REBOOT                              (0x0118)
#define RADIOLIB_LR11X0_CMD_GET_VBAT                            (0x0119)
#define RADIOLIB_LR11X0_CMD_GET_TEMP                            (0x011A)
#define RADIOLIB_LR11X0_CMD_SET_SLEEP                           (0x011B)
#define RADIOLIB_LR11X0_CMD_SET_STANDBY                         (0x011C)
#define RADIOLIB_LR11X0_CMD_SET_FS                              (0x011D)
#define RADIOLIB_LR11X0_CMD_GET_RANDOM_NUMBER                   (0x0120)
#define RADIOLIB_LR11X0_CMD_ERASE_INFO_PAGE                     (0x0121)
#define RADIOLIB_LR11X0_CMD_WRITE_INFO_PAGE                     (0x0122)
#define RADIOLIB_LR11X0_CMD_READ_INFO_PAGE                      (0x0123)
#define RADIOLIB_LR11X0_CMD_GET_CHIP_EUI                        (0x0125)
#define RADIOLIB_LR11X0_CMD_GET_SEMTECH_JOIN_EUI                (0x0126)
#define RADIOLIB_LR11X0_CMD_DERIVE_ROOT_KEYS_AND_GET_PIN        (0x0127)
#define RADIOLIB_LR11X0_CMD_ENABLE_SPI_CRC                      (0x0128)
#define RADIOLIB_LR11X0_CMD_DRIVE_DIOS_IN_SLEEP_MODE            (0x012A)
#define RADIOLIB_LR11X0_CMD_RESET_STATS                         (0x0200)
#define RADIOLIB_LR11X0_CMD_GET_STATS                           (0x0201)
#define RADIOLIB_LR11X0_CMD_GET_PACKET_TYPE                     (0x0202)
#define RADIOLIB_LR11X0_CMD_GET_RX_BUFFER_STATUS                (0x0203)
#define RADIOLIB_LR11X0_CMD_GET_PACKET_STATUS                   (0x0204)
#define RADIOLIB_LR11X0_CMD_GET_RSSI_INST                       (0x0205)
#define RADIOLIB_LR11X0_CMD_SET_GFSK_SYNC_WORD                  (0x0206)
#define RADIOLIB_LR11X0_CMD_SET_LORA_PUBLIC_NETWORK             (0x0208)
#define RADIOLIB_LR11X0_CMD_SET_RX                              (0x0209)
#define RADIOLIB_LR11X0_CMD_SET_TX                              (0x020A)
#define RADIOLIB_LR11X0_CMD_SET_RF_FREQUENCY                    (0x020B)
#define RADIOLIB_LR11X0_CMD_AUTO_TX_RX                          (0x020C)
#define RADIOLIB_LR11X0_CMD_SET_CAD_PARAMS                      (0x020D)
#define RADIOLIB_LR11X0_CMD_SET_PACKET_TYPE                     (0x020E)
#define RADIOLIB_LR11X0_CMD_SET_MODULATION_PARAMS               (0x020F)
#define RADIOLIB_LR11X0_CMD_SET_PACKET_PARAMS                   (0x0210)
#define RADIOLIB_LR11X0_CMD_SET_TX_PARAMS                       (0x0211)
#define RADIOLIB_LR11X0_CMD_SET_PACKET_ADRS                     (0x0212)
#define RADIOLIB_LR11X0_CMD_SET_RX_TX_FALLBACK_MODE             (0x0213)
#define RADIOLIB_LR11X0_CMD_SET_RX_DUTY_CYCLE                   (0x0214)
#define RADIOLIB_LR11X0_CMD_SET_PA_CONFIG                       (0x0215)
#define RADIOLIB_LR11X0_CMD_STOP_TIMEOUT_ON_PREAMBLE            (0x0217)
#define RADIOLIB_LR11X0_CMD_SET_CAD                             (0x0218)
#define RADIOLIB_LR11X0_CMD_SET_TX_CW                           (0x0219)
#define RADIOLIB_LR11X0_CMD_SET_TX_INFINITE_PREAMBLE            (0x021A)
#define RADIOLIB_LR11X0_CMD_SET_LORA_SYNCH_TIMEOUT              (0x021B)
#define RADIOLIB_LR11X0_CMD_SET_RANGING_ADDR                    (0x021C)
#define RADIOLIB_LR11X0_CMD_SET_RANGING_REQ_ADDR                (0x021D)
#define RADIOLIB_LR11X0_CMD_GET_RANGING_RESULT                  (0x021E)
#define RADIOLIB_LR11X0_CMD_SET_RANGING_TX_RX_DELAY             (0x021F)
#define RADIOLIB_LR11X0_CMD_SET_GFSK_CRC_PARAMS                 (0x0224)
#define RADIOLIB_LR11X0_CMD_SET_GFSK_WHIT_PARAMS                (0x0225)
#define RADIOLIB_LR11X0_CMD_SET_RX_BOOSTED                      (0x0227)
#define RADIOLIB_LR11X0_CMD_SET_RANGING_PARAMETER               (0x0228)
#define RADIOLIB_LR11X0_CMD_SET_RSSI_CALIBRATION                (0x0229)
#define RADIOLIB_LR11X0_CMD_SET_LORA_SYNC_WORD                  (0x022B)
#define RADIOLIB_LR11X0_CMD_LR_FHSS_BUILD_FRAME                 (0x022C)
#define RADIOLIB_LR11X0_CMD_LR_FHSS_SET_SYNC_WORD               (0x022D)
#define RADIOLIB_LR11X0_CMD_CONFIG_BLE_BEACON                   (0x022E)
#define RADIOLIB_LR11X0_CMD_GET_LORA_RX_HEADER_INFOS            (0x0230)
#define RADIOLIB_LR11X0_CMD_BLE_BEACON_SEND                     (0x0231)
#define RADIOLIB_LR11X0_CMD_WIFI_SCAN                           (0x0300)
#define RADIOLIB_LR11X0_CMD_WIFI_SCAN_TIME_LIMIT                (0x0301)
#define RADIOLIB_LR11X0_CMD_WIFI_COUNTRY_CODE                   (0x0302)
#define RADIOLIB_LR11X0_CMD_WIFI_COUNTRY_CODE_TIME_LIMIT        (0x0303)
#define RADIOLIB_LR11X0_CMD_WIFI_GET_NB_RESULTS                 (0x0305)
#define RADIOLIB_LR11X0_CMD_WIFI_READ_RESULTS                   (0x0306)
#define RADIOLIB_LR11X0_CMD_WIFI_RESET_CUMUL_TIMINGS            (0x0307)
#define RADIOLIB_LR11X0_CMD_WIFI_READ_CUMUL_TIMINGS             (0x0308)
#define RADIOLIB_LR11X0_CMD_WIFI_GET_NB_COUNTRY_CODE_RESULTS    (0x0309)
#define RADIOLIB_LR11X0_CMD_WIFI_READ_COUNTRY_CODE_RESULTS      (0x030A)
#define RADIOLIB_LR11X0_CMD_WIFI_CFG_TIMESTAMP_AP_PHONE         (0x030B)
#define RADIOLIB_LR11X0_CMD_WIFI_READ_VERSION                   (0x0320)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_RSSI                      (0x0222)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_CONSTELLATION_TO_USE       (0x0400)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_CONSTELLATION_TO_USE      (0x0401)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_ALMANAC_UPDATE             (0x0402)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_UPDATE            (0x0403)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_FREQ_SEARCH_SPACE          (0x0404)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_FREQ_SEARCH_SPACE         (0x0405)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_VERSION                   (0x0406)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_SUPPORTED_CONSTELLATIONS  (0x0407)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_MODE                       (0x0408)
#define RADIOLIB_LR11X0_CMD_GNSS_AUTONOMOUS                     (0x0409)
#define RADIOLIB_LR11X0_CMD_GNSS_ASSISTED                       (0x040A)
#define RADIOLIB_LR11X0_CMD_GNSS_SCAN                           (0x040B)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_RESULT_SIZE                (0x040C)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_RESULTS                   (0x040D)
#define RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_FULL_UPDATE            (0x040E)
#define RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_READ_ADDR_SIZE         (0x040F)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_ASSISTANCE_POSITION        (0x0410)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_ASSISTANCE_POSITION       (0x0411)
#define RADIOLIB_LR11X0_CMD_GNSS_PUSH_SOLVER_MSG                (0x0414)
#define RADIOLIB_LR11X0_CMD_GNSS_PUSH_DM_MSG                    (0x0415)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_CONTEXT_STATUS             (0x0416)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_NB_SV_DETECTED             (0x0417)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_SV_DETECTED                (0x0418)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_CONSUMPTION                (0x0419)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_PER_SATELLITE     (0x041A)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_SV_VISIBLE                 (0x041F)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_SV_VISIBLE_DOPPLER         (0x0420)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_LAST_SCAN_MODE_LAUNCHED   (0x0426)
#define RADIOLIB_LR11X0_CMD_GNSS_FETCH_TIME                     (0x0432)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_TIME                      (0x0434)
#define RADIOLIB_LR11X0_CMD_GNSS_RESET_TIME                     (0x0435)
#define RADIOLIB_LR11X0_CMD_GNSS_RESET_POSITION                 (0x0437)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_WEEK_NUMBER_ROLLOWER      (0x0438)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_DEMOD_STATUS              (0x0439)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_CUMUL_TIMING              (0x044A)
#define RADIOLIB_LR11X0_CMD_GNSS_SET_TIME                       (0x044B)
#define RADIOLIB_LR11X0_CMD_GNSS_CONFIG_DELAY_RESET_AP          (0x044D)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_DOPPLER_SOLVER_RES        (0x044F)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_DELAY_RESET_AP            (0x0453)
#define RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_UPDATE_FROM_SAT        (0x0454)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_KEEP_SYNC_STATUS          (0x0456)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_STATUS            (0x0457)
#define RADIOLIB_LR11X0_CMD_GNSS_CONFIG_ALMANAC_UPDATE_PERIOD   (0x0463)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_UPDATE_PERIOD     (0x0464)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_SV_WARM_START              (0x0466)
#define RADIOLIB_LR11X0_CMD_GNSS_GET_SV_SYNC                    (0x0466)
#define RADIOLIB_LR11X0_CMD_GNSS_READ_WARM_START_STATUS         (0x0469)
#define RADIOLIB_LR11X0_CMD_GNSS_WRITE_BIT_MASK_SAT_ACTIVATED   (0x0472)
#define RADIOLIB_LR11X0_CMD_CRYPTO_SET_KEY                      (0x0502)
#define RADIOLIB_LR11X0_CMD_CRYPTO_DERIVE_KEY                   (0x0503)
#define RADIOLIB_LR11X0_CMD_CRYPTO_PROCESS_JOIN_ACCEPT          (0x0504)
#define RADIOLIB_LR11X0_CMD_CRYPTO_COMPUTE_AES_CMAC             (0x0505)
#define RADIOLIB_LR11X0_CMD_CRYPTO_VERIFY_AES_CMAC              (0x0506)
#define RADIOLIB_LR11X0_CMD_CRYPTO_AES_ENCRYPT_01               (0x0507)
#define RADIOLIB_LR11X0_CMD_CRYPTO_AES_ENCRYPT                  (0x0508)
#define RADIOLIB_LR11X0_CMD_CRYPTO_AES_DECRYPT                  (0x0509)
#define RADIOLIB_LR11X0_CMD_CRYPTO_STORE_TO_FLASH               (0x050A)
#define RADIOLIB_LR11X0_CMD_CRYPTO_RESTORE_FROM_FLASH           (0x050B)
#define RADIOLIB_LR11X0_CMD_CRYPTO_SET_PARAM                    (0x050D)
#define RADIOLIB_LR11X0_CMD_CRYPTO_GET_PARAM                    (0x050E)
#define RADIOLIB_LR11X0_CMD_CRYPTO_CHECK_ENCRYPTED_FIRMWARE_IMAGE         (0x050F)
#define RADIOLIB_LR11X0_CMD_CRYPTO_CHECK_ENCRYPTED_FIRMWARE_IMAGE_RESULT  (0x0510)
#define RADIOLIB_LR11X0_CMD_BOOT_ERASE_FLASH                    (0x8000)
#define RADIOLIB_LR11X0_CMD_BOOT_WRITE_FLASH_ENCRYPTED          (0x8003)
#define RADIOLIB_LR11X0_CMD_BOOT_GET_HASH                       (0x8004)
#define RADIOLIB_LR11X0_CMD_BOOT_REBOOT                         (0x8005)
#define RADIOLIB_LR11X0_CMD_BOOT_GET_PIN                        (0x800B)
#define RADIOLIB_LR11X0_CMD_BOOT_GET_CHIP_EUI                   (0x800C)
#define RADIOLIB_LR11X0_CMD_BOOT_GET_JOIN_EUI                   (0x800D)

// LR11X0 SPI command variables

// RADIOLIB_LR11X0_CMD_GET_STATUS                                                   MSB   LSB   DESCRIPTION
#define RADIOLIB_LR11X0_STAT_1_CMD_FAIL                         (0x00UL << 1)   //  3     1     command status: last command could not be executed
#define RADIOLIB_LR11X0_STAT_1_CMD_PERR                         (0x01UL << 1)   //  3     1                     processing error
#define RADIOLIB_LR11X0_STAT_1_CMD_OK                           (0x02UL << 1)   //  3     1                     successfully processed
#define RADIOLIB_LR11X0_STAT_1_CMD_DAT                          (0x03UL << 1)   //  3     1                     successfully processed, data is being transmitted
#define RADIOLIB_LR11X0_STAT_1_IRQ_INACTIVE                     (0x00UL << 0)   //  0     0     interrupt status: inactive
#define RADIOLIB_LR11X0_STAT_1_IRQ_ACTIVE                       (0x01UL << 0)   //  0     0                       at least 1 interrupt active
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_CLEARED                  (0x00UL << 4)   //  7     4     reset status: cleared
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_ANALOG                   (0x01UL << 4)   //  7     4                   analog (power on, brown-out)
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_EXTERNAL                 (0x02UL << 4)   //  7     4                   NRESET pin
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_SYSTEM                   (0x03UL << 4)   //  7     4                   system
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_WATCHDOG                 (0x04UL << 4)   //  7     4                   watchdog
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_WAKEUP                   (0x05UL << 4)   //  7     4                   NSS toggling wake-up
#define RADIOLIB_LR11X0_STAT_2_CMD_RST_RTC                      (0x06UL << 4)   //  7     4                   realtime clock
#define RADIOLIB_LR11X0_STAT_2_MODE_SLEEP                       (0x00UL << 1)   //  3     1     chip mode: sleep
#define RADIOLIB_LR11X0_STAT_2_MODE_STBY_RC                     (0x01UL << 1)   //  3     1                standby with RC oscillator
#define RADIOLIB_LR11X0_STAT_2_MODE_STBY_OSC                    (0x02UL << 1)   //  3     1                standby with external oscillator
#define RADIOLIB_LR11X0_STAT_2_MODE_FS                          (0x03UL << 1)   //  3     1                frequency synthesis
#define RADIOLIB_LR11X0_STAT_2_MODE_RX                          (0x04UL << 1)   //  3     1                receive
#define RADIOLIB_LR11X0_STAT_2_MODE_TX                          (0x05UL << 1)   //  3     1                transmit
#define RADIOLIB_LR11X0_STAT_2_MODE_WIFI_GNSS                   (0x06UL << 1)   //  3     1                WiFi or GNSS geolocation
#define RADIOLIB_LR11X0_STAT_2_BOOT                             (0x00UL << 0)   //  0     0     code executed from: bootloader
#define RADIOLIB_LR11X0_STAT_2_FLASH                            (0x01UL << 0)   //  0     0                         flash

// RADIOLIB_LR11X0_CMD_WRITE_REG_MEM
#define RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN                  (256)           //  7     0     maximum length of read/write SPI payload in bytes

// RADIOLIB_LR11X0_CMD_GET_VERSION
#define RADIOLIB_LR11X0_DEVICE_LR1110                           (0x01UL << 0)   //  7     0     HW device: LR1110
#define RADIOLIB_LR11X0_DEVICE_LR1120                           (0x02UL << 0)   //  7     0                LR1120
#define RADIOLIB_LR11X0_DEVICE_LR1121                           (0x03UL << 0)   //  7     0                LR1121
#define RADIOLIB_LR11X0_DEVICE_BOOT                             (0xDFUL << 0)   //  7     0                bootloader mode

// RADIOLIB_LR11X0_CMD_GET_ERRORS
#define RADIOLIB_LR11X0_ERROR_STAT_LF_RC_CALIB_ERR              (0x01UL << 0)   //  15    0     error: low frequency RC not calibrated
#define RADIOLIB_LR11X0_ERROR_STAT_HF_RC_CALIB_ERR              (0x01UL << 1)   //  15    0            high frequency RC not calibrated
#define RADIOLIB_LR11X0_ERROR_STAT_ADC_CALIB_ERR                (0x01UL << 2)   //  15    0            ADC not calibrated
#define RADIOLIB_LR11X0_ERROR_STAT_PLL_CALIB_ERR                (0x01UL << 3)   //  15    0            PLL not calibrated
#define RADIOLIB_LR11X0_ERROR_STAT_IMG_CALIB_ERR                (0x01UL << 4)   //  15    0            image rejection not calibrated
#define RADIOLIB_LR11X0_ERROR_STAT_HF_XOSC_START_ERR            (0x01UL << 5)   //  15    0            high frequency oscillator failed to start
#define RADIOLIB_LR11X0_ERROR_STAT_LF_XOSC_START_ERR            (0x01UL << 6)   //  15    0            low frequency oscillator failed to start
#define RADIOLIB_LR11X0_ERROR_STAT_PLL_LOCK_ERR                 (0x01UL << 7)   //  15    0            PLL failed to lock
#define RADIOLIB_LR11X0_ERROR_STAT_RX_ADC_OFFSET_ERR            (0x01UL << 8)   //  15    0            ADC offset not calibrated

// RADIOLIB_LR11X0_CMD_CALIBRATE
#define RADIOLIB_LR11X0_CALIBRATE_PLL_TX                        (0x01UL << 5)   //  5     5     calibrate: Tx PLL
#define RADIOLIB_LR11X0_CALIBRATE_IMG                           (0x01UL << 4)   //  4     4                image rejection
#define RADIOLIB_LR11X0_CALIBRATE_ADC                           (0x01UL << 3)   //  3     3                A/D converter
#define RADIOLIB_LR11X0_CALIBRATE_PLL                           (0x01UL << 2)   //  2     2                PLL
#define RADIOLIB_LR11X0_CALIBRATE_HF_RC                         (0x01UL << 1)   //  1     1                high frequency RC
#define RADIOLIB_LR11X0_CALIBRATE_LF_RC                         (0x01UL << 0)   //  0     0                low frequency RC
#define RADIOLIB_LR11X0_CALIBRATE_ALL                           (0x3FUL << 0)   //  5     0                everything
#define RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG_MHZ                   (20.0f)

// RADIOLIB_LR11X0_CMD_SET_REG_MODE
#define RADIOLIB_LR11X0_REG_MODE_LDO                            (0x00UL << 0)   //  0     0     regulator mode: LDO in all modes
#define RADIOLIB_LR11X0_REG_MODE_DC_DC                          (0x01UL << 0)   //  0     0                     DC-DC and LDO

// RADIOLIB_LR11X0_CMD_SET_DIO_AS_RF_SWITCH
#define RADIOLIB_LR11X0_RFSW_DIO5_ENABLED                       (0x01UL << 0)   //  4     0     RF switch: DIO5 enabled
#define RADIOLIB_LR11X0_RFSW_DIO5_DISABLED                      (0x00UL << 0)   //  4     0                DIO5 disabled (default)
#define RADIOLIB_LR11X0_RFSW_DIO6_ENABLED                       (0x01UL << 1)   //  4     0     RF switch: DIO6 enabled
#define RADIOLIB_LR11X0_RFSW_DIO6_DISABLED                      (0x00UL << 1)   //  4     0                DIO6 disabled (default)
#define RADIOLIB_LR11X0_RFSW_DIO7_ENABLED                       (0x01UL << 2)   //  4     0     RF switch: DIO7 enabled
#define RADIOLIB_LR11X0_RFSW_DIO7_DISABLED                      (0x00UL << 2)   //  4     0                DIO7 disabled (default)
#define RADIOLIB_LR11X0_RFSW_DIO8_ENABLED                       (0x01UL << 3)   //  4     0     RF switch: DIO8 enabled
#define RADIOLIB_LR11X0_RFSW_DIO8_DISABLED                      (0x00UL << 3)   //  4     0                DIO8 disabled (default)
#define RADIOLIB_LR11X0_RFSW_DIO10_ENABLED                      (0x01UL << 4)   //  4     0     RF switch: DIO10 enabled
#define RADIOLIB_LR11X0_RFSW_DIO10_DISABLED                     (0x00UL << 4)   //  4     0                DIO10 disabled (default)
#define RADIOLIB_LR11X0_DIO5                                    (RADIOLIB_LRXXXX_DIOx(0))
#define RADIOLIB_LR11X0_DIO6                                    (RADIOLIB_LRXXXX_DIOx(1))
#define RADIOLIB_LR11X0_DIO7                                    (RADIOLIB_LRXXXX_DIOx(2))
#define RADIOLIB_LR11X0_DIO8                                    (RADIOLIB_LRXXXX_DIOx(3))
#define RADIOLIB_LR11X0_DIO10                                   (RADIOLIB_LRXXXX_DIOx(4))

// RADIOLIB_LR11X0_CMD_SET_DIO_IRQ_PARAMS
#define RADIOLIB_LR11X0_IRQ_TX_DONE                             (0x01UL << 2)   //  31    0     interrupt: packet transmitted
#define RADIOLIB_LR11X0_IRQ_RX_DONE                             (0x01UL << 3)   //  31    0                packet received
#define RADIOLIB_LR11X0_IRQ_PREAMBLE_DETECTED                   (0x01UL << 4)   //  31    0                preamble detected
#define RADIOLIB_LR11X0_IRQ_SYNC_WORD_HEADER_VALID              (0x01UL << 5)   //  31    0                sync word or LoRa header valid
#define RADIOLIB_LR11X0_IRQ_HEADER_ERR                          (0x01UL << 6)   //  31    0                LoRa header CRC error
#define RADIOLIB_LR11X0_IRQ_CRC_ERR                             (0x01UL << 7)   //  31    0                packet CRC error 
#define RADIOLIB_LR11X0_IRQ_CAD_DONE                            (0x01UL << 8)   //  31    0                CAD completed
#define RADIOLIB_LR11X0_IRQ_CAD_DETECTED                        (0x01UL << 9)   //  31    0                CAD detected
#define RADIOLIB_LR11X0_IRQ_TIMEOUT                             (0x01UL << 10)  //  31    0                Rx or Tx timeout
#define RADIOLIB_LR11X0_IRQ_LR_FHSS_HOP                         (0x01UL << 11)  //  31    0                FHSS hop
#define RADIOLIB_LR11X0_IRQ_GNSS_DONE                           (0x01UL << 19)  //  31    0                GNSS scan finished
#define RADIOLIB_LR11X0_IRQ_WIFI_DONE                           (0x01UL << 20)  //  31    0                WiFi scan finished
#define RADIOLIB_LR11X0_IRQ_LBD                                 (0x01UL << 21)  //  31    0                low battery detected
#define RADIOLIB_LR11X0_IRQ_CMD_ERROR                           (0x01UL << 22)  //  31    0                command error
#define RADIOLIB_LR11X0_IRQ_ERROR                               (0x01UL << 23)  //  31    0                some other error than CMD_ERR
#define RADIOLIB_LR11X0_IRQ_FSK_LEN_ERROR                       (0x01UL << 24)  //  31    0                FSK packet received with length error
#define RADIOLIB_LR11X0_IRQ_FSK_ADDR_ERROR                      (0x01UL << 25)  //  31    0                FSK packet received with address error
#define RADIOLIB_LR11X0_IRQ_LORA_RX_TIMESTAMP                   (0x01UL << 27)  //  31    0                last LoRa symbol was received (timestamp source)
#define RADIOLIB_LR11X0_IRQ_GNSS_ABORT                          (0x01UL << 28)  //  31    0                GNSS scan aborted
#define RADIOLIB_LR11X0_IRQ_ALL                                 (0x1BF80FFCUL)  //  31    0                all interrupts
#define RADIOLIB_LR11X0_IRQ_NONE                                (0x00UL << 0)   //  31    0                no interrupts

// RADIOLIB_LR11X0_CMD_CONFIG_LF_CLOCK
#define RADIOLIB_LR11X0_LF_CLK_RC                               (0x00UL << 0)   //  1     0     32.768 kHz source: RC oscillator
#define RADIOLIB_LR11X0_LF_CLK_XOSC                             (0x01UL << 0)   //  1     0                        crystal oscillator
#define RADIOLIB_LR11X0_LF_CLK_EXT                              (0x02UL << 0)   //  1     0                        external signal on DIO11
#define RADIOLIB_LR11X0_LF_BUSY_RELEASE_DISABLED                (0x00UL << 2)   //  2     2
#define RADIOLIB_LR11X0_LF_BUSY_RELEASE_ENABLED                 (0x01UL << 2)   //  2     2

// RADIOLIB_LR11X0_CMD_SET_SLEEP
#define RADIOLIB_LR11X0_SLEEP_RETENTION_DISABLED                (0x00UL << 0)   //  0     0     configuration retention in sleep mode: disabled
#define RADIOLIB_LR11X0_SLEEP_RETENTION_ENABLED                 (0x01UL << 0)   //  0     0                                            enabled
#define RADIOLIB_LR11X0_SLEEP_WAKEUP_DISABLED                   (0x00UL << 0)   //  1     1     automated wakeup: disabled
#define RADIOLIB_LR11X0_SLEEP_WAKEUP_ENABLED                    (0x01UL << 0)   //  1     1                       enabled

// RADIOLIB_LR11X0_CMD_SET_STANDBY
#define RADIOLIB_LR11X0_STANDBY_RC                              (0x00UL << 0)   //  7     0     standby mode: RC oscillator
#define RADIOLIB_LR11X0_STANDBY_XOSC                            (0x00UL << 0)   //  7     0                   XTAL/TCXO oscillator

// RADIOLIB_LR11X0_CMD_ERASE_INFO_PAGE
#define RADIOLIB_LR11X0_INFO_PAGE                               (1)

// RADIOLIB_LR11X0_CMD_GET_CHIP_EUI
#define RADIOLIB_LR11X0_EUI_LEN                                 (8)

// RADIOLIB_LR11X0_CMD_GET_HASH
#define RADIOLIB_LR11X0_HASH_LEN                                (10)

// RADIOLIB_LR11X0_CMD_DERIVE_ROOT_KEYS_AND_GET_PIN
#define RADIOLIB_LR11X0_PIN_LEN                                 (4)

// RADIOLIB_LR11X0_CMD_GET_PACKET_STATUS
#define RADIOLIB_LR11X0_RX_STATUS_ADDR_ERR                      (0x01UL << 5)   //  7     0     Rx status: address filtering error
#define RADIOLIB_LR11X0_RX_STATUS_CRC_ERR                       (0x01UL << 4)   //  7     0                CRC error
#define RADIOLIB_LR11X0_RX_STATUS_LEN_ERR                       (0x01UL << 3)   //  7     0                length filtering error
#define RADIOLIB_LR11X0_RX_STATUS_ABORTED                       (0x01UL << 2)   //  7     0                packet reception aborted
#define RADIOLIB_LR11X0_RX_STATUS_PACKET_RECEIVED               (0x01UL << 1)   //  7     0                packet received
#define RADIOLIB_LR11X0_RX_STATUS_PACKET_SENT                   (0x01UL << 0)   //  7     0                packet sent

// RADIOLIB_LR11X0_CMD_SET_GFSK_SYNC_WORD
#define RADIOLIB_LR11X0_GFSK_SYNC_WORD_LEN                      (8)

// RADIOLIB_LR11X0_CMD_SET_LORA_PUBLIC_NETWORK
#define RADIOLIB_LR11X0_LORA_PRIVATE_NETWORK                    (0x00UL << 0)   //  7     0     LoRa sync word: private network
#define RADIOLIB_LR11X0_LORA_PUBLIC_NETWORK                     (0x01UL << 0)   //  7     0                     public network

// RADIOLIB_LR11X0_CMD_SET_RX
#define RADIOLIB_LR11X0_RX_TIMEOUT_NONE                         (0x000000UL)    //  23    0     Rx timeout duration: no timeout (Rx single mode)
#define RADIOLIB_LR11X0_RX_TIMEOUT_INF                          (0xFFFFFFUL)    //  23    0                          infinite (Rx continuous mode)

// RADIOLIB_LR11X0_CMD_SET_TX
#define RADIOLIB_LR11X0_TX_TIMEOUT_NONE                         (0x000000UL)    //  23    0     disable Tx timeout

// RADIOLIB_LR11X0_CMD_AUTO_TX_RX
#define RADIOLIB_LR11X0_AUTO_TX_RX_DISABLED                     (0xFFFFFFUL)    //  23    0     disable auto Tx/Rx mode
#define RADIOLIB_LR11X0_AUTO_TX_RX_SKIP_INT                     (0x000000UL)    //  23    0     skip intermediary mode
#define RADIOLIB_LR11X0_AUTO_INTERMEDIARY_MODE_SLEEP            (0x00UL << 0)   //  1     0     intermediary mode: sleep
#define RADIOLIB_LR11X0_AUTO_INTERMEDIARY_MODE_STBY_RC          (0x01UL << 0)   //  1     0                        standby with RC
#define RADIOLIB_LR11X0_AUTO_INTERMEDIARY_MODE_STBY_XOSC        (0x02UL << 0)   //  1     0                        standby with XOSC
#define RADIOLIB_LR11X0_AUTO_INTERMEDIARY_MODE_FS               (0x03UL << 0)   //  1     0                        frequency synthesis
#define RADIOLIB_LR11X0_AUTO_TX_RX_TIMEOUT_DISABLED             (0x000000UL)    //  23    0     disable timeout of the second mode

// RADIOLIB_LR11X0_CMD_SET_CAD_PARAMS
#define RADIOLIB_LR11X0_CAD_EXIT_MODE_STBY_RC                   (0x00UL << 0)   //  7     0     mode to set after CAD: standby with RC
#define RADIOLIB_LR11X0_CAD_EXIT_MODE_RX                        (0x01UL << 0)   //  7     0                            receive if activity detected
#define RADIOLIB_LR11X0_CAD_EXIT_MODE_LBT                       (0x10UL << 0)   //  7     0                            transmit if no activity detected
#define RADIOLIB_LR11X0_CAD_PARAM_DEFAULT                       (0xFFUL << 0)   //  7     0     used by the CAD methods to specify default parameter value

// RADIOLIB_LR11X0_CMD_SET_PACKET_TYPE
#define RADIOLIB_LR11X0_PACKET_TYPE_NONE                        (0x00UL << 0)   //  2     0     packet type: none
#define RADIOLIB_LR11X0_PACKET_TYPE_GFSK                        (0x01UL << 0)   //  2     0                  (G)FSK
#define RADIOLIB_LR11X0_PACKET_TYPE_LORA                        (0x02UL << 0)   //  2     0                  LoRa
#define RADIOLIB_LR11X0_PACKET_TYPE_SIGFOX                      (0x03UL << 0)   //  2     0                  Sigfox
#define RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS                     (0x04UL << 0)   //  2     0                  GMSK/LR-FHSS
#define RADIOLIB_LR11X0_PACKET_TYPE_RANGING                     (0x05UL << 0)   //  2     0                  ranging
#define RADIOLIB_LR11X0_PACKET_TYPE_BLE                         (0x06UL << 0)   //  2     0                  BLE beacon

// RADIOLIB_LR11X0_CMD_SET_MODULATION_PARAMS
#define RADIOLIB_LR11X0_LORA_BW_62_5                            (0x03UL << 0)   //  7     0     LoRa bandwidth: 62.5 kHz
#define RADIOLIB_LR11X0_LORA_BW_125_0                           (0x04UL << 0)   //  7     0                     125.0 kHz
#define RADIOLIB_LR11X0_LORA_BW_250_0                           (0x05UL << 0)   //  7     0                     250.0 kHz
#define RADIOLIB_LR11X0_LORA_BW_500_0                           (0x06UL << 0)   //  7     0                     500.0 kHz
#define RADIOLIB_LR11X0_LORA_BW_203_125                         (0x0DUL << 0)   //  7     0                     203.0 kHz (2.4GHz only)
#define RADIOLIB_LR11X0_LORA_BW_406_25                          (0x0EUL << 0)   //  7     0                     406.0 kHz (2.4GHz only)
#define RADIOLIB_LR11X0_LORA_BW_812_50                          (0x0FUL << 0)   //  7     0                     812.0 kHz (2.4GHz only)
#define RADIOLIB_LR11X0_LORA_CR_4_5_SHORT                       (0x01UL << 0)   //  7     0     coding rate: 4/5 with short interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_6_SHORT                       (0x02UL << 0)   //  7     0                  4/6 with short interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_7_SHORT                       (0x03UL << 0)   //  7     0                  4/7 with short interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_8_SHORT                       (0x04UL << 0)   //  7     0                  4/8 with short interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_5_LONG                        (0x05UL << 0)   //  7     0                  4/5 with long interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_6_LONG                        (0x06UL << 0)   //  7     0                  4/6 with long interleaver
#define RADIOLIB_LR11X0_LORA_CR_4_8_LONG                        (0x07UL << 0)   //  7     0                  4/8 with long interleaver
#define RADIOLIB_LR11X0_LORA_LDRO_DISABLED                      (0x00UL << 0)   //  7     0     low data rate optimize: disabled
#define RADIOLIB_LR11X0_LORA_LDRO_ENABLED                       (0x01UL << 0)   //  7     0                             enabled
#define RADIOLIB_LR11X0_GFSK_BIT_RATE_DIV_DISABLED              (0x00UL << 31)  //  31    0     divide bit rate value by 256: disabled
#define RADIOLIB_LR11X0_GFSK_BIT_RATE_DIV_ENABLED               (0x01UL << 31)  //  31    0                                   enabled
#define RADIOLIB_LR11X0_GFSK_SHAPING_NONE                       (0x00UL << 0)   //  7     0     shaping filter: none
#define RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_3            (0x08UL << 0)   //  7     0                     Gaussian, BT = 0.3
#define RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_5            (0x09UL << 0)   //  7     0                     Gaussian, BT = 0.5
#define RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_7            (0x0AUL << 0)   //  7     0                     Gaussian, BT = 0.7
#define RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_1_0            (0x0BUL << 0)   //  7     0                     Gaussian, BT = 1.0
#define RADIOLIB_LR11X0_GFSK_SHAPING_RAISED_COSINE_BT_0_7       (0x16UL << 0)   //  7     0                     raised cosine, BT = 0.7
#define RADIOLIB_LR11X0_GFSK_RX_BW_4_8                          (0x1FUL << 0)   //  7     0     GFSK Rx bandwidth: 4.8 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_5_8                          (0x17UL << 0)   //  7     0                        5.8 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_7_3                          (0x0FUL << 0)   //  7     0                        7.3 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_9_7                          (0x1EUL << 0)   //  7     0                        9.7 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_11_7                         (0x16UL << 0)   //  7     0                        11.7 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_14_6                         (0x0EUL << 0)   //  7     0                        14.6 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_19_5                         (0x1DUL << 0)   //  7     0                        19.5 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_23_4                         (0x15UL << 0)   //  7     0                        23.4 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_29_3                         (0x0DUL << 0)   //  7     0                        29.3 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_39_0                         (0x1CUL << 0)   //  7     0                        39.0 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_46_9                         (0x14UL << 0)   //  7     0                        46.9 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_58_6                         (0x0CUL << 0)   //  7     0                        58.6 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_78_2                         (0x1BUL << 0)   //  7     0                        78.2 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_93_8                         (0x13UL << 0)   //  7     0                        93.8 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_117_3                        (0x0BUL << 0)   //  7     0                        117.3 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_156_2                        (0x1AUL << 0)   //  7     0                        156.2 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_187_2                        (0x12UL << 0)   //  7     0                        187.2 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_234_3                        (0x0AUL << 0)   //  7     0                        234.3 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_312_0                        (0x19UL << 0)   //  7     0                        312.0 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_373_6                        (0x11UL << 0)   //  7     0                        373.6 kHz
#define RADIOLIB_LR11X0_GFSK_RX_BW_467_0                        (0x09UL << 0)   //  7     0                        467.0 kHz
#define RADIOLIB_LR11X0_LR_FHSS_SHAPING_GAUSSIAN_BT_1_0         (0x0BUL << 0)   //  7     0     shaping filter: Gaussian, BT = 1.0
#define RADIOLIB_LR11X0_SIGFOX_SHAPING_GAUSSIAN_BT_0_7          (0x16UL << 0)   //  7     0     shaping filter: Gaussian, BT = 0.7

// RADIOLIB_LR11X0_CMD_SET_PACKET_PARAMS
#define RADIOLIB_LR11X0_LORA_PAYLOAD_LEN_ANY                    (0x00UL << 0)   //  7     0     accept any payload length
#define RADIOLIB_LR11X0_LORA_IQ_STANDARD                        (0x00UL << 0)   //  7     0     IQ setup: standard
#define RADIOLIB_LR11X0_LORA_IQ_INVERTED                        (0x01UL << 0)   //  7     0               inverted
#define RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_DISABLED           (0x00UL << 0)   //  7     0     preamble detector: disabled
#define RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_8_BITS             (0x04UL << 0)   //  7     0                        8 bits
#define RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_16_BITS            (0x05UL << 0)   //  7     0                        16 bits
#define RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_24_BITS            (0x06UL << 0)   //  7     0                        24 bits
#define RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_32_BITS            (0x07UL << 0)   //  7     0                        32 bits
#define RADIOLIB_LR11X0_GFSK_ADDR_FILTER_DISABLED               (0x00UL << 0)   //  7     0     address filtering: disabled
#define RADIOLIB_LR11X0_GFSK_ADDR_FILTER_NODE                   (0x01UL << 0)   //  7     0                        node address
#define RADIOLIB_LR11X0_GFSK_ADDR_FILTER_NODE_BROADCAST         (0x02UL << 0)   //  7     0                        node and broadcast address
#define RADIOLIB_LR11X0_GFSK_PACKET_LENGTH_FIXED                (0x00UL << 0)   //  7     0     packet length: fixed
#define RADIOLIB_LR11X0_GFSK_PACKET_LENGTH_VARIABLE             (0x01UL << 0)   //  7     0                    variable
#define RADIOLIB_LR11X0_GFSK_PACKET_LENGTH_VARIABLE_SX128X      (0x02UL << 0)   //  7     0                    variable, SX128x 9-bit length encoding
#define RADIOLIB_LR11X0_GFSK_PAYLOAD_LEN_ANY                    (0x00UL << 0)   //  7     0     accept any payload length
#define RADIOLIB_LR11X0_GFSK_CRC_DISABLED                       (0x01UL << 0)   //  7     0     CRC: disabled
#define RADIOLIB_LR11X0_GFSK_CRC_1_BYTE                         (0x00UL << 0)   //  7     0          1-byte
#define RADIOLIB_LR11X0_GFSK_CRC_2_BYTE                         (0x02UL << 0)   //  7     0          2-byte
#define RADIOLIB_LR11X0_GFSK_CRC_1_BYTE_INV                     (0x04UL << 0)   //  7     0          1-byte, inverted
#define RADIOLIB_LR11X0_GFSK_CRC_2_BYTE_INV                     (0x06UL << 0)   //  7     0          2-byte, inverted
#define RADIOLIB_LR11X0_GFSK_WHITENING_DISABLED                 (0x00UL << 0)   //  7     0     whitening: disabled
#define RADIOLIB_LR11X0_GFSK_WHITENING_ENABLED                  (0x01UL << 0)   //  7     0                enabled

// RADIOLIB_LR11X0_CMD_SET_RX_TX_FALLBACK_MODE
#define RADIOLIB_LR11X0_FALLBACK_MODE_STBY_RC                   (0x01UL << 0)   //  1     0     fallback mode after Rx/Tx: standby with RC
#define RADIOLIB_LR11X0_FALLBACK_MODE_STBY_XOSC                 (0x02UL << 0)   //  1     0                                standby with XOSC
#define RADIOLIB_LR11X0_FALLBACK_MODE_FS                        (0x03UL << 0)   //  1     0                                frequency synthesis

// RADIOLIB_LR11X0_CMD_SET_RX_DUTY_CYCLE
#define RADIOLIB_LR11X0_RX_DUTY_CYCLE_MODE_RX                   (0x00UL << 0)   //  0     0     mode in Rx windows: Rx (default)
#define RADIOLIB_LR11X0_RX_DUTY_CYCLE_MODE_CAD                  (0x01UL << 0)   //  0     0                         CAD
#define RADIOLIB_LR11X0_TIMING_STEP                             (1.0f/32768.0f) //  23    0     timing step fo delays

// RADIOLIB_LR11X0_CMD_SET_PA_CONFIG
#define RADIOLIB_LR11X0_PA_SEL_LP                               (0x00UL << 0)   //  7     0     PA select: low power PA
#define RADIOLIB_LR11X0_PA_SEL_HP                               (0x01UL << 0)   //  7     0                high power PA
#define RADIOLIB_LR11X0_PA_SEL_HF                               (0x02UL << 0)   //  7     0                high frequency PA
#define RADIOLIB_LR11X0_PA_SUPPLY_INTERNAL                      (0x00UL << 0)   //  7     0     PA power source: internal
#define RADIOLIB_LR11X0_PA_SUPPLY_VBAT                          (0x01UL << 0)   //  7     0                      VBAT (required for >= 14 dBm)
#define RADIOLIB_LR11X0_PA_DC_DEFAULT                           (0x04UL << 0)   //  7     0     PA duty cycle: default value
#define RADIOLIB_LR11X0_PA_DC_MAX                               (0x07UL << 0)   //  7     0                    maximum value

// RADIOLIB_LR11X0_CMD_STOP_TIMEOUT_ON_PREAMBLE
#define RADIOLIB_LR11X0_STOP_ON_SYNC_HEADER                     (0x00UL << 0)   //  0     0     stop timeout on: sync word or header (default)
#define RADIOLIB_LR11X0_STOP_ON_PREAMBLE                        (0x01UL << 0)   //  0     0                      preamble

// RADIOLIB_LR11X0_CMD_GET_RANGING_RESULT
#define RADIOLIB_LR11X0_RANGING_RESULT_DISTANCE                 (0)             //  7     0     ranging result type: distance
#define RADIOLIB_LR11X0_RANGING_RESULT_RSSI                     (1)             //  7     0                          RSSI

// RADIOLIB_LR11X0_CMD_SET_RX_BOOSTED
#define RADIOLIB_LR11X0_RX_BOOSTED_ENABLED                      (0x01UL << 0)   //  0     0     Rx boosted mode: enabled
#define RADIOLIB_LR11X0_RX_BOOSTED_DISABLED                     (0x00UL << 0)   //  0     0                      disabled

// RADIOLIB_LR11X0_CMD_SET_LORA_SYNC_WORD
#define RADIOLIB_LR11X0_LORA_SYNC_WORD_PRIVATE                  (0x12)
#define RADIOLIB_LR11X0_LORA_SYNC_WORD_PUBLIC                   (0x34)

// RADIOLIB_LR11X0_CMD_GET_LORA_RX_HEADER_INFOS
#define RADIOLIB_LR11X0_LAST_HEADER_CRC_ENABLED                 (0x01UL << 4)   //  4     4     last header CRC: enabled
#define RADIOLIB_LR11X0_LAST_HEADER_CRC_DISABLED                (0x00UL << 4)   //  4     4                      disabled

// RADIOLIB_LR11X0_CMD_WIFI_SCAN
#define RADIOLIB_LR11X0_WIFI_SCAN_802_11_B                      (0x01UL << 0)   //  7     0     Wi-Fi type to scan: 802.11b
#define RADIOLIB_LR11X0_WIFI_SCAN_802_11_G                      (0x02UL << 0)   //  7     0                         802.11g
#define RADIOLIB_LR11X0_WIFI_SCAN_802_11_N                      (0x03UL << 0)   //  7     0                         802.11n
#define RADIOLIB_LR11X0_WIFI_SCAN_ALL                           (0x04UL << 0)   //  7     0                         all (802.11b first)
#define RADIOLIB_LR11X0_WIFI_ACQ_MODE_BEACON_ONLY               (0x01UL << 0)   //  7     0     Wi-Fi acquisition mode: beacon only
#define RADIOLIB_LR11X0_WIFI_ACQ_MODE_BEACON_PACKET             (0x02UL << 0)   //  7     0                             beacon and packet
#define RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_TRAFFIC              (0x03UL << 0)   //  7     0                             full traffic
#define RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON               (0x04UL << 0)   //  7     0                             full beacon
#define RADIOLIB_LR11X0_WIFI_ACQ_MODE_SSID_BEACON               (0x05UL << 0)   //  7     0                             SSID beacon
#define RADIOLIB_LR11X0_WIFI_ABORT_ON_TIMEOUT_ENABLED           (0x01UL << 0)   //  7     0     abort scanning on preamble timeout: enabled
#define RADIOLIB_LR11X0_WIFI_ABORT_ON_TIMEOUT_DISABLED          (0x00UL << 0)   //  7     0                                         disabled
#define RADIOLIB_LR11X0_WIFI_MAX_NUM_RESULTS                    (32)            //  7     0     maximum possible number of Wi-Fi scan results
#define RADIOLIB_LR11X0_WIFI_ALL_CHANNELS                       (0x3FFFUL)      //  16    0     scan all channels

// RADIOLIB_LR11X0_CMD_WIFI_READ_RESULTS
#define RADIOLIB_LR11X0_WIFI_RESULT_TYPE_COMPLETE               (0x01UL << 0)   //  7     0     Wi-Fi scan result type: complete
#define RADIOLIB_LR11X0_WIFI_RESULT_TYPE_BASIC                  (0x04UL << 0)   //  7     0                             basic
#define RADIOLIB_LR11X0_WIFI_RESULT_MAX_LEN                     (79)            //  7     0     maximum possible Wi-Fi scan size

// RADIOLIB_LR11X0_CMD_GNSS_SET_CONSTELLATION_TO_USE
#define RADIOLIB_LR11X0_GNSS_CONSTELLATION_GPS                  (0x01UL << 0)   //  7     0     GNSS constellation to use: GPS
#define RADIOLIB_LR11X0_GNSS_CONSTELLATION_BEIDOU               (0x01UL << 1)   //  7     0                                BeiDou

// RADIOLIB_LR11X0_CMD_GNSS_SET_MODE
#define RADIOLIB_LR11X0_GNSS_MODE_SINGLE_SCAN                   (0x00UL << 0)   //  7     0     GNSS scanning mode: single/legacy
#define RADIOLIB_LR11X0_GNSS_MODE_SINGLE_MULTIPLE               (0x03UL << 1)   //  7     0                         multiple/advanced

// RADIOLIB_LR11X0_CMD_GNSS_AUTONOMOUS
#define RADIOLIB_LR11X0_GNSS_RES_PSEUDO_DOPPLER_ENABLED         (0x01UL << 0)   //  0     0     GNSS results in NAV message: pseudo-range (in single scan mode) or Doppler information (in multiple scan mode)
#define RADIOLIB_LR11X0_GNSS_RES_PSEUDO_DOPPLER_DISABLED        (0x00UL << 0)   //  0     0                                  not included
#define RADIOLIB_LR11X0_GNSS_RES_DOPPLER_ENABLED                (0x01UL << 1)   //  1     1                                  Doppler information
#define RADIOLIB_LR11X0_GNSS_RES_DOPPLER_DISABLED               (0x00UL << 1)   //  1     1                                  not included
#define RADIOLIB_LR11X0_GNSS_NB_SV_ALL                          (0x00UL << 0)   //  7     0     include all detected satellites
#define RADIOLIB_LR11X0_GNSS_AUTO_EFFORT_MODE                   (0x00UL << 0)   //  7     0     reserved, always 0

// RADIOLIB_LR11X0_CMD_GNSS_ASSISTED
#define RADIOLIB_LR11X0_GNSS_ASSIST_LOW_POWER                   (0x00UL << 0)   //  7     0     effort mode: low power
#define RADIOLIB_LR11X0_GNSS_ASSIST_BEST_EFFORT                 (0x01UL << 0)   //  7     0                  best effort

// RADIOLIB_LR11X0_CMD_GNSS_GET_CONTEXT_STATUS
#define RADIOLIB_LR11X0_GNSS_CONTEXT_ERR_NONE                   (0x00UL << 0)   //  7     4     error code: none
#define RADIOLIB_LR11X0_GNSS_CONTEXT_ERR_ALMANAC_OLD            (0x01UL << 0)   //  7     4                 almanac too old
#define RADIOLIB_LR11X0_GNSS_CONTEXT_ERR_ALMANAC_CRC            (0x02UL << 0)   //  7     4                 almanac CRC mismatch
#define RADIOLIB_LR11X0_GNSS_CONTEXT_ERR_FLASH                  (0x03UL << 0)   //  7     4                 flash integrity error
#define RADIOLIB_LR11X0_GNSS_CONTEXT_ERR_ALMANAC_UPD            (0x04UL << 0)   //  7     4                 almanac update not allowed
#define RADIOLIB_LR11X0_GNSS_CONTEXT_FREQ_SPACE_250_HZ          (0x00UL << 0)   //  8     7     frequency search space: 250 Hz
#define RADIOLIB_LR11X0_GNSS_CONTEXT_FREQ_SPACE_500_HZ          (0x01UL << 0)   //  8     7                             500 Hz
#define RADIOLIB_LR11X0_GNSS_CONTEXT_FREQ_SPACE_1000_HZ         (0x02UL << 0)   //  8     7                             1000 Hz
#define RADIOLIB_LR11X0_GNSS_CONTEXT_FREQ_SPACE_2000_HZ         (0x03UL << 0)   //  8     7                             2000 Hz

// RADIOLIB_LR11X0_CMD_GNSS_GET_SV_VISIBLE
#define RADIOLIB_LR11X0_SV_CONSTELLATION_GPS                    (0x00UL << 0)   //  7     0     GNSS constellation: GPS
#define RADIOLIB_LR11X0_SV_CONSTELLATION_BEIDOU                 (0x01UL << 0)   //  7     0                         BeiDou

// RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_FULL_UPDATE
#define RADIOLIB_LR11X0_GNSS_ALMANAC_HEADER_ID                  (0x80UL << 0)   //  7     0     starting byte of GNSS almanac header
#define RADIOLIB_LR11X0_GNSS_ALMANAC_BLOCK_SIZE                 (20)

// RADIOLIB_LR11X0_CMD_GNSS_FETCH_TIME
#define RADIOLIB_LR11X0_GNSS_EFFORT_LOW                         (0x00UL << 0)   //  7     0     GNSS effort mode: low sensitivity
#define RADIOLIB_LR11X0_GNSS_EFFORT_MID                         (0x01UL << 0)   //  7     0                        medium sensitivity
#define RADIOLIB_LR11X0_GNSS_FETCH_TIME_OPT_TOW                 (0x00UL << 0)   //  7     0     time fetch options: ToW only, requires WN to demodulated beforehand
#define RADIOLIB_LR11X0_GNSS_FETCH_TIME_OPT_TOW_WN              (0x01UL << 0)   //  7     0                         ToW and WN
#define RADIOLIB_LR11X0_GNSS_FETCH_TIME_OPT_TOW_WN_ROLL         (0x02UL << 0)   //  7     0                         ToW, WN and rollover

// RADIOLIB_LR11X0_CMD_GNSS_READ_DEMOD_STATUS
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NOT_POSSIBLE          (-21)           //  7     0     GNSS demodulation status: not possible to demodulate
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_SAT_LOST              (-20)           //  7     0                               satellite lost
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_ALMANAC_DEMOD_ERROR   (-19)           //  7     0                               almanac demodulation error
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_TOO_LATE              (-18)           //  7     0                               woke up after preamble (demodulation started too late)
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_20_MS_FAIL            (-17)           //  7     0                               20ms real-time clock failed
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WAKE_UP_FAIL          (-16)           //  7     0                               wake up sync failed
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WN_INVALID            (-15)           //  7     0                               week number not validated
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NO_ACTIVE_SAT         (-14)           //  7     0                               no active satellite selected in satellite list
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_SLEEP_TOO_LONG        (-13)           //  7     0                               sleep time too long
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_TOW_INVALID           (-12)           //  7     0                               wrong time-of-week demodulated
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_PREAMBLE_INVALID      (-11)           //  7     0                               preamble not validated
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_DISABLED              (-10)           //  7     0                               demodulator disabled
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_EXTR_FAILED           (-9)            //  7     0                               demodulator extraction failed
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NO_BIT_CHANGE         (-8)            //  7     0                               no bit change found during demodulation start
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NO_BIT_CHANGE_ADV     (-7)            //  7     0                               no bit change found during advanced scan
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NO_SAT_FOUND          (-6)            //  7     0                               no satellites found
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_SYNC_LOST             (-5)            //  7     0                               word sync lost
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_PARITY_NOT_ENOUGH     (-3)            //  7     0                               parity check fail (not enough)
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_PARITY_TOO_MANY       (-2)            //  7     0                               parity check fail (too many)
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_NO_PARITY             (-1)            //  7     0                               parity check fail (no parity found)
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WORD_SYNC_NONE        (0)             //  7     0                               word sync search not started
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WORD_SYNC_POT         (1)             //  7     0                               potential word sync found
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WORD_SYNC_OK          (2)             //  7     0                               word sync found
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_TOW_FOUND             (3)             //  7     0                               time-of-week found
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_WN_FOUND              (4)             //  7     0                               week number and time-of-week found
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_ALM_FOUND_UNSAVED     (5)             //  7     0                               almanac found but not saved
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_HALF_ALM_SAVED        (6)             //  7     0                               half of almanac found and saved
#define RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_FULL_ALM_SAVED        (7)             //  7     0                               full almanac found and saved
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_WORD_SYNC_FOUND         (0x01UL << 0)   //  7     0     GNSS demodulation info: word synchronization found
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_TOW_FOUND               (0x01UL << 1)   //  7     0                             time-of-week found
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_WN_DEMODED              (0x01UL << 2)   //  7     0                             week number demodulated
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_WN_FOUND                (0x01UL << 3)   //  7     0                             week number found
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_SUBFRAME_1_FOUND        (0x01UL << 4)   //  7     0                             subframe 1 found
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_SUBFRAME_4_FOUND        (0x01UL << 5)   //  7     0                             subframe 4 found
#define RADIOLIB_LR11X0_GNSS_DEMOD_INFO_SUBFRAME_5_FOUND        (0x01UL << 6)   //  7     0                             subframe 5 found

// RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_STATUS
#define RADIOLIB_LR11X0_GNSS_ALMANAC_STATUS_UP_TO_DATE          (0)             //  7     0     GPS/BeiDou almanac status: all satellites up-to-date
#define RADIOLIB_LR11X0_GNSS_ALMANAC_STATUS_OUTDATED            (1)             //  7     0                                at least one satellite needs update

// RADIOLIB_LR11X0_CMD_GNSS_READ_DOPPLER_SOLVER_RES
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_NONE                    (0)             //  7     0     internal 2D solver error: no error
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_RES_HIGH                (1)             //  7     0                               residue too high
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_NOT_CONVERGED           (2)             //  7     0                               not converged on solution
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_NOT_ENOUGH_SV           (3)             //  7     0                               not enough satellites
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_ILL_MATRIX              (4)             //  7     0                               matrix error (?)
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_TIME                    (5)             //  7     0                               time error
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_ALM_PART_OLD            (6)             //  7     0                               part of almanac too old or not available
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_INCONSISTENT            (7)             //  7     0                               not consistent with history (?)
#define RADIOLIB_LR11X0_GNSS_SOLVER_ERR_ALM_OLD                 (8)             //  7     0                               all of almanac too old

// RADIOLIB_LR11X0_CMD_CRYPTO_SET_KEY
#define RADIOLIB_LR11X0_CRYPTO_STATUS_SUCCESS                   (0x00UL << 0)   //  7     0     crypto engine status: success
#define RADIOLIB_LR11X0_CRYPTO_STATUS_FAIL_CMAC                 (0x01UL << 0)   //  7     0                           MIC check failed
#define RADIOLIB_LR11X0_CRYPTO_STATUS_INV_KEY_ID                (0x03UL << 0)   //  7     0                           key/parameter source or destination ID error
#define RADIOLIB_LR11X0_CRYPTO_STATUS_BUF_SIZE                  (0x05UL << 0)   //  7     0                           data buffer size invalid
#define RADIOLIB_LR11X0_CRYPTO_STATUS_ERROR                     (0x06UL << 0)   //  7     0                           generic error

// RADIOLIB_LR11X0_CMD_CRYPTO_PROCESS_JOIN_ACCEPT
#define RADIOLIB_LR11X0_CRYPTO_LORAWAN_VERSION_1_0              (0x00UL << 0)   //  7     0     LoRaWAN version: 1.0.x
#define RADIOLIB_LR11X0_CRYPTO_LORAWAN_VERSION_1_1              (0x01UL << 0)   //  7     0                      1.1

#endif

#endif
