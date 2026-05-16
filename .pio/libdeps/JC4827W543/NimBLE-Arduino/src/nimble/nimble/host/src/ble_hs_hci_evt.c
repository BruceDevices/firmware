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
#include "nimble/nimble/include/nimble/hci_common.h"
#include "nimble/nimble/host/include/host/ble_gap.h"
#include "ble_hs_priv.h"
#include "ble_hs_resolv_priv.h"
#ifdef ESP_PLATFORM
#include "nimble/esp_port/port/include/esp_nimble_mem.h"
#endif

#if MYNEWT_VAL(BLE_ENABLE_CONN_REATTEMPT)
struct ble_gap_reattempt_ctxt {
    ble_addr_t peer_addr;
    uint8_t count;
}reattempt_conn;

extern int ble_gap_master_connect_reattempt(uint16_t conn_handle);
extern int ble_gap_slave_adv_reattempt(void);
extern int slave_conn[MYNEWT_VAL(BLE_MAX_CONNECTIONS) + 1];
#endif

#if MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
static struct ble_npl_mutex adv_list_lock;
static uint16_t ble_adv_list_count;
#define  BLE_ADV_LIST_MAX_LENGTH    50
#define  BLE_ADV_LIST_MAX_COUNT     200
#endif

_Static_assert(sizeof (struct hci_data_hdr) == BLE_HCI_DATA_HDR_SZ,
               "struct hci_data_hdr must be 4 bytes");

typedef int ble_hs_hci_evt_fn(uint8_t event_code, const void *data,
                              unsigned int len);
static ble_hs_hci_evt_fn ble_hs_hci_evt_hw_error;
static ble_hs_hci_evt_fn ble_hs_hci_evt_num_completed_pkts;
static ble_hs_hci_evt_fn ble_hs_hci_evt_rd_rem_ver_complete;
#if NIMBLE_BLE_CONNECT
static ble_hs_hci_evt_fn ble_hs_hci_evt_disconn_complete;
static ble_hs_hci_evt_fn ble_hs_hci_evt_encrypt_change;
static ble_hs_hci_evt_fn ble_hs_hci_evt_enc_key_refresh;
#endif
static ble_hs_hci_evt_fn ble_hs_hci_evt_le_meta;
#if MYNEWT_VAL(BLE_HCI_VS)
static ble_hs_hci_evt_fn ble_hs_hci_evt_vs;
#endif

static ble_hs_hci_evt_fn ble_hs_hci_evt_rx_test;
static ble_hs_hci_evt_fn ble_hs_hci_evt_tx_test;
static ble_hs_hci_evt_fn ble_hs_hci_evt_end_test;

typedef int ble_hs_hci_evt_le_fn(uint8_t subevent, const void *data,
                                 unsigned int len);
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_adv_rpt;
#if NIMBLE_BLE_CONNECT
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_conn_complete;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_conn_upd_complete;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_lt_key_req;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_conn_parm_req;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_data_len_change;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_phy_update_complete;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_enh_conn_complete;
#endif
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_dir_adv_rpt;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_ext_adv_rpt;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_rd_rem_used_feat_complete;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_scan_timeout;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_adv_set_terminated;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_periodic_adv_sync_estab;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_periodic_adv_rpt;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_periodic_adv_sync_lost;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_scan_req_rcvd;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_periodic_adv_sync_transfer;
#if MYNEWT_VAL(BLE_PERIODIC_ADV_SYNC_BIGINFO_REPORTS)
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_biginfo_adv_report;
#endif
#if MYNEWT_VAL(BLE_POWER_CONTROL)
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_pathloss_threshold;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_transmit_power_report;
#endif
#if MYNEWT_VAL(BLE_CONN_SUBRATING)
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_subrate_change;
#endif
#if MYNEWT_VAL(BLE_AOA_AOD)
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_connless_iq_report;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_conn_iq_report;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_cte_req_failed;
#endif
#if MYNEWT_VAL(BLE_PERIODIC_ADV_WITH_RESPONSES)
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_periodic_adv_subev_data_req;
static ble_hs_hci_evt_le_fn ble_hs_hci_evt_le_periodic_adv_subev_resp_rep;
#endif

/* Statistics */
struct host_hci_stats {
    uint32_t events_rxd;
    uint32_t good_acks_rxd;
    uint32_t bad_acks_rxd;
    uint32_t unknown_events_rxd;
};

#define BLE_HS_HCI_EVT_TIMEOUT        50      /* Milliseconds. */

/** Dispatch table for incoming HCI events.  Sorted by event code field. */
struct ble_hs_hci_evt_dispatch_entry {
    uint8_t event_code;
    ble_hs_hci_evt_fn *cb;
};

static const struct ble_hs_hci_evt_dispatch_entry ble_hs_hci_evt_dispatch[] = {
    { BLE_HCI_EVCODE_LE_META, ble_hs_hci_evt_le_meta },
    { BLE_HCI_EVCODE_RD_REM_VER_INFO_CMP, ble_hs_hci_evt_rd_rem_ver_complete },
    { BLE_HCI_EVCODE_NUM_COMP_PKTS, ble_hs_hci_evt_num_completed_pkts },
#if NIMBLE_BLE_CONNECT
    { BLE_HCI_EVCODE_DISCONN_CMP, ble_hs_hci_evt_disconn_complete },
    { BLE_HCI_EVCODE_ENCRYPT_CHG, ble_hs_hci_evt_encrypt_change },
    { BLE_HCI_EVCODE_ENC_KEY_REFRESH, ble_hs_hci_evt_enc_key_refresh },
#endif
    { BLE_HCI_EVCODE_HW_ERROR, ble_hs_hci_evt_hw_error },
#if MYNEWT_VAL(BLE_HCI_VS)
    { BLE_HCI_EVCODE_VS, ble_hs_hci_evt_vs },
#endif
    { BLE_HCI_OCF_LE_RX_TEST, ble_hs_hci_evt_rx_test },
    { BLE_HCI_OCF_LE_TX_TEST, ble_hs_hci_evt_tx_test },
    { BLE_HCI_OCF_LE_TEST_END, ble_hs_hci_evt_end_test },
    { BLE_HCI_OCF_LE_RX_TEST_V2, ble_hs_hci_evt_rx_test },
    { BLE_HCI_OCF_LE_TX_TEST_V2, ble_hs_hci_evt_tx_test },
};

#define BLE_HS_HCI_EVT_DISPATCH_SZ \
    (sizeof ble_hs_hci_evt_dispatch / sizeof ble_hs_hci_evt_dispatch[0])

