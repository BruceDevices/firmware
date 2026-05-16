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
#ifndef ESP_PLATFORM

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/porting/nimble/include/os/os.h"
#include "nimble/nimble/include/nimble/ble.h"
#include "nimble/nimble/include/nimble/hci_common.h"
#include "nimble/nimble/controller/include/controller/ble_ll.h"
#include "nimble/nimble/controller/include/controller/ble_ll_utils.h"
#include "nimble/nimble/controller/include/controller/ble_ll_hci.h"
#include "nimble/nimble/controller/include/controller/ble_ll_conn.h"
#include "nimble/nimble/controller/include/controller/ble_ll_ctrl.h"
#include "nimble/nimble/controller/include/controller/ble_ll_scan.h"
#include "nimble/nimble/controller/include/controller/ble_ll_adv.h"
#include "ble_ll_conn_priv.h"

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL) || MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)

/*
 * Used to limit the rate at which we send the number of completed packets
 * event to the host. This is the os time at which we can send an event.
 */
static ble_npl_time_t g_ble_ll_last_num_comp_pkt_evt;
extern uint8_t *g_ble_ll_conn_comp_ev;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
static const uint8_t ble_ll_conn_create_valid_phy_mask = (
        BLE_HCI_LE_PHY_1M_PREF_MASK |
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_2M_PHY)
        BLE_HCI_LE_PHY_2M_PREF_MASK |
#endif
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
        BLE_HCI_LE_PHY_CODED_PREF_MASK |
#endif
        0);

static const uint8_t ble_ll_conn_create_required_phy_mask = (
        BLE_HCI_LE_PHY_1M_PREF_MASK |
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
        BLE_HCI_LE_PHY_CODED_PREF_MASK |
#endif
        0);
#endif

/**
 * Allocate an event to send a connection complete event when initiating
 *
 * @return int 0: success -1: failure
 */
static int
ble_ll_init_alloc_conn_comp_ev(void)
{
    int rc;
    uint8_t *evbuf;

    rc = 0;
    evbuf = g_ble_ll_conn_comp_ev;
    if (evbuf == NULL) {
        evbuf = ble_transport_alloc_evt(0);
        if (!evbuf) {
            rc = -1;
        } else {
            g_ble_ll_conn_comp_ev = evbuf;
        }
    }

    return rc;
}

/**
 * Called to check that the connection parameters are within range
 *
 * @param itvl_min
 * @param itvl_max
 * @param latency
 * @param spvn_tmo
 *
 * @return int BLE_ERR_INV_HCI_CMD_PARMS if invalid parameters, 0 otherwise
 */
int
ble_ll_conn_hci_chk_conn_params(uint16_t itvl_min, uint16_t itvl_max,
                                uint16_t latency, uint16_t spvn_tmo)
{
    uint32_t spvn_tmo_usecs;
    uint32_t min_spvn_tmo_usecs;

    if ((itvl_min > itvl_max) ||
        (itvl_min < BLE_HCI_CONN_ITVL_MIN) ||
        (itvl_max > BLE_HCI_CONN_ITVL_MAX) ||
        (latency > BLE_HCI_CONN_LATENCY_MAX) ||
        (spvn_tmo < BLE_HCI_CONN_SPVN_TIMEOUT_MIN) ||
        (spvn_tmo > BLE_HCI_CONN_SPVN_TIMEOUT_MAX)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /*
    * Supervision timeout (in msecs) must be more than:
    *  (1 + connLatency) * connIntervalMax * 1.25 msecs * 2.
    */
    spvn_tmo_usecs = spvn_tmo;
    spvn_tmo_usecs *= (BLE_HCI_CONN_SPVN_TMO_UNITS * 1000);
    min_spvn_tmo_usecs = (uint32_t)itvl_max * 2 * BLE_LL_CONN_ITVL_USECS;
    min_spvn_tmo_usecs *= (1 + latency);
    if (spvn_tmo_usecs <= min_spvn_tmo_usecs) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    return BLE_ERR_SUCCESS;
}

/**
 * Send a connection complete event
 *
 * @param status The BLE error code associated with the event
 */
void
ble_ll_conn_comp_event_send(struct ble_ll_conn_sm *connsm, uint8_t status,
                            uint8_t *evbuf, struct ble_ll_adv_sm *advsm)
{
    struct ble_hci_ev_le_subev_enh_conn_complete *enh_ev;
    struct ble_hci_ev_le_subev_conn_complete *ev;
    struct ble_hci_ev *hci_ev = (void *) evbuf;
    uint8_t *rpa = NULL;

    BLE_LL_ASSERT(evbuf);

    if (ble_ll_hci_is_le_event_enabled(BLE_HCI_LE_SUBEV_ENH_CONN_COMPLETE)) {
        hci_ev->opcode = BLE_HCI_EVCODE_LE_META;
        hci_ev->length = sizeof(*enh_ev);
        enh_ev = (void *) hci_ev->data;

        memset(enh_ev, 0, sizeof(*enh_ev));

        enh_ev->subev_code = BLE_HCI_LE_SUBEV_ENH_CONN_COMPLETE;
        enh_ev->status = status;

        if (connsm) {
            enh_ev->conn_handle = htole16(connsm->conn_handle);
            enh_ev->role = connsm->conn_role - 1;
            enh_ev->peer_addr_type = connsm->peer_addr_type;
            memcpy(enh_ev->peer_addr, connsm->peer_addr, BLE_DEV_ADDR_LEN);

            switch (connsm->conn_role) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
            case BLE_LL_CONN_ROLE_CENTRAL:
                if (connsm->inita_identity_used) {
                    /* We used identity address in CONNECT_IND which can be just
                     * fine if
                     * a) it was direct advertising we replied to and remote uses
                     *    its identity address in device privacy mode or IRK is all
                     *    zeros.
                     * b) peer uses RPA and this is first time we connect to him
                     */
                    rpa = NULL;
                } else  if (connsm->own_addr_type > BLE_HCI_ADV_OWN_ADDR_RANDOM) {
                    rpa = ble_ll_scan_get_local_rpa();
                } else {
                    rpa = NULL;
                }
                break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
            case BLE_LL_CONN_ROLE_PERIPHERAL:
                rpa = ble_ll_adv_get_local_rpa(advsm);
                break;
#endif
            default:
                BLE_LL_ASSERT(0);
                break;
            }

            if (rpa) {
                memcpy(enh_ev->local_rpa, rpa, BLE_DEV_ADDR_LEN);
            }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
            /* Adjust address type if peer address was resolved */
             if (connsm->peer_addr_resolved) {
                 enh_ev->peer_addr_type += 2;
             }
#endif

             if (enh_ev->peer_addr_type > BLE_HCI_CONN_PEER_ADDR_RANDOM) {
                 switch (connsm->conn_role) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
                 case BLE_LL_CONN_ROLE_CENTRAL:
                     rpa = ble_ll_scan_get_peer_rpa();
                     break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
                 case BLE_LL_CONN_ROLE_PERIPHERAL:
                     rpa = ble_ll_adv_get_peer_rpa(advsm);
                 break;
#endif
                 default:
                     BLE_LL_ASSERT(0);
                     break;
                 }

                 memcpy(enh_ev->peer_rpa, rpa, BLE_DEV_ADDR_LEN);
             }

            enh_ev->conn_itvl = htole16(connsm->conn_itvl);
            enh_ev->conn_latency = htole16(connsm->periph_latency);
            enh_ev->supervision_timeout = htole16(connsm->supervision_tmo);
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
            if (connsm->conn_role == BLE_LL_CONN_ROLE_PERIPHERAL) {
                enh_ev->mca = connsm->central_sca;
            }
#endif
        }

        ble_ll_hci_event_send(hci_ev);
        return;
    }

    if (ble_ll_hci_is_le_event_enabled(BLE_HCI_LE_SUBEV_CONN_COMPLETE)) {
        hci_ev->opcode = BLE_HCI_EVCODE_LE_META;
        hci_ev->length = sizeof(*ev);
        ev = (void *) hci_ev->data;

        memset(ev, 0, sizeof(*ev));

        ev->subev_code = BLE_HCI_LE_SUBEV_CONN_COMPLETE;
        ev->status = status;

        if (connsm) {
            ev->conn_handle = htole16(connsm->conn_handle);
            ev->role = connsm->conn_role - 1;
            ev->peer_addr_type = connsm->peer_addr_type;
            memcpy(ev->peer_addr, connsm->peer_addr, BLE_DEV_ADDR_LEN);
            ev->conn_itvl = htole16(connsm->conn_itvl);
            ev->conn_latency = htole16(connsm->periph_latency);
            ev->supervision_timeout = htole16(connsm->supervision_tmo);
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
            if (connsm->conn_role == BLE_LL_CONN_ROLE_PERIPHERAL) {
                ev->mca = connsm->central_sca;
            }
#endif
        }

        ble_ll_hci_event_send(hci_ev);
        return;
    }

    ble_transport_free(evbuf);
}

