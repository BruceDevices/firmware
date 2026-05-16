/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "nimble/porting/nimble/include/os/os.h"
#include "nimble/porting/nimble/include/mem/mem.h"
#include "ble_hs_priv.h"

#include "nimble/nimble/transport/include/nimble/transport.h"
// #include "bt_common.h"
// #if (BT_HCI_LOG_INCLUDED == TRUE)
// #include "hci_log/bt_hci_log.h"
// #endif // (BT_HCI_LOG_INCLUDED == TRUE)

#define BLE_HCI_CMD_TIMEOUT_MS  2000

/* HCI ERROR */
#define BLE_ERR_UNKNOWN_HCI_CMD      0x01
#define BLE_ERR_UNK_CONN_ID          0x02
#define BLE_ERR_HW_FAIL              0x03
#define BLE_ERR_PAGE_TMO             0x04
#define BLE_ERR_AUTH_FAIL            0x05
#define BLE_ERR_PINKEY_MISSING       0x06
#define BLE_ERR_MEM_CAPACITY         0x07
#define BLE_ERR_CONN_SPVN_TMO        0x08
#define BLE_ERR_CONN_LIMIT           0x09
#define BLE_ERR_SYNCH_CONN_LIMIT     0x0a
#define BLE_ERR_ACL_CONN_EXISTS      0x0b
#define BLE_ERR_CMD_DISALLOWED       0x0c
#define BLE_ERR_CONN_REJ_RESOURCES   0x0d
#define BLE_ERR_CONN_REJ_SECURITY    0x0e
#define BLE_ERR_CONN_REJ_BD_ADDR     0x0f
#define BLE_ERR_CONN_ACCEPT_TMO      0x10
#define BLE_ERR_UNSUPPORTED          0x11
#define BLE_ERR_INV_HCI_CMD_PARMS    0x12
#define BLE_ERR_REM_USER_CONN_TERM   0x13
#define BLE_ERR_RD_CONN_TERM_RESRCS  0x14
#define BLE_ERR_RD_CONN_TERM_PWROFF  0x15
#define BLE_ERR_CONN_TERM_LOCAL      0x16
#define BLE_ERR_REPEATED_ATTEMPTS    0x17
#define BLE_ERR_NO_PAIRING           0x18
#define BLE_ERR_UNK_LMP              0x19
#define BLE_ERR_UNSUPP_REM_FEATURE   0x1a
#define BLE_ERR_SCO_OFFSET           0x1b
#define BLE_ERR_SCO_ITVL             0x1c
#define BLE_ERR_SCO_AIR_MODE         0x1d
#define BLE_ERR_INV_LMP_LL_PARM      0x1e
#define BLE_ERR_UNSPECIFIED          0x1f
#define BLE_ERR_UNSUPP_LMP_LL_PARM   0x20
#define BLE_ERR_NO_ROLE_CHANGE       0x21
#define BLE_ERR_LMP_LL_RSP_TMO       0x22
#define BLE_ERR_LMP_COLLISION        0x23
#define BLE_ERR_LMP_PDU              0x24
#define BLE_ERR_ENCRYPTION_MODE      0x25
#define BLE_ERR_LINK_KEY_CHANGE      0x26
#define BLE_ERR_UNSUPP_QOS           0x27
#define BLE_ERR_INSTANT_PASSED       0x28
#define BLE_ERR_UNIT_KEY_PAIRING     0x29
#define BLE_ERR_DIFF_TRANS_COLL      0x2a
/*#define BLE_ERR_RESERVED             0x2b*/
#define BLE_ERR_QOS_PARM             0x2c
#define BLE_ERR_QOS_REJECTED         0x2d
#define BLE_ERR_CHAN_CLASS           0x2e
#define BLE_ERR_INSUFFICIENT_SEC     0x2f
#define BLE_ERR_PARM_OUT_OF_RANGE    0x30
/*#define BLE_ERR_RESERVED             0x31*/
#define BLE_ERR_PENDING_ROLE_SW      0x32
/*#define BLE_ERR_RESERVED             0x33*/
#define BLE_ERR_RESERVED_SLOT        0x34
#define BLE_ERR_ROLE_SW_FAIL         0x35
#define BLE_ERR_INQ_RSP_TOO_BIG      0x36
#define BLE_ERR_SEC_SIMPLE_PAIR      0x37
#define BLE_ERR_HOST_BUSY_PAIR       0x38
#define BLE_ERR_CONN_REJ_CHANNEL     0x39
#define BLE_ERR_CTLR_BUSY            0x3a
#define BLE_ERR_CONN_PARMS           0x3b
#define BLE_ERR_DIR_ADV_TMO          0x3c
#define BLE_ERR_CONN_TERM_MIC        0x3d
#define BLE_ERR_CONN_ESTABLISHMENT   0x3e
#define BLE_ERR_MAC_CONN_FAIL        0x3f
#define BLE_ERR_COARSE_CLK_ADJ       0x40
#define BLE_ERR_TYPE0_SUBMAP_NDEF    0x41
#define BLE_ERR_UNK_ADV_INDENT       0x42
#define BLE_ERR_LIMIT_REACHED        0x43
#define BLE_ERR_OPERATION_CANCELLED  0x44
#define BLE_ERR_PACKET_TOO_LONG      0x45
#define BLE_ERR_MAX                  0xff

struct err_code {
    int error_code;
    const char *msg;
};