static ble_hs_hci_evt_le_fn * const ble_hs_hci_evt_le_dispatch[] = {
#if NIMBLE_BLE_CONNECT
    [BLE_HCI_LE_SUBEV_CONN_COMPLETE] = ble_hs_hci_evt_le_conn_complete,
#endif
    [BLE_HCI_LE_SUBEV_ADV_RPT] = ble_hs_hci_evt_le_adv_rpt,
#if NIMBLE_BLE_CONNECT
    [BLE_HCI_LE_SUBEV_CONN_UPD_COMPLETE] = ble_hs_hci_evt_le_conn_upd_complete,
    [BLE_HCI_LE_SUBEV_LT_KEY_REQ] = ble_hs_hci_evt_le_lt_key_req,
    [BLE_HCI_LE_SUBEV_REM_CONN_PARM_REQ] = ble_hs_hci_evt_le_conn_parm_req,
    [BLE_HCI_LE_SUBEV_DATA_LEN_CHG] = ble_hs_hci_evt_le_data_len_change,
    [BLE_HCI_LE_SUBEV_ENH_CONN_COMPLETE] = ble_hs_hci_evt_le_enh_conn_complete,
#endif
    [BLE_HCI_LE_SUBEV_DIRECT_ADV_RPT] = ble_hs_hci_evt_le_dir_adv_rpt,
#if NIMBLE_BLE_CONNECT
    [BLE_HCI_LE_SUBEV_PHY_UPDATE_COMPLETE] = ble_hs_hci_evt_le_phy_update_complete,
#endif
    [BLE_HCI_LE_SUBEV_EXT_ADV_RPT] = ble_hs_hci_evt_le_ext_adv_rpt,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_SYNC_ESTAB] = ble_hs_hci_evt_le_periodic_adv_sync_estab,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_RPT] = ble_hs_hci_evt_le_periodic_adv_rpt,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_SYNC_LOST] = ble_hs_hci_evt_le_periodic_adv_sync_lost,
    [BLE_HCI_LE_SUBEV_RD_REM_USED_FEAT] = ble_hs_hci_evt_le_rd_rem_used_feat_complete,
    [BLE_HCI_LE_SUBEV_SCAN_TIMEOUT] = ble_hs_hci_evt_le_scan_timeout,
    [BLE_HCI_LE_SUBEV_ADV_SET_TERMINATED] = ble_hs_hci_evt_le_adv_set_terminated,
    [BLE_HCI_LE_SUBEV_SCAN_REQ_RCVD] = ble_hs_hci_evt_le_scan_req_rcvd,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_SYNC_TRANSFER] = ble_hs_hci_evt_le_periodic_adv_sync_transfer,
#if MYNEWT_VAL(BLE_PERIODIC_ADV_SYNC_BIGINFO_REPORTS)
    [BLE_HCI_LE_SUBEV_BIGINFO_ADV_REPORT] = ble_hs_hci_evt_le_biginfo_adv_report,
#endif
#if MYNEWT_VAL(BLE_POWER_CONTROL)
    [BLE_HCI_LE_SUBEV_PATH_LOSS_THRESHOLD] = ble_hs_hci_evt_le_pathloss_threshold,
    [BLE_HCI_LE_SUBEV_TRANSMIT_POWER_REPORT] = ble_hs_hci_evt_le_transmit_power_report,
#endif
#if MYNEWT_VAL(BLE_CONN_SUBRATING)
    [BLE_HCI_LE_SUBEV_SUBRATE_CHANGE] = ble_hs_hci_evt_le_subrate_change,
#endif
#if MYNEWT_VAL(BLE_AOA_AOD)
    [BLE_HCI_LE_SUBEV_CONNLESS_IQ_RPT] = ble_hs_hci_evt_le_connless_iq_report,
    [BLE_HCI_LE_SUBEV_CONN_IQ_RPT] = ble_hs_hci_evt_le_conn_iq_report,
    [BLE_HCI_LE_SUBEV_CTE_REQ_FAILED] = ble_hs_hci_evt_le_cte_req_failed,
#endif
#if MYNEWT_VAL(BLE_PERIODIC_ADV_WITH_RESPONSES)
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_SYNC_ESTAB_V2] = ble_hs_hci_evt_le_periodic_adv_sync_estab,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_RPT_V2] = ble_hs_hci_evt_le_periodic_adv_rpt,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_SYNC_TRANSFER_V2] = ble_hs_hci_evt_le_periodic_adv_sync_transfer,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_SUBEV_DATA_REQ] = ble_hs_hci_evt_le_periodic_adv_subev_data_req,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_RESP_REPORT] = ble_hs_hci_evt_le_periodic_adv_subev_resp_rep,
    [BLE_HCI_LE_SUBEV_ENH_CONN_COMPLETE_V2] = ble_hs_hci_evt_le_enh_conn_complete,
#endif
};

#define BLE_HS_HCI_EVT_LE_DISPATCH_SZ \
    (sizeof ble_hs_hci_evt_le_dispatch / sizeof ble_hs_hci_evt_le_dispatch[0])

static const struct ble_hs_hci_evt_dispatch_entry *
ble_hs_hci_evt_dispatch_find(uint8_t event_code)
{
    const struct ble_hs_hci_evt_dispatch_entry *entry;
    unsigned int i;

    for (i = 0; i < BLE_HS_HCI_EVT_DISPATCH_SZ; i++) {
        entry = ble_hs_hci_evt_dispatch + i;
        if (entry->event_code == event_code) {
            return entry;
        }
    }

    return NULL;
}

#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
    static const uint8_t ble_hs_conn_null_addr[6];
#endif

static ble_hs_hci_evt_le_fn *
ble_hs_hci_evt_le_dispatch_find(uint8_t event_code)
{
    if (event_code >= BLE_HS_HCI_EVT_LE_DISPATCH_SZ) {
        return NULL;
    }
    return ble_hs_hci_evt_le_dispatch[event_code];
}