/**
 * Called to create and send the number of completed packets event to the
 * host.
 */
void
ble_ll_conn_num_comp_pkts_event_send(struct ble_ll_conn_sm *connsm)
{
    /** The maximum number of handles that will fit in an event buffer. */
    static const int max_handles =
            (BLE_LL_MAX_EVT_LEN - sizeof(struct ble_hci_ev_num_comp_pkts) - 1) / 4;
    struct ble_hci_ev_num_comp_pkts *ev;
    struct ble_hci_ev *hci_ev;
    int event_sent;

    if (connsm == NULL) {
        goto skip_conn;
    }

    /*
     * At some periodic rate, make sure we go through all active connections
     * and send the number of completed packet events. We do this mainly
     * because the spec says we must update the host even though no packets
     * have completed but there are data packets in the controller buffers
     * (i.e. enqueued in a connection state machine).
     */
    if ((ble_npl_stime_t)(ble_npl_time_get() - g_ble_ll_last_num_comp_pkt_evt) <
        ble_npl_time_ms_to_ticks32(MYNEWT_VAL(BLE_LL_NUM_COMP_PKT_ITVL_MS))) {
        /*
         * If this connection has completed packets, send an event right away.
         * We do this to increase throughput but we dont want to search the
         * entire active list every time.
         */
        if (connsm->completed_pkts) {
            hci_ev = ble_transport_alloc_evt(0);
            if (hci_ev) {
                hci_ev->opcode = BLE_HCI_EVCODE_NUM_COMP_PKTS;
                hci_ev->length = sizeof(*ev);
                ev = (void *)hci_ev->data;

                ev->count = 1;
                ev->completed[0].handle = htole16(connsm->conn_handle);
                ev->completed[0].packets = htole16(connsm->completed_pkts);
                hci_ev->length += sizeof(ev->completed[0]);

                connsm->completed_pkts = 0;

                ble_ll_hci_event_send(hci_ev);
            }
        }
        return;
    }

    /* Iterate through all the active, created connections */
skip_conn:
    hci_ev = NULL;
    ev = NULL;

    event_sent = 0;
    SLIST_FOREACH(connsm, &g_ble_ll_conn_active_list, act_sle) {
        /*
         * Only look at connections that we have sent a connection complete
         * event and that either has packets enqueued or has completed packets.
         */
        if ((connsm->conn_state != BLE_LL_CONN_STATE_IDLE) &&
            (connsm->completed_pkts || !STAILQ_EMPTY(&connsm->conn_txq))) {
            /* If no buffer, get one, If cant get one, leave. */
            if (!hci_ev) {
                hci_ev = ble_transport_alloc_evt(0);
                if (!hci_ev) {
                    break;
                }

                hci_ev->opcode = BLE_HCI_EVCODE_NUM_COMP_PKTS;
                hci_ev->length = sizeof(*ev);
                ev = (void *)hci_ev->data;

                ev->count = 0;
            }

            /* Add handle and complete packets */
            ev->completed[ev->count].handle = htole16(connsm->conn_handle);
            ev->completed[ev->count].packets = htole16(connsm->completed_pkts);
            hci_ev->length += sizeof(ev->completed[ev->count]);
            ev->count++;

            connsm->completed_pkts = 0;

            /* Send now if the buffer is full. */
            if (ev->count == max_handles) {
                ble_ll_hci_event_send(hci_ev);
                hci_ev = NULL;
                event_sent = 1;
            }
        }
    }

    /* Send event if there is an event to send */
    if (hci_ev) {
        ble_ll_hci_event_send(hci_ev);
        event_sent = 1;
    }

    if (event_sent) {
        g_ble_ll_last_num_comp_pkt_evt = ble_npl_time_get();
    }
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_PING)
/**
 * Send a authenticated payload timeout event
 *
 * NOTE: we currently only send this event when we have a reason to send it;
 * not when it fails.
 *
 * @param reason The BLE error code to send as a disconnect reason
 */
void
ble_ll_auth_pyld_tmo_event_send(struct ble_ll_conn_sm *connsm)
{
    struct ble_hci_ev_auth_pyld_tmo *ev;
    struct ble_hci_ev *hci_ev;

    if (ble_ll_hci_is_event_enabled(BLE_HCI_EVCODE_AUTH_PYLD_TMO)) {
        hci_ev = ble_transport_alloc_evt(0);
        if (hci_ev) {
            hci_ev->opcode = BLE_HCI_EVCODE_AUTH_PYLD_TMO;
            hci_ev->length = sizeof(*ev);

            ev = (void *) hci_ev->data;
            ev->conn_handle = htole16(connsm->conn_handle);

            ble_ll_hci_event_send(hci_ev);
        }
    }
}
#endif

/**
 * Send a disconnection complete event.
 *
 * NOTE: we currently only send this event when we have a reason to send it;
 * not when it fails.
 *
 * @param reason The BLE error code to send as a disconnect reason
 */
void
ble_ll_disconn_comp_event_send(struct ble_ll_conn_sm *connsm, uint8_t reason)
{
    struct ble_hci_ev_disconn_cmp *ev;
    struct ble_hci_ev *hci_ev;

    if (ble_ll_hci_is_event_enabled(BLE_HCI_EVCODE_DISCONN_CMP)) {
        hci_ev = ble_transport_alloc_evt(0);
        if (hci_ev) {
            hci_ev->opcode = BLE_HCI_EVCODE_DISCONN_CMP;
            hci_ev->length = sizeof(*ev);

            ev = (void *) hci_ev->data;

            ev->status = BLE_ERR_SUCCESS;
            ev->conn_handle = htole16(connsm->conn_handle);
            ev->reason = reason;

            ble_ll_hci_event_send(hci_ev);
        }
    }
}

