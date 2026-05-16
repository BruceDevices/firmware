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

#ifndef H_BLE_LL_CONN_
#define H_BLE_LL_CONN_

#include "nimble/porting/nimble/include/os/os.h"
#include "nimble/nimble/include/nimble/ble.h"
#include "nimble/nimble/include/nimble/hci_common.h"
#include "nimble/nimble/include/nimble/nimble_npl.h"
#include "nimble/nimble/controller/include/controller/ble_ll.h"
#include "nimble/nimble/controller/include/controller/ble_ll_sched.h"
#include "nimble/nimble/controller/include/controller/ble_ll_ctrl.h"
#include "nimble/nimble/controller/include/controller/ble_phy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Roles */
#define BLE_LL_CONN_ROLE_NONE           (0)

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
#define BLE_LL_CONN_ROLE_CENTRAL        (1)
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
#define BLE_LL_CONN_ROLE_PERIPHERAL     (2)
#endif

/* Connection states */
#define BLE_LL_CONN_STATE_IDLE          (0)
#define BLE_LL_CONN_STATE_CREATED       (1)
#define BLE_LL_CONN_STATE_ESTABLISHED   (2)

/* Definition for RSSI when the RSSI is unknown */
#define BLE_LL_CONN_UNKNOWN_RSSI        (127)

#define BLE_LL_CONN_HANDLE_ISO_OFFSET   (0x0100)

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
/*
 * Encryption states for a connection
 *
 * NOTE: the states are ordered so that we can check to see if the state
 * is greater than ENCRYPTED. If so, it means that the start or pause
 * encryption procedure is running and we should not send data pdu's.
 */
enum conn_enc_state {
    CONN_ENC_S_UNENCRYPTED = 1,
    CONN_ENC_S_ENCRYPTED,
    CONN_ENC_S_ENC_RSP_TO_BE_SENT,
    CONN_ENC_S_ENC_RSP_WAIT,
    CONN_ENC_S_PAUSE_ENC_RSP_WAIT,
    CONN_ENC_S_PAUSED,
    CONN_ENC_S_START_ENC_REQ_WAIT,
    CONN_ENC_S_START_ENC_RSP_WAIT,
    CONN_ENC_S_LTK_REQ_WAIT,
    CONN_ENC_S_LTK_NEG_REPLY
};

/*
 * Note that the LTK is the key, the SDK is the plain text, and the
 * session key is the cipher text portion of the encryption block.
 *
 * NOTE: we have intentionally violated the specification by making the
 * transmit and receive packet counters 32-bits as opposed to 39 (as per the
 * specification). We do this to save code space, ram and calculation time. The
 * only drawback is that any encrypted connection that sends more than 2^32
 * packets will suffer a MIC failure and thus be disconnected.
 */
struct ble_ll_conn_enc_data
{
    uint8_t enc_state;
    uint8_t tx_encrypted;
    uint16_t enc_div;
    uint32_t tx_pkt_cntr;
    uint32_t rx_pkt_cntr;
    uint64_t host_rand_num;
    uint8_t iv[8];
    struct ble_encryption_block enc_block;
};
#endif

/* Connection state machine flags. */
struct ble_ll_conn_sm_flags {
    uint32_t pkt_rxd : 1;
    uint32_t last_txd_md : 1;
    uint32_t empty_pdu_txd : 1;
    uint32_t periph_use_latency : 1;
    uint32_t periph_set_last_anchor : 1;
    uint32_t csa2 : 1;
    uint32_t encrypted : 1;
    uint32_t encrypt_ltk_req : 1;
    uint32_t encrypt_event_sent : 1;
    uint32_t version_ind_txd : 1;
    uint32_t version_ind_rxd : 1;
    uint32_t features_rxd : 1;
    uint32_t features_host_req : 1;
    uint32_t terminate_started : 1;
    uint32_t terminate_ind_txd : 1;
    uint32_t terminate_ind_rxd : 1;
    uint32_t terminate_ind_rxd_acked : 1;
    uint32_t conn_update_sched : 1;
    uint32_t conn_update_use_cp : 1;
    uint32_t conn_update_host_w4reply : 1;
    uint32_t conn_update_host_w4event : 1;
    uint32_t chanmap_update_sched : 1;
    uint32_t phy_update_sched : 1;
    uint32_t phy_update_self_initiated : 1;
    uint32_t phy_update_peer_initiated : 1;
    uint32_t phy_update_host_initiated : 1;
    uint32_t phy_update_host_w4event : 1;
    uint32_t le_ping_supp : 1;
#if MYNEWT_VAL(BLE_LL_CONN_INIT_AUTO_DLE)
    uint32_t pending_initiate_dle : 1;
#endif
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
    uint8_t subrate_trans : 1;
    uint8_t subrate_ind_txd : 1;
    uint8_t subrate_host_req : 1;
#endif
};

