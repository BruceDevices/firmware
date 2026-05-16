#if !defined(_RADIOLIB_LORAWAN_H) && !RADIOLIB_EXCLUDE_LORAWAN
#define _RADIOLIB_LORAWAN_H

#include "../../TypeDef.h"
#include "../PhysicalLayer/PhysicalLayer.h"
#include "../../utils/Cryptography.h"

// activation mode
#define RADIOLIB_LORAWAN_MODE_OTAA                              (0x07AA)
#define RADIOLIB_LORAWAN_MODE_ABP                               (0x0AB9)
#define RADIOLIB_LORAWAN_MODE_NONE                              (0x0000)

// operation mode
#define RADIOLIB_LORAWAN_CLASS_A                                (0x00)
#define RADIOLIB_LORAWAN_CLASS_B                                (0x01)
#define RADIOLIB_LORAWAN_CLASS_C                                (0x02)

// preamble format
#define RADIOLIB_LORAWAN_LORA_SYNC_WORD                         (0x34)
#define RADIOLIB_LORAWAN_LORA_PREAMBLE_LEN                      (8)
#define RADIOLIB_LORAWAN_GFSK_SYNC_WORD                         (0xC194C1)
#define RADIOLIB_LORAWAN_GFSK_PREAMBLE_LEN                      (5)
#define RADIOLIB_LORAWAN_LR_FHSS_SYNC_WORD                      (0x2C0F7995)

// MAC header field encoding                                                    MSB   LSB   DESCRIPTION
#define RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_REQUEST                (0x00 << 5) //  7     5     message type: join request
#define RADIOLIB_LORAWAN_MHDR_MTYPE_JOIN_ACCEPT                 (0x01 << 5) //  7     5                   join accept
#define RADIOLIB_LORAWAN_MHDR_MTYPE_UNCONF_DATA_UP              (0x02 << 5) //  7     5                   unconfirmed data up
#define RADIOLIB_LORAWAN_MHDR_MTYPE_UNCONF_DATA_DOWN            (0x03 << 5) //  7     5                   unconfirmed data down
#define RADIOLIB_LORAWAN_MHDR_MTYPE_CONF_DATA_UP                (0x04 << 5) //  7     5                   confirmed data up
#define RADIOLIB_LORAWAN_MHDR_MTYPE_CONF_DATA_DOWN              (0x05 << 5) //  7     5                   confirmed data down
#define RADIOLIB_LORAWAN_MHDR_MTYPE_PROPRIETARY                 (0x07 << 5) //  7     5                   proprietary
#define RADIOLIB_LORAWAN_MHDR_MTYPE_MASK                        (0x07 << 5) //  7     5                   bitmask of all possible options
#define RADIOLIB_LORAWAN_MHDR_MAJOR_R1                          (0x00 << 0) //  1     0     major version: LoRaWAN R1

// frame control field encoding
#define RADIOLIB_LORAWAN_FCTRL_ADR_ENABLED                      (0x01 << 7) //  7     7     adaptive data rate: enabled
#define RADIOLIB_LORAWAN_FCTRL_ADR_DISABLED                     (0x00 << 7) //  7     7                         disabled
#define RADIOLIB_LORAWAN_FCTRL_ADR_ACK_REQ                      (0x01 << 6) //  6     6     adaptive data rate ACK request
#define RADIOLIB_LORAWAN_FCTRL_ACK                              (0x01 << 5) //  5     5     confirmed message acknowledge
#define RADIOLIB_LORAWAN_FCTRL_FRAME_PENDING                    (0x01 << 4) //  4     4     downlink frame is pending

// fPort field
#define RADIOLIB_LORAWAN_FPORT_MAC_COMMAND                      (0x00 << 0) //  7     0     payload contains MAC commands only
#define RADIOLIB_LORAWAN_FPORT_PAYLOAD_MIN                      (0x01 << 0) //  7     0     start of user-allowed fPort range
#define RADIOLIB_LORAWAN_FPORT_PAYLOAD_MAX                      (0xDF << 0) //  7     0     end of user-allowed fPort range
#define RADIOLIB_LORAWAN_FPORT_RESERVED                         (0xE0 << 0) //  7     0     fPort values equal to and larger than this are reserved

// data rate encoding
#define RADIOLIB_LORAWAN_DATA_RATE_UNUSED                       (0x0F << 0) // reserved / unused data rate
#define RADIOLIB_LORAWAN_TX_POWER_UNUSED                        (0x0F << 0) // reserved / unused Tx power

// channels and channel plans
#define RADIOLIB_LORAWAN_UPLINK                                 (0x00 << 0)
#define RADIOLIB_LORAWAN_DOWNLINK                               (0x01 << 0)
#define RADIOLIB_LORAWAN_RX1                                    (0x01 << 0)
#define RADIOLIB_LORAWAN_RX2                                    (0x02 << 0)
#define RADIOLIB_LORAWAN_RX_BC                                  (0x03 << 0)
#define RADIOLIB_LORAWAN_BAND_DYNAMIC                           (0)
#define RADIOLIB_LORAWAN_BAND_FIXED                             (1)
#define RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES                  (15)

// recommended default settings
#define RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS                     (1000)
#define RADIOLIB_LORAWAN_RECEIVE_DELAY_2_MS                     ((RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS) + 1000)
#define RADIOLIB_LORAWAN_RX1_DR_OFFSET                          (0)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_1_MS                 (5000)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DELAY_2_MS                 (6000)
#define RADIOLIB_LORAWAN_ADR_ACK_LIMIT_EXP                      (0x06)
#define RADIOLIB_LORAWAN_ADR_ACK_DELAY_EXP                      (0x05)
#define RADIOLIB_LORAWAN_RETRANSMIT_TIMEOUT_MIN_MS              (1000)
#define RADIOLIB_LORAWAN_RETRANSMIT_TIMEOUT_MAX_MS              (3000)
#define RADIOLIB_LORAWAN_POWER_STEP_SIZE_DBM                    (-2)
#define RADIOLIB_LORAWAN_REJOIN_MAX_COUNT_N                     (10)  // send rejoin request 16384 uplinks
#define RADIOLIB_LORAWAN_REJOIN_MAX_TIME_N                      (15)  // once every year, not actually implemented

// join request message layout
#define RADIOLIB_LORAWAN_JOIN_REQUEST_LEN                       (23)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_JOIN_EUI_POS              (1)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_EUI_POS               (9)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_DEV_NONCE_POS             (17)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE                      (0xFF)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE_0                    (0x00)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE_1                    (0x01)
#define RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE_2                    (0x02)

// join accept message layout
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_MAX_LEN                    (33)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_JOIN_NONCE_POS             (1)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_HOME_NET_ID_POS            (4)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DEV_ADDR_POS               (7)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_DL_SETTINGS_POS            (11)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_RX_DELAY_POS               (12)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS                 (13)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN                 (16)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_TYPE_POS            (RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_POS + RADIOLIB_LORAWAN_JOIN_ACCEPT_CFLIST_LEN - 1)

// join accept key derivation layout
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_AES_JOIN_NONCE_POS         (1)   // regular keys
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_AES_JOIN_EUI_POS           (4)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_AES_DEV_NONCE_POS          (12)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_AES_DEV_ADDR_POS           (1)   // relay keys

// join accept message variables
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_R_1_0                      (0x00 << 7) //  7     7     LoRaWAN revision: 1.0
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_R_1_1                      (0x01 << 7) //  7     7                       1.1
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_F_NWK_S_INT_KEY            (0x01)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_APP_S_KEY                  (0x02)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_S_NWK_S_INT_KEY            (0x03)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_NWK_S_ENC_KEY              (0x04)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_JS_ENC_KEY                 (0x05)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_JS_INT_KEY                 (0x06)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_ROOT_WOR_S_KEY             (0x01)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_WOR_S_INT_KEY              (0x01)
#define RADIOLIB_LORAWAN_JOIN_ACCEPT_WOR_S_ENC_KEY              (0x02)