int
ble_ll_conn_hci_create_check_scan(struct ble_ll_conn_create_scan *p)
{
    if (p->filter_policy > BLE_HCI_INITIATOR_FILT_POLICY_MAX) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if ((p->filter_policy == 0) &&
        (p->peer_addr_type > BLE_HCI_CONN_PEER_ADDR_MAX)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (p->own_addr_type > BLE_HCI_ADV_OWN_ADDR_MAX) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    if (p->init_phy_mask & ~ble_ll_conn_create_valid_phy_mask) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (!(p->init_phy_mask & ble_ll_conn_create_required_phy_mask)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }
#endif

    return 0;
}

static int
ble_ll_conn_hci_create_check_params(struct ble_ll_conn_create_params *cc_params)
{
    int rc;

    rc = ble_ll_conn_hci_chk_conn_params(cc_params->conn_itvl,
                                         cc_params->conn_itvl,
                                         cc_params->conn_latency,
                                         cc_params->supervision_timeout);
    if (rc) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (cc_params->min_ce_len > cc_params->max_ce_len) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Adjust min/max ce length to be less than interval
     * Note that interval is in 1.25ms and CE is in 625us
     */
    if (cc_params->min_ce_len > cc_params->conn_itvl * 2) {
        cc_params->min_ce_len = cc_params->conn_itvl * 2;
    }
    if (cc_params->max_ce_len > cc_params->conn_itvl * 2) {
        cc_params->max_ce_len = cc_params->conn_itvl * 2;
    }

    /* Precalculate conn interval */
    ble_ll_conn_itvl_to_ticks(cc_params->conn_itvl, &cc_params->conn_itvl_ticks,
                              &cc_params->conn_itvl_usecs);

    return 0;
}

/**
 * Process the HCI command to create a connection.
 *
 * Context: Link Layer task (HCI command processing)
 *
 * @param cmdbuf
 *
 * @return int
 */
int
ble_ll_conn_hci_create(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_create_conn_cp *cmd = (const void *) cmdbuf;
    struct ble_ll_conn_create_scan cc_scan;
    struct ble_ll_conn_create_params cc_params;
    struct ble_ll_conn_sm *connsm;
    uint16_t conn_itvl_min;
    uint16_t conn_itvl_max;
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    uint16_t css_slot_idx = 0;
#endif
    int rc;

    if (len < sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* If we are already creating a connection we should leave */
    if (g_ble_ll_conn_create_sm.connsm) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    /* If already enabled, we return an error */
    if (ble_ll_scan_enabled()) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    if (ble_ll_conn_find_by_peer_addr(cmd->peer_addr, cmd->peer_addr_type)) {
        return BLE_ERR_ACL_CONN_EXISTS;
    }

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    if (ble_ll_sched_css_is_enabled()) {
        css_slot_idx = ble_ll_conn_css_get_next_slot();
        if (css_slot_idx == BLE_LL_CONN_CSS_NO_SLOT) {
            return BLE_ERR_MEM_CAPACITY;
        }
    }
#endif

    cc_scan.own_addr_type = cmd->own_addr_type;
    cc_scan.filter_policy = cmd->filter_policy;
    if (cc_scan.filter_policy == 0) {
        cc_scan.peer_addr_type = cmd->peer_addr_type;
        memcpy(&cc_scan.peer_addr, cmd->peer_addr, BLE_DEV_ADDR_LEN);
    } else {
        cc_scan.peer_addr_type = 0;
        memset(&cc_scan.peer_addr, 0, BLE_DEV_ADDR_LEN);
    }
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    cc_scan.init_phy_mask = BLE_HCI_LE_PHY_1M_PREF_MASK;
#endif

    cc_scan.scan_params[PHY_UNCODED].itvl = le16toh(cmd->scan_itvl);
    cc_scan.scan_params[PHY_UNCODED].window = le16toh(cmd->scan_window);

    rc = ble_ll_conn_hci_create_check_scan(&cc_scan);
    if (rc) {
        return rc;
    }

    conn_itvl_min = le16toh(cmd->min_conn_itvl);
    conn_itvl_max = le16toh(cmd->max_conn_itvl);

    /* Check min/max interval here since generic check does not have min/max
     * parameters to check.
     */
    if (conn_itvl_min > conn_itvl_max) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    if (ble_ll_sched_css_is_enabled()) {
        cc_params.conn_itvl = ble_ll_sched_css_get_conn_interval_us();
        if ((cc_params.conn_itvl < conn_itvl_min) ||
            (cc_params.conn_itvl > conn_itvl_max)) {
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }
    } else {
        cc_params.conn_itvl = conn_itvl_max;
    }
#else
    cc_params.conn_itvl = conn_itvl_max;
#endif
    cc_params.conn_latency = le16toh(cmd->conn_latency);
    cc_params.supervision_timeout = le16toh(cmd->tmo);
    cc_params.min_ce_len = le16toh(cmd->min_ce);
    cc_params.max_ce_len = le16toh(cmd->max_ce);

    rc = ble_ll_conn_hci_create_check_params(&cc_params);
    if (rc) {
        return rc;
    }

    /* Make sure we can allocate an event to send the connection complete */
    if (ble_ll_init_alloc_conn_comp_ev()) {
        return BLE_ERR_MEM_CAPACITY;
    }

    /* Make sure we can accept a connection! */
    connsm = ble_ll_conn_sm_get();
    if (connsm == NULL) {
        return BLE_ERR_CONN_LIMIT;
    }

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    connsm->css_slot_idx = css_slot_idx;
    connsm->css_slot_idx_pending = BLE_LL_CONN_CSS_NO_SLOT;
#endif

    /* Initialize state machine in central role and start state machine */
    ble_ll_conn_central_init(connsm, &cc_scan, &cc_params);
    ble_ll_conn_sm_new(connsm);

    /* Start scanning */
    rc = ble_ll_scan_initiator_start(connsm, 0, &cc_scan);
    if (rc) {
        SLIST_REMOVE(&g_ble_ll_conn_active_list,connsm,ble_ll_conn_sm,act_sle);
        STAILQ_INSERT_TAIL(&g_ble_ll_conn_free_list, connsm, free_stqe);
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
        if (ble_ll_sched_css_is_enabled()) {
            SLIST_REMOVE(&g_ble_ll_conn_css_list, connsm, ble_ll_conn_sm, css_sle);
        }
#endif
    }

    return rc;
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
static void
ble_ll_conn_hci_ext_create_set_fb_params(uint8_t init_phy_mask,
                                         struct ble_ll_conn_create_params *cc_params_fb)
{
    if ((init_phy_mask & BLE_PHY_MASK_1M) == 0) {
        g_ble_ll_conn_create_sm.params[BLE_PHY_IDX_1M] = *cc_params_fb;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_2M_PHY)
    if ((init_phy_mask & BLE_PHY_MASK_2M) == 0) {
        g_ble_ll_conn_create_sm.params[BLE_PHY_IDX_2M] = *cc_params_fb;
    }
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    if ((init_phy_mask & BLE_PHY_MASK_CODED) == 0) {
        g_ble_ll_conn_create_sm.params[BLE_PHY_IDX_CODED] = *cc_params_fb;
    }
#endif
}

static int
ble_ll_conn_hci_ext_create_parse_params(const struct conn_params *params,
                                        uint8_t phy,
                                        struct ble_ll_conn_create_scan *cc_scan,
                                        struct ble_ll_conn_create_params *cc_params)
{
    uint16_t conn_itvl_min;
    uint16_t conn_itvl_max;
    uint16_t scan_itvl;
    uint16_t scan_window;
    int rc;

    conn_itvl_min = le16toh(params->conn_min_itvl);
    conn_itvl_max = le16toh(params->conn_max_itvl);

    /* Check min/max interval here since generic check does not have min/max
     * parameters to check.
     */
    if (conn_itvl_min > conn_itvl_max) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    if (ble_ll_sched_css_is_enabled()) {
        cc_params->conn_itvl = ble_ll_sched_css_get_conn_interval_us();
        if ((cc_params->conn_itvl < conn_itvl_min) ||
            (cc_params->conn_itvl > conn_itvl_max)) {
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }
    } else {
        cc_params->conn_itvl = conn_itvl_max;
    }
#else
    cc_params->conn_itvl = conn_itvl_max;
#endif
    cc_params->conn_latency = le16toh(params->conn_latency);
    cc_params->supervision_timeout = le16toh(params->supervision_timeout);
    cc_params->min_ce_len = le16toh(params->min_ce);
    cc_params->max_ce_len = le16toh(params->max_ce);

    rc = ble_ll_conn_hci_create_check_params(cc_params);
    if (rc) {
        return rc;
    }

    if (phy != BLE_PHY_2M) {
        scan_itvl = le16toh(params->scan_itvl);
        scan_window = le16toh(params->scan_window);

        if ((scan_itvl < BLE_HCI_SCAN_ITVL_MIN) ||
            (scan_itvl > BLE_HCI_SCAN_ITVL_MAX) ||
            (scan_window < BLE_HCI_SCAN_WINDOW_MIN) ||
            (scan_window > BLE_HCI_SCAN_WINDOW_MAX) ||
            (scan_itvl < scan_window)) {
                return BLE_ERR_INV_HCI_CMD_PARMS;
        }

        if (phy == BLE_PHY_1M) {
            cc_scan->scan_params[PHY_UNCODED].itvl = scan_itvl;
            cc_scan->scan_params[PHY_UNCODED].window = scan_window;
        } else {
            cc_scan->scan_params[PHY_CODED].itvl = scan_itvl;
            cc_scan->scan_params[PHY_CODED].window = scan_window;
        }
    }

    return 0;
}

int
ble_ll_conn_hci_ext_create(const uint8_t *cmdbuf, uint8_t len)
{
    static const struct init_phy {
        uint8_t idx;
        uint8_t mask;
        uint8_t phy;
    } init_phys[] = {
            {BLE_PHY_IDX_1M, BLE_PHY_MASK_1M, BLE_PHY_1M},
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_2M_PHY)
            {BLE_PHY_IDX_2M, BLE_PHY_MASK_2M, BLE_PHY_2M},
#endif
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
            {BLE_PHY_IDX_CODED, BLE_PHY_MASK_CODED, BLE_PHY_CODED},
#endif
    };

    const struct ble_hci_le_ext_create_conn_cp *cmd = (const void *)cmdbuf;
    const struct conn_params *params = cmd->conn_params;
    struct ble_ll_conn_create_scan cc_scan;
    struct ble_ll_conn_create_params *cc_params;
    struct ble_ll_conn_create_params *cc_params_fb;
    struct ble_ll_conn_sm *connsm;
    const struct init_phy *init_phy;
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    uint16_t css_slot_idx = 0;
#endif
    int rc;

    /* validate length */
    if (len < sizeof(*cmd) +
              __builtin_popcount(cmd->init_phy_mask) * sizeof(*params)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* If we are already creating a connection we should leave */
    if (g_ble_ll_conn_create_sm.connsm) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    /* If already enabled, we return an error */
    if (ble_ll_scan_enabled()) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    if (ble_ll_conn_find_by_peer_addr(cmd->peer_addr, cmd->peer_addr_type)) {
        return BLE_ERR_ACL_CONN_EXISTS;
    }

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    if (ble_ll_sched_css_is_enabled()) {
        css_slot_idx = ble_ll_conn_css_get_next_slot();
        if (css_slot_idx == BLE_LL_CONN_CSS_NO_SLOT) {
            return BLE_ERR_MEM_CAPACITY;
        }
    }
#endif

    cc_scan.own_addr_type = cmd->own_addr_type;
    cc_scan.filter_policy = cmd->filter_policy;
    if (cc_scan.filter_policy == 0) {
        cc_scan.peer_addr_type = cmd->peer_addr_type;
        memcpy(cc_scan.peer_addr, cmd->peer_addr, BLE_DEV_ADDR_LEN);
    } else {
        cc_scan.peer_addr_type = 0;
        memset(cc_scan.peer_addr, 0, BLE_DEV_ADDR_LEN);
    }
    cc_scan.init_phy_mask = cmd->init_phy_mask;

    rc = ble_ll_conn_hci_create_check_scan(&cc_scan);
    if (rc) {
        return rc;
    }

    cc_params_fb = NULL;

    for (unsigned int i = 0; i < ARRAY_SIZE(init_phys); i++) {
        init_phy = &init_phys[i];

        if ((cc_scan.init_phy_mask & init_phy->mask) == 0) {
            continue;
        }

        cc_params = &g_ble_ll_conn_create_sm.params[init_phy->idx];

        rc = ble_ll_conn_hci_ext_create_parse_params(params, init_phy->phy,
                                                     &cc_scan, cc_params);
        if (rc) {
            return rc;
        }

        if (!cc_params_fb) {
            cc_params_fb = cc_params;
        }
        params++;
    }

    ble_ll_conn_hci_ext_create_set_fb_params(cc_scan.init_phy_mask,
                                             cc_params_fb);

    /* Make sure we can allocate an event to send the connection complete */
    if (ble_ll_init_alloc_conn_comp_ev()) {
        return BLE_ERR_MEM_CAPACITY;
    }

    /* Make sure we can accept a connection! */
    connsm = ble_ll_conn_sm_get();
    if (connsm == NULL) {
        return BLE_ERR_CONN_LIMIT;
    }

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    connsm->css_slot_idx = css_slot_idx;
    connsm->css_slot_idx_pending = BLE_LL_CONN_CSS_NO_SLOT;
#endif

    /* Initialize state machine in central role and start state machine */
    ble_ll_conn_central_init(connsm, &cc_scan,
                             &g_ble_ll_conn_create_sm.params[0]);
    ble_ll_conn_sm_new(connsm);

    /* Start scanning */
    rc = ble_ll_scan_initiator_start(connsm, 1, &cc_scan);
    if (rc) {
        SLIST_REMOVE(&g_ble_ll_conn_active_list,connsm,ble_ll_conn_sm,act_sle);
        STAILQ_INSERT_TAIL(&g_ble_ll_conn_free_list, connsm, free_stqe);
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
        if (ble_ll_sched_css_is_enabled()) {
            SLIST_REMOVE(&g_ble_ll_conn_css_list, connsm, ble_ll_conn_sm, css_sle);
        }
#endif
    }

    return rc;
}
#endif

static int
ble_ll_conn_process_conn_params(const struct ble_hci_le_rem_conn_param_rr_cp *cmd,
                                struct ble_ll_conn_sm *connsm)
{
    int rc;
    struct hci_conn_update *hcu;

    /* Retrieve command data */
    hcu = &connsm->conn_param_req;
    hcu->handle = connsm->conn_handle;

    BLE_LL_ASSERT(connsm->conn_handle == le16toh(cmd->conn_handle));

    hcu->conn_itvl_min = le16toh(cmd->conn_itvl_min);
    hcu->conn_itvl_max = le16toh(cmd->conn_itvl_max);
    hcu->conn_latency = le16toh(cmd->conn_latency);
    hcu->supervision_timeout = le16toh(cmd->supervision_timeout);
    hcu->min_ce_len = le16toh(cmd->min_ce);
    hcu->max_ce_len = le16toh(cmd->max_ce);

    /* Check that parameter values are in range */
    rc = ble_ll_conn_hci_chk_conn_params(hcu->conn_itvl_min,
                                         hcu->conn_itvl_max,
                                         hcu->conn_latency,
                                         hcu->supervision_timeout);

    /* Check valid min/max ce length */
    if (rc || (hcu->min_ce_len > hcu->max_ce_len)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }
    return rc;
}

/**
 * Called when the host issues the read remote features command
 *
 * @param cmdbuf
 *
 * @return int
 */
int
ble_ll_conn_hci_read_rem_features(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_rd_rem_feat_cp *cmd = (const void *) cmdbuf;
    struct ble_ll_conn_sm *connsm;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* If no connection handle exit with error */
    connsm = ble_ll_conn_find_by_handle(le16toh(cmd->conn_handle));
    if (!connsm) {
        return BLE_ERR_UNK_CONN_ID;
    }

    /* If already pending exit with error */
    if (connsm->flags.features_host_req) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    /*
     * Start control procedure if we did not receive peer's features and did not
     * start procedure already.
     */
    if (!connsm->flags.features_rxd &&
        !IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_FEATURE_XCHG)) {
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
        if ((connsm->conn_role == BLE_LL_CONN_ROLE_PERIPHERAL) &&
            !(ble_ll_read_supp_features() & BLE_LL_FEAT_PERIPH_INIT)) {
                return BLE_ERR_CMD_DISALLOWED;
        }
#endif

        ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_FEATURE_XCHG, NULL);
    }

    connsm->flags.features_host_req = 1;

    return BLE_ERR_SUCCESS;
}

static bool
ble_ll_conn_params_in_range(struct ble_ll_conn_sm *connsm,
                            const struct ble_hci_le_conn_update_cp *cmd)
{
    if (!IN_RANGE(connsm->conn_itvl, le16toh(cmd->conn_itvl_min),
                  le16toh(cmd->conn_itvl_max))) {
        return false;
    }

    if (connsm->periph_latency != le16toh(cmd->conn_latency)) {
        return false;
    }

    if (connsm->supervision_tmo != le16toh(cmd->supervision_timeout)) {
        return false;
    }

    return true;
}

static void
ble_ll_conn_hci_update_complete_event(void *user_data)
{
    struct ble_ll_conn_sm *connsm = user_data;

    ble_ll_hci_ev_conn_update(connsm, BLE_ERR_SUCCESS);
}

/**
 * Called to process a connection update command.
 *
 * @param cmdbuf
 *
 * @return int
 */
int
ble_ll_conn_hci_update(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_conn_update_cp *cmd = (const void *) cmdbuf;
    int rc;
    uint8_t ctrl_proc;
    uint16_t handle;
    struct ble_ll_conn_sm *connsm;
    struct hci_conn_update *hcu;
    uint16_t max_ce_len;

    /*
     * XXX: must deal with peripheral not supporting this feature and using
     * conn update! Right now, we only check if WE support the connection
     * parameters request procedure. We dont check if the remote does.
     * We should also be able to deal with sending the parameter request,
     * getting an UNKOWN_RSP ctrl pdu and resorting to use normal
     * connection update procedure.
     */

    /* If no connection handle exit with error */
    handle = le16toh(cmd->conn_handle);
    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        return BLE_ERR_UNK_CONN_ID;
    }

    /* if current connection parameters fit updated values we don't have to
     * trigger LL procedure and just update local values (Min/Max CE Length)
     *
     * Note that this is also allowed for CSS as nothing changes WRT connections
     * scheduling.
     */
    if (ble_ll_conn_params_in_range(connsm, cmd)) {
        max_ce_len = le16toh(cmd->max_ce_len);

        if (le16toh(cmd->min_ce_len) > max_ce_len) {
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }

        connsm->max_ce_len_ticks = ble_ll_tmr_u2t_up(max_ce_len * BLE_LL_CONN_CE_USECS);

        ble_ll_hci_post_cmd_cb_set(ble_ll_conn_hci_update_complete_event, connsm);

        return BLE_ERR_SUCCESS;
    }

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    /* Do not allow connection update if css in enabled, we only allow to move
     * anchor point (i.e. change slot) via dedicated HCI command.
     */
    if (ble_ll_sched_css_is_enabled() &&
        connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
        return BLE_ERR_CMD_DISALLOWED;
    }
#endif

    /* Better not have this procedure ongoing! */
    if (IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_CONN_PARAM_REQ) ||
        IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_CONN_UPDATE)) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    /* See if this feature is supported on both sides */
    if ((connsm->conn_features & BLE_LL_FEAT_CONN_PARM_REQ) == 0) {
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
        if (connsm->conn_role == BLE_LL_CONN_ROLE_PERIPHERAL) {
            return BLE_ERR_UNSUPP_REM_FEATURE;
        }
#endif
        ctrl_proc = BLE_LL_CTRL_PROC_CONN_UPDATE;
    } else {
        ctrl_proc = BLE_LL_CTRL_PROC_CONN_PARAM_REQ;
    }

    /*
     * If we are a peripheral and the central has initiated the procedure already
     * we should deny the peripheral request for now. If we are a central and the
     * peripheral has initiated the procedure, we need to send a reject to the
     * peripheral.
     */
    if (connsm->flags.conn_update_host_w4reply) {
        switch (connsm->conn_role) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
        case BLE_LL_CONN_ROLE_CENTRAL:
            connsm->flags.conn_update_host_w4reply = 0;

            /* XXX: If this fails no reject ind will be sent! */
            ble_ll_ctrl_reject_ind_send(connsm, connsm->host_reply_opcode,
                                        BLE_ERR_LMP_COLLISION);
            break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
        case BLE_LL_CONN_ROLE_PERIPHERAL:
            return BLE_ERR_LMP_COLLISION;
        break;
#endif
        default:
            BLE_LL_ASSERT(0);
            break;
        }
    }

    /*
     * If we are a peripheral and the central has initiated the channel map
     * update procedure we should deny the peripheral request for now.
     */
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    if (connsm->flags.chanmap_update_sched) {
        if (connsm->conn_role == BLE_LL_CONN_ROLE_PERIPHERAL) {
            return BLE_ERR_DIFF_TRANS_COLL;
        }
    }