/**
 * Structure used for PHY data inside a connection.
 *
 * NOTE: the new phy's are the phys we will change to when a phy update
 * procedure is ongoing and the event counter hits the instant.
 *
 * tx_phy_mode: chip specific phy mode for tx
 * rx_phy_mode: chip specific phy mode for rx
 * cur_tx_phy: value denoting current tx_phy (not a bitmask!)
 * cur_rx_phy: value denoting current rx phy (not a bitmask!)
 * new_tx_phy: value denoting new tx_phy (not a bitmask!)
 * new_rx_phy: value denoting new rx phy (not a bitmask!)
 * req_pref_tx_phy: tx phy sent in a phy request (may be different than host)
 * req_pref_rx_phy: rx phy sent in a phy request (may be different than host)
 * host_pref_tx_phys: bitmask of preferred transmit PHYs sent by host
 * host_pref_rx_phys: bitmask of preferred receive PHYs sent by host
 * phy_options: preferred phy options for coded phy
 */
struct ble_ll_conn_phy_data
{
    uint32_t tx_phy_mode: 2;
    uint32_t rx_phy_mode: 2;
    uint32_t cur_tx_phy: 2;
    uint32_t cur_rx_phy: 2;
    uint32_t new_tx_phy: 2;
    uint32_t new_rx_phy: 2;
    uint32_t pref_mask_tx: 3;
    uint32_t pref_mask_rx: 3;
    uint32_t pref_mask_tx_req: 3;
    uint32_t pref_mask_rx_req: 3;
    uint32_t pref_opts: 2;
}  __attribute__((packed));

#define CONN_CUR_TX_PHY_MASK(csm)   (1 << ((csm)->phy_data.cur_tx_phy - 1))
#define CONN_CUR_RX_PHY_MASK(csm)   (1 << ((csm)->phy_data.cur_rx_phy - 1))

struct hci_conn_update
{
    uint16_t handle;
    uint16_t conn_itvl_min;
    uint16_t conn_itvl_max;
    uint16_t conn_latency;
    uint16_t supervision_timeout;
    uint16_t min_ce_len;
    uint16_t max_ce_len;
};

struct ble_ll_conn_subrate_params {
    uint16_t subrate_factor;
    uint16_t subrate_base_event;
    uint16_t periph_latency;
    uint16_t cont_num;
    uint16_t supervision_tmo;
};

struct ble_ll_conn_subrate_req_params {
    uint16_t subrate_min;
    uint16_t subrate_max;
    uint16_t max_latency;
    uint16_t cont_num;
    uint16_t supervision_tmo;
};

/* Connection state machine */
struct ble_ll_conn_sm
{
    /* Connection state machine flags */
    struct ble_ll_conn_sm_flags flags;

    /* Current connection handle, state and role */
    uint16_t conn_handle;
    uint8_t conn_state;
    uint8_t conn_role;          /* Can possibly be 1 bit */

    /* RSSI */
    int8_t conn_rssi;

    /* Connection data length management */
    uint8_t max_tx_octets;
    uint8_t max_rx_octets;
    uint8_t rem_max_tx_octets;
    uint8_t rem_max_rx_octets;
    uint8_t eff_max_tx_octets;
    uint8_t eff_max_rx_octets;
    uint16_t max_tx_time;
    uint16_t max_rx_time;
    uint16_t rem_max_tx_time;
    uint16_t rem_max_rx_time;
    uint16_t eff_max_tx_time;
    uint16_t eff_max_rx_time;
    uint16_t ota_max_rx_time;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    uint16_t host_req_max_tx_time;
    uint16_t host_req_max_rx_time;
#endif

#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    struct ble_ll_conn_phy_data phy_data;
    uint16_t phy_instant;
    uint8_t phy_tx_transition;
#endif

    /* Used to calculate data channel index for connection */
    uint8_t chan_map[BLE_LL_CHAN_MAP_LEN];
    uint8_t req_chanmap[BLE_LL_CHAN_MAP_LEN];
    uint16_t chanmap_instant;
    uint16_t channel_id; /* TODO could be union with hop and last chan used */
    uint8_t hop_inc;
    uint8_t data_chan_index;
    uint8_t last_unmapped_chan;
    uint8_t chan_map_used;