static struct err_code core_err_code_list[] = {
      { BLE_HS_EAGAIN,                                ": BLE_HS_EAGAIN (Temporary failure; try again)" },
      { BLE_HS_EALREADY,                              ": BLE_HS_EALREADY (Operation already in progress or completed)" },
      { BLE_HS_EINVAL,                                ": BLE_HS_EINVAL (One or more arguments are invalid)" },
      { BLE_HS_EMSGSIZE,                              ": BLE_HS_EMSGSIZE (The provided buffer is too small)" },
      { BLE_HS_ENOENT,                                ": BLE_HS_ENOENT (No entry matching the specified criteria)" },
      { BLE_HS_ENOMEM,                                ": BLE_HS_ENOMEM (Operation failed due to resource exhaustion)" },
      { BLE_HS_ENOTCONN,                              ": BLE_HS_ENOTCONN (No open connection with the specified handle)" },
      { BLE_HS_ENOTSUP,                               ": BLE_HS_ENOTSUP (Operation disabled at compile time)" },
      { BLE_HS_EAPP,                                  ": BLE_HS_EAPP (Application callback behaved unexpectedly)" },
      { BLE_HS_EBADDATA,                              ": BLE_HS_EBADDATA (Command from peer is invalid)" },
      { BLE_HS_EOS,                                   ": BLE_HS_EOS (Mynewt OS error)" },
      { BLE_HS_ECONTROLLER,                           ": BLE_HS_ECONTROLLER (Event from controller is invalid)" },
      { BLE_HS_ETIMEOUT,                              ": BLE_HS_ETIMEOUT (Operation timed out)" },
      { BLE_HS_EDONE,                                 ": BLE_HS_EDONE (Operation completed successfully)" },
      { BLE_HS_EBUSY,                                 ": BLE_HS_EBUSY (Operation cannot be performed until procedure completes)" },
      { BLE_HS_EREJECT,                               ": BLE_HS_EREJECT (Peer rejected a connection parameter update request)" },
      { BLE_HS_EUNKNOWN,                              ": BLE_HS_EUNKNOWN (Unexpected failure; catch all)" },
      { BLE_HS_EROLE,                                 ": BLE_HS_EROLE (Operation requires different role (e.g., central vs. peripheral))" },
      { BLE_HS_ETIMEOUT_HCI,                          ": BLE_HS_ETIMEOUT_HCI (HCI request timed out; controller unresponsive)" },
      { BLE_HS_ENOMEM_EVT,                            ": BLE_HS_ENOMEM_EVT (Controller failed to send event due to memory exhaustion)" },
      { BLE_HS_ENOADDR,                               ": BLE_HS_ENOADDR (Operation requires an identity address but none configured)" },
      { BLE_HS_ENOTSYNCED,                            ": BLE_HS_ENOTSYNCED (Attempt to use the host before it is synced with controller)" },
      { BLE_HS_EAUTHEN,                               ": BLE_HS_EAUTHEN (Insufficient authentication)" },
      { BLE_HS_EAUTHOR,                               ": BLE_HS_EAUTHOR (Insufficient authorization)" },
      { BLE_HS_EENCRYPT,                              ": BLE_HS_EENCRYPT (Insufficient encryption level)" },
      { BLE_HS_EENCRYPT_KEY_SZ,                       ": BLE_HS_EENCRYPT_KEY_SZ (Insufficient key size" },
      { BLE_HS_ESTORE_CAP,                            ": BLE_HS_ESTORE_CAP (BLE_HS_ESTORE_FAIL,)" },
      { BLE_HS_ESTORE_FAIL,                           ": BLE_HS_ESTORE_FAIL (Storage IO error)" },
      { BLE_HS_EPREEMPTED,                            ": BLE_HS_EPREEMPTED (ation was preempted)" },
      { BLE_HS_EDISABLED,                             ": BLE_HS_EDISABLED (Operation disabled)" },
      { BLE_HS_ESTALLED,                              ": BLE_HS_ESTALLED (Operation stalled)" }
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
static struct err_code err_code_list[] = {
      { BLE_HS_HCI_ERR(BLE_ERR_UNKNOWN_HCI_CMD),      ": BLE_ERR_UNKNOWN_HCI_CMD (Unknown HCI Command)" },
      { BLE_HS_HCI_ERR(BLE_ERR_UNK_CONN_ID),          ": BLE_ERR_UNK_CONN_ID (Unknown Connection Identifier)" },
      { BLE_HS_HCI_ERR(BLE_ERR_HW_FAIL),              ": BLE_ERR_HW_FAIL (Hardware Failure)" },
      { BLE_HS_HCI_ERR(BLE_ERR_PAGE_TMO),             ": BLE_ERR_PAGE_TMO (Page Timeout)" },
      { BLE_HS_HCI_ERR(BLE_ERR_AUTH_FAIL),            ": BLE_ERR_AUTH_FAIL (Authentication Failure)" },
      { BLE_HS_HCI_ERR(BLE_ERR_PINKEY_MISSING),       ": BLE_ERR_PINKEY_MISSING (PIN or Key Missing)" },
      { BLE_HS_HCI_ERR(BLE_ERR_MEM_CAPACITY),         ": BLE_ERR_MEM_CAPACITY (Memory Capacity Exceeded)" },
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_SPVN_TMO),        ": BLE_ERR_CONN_SPVN_TMO (Connection Timeout)" },
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_LIMIT),           ": BLE_ERR_CONN_LIMIT (Connection Limit Exceeded)" },
      { BLE_HS_HCI_ERR(BLE_ERR_SYNCH_CONN_LIMIT),     ": BLE_ERR_SYNCH_CONN_LIMIT (Synchronous Connection Limit To A Device Exceeded)" },
      { BLE_HS_HCI_ERR(BLE_ERR_ACL_CONN_EXISTS),      ": BLE_ERR_ACL_CONN_EXISTS (ACL Connection Already Exists)" },
      { BLE_HS_HCI_ERR(BLE_ERR_CMD_DISALLOWED),       ": BLE_ERR_CMD_DISALLOWED (Command Disallowed)" },
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_REJ_RESOURCES),   ": BLE_ERR_CONN_REJ_RESOURCES (Connection Rejected due to Limited Resources)" },
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_REJ_SECURITY),    ": BLE_ERR_CONN_REJ_SECURITY (Connection Rejected Due To Security Reasons)" },
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_REJ_BD_ADDR),     ": BLE_ERR_CONN_REJ_BD_ADDR (Connection Rejected due to Unacceptable BD_ADDR)" },
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_ACCEPT_TMO),      ": BLE_ERR_CONN_ACCEPT_TMO (Connection Accept Timeout Exceeded)" },
      { BLE_HS_HCI_ERR(BLE_ERR_UNSUPPORTED),          ": BLE_ERR_UNSUPPORTED (Unsupported Feature or Parameter Value)" },
      { BLE_HS_HCI_ERR(BLE_ERR_INV_HCI_CMD_PARMS),    ": BLE_ERR_INV_HCI_CMD_PARMS (Invalid HCI Command Parameters)" },
      { BLE_HS_HCI_ERR(BLE_ERR_REM_USER_CONN_TERM),   ": BLE_ERR_REM_USER_CONN_TERM (Remote User Terminated Connection)" },
      { BLE_HS_HCI_ERR(BLE_ERR_RD_CONN_TERM_RESRCS),  ": BLE_ERR_RD_CONN_TERM_RESRCS (Remote Device Terminated Connection due to Low Resources)" },
      { BLE_HS_HCI_ERR(BLE_ERR_RD_CONN_TERM_PWROFF),  ": BLE_ERR_RD_CONN_TERM_PWROFF (Remote Device Terminated Connection due to Power Off)" },
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_TERM_LOCAL),      ": BLE_ERR_CONN_TERM_LOCAL (Connection Terminated By Local Host)" },
      { BLE_HS_HCI_ERR(BLE_ERR_REPEATED_ATTEMPTS),    ": BLE_ERR_REPEATED_ATTEMPTS (Repeated Attempts)" },
      { BLE_HS_HCI_ERR(BLE_ERR_NO_PAIRING),           ": BLE_ERR_NO_PAIRING (Pairing Not Allowed)" },
      { BLE_HS_HCI_ERR(BLE_ERR_UNK_LMP),              ": BLE_ERR_UNK_LMP (Unknown LMP PDU)" },
      { BLE_HS_HCI_ERR(BLE_ERR_UNSUPP_REM_FEATURE),   ": BLE_ERR_UNSUPP_REM_FEATURE (Unsupported Remote Feature / Unsupported LMP Feature)" },
      { BLE_HS_HCI_ERR(BLE_ERR_SCO_OFFSET),           ": BLE_ERR_SCO_OFFSET (SCO Offset Rejected)" },
      { BLE_HS_HCI_ERR(BLE_ERR_SCO_ITVL),             ": BLE_ERR_SCO_ITVL (SCO Interval Rejected)" },
      { BLE_HS_HCI_ERR(BLE_ERR_SCO_AIR_MODE),         ": BLE_ERR_SCO_AIR_MODE (SCO Air Mode Rejected)" },
      { BLE_HS_HCI_ERR(BLE_ERR_INV_LMP_LL_PARM),      ": BLE_ERR_INV_LMP_LL_PARM (Invalid LMP Parameters / Invalid LL Parameters)" },
      { BLE_HS_HCI_ERR(BLE_ERR_UNSPECIFIED),          ": BLE_ERR_UNSPECIFIED (Unspecified Error)" },
      { BLE_HS_HCI_ERR(BLE_ERR_UNSUPP_LMP_LL_PARM),   ": BLE_ERR_UNSUPP_LMP_LL_PARM (Unsupported LMP Parameter Value / Unsupported LL Parameter Value)" },
      { BLE_HS_HCI_ERR(BLE_ERR_NO_ROLE_CHANGE),       ": BLE_ERR_NO_ROLE_CHANGE (Role Change Not Allowed)" },
      { BLE_HS_HCI_ERR(BLE_ERR_LMP_LL_RSP_TMO),       ": BLE_ERR_LMP_LL_RSP_TMO (LMP Response Timeout / LL Response Timeout)" },
      { BLE_HS_HCI_ERR(BLE_ERR_LMP_COLLISION),        ": BLE_ERR_LMP_COLLISION (LMP Error Transaction Collision)" },
      { BLE_HS_HCI_ERR(BLE_ERR_LMP_PDU),              ": BLE_ERR_LMP_PDU (LMP PDU Not Allowed)" },
      { BLE_HS_HCI_ERR(BLE_ERR_ENCRYPTION_MODE),      ": BLE_ERR_ENCRYPTION_MODE (Encryption Mode Not Acceptable)"},
      { BLE_HS_HCI_ERR(BLE_ERR_LINK_KEY_CHANGE),      ": BLE_ERR_LINK_KEY_CHANGE (Link Key cannot be Changed)"},
      { BLE_HS_HCI_ERR(BLE_ERR_UNSUPP_QOS),           ": BLE_ERR_UNSUPP_QOS (Requested QoS Not Supported)"},
      { BLE_HS_HCI_ERR(BLE_ERR_INSTANT_PASSED),       ": BLE_ERR_INSTANT_PASSED (Instant Passed)"},
      { BLE_HS_HCI_ERR(BLE_ERR_UNIT_KEY_PAIRING),     ": BLE_ERR_UNIT_KEY_PAIRING (Pairing With Unit Key Not Supported)"},
      { BLE_HS_HCI_ERR(BLE_ERR_DIFF_TRANS_COLL),      ": BLE_ERR_DIFF_TRANS_COLL (Different Transaction Collision)"},
      { BLE_HS_HCI_ERR(BLE_ERR_QOS_PARM),             ": BLE_ERR_QOS_PARM (QoS Unacceptable Parameter)"},
      { BLE_HS_HCI_ERR(BLE_ERR_QOS_REJECTED),         ": BLE_ERR_QOS_REJECTED (QoS Rejected)"},
      { BLE_HS_HCI_ERR(BLE_ERR_CHAN_CLASS),           ": BLE_ERR_CHAN_CLASS (Channel Classification Not Supported)"},
      { BLE_HS_HCI_ERR(BLE_ERR_INSUFFICIENT_SEC),     ": BLE_ERR_INSUFFICIENT_SEC (Insufficient Security)"},
      { BLE_HS_HCI_ERR(BLE_ERR_PARM_OUT_OF_RANGE),    ": BLE_ERR_PARM_OUT_OF_RANGE (Parameter Out Of Mandatory Range)"},
      { BLE_HS_HCI_ERR(BLE_ERR_PENDING_ROLE_SW),      ": BLE_ERR_PENDING_ROLE_SW (Role Switch Pending)"},
      { BLE_HS_HCI_ERR(BLE_ERR_RESERVED_SLOT),        ": BLE_ERR_RESERVED_SLOT (Reserved Slot Violation)"},
      { BLE_HS_HCI_ERR(BLE_ERR_ROLE_SW_FAIL),         ": BLE_ERR_ROLE_SW_FAIL (Role Switch Failed)"},
      { BLE_HS_HCI_ERR(BLE_ERR_INQ_RSP_TOO_BIG),      ": BLE_ERR_INQ_RSP_TOO_BIG (Extended Inquiry Response Too Large)"},
      { BLE_HS_HCI_ERR(BLE_ERR_SEC_SIMPLE_PAIR),      ": BLE_ERR_SEC_SIMPLE_PAIR (Secure Simple Pairing Not Supported By Host)"},
      { BLE_HS_HCI_ERR(BLE_ERR_HOST_BUSY_PAIR),       ": BLE_ERR_HOST_BUSY_PAIR (Host Busy - Pairing)"},
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_REJ_CHANNEL),     ": BLE_ERR_CONN_REJ_CHANNEL (Connection Rejected due to No Suitable Channel Found)"},
      { BLE_HS_HCI_ERR(BLE_ERR_CTLR_BUSY),            ": BLE_ERR_CTLR_BUSY (Controller Busy)"},
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_PARMS),           ": BLE_ERR_CONN_PARMS (Unacceptable Connection Parameters)"},
      { BLE_HS_HCI_ERR(BLE_ERR_DIR_ADV_TMO),          ": BLE_ERR_DIR_ADV_TMO (Directed Advertising Timeout)"},
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_TERM_MIC),        ": BLE_ERR_CONN_TERM_MIC (Connection Terminated due to MIC Failure)"},
      { BLE_HS_HCI_ERR(BLE_ERR_CONN_ESTABLISHMENT),   ": BLE_ERR_CONN_ESTABLISHMENT (Connection Failed to be Established)"},
      { BLE_HS_HCI_ERR(BLE_ERR_MAC_CONN_FAIL),        ": BLE_ERR_MAC_CONN_FAIL (MAC Connection Failed)"},
      { BLE_HS_HCI_ERR(BLE_ERR_COARSE_CLK_ADJ),       ": BLE_ERR_COARSE_CLK_ADJ (Coarse Clock Adjustment Rejected but Will Try to Adjust Using Clock Dragging)"},
      { BLE_HS_HCI_ERR(BLE_ERR_TYPE0_SUBMAP_NDEF),    ": BLE_ERR_TYPE0_SUBMAP_NDEF (Type0 Submap Not Defined)"},
      { BLE_HS_HCI_ERR(BLE_ERR_UNK_ADV_INDENT),       ": BLE_ERR_UNK_ADV_INDENT (Unknown Advertising Identifier)"},
      { BLE_HS_HCI_ERR(BLE_ERR_LIMIT_REACHED),        ": BLE_ERR_LIMIT_REACHED (Limit Reached)"},
      { BLE_HS_HCI_ERR(BLE_ERR_OPERATION_CANCELLED),  ": BLE_ERR_OPERATION_CANCELLED (Operation Cancelled by Host)"},
      { BLE_HS_HCI_ERR(BLE_ERR_PACKET_TOO_LONG),      ": BLE_ERR_PACKET_TOO_LONG (Packet Too Long)"},
      { BLE_HS_HCI_ERR(BLE_ERR_MAX),                  ": BLE_ERR_MAX"}
};
#pragma GCC diagnostic pop