#endif

    /* Retrieve command data */
    hcu = &connsm->conn_param_req;
    hcu->conn_itvl_min = le16toh(cmd->conn_itvl_min);
    hcu->conn_itvl_max = le16toh(cmd->conn_itvl_max);
    hcu->conn_latency = le16toh(cmd->conn_latency);
    hcu->supervision_timeout = le16toh(cmd->supervision_timeout);
    hcu->min_ce_len = le16toh(cmd->min_ce_len);
    hcu->max_ce_len = le16toh(cmd->max_ce_len);
    if (hcu->min_ce_len > hcu->max_ce_len) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check that parameter values are in range */
    rc = ble_ll_conn_hci_chk_conn_params(hcu->conn_itvl_min,
                                         hcu->conn_itvl_max,
                                         hcu->conn_latency,
                                         hcu->supervision_timeout);
    if (!rc) {
        hcu->handle = handle;

        /* Start the control procedure */
        ble_ll_ctrl_proc_start(connsm, ctrl_proc, NULL);
    }

    return rc;
}

int
ble_ll_conn_hci_param_rr(const uint8_t *cmdbuf, uint8_t len,
                         uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_rem_conn_param_rr_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_le_rem_conn_param_rr_rp *rsp = (void *) rspbuf;
    int rc;
    uint8_t *dptr;
    uint8_t rsp_opcode;
    uint16_t handle;
    struct os_mbuf *om;
    struct ble_ll_conn_sm *connsm;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    handle = le16toh(cmd->conn_handle);

    /* See if we support this feature */
    if ((ble_ll_read_supp_features() & BLE_LL_FEAT_CONN_PARM_REQ) == 0) {
        rc = BLE_ERR_UNKNOWN_HCI_CMD;
        goto done;
    }

    /* If we dont have a handle we cant do anything */
    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        rc = BLE_ERR_UNK_CONN_ID;
        goto done;
    }

    /* Make sure connection parameters are valid */
    rc = ble_ll_conn_process_conn_params(cmd, connsm);

    /* The connection should be awaiting a reply. If not, just discard */
    if (connsm->flags.conn_update_host_w4reply) {
        /* Get a control packet buffer */
        if (rc == BLE_ERR_SUCCESS) {
            om = os_msys_get_pkthdr(BLE_LL_CTRL_MAX_PDU_LEN,
                                    sizeof(struct ble_mbuf_hdr));
            if (om) {
                dptr = om->om_data;
                rsp_opcode = ble_ll_ctrl_conn_param_reply(connsm, dptr,
                                                          &connsm->conn_cp);
                dptr[0] = rsp_opcode;
                len = g_ble_ll_ctrl_pkt_lengths[rsp_opcode] + 1;
                ble_ll_conn_enqueue_pkt(connsm, om, BLE_LL_LLID_CTRL, len);
            }
        } else {
            /* XXX: check return code and deal */
            ble_ll_ctrl_reject_ind_send(connsm, connsm->host_reply_opcode,
                                        BLE_ERR_CONN_PARMS);
        }
        connsm->flags.conn_update_host_w4reply = 0;

        /* XXX: if we cant get a buffer, what do we do? We need to remember
         * reason if it was a negative reply. We also would need to have
         * some state to tell us this happened
         */
    }