    /* Ack/Flow Control */
    uint8_t tx_seqnum;          /* note: can be 1 bit */
    uint8_t next_exp_seqnum;    /* note: can be 1 bit */
    uint8_t cons_rxd_bad_crc;   /* note: can be 1 bit */
    uint8_t last_rxd_sn;        /* note: cant be 1 bit given current code */
    uint8_t last_rxd_hdr_byte;  /* note: possibly can make 1 bit since we
                                   only use the MD bit now */

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_CTRL_TO_HOST_FLOW_CONTROL)
    uint16_t cth_flow_pending;
#endif

    /* connection event mgmt */
    uint8_t reject_reason;
    uint8_t host_reply_opcode;
    uint8_t central_sca;
    uint8_t tx_win_size;
    uint8_t cur_ctrl_proc;
    uint8_t disconnect_reason;
    uint8_t rxd_disconnect_reason;
    uint8_t vers_nr;
    uint8_t conn_features;
    uint8_t remote_features[7];
    uint16_t pending_ctrl_procs;
    uint16_t event_cntr;
    uint16_t completed_pkts;
    uint16_t comp_id;
    uint16_t sub_vers_nr;
    uint16_t auth_pyld_tmo;         /* could be ifdef'd. 10 msec units */

    uint32_t access_addr;
    uint32_t crcinit;               /* only low 24 bits used */
    /* XXX: do we need ce_end_time? Cant this be sched end time? */
    uint32_t ce_end_time;   /* cputime at which connection event should end */
    uint32_t terminate_timeout;
    uint32_t last_scheduled;

    /* Connection timing */
    uint16_t conn_itvl;
    uint16_t supervision_tmo;
    uint32_t max_ce_len_ticks;
    uint16_t tx_win_off;
    uint32_t anchor_point;
    uint8_t anchor_point_usecs;     /* XXX: can this be uint8_t ?*/
    uint8_t conn_itvl_usecs;
    uint32_t conn_itvl_ticks;
    uint32_t last_anchor_point;     /* Slave only */
    uint32_t periph_cur_tx_win_usecs;
    uint32_t periph_cur_window_widening;
    uint32_t last_rxd_pdu_cputime;  /* Used exclusively for supervision timer */

    uint16_t periph_latency;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
    uint16_t acc_subrate_min;
    uint16_t acc_subrate_max;
    uint16_t acc_max_latency;
    uint16_t acc_cont_num;
    uint16_t acc_supervision_tmo;

    uint16_t subrate_base_event;
    uint16_t subrate_factor;
    uint16_t cont_num;
    uint16_t cont_num_left;
    uint8_t has_nonempty_pdu;

    union {
        struct ble_ll_conn_subrate_params subrate_trans;
        struct ble_ll_conn_subrate_req_params subrate_req;
    };
#endif

    /*
     * Used to mark that identity address was used as InitA
     */
    uint8_t inita_identity_used;

    /* address information */
    uint8_t own_addr_type;
    uint8_t peer_addr_type;
    uint8_t peer_addr[BLE_DEV_ADDR_LEN];
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    uint8_t peer_addr_resolved;
#endif

    /*
     * XXX: TODO. Could save memory. Have single event at LL and put these
     * on a singly linked list. Only would need list pointer here.
     */
    /* Connection end event */
    struct ble_npl_event conn_ev_end;

    /* Packet transmit queue */
    struct os_mbuf *cur_tx_pdu;
    STAILQ_HEAD(conn_txq_head, os_mbuf_pkthdr) conn_txq;

    /* List entry for active/free connection pools */
    union {
        SLIST_ENTRY(ble_ll_conn_sm) act_sle;
        STAILQ_ENTRY(ble_ll_conn_sm) free_stqe;
    };

    /* LL control procedure response timer */
    struct ble_npl_callout ctrl_proc_rsp_timer;

    /* For scheduling connections */
    struct ble_ll_sched_item conn_sch;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_PING)
    struct ble_npl_callout auth_pyld_timer;
#endif

    /*
     * XXX: a note on all these structures for control procedures. First off,
     * all of these need to be ifdef'd to save memory. Another thing to
     * consider is this: since most control procedures can only run when no
     * others are running, can I use just one structure (a union)? Should I
     * allocate these from a pool? Not sure what to do. For now, I just use
     * a large chunk of memory per connection.
     */
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
    struct ble_ll_conn_enc_data enc_data;