// frame header layout
#define RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS                    (16)
#define RADIOLIB_LORAWAN_FHDR_DEV_ADDR_POS                      (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 1)
#define RADIOLIB_LORAWAN_FHDR_FCTRL_POS                         (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 5)
#define RADIOLIB_LORAWAN_FHDR_FCNT_POS                          (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 6)
#define RADIOLIB_LORAWAN_FHDR_FOPTS_POS                         (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 8)
#define RADIOLIB_LORAWAN_FHDR_FOPTS_LEN_MASK                    (0x0F)
#define RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN                     (15)
#define RADIOLIB_LORAWAN_FHDR_FPORT_POS(FOPTS)                  (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 8 + (FOPTS))
#define RADIOLIB_LORAWAN_FRAME_PAYLOAD_POS(FOPTS)               (RADIOLIB_LORAWAN_FHDR_LEN_START_OFFS + 9 + (FOPTS))
#define RADIOLIB_LORAWAN_FRAME_LEN(PAYLOAD, FOPTS)              (16 + 13 + (PAYLOAD) + (FOPTS))

// payload encryption/MIC blocks common layout
#define RADIOLIB_LORAWAN_BLOCK_MAGIC_POS                        (0)
#define RADIOLIB_LORAWAN_BLOCK_CONF_FCNT_POS                    (1)
#define RADIOLIB_LORAWAN_BLOCK_DIR_POS                          (5)
#define RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS                     (6)
#define RADIOLIB_LORAWAN_BLOCK_FCNT_POS                         (10)

// payload encryption block layout
#define RADIOLIB_LORAWAN_ENC_BLOCK_MAGIC                        (0x01)
#define RADIOLIB_LORAWAN_ENC_BLOCK_COUNTER_ID_POS               (4)
#define RADIOLIB_LORAWAN_ENC_BLOCK_COUNTER_POS                  (15)

// payload MIC blocks layout
#define RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC                        (0x49)
#define RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS                      (15)
#define RADIOLIB_LORAWAN_MIC_DATA_RATE_POS                      (3)
#define RADIOLIB_LORAWAN_MIC_CH_INDEX_POS                       (4)

// maximum allowed dwell time on bands that implement dwell time limitations
#define RADIOLIB_LORAWAN_DWELL_TIME                             (400)

// unused frame counter value
#define RADIOLIB_LORAWAN_FCNT_NONE                              (0xFFFFFFFF)

// TR013 CSMA recommended values
#define RADIOLIB_LORAWAN_DIFS_DEFAULT                           (2)
#define RADIOLIB_LORAWAN_BACKOFF_MAX_DEFAULT                    (6)
#define RADIOLIB_LORAWAN_MAX_CHANGES_DEFAULT                    (4)

// MAC commands
#define RADIOLIB_LORAWAN_NUM_MAC_COMMANDS                       (24)

#define RADIOLIB_LORAWAN_MAC_RESET                              (0x01)
#define RADIOLIB_LORAWAN_MAC_LINK_CHECK                         (0x02)
#define RADIOLIB_LORAWAN_MAC_LINK_ADR                           (0x03)
#define RADIOLIB_LORAWAN_MAC_DUTY_CYCLE                         (0x04)
#define RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP                     (0x05)
#define RADIOLIB_LORAWAN_MAC_DEV_STATUS                         (0x06)
#define RADIOLIB_LORAWAN_MAC_NEW_CHANNEL                        (0x07)
#define RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP                    (0x08)
#define RADIOLIB_LORAWAN_MAC_TX_PARAM_SETUP                     (0x09)
#define RADIOLIB_LORAWAN_MAC_DL_CHANNEL                         (0x0A)
#define RADIOLIB_LORAWAN_MAC_REKEY                              (0x0B)
#define RADIOLIB_LORAWAN_MAC_ADR_PARAM_SETUP                    (0x0C)
#define RADIOLIB_LORAWAN_MAC_DEVICE_TIME                        (0x0D)
#define RADIOLIB_LORAWAN_MAC_FORCE_REJOIN                       (0x0E)
#define RADIOLIB_LORAWAN_MAC_REJOIN_PARAM_SETUP                 (0x0F)
#define RADIOLIB_LORAWAN_MAC_DEVICE_MODE                        (0x20)
#define RADIOLIB_LORAWAN_MAC_PROPRIETARY                        (0x80)

// number of supported LoRaWAN TS packages
#define RADIOLIB_LORAWAN_NUM_SUPPORTED_PACKAGES                 (7)

// Package ID numbering as per TS008 (TS011 ID is made up)
#define RADIOLIB_LORAWAN_PACKAGE_TS007                          (0)
#define RADIOLIB_LORAWAN_PACKAGE_TS003                          (1)
#define RADIOLIB_LORAWAN_PACKAGE_TS005                          (2)
#define RADIOLIB_LORAWAN_PACKAGE_TS004                          (3)
#define RADIOLIB_LORAWAN_PACKAGE_TS006                          (4)
#define RADIOLIB_LORAWAN_PACKAGE_TS009                          (5)
#define RADIOLIB_LORAWAN_PACKAGE_TS011                          (6)

// Package FPort (specified or recommended)
#define RADIOLIB_LORAWAN_FPORT_TS007                            (225)
#define RADIOLIB_LORAWAN_FPORT_TS003                            (202)
#define RADIOLIB_LORAWAN_FPORT_TS005                            (200)
#define RADIOLIB_LORAWAN_FPORT_TS004                            (201)
#define RADIOLIB_LORAWAN_FPORT_TS006                            (203)
#define RADIOLIB_LORAWAN_FPORT_TS009                            (224)
#define RADIOLIB_LORAWAN_FPORT_TS011                            (226)

// the maximum number of simultaneously available channels
#define RADIOLIB_LORAWAN_MAX_NUM_DYNAMIC_CHANNELS               (16)
#define RADIOLIB_LORAWAN_MAX_NUM_SUBBANDS                       (12)
#define RADIOLIB_LORAWAN_MAX_NUM_FIXED_CHANNELS                 (96)

// maximum MAC command sizes
#define RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_DOWN               (5)
#define RADIOLIB_LORAWAN_MAX_MAC_COMMAND_LEN_UP                 (2)
#define RADIOLIB_LORAWAN_MAX_NUM_ADR_COMMANDS                   (8)

#define RADIOLIB_LORAWAN_MAX_DOWNLINK_SIZE                      (250)

// session states
#define RADIOLIB_LORAWAN_SESSION_NONE                           (0x00)
#define RADIOLIB_LORAWAN_SESSION_ACTIVATING                     (0x01)
#define RADIOLIB_LORAWAN_SESSION_PENDING                        (0x02)
#define RADIOLIB_LORAWAN_SESSION_ACTIVE                         (0x03)

// threshold at which sleeping via user callback enabled, in ms
#define RADIOLIB_LORAWAN_DELAY_SLEEP_THRESHOLD                  (50)

/*!
  \struct LoRaWANMacCommand_t
  \brief MAC command specification structure.
*/
struct LoRaWANMacCommand_t {
  /*! \brief Command ID */
  const uint8_t cid;
  
  /*! \brief Uplink message length */
  const uint8_t lenDn;
  
  /*! \brief Downlink message length */
  const uint8_t lenUp;

  /*! \brief Some commands must be resent until Class A downlink received */
  const bool persist;
  
  /*! \brief Whether this MAC command can be issued by the user or not */
  const bool user;
};

#define RADIOLIB_LORAWAN_MAC_COMMAND_NONE { .cid = 0, .lenDn = 0, .lenUp = 0, .persist = false, .user = false }

constexpr LoRaWANMacCommand_t MacTable[RADIOLIB_LORAWAN_NUM_MAC_COMMANDS] = {
  { RADIOLIB_LORAWAN_MAC_RESET,               1, 1, true,  false },
  { RADIOLIB_LORAWAN_MAC_LINK_CHECK,          2, 0, false, true  },
  { RADIOLIB_LORAWAN_MAC_LINK_ADR,            4, 1, false, false },
  { RADIOLIB_LORAWAN_MAC_DUTY_CYCLE,          1, 0, false, false },
  { RADIOLIB_LORAWAN_MAC_RX_PARAM_SETUP,      4, 1, true,  false },
  { RADIOLIB_LORAWAN_MAC_DEV_STATUS,          0, 2, false, false },
  { RADIOLIB_LORAWAN_MAC_NEW_CHANNEL,         5, 1, false, false },
  { RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP,     1, 0, true,  false },
  { RADIOLIB_LORAWAN_MAC_TX_PARAM_SETUP,      1, 0, true,  false },
  { RADIOLIB_LORAWAN_MAC_DL_CHANNEL,          4, 1, true,  false },
  { RADIOLIB_LORAWAN_MAC_REKEY,               1, 1, true,  false },
  { RADIOLIB_LORAWAN_MAC_ADR_PARAM_SETUP,     1, 0, false, false },
  { RADIOLIB_LORAWAN_MAC_DEVICE_TIME,         5, 0, false, true  },
  { RADIOLIB_LORAWAN_MAC_FORCE_REJOIN,        2, 0, false, false },
  { RADIOLIB_LORAWAN_MAC_REJOIN_PARAM_SETUP,  1, 1, false, false },
  { RADIOLIB_LORAWAN_MAC_DEVICE_MODE,         1, 1, true,  false },
  { RADIOLIB_LORAWAN_MAC_PROPRIETARY,         5, 0, false, true  },
};