done:
    rsp->conn_handle = htole16(handle);

    *rsplen = sizeof(*rsp);
    return rc;
}

int
ble_ll_conn_hci_param_nrr(const uint8_t *cmdbuf, uint8_t len,
                          uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_rem_conn_params_nrr_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_le_rem_conn_params_nrr_rp *rsp = (void *) rspbuf;
    struct ble_ll_conn_sm *connsm;
    uint16_t handle;
    int rc;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    handle = le16toh(cmd->conn_handle);

    /* See if we support this feature */
    if ((ble_ll_read_supp_features() & BLE_LL_FEAT_CONN_PARM_REQ) == 0) {
        rc = BLE_ERR_UNKNOWN_HCI_CMD;
        goto done;
    }

    /* If we dont have a handle we cant do anything */
    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        rc = BLE_ERR_UNK_CONN_ID;
        goto done;
    }

    rc = BLE_ERR_SUCCESS;

    /* The connection should be awaiting a reply. If not, just discard */
    if (connsm->flags.conn_update_host_w4reply) {
        /* XXX: check return code and deal */
        ble_ll_ctrl_reject_ind_send(connsm, connsm->host_reply_opcode,
                                    cmd->reason);
        connsm->flags.conn_update_host_w4reply = 0;

        /* XXX: if we cant get a buffer, what do we do? We need to remember
         * reason if it was a negative reply. We also would need to have
         * some state to tell us this happened
         */
    }

done:
    rsp->conn_handle = htole16(handle);

    *rsplen = sizeof(*rsp);
    return rc;
}

/* this is called from same context after cmd complete is send so it is
 * safe to use g_ble_ll_conn_comp_ev
 */
static void
ble_ll_conn_hci_cancel_conn_complete_event(void *user_data)
{
    BLE_LL_ASSERT(g_ble_ll_conn_comp_ev);

    ble_ll_conn_comp_event_send(NULL, BLE_ERR_UNK_CONN_ID,
                                g_ble_ll_conn_comp_ev, NULL);
    g_ble_ll_conn_comp_ev = NULL;
}

/**
 * Called when HCI command to cancel a create connection command has been
 * received.
 *
 * Context: Link Layer (HCI command parser)
 *
 * @return int
 */