static void esp_core_err_to_name(int error_code, uint16_t *opcode)
{
    for (int i = 0; i<sizeof(core_err_code_list) / sizeof(core_err_code_list[0]); i++) {
        if (core_err_code_list[i].error_code == error_code) {
            if (opcode == NULL) {
                  MODLOG_DFLT(INFO, "core_err=0x%02X %s\n", error_code, core_err_code_list[i].msg);
            }
            else {
                  MODLOG_DFLT(INFO, "ogf=0x%02x, ocf=0x%04x, core_err=0x%02X %s\n",
                              BLE_HCI_OGF(*opcode), BLE_HCI_OCF(*opcode), error_code, core_err_code_list[i].msg);
            }
            break;
        }
    }
    return;
}

static void esp_hci_err_to_name(int error_code, uint16_t *opcode)
{
    if (error_code == 0) {
        return;
    }
    else if (error_code <  0x20) {
       esp_core_err_to_name(error_code, opcode);
       return;
    }

    for (int i = 0; i<sizeof(err_code_list) / sizeof(err_code_list[0]); i++) {
        if (err_code_list[i].error_code == error_code) {
            if (opcode == NULL) {
                  MODLOG_DFLT(INFO, "hci_err=0x%02X %s\n", error_code, err_code_list[i].msg);
            }
            else {
                  MODLOG_DFLT(INFO, "ogf=0x%02x, ocf=0x%04x, hci_err=0x%02X %s\n",
                              BLE_HCI_OGF(*opcode), BLE_HCI_OCF(*opcode), error_code, err_code_list[i].msg);
            }
            break;
        }
    }
    return;
}