/*! \brief A user-supplied callback for LoRaWAN Application Packages (TSxxx) */
typedef void (*PackageCb_t)(uint8_t* dataDown, size_t lenDown);

/*!
  \struct LoRaWANPackage_t
  \brief LoRaWAN Packages structure (for TSxxx documents).
*/
struct LoRaWANPackage_t {
  /*! \brief Package ID as per TS008 */
  uint8_t packId;

  /*! \brief Package default FPort (specified or recommended) */
  uint8_t packFPort;

  /*! \brief Whether the package runs through the Application layer */
  bool isAppPack;

  /*! \brief Whether the FPort value has a fixed value by specification */
  bool fixedFPort;

  /*! \brief Whether the package is currently in use */
  bool enabled;

  /*! \brief User-provided callback for handling package downlinks */
  PackageCb_t callback;
};

constexpr LoRaWANPackage_t PackageTable[RADIOLIB_LORAWAN_NUM_SUPPORTED_PACKAGES] = {
  { RADIOLIB_LORAWAN_PACKAGE_TS007, RADIOLIB_LORAWAN_FPORT_TS007, true,  true,  false, NULL },
  { RADIOLIB_LORAWAN_PACKAGE_TS003, RADIOLIB_LORAWAN_FPORT_TS003, true,  false, false, NULL },
  { RADIOLIB_LORAWAN_PACKAGE_TS005, RADIOLIB_LORAWAN_FPORT_TS005, true,  false, false, NULL },
  { RADIOLIB_LORAWAN_PACKAGE_TS004, RADIOLIB_LORAWAN_FPORT_TS004, true,  false, false, NULL },
  { RADIOLIB_LORAWAN_PACKAGE_TS006, RADIOLIB_LORAWAN_FPORT_TS006, true,  false, false, NULL },
  { RADIOLIB_LORAWAN_PACKAGE_TS009, RADIOLIB_LORAWAN_FPORT_TS009, true,  true,  false, NULL },
  { RADIOLIB_LORAWAN_PACKAGE_TS011, RADIOLIB_LORAWAN_FPORT_TS011, false, true,  false, NULL }
};

#define RADIOLIB_LORAWAN_PACKAGE_NONE { .packId = 0, .packFPort = 0, .isAppPack = false, .fixedFPort = false, .enabled = false, .callback = NULL }

#define RADIOLIB_LORAWAN_NONCES_VERSION_VAL (0x0003)

enum LoRaWANSchemeBase_t {
  RADIOLIB_LORAWAN_NONCES_START       = 0x00,
  RADIOLIB_LORAWAN_NONCES_VERSION     = RADIOLIB_LORAWAN_NONCES_START,                            // 2 bytes
  RADIOLIB_LORAWAN_NONCES_MODE        = RADIOLIB_LORAWAN_NONCES_VERSION + sizeof(uint16_t),       // 2 bytes
  RADIOLIB_LORAWAN_NONCES_PLAN        = RADIOLIB_LORAWAN_NONCES_MODE + sizeof(uint16_t),          // 1 byte
  RADIOLIB_LORAWAN_NONCES_CHECKSUM    = RADIOLIB_LORAWAN_NONCES_PLAN + sizeof(uint8_t),           // 2 bytes
  RADIOLIB_LORAWAN_NONCES_DEV_NONCE   = RADIOLIB_LORAWAN_NONCES_CHECKSUM + sizeof(uint16_t),      // 2 bytes
  RADIOLIB_LORAWAN_NONCES_JOIN_NONCE  = RADIOLIB_LORAWAN_NONCES_DEV_NONCE + sizeof(uint16_t),     // 3 bytes
  RADIOLIB_LORAWAN_NONCES_SIGNATURE   = RADIOLIB_LORAWAN_NONCES_JOIN_NONCE + 3,                   // 2 bytes
  RADIOLIB_LORAWAN_NONCES_BUF_SIZE    = RADIOLIB_LORAWAN_NONCES_SIGNATURE + sizeof(uint16_t)      // Nonces buffer size
};

enum LoRaWANSchemeSession_t {
  RADIOLIB_LORAWAN_SESSION_START              = 0x00,
  RADIOLIB_LORAWAN_SESSION_STATUS             = RADIOLIB_LORAWAN_SESSION_START,                             // 1 byte
  RADIOLIB_LORAWAN_SESSION_NWK_SENC_KEY       = RADIOLIB_LORAWAN_SESSION_STATUS + 1,                        // 16 bytes
  RADIOLIB_LORAWAN_SESSION_APP_SKEY           = RADIOLIB_LORAWAN_SESSION_NWK_SENC_KEY + RADIOLIB_AES128_KEY_SIZE,   // 16 bytes
  RADIOLIB_LORAWAN_SESSION_FNWK_SINT_KEY      = RADIOLIB_LORAWAN_SESSION_APP_SKEY + RADIOLIB_AES128_KEY_SIZE,       // 16 bytes
  RADIOLIB_LORAWAN_SESSION_SNWK_SINT_KEY      = RADIOLIB_LORAWAN_SESSION_FNWK_SINT_KEY + RADIOLIB_AES128_KEY_SIZE,  // 16 bytes
  RADIOLIB_LORAWAN_SESSION_DEV_ADDR           = RADIOLIB_LORAWAN_SESSION_SNWK_SINT_KEY + RADIOLIB_AES128_KEY_SIZE,  // 4 bytes
  RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE   = RADIOLIB_LORAWAN_SESSION_DEV_ADDR + sizeof(uint32_t),               // 2 bytes
  RADIOLIB_LORAWAN_SESSION_FCNT_UP            = RADIOLIB_LORAWAN_SESSION_NONCES_SIGNATURE + sizeof(uint16_t),       // 4 bytes
  RADIOLIB_LORAWAN_SESSION_N_FCNT_DOWN        = RADIOLIB_LORAWAN_SESSION_FCNT_UP + sizeof(uint32_t),        // 4 bytes
  RADIOLIB_LORAWAN_SESSION_A_FCNT_DOWN        = RADIOLIB_LORAWAN_SESSION_N_FCNT_DOWN + sizeof(uint32_t),    // 4 bytes
  RADIOLIB_LORAWAN_SESSION_ADR_FCNT           = RADIOLIB_LORAWAN_SESSION_A_FCNT_DOWN + sizeof(uint32_t),    // 4 bytes
  RADIOLIB_LORAWAN_SESSION_CONF_FCNT_UP       = RADIOLIB_LORAWAN_SESSION_ADR_FCNT + sizeof(uint32_t), 	    // 4 bytes
  RADIOLIB_LORAWAN_SESSION_CONF_FCNT_DOWN     = RADIOLIB_LORAWAN_SESSION_CONF_FCNT_UP + sizeof(uint32_t),   // 4 bytes
  RADIOLIB_LORAWAN_SESSION_RJ_COUNT0          = RADIOLIB_LORAWAN_SESSION_CONF_FCNT_DOWN + sizeof(uint32_t), // 2 bytes
  RADIOLIB_LORAWAN_SESSION_RJ_COUNT1          = RADIOLIB_LORAWAN_SESSION_RJ_COUNT0 + sizeof(uint16_t), 	    // 2 bytes
  RADIOLIB_LORAWAN_SESSION_HOMENET_ID         = RADIOLIB_LORAWAN_SESSION_RJ_COUNT1 + sizeof(uint16_t), 	    // 4 bytes
  RADIOLIB_LORAWAN_SESSION_VERSION            = RADIOLIB_LORAWAN_SESSION_HOMENET_ID + sizeof(uint32_t), 	  // 1 byte
  RADIOLIB_LORAWAN_SESSION_CLASS              = RADIOLIB_LORAWAN_SESSION_VERSION + 1,                       // 1 byte
  RADIOLIB_LORAWAN_SESSION_LINK_ADR           = RADIOLIB_LORAWAN_SESSION_CLASS + sizeof(uint8_t),           // 14 bytes
  RADIOLIB_LORAWAN_SESSION_DUTY_CYCLE         = RADIOLIB_LORAWAN_SESSION_LINK_ADR + 14, 	                  // 1 byte
  RADIOLIB_LORAWAN_SESSION_RX_PARAM_SETUP     = RADIOLIB_LORAWAN_SESSION_DUTY_CYCLE + 1, 	                  // 4 bytes
  RADIOLIB_LORAWAN_SESSION_RX_TIMING_SETUP    = RADIOLIB_LORAWAN_SESSION_RX_PARAM_SETUP + 4, 	              // 1 byte
  RADIOLIB_LORAWAN_SESSION_TX_PARAM_SETUP     = RADIOLIB_LORAWAN_SESSION_RX_TIMING_SETUP + 1,               // 1 byte
  RADIOLIB_LORAWAN_SESSION_ADR_PARAM_SETUP    = RADIOLIB_LORAWAN_SESSION_TX_PARAM_SETUP + 1, 	              // 1 byte
  RADIOLIB_LORAWAN_SESSION_REJOIN_PARAM_SETUP = RADIOLIB_LORAWAN_SESSION_ADR_PARAM_SETUP + 1,               // 1 byte
  RADIOLIB_LORAWAN_SESSION_UL_CHANNELS        = RADIOLIB_LORAWAN_SESSION_REJOIN_PARAM_SETUP + 1, 	          // 16*5 bytes
  RADIOLIB_LORAWAN_SESSION_DL_CHANNELS        = RADIOLIB_LORAWAN_SESSION_UL_CHANNELS + RADIOLIB_LORAWAN_MAX_NUM_DYNAMIC_CHANNELS*5, // 16*4 bytes
  RADIOLIB_LORAWAN_SESSION_AVAILABLE_CHANNELS = RADIOLIB_LORAWAN_SESSION_DL_CHANNELS + RADIOLIB_LORAWAN_MAX_NUM_DYNAMIC_CHANNELS*4, // 2 bytes
  RADIOLIB_LORAWAN_SESSION_MAC_QUEUE          = RADIOLIB_LORAWAN_SESSION_AVAILABLE_CHANNELS + RADIOLIB_LORAWAN_MAX_NUM_SUBBANDS,    // 12 bytes                   // 15 bytes
  RADIOLIB_LORAWAN_SESSION_MAC_QUEUE_LEN      = RADIOLIB_LORAWAN_SESSION_MAC_QUEUE + RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN,           // 1 byte
  RADIOLIB_LORAWAN_SESSION_SIGNATURE          = RADIOLIB_LORAWAN_SESSION_MAC_QUEUE_LEN + sizeof(uint8_t),   // 2 bytes
  RADIOLIB_LORAWAN_SESSION_BUF_SIZE           = RADIOLIB_LORAWAN_SESSION_SIGNATURE + sizeof(uint16_t)       // Session buffer size
};