int
ble_ll_conn_create_cancel(void)
{
    int rc;
    struct ble_ll_conn_sm *connsm;
    os_sr_t sr;

    /*
     * If we receive this command and we have not got a connection
     * create command, we have to return disallowed. The spec does not say
     * what happens if the connection has already been established. We
     * return disallowed as well
     */
    OS_ENTER_CRITICAL(sr);
    connsm = g_ble_ll_conn_create_sm.connsm;
    if (connsm && (connsm->conn_state == BLE_LL_CONN_STATE_IDLE)) {
        /* stop scanning and end the connection event */
        g_ble_ll_conn_create_sm.connsm = NULL;
        ble_ll_scan_sm_stop(1);
        ble_ll_conn_end(connsm, BLE_ERR_UNK_CONN_ID);

        ble_ll_hci_post_cmd_cb_set(ble_ll_conn_hci_cancel_conn_complete_event, NULL);

        rc = BLE_ERR_SUCCESS;
    } else {
        /* If we are not attempting to create a connection*/
        rc = BLE_ERR_CMD_DISALLOWED;
    }
    OS_EXIT_CRITICAL(sr);

    return rc;
}

/**
 * Called to process a HCI disconnect command
 *
 * Context: Link Layer task (HCI command parser).
 *
 * @param cmdbuf
 *
 * @return int
 */
int
ble_ll_conn_hci_disconnect_cmd(const struct ble_hci_lc_disconnect_cp *cmd)
{
    int rc;
    uint16_t handle;
    struct ble_ll_conn_sm *connsm;

    /* Check for valid parameters */
    handle = le16toh(cmd->conn_handle);

    rc = BLE_ERR_INV_HCI_CMD_PARMS;
    if (handle <= BLE_LL_CONN_MAX_CONN_HANDLE) {
        /* Make sure reason is valid */
        switch (cmd->reason) {
        case BLE_ERR_AUTH_FAIL:
        case BLE_ERR_REM_USER_CONN_TERM:
        case BLE_ERR_RD_CONN_TERM_RESRCS:
        case BLE_ERR_RD_CONN_TERM_PWROFF:
        case BLE_ERR_UNSUPP_REM_FEATURE:
        case BLE_ERR_UNIT_KEY_PAIRING:
        case BLE_ERR_CONN_PARMS:
            connsm = ble_ll_conn_find_by_handle(handle);
            if (connsm) {
                /* Do not allow command if we are in process of disconnecting */
                if (connsm->disconnect_reason) {
                    rc = BLE_ERR_CMD_DISALLOWED;
                } else {
                    /* This control procedure better not be pending! */
                    BLE_LL_ASSERT(connsm->flags.terminate_started == 0);

                    /* Record the disconnect reason */
                    connsm->disconnect_reason = cmd->reason;

                    /* Start this control procedure */
                    ble_ll_ctrl_terminate_start(connsm);

                    rc = BLE_ERR_SUCCESS;
                }
            } else {
                rc = BLE_ERR_UNK_CONN_ID;
            }
            break;
        default:
            break;
        }
    }

    return rc;
}

/**
 * Called to process a HCI disconnect command
 *
 * Context: Link Layer task (HCI command parser).
 *
 * @param cmdbuf
 *
 * @return int
 */
int
ble_ll_conn_hci_rd_rem_ver_cmd(const uint8_t *cmdbuf, uint8_t len)
{
    struct ble_ll_conn_sm *connsm;
    const struct ble_hci_rd_rem_ver_info_cp *cmd = (const void *) cmdbuf;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check for valid parameters */
    connsm = ble_ll_conn_find_by_handle(le16toh(cmd->conn_handle));
    if (!connsm) {
        return BLE_ERR_UNK_CONN_ID;
    }

    /* Return error if in progress */
    if (IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_VERSION_XCHG)) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    /*
     * Start this control procedure. If we have already done this control
     * procedure we set the pending bit so that the host gets an event because
     * it is obviously expecting one (or would not have sent the command).
     * NOTE: we cant just send the event here. That would cause the event to
     * be queued before the command status.
     */
    if (!connsm->flags.version_ind_txd) {
        ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_VERSION_XCHG, NULL);
    } else {
        connsm->pending_ctrl_procs |= (1 << BLE_LL_CTRL_PROC_VERSION_XCHG);
    }

    return BLE_ERR_SUCCESS;
}

/**
 * Called to read the RSSI for a given connection handle
 *
 * @param cmdbuf
 * @param rspbuf
 * @param rsplen
 *
 * @return int
 */
int
ble_ll_conn_hci_rd_rssi(const uint8_t *cmdbuf, uint8_t len, uint8_t *rspbuf, uint8_t *rsplen)
{

    const struct ble_hci_rd_rssi_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_rd_rssi_rp *rsp = (void *) rspbuf;
    struct ble_ll_conn_sm *connsm;
    int rc;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    rsp->handle = cmd->handle;

    connsm = ble_ll_conn_find_by_handle(le16toh(cmd->handle));
    if (!connsm) {
        rsp->rssi = 127;
        rc = BLE_ERR_UNK_CONN_ID;
    } else {
        rsp->rssi = connsm->conn_rssi;
        rc = BLE_ERR_SUCCESS;
    }

    *rsplen = sizeof(*rsp);
    return rc;
}

/**
 * Called to read the current channel map of a connection
 *
 * @param cmdbuf
 * @param rspbuf
 * @param rsplen
 *
 * @return int
 */
int
ble_ll_conn_hci_rd_chan_map(const uint8_t *cmdbuf, uint8_t len,
                            uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_rd_chan_map_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_le_rd_chan_map_rp *rsp = (void *) rspbuf;
    struct ble_ll_conn_sm *connsm;
    uint16_t handle;
    int rc;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    handle = le16toh(cmd->conn_handle);
    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        rc = BLE_ERR_UNK_CONN_ID;
        memset(rsp->chan_map, 0, sizeof(rsp->chan_map));
    } else {
        if (connsm->flags.chanmap_update_sched) {
            memcpy(rsp->chan_map, connsm->req_chanmap, BLE_LL_CHAN_MAP_LEN);
        } else {
            memcpy(rsp->chan_map, connsm->chan_map, BLE_LL_CHAN_MAP_LEN);
        }
        rc = BLE_ERR_SUCCESS;
    }

    rsp->conn_handle = htole16(handle);

    *rsplen = sizeof(*rsp);
    return rc;
}
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_DATA_LEN_EXT)
int
ble_ll_conn_hci_set_data_len(const uint8_t *cmdbuf, uint8_t len,
                             uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_set_data_len_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_le_set_data_len_rp *rsp = (void *) rspbuf;
    int rc;
    uint16_t handle;
    uint16_t tx_octets;
    uint16_t tx_time;
    struct ble_ll_conn_sm *connsm;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Find connection */
    handle = le16toh(cmd->conn_handle);
    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        rc = BLE_ERR_UNK_CONN_ID;
        goto done;
    }

    tx_octets = le16toh(cmd->tx_octets);
    tx_time = le16toh(cmd->tx_time);

    if (!ble_ll_hci_check_dle(tx_octets, tx_time)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    rc = ble_ll_conn_set_data_len(connsm, tx_octets, tx_time, 0, 0);

done:
    rsp->conn_handle = htole16(handle);
    *rsplen = sizeof(*rsp);

    return rc;
}
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
/**
 * LE start encrypt command
 *
 * @param cmdbuf
 *
 * @return int
 */
int
ble_ll_conn_hci_le_start_encrypt(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_start_encrypt_cp *cmd = (const void *) cmdbuf;
    struct ble_ll_conn_sm *connsm;
    int rc;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    connsm = ble_ll_conn_find_by_handle(le16toh(cmd->conn_handle));
    if (!connsm) {
        rc = BLE_ERR_UNK_CONN_ID;
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    } else if (connsm->conn_role == BLE_LL_CONN_ROLE_PERIPHERAL) {
        rc = BLE_ERR_UNSPECIFIED;
#endif
    } else if (connsm->cur_ctrl_proc == BLE_LL_CTRL_PROC_ENCRYPT) {
        /*
         * The specification does not say what to do here but the host should
         * not be telling us to start encryption while we are in the process
         * of honoring a previous start encrypt.
         */
        rc = BLE_ERR_CMD_DISALLOWED;
    } else {
        /* Start the control procedure */
        connsm->enc_data.host_rand_num = le64toh(cmd->rand);
        connsm->enc_data.enc_div = le16toh(cmd->div);
        swap_buf(connsm->enc_data.enc_block.key, cmd->ltk, 16);
        ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_ENCRYPT, NULL);
        rc = BLE_ERR_SUCCESS;
    }

    return rc;
}

/**
 * Called to process the LE long term key reply.
 *
 * Context: Link Layer Task.
 *
 * @param cmdbuf
 * @param rspbuf
 * @param ocf
 *
 * @return int
 */