static struct ble_npl_mutex ble_hs_hci_mutex;
static struct ble_npl_sem ble_hs_hci_sem;

static struct ble_hci_ev *ble_hs_hci_ack;
static uint16_t ble_hs_hci_buf_sz;
static uint8_t ble_hs_hci_max_pkts;

/* For now 32-bits of features is enough */
static uint32_t ble_hs_hci_sup_feat;

static uint8_t ble_hs_hci_version;

static struct ble_hs_hci_sup_cmd ble_hs_hci_sup_cmd;

#if CONFIG_BT_NIMBLE_LEGACY_VHCI_ENABLE
#define BLE_HS_HCI_FRAG_DATABUF_SIZE    \
    (BLE_ACL_MAX_PKT_SIZE +             \
     BLE_HCI_DATA_HDR_SZ +              \
     sizeof (struct os_mbuf_pkthdr) +   \
     sizeof (struct ble_mbuf_hdr) +     \
     sizeof (struct os_mbuf))
#else
#define BLE_HS_HCI_FRAG_DATABUF_SIZE    \
     (BLE_ACL_MAX_PKT_SIZE +            \
      BLE_HCI_DATA_HDR_SZ +             \
      BLE_HS_CTRL_DATA_HDR_SZ +         \
      sizeof (struct os_mbuf_pkthdr) +  \
      sizeof (struct ble_mbuf_hdr) +    \
      sizeof (struct os_mbuf))