/*!
  \struct LoRaWANChannel_t
  \brief Structure to save information about LoRaWAN channels.
  To save space, adjacent channels are saved in "spans".
*/
struct LoRaWANChannel_t {
  /*! \brief The channel number, as specified by defaults or the network */
  uint8_t idx;

  /*! \brief The channel frequency (coded in 100 Hz steps) */
  uint32_t freq;

  /*! \brief Minimum allowed datarate for this channel */
  uint8_t drMin;

  /*! \brief Maximum allowed datarate for this channel (inclusive) */
  uint8_t drMax;

  /*! \brief Datarate currently in use on this channel */
  uint8_t dr;
};

// alias for unused channel
#define RADIOLIB_LORAWAN_CHANNEL_NONE    { .idx = 0, .freq = 0, .drMin = 0, .drMax = 0, \
                                           .dr = RADIOLIB_LORAWAN_DATA_RATE_UNUSED }

/*!
  \struct LoRaWANChannelSpan_t
  \brief Structure to save information about LoRaWAN channels.
  To save space, adjacent channels are saved in "spans".
*/
struct LoRaWANChannelSpan_t {
  /*! \brief Total number of channels in the span */
  uint8_t numChannels;

  /*! \brief Center frequency of the first channel in span (coded in 100 Hz steps) */
  uint32_t freqStart;

  /*! \brief Frequency step between adjacent channels (coded in 100 Hz steps) */
  uint32_t freqStep;

  /*! \brief Minimum allowed datarate for all channels in this span */
  uint8_t drMin;

  /*! \brief Maximum allowed datarate for all channels in this span (inclusive) */
  uint8_t drMax;
  
  /*! \brief Allowed data rates for a join request message */
  uint8_t drJoinRequest;
};

// alias for unused channel span
#define RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE    { .numChannels = 0, .freqStart = 0, .freqStep = 0, .drMin = 0, .drMax = 0, .drJoinRequest = RADIOLIB_LORAWAN_DATA_RATE_UNUSED }

struct LoRaWANDataRate_t {
  ModemType_t modem;
  DataRate_t dr;
  PacketConfig_t pc;
};

#define RADIOLIB_DATARATE_NONE { .modem = RADIOLIB_MODEM_NONE, .dr = {.lora = {0, 0, 0}}, .pc = {.lora = { 8, 0, 0, 0}}}

/*!
  \struct LoRaWANBand_t
  \brief Structure to save information about LoRaWAN band
*/
struct LoRaWANBand_t {
  /*! \brief Identier for this band */
  uint8_t bandNum;

  /*! \brief Whether the channels are fixed per specification, or dynamically allocated through the network (plus defaults) */
  uint8_t bandType;

  /*! \brief Minimum allowed frequency (coded in 100 Hz steps) */
  uint32_t freqMin;

  /*! \brief Maximum allowed frequency (coded in 100 Hz steps) */
  uint32_t freqMax;

  /*! \brief Array of allowed maximum application payload lengths for each data rate (N-value) */
  uint8_t payloadLenMax[RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES];

  /*! \brief Maximum allowed output power in this band in dBm */
  int8_t powerMax;

  /*! \brief Number of power steps in this band */
  int8_t powerNumSteps;

  /*! \brief Number of milliseconds per hour of allowed Time-on-Air */
  RadioLibTime_t dutyCycle;

  /*! \brief Maximum dwell time per uplink message in milliseconds */
  RadioLibTime_t dwellTimeUp;

  /*! \brief Maximum dwell time per downlink message in milliseconds */
  RadioLibTime_t dwellTimeDn;

  /*! \brief Whether this band implements the MAC command TxParamSetupReq */
  bool txParamSupported;

  /*! \brief A set of default uplink (TX) channels for dynamic bands */
  LoRaWANChannel_t txFreqs[3];

  /*! \brief The number of TX channel spans for fixed bands */
  uint8_t numTxSpans;

  /*! \brief Default uplink (TX) channel spans for fixed bands, including Join-Request parameters */
  LoRaWANChannelSpan_t txSpans[2];

  /*! \brief Default downlink (RX1) channel span for fixed bands */
  LoRaWANChannelSpan_t rx1Span;

  uint8_t rx1DrTable[RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES][8];

  /*! \brief Backup channel for downlink (RX2) window */
  LoRaWANChannel_t rx2;

  /*! \brief Relay channels for WoR uplink */
  LoRaWANChannel_t txWoR[2];

  /*! \brief Relay channels for ACK downlink */
  LoRaWANChannel_t txAck[2];
  
  /*! \brief The corresponding datarates, bandwidths and coding rates for DR index */
  LoRaWANDataRate_t dataRates[RADIOLIB_LORAWAN_CHANNEL_NUM_DATARATES];
};

// supported bands
extern const LoRaWANBand_t EU868;
extern const LoRaWANBand_t US915;
extern const LoRaWANBand_t EU433;
extern const LoRaWANBand_t AU915;
extern const LoRaWANBand_t CN470;
extern const LoRaWANBand_t AS923;
extern const LoRaWANBand_t AS923_2;
extern const LoRaWANBand_t AS923_3;
extern const LoRaWANBand_t AS923_4;
extern const LoRaWANBand_t KR920;
extern const LoRaWANBand_t IN865;

/*!
  \struct LoRaWANBandNum_t
  \brief IDs of all currently supported bands
*/
enum LoRaWANBandNum_t {
  BandEU868,
  BandUS915,
  BandEU433,
  BandAU915,
  BandCN470,
  BandAS923,
  BandAS923_2,
  BandAS923_3,
  BandAS923_4,
  BandKR920,
  BandIN865,
  BandLast
};

