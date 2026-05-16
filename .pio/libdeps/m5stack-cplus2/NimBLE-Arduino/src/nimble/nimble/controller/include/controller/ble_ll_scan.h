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

#ifndef H_BLE_LL_SCAN_
#define H_BLE_LL_SCAN_

#include "nimble/nimble/controller/include/controller/ble_ll_sched.h"
#include "nimble/nimble/controller/include/controller/ble_ll_tmr.h"
#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/nimble/include/nimble/nimble_npl.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SCAN_REQ
 *      -> ScanA    (6 bytes)
 *      -> AdvA     (6 bytes)
 *
 *  ScanA is the scanners public (TxAdd=0) or random (TxAdd = 1) address
 *  AdvaA is the advertisers public (RxAdd=0) or random (RxAdd=1) address.
 *
 * Sent by the LL in the Scanning state; received by the LL in the advertising
 * state. The advertising address is the intended recipient of this frame.
 */
#define BLE_SCAN_REQ_LEN                (12)

/*
 * SCAN_RSP
 *      -> AdvA         (6 bytes)
 *      -> ScanRspData  (0 - 31 bytes)
 *
 *  AdvaA is the advertisers public (TxAdd=0) or random (TxAdd=1) address.
 *  ScanRspData may contain any data from the advertisers host.
 *
 * Sent by the LL in the advertising state; received by the LL in the
 * scanning state.
 */
#define BLE_SCAN_RSP_LEGACY_DATA_MAX_LEN       (31)
#define BLE_SCAN_LEGACY_MAX_PKT_LEN            (37)

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
#define BLE_SCAN_RSP_DATA_MAX_LEN       MYNEWT_VAL(BLE_EXT_ADV_MAX_SIZE)

/* For Bluetooth 5.0 we need state machine for two PHYs*/
#define BLE_LL_SCAN_PHY_NUMBER          (2)
#else
#define BLE_LL_SCAN_PHY_NUMBER          (1)
#define BLE_SCAN_RSP_DATA_MAX_LEN       BLE_SCAN_RSP_LEGACY_DATA_MAX_LEN
#endif

#define PHY_UNCODED                    (0)
#define PHY_CODED                      (1)

#define BLE_LL_EXT_ADV_MODE_NON_CONN    (0x00)
#define BLE_LL_EXT_ADV_MODE_CONN        (0x01)
#define BLE_LL_EXT_ADV_MODE_SCAN        (0x02)

/* All values are stored as ticks */
struct ble_ll_scan_timing {
    uint32_t interval;
    uint32_t window;
    uint32_t start_time;
};

struct ble_ll_scan_phy
{
    uint8_t phy;
    uint8_t configured;
    uint8_t scan_type;
    uint8_t scan_chan;
    struct ble_ll_scan_timing timing;
};

struct ble_ll_scan_pdu_data {
    uint8_t hdr_byte;
    /* ScanA for SCAN_REQ and InitA for CONNECT_IND */
    union {
        uint8_t scana[BLE_DEV_ADDR_LEN];
        uint8_t inita[BLE_DEV_ADDR_LEN];
    };
    uint8_t adva[BLE_DEV_ADDR_LEN];
};

struct ble_ll_scan_addr_data {
    uint8_t *adva;
    uint8_t *targeta;
    uint8_t *adv_addr;
    uint8_t adva_type : 1;
    uint8_t targeta_type : 1;
    uint8_t adv_addr_type : 1;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    uint8_t adva_resolved : 1;
    uint8_t targeta_resolved : 1;
    int8_t rpa_index;
#endif
};

struct ble_ll_scan_sm
{
    uint8_t scan_enabled;

    uint8_t own_addr_type;
    uint8_t scan_filt_policy;
    uint8_t scan_filt_dups;
    uint8_t scan_rsp_pending;
    uint8_t scan_rsp_cons_fails;
    uint8_t scan_rsp_cons_ok;
    uint8_t scan_peer_rpa[BLE_DEV_ADDR_LEN];
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    ble_npl_time_t scan_nrpa_timer;
    uint8_t scan_nrpa[BLE_DEV_ADDR_LEN];
#endif
    struct ble_ll_scan_pdu_data pdu_data;

    /* XXX: Shall we count backoff per phy? */
    uint16_t upper_limit;
    uint16_t backoff_count;
    uint32_t scan_win_start_time;
    struct ble_npl_event scan_sched_ev;
    struct ble_ll_tmr scan_timer;
    struct ble_npl_event scan_interrupted_ev;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    struct ble_npl_callout duration_timer;
    struct ble_npl_callout period_timer;
    ble_npl_time_t duration_ticks;
    ble_npl_time_t period_ticks;
    uint8_t ext_scanning;
#endif

    uint8_t restart_timer_needed;