#endif

#define BLE_HS_HCI_FRAG_MEMBLOCK_SIZE   \
    (OS_ALIGN(BLE_HS_HCI_FRAG_DATABUF_SIZE, 4))

#define BLE_HS_HCI_FRAG_MEMPOOL_SIZE    \
    OS_MEMPOOL_SIZE(1, BLE_HS_HCI_FRAG_MEMBLOCK_SIZE)

/**
 *  A one-element mbuf pool dedicated to holding outgoing ACL data packets.
 *  This dedicated pool prevents a deadlock caused by mbuf exhaustion.  Without
 *  this pool, all msys mbufs could be permanently allocated, preventing us
 *  from fragmenting outgoing packets and sending them (and ultimately freeing
 *  them).
 */
static os_membuf_t ble_hs_hci_frag_data[BLE_HS_HCI_FRAG_MEMPOOL_SIZE];
static struct os_mbuf_pool ble_hs_hci_frag_mbuf_pool;
static struct os_mempool ble_hs_hci_frag_mempool;

/**
 * The number of available ACL transmit buffers on the controller.  This
 * variable must only be accessed while the host mutex is locked.
 */
uint16_t ble_hs_hci_avail_pkts;

#if MYNEWT_VAL(BLE_HS_PHONY_HCI_ACKS)
static ble_hs_hci_phony_ack_fn *ble_hs_hci_phony_ack_cb;
#endif

#if MYNEWT_VAL(BLE_HS_PHONY_HCI_ACKS)
void
ble_hs_hci_set_phony_ack_cb(ble_hs_hci_phony_ack_fn *cb)
{
    ble_hs_hci_phony_ack_cb = cb;
}
#endif

static void
ble_hs_hci_lock(void)
{
    int rc;

    rc = ble_npl_mutex_pend(&ble_hs_hci_mutex, BLE_NPL_TIME_FOREVER);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0 || rc == OS_NOT_STARTED);
}

static void
ble_hs_hci_unlock(void)
{
    int rc;

    rc = ble_npl_mutex_release(&ble_hs_hci_mutex);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0 || rc == OS_NOT_STARTED);
}

int
ble_hs_hci_set_buf_sz(uint16_t pktlen, uint16_t max_pkts)
{
    if (pktlen == 0 || max_pkts == 0) {
        return BLE_HS_EINVAL;
    }

    ble_hs_hci_buf_sz = pktlen;
    ble_hs_hci_max_pkts = max_pkts;
    ble_hs_hci_avail_pkts = max_pkts;

    return 0;
}

/**
 * Increases the count of available controller ACL buffers.
 */
void
ble_hs_hci_add_avail_pkts(uint16_t delta)
{
    BLE_HS_DBG_ASSERT(ble_hs_locked_by_cur_task());

    if (ble_hs_hci_avail_pkts + delta > UINT16_MAX) {
        ble_hs_sched_reset(BLE_HS_ECONTROLLER);
    } else {
        ble_hs_hci_avail_pkts += delta;
    }
}

static int
ble_hs_hci_rx_cmd_complete(const void *data, int len,
                           struct ble_hs_hci_ack *out_ack)
{
    const struct ble_hci_ev_command_complete *ev = data;
    const struct ble_hci_ev_command_complete_nop *nop = data;
    uint16_t opcode;

    if (len < (int)sizeof(*ev)) {
        if (len < (int)sizeof(*nop)) {
            return BLE_HS_ECONTROLLER;
        }

        /* nop is special as it doesn't have status and response */

        opcode = le16toh(nop->opcode);
        if (opcode != BLE_HCI_OPCODE_NOP) {
            return BLE_HS_ECONTROLLER;
        }

        /* TODO Process num_pkts field. */

        out_ack->bha_status = 0;
        out_ack->bha_params = NULL;
        out_ack->bha_params_len = 0;
        return 0;
    }

    opcode = le16toh(ev->opcode);

    /* TODO Process num_pkts field. */

    out_ack->bha_opcode = opcode;

    out_ack->bha_status = BLE_HS_HCI_ERR(ev->status);
    out_ack->bha_params_len = len - sizeof(*ev);
    if (out_ack->bha_params_len) {
        out_ack->bha_params = ev->return_params;
    } else {
        out_ack->bha_params = NULL;
    }

    return 0;
}

static int
ble_hs_hci_rx_cmd_status(const void *data, int len,
                         struct ble_hs_hci_ack *out_ack)
{
    const struct ble_hci_ev_command_status *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    /* XXX: Process num_pkts field. */

    out_ack->bha_opcode = le16toh(ev->opcode);
    out_ack->bha_params = NULL;
    out_ack->bha_params_len = 0;
    out_ack->bha_status = BLE_HS_HCI_ERR(ev->status);

    return 0;
}