// provide easy access to the number of currently supported bands
#define RADIOLIB_LORAWAN_NUM_SUPPORTED_BANDS      (BandLast - BandEU868)

// array of currently supported bands
extern const LoRaWANBand_t* LoRaWANBands[];

/*!
  \struct LoRaWANJoinEvent_t
  \brief Structure to save extra information about activation event.
*/
struct LoRaWANJoinEvent_t {
  /*! \brief Whether a new session was started */
  bool newSession = false;

  /*! \brief The transmitted Join-Request DevNonce value */
  uint16_t devNonce = 0;

  /*! \brief The received Join-Request JoinNonce value */
  uint32_t joinNonce = 0;
};

/*!
  \struct LoRaWANEvent_t
  \brief Structure to save extra information about uplink/downlink event.
*/
struct LoRaWANEvent_t {
  /*! \brief Event direction, one of RADIOLIB_LORAWAN_CHANNEL_DIR_* */
  uint8_t dir;
  
  /*! \brief Whether the event is confirmed or not (e.g., confirmed uplink sent by user application) */
  bool confirmed;
  
  /*! \brief Whether the event is confirming a previous request
  (e.g., server downlink reply to confirmed uplink sent by user application)*/
  bool confirming;

  /*! \brief Whether further downlink messages are pending on the server side. */
  bool frmPending;

  /*! \brief Datarate */
  uint8_t datarate;
  
  /*! \brief Frequency in MHz */
  float freq;
  
  /*! \brief Transmit power in dBm for uplink, or RSSI for downlink */
  int16_t power;
  
  /*! \brief The appropriate frame counter - for different events, different frame counters will be reported! */
  uint32_t fCnt;
  
  /*! \brief Port number */
  uint8_t fPort;

  /*! \brief Number of times this uplink was transmitted (ADR)*/
  uint8_t nbTrans;

  /*! \brief Multicast or unicast */
  bool multicast;
};

/*!
  \class LoRaWANNode
  \brief LoRaWAN-compatible node (class A device).
*/
class LoRaWANNode {
  public:

    /*!
      \brief Default constructor.
      \param phy Pointer to the PhysicalLayer radio module.
      \param band Pointer to the LoRaWAN band to use.
      \param subBand The sub-band to be used (starting from 1!)
    */
    LoRaWANNode(PhysicalLayer* phy, const LoRaWANBand_t* band, uint8_t subBand = 0);

    /*!
      \brief Returns the pointer to the internal buffer that holds the LW base parameters
      \returns Pointer to uint8_t array of size RADIOLIB_LORAWAN_NONCES_BUF_SIZE
    */
    uint8_t* getBufferNonces();

    /*!
      \brief Fill the internal buffer that holds the LW base parameters with a supplied buffer
      \param persistentBuffer Buffer that should match the internal format (previously extracted using getBufferNonces)
      \returns \ref status_codes
    */
    int16_t setBufferNonces(const uint8_t* persistentBuffer);

    /*!
      \brief Clear an active session. This requires the device to rejoin the network.
    */
    void clearSession();

    /*!
      \brief Returns the pointer to the internal buffer that holds the LW session parameters
      \returns Pointer to uint8_t array of size RADIOLIB_LORAWAN_SESSION_BUF_SIZE
    */
    uint8_t* getBufferSession();

    /*!
      \brief Fill the internal buffer that holds the LW session parameters with a supplied buffer
      \param persistentBuffer Buffer that should match the internal format (previously extracted using getBufferSession)
      \returns \ref status_codes
    */
    int16_t setBufferSession(const uint8_t* persistentBuffer);

    /*!
      \brief Set the device credentials and activation configuration
      \param joinEUI 8-byte application identifier.
      \param devEUI 8-byte device identifier.
      \param nwkKey Pointer to the network AES-128 key.
      \param appKey Pointer to the application AES-128 key.
      \returns \ref status_codes
    */
    int16_t beginOTAA(uint64_t joinEUI, uint64_t devEUI, const uint8_t* nwkKey, const uint8_t* appKey);

    /*!
      \brief Set the device credentials and activation configuration
      \param addr Device address.
      \param fNwkSIntKey Pointer to the Forwarding network session (LoRaWAN 1.1), NULL for LoRaWAN 1.0.
      \param sNwkSIntKey Pointer to the Serving network session (LoRaWAN 1.1), NULL for LoRaWAN 1.0.
      \param nwkSEncKey Pointer to the MAC command network session key [NwkSEncKey] (LoRaWAN 1.1) 
                                    or network session AES-128 key [NwkSKey] (LoRaWAN 1.0).
      \param appSKey Pointer to the application session AES-128 key.
      \returns \ref status_codes
    */
    int16_t beginABP(uint32_t addr, const uint8_t* fNwkSIntKey, const uint8_t* sNwkSIntKey, const uint8_t* nwkSEncKey, const uint8_t* appSKey);

    /*!
      \brief Join network by restoring OTAA session or performing over-the-air activation. By this procedure,
      the device will perform an exchange with the network server and set all necessary configuration. 
      \returns \ref status_codes
    */
    virtual int16_t activateOTAA(LoRaWANJoinEvent_t *joinEvent = NULL);

    /*!
      \brief Join network by restoring ABP session or performing over-the-air activation. 
      In this procedure, all necessary configuration must be provided by the user.
      \returns \ref status_codes
    */
    virtual int16_t activateABP();

    /*! \brief Whether there is an ongoing session active */
    bool isActivated();

    /*! \brief Configure class (RADIOLIB_LORAWAN_CLASS_A or RADIOLIB_LORAWAN_CLASS_C) */
    int16_t setClass(uint8_t cls);

    /*!
      \brief Start a Multicast session.
      \param cls The LoRaWAN Class used for this session (only C is supported).
      \param mcAddr The Multicast address.
      \param mcAppSKey The Multicast payload encryption key.
      \param mcNwkSKey The Multicast payload integrity key.
      \param mcFCntMin The minimum expected Multicast frame counter.
      \param mcFCntMin The maximum allowed Multicast frame counter.
      \param mcFreq The frequency used for the Multicast downlinks (in Hz). Default = 0 uses Rx2 frequency.
      \param mcDr The datarate used for the Multicast downlinks. Default = 0 uses Rx2 datarate.
      \returns \ref status_codes
    */
    int16_t startMulticastSession(uint8_t cls, uint32_t mcAddr, const uint8_t* mcAppSKey, const uint8_t* mcNwkSKey, uint32_t mcFCntMin = 0, uint32_t mcFCntMax = 0xFFFFFFFF, uint32_t mcFreq = 0, uint8_t mcDr = RADIOLIB_LORAWAN_DATA_RATE_UNUSED);

    /*! \brief Stop an ongoing multicast session */
    void stopMulticastSession();

    #if defined(RADIOLIB_BUILD_ARDUINO)
    /*!
      \brief Send a message to the server and wait for a downlink during Rx1 and/or Rx2 window.
      \param strUp Address of Arduino String that will be transmitted.
      \param fPort Port number to send the message to.
      \param strDown Address of Arduino String to save the received data.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param eventUp Pointer to a structure to store extra information about the uplink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \param eventDown Pointer to a structure to store extra information about the downlink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns Window number > 0 if downlink was received, 0 is no downlink was received, otherwise \ref status_codes
    */
    virtual int16_t sendReceive(const String& strUp, uint8_t fPort, String& strDown, bool isConfirmed = false, LoRaWANEvent_t* eventUp = NULL, LoRaWANEvent_t* eventDown = NULL);
    #endif

    /*!
      \brief Send a message to the server and wait for a downlink during Rx1 and/or Rx2 window.
      \param strUp C-string that will be transmitted.
      \param fPort Port number to send the message to.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param eventUp Pointer to a structure to store extra information about the uplink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \param eventDown Pointer to a structure to store extra information about the downlink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns Window number > 0 if downlink was received, 0 is no downlink was received, otherwise \ref status_codes
    */
    virtual int16_t sendReceive(const char* strUp, uint8_t fPort, bool isConfirmed = false, LoRaWANEvent_t* eventUp = NULL, LoRaWANEvent_t* eventDown = NULL);