int
ble_ll_conn_hci_le_ltk_reply(const uint8_t *cmdbuf, uint8_t len,
                             uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_lt_key_req_reply_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_le_lt_key_req_reply_rp *rsp = (void *) rspbuf;
    struct ble_ll_conn_sm *connsm;
    uint16_t handle;
    int rc;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Find connection handle */
    handle = le16toh(cmd->conn_handle);
    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        rc = BLE_ERR_UNK_CONN_ID;
        goto ltk_key_cmd_complete;
    }

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    /* Should never get this if we are a central! */
    if (connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
        rc = BLE_ERR_UNSPECIFIED;
        goto ltk_key_cmd_complete;
    }
#endif

    /* The connection should be awaiting a reply. If not, just discard */
    if (connsm->enc_data.enc_state != CONN_ENC_S_LTK_REQ_WAIT) {
        rc = BLE_ERR_CMD_DISALLOWED;
        goto ltk_key_cmd_complete;
    }

    swap_buf(connsm->enc_data.enc_block.key, cmd->ltk, 16);
    ble_ll_calc_session_key(connsm);
    ble_ll_ctrl_start_enc_send(connsm);
    rc = BLE_ERR_SUCCESS;

ltk_key_cmd_complete:
    rsp->conn_handle = htole16(handle);

    *rsplen = sizeof(*rsp);
    return rc;
}

/**
 * Called to process the LE long term key negative reply.
 *
 * Context: Link Layer Task.
 *
 * @param cmdbuf
 * @param rspbuf
 * @param ocf
 *
 * @return int
 */
int
ble_ll_conn_hci_le_ltk_neg_reply(const uint8_t *cmdbuf, uint8_t len,
                                 uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_lt_key_req_neg_reply_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_le_lt_key_req_neg_reply_rp *rsp = (void *) rspbuf;
    struct ble_ll_conn_sm *connsm;
    uint16_t handle;
    int rc;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Find connection handle */
    handle = le16toh(cmd->conn_handle);
    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        rc = BLE_ERR_UNK_CONN_ID;
        goto ltk_key_cmd_complete;
    }

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    /* Should never get this if we are a central! */
    if (connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
        rc = BLE_ERR_UNSPECIFIED;
        goto ltk_key_cmd_complete;
    }
#endif

    /* The connection should be awaiting a reply. If not, just discard */
    if (connsm->enc_data.enc_state != CONN_ENC_S_LTK_REQ_WAIT) {
        rc = BLE_ERR_CMD_DISALLOWED;
        goto ltk_key_cmd_complete;
    }

    /* We received a negative reply! Send REJECT_IND */
    ble_ll_ctrl_reject_ind_send(connsm, BLE_LL_CTRL_ENC_REQ,
                                BLE_ERR_PINKEY_MISSING);
    connsm->enc_data.enc_state = CONN_ENC_S_LTK_NEG_REPLY;

    rc = BLE_ERR_SUCCESS;

ltk_key_cmd_complete:
    rsp->conn_handle = htole16(handle);

    *rsplen = sizeof(*rsp);
    return rc;
}
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_SCA_UPDATE)
int
ble_ll_conn_req_peer_sca(const uint8_t *cmdbuf, uint8_t len,
                         uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_request_peer_sca_cp *params = (const void *)cmdbuf;
    struct ble_ll_conn_sm *connsm;

    connsm = ble_ll_conn_find_by_handle(le16toh(params->conn_handle));
    if (!connsm) {
        return BLE_ERR_UNK_CONN_ID;
    }

    if (!ble_ll_conn_rem_feature_check(connsm, BLE_LL_FEAT_SCA_UPDATE)) {
        return BLE_ERR_UNSUPP_REM_FEATURE;
    }

    if (IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_SCA_UPDATE)) {
        /* Not really specified what we should return */
        return BLE_ERR_CTLR_BUSY;
    }

    ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_SCA_UPDATE, NULL);

    return 0;
}
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
int
ble_ll_conn_hci_set_default_subrate(const uint8_t *cmdbuf, uint8_t len,
                                    uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_set_default_subrate_cp *cp = (const void *)cmdbuf;
    struct ble_ll_conn_global_params *gcp = &g_ble_ll_conn_params;
    uint16_t subrate_min;
    uint16_t subrate_max;
    uint16_t max_latency;
    uint16_t cont_num;
    uint16_t supervision_tmo;

    subrate_min = le16toh(cp->subrate_min);
    subrate_max = le16toh(cp->subrate_max);
    max_latency = le16toh(cp->max_latency);
    cont_num = le16toh(cp->cont_num);
    supervision_tmo = le16toh(cp->supervision_tmo);

    if ((subrate_min < 0x0001) || (subrate_min > 0x01f4) ||
        (subrate_max < 0x0001) || (subrate_max > 0x01f4) ||
        (max_latency > 0x01f3) || (cont_num > 0x01f3) ||
        (supervision_tmo < 0x000a) || (supervision_tmo > 0x0c80)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (subrate_max * (max_latency + 1) > 500) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (subrate_max < subrate_min) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (cont_num >= subrate_max) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    gcp->acc_subrate_min = subrate_min;
    gcp->acc_subrate_max = subrate_max;
    gcp->acc_max_latency = max_latency;
    gcp->acc_cont_num = cont_num;
    gcp->acc_supervision_tmo = supervision_tmo;

    return 0;
}

int
ble_ll_conn_hci_subrate_req(const uint8_t *cmdbuf, uint8_t len,
                            uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_subrate_req_cp *cp = (const void *)cmdbuf;
    struct ble_ll_conn_subrate_req_params srp;
    struct ble_ll_conn_sm *connsm;
    uint16_t conn_handle;
    int rc;

    conn_handle = le16toh(cp->conn_handle);
    srp.subrate_min = le16toh(cp->subrate_min);
    srp.subrate_max = le16toh(cp->subrate_max);
    srp.max_latency = le16toh(cp->max_latency);
    srp.cont_num = le16toh(cp->cont_num);
    srp.supervision_tmo = le16toh(cp->supervision_tmo);

    connsm = ble_ll_conn_find_by_handle(conn_handle);
    if (!connsm) {
        return BLE_ERR_UNK_CONN_ID;
    }

    rc = ble_ll_conn_subrate_req_hci(connsm, &srp);
    if (rc < 0) {
        if (rc == -EINVAL) {
            return BLE_ERR_INV_HCI_CMD_PARMS;
        } else if (rc == -EBUSY) {
            return BLE_ERR_CTLR_BUSY;
        } else if (rc == -ENOTSUP) {
            return BLE_ERR_UNSUPP_REM_FEATURE;
        } else {
            return BLE_ERR_UNSPECIFIED;
        }
    }

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    if (connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
        connsm->acc_subrate_min = srp.subrate_min;
        connsm->acc_subrate_min = srp.subrate_max;
        connsm->acc_max_latency = srp.max_latency;
        connsm->acc_cont_num = srp.cont_num;
        connsm->acc_supervision_tmo = srp.supervision_tmo;
    }
#endif

    return 0;
}
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_PING)
/**
 * Read authenticated payload timeout (OGF=3, OCF==0x007B)
 *
 * @param cmdbuf
 * @param rsplen
 *
 * @return int
 */
int
ble_ll_conn_hci_rd_auth_pyld_tmo(const uint8_t *cmdbuf, uint8_t len,
                                 uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_cb_rd_auth_pyld_tmo_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_cb_rd_auth_pyld_tmo_rp *rsp = (void *) rspbuf;
    struct ble_ll_conn_sm *connsm;
    uint16_t handle;
    int rc;


    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    handle = le16toh(cmd->conn_handle);
    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        rc = BLE_ERR_UNK_CONN_ID;
        rsp->tmo = 0;
    } else {
        rc = BLE_ERR_SUCCESS;
        rsp->tmo = htole16(connsm->auth_pyld_tmo);
    }

    rsp->conn_handle = htole16(handle);

    *rsplen = sizeof(*rsp);
    return rc;
}

/**
 * Write authenticated payload timeout (OGF=3, OCF=00x7C)
 *
 * @param cmdbuf
 * @param rsplen
 *
 * @return int
 */