static int
ble_hs_hci_process_ack(uint16_t expected_opcode,
                       uint8_t *params_buf, uint8_t params_buf_len,
                       struct ble_hs_hci_ack *out_ack)
{
    int rc;

    BLE_HS_DBG_ASSERT(ble_hs_hci_ack != NULL);

    /* Count events received */
    STATS_INC(ble_hs_stats, hci_event);


    /* Clear ack fields up front to silence spurious gcc warnings. */
    memset(out_ack, 0, sizeof *out_ack);

    switch (ble_hs_hci_ack->opcode) {
    case BLE_HCI_EVCODE_COMMAND_COMPLETE:
        rc = ble_hs_hci_rx_cmd_complete(ble_hs_hci_ack->data,
                                        ble_hs_hci_ack->length, out_ack);
        break;

    case BLE_HCI_EVCODE_COMMAND_STATUS:
        rc = ble_hs_hci_rx_cmd_status(ble_hs_hci_ack->data,
                                      ble_hs_hci_ack->length, out_ack);
        break;

    default:
        BLE_HS_DBG_ASSERT(0);
        rc = BLE_HS_EUNKNOWN;
        break;
    }

    if (rc == 0) {
        if (params_buf == NULL || out_ack->bha_params == NULL) {
            out_ack->bha_params_len = 0;
        } else {
            if (out_ack->bha_params_len > params_buf_len) {
                out_ack->bha_params_len = params_buf_len;
                rc = BLE_HS_ECONTROLLER;
            }
            memcpy(params_buf, out_ack->bha_params, out_ack->bha_params_len);
        }
        out_ack->bha_params = params_buf;

        if (out_ack->bha_opcode != expected_opcode) {
            rc = BLE_HS_ECONTROLLER;
        }
    }

    if (rc != 0) {
        STATS_INC(ble_hs_stats, hci_invalid_ack);
    }

    return rc;
}

static int
ble_hs_hci_wait_for_ack(void)
{
    int rc;

#if MYNEWT_VAL(BLE_HS_PHONY_HCI_ACKS)
    if (ble_hs_hci_phony_ack_cb == NULL) {
        rc = BLE_HS_ETIMEOUT_HCI;
    } else {
        ble_hs_hci_ack = ble_transport_alloc_cmd();
        BLE_HS_DBG_ASSERT(ble_hs_hci_ack != NULL);
        rc = ble_hs_hci_phony_ack_cb((void *)ble_hs_hci_ack, 260);
    }
#else
    rc = ble_npl_sem_pend(&ble_hs_hci_sem,
                          ble_npl_time_ms_to_ticks32(BLE_HCI_CMD_TIMEOUT_MS));
    switch (rc) {
    case 0:
        BLE_HS_DBG_ASSERT(ble_hs_hci_ack != NULL);
        break;
    case OS_TIMEOUT:
        rc = BLE_HS_ETIMEOUT_HCI;
        STATS_INC(ble_hs_stats, hci_timeout);
        break;
    default:
        rc = BLE_HS_EOS;
        break;
    }
#endif
    return rc;
}

int
ble_hs_hci_cmd_tx_no_rsp(uint16_t opcode, const void *cmd, uint8_t cmd_len)
{
    int rc;

    ble_hs_hci_lock();

    rc = ble_hs_hci_cmd_send_buf(opcode, cmd, cmd_len);

    ble_hs_hci_unlock();

    return rc;
}

int
ble_hs_hci_cmd_tx(uint16_t opcode, const void *cmd, uint8_t cmd_len,
                  void *rsp, uint8_t rsp_len)
{
    struct ble_hs_hci_ack ack;
    int rc;

    BLE_HS_DBG_ASSERT(ble_hs_hci_ack == NULL);
    ble_hs_hci_lock();

    rc = ble_hs_hci_cmd_send_buf(opcode, cmd, cmd_len);
    if (rc != 0) {
        goto done;
    }

    rc = ble_hs_hci_wait_for_ack();
    if (rc != 0) {
        BLE_HS_LOG(ERROR, "HCI wait for ack returned %d \n", rc);
        ble_hs_sched_reset(rc);
        goto done;
    }

    rc = ble_hs_hci_process_ack(opcode, rsp, rsp_len, &ack);
    if (rc != 0) {
        BLE_HS_LOG(ERROR, "HCI process ack returned %d \n", rc);
        ble_hs_sched_reset(rc);
        goto done;
    }

    rc = ack.bha_status;

    /* on success we should always get full response */
    if (!rc && (ack.bha_params_len != rsp_len)) {
        BLE_HS_LOG(ERROR, "Received status %d \n", rc);
        ble_hs_sched_reset(rc);
        goto done;
    }
done:
    if (ble_hs_hci_ack != NULL) {
        ble_transport_free((uint8_t *) ble_hs_hci_ack);
        ble_hs_hci_ack = NULL;
    }

    ble_hs_hci_unlock();
    esp_hci_err_to_name(rc, &opcode);
    return rc;
}

#if MYNEWT_VAL(BLE_HCI_VS)
int
ble_hs_hci_send_vs_cmd(uint16_t ocf, const void *cmdbuf, uint8_t cmdlen,
                       void *rspbuf, uint8_t rsplen)
{
    int rc;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, ocf),
                           cmdbuf, cmdlen, rspbuf, rsplen);

    return rc;
}
#endif

static void
ble_hs_hci_rx_ack(uint8_t *ack_ev)
{
    if (ble_npl_sem_get_count(&ble_hs_hci_sem) > 0) {
        /* This ack is unexpected; ignore it. */
        ble_transport_free(ack_ev);
        return;
    }
    BLE_HS_DBG_ASSERT(ble_hs_hci_ack == NULL);

    /* Unblock the application now that the HCI command buffer is populated
     * with the acknowledgement.
     */
    ble_hs_hci_ack = (struct ble_hci_ev *) ack_ev;
    ble_npl_sem_release(&ble_hs_hci_sem);
}