    /*!
      \brief Send a message to the server and wait for a downlink during Rx1 and/or Rx2 window.
      \param strUp C-string that will be transmitted.
      \param fPort Port number to send the message to.
      \param dataDown Buffer to save received data into.
      \param lenDown Pointer to variable that will be used to save the number of received bytes.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param eventUp Pointer to a structure to store extra information about the uplink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \param eventDown Pointer to a structure to store extra information about the downlink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns Window number > 0 if downlink was received, 0 is no downlink was received, otherwise \ref status_codes
    */
    virtual int16_t sendReceive(const char* strUp, uint8_t fPort, uint8_t* dataDown, size_t* lenDown, bool isConfirmed = false, LoRaWANEvent_t* eventUp = NULL, LoRaWANEvent_t* eventDown = NULL);

    /*!
      \brief Send a message to the server and wait for a downlink but don't bother the user with downlink contents
      \param dataUp Data to send.
      \param lenUp Length of the data.
      \param fPort Port number to send the message to.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param eventUp Pointer to a structure to store extra information about the uplink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \param eventDown Pointer to a structure to store extra information about the downlink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns Window number > 0 if downlink was received, 0 is no downlink was received, otherwise \ref status_codes
    */
    virtual int16_t sendReceive(const uint8_t* dataUp, size_t lenUp, uint8_t fPort = 1, bool isConfirmed = false, LoRaWANEvent_t* eventUp = NULL, LoRaWANEvent_t* eventDown = NULL);

    /*!
      \brief Send a message to the server and wait for a downlink during Rx1 and/or Rx2 window.
      \param dataUp Data to send.
      \param lenUp Length of the data.
      \param fPort Port number to send the message to.
      \param dataDown Buffer to save received data into.
      \param lenDown Pointer to variable that will be used to save the number of received bytes.
      \param isConfirmed Whether to send a confirmed uplink or not.
      \param eventUp Pointer to a structure to store extra information about the uplink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \param eventDown Pointer to a structure to store extra information about the downlink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns Window number > 0 if downlink was received, 0 is no downlink was received, otherwise \ref status_codes
    */
    virtual int16_t sendReceive(const uint8_t* dataUp, size_t lenUp, uint8_t fPort, uint8_t* dataDown, size_t* lenDown, bool isConfirmed = false, LoRaWANEvent_t* eventUp = NULL, LoRaWANEvent_t* eventDown = NULL);

    /*!
      \brief Check if there is an RxC downlink and parse it if available.
      \param dataDown Buffer to save received data into.
      \param lenDown Pointer to variable that will be used to save the number of received bytes.
      \param eventDown Pointer to a structure to store extra information about the downlink event
      (fPort, frame counter, etc.). If set to NULL, no extra information will be passed to the user.
      \returns Window number > 0 if downlink was received, 0 is no downlink was received, otherwise \ref status_codes
    */
    int16_t getDownlinkClassC(uint8_t* dataDown, size_t* lenDown, LoRaWANEvent_t* eventDown = NULL);

    /*!
      \brief Add a MAC command to the uplink queue.
      Only LinkCheck and DeviceTime are available to the user. 
      Other commands are ignored; duplicate MAC commands are discarded.
      \param cid ID of the MAC command
      \returns \ref status_codes
    */
    int16_t sendMacCommandReq(uint8_t cid);

    /*!
      \brief Returns the quality of connectivity after requesting a LinkCheck MAC command.
      \param margin Link margin in dB of LinkCheckReq demodulation at gateway side.
      \param gwCnt Number of gateways that received the LinkCheckReq.
      \returns \ref status_codes
    */
    int16_t getMacLinkCheckAns(uint8_t* margin, uint8_t* gwCnt);

    /*!
      \brief Returns the network time after requesting a DeviceTime MAC command.
      Note: the network returns the time at the end of the uplink transmission.
      The return value of this function automatically adjusts to the current time.
      This time is supposed to be <100ms accurate, but may be accurate to 1 second.
      \param timestamp Number of seconds since GPS epoch (Jan. 6th 1980).
      \param milliseconds Milliseconds on top of the timestamp.
      \param returnUnix If true, returns Unix timestamp instead of GPS (default true).
      \returns \ref status_codes
    */
    int16_t getMacDeviceTimeAns(uint32_t* timestamp, uint16_t* milliseconds, bool returnUnix = true);

    /*!
      \brief Set uplink datarate. This should not be used when ADR is enabled.
      \param drUp Datarate to use for uplinks.
      \returns \ref status_codes
    */
    int16_t setDatarate(uint8_t drUp);

    /*!
      \brief Configure TX power of the radio module.
      \param txPower Output power during TX mode to be set in dBm.
      \returns \ref status_codes
    */
    int16_t setTxPower(int8_t txPower);

    /*! 
      \brief Configure the Rx2 datarate for ABP mode.
      This should not be needed for LoRaWAN 1.1 as it is configured through the first downlink.
      \param dr The datarate to be used for listening for downlinks in Rx2.
      \returns \ref status_codes
    */
    int16_t setRx2Dr(uint8_t dr);

    /*!
      \brief Toggle ADR to on or off.
      \param enable Whether to disable ADR or not.
    */
    void setADR(bool enable = true);

    /*!
      \brief Toggle adherence to dutyCycle limits to on or off.
      \param enable Whether to adhere to dutyCycle limits or not (default true).
      \param msPerHour The maximum allowed Time-on-Air per hour in milliseconds 
      (default 0 = maximum allowed for configured band).
    */
    void setDutyCycle(bool enable = true, RadioLibTime_t msPerHour = 0);

    /*!
      \brief Set or disable uplink dwell time limitation; enabled by default if mandatory.
      \param enable Whether to adhere to dwellTime limits or not (default true).
      \param msPerUplink The maximum allowed Time-on-Air per uplink in milliseconds 
      (default 0 = band default value); make sure you follow regulations/law!
    */
    void setDwellTime(bool enable, RadioLibTime_t msPerUplink = 0);

    /*!
      \brief Configures CSMA for LoRaWAN as per TR013, LoRa Alliance.
      \param csmaEnabled Enable/disable CSMA for LoRaWAN.
      \param maxChanges Maximum number of channel hops if channel is used (default 4).
      \param backoffMax Num of BO slots to be decremented after DIFS phase. 0 to disable BO (default).
      \param difsSlots Num of CADs to estimate a clear CH (default 2).
    */
    void setCSMA(bool csmaEnabled, uint8_t maxChanges = 4, uint8_t backoffMax = 0, uint8_t difsSlots = 2);

    /*!
      \brief Set device status.
      \param battLevel Battery level to set. 0 for external power source, 1 for lowest battery,
      254 for highest battery, 255 for unable to measure.
    */
    void setDeviceStatus(uint8_t battLevel);

    /*!
      \brief Set the exact time a transmission should occur. Note: this is the internal clock time.
      On Arduino platforms, this is the usual time supplied by millis().
      If the supplied time is larger than the current time, sendReceive() or uplink() will delay
      until the scheduled time.
      \param tUplink Transmission time in milliseconds, based on internal clock.
    */
    void scheduleTransmission(RadioLibTime_t tUplink);

    /*! 
      \brief Get the LoRaWAN band used by this node.
      \returns Pointer to the used band.
    */
    const LoRaWANBand_t* getBand();

    /*! 
      \brief Get the LoRaWAN class of this node.
      \returns Class type (RADIOLIB_LORAWAN_CLASS_*).
    */
    uint8_t getClass();

    /*! 
      \brief Get the LoRaWAN version of this node.
      \returns Version major number (0 for LoRaWAN 1.0.x, 1 for LoRaWAN 1.1).
    */
    uint8_t getVersionMajor();

    /*! 
        \brief Returns the last uplink's frame counter; 
        also 0 if no uplink occured yet. 
    */
    uint32_t getFCntUp();

    /*! 
        \brief Returns the last network downlink's frame counter; 
        also 0 if no network downlink occured yet. 
    */
    uint32_t getNFCntDown();

    /*! 
        \brief Returns the last application downlink's frame counter; 
        also 0 if no application downlink occured yet. 
    */
    uint32_t getAFCntDown();

    /*!
      \brief Returns the DevAddr of the device, regardless of OTAA or ABP mode
      \returns 4-byte DevAddr
    */
    uint32_t getDevAddr();

    /*!
      \brief Get the Time-on-air of the last uplink message (in milliseconds).
      \returns (RadioLibTime_t) time-on-air (ToA) of last uplink message (in milliseconds).
    */
    RadioLibTime_t getLastToA();