#if NIMBLE_BLE_CONNECT
static int
ble_hs_hci_evt_disconn_complete(uint8_t event_code, const void *data,
                                unsigned int len)
{
    const struct ble_hci_ev_disconn_cmp *ev = data;
    const struct ble_hs_conn *conn;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_hs_lock();
    conn = ble_hs_conn_find(le16toh(ev->conn_handle));
    if (conn != NULL) {
        ble_hs_hci_add_avail_pkts(conn->bhc_outstanding_pkts);
    }
    ble_hs_unlock();

#if MYNEWT_VAL(BLE_ENABLE_CONN_REATTEMPT)
    if (conn) {
        uint16_t handle;
	int rc;

	/* For master role, check if failure reason is 0x3E, to restart connect attempt
	 * For slave role, check whether
	 *   a. Failure reason is 0x3E
	 *   b. Connect event was not posted and 0x8 was received
	 *  Restart advertising in above reasons for slave.
	 */

        if ((conn->bhc_flags & BLE_HS_CONN_F_MASTER) && \
	    (ev->reason == BLE_ERR_CONN_ESTABLISHMENT)) {  // master
	    if (reattempt_conn.count < MAX_REATTEMPT_ALLOWED) {
                /* Go for connection */
                BLE_HS_LOG(INFO, "Reattempt connection; reason = 0x%x, status = %d,"
                                 "reattempt count = %d ", ev->reason, ev->status,
                                  reattempt_conn.count);
                reattempt_conn.count += 1;

		handle = le16toh(ev->conn_handle);
		/* Post event to interested application */
		ble_gap_reattempt_count(handle, reattempt_conn.count);

                rc = ble_gap_master_connect_reattempt(ev->conn_handle);
		if (rc != 0) {
		    BLE_HS_LOG(INFO, "Master reconnect attempt failed; rc = %d", rc);
		}
	    } else {
	        /* Exhausted attempts */
		memset(&reattempt_conn, 0x0, sizeof (struct ble_gap_reattempt_ctxt));
	    }
	}
	else if (!(conn->bhc_flags & BLE_HS_CONN_F_MASTER) && \
		((ev->reason == BLE_ERR_CONN_ESTABLISHMENT) || \
		(!slave_conn[ev->conn_handle] && ev->reason == BLE_ERR_CONN_SPVN_TMO))) { //slave

	    BLE_HS_LOG(INFO, "Reattempt advertising; reason: 0x%x, status = %x",
                             ev->reason, ev->status);
            ble_l2cap_sig_conn_broken(ev->conn_handle, BLE_ERR_CONN_ESTABLISHMENT);
            ble_sm_connection_broken(ev->conn_handle);
            ble_gatts_connection_broken(ev->conn_handle);
            ble_gattc_connection_broken(ev->conn_handle);
            ble_hs_flow_connection_broken(ev->conn_handle);;
#if MYNEWT_VAL(BLE_GATT_CACHING)
            ble_gattc_cache_conn_broken(ev->conn_handle);
#endif
            rc = ble_hs_atomic_conn_delete(ev->conn_handle);
            if (rc != 0) {
                return rc;
            }

            rc = ble_gap_slave_adv_reattempt();
            if (rc != 0) {
                BLE_HS_LOG(INFO, "Adv reattempt failed; rc= %d ", rc);
            }

            return 0;  // Restart advertising, so don't post disconnect event
	} else {
            /* Normal disconnect. Reset the structure */
            memset(&reattempt_conn, 0x0, sizeof (struct ble_gap_reattempt_ctxt));
	}
    }
#endif

    ble_gap_rx_disconn_complete(ev);

    /* The connection termination may have freed up some capacity in the
     * controller for additional ACL data packets.  Wake up any stalled
     * connections.
     */
    ble_hs_wakeup_tx();

    return 0;
}