int
ble_ll_conn_hci_wr_auth_pyld_tmo(const uint8_t *cmdbuf, uint8_t len,
                                 uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_cb_wr_auth_pyld_tmo_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_cb_wr_auth_pyld_tmo_rp *rsp = (void *) rspbuf;
    struct ble_ll_conn_sm *connsm;
    uint32_t min_tmo;
    uint16_t handle;
    uint16_t tmo;
    int rc;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    rc = BLE_ERR_SUCCESS;

    handle = le16toh(cmd->conn_handle);

    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        rc = BLE_ERR_UNK_CONN_ID;
    } else {
        /*
         * The timeout is in units of 10 msecs. We need to make sure that the
         * timeout is greater than or equal to connItvl * (1 + peripheralLatency)
         */
        tmo = le16toh(cmd->tmo);
        min_tmo = (uint32_t)connsm->conn_itvl * BLE_LL_CONN_ITVL_USECS;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
        min_tmo *= connsm->subrate_factor;
#endif
        min_tmo *= (connsm->periph_latency + 1);
        min_tmo /= 10000;

        if (tmo < min_tmo) {
            rc = BLE_ERR_CMD_DISALLOWED;
        } else {
            connsm->auth_pyld_tmo = tmo;
            if (ble_npl_callout_is_active(&connsm->auth_pyld_timer)) {
                ble_ll_conn_auth_pyld_timer_start(connsm);
            }
        }
    }

    rsp->conn_handle = htole16(handle);
    *rsplen = sizeof(*rsp);
    return rc;
}
#endif

#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
/**
 * Read current phy for connection (OGF=8, OCF==0x0030)
 *
 * @param cmdbuf
 * @param rsplen
 *
 * @return int
 */
int
ble_ll_conn_hci_le_rd_phy(const uint8_t *cmdbuf, uint8_t len,
                          uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_rd_phy_cp *cmd = (const void *) cmdbuf;
    struct ble_hci_le_rd_phy_rp *rsp = (void *) rspbuf;
    int rc;
    uint16_t handle;
    struct ble_ll_conn_sm *connsm;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    handle = le16toh(cmd->conn_handle);
    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        rsp->tx_phy = 0;
        rsp->rx_phy = 0;
        rc = BLE_ERR_UNK_CONN_ID;
    } else {
        rsp->tx_phy = connsm->phy_data.cur_tx_phy;
        rsp->rx_phy = connsm->phy_data.cur_rx_phy;
        rc = BLE_ERR_SUCCESS;
    }

    rsp->conn_handle = htole16(handle);

    *rsplen = sizeof(*rsp);
    return rc;
}

/**
 * Set PHY preferences for connection
 *
 * @param cmdbuf
 *
 * @return int
 */
int
ble_ll_conn_hci_le_set_phy(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_set_phy_cp *cmd = (const void *) cmdbuf;
    int rc;
    uint16_t phy_options;
    uint8_t tx_phys;
    uint8_t rx_phys;
    uint16_t handle;
    struct ble_ll_conn_sm *connsm;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    handle = le16toh(cmd->conn_handle);
    connsm = ble_ll_conn_find_by_handle(handle);
    if (!connsm) {
        return BLE_ERR_UNK_CONN_ID;
    }

    /*
     * If host has requested a PHY update and we are not finished do
     * not allow another one
     */
    if (connsm->flags.phy_update_host_initiated) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    phy_options = le16toh(cmd->phy_options);
    if (phy_options > BLE_HCI_LE_PHY_CODED_S8_PREF) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* Check valid parameters */
    rc = ble_ll_hci_chk_phy_masks(cmd->all_phys, cmd->tx_phys, cmd->rx_phys,
                                  &tx_phys, &rx_phys);
    if (rc) {
        goto phy_cmd_param_err;
    }

    connsm->phy_data.pref_opts = phy_options & 0x03;
    connsm->phy_data.pref_mask_tx = tx_phys,
    connsm->phy_data.pref_mask_rx = rx_phys;

    /*
     * The host preferences override the default phy preferences. Currently,
     * the only reason the controller will initiate a procedure on its own
     * is due to the fact that the host set default preferences. So if there
     * is a pending control procedure and it has not yet started, we do not
     * need to perform the default controller procedure.
     */
    if (IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_PHY_UPDATE)) {
        if (connsm->cur_ctrl_proc != BLE_LL_CTRL_PROC_PHY_UPDATE) {
            connsm->flags.phy_update_self_initiated = 0;
        }
        connsm->flags.phy_update_host_initiated = 1;
    } else {
        /*
         * We could be doing a peer-initiated PHY update procedure. If this
         * is the case the requested phy preferences will not both be 0. If
         * we are not done with a peer-initiated procedure we just set the
         * pending bit but do not start the control procedure.
         */
        if (connsm->flags.phy_update_peer_initiated) {
            connsm->pending_ctrl_procs |= (1 << BLE_LL_CTRL_PROC_PHY_UPDATE);
            connsm->flags.phy_update_host_initiated = 1;
        } else {
            /* Check if we should start phy update procedure */
            if (!ble_ll_conn_phy_update_if_needed(connsm)) {
                connsm->flags.phy_update_host_initiated = 1;
            } else {
                /*
                 * Set flag to send a PHY update complete event. We set flag
                 * even if we do not do an update procedure since we have to
                 * inform the host even if we decide not to change anything.
                 */
                connsm->flags.phy_update_host_w4event = 1;
            }
        }
    }

phy_cmd_param_err:
    return rc;
}
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV_SYNC_TRANSFER)
int
ble_ll_set_sync_transfer_params(const uint8_t *cmdbuf, uint8_t len,
                                uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_periodic_adv_sync_transfer_params_cp *cmd = (const void *)cmdbuf;
    struct ble_hci_le_periodic_adv_sync_transfer_params_rp *rsp = (void *) rspbuf;
    struct ble_ll_conn_sm *connsm;
    uint16_t sync_timeout;
    uint16_t skip;
    int rc;

    if (len != sizeof(*cmd)) {
        rc = BLE_ERR_INV_HCI_CMD_PARMS;
        goto done;
    }

    if ((MYNEWT_VAL(BLE_VERSION) >= 53 ) && (cmd->mode == 0x03)) {
        /* We do not support ADI in periodic advertising thus cannot enable
         * duplicate filtering.
         */
        return BLE_ERR_UNSUPPORTED;
    } else if (cmd->mode > 0x02) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    skip = le16toh(cmd->skip);
    if (skip > 0x01f3) {
        rc = BLE_ERR_INV_HCI_CMD_PARMS;
        goto done;
    }

    sync_timeout = le16toh(cmd->sync_timeout);
    if ((sync_timeout < 0x000a) || (sync_timeout > 0x4000)) {
        rc = BLE_ERR_INV_HCI_CMD_PARMS;
        goto done;
    }

    /* we don't support any CTE yet */
    if (cmd->sync_cte_type) {
        rc = BLE_ERR_UNSUPPORTED;
        goto done;
    }

    connsm = ble_ll_conn_find_by_handle(le16toh(cmd->conn_handle));
    if (!connsm) {
        rc = BLE_ERR_UNK_CONN_ID;
        goto done;
    }

    /* timeout in 10ms units */
    connsm->sync_transfer_sync_timeout = sync_timeout * 10000;
    connsm->sync_transfer_mode = cmd->mode;
    connsm->sync_transfer_skip = skip;

    rc = BLE_ERR_SUCCESS;

done:
    rsp->conn_handle = cmd->conn_handle;
    *rsplen = sizeof(*rsp);
    return rc;
}

int
ble_ll_set_default_sync_transfer_params(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_set_default_periodic_sync_transfer_params_cp *cmd = (const void *)cmdbuf;
    uint16_t sync_timeout;
    uint16_t skip;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if ((MYNEWT_VAL(BLE_VERSION) >= 53 ) && (cmd->mode == 0x03)) {
        /* We do not support ADI in periodic advertising thus cannot enable
         * duplicate filtering.
         */
        return BLE_ERR_UNSUPPORTED;
    } else if (cmd->mode > 0x02) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    skip = le16toh(cmd->skip);
    if (skip > 0x01f3) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    sync_timeout = le16toh(cmd->sync_timeout);
    if ((sync_timeout < 0x000a) || (sync_timeout > 0x4000)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    /* we don't support any CTE yet */
    if (cmd->sync_cte_type) {
        return BLE_ERR_UNSUPPORTED;
    }

    /* timeout in 10ms units */
    g_ble_ll_conn_sync_transfer_params.sync_timeout_us = sync_timeout * 10000;
    g_ble_ll_conn_sync_transfer_params.mode = cmd->mode;
    g_ble_ll_conn_sync_transfer_params.max_skip = skip;

    return BLE_ERR_SUCCESS;
}
#endif
#endif
#endif