    /*!
      \brief Calculate the minimum interval to adhere to a certain dutyCycle.
      This interval is based on the ToA of one uplink and does not actually keep track of total airtime.
      \param msPerHour The maximum allowed duty cycle (in milliseconds per hour).
      \param airtime The airtime of the uplink.
      \returns Required interval (delay) in milliseconds between consecutive uplinks.
    */
    RadioLibTime_t dutyCycleInterval(RadioLibTime_t msPerHour, RadioLibTime_t airtime);

    /*! \brief Returns time in milliseconds until next uplink is available under dutyCycle limits */
    RadioLibTime_t timeUntilUplink();

    /*! 
      \brief Returns the maximum allowed uplink payload size given the current MAC state.
      Most importantly, this includes dwell time limitations and ADR.
    */
    uint8_t getMaxPayloadLen();

    /*! \brief Callback to a user-provided sleep function. */
    typedef void (*SleepCb_t)(RadioLibTime_t ms);

    /*! 
      \brief Set custom delay/sleep function callback. If set, LoRaWAN node will call
      this function to wait for periods of time longer than RADIOLIB_LORAWAN_DELAY_SLEEP_THRESHOLD.
      This can be used to lower the power consumption by putting the host microcontroller to sleep.
      NOTE: Since this method will call a user-provided function, it is up to the user to ensure
      that the time duration spent in that sleep function is accurate to at least 1 ms!
    */
    void setSleepFunction(SleepCb_t cb); 

    /*!
      \brief Add a LoRaWAN Application Package as defined in one of the TSxxx documents.
      Any downlinks that occur on the corresponding FPort will be redirected to 
      a supplied callback that implements this package. These downlink contents will be
      hidden from the user as the downlink buffer will be empty and the length zero.
      The package may need to overrule the behaviour of your device - refer to the examples.
      Advanced users only!
      \param packageId The ID of the package (one of RADIOLIB_LORAWAN_PACKAGE_TSxxx).
      \param callback The downlink handler for this package of type (uint8_t* dataDown, size_t lenDown).
      \returns \ref status_codes
    */
    int16_t addAppPackage(uint8_t packageId, PackageCb_t callback);

    /*!
      \brief Add a LoRaWAN Application Package as defined in one of the TSxxx documents.
      Any downlinks that occur on the corresponding FPort will be redirected to 
      a supplied callback that implements this package. These downlink contents will be
      hidden from the user as the downlink buffer will be empty and the length zero.
      The package may need to overrule the behaviour of your device - refer to the examples.
      Advanced users only!
      \param packageId The ID of the package (one of RADIOLIB_LORAWAN_PACKAGE_TSxxx).
      \param callback The downlink handler for this package of type (uint8_t* dataDown, size_t lenDown).
      \param fPort A custom FPort for packages that have a default FPort < 224.
      \returns \ref status_codes
    */
    int16_t addAppPackage(uint8_t packageId, PackageCb_t callback, uint8_t fPort);

    /*!
      \brief Disable a package that was previously added.
    */
    void removePackage(uint8_t packageId);

    /*!
      \brief Rx window padding in milliseconds
      according to the spec, the Rx window must be at least enough time to effectively detect a preamble
      but we pad it a bit on both sides (start and end) to make sure it is wide enough
      The larger this number the more power will be consumed! So be careful about changing it.
      For debugging purposes 50 is a reasonable start, but for production devices it should
      be as low as possible.
      0 is a valid time.

      500 is the **maximum** value, but it is not a good idea to go anywhere near that.
      If you have to go above 50 you probably have a bug somewhere. Check your device timing.
    */
    RadioLibTime_t scanGuard = 10;

#if !RADIOLIB_GODMODE
  protected:
#endif
    PhysicalLayer* phyLayer = NULL;
    const LoRaWANBand_t* band = NULL;