#endif
    /*
     * For connection update procedure. XXX: can make this a pointer and
     * malloc it if we want to save space.
     */
    struct hci_conn_update conn_param_req;

    /* For connection update procedure */
    struct ble_ll_conn_upd_req conn_update_req;
    uint16_t conn_update_anchor_offset_req;

    /* XXX: for now, just store them all */
    struct ble_ll_conn_params conn_cp;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV_SYNC_TRANSFER)
    uint8_t  sync_transfer_mode;
    uint16_t sync_transfer_skip;
    uint32_t sync_transfer_sync_timeout;
#endif

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    SLIST_ENTRY(ble_ll_conn_sm) css_sle;
    uint16_t css_slot_idx;
    uint16_t css_slot_idx_pending;
    uint8_t css_period_idx;
#endif
};

/* Role */
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
#define CONN_IS_CENTRAL(csm)        (csm->conn_role == BLE_LL_CONN_ROLE_CENTRAL)
#else
#define CONN_IS_CENTRAL(csm)        (false)
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
#define CONN_IS_PERIPHERAL(csm)     (csm->conn_role == BLE_LL_CONN_ROLE_PERIPHERAL)
#else
#define CONN_IS_PERIPHERAL(csm)     (false)
#endif

static inline int
ble_ll_conn_rem_feature_check(struct ble_ll_conn_sm *connsm, uint64_t feature)
{
    uint8_t byte_idx;

    /* 8 lsb are conn features */
    feature >>= 8;

    byte_idx = __builtin_ctzll(feature) / 8;
    return connsm->remote_features[byte_idx] & (feature >> (byte_idx * 8));
}


static inline void
ble_ll_conn_rem_feature_add(struct ble_ll_conn_sm *connsm, uint64_t feature)
{
    uint8_t byte_idx;

    /* 8 lsb are conn features */
    feature >>= 8;

    byte_idx = __builtin_ctzll(feature) / 8;
    connsm->remote_features[byte_idx] |= (feature >> (byte_idx * 8));
}


struct ble_ll_conn_sm *ble_ll_conn_find_by_handle(uint16_t handle);
struct ble_ll_conn_sm *ble_ll_conn_find_by_peer_addr(const uint8_t* addr,
                                                     uint8_t addr_type);

/* Perform channel map update on all connections (applies to central role) */
void ble_ll_conn_chan_map_update(void);

/* required for unit testing */
uint8_t ble_ll_conn_calc_dci(struct ble_ll_conn_sm *conn, uint16_t latency);

/* used to get anchor point for connection event specified */
void ble_ll_conn_get_anchor(struct ble_ll_conn_sm *connsm, uint16_t conn_event,
                            uint32_t *anchor, uint8_t *anchor_usecs);

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
int ble_ll_conn_move_anchor(struct ble_ll_conn_sm *connsm, uint16_t offset);
#endif

struct ble_ll_scan_addr_data;
struct ble_ll_scan_pdu_data;

uint8_t ble_ll_conn_tx_connect_ind_pducb(uint8_t *dptr, void *pducb_arg,
                                         uint8_t *hdr_byte);
void ble_ll_conn_prepare_connect_ind(struct ble_ll_conn_sm *connsm,
                                     struct ble_ll_scan_pdu_data *pdu_data,
                                     struct ble_ll_scan_addr_data *addrd,
                                     uint8_t channel);

/* Send CONNECT_IND/AUX_CONNECT_REQ */
int ble_ll_conn_send_connect_req(struct os_mbuf *rxpdu,
                                 struct ble_ll_scan_addr_data *addrd,
                                 uint8_t ext);
/* Cancel connection after AUX_CONNECT_REQ was sent */
void ble_ll_conn_send_connect_req_cancel(void);
/* Signal connection created via CONNECT_IND */
void ble_ll_conn_created_on_legacy(struct os_mbuf *rxpdu,
                                   struct ble_ll_scan_addr_data *addrd,
                                   uint8_t *targeta);
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
/* Signal connection created via AUX_CONNECT_REQ */
void ble_ll_conn_created_on_aux(struct os_mbuf *rxpdu,
                                struct ble_ll_scan_addr_data *addrd,
                                uint8_t *targeta);
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
int ble_ll_conn_subrate_req_hci(struct ble_ll_conn_sm *connsm,
                                struct ble_ll_conn_subrate_req_params *srp);
int ble_ll_conn_subrate_req_llcp(struct ble_ll_conn_sm *connsm,
                                 struct ble_ll_conn_subrate_req_params *srp);
void ble_ll_conn_subrate_set(struct ble_ll_conn_sm *connsm,
                             struct ble_ll_conn_subrate_params *sp);
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_LL_CONN_ */