static int
ble_hs_hci_evt_encrypt_change(uint8_t event_code, const void *data,
                              unsigned int len)
{
    const struct ble_hci_ev_enrypt_chg *ev = data;

    if (len != sizeof (*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_sm_enc_change_rx(ev);

    return 0;
}
#endif
static int
ble_hs_hci_evt_hw_error(uint8_t event_code, const void *data, unsigned int len)
{
    const struct ble_hci_ev_hw_error *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_hs_hw_error(ev->hw_code);

    return 0;
}

#if NIMBLE_BLE_CONNECT
static int
ble_hs_hci_evt_enc_key_refresh(uint8_t event_code, const void *data,
                               unsigned int len)
{
    const struct ble_hci_ev_enc_key_refresh *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_sm_enc_key_refresh_rx(ev);

    return 0;
}
#endif

static int
ble_hs_hci_evt_num_completed_pkts(uint8_t event_code, const void *data,
                                  unsigned int len)
{
    const struct ble_hci_ev_num_comp_pkts *ev = data;
    struct ble_hs_conn *conn;
    uint16_t num_pkts;
    int i;

    if (len < sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    if (len != sizeof(*ev) + (ev->count * sizeof(ev->completed[0]))) {
        return BLE_HS_ECONTROLLER;
    }

    for (i = 0; i < ev->count; i++) {
        num_pkts = le16toh(ev->completed[i].packets);

        if (num_pkts > 0) {
            ble_hs_lock();
            conn = ble_hs_conn_find(le16toh(ev->completed[i].handle));
            if (conn != NULL) {
                if (conn->bhc_outstanding_pkts < num_pkts) {
                    ble_hs_sched_reset(BLE_HS_ECONTROLLER);
                } else {
                    conn->bhc_outstanding_pkts -= num_pkts;
                }

                ble_hs_hci_add_avail_pkts(num_pkts);
            }
            ble_hs_unlock();
        }
    }

    /* If any transmissions have stalled, wake them up now. */
    ble_hs_wakeup_tx();

    return 0;
}

#if MYNEWT_VAL(BLE_HCI_VS)
static int
ble_hs_hci_evt_vs(uint8_t event_code, const void *data, unsigned int len)
{
    const struct ble_hci_ev_vs *ev = data;

    if (len < sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_vs_hci_event(data, len);

    return 0;
}
#endif

static int
ble_hs_hci_evt_rx_test(uint8_t event_code, const void *data, unsigned int len)
{
    ble_gap_rx_test_evt(data, len);

    return 0;
}

static int
ble_hs_hci_evt_tx_test(uint8_t event_code, const void *data, unsigned int len)
{
    ble_gap_tx_test_evt(data, len);

    return 0;
}

static int
ble_hs_hci_evt_end_test(uint8_t event_code, const void *data, unsigned int len)
{
    ble_gap_end_test_evt(data, len);

    return 0;
}

static int
ble_hs_hci_evt_le_meta(uint8_t event_code, const void *data, unsigned int len)
{
    const struct ble_hci_ev_le_meta *ev = data;
    ble_hs_hci_evt_le_fn *fn;

    if (len < sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    fn = ble_hs_hci_evt_le_dispatch_find(ev->subevent);
    if (fn) {
        return fn(ev->subevent, data, len);
    }

    return 0;
}

#if MYNEWT_VAL(BLE_EXT_ADV)
static struct ble_gap_conn_complete pend_conn_complete;
#endif

#if NIMBLE_BLE_CONNECT
static int
ble_hs_hci_evt_le_enh_conn_complete(uint8_t subevent, const void *data,
                                    unsigned int len)
{
    const struct ble_hci_ev_le_subev_enh_conn_complete *ev = data;
    struct ble_gap_conn_complete evt;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    memset(&evt, 0, sizeof(evt));

    evt.status = ev->status;

    if (evt.status == BLE_ERR_SUCCESS) {
        evt.connection_handle = le16toh(ev->conn_handle);
        evt.role = ev->role;
        evt.peer_addr_type = ev->peer_addr_type;
        memcpy(evt.peer_addr, ev->peer_addr, BLE_DEV_ADDR_LEN);
        memcpy(evt.local_rpa, ev->local_rpa, BLE_DEV_ADDR_LEN);
        memcpy(evt.peer_rpa,ev->peer_rpa, BLE_DEV_ADDR_LEN);

#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
        /* RPA needs to be resolved here, as controller is not aware of the
         * address is RPA in Host based RPA  */
        if (ble_host_rpa_enabled() && ((!memcmp(evt.local_rpa, ble_hs_conn_null_addr, 6)) == 0)) {
            uint8_t *local_id_rpa = ble_hs_get_rpa_local();
            memcpy(evt.local_rpa, local_id_rpa, BLE_DEV_ADDR_LEN);
        }

        struct ble_hs_resolv_entry *rl = NULL;
        ble_rpa_replace_peer_params_with_rl(evt.peer_addr,
                                            &evt.peer_addr_type, &rl);
#endif
        evt.conn_itvl = le16toh(ev->conn_itvl);
        evt.conn_latency = le16toh(ev->conn_latency);
        evt.supervision_timeout = le16toh(ev->supervision_timeout);
        evt.master_clk_acc = ev->mca;
    } else {
#if MYNEWT_VAL(BLE_HS_DEBUG)
        evt.connection_handle = BLE_HS_CONN_HANDLE_NONE;
#endif
    }
#if MYNEWT_VAL(BLE_EXT_ADV) && !MYNEWT_VAL(BLE_PERIODIC_ADV_WITH_RESPONSES)
    if (evt.status == BLE_ERR_DIR_ADV_TMO ||
                            evt.role == BLE_HCI_LE_CONN_COMPLETE_ROLE_SLAVE) {
    /* store this until we get set terminated event with adv handle */
        memcpy(&pend_conn_complete, &evt, sizeof(evt));
        return 0;
    }
#endif

#if MYNEWT_VAL(BLE_PERIODIC_ADV_WITH_RESPONSES)
    if (subevent == BLE_HCI_LE_SUBEV_ENH_CONN_COMPLETE) {
        evt.adv_handle = 0xFF;
        evt.sync_handle = 0xFF;
    } else {
        evt.adv_handle = ev->adv_handle;
        evt.sync_handle = ev->sync_handle;
    }
#endif
    return ble_gap_rx_conn_complete(&evt, 0);

}

static int
ble_hs_hci_evt_le_conn_complete(uint8_t subevent, const void *data,
                                unsigned int len)
{
    const struct ble_hci_ev_le_subev_conn_complete *ev = data;
    struct ble_gap_conn_complete evt;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    memset(&evt, 0, sizeof(evt));

    evt.status = ev->status;

    if (evt.status == BLE_ERR_SUCCESS) {
        evt.connection_handle = le16toh(ev->conn_handle);
        evt.role = ev->role;
        evt.peer_addr_type = ev->peer_addr_type;
        memcpy(evt.peer_addr, ev->peer_addr, BLE_DEV_ADDR_LEN);

#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
        /* RPA needs to be resolved here, as controller is not aware of the
         * address is RPA in Host based RPA  */
        if (ble_host_rpa_enabled()) {
            uint8_t *local_id_rpa = ble_hs_get_rpa_local();
            memcpy(evt.local_rpa, local_id_rpa, BLE_DEV_ADDR_LEN);
        }

        struct ble_hs_resolv_entry *rl = NULL;
        ble_rpa_replace_peer_params_with_rl(evt.peer_addr,
                                            &evt.peer_addr_type, &rl);
#endif
        evt.conn_itvl = le16toh(ev->conn_itvl);
        evt.conn_latency = le16toh(ev->conn_latency);
        evt.supervision_timeout = le16toh(ev->supervision_timeout);
        evt.master_clk_acc = ev->mca;
    } else {
#if MYNEWT_VAL(BLE_HS_DEBUG)
        evt.connection_handle = BLE_HS_CONN_HANDLE_NONE;
#endif
    }

#if MYNEWT_VAL(BLE_EXT_ADV)
    if (evt.status == BLE_ERR_DIR_ADV_TMO ||
                            evt.role == BLE_HCI_LE_CONN_COMPLETE_ROLE_SLAVE) {
    /* store this until we get set terminated event with adv handle */
        memcpy(&pend_conn_complete, &evt, sizeof(evt));
        return 0;
    }
#endif
    return ble_gap_rx_conn_complete(&evt, 0);
}
#endif

static int
ble_hs_hci_evt_le_adv_rpt_first_pass(const void *data, unsigned int len)
{
    const struct ble_hci_ev_le_subev_adv_rpt *ev = data;
    const struct adv_report *rpt;
    int i;

    if (len < sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    len -= sizeof(*ev);
    data += sizeof(*ev);

    if (ev->num_reports < BLE_HCI_LE_ADV_RPT_NUM_RPTS_MIN ||
        ev->num_reports > BLE_HCI_LE_ADV_RPT_NUM_RPTS_MAX) {
        return BLE_HS_EBADDATA;
    }

    for (i = 0; i < ev->num_reports; i++) {
        /* extra byte for RSSI after adv data */
        if (len < sizeof(*rpt) + 1) {
            return BLE_HS_ECONTROLLER;
        }

        rpt = data;

        if (rpt->data_len > len) {
            return BLE_HS_ECONTROLLER;
        }

        /* extra byte for RSSI after adv data */
        len -= sizeof(*rpt) + 1 + rpt->data_len;
        data += sizeof(*rpt) + 1 + rpt->data_len;
    }

    /* Make sure length was correct */
    if (len) {
        return BLE_HS_ECONTROLLER;
    }

    return 0;
}

static int
ble_hs_hci_evt_le_adv_rpt(uint8_t subevent, const void *data, unsigned int len)
{
    const struct ble_hci_ev_le_subev_adv_rpt *ev = data;
    struct ble_gap_disc_desc desc = {0};
    const struct adv_report *rpt;
    int rc;
    int i;

    /* Validate the event is formatted correctly */
    rc = ble_hs_hci_evt_le_adv_rpt_first_pass(data, len);
    if (rc != 0) {
        return rc;
    }

    data += sizeof(*ev);

    desc.direct_addr = *BLE_ADDR_ANY;

    /* BLE Queue Congestion check*/
#if MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
    if (ble_get_adv_list_length() > BLE_ADV_LIST_MAX_LENGTH || ble_adv_list_count > BLE_ADV_LIST_MAX_COUNT) {
        ble_adv_list_refresh();
    }
    ble_adv_list_count++;
#endif

    for (i = 0; i < ev->num_reports; i++) {

    /* Avoiding further processing, if the adv report is from the same device*/
#if MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
    if (ble_check_adv_list(ev->reports[i].addr, ev->reports[i].addr_type) == true) {
        continue;
    }
#endif
        rpt = data;

        data += sizeof(rpt) + rpt->data_len + 1;

        desc.event_type = rpt->type;
        desc.addr.type = rpt->addr_type;
        memcpy(desc.addr.val, rpt->addr, BLE_DEV_ADDR_LEN);

#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
    struct ble_hs_resolv_entry *rl = NULL;
    rl = ble_hs_resolv_rpa_addr(desc.addr.val, desc.addr.type);

    if (rl != NULL) {
        if(desc.addr.type == 1) {
           rl->rl_isrpa = 1;
        }
        memcpy(desc.addr.val, rl->rl_identity_addr, BLE_DEV_ADDR_LEN);
        desc.addr.type = rl->rl_addr_type;
    }
#endif

        desc.length_data = rpt->data_len;
        desc.data = rpt->data;
        desc.rssi = rpt->data[rpt->data_len];

        ble_gap_rx_adv_report(&desc);
    }

    return 0;
}

static int
ble_hs_hci_evt_le_dir_adv_rpt(uint8_t subevent, const void *data, unsigned int len)
{
    const struct ble_hci_ev_le_subev_direct_adv_rpt *ev = data;
    struct ble_gap_disc_desc desc = {0};
    int i;

    if (len < sizeof(*ev) || len != ev->num_reports * sizeof(ev->reports[0])) {
        return BLE_HS_ECONTROLLER;
    }

    /* Data fields not present in a direct advertising report. */
    desc.data = NULL;
    desc.length_data = 0;

    for (i = 0; i < ev->num_reports; i++) {
        desc.event_type = ev->reports[i].type;
        desc.addr.type = ev->reports[i].addr_type;
        memcpy(desc.addr.val, ev->reports[i].addr, BLE_DEV_ADDR_LEN);
        desc.direct_addr.type = ev->reports[i].dir_addr_type;
        memcpy(desc.direct_addr.val, ev->reports[i].dir_addr, BLE_DEV_ADDR_LEN);
        desc.rssi = ev->reports[i].rssi;

        ble_gap_rx_adv_report(&desc);
    }

    return 0;
}

static int
ble_hs_hci_evt_le_rd_rem_used_feat_complete(uint8_t subevent, const void *data,
                                            unsigned int len)
{
    const struct ble_hci_ev_le_subev_rd_rem_used_feat *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_rd_rem_sup_feat_complete(ev);

    return 0;
}

static int
ble_hs_hci_evt_rd_rem_ver_complete(uint8_t subevent, const void *data,
                                   unsigned int len)
{
    const struct ble_hci_ev_rd_rem_ver_info_cmp *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_rd_rem_ver_info_complete(ev);

    return 0;
}


#if MYNEWT_VAL(BLE_EXT_ADV) && NIMBLE_BLE_SCAN
static int
ble_hs_hci_decode_legacy_type(uint16_t evt_type)
{
    switch (evt_type) {
    case BLE_HCI_LEGACY_ADV_EVTYPE_ADV_IND:
        return BLE_HCI_ADV_RPT_EVTYPE_ADV_IND;
    case BLE_HCI_LEGACY_ADV_EVTYPE_ADV_DIRECT_IND:
        return BLE_HCI_ADV_RPT_EVTYPE_DIR_IND;
    case BLE_HCI_LEGACY_ADV_EVTYPE_ADV_SCAN_IND:
        return BLE_HCI_ADV_RPT_EVTYPE_SCAN_IND;
    case BLE_HCI_LEGACY_ADV_EVTYPE_ADV_NONCON_IND:
        return BLE_HCI_ADV_RPT_EVTYPE_NONCONN_IND;
    case BLE_HCI_LEGACY_ADV_EVTYPE_SCAN_RSP_ADV_IND:
    case BLE_HCI_LEGACY_ADV_EVTYPE_SCAN_RSP_ADV_SCAN_IND:
        return BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP;
    default:
        return -1;
    }
}

static int
ble_hs_hci_evt_le_ext_adv_rpt_first_pass(const void *data, unsigned int len)
{
    const struct ble_hci_ev_le_subev_ext_adv_rpt *ev = data;
    const struct ext_adv_report *report;
    int i;

    if (len < sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    len -= sizeof(*ev);
    data += sizeof(*ev);

    if (ev->num_reports < BLE_HCI_LE_ADV_RPT_NUM_RPTS_MIN ||
        ev->num_reports > BLE_HCI_LE_ADV_RPT_NUM_RPTS_MAX) {
        return BLE_HS_EBADDATA;
    }

    for (i = 0; i < ev->num_reports; i++) {
        if (len < sizeof(*report)) {
            return BLE_HS_ECONTROLLER;
        }

        report = data;

        if (report->data_len > len) {
            return BLE_HS_ECONTROLLER;
        }

        len -= sizeof(*report) + report->data_len;
        data += sizeof(*report) + report->data_len;
    }

    /* Make sure length was correct */
    if (len) {
        return BLE_HS_ECONTROLLER;
    }

    return 0;
}
#endif

static int
ble_hs_hci_evt_le_ext_adv_rpt(uint8_t subevent, const void *data,
                              unsigned int len)
{
#if MYNEWT_VAL(BLE_EXT_ADV) && NIMBLE_BLE_SCAN
    const struct ble_hci_ev_le_subev_ext_adv_rpt *ev = data;
    const struct ext_adv_report *report;
    struct ble_gap_ext_disc_desc desc;
    int legacy_event_type;
    int rc;
    int i;

    rc = ble_hs_hci_evt_le_ext_adv_rpt_first_pass(data, len);
    if (rc != 0) {
        return rc;
    }

    report = &ev->reports[0];
    for (i = 0; i < ev->num_reports; i++) {
        memset(&desc, 0, sizeof(desc));

        desc.props = (report->evt_type) & 0x1F;
        if (desc.props & BLE_HCI_ADV_LEGACY_MASK) {
            legacy_event_type = ble_hs_hci_decode_legacy_type(report->evt_type);
            if (legacy_event_type < 0) {
                report = (const void *) &report->data[report->data_len];
                continue;
            }
            desc.legacy_event_type = legacy_event_type;
            desc.data_status = BLE_GAP_EXT_ADV_DATA_STATUS_COMPLETE;
        } else {
            switch(report->evt_type & BLE_HCI_ADV_DATA_STATUS_MASK) {
            case BLE_HCI_ADV_DATA_STATUS_COMPLETE:
                desc.data_status = BLE_GAP_EXT_ADV_DATA_STATUS_COMPLETE;
                break;
            case BLE_HCI_ADV_DATA_STATUS_INCOMPLETE:
                desc.data_status = BLE_GAP_EXT_ADV_DATA_STATUS_INCOMPLETE;
                break;
            case BLE_HCI_ADV_DATA_STATUS_TRUNCATED:
                desc.data_status = BLE_GAP_EXT_ADV_DATA_STATUS_TRUNCATED;
                break;
            default:
                assert(false);
            }
        }
        desc.addr.type = report->addr_type;
        memcpy(desc.addr.val, report->addr, 6);
        desc.length_data = report->data_len;
        desc.data = report->data;
        desc.rssi = report->rssi;
        desc.tx_power = report->tx_power;
        memcpy(desc.direct_addr.val, report->dir_addr, 6);
        desc.direct_addr.type = report->dir_addr_type;
        desc.sid = report->sid;
        desc.prim_phy = report->pri_phy;
        desc.sec_phy = report->sec_phy;
        desc.periodic_adv_itvl = report->periodic_itvl;

        ble_gap_rx_ext_adv_report(&desc);

        report = (const void *) &report->data[report->data_len];
    }
#endif
    return 0;
}

static int
ble_hs_hci_evt_le_periodic_adv_sync_estab(uint8_t subevent, const void *data,
                                          unsigned int len)
{
#if MYNEWT_VAL(BLE_PERIODIC_ADV)
    const struct ble_hci_ev_le_subev_periodic_adv_sync_estab *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_peroidic_adv_sync_estab(ev);
#endif

    return 0;
}

static int
ble_hs_hci_evt_le_periodic_adv_rpt(uint8_t subevent, const void *data,
                                   unsigned int len)
{
#if MYNEWT_VAL(BLE_PERIODIC_ADV)
    const struct ble_hci_ev_le_subev_periodic_adv_rpt *ev = data;

    if (len < sizeof(*ev) || len != (sizeof(*ev) + ev->data_len)) {
        return BLE_HS_EBADDATA;
    }

    ble_gap_rx_periodic_adv_rpt(ev);
#endif

return 0;
}

static int
ble_hs_hci_evt_le_periodic_adv_sync_lost(uint8_t subevent, const void *data,
                                         unsigned int len)
{
#if MYNEWT_VAL(BLE_PERIODIC_ADV)
    const struct ble_hci_ev_le_subev_periodic_adv_sync_lost *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_EBADDATA;
    }

    ble_gap_rx_periodic_adv_sync_lost(ev);

#endif
    return 0;
}

#if MYNEWT_VAL(BLE_POWER_CONTROL)
static int
ble_hs_hci_evt_le_pathloss_threshold(uint8_t subevent, const void *data,
					     unsigned int len)
{
    const struct ble_hci_ev_le_subev_path_loss_threshold *ev = data;

    if (len != sizeof(*ev)) {
	return BLE_HS_EBADDATA;
    }

    ble_gap_rx_le_pathloss_threshold(ev);
    return 0;
}

static int
ble_hs_hci_evt_le_transmit_power_report(uint8_t subevent, const void *data,
					     unsigned int len)
{
    const struct ble_hci_ev_le_subev_transmit_power_report *ev = data;

    if (len != sizeof(*ev)) {
	return BLE_HS_EBADDATA;
    }

    ble_gap_rx_transmit_power_report(ev);
    return 0;
}
#endif

static int
ble_hs_hci_evt_le_periodic_adv_sync_transfer(uint8_t subevent, const void *data,
                                             unsigned int len)
{
#if MYNEWT_VAL(BLE_PERIODIC_ADV_SYNC_TRANSFER)
    const struct ble_hci_ev_le_subev_periodic_adv_sync_transfer *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_EBADDATA;
    }

    ble_gap_rx_periodic_adv_sync_transfer(ev);

#endif
    return 0;
}

#if MYNEWT_VAL(BLE_PERIODIC_ADV_SYNC_BIGINFO_REPORTS)
static int
ble_hs_hci_evt_le_biginfo_adv_report(uint8_t subevent, const void *data,
                                     unsigned int len)
{
    const struct ble_hci_ev_le_subev_biginfo_adv_report *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_EBADDATA;
    }

    ble_gap_rx_biginfo_adv_rpt(ev);

    return 0;
}
#endif

static int
ble_hs_hci_evt_le_scan_timeout(uint8_t subevent, const void *data,
                               unsigned int len)
{
#if MYNEWT_VAL(BLE_EXT_ADV) && NIMBLE_BLE_SCAN
    const struct ble_hci_ev_le_subev_scan_timeout *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_EBADDATA;
    }

    ble_gap_rx_le_scan_timeout();
#endif
    return 0;
}

static int
ble_hs_hci_evt_le_adv_set_terminated(uint8_t subevent, const void *data,
                                     unsigned int len)
{
#if MYNEWT_VAL(BLE_EXT_ADV)
    const struct ble_hci_ev_le_subev_adv_set_terminated *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    /* this indicates bug in controller as host uses instances from
     * 0-BLE_ADV_INSTANCES range only
     */
    if (ev->adv_handle >= BLE_ADV_INSTANCES) {
        return BLE_HS_ECONTROLLER;
    }

    if (ev->status == 0) {
        /* ignore return code as we need to terminate advertising set anyway */
        ble_gap_rx_conn_complete(&pend_conn_complete, ev->adv_handle);
    }
    ble_gap_rx_adv_set_terminated(ev);
#endif

    return 0;
}

static int
ble_hs_hci_evt_le_scan_req_rcvd(uint8_t subevent, const void *data,
                                unsigned int len)
{
#if MYNEWT_VAL(BLE_EXT_ADV)
    const struct ble_hci_ev_le_subev_scan_req_rcvd *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    /* this indicates bug in controller as host uses instances from
     * 0-BLE_ADV_INSTANCES range only
     */
    if (ev->adv_handle >= BLE_ADV_INSTANCES) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_scan_req_rcvd(ev);
#endif

    return 0;
}

#if MYNEWT_VAL(BLE_CONN_SUBRATING)
static int
ble_hs_hci_evt_le_subrate_change(uint8_t subevent, const void *data,
                                 unsigned int len)
{
    const struct ble_hci_ev_le_subev_subrate_change *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_subrate_change(ev);

    return 0;
}
#endif

#if MYNEWT_VAL(BLE_AOA_AOD)
static int
ble_hs_hci_evt_le_connless_iq_report(uint8_t subevent, const void *data,
                                 unsigned int len)
{
    const struct ble_hci_ev_le_subev_connless_iq_rpt *ev = data;

    if (len < sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_connless_iq_report(ev);

    return 0;
}

static int
ble_hs_hci_evt_le_conn_iq_report(uint8_t subevent, const void *data,
                                 unsigned int len)
{
    const struct ble_hci_ev_le_subev_conn_iq_rpt *ev = data;
    if (len < sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_conn_iq_report(ev);

    return 0;
}

static int
ble_hs_hci_evt_le_cte_req_failed(uint8_t subevent, const void *data,
                                 unsigned int len)
{
    const struct ble_hci_ev_le_subev_cte_req_failed *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_cte_req_failed(ev);

    return 0;
}
#endif

#if MYNEWT_VAL(BLE_PERIODIC_ADV_WITH_RESPONSES)
static int
ble_hs_hci_evt_le_periodic_adv_subev_data_req(uint8_t subevent, const void *data,
                                 unsigned int len)
{
    const struct ble_hci_ev_le_subev_periodic_adv_subev_data_req *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_periodic_adv_subev_data_req(ev);

    return 0;
}

static int
ble_hs_hci_evt_le_periodic_adv_subev_resp_rep(uint8_t subevent, const void *data, unsigned int len)
{
    const struct ble_hci_ev_le_subev_periodic_adv_resp_rep *ev = data;
    const struct periodic_adv_response *response;
    struct ble_gap_periodic_adv_response resp;
    uint32_t size;

    /* TODO: compare with the total length including the response data. */
    size = sizeof(*ev);
    for (uint8_t i = 0; i < ev->num_responses; i ++) {
        size += sizeof(struct periodic_adv_response) + ev->responses[i].data_length;
    }
    if (len < size) {
        return BLE_HS_ECONTROLLER;
    }

    len -= sizeof(*ev);
    data += sizeof(*ev);

    resp.adv_handle = ev->adv_handle;
    resp.subevent = ev->subevent;
    resp.tx_status = ev->tx_status;

    for (uint8_t i = 0; i < ev->num_responses; i++) {
        response = data;
        if (len < sizeof(*response) + response->data_length) {
            return BLE_HS_ECONTROLLER;
        }

        len -= sizeof(*response) + response->data_length;
        data += sizeof(*response) + response->data_length;

        resp.tx_power = response->tx_power;
        resp.rssi = response->rssi;
        resp.cte_type = response->cte_type;
        resp.response_slot = response->response_slot;
        resp.data_length = response->data_length;
        resp.data = response->data;
        resp.data_status = response->data_status;

        ble_gap_rx_periodic_adv_response(resp);
    }

    return 0;
}
#endif

#if NIMBLE_BLE_CONNECT
static int
ble_hs_hci_evt_le_conn_upd_complete(uint8_t subevent, const void *data,
                                    unsigned int len)
{
    const struct ble_hci_ev_le_subev_conn_upd_complete *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    if (ev->status == 0) {
        BLE_HS_DBG_ASSERT(le16toh(ev->conn_itvl) >= BLE_HCI_CONN_ITVL_MIN);
        BLE_HS_DBG_ASSERT(le16toh(ev->conn_itvl) <= BLE_HCI_CONN_ITVL_MAX);

        BLE_HS_DBG_ASSERT(le16toh(ev->conn_latency) >= BLE_HCI_CONN_LATENCY_MIN);
        BLE_HS_DBG_ASSERT(le16toh(ev->conn_latency) <= BLE_HCI_CONN_LATENCY_MAX);

        BLE_HS_DBG_ASSERT(le16toh(ev->supervision_timeout) >= BLE_HCI_CONN_SPVN_TIMEOUT_MIN);
        BLE_HS_DBG_ASSERT(le16toh(ev->supervision_timeout) <= BLE_HCI_CONN_SPVN_TIMEOUT_MAX);
    }

    ble_gap_rx_update_complete(ev);

    return 0;
}

static int
ble_hs_hci_evt_le_lt_key_req(uint8_t subevent, const void *data, unsigned int len)
{
    const struct ble_hci_ev_le_subev_lt_key_req *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_sm_ltk_req_rx(ev);

    return 0;
}

static int
ble_hs_hci_evt_le_conn_parm_req(uint8_t subevent, const void *data, unsigned int len)
{
    const struct ble_hci_ev_le_subev_rem_conn_param_req *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    BLE_HS_DBG_ASSERT(le16toh(ev->min_interval) >= BLE_HCI_CONN_ITVL_MIN);
    BLE_HS_DBG_ASSERT(le16toh(ev->max_interval) <= BLE_HCI_CONN_ITVL_MAX);
    BLE_HS_DBG_ASSERT(le16toh(ev->max_interval) >= le16toh(ev->min_interval));

    BLE_HS_DBG_ASSERT(le16toh(ev->latency) >= BLE_HCI_CONN_LATENCY_MIN);
    BLE_HS_DBG_ASSERT(le16toh(ev->latency) <= BLE_HCI_CONN_LATENCY_MAX);

    BLE_HS_DBG_ASSERT(le16toh(ev->timeout) >= BLE_HCI_CONN_SPVN_TIMEOUT_MIN);
    BLE_HS_DBG_ASSERT(le16toh(ev->timeout) <= BLE_HCI_CONN_SPVN_TIMEOUT_MAX);

    ble_gap_rx_param_req(ev);

    return 0;
}

static int
ble_hs_hci_evt_le_phy_update_complete(uint8_t subevent, const void *data,
                                      unsigned int len)
{
    const struct ble_hci_ev_le_subev_phy_update_complete *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_phy_update_complete(ev);

    return 0;
}

static int
ble_hs_hci_evt_le_data_len_change(uint8_t subevent, const void *data,
				  unsigned int len)
{
    const struct ble_hci_ev_le_subev_data_len_chg *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HS_ECONTROLLER;
    }

    ble_gap_rx_data_len_change(ev);

    return 0;

}
#endif

int
ble_hs_hci_evt_process(struct ble_hci_ev *ev)
{
    const struct ble_hs_hci_evt_dispatch_entry *entry;
    int rc;

    /* Count events received */
    STATS_INC(ble_hs_stats, hci_event);

    if(ev->opcode == BLE_HCI_EVCODE_COMMAND_COMPLETE) {
        /* Check if this Command complete has a parsable opcode */
        struct ble_hci_ev_command_complete *cmd_complete = (void *) ev->data;
        entry = ble_hs_hci_evt_dispatch_find(cmd_complete->opcode);
    }
    else {
         entry = ble_hs_hci_evt_dispatch_find(ev->opcode);
    }

    if (entry == NULL) {
        STATS_INC(ble_hs_stats, hci_unknown_event);
        rc = BLE_HS_ENOTSUP;
    } else {
#if !BLE_MONITOR
	/* Ignore NOCP for debug */
       if(ev->opcode != 0x13) {
           BLE_HS_LOG(DEBUG, "ble_hs_event_rx_hci_ev; opcode=0x%x ", ev->opcode);

	   /* For LE Meta, print subevent code */
           if(ev->opcode == 0x3e) {
              BLE_HS_LOG(DEBUG, "subevent: 0x%x", ev->data[0]);
           }

           BLE_HS_LOG(DEBUG, "\n");
        }
#endif
        rc = entry->cb(ev->opcode, ev->data, ev->length);
    }

    ble_transport_free((uint8_t *)ev);

    return rc;
}

/**
 * Called when a data packet is received from the controller.  This function
 * consumes the supplied mbuf, regardless of the outcome.
 *
 * @param om                    The incoming data packet, beginning with the
 *                                  HCI ACL data header.
 *
 * @return                      0 on success; nonzero on failure.
 */
int
ble_hs_hci_evt_acl_process(struct os_mbuf *om)
{
#if NIMBLE_BLE_CONNECT
    struct hci_data_hdr hci_hdr;
    struct ble_hs_conn *conn;
    ble_l2cap_rx_fn *rx_cb;
    uint16_t conn_handle;
    int reject_cid;
    int rc;

    rc = ble_hs_hci_util_data_hdr_strip(om, &hci_hdr);
    if (rc != 0) {
        goto err;
    }

#if (BLETEST_THROUGHPUT_TEST == 0)
#if !BLE_MONITOR
    BLE_HS_LOG(DEBUG, "ble_hs_hci_evt_acl_process(): conn_handle=%u pb=%x "
                      "len=%u data=",
               BLE_HCI_DATA_HANDLE(hci_hdr.hdh_handle_pb_bc),
               BLE_HCI_DATA_PB(hci_hdr.hdh_handle_pb_bc),
               hci_hdr.hdh_len);
    ble_hs_log_mbuf(om);
    BLE_HS_LOG(DEBUG, "\n");
#endif
#endif

    if (hci_hdr.hdh_len != OS_MBUF_PKTHDR(om)->omp_len) {
        rc = BLE_HS_EBADDATA;
        goto err;
    }

    conn_handle = BLE_HCI_DATA_HANDLE(hci_hdr.hdh_handle_pb_bc);

    ble_hs_lock();

    conn = ble_hs_conn_find(conn_handle);
    if (conn == NULL) {
        /* Peer not connected; quietly discard packet. */
        rc = BLE_HS_ENOTCONN;
        reject_cid = -1;
    } else {
        /* Forward ACL data to L2CAP. */
        rc = ble_l2cap_rx(conn, &hci_hdr, om, &rx_cb, &reject_cid);
        om = NULL;
    }

    ble_hs_unlock();

    switch (rc) {
    case 0:
        /* Final fragment received. */
        BLE_HS_DBG_ASSERT(rx_cb != NULL);
        rc = rx_cb(conn->bhc_rx_chan);
        ble_l2cap_remove_rx(conn, conn->bhc_rx_chan);
        break;

    case BLE_HS_EAGAIN:
        /* More fragments on the way. */
        break;

    default:
        if (reject_cid != -1) {
            ble_l2cap_sig_reject_invalid_cid_tx(conn_handle, 0, 0, reject_cid);
        }
        goto err;
    }

    return 0;

err:
    os_mbuf_free_chain(om);
    return rc;
#else
    return BLE_HS_ENOTSUP;
#endif
}

#if MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
struct ble_addr_list_entry
{
    ble_addr_t addr;
    SLIST_ENTRY(ble_addr_list_entry) next;
};

SLIST_HEAD(ble_device_list, ble_addr_list_entry) ble_adv_list;

void ble_adv_list_init(void)
{
    ble_npl_mutex_init(&adv_list_lock);

    SLIST_INIT(&ble_adv_list);
    ble_adv_list_count = 0;
}

void ble_adv_list_deinit(void)
{
    struct ble_addr_list_entry *device;
    struct ble_addr_list_entry *temp;

    ble_npl_mutex_pend(&adv_list_lock, BLE_NPL_TIME_FOREVER);

    SLIST_FOREACH_SAFE(device, &ble_adv_list, next, temp) {
        SLIST_REMOVE(&ble_adv_list, device, ble_addr_list_entry, next);
        free(device);
    }

    ble_npl_mutex_release(&adv_list_lock);

    ble_npl_mutex_deinit(&adv_list_lock);
}

void ble_adv_list_add_packet(void *data)
{
    struct ble_addr_list_entry *device;

    if (!data) {
        BLE_HS_LOG(ERROR, "%s data is NULL", __func__);
        return;
    }

    ble_npl_mutex_pend(&adv_list_lock, BLE_NPL_TIME_FOREVER);

    device = (struct ble_addr_list_entry *)data;
    SLIST_INSERT_HEAD(&ble_adv_list, device, next);

    ble_npl_mutex_release(&adv_list_lock);
}

uint32_t ble_get_adv_list_length(void)
{
    uint32_t length = 0;
    struct ble_addr_list_entry *device;

    SLIST_FOREACH(device, &ble_adv_list, next) {
        length++;
    }

    return length;
}

void ble_adv_list_refresh(void)
{
    struct ble_addr_list_entry *device;
    struct ble_addr_list_entry *temp;
    ble_adv_list_count = 0;

    if (SLIST_EMPTY(&ble_adv_list)) {
        BLE_HS_LOG(ERROR, "%s ble_adv_list is empty", __func__);
        return;
    }

    ble_npl_mutex_pend(&adv_list_lock, BLE_NPL_TIME_FOREVER);

    SLIST_FOREACH_SAFE(device, &ble_adv_list, next, temp) {
        SLIST_REMOVE(&ble_adv_list, device, ble_addr_list_entry, next);
        free(device);
    }

    ble_npl_mutex_release(&adv_list_lock);
}

bool ble_check_adv_list(const uint8_t *addr, uint8_t addr_type)
{
    struct ble_addr_list_entry *device;
    struct ble_addr_list_entry *adv_packet;
    bool found = false;

    if (!addr) {
        BLE_HS_LOG(ERROR, "%s addr is NULL", __func__);
        return found;
    }

    ble_npl_mutex_pend(&adv_list_lock, BLE_NPL_TIME_FOREVER);

    SLIST_FOREACH(device, &ble_adv_list, next) {
        if (!memcmp(addr, device->addr.val, BLE_DEV_ADDR_LEN) && device->addr.type == addr_type) {
            found = true;
            break;
        }
    }

    ble_npl_mutex_release(&adv_list_lock);

    if (!found) {
        adv_packet = nimble_platform_mem_malloc(sizeof(struct ble_addr_list_entry));
        if (adv_packet) {
            adv_packet->addr.type = addr_type;
            memcpy(adv_packet->addr.val, addr, BLE_DEV_ADDR_LEN);
            ble_adv_list_add_packet(adv_packet);
        } else {
            BLE_HS_LOG(ERROR, "%s adv_packet malloc failed", __func__);
        }
    }

    return found;
}
#endif