    // a buffer that holds all LW base parameters that should persist at all times!
    uint8_t bufferNonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE] = { 0 };

    // a buffer that holds all LW session parameters that preferably persist, but can be afforded to get lost
    uint8_t bufferSession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE] = { 0 };

    uint8_t fOptsUp[RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN] = { 0 };
    uint8_t fOptsDown[RADIOLIB_LORAWAN_FHDR_FOPTS_MAX_LEN] = { 0 };
    uint8_t fOptsUpLen = 0;
    uint8_t fOptsDownLen = 0;

    uint16_t lwMode = RADIOLIB_LORAWAN_MODE_NONE;
    uint8_t lwClass = RADIOLIB_LORAWAN_CLASS_A;
    uint8_t sessionStatus = RADIOLIB_LORAWAN_SESSION_NONE;

    uint64_t joinEUI = 0;
    uint64_t devEUI = 0;
    uint8_t nwkKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t appKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };

    // the following is either provided by the network server (OTAA)
    // or directly entered by the user (ABP)
    uint32_t devAddr = 0;
    uint8_t appSKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t fNwkSIntKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t sNwkSIntKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t nwkSEncKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t jSIntKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };

    uint16_t keyCheckSum = 0;
    
    // device-specific parameters, persistent through sessions
    uint16_t devNonce = 0;
    uint32_t joinNonce = 0;

    // session-specific parameters
    uint32_t homeNetId = 0;
    uint8_t adrLimitExp = RADIOLIB_LORAWAN_ADR_ACK_LIMIT_EXP;
    uint8_t adrDelayExp = RADIOLIB_LORAWAN_ADR_ACK_DELAY_EXP;
    uint8_t nbTrans = 1;            // Number of allowed frame retransmissions
    uint8_t txPowerSteps = 0;
    uint8_t txPowerMax = 0;
    uint32_t fCntUp = 0;
    uint32_t aFCntDown = 0;
    uint32_t nFCntDown = 0;
    uint32_t confFCntUp = RADIOLIB_LORAWAN_FCNT_NONE;
    uint32_t confFCntDown = RADIOLIB_LORAWAN_FCNT_NONE;
    uint32_t adrFCnt = 0;

    // ADR is enabled by default
    bool adrEnabled = true;

    // duty cycle is set upon initialization and activated in regions that impose this
    bool dutyCycleEnabled = false;
    uint32_t dutyCycle = 0;

    // dwell time is set upon initialization and activated in regions that impose this
    uint16_t dwellTimeUp = 0;
    uint16_t dwellTimeDn = 0;

    RadioLibTime_t tUplink = 0;   // scheduled uplink transmission time (internal clock)
    RadioLibTime_t tDownlink = 0; // time at end of downlink reception

    // multicast parameters
    uint8_t multicast = false;
    uint32_t mcAddr = 0;
    uint8_t mcAppSKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint8_t mcNwkSKey[RADIOLIB_AES128_KEY_SIZE] = { 0 };
    uint32_t mcAFCnt = 0;
    uint32_t mcAFCntMax = 0;

    // enabled TS packages
    LoRaWANPackage_t packages[RADIOLIB_LORAWAN_NUM_SUPPORTED_PACKAGES];

    // enable/disable CSMA for LoRaWAN
    bool csmaEnabled = false;

    // maximum number of channel hops during CSMA
    uint8_t maxChanges = RADIOLIB_LORAWAN_MAX_CHANGES_DEFAULT;

    // number of backoff slots to be checked after DIFS phase.
    // A random BO avoids collisions in the case where two or more nodes start the CSMA
    // process at the same time. 
    uint8_t backoffMax = RADIOLIB_LORAWAN_BACKOFF_MAX_DEFAULT;
    
    // number of CADs to estimate a clear channel
    uint8_t difsSlots = RADIOLIB_LORAWAN_DIFS_DEFAULT;

    // dynamic channels
    LoRaWANChannel_t dynamicChannels[2][RADIOLIB_LORAWAN_MAX_NUM_DYNAMIC_CHANNELS];

    // masks: which channels are defined/active; flags: which should be used next
    uint16_t channelMasks[RADIOLIB_LORAWAN_MAX_NUM_FIXED_CHANNELS / 16] = { 0 };
    uint16_t channelFlags[RADIOLIB_LORAWAN_MAX_NUM_FIXED_CHANNELS / 16] = { 0 };

    // currently configured channels for Tx, Rx1, Rx2, RxBC
    LoRaWANChannel_t channels[4] = { RADIOLIB_LORAWAN_CHANNEL_NONE, RADIOLIB_LORAWAN_CHANNEL_NONE,
                                     RADIOLIB_LORAWAN_CHANNEL_NONE, RADIOLIB_LORAWAN_CHANNEL_NONE };

    // delays between the uplink and Rx windows
    // the first field is meaningless, but is used for offsetting the Rx windows
    RadioLibTime_t rxDelays[4] = { 0, 
                                   RADIOLIB_LORAWAN_RECEIVE_DELAY_1_MS, 
                                   RADIOLIB_LORAWAN_RECEIVE_DELAY_2_MS,
                                   0 };

    // offset between Tx and Rx1 (such that Rx1 has equal or lower DR)
    uint8_t rx1DrOffset = 0;

    // LoRaWAN revision (1.0 vs 1.1)
    uint8_t rev = 0;

    // Time on Air of last uplink
    RadioLibTime_t lastToA = 0;

    // timestamp to measure the Rx1/2 delay (from uplink end)
    RadioLibTime_t tUplinkEnd = 0;

    // duration of SPI transaction for phyLayer->launchMode()
    RadioLibTime_t launchDuration = 0;

    // device status - battery level
    uint8_t battLevel = 0xFF;

    // indicates whether an uplink has MAC commands as payload
    bool isMACPayload = false;

    // save the selected sub-band in case this must be restored in ADR control
    uint8_t subBand = 0;

    // user-provided sleep callback
    SleepCb_t sleepCb = nullptr;

    // this will reset the device credentials, so the device starts completely new
    void clearNonces();

    // setup an empty session with default parameters
    void createSession();

    // setup Join-Request payload
    void composeJoinRequest(uint8_t* joinRequestMsg);

    // extract Join-Accept payload and start a new session
    int16_t processJoinAccept(LoRaWANJoinEvent_t *joinEvent);

    // a join-accept can piggy-back a set of channels or channel masks
    void processCFList(const uint8_t* cfList);

    // check whether payload length and fport are allowed
    int16_t isValidUplink(size_t len, uint8_t fPort);
    
    // perform ADR backoff
    void adrBackoff();

    // create an encrypted uplink buffer, composing metadata, user data and MAC data
    void composeUplink(const uint8_t* in, uint8_t lenIn, uint8_t* out, uint8_t fPort, bool isConfirmed);

    // generate and set the MIC of an uplink buffer (depends on selected channels)
    void micUplink(uint8_t* inOut, size_t lenInOut);

    // transmit uplink buffer on a specified channel
    int16_t transmitUplink(const LoRaWANChannel_t* chnl, uint8_t* in, uint8_t len);

    // handle one of the Class A receive windows with a given channel and certain timestamps
    int16_t receiveClassA(uint8_t dir, const LoRaWANChannel_t* dlChannel, uint8_t window, const RadioLibTime_t dlDelay, RadioLibTime_t tReference);

    // handle a Class C receive window with timeout (between Class A windows) or without (between uplinks)
    int16_t receiveClassC(RadioLibTime_t timeout = 0);

    // open a series of Class A (and C) downlinks
    virtual int16_t receiveDownlink();

    // extract downlink payload and process MAC commands
    int16_t parseDownlink(uint8_t* data, size_t* len, uint8_t window, LoRaWANEvent_t* event = NULL);

    // add a LoRaWAN package that runs through the network layer 
    // (not available to users, they are only allowed to add application packages)
    int16_t addNwkPackage(uint8_t packageId);

    // execute mac command, return the number of processed bytes for sequential processing
    bool execMacCommand(uint8_t cid, uint8_t* optIn, uint8_t lenIn);
    bool execMacCommand(uint8_t cid, uint8_t* optIn, uint8_t lenIn, uint8_t* optOut);

    // possible override for additional MAC commands that are not in the base specification
    virtual bool derivedMacHandler(uint8_t cid, uint8_t* optIn, uint8_t lenIn, uint8_t* optOut);

    // pre-process a (set of) LinkAdrReq commands into one super-channel-mask + Tx/Dr/NbTrans fields
    void preprocessMacLinkAdr(uint8_t* mPtr, uint8_t cLen, uint8_t* mAdrOpt);

    // post-process a (set of) LinkAdrAns commands depending on LoRaWAN version
    void postprocessMacLinkAdr(uint8_t* ack, uint8_t cLen);

    // get the properties of a MAC command given a certain command ID
    int16_t getMacCommand(uint8_t cid, LoRaWANMacCommand_t* cmd);

    // possible override for additional MAC commands that are not in the base specification
    virtual int16_t derivedMacFinder(uint8_t cid, LoRaWANMacCommand_t* cmd);

    // get the length of a certain MAC command in a specific direction (up/down)
    // if inclusive is true, add one for the CID byte
    // include payload in case the MAC command has a dynamic length
    virtual int16_t getMacLen(uint8_t cid, uint8_t* len, uint8_t dir, bool inclusive = false, uint8_t* payload = NULL);

    // find out of a MAC command should persist destruction
    // in uplink direction, some commands must persist if no downlink is received
    // in downlink direction, the user-accessible MAC commands remain available for retrieval
    bool isPersistentMacCommand(uint8_t cid, uint8_t dir);

    // push MAC command to queue, done by copy
    int16_t pushMacCommand(uint8_t cid, const uint8_t* cOcts, uint8_t* out, uint8_t* lenOut, uint8_t dir);

    // retrieve the payload of a certain MAC command, if present in the buffer
    int16_t getMacPayload(uint8_t cid, const uint8_t* in, uint8_t lenIn, uint8_t* out, uint8_t dir);

    // delete a specific MAC command from queue, indicated by the command ID
    int16_t deleteMacCommand(uint8_t cid, uint8_t* inOut, uint8_t* lenInOut, uint8_t dir);

    // clear a MAC buffer, possible retaining persistent MAC commands
    void clearMacCommands(uint8_t* inOut, uint8_t* lenInOut, uint8_t dir);

    // configure the common physical layer properties (frequency, sync word etc.)
    int16_t setPhyProperties(const LoRaWANChannel_t* chnl, uint8_t dir, int8_t pwr, size_t pre = 0);

    // Performs CSMA as per LoRa Alliance Technical Recommendation 13 (TR-013).
    bool csmaChannelClear(uint8_t difs, uint8_t numBackoff);

    // perform a single CAD operation for the under SF/CH combination. Returns either busy or otherwise.
    bool cadChannelClear();

    // enable all default channels on top of the current channels
    void enableDefaultChannels(bool addDynamic = false);

    // calculate which channels are available given the current datarate
    // returns true if there is any such channel, false otherwise
    bool calculateChannelFlags();

    // select a set of random TX/RX channels for up- and downlink
    int16_t selectChannels();

    // method to generate message integrity code
    uint32_t generateMIC(const uint8_t* msg, size_t len, uint8_t* key);

    // method to verify message integrity code
    // it assumes that the MIC is the last 4 bytes of the message
    bool verifyMIC(uint8_t* msg, size_t len, uint8_t* key);

    // function to encrypt and decrypt payloads (regular uplink/downlink)
    void processAES(const uint8_t* in, size_t len, uint8_t* key, uint8_t* out, uint32_t addr, uint32_t fCnt, uint8_t dir, uint8_t ctrId, bool counter);

    // function that allows sleeping via user-provided callback
    void sleepDelay(RadioLibTime_t ms, bool radioOff = true);

    // 16-bit checksum method that takes a uint8_t array of even length and calculates the checksum
    static uint16_t checkSum16(const uint8_t *key, uint16_t keyLen);

    // check the integrity of a buffer using a 16-bit checksum located in the last two bytes of the buffer
    static int16_t checkBufferCommon(const uint8_t *buffer, uint16_t size);

    // network-to-host conversion method - takes data from network packet and converts it to the host endians
    template<typename T>
    static T ntoh(const uint8_t* buff, size_t size = 0);

    // host-to-network conversion method - takes data from host variable and and converts it to network packet endians
    template<typename T>
    static void hton(uint8_t* buff, T val, size_t size = 0);
};

template<typename T>
T LoRaWANNode::ntoh(const uint8_t* buff, size_t size) {
  const uint8_t* buffPtr = buff;
  size_t targetSize = sizeof(T);
  if(size != 0) {
    targetSize = size;
  }
  T res = 0;
  for(size_t i = 0; i < targetSize; i++) {
    res |= (uint32_t)(*(buffPtr++)) << 8*i;
  }
  return(res);
}

template<typename T>
void LoRaWANNode::hton(uint8_t* buff, T val, size_t size) {
  uint8_t* buffPtr = buff;
  size_t targetSize = sizeof(T);
  if(size != 0) {
    targetSize = size;
  }
  for(size_t i = 0; i < targetSize; i++) {
    *(buffPtr++) = val >> 8*i;
  }
}

#endif