    struct ble_ll_scan_phy *scanp;
    struct ble_ll_scan_phy *scanp_next;
    struct ble_ll_scan_phy scan_phys[BLE_LL_SCAN_PHY_NUMBER];

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    /* Connection sm for initiator scan */
    struct ble_ll_conn_sm *connsm;
#endif
};

/* Scan types */
#define BLE_SCAN_TYPE_PASSIVE   (BLE_HCI_SCAN_TYPE_PASSIVE)
#define BLE_SCAN_TYPE_ACTIVE    (BLE_HCI_SCAN_TYPE_ACTIVE)
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
#define BLE_SCAN_TYPE_INITIATE  (2)
#endif

/*---- HCI ----*/
/* Set scanning parameters */
int ble_ll_scan_hci_set_params(const uint8_t *cmdbuf, uint8_t len);

/* Turn scanning on/off */
int ble_ll_scan_hci_set_enable(const uint8_t *cmdbuf, uint8_t len);
int ble_ll_scan_hci_set_ext_enable(const uint8_t *cmdbuf, uint8_t len);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
int ble_ll_scan_hci_set_ext_params(const uint8_t *cmdbuf, uint8_t len);
#endif

/*--- Controller Internal API ---*/
/* Initialize the scanner */
void ble_ll_scan_init(void);

/* Reset the scanner */
void ble_ll_scan_reset(void);

/* Called when Link Layer starts to receive a PDU and is in scanning state */
int ble_ll_scan_rx_isr_start(uint8_t pdu_type, uint16_t *rxflags);

/* Called when Link Layer has finished receiving a PDU while scanning */
int ble_ll_scan_rx_isr_end(struct os_mbuf *rxpdu, uint8_t crcok);

/* Process a scan response PDU */
void ble_ll_scan_rx_pkt_in(uint8_t pdu_type, struct os_mbuf *om,
                           struct ble_mbuf_hdr *hdr);

/* Boolean function denoting whether or not the whitelist can be changed */
int ble_ll_scan_can_chg_whitelist(void);

/* Boolean function returning true if scanning enabled */
int ble_ll_scan_enabled(void);

/* Initialize the scanner when we start initiating */
struct ble_ll_conn_create_scan;
struct ble_ll_conn_create_params;
int
ble_ll_scan_initiator_start(struct ble_ll_conn_sm *connsm, uint8_t ext,
                            struct ble_ll_conn_create_scan *cc_scan);

/* Returns storage for PDU data (for SCAN_REQ or CONNECT_IND) */
struct ble_ll_scan_pdu_data *ble_ll_scan_get_pdu_data(void);

/* Called to set the resolvable private address of the last connected peer */
void ble_ll_scan_set_peer_rpa(uint8_t *rpa);

/* Returns peer RPA of last connection made */
uint8_t *ble_ll_scan_get_peer_rpa(void);

/* Returns the local RPA used by the scanner/initiator */
uint8_t *ble_ll_scan_get_local_rpa(void);

/* Stop the scanning state machine */
void ble_ll_scan_sm_stop(int chk_disable);

/* Resume scanning */
#if MYNEWT_VAL(BLE_LL_ROLE_OBSERVER)
void ble_ll_scan_chk_resume(void);
#else
static inline void ble_ll_scan_chk_resume(void) { };
#endif

/* Called when wait for response timer expires in scanning mode */
void ble_ll_scan_wfr_timer_exp(void);

/* Called when scan could be interrupted  */
void ble_ll_scan_interrupted(struct ble_ll_scan_sm *scansm);

/* Called to halt currently running scan */
void ble_ll_scan_halt(void);

uint8_t *ble_ll_get_scan_nrpa(void);
uint8_t ble_ll_scan_get_own_addr_type(void);
uint8_t ble_ll_scan_get_filt_policy(void);
uint8_t ble_ll_scan_get_filt_dups(void);
uint8_t ble_ll_scan_backoff_kick(void);
void ble_ll_scan_backoff_update(int success);

int ble_ll_scan_dup_check_ext(uint8_t addr_type, uint8_t *addr, bool has_aux,
                              uint16_t adi);
int ble_ll_scan_dup_update_ext(uint8_t addr_type, uint8_t *addr, bool has_aux,
                               uint16_t adi);
int ble_ll_scan_have_rxd_scan_rsp(uint8_t *addr, uint8_t txadd, uint8_t ext_adv,
                                  uint16_t adi);
void ble_ll_scan_add_scan_rsp_adv(uint8_t *addr, uint8_t txadd, uint8_t ext_adv,
                                  uint16_t adi);

int
ble_ll_scan_rx_filter(uint8_t own_addr_type, uint8_t scan_filt_policy,
                      struct ble_ll_scan_addr_data *addrd, uint8_t *scan_ok);
int ble_ll_scan_rx_check_init(struct ble_ll_scan_addr_data *addrd);

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_LL_SCAN_ */