// #if (BT_HCI_LOG_INCLUDED == TRUE)
// bool host_recv_adv_packet(uint8_t *data)
// {
//     assert(data);
//     if(data[0] == BLE_HCI_EVCODE_LE_META) {
//         if((data[2] ==  BLE_HCI_LE_SUBEV_ADV_RPT) || (data[2] == BLE_HCI_LE_SUBEV_DIRECT_ADV_RPT) ||
//         (data[2] == BLE_HCI_LE_SUBEV_EXT_ADV_RPT) || (data[2] == BLE_HCI_LE_SUBEV_PERIODIC_ADV_RPT)
// #if (BLE_ADV_REPORT_FLOW_CONTROL == TRUE)
//         || (data[2] ==  BLE_HCI_LE_SUBEV_DISCARD_REPORT_EVT)
// #endif
//         ) {
//             return true;
//         }
//     }
//     return false;
// }
// #endif // (BT_HCI_LOG_INCLUDED == TRUE)

int
ble_hs_hci_rx_evt(uint8_t *hci_ev, void *arg)
{
    struct ble_hci_ev *ev = (void *) hci_ev;
    struct ble_hci_ev_command_complete *cmd_complete = (void *) ev->data;
    struct ble_hci_ev_command_status *cmd_status = (void *) ev->data;
    int enqueue;

    BLE_HS_DBG_ASSERT(hci_ev != NULL);

// #if ((BT_HCI_LOG_INCLUDED == TRUE) && SOC_ESP_NIMBLE_CONTROLLER && CONFIG_BT_CONTROLLER_ENABLED)
//     uint16_t len = hci_ev[1] + 3;
//     if (host_recv_adv_packet(hci_ev)) {
//         bt_hci_log_record_hci_adv(HCI_LOG_DATA_TYPE_ADV, &hci_ev[1], len - 2);
//     } else {
//         bt_hci_log_record_hci_data(0x04, &hci_ev[0], len - 1);
//     }
// #endif // #if (BT_HCI_LOG_INCLUDED == TRUE)

    switch (ev->opcode) {
    case BLE_HCI_EVCODE_COMMAND_COMPLETE:
        enqueue = (cmd_complete->opcode == BLE_HCI_OPCODE_NOP);

	/* Check for BLE transmit opcodes which come in command complete */
	switch(cmd_complete->opcode) {
	    case BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RX_TEST):
	    case BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_TX_TEST):
	    case BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_TEST_END):
	    case BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RX_TEST_V2):
	    case BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_TX_TEST_V2):
	        enqueue = 1;
	        break;
	}
        break;
    case BLE_HCI_EVCODE_COMMAND_STATUS:
        enqueue = (cmd_status->opcode == BLE_HCI_OPCODE_NOP);
        break;
    default:
        enqueue = 1;
        break;
    }

    if (enqueue) {
        ble_hs_enqueue_hci_event(hci_ev);
    } else {
        ble_hs_hci_rx_ack(hci_ev);
    }

    return 0;
}

#if !(SOC_ESP_NIMBLE_CONTROLLER) || !(CONFIG_BT_CONTROLLER_ENABLED)
/**
 * Calculates the largest ACL payload that the controller can accept.
 */
static uint16_t
ble_hs_hci_max_acl_payload_sz(void)
{
    /* As per BLE 5.1 Standard, Vol. 2, Part E, section 7.8.2:
     * The LE_Read_Buffer_Size command is used to read the maximum size of the
     * data portion of HCI LE ACL Data Packets sent from the Host to the
     * Controller.
     */
    return ble_hs_hci_buf_sz;
}
#endif

/**
 * Allocates an mbuf to contain an outgoing ACL data fragment.
 */
static struct os_mbuf *
ble_hs_hci_frag_alloc(uint16_t frag_size, void *arg)
{
    struct os_mbuf *om;

    /* Prefer the dedicated one-element fragment pool. */
#if MYNEWT_VAL(BLE_CONTROLLER)
    om = os_mbuf_get_pkthdr(&ble_hs_hci_frag_mbuf_pool, sizeof(struct ble_mbuf_hdr));
#else
    om = os_mbuf_get_pkthdr(&ble_hs_hci_frag_mbuf_pool, 0);
#endif
    if (om != NULL) {
#if CONFIG_BT_NIMBLE_LEGACY_VHCI_ENABLE
        om->om_data += BLE_HCI_DATA_HDR_SZ;
#else
        om->om_data += BLE_HCI_DATA_HDR_SZ + BLE_HS_CTRL_DATA_HDR_SZ;
#endif
        return om;
    }

    /* Otherwise, fall back to msys. */
    om = ble_hs_mbuf_acl_pkt();
    if (om != NULL) {
        return om;
    }

    return NULL;
}

/**
 * Retrieves the total capacity of the ACL fragment pool (always 1).
 */
int
ble_hs_hci_frag_num_mbufs(void)
{
    return ble_hs_hci_frag_mempool.mp_num_blocks;
}

/**
 * Retrieves the the count of free buffers in the ACL fragment pool.
 */
int
ble_hs_hci_frag_num_mbufs_free(void)
{
    return ble_hs_hci_frag_mempool.mp_num_free;
}

static struct os_mbuf *
ble_hs_hci_acl_hdr_prepend(struct os_mbuf *om, uint16_t handle,
                           uint8_t pb_flag)
{
    struct hci_data_hdr hci_hdr;
    struct os_mbuf *om2;

    put_le16(&hci_hdr.hdh_handle_pb_bc,
             ble_hs_hci_util_handle_pb_bc_join(handle, pb_flag, 0));
    put_le16(&hci_hdr.hdh_len, OS_MBUF_PKTHDR(om)->omp_len);

    om2 = os_mbuf_prepend(om, sizeof hci_hdr);
    if (om2 == NULL) {
        return NULL;
    }

    om = om2;
    om = os_mbuf_pullup(om, sizeof hci_hdr);
    if (om == NULL) {
        return NULL;
    }

    memcpy(om->om_data, &hci_hdr, sizeof hci_hdr);

#if !BLE_MONITOR
    BLE_HS_LOG(DEBUG, "host tx hci data; handle=%d length=%d\n", handle,
               get_le16(&hci_hdr.hdh_len));
#endif

    return om;
}

int
ble_hs_hci_acl_tx_now(struct ble_hs_conn *conn, struct os_mbuf **om)
{
    struct os_mbuf *txom;
    struct os_mbuf *frag;
    uint8_t pb;
    int rc;

    BLE_HS_DBG_ASSERT(ble_hs_locked_by_cur_task());

    txom = *om;
    *om = NULL;

    if (!(conn->bhc_flags & BLE_HS_CONN_F_TX_FRAG)) {
        /* The first fragment uses the first-non-flush packet boundary value.
         * After sending the first fragment, pb gets set appropriately for all
         * subsequent fragments in this packet.
         */
        pb = BLE_HCI_PB_FIRST_NON_FLUSH;
    } else {
        pb = BLE_HCI_PB_MIDDLE;
    }

    /* Send fragments until the entire packet has been sent. */
    while (txom != NULL && ble_hs_hci_avail_pkts > 0) {
#if SOC_ESP_NIMBLE_CONTROLLER && CONFIG_BT_CONTROLLER_ENABLED
        frag = mem_split_frag(&txom, BLE_ACL_MAX_PKT_SIZE, ble_hs_hci_frag_alloc, NULL);
#else
        frag = mem_split_frag(&txom, ble_hs_hci_max_acl_payload_sz(), ble_hs_hci_frag_alloc, NULL);
#endif
        if (frag == NULL) {
            *om = txom;
            return BLE_HS_EAGAIN;
        }

        frag = ble_hs_hci_acl_hdr_prepend(frag, conn->bhc_handle, pb);
        if (frag == NULL) {
            rc = BLE_HS_ENOMEM;
            goto err;
        }

#if !BLE_MONITOR
        BLE_HS_LOG(DEBUG, "ble_hs_hci_acl_tx(): ");
        ble_hs_log_mbuf(frag);
        BLE_HS_LOG(DEBUG, "\n");
#endif

        rc = ble_hs_tx_data(frag);
        if (rc != 0) {
            goto err;
        }

        /* If any fragments remain, they should be marked as 'middle'
         * fragments.
         */
        conn->bhc_flags |= BLE_HS_CONN_F_TX_FRAG;
        pb = BLE_HCI_PB_MIDDLE;

        /* Account for the controller buf that will hold the txed fragment. */
        conn->bhc_outstanding_pkts++;
        ble_hs_hci_avail_pkts--;
    }

    if (txom != NULL) {
        /* The controller couldn't accommodate some or all of the packet. */
        *om = txom;
        return BLE_HS_EAGAIN;
    }

    /* The entire packet was transmitted. */
    conn->bhc_flags &= ~BLE_HS_CONN_F_TX_FRAG;

    return 0;

err:
    BLE_HS_DBG_ASSERT(rc != 0);

    conn->bhc_flags &= ~BLE_HS_CONN_F_TX_FRAG;
    os_mbuf_free_chain(txom);
    return rc;
}

/**
 * Transmits an HCI ACL data packet.  This function consumes the supplied mbuf,
 * regardless of the outcome.
 *
 * @return                      0 on success;
 *                              BLE_HS_EAGAIN if the packet could not be sent
 *                                  in its entirety due to controller buffer
 *                                  exhaustion.  The unsent data is pointed to
 *                                  by the `om` parameter.
 *                              A BLE host core return code on unexpected
 *                                  error.
 *
 */
int
ble_hs_hci_acl_tx(struct ble_hs_conn *conn, struct os_mbuf **om)
{
    BLE_HS_DBG_ASSERT(ble_hs_locked_by_cur_task());

    /* If this conn is already backed up, don't even try to send. */
    if (STAILQ_FIRST(&conn->bhc_tx_q) != NULL) {
        return BLE_HS_EAGAIN;
    }

    return ble_hs_hci_acl_tx_now(conn, om);
}

void
ble_hs_hci_set_le_supported_feat(uint32_t feat)
{
    ble_hs_hci_sup_feat = feat;
}

uint32_t
ble_hs_hci_get_le_supported_feat(void)
{
    return ble_hs_hci_sup_feat;
}

void
ble_hs_hci_set_hci_version(uint8_t hci_version)
{
    ble_hs_hci_version = hci_version;
}

uint8_t
ble_hs_hci_get_hci_version(void)
{
    return ble_hs_hci_version;
}

void
ble_hs_hci_set_hci_supported_cmd(struct ble_hs_hci_sup_cmd sup_cmd)
{
    ble_hs_hci_sup_cmd = sup_cmd;
}

struct ble_hs_hci_sup_cmd
ble_hs_hci_get_hci_supported_cmd(void)
{
    return ble_hs_hci_sup_cmd;
}

void
ble_hs_hci_init(void)
{
    int rc;

    rc = ble_npl_sem_init(&ble_hs_hci_sem, 0);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0);

    rc = ble_npl_mutex_init(&ble_hs_hci_mutex);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0);

    rc = mem_init_mbuf_pool(ble_hs_hci_frag_data,
                            &ble_hs_hci_frag_mempool,
                            &ble_hs_hci_frag_mbuf_pool,
                            1,
                            BLE_HS_HCI_FRAG_MEMBLOCK_SIZE,
                            "ble_hs_hci_frag");
    BLE_HS_DBG_ASSERT_EVAL(rc == 0);
}

void ble_hs_hci_deinit(void)
{
    int rc;

    rc = ble_npl_mutex_deinit(&ble_hs_hci_mutex);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0);

    rc = ble_npl_sem_deinit(&ble_hs_hci_sem);
    BLE_HS_DBG_ASSERT_EVAL(rc == 0);
}
