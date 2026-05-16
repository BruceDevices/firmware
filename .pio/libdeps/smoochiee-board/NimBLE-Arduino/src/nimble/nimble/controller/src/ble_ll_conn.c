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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/porting/nimble/include/os/os.h"
#include "nimble/nimble/include/nimble/ble.h"
#include "nimble/nimble/include/nimble/hci_common.h"
#include "nimble/nimble/transport/include/nimble/transport.h"
#include "nimble/nimble/controller/include/controller/ble_ll.h"
#include "nimble/nimble/controller/include/controller/ble_ll_pdu.h"
#include "nimble/nimble/controller/include/controller/ble_ll_conn.h"
#include "nimble/nimble/controller/include/controller/ble_ll_hci.h"
#include "nimble/nimble/controller/include/controller/ble_ll_scan.h"
#include "nimble/nimble/controller/include/controller/ble_ll_whitelist.h"
#include "nimble/nimble/controller/include/controller/ble_ll_sched.h"
#include "nimble/nimble/controller/include/controller/ble_ll_ctrl.h"
#include "nimble/nimble/controller/include/controller/ble_ll_resolv.h"
#include "nimble/nimble/controller/include/controller/ble_ll_adv.h"
#include "nimble/nimble/controller/include/controller/ble_ll_trace.h"
#include "nimble/nimble/controller/include/controller/ble_ll_rfmgmt.h"
#include "nimble/nimble/controller/include/controller/ble_ll_tmr.h"
#include "nimble/nimble/controller/include/controller/ble_phy.h"
#include "nimble/nimble/controller/include/controller/ble_ll_utils.h"
#include "ble_ll_conn_priv.h"
#include "ble_ll_ctrl_priv.h"
#include "ble_ll_priv.h"
#if MYNEWT_PKG_apache_mynewt_nimble__nimble_transport_common_hci_ipc
#include <nimble/nimble/transport/common/hci_ipc/include/nimble/transport/hci_ipc.h>
#endif

#if (BLETEST_THROUGHPUT_TEST == 1)
extern void bletest_completed_pkt(uint16_t handle);
#endif

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
struct ble_ll_conn_sm *g_ble_ll_conn_css_ref;
#endif

/* XXX TODO
 * 1) I think if we are initiating and we already have a connection with
 * a device that we will still try and connect to it. Fix this.
 *  -> This is true. There are a couple things to do
 *      i) When a connection create is issued, if we already are connected
 *      deny it. BLE ERROR = 0x0B (ACL connection exists).
 *      ii) If we receive an advertisement while initiating and want to send
 *      a connect request to the device, make sure we dont have it.
 *      iii) I think I need to do something like this: I am initiating and
 *      advertising. Suppose the device I want to connect to sends me a connect
 *      request because I am advertising? What happens to connection? Deal
 *      with this!
 *
 * 2) Make sure we check incoming data packets for size and all that. You
 * know, supported octets and all that. For both rx and tx.
 *
 * 3) Make sure we are setting the schedule end time properly for both peripheral
 * and central. We should just set this to the end of the connection event.
 * We might want to guarantee a IFS time as well since the next event needs
 * to be scheduled prior to the start of the event to account for the time it
 * takes to get a frame ready (which is pretty much the IFS time).
 *
 * 4) looks like the current code will allow the 1st packet in a
 * connection to extend past the end of the allocated connection end
 * time. That is not good. Need to deal with that. Need to extend connection
 * end time.
 *
 * 6) Use error code 0x3E correctly! Connection failed to establish. If you
 * read the LE connection complete event, it says that if the connection
 * fails to be established that the connection complete event gets sent to
 * the host that issued the create connection. Need to resolve this.
 *
 * 7) How does peer address get set if we are using whitelist? Look at filter
 * policy and make sure you are doing this correctly.
 *
 * 8) Right now I use a fixed definition for required slots. CHange this.
 *
 * 10) See what connection state machine elements are purely central and
 * purely peripheral. We can make a union of them.
 *
 * 11) Not sure I am dealing with the connection terminate timeout perfectly.
 * I may extend a connection event too long although if it is always in terms
 * of connection events I am probably fine. Checking at end that the next
 * connection event will occur past terminate timeould would be fine.
 *
 * 12) When a peripheral receives a data packet in a connection it has to send a
 * response. Well, it should. If this packet will overrun the next scheduled
 * event, what should we do? Transmit anyway? Not transmit? For now, we just
 * transmit.
 *
 * 32kHz crystal
 * 1) When scheduling, I need to make sure I have time between
 * this one and the next. Should I deal with this in the sched. Or
 * is this basically accounted for given a slot? I really just need to
 * make sure everything is over N ticks before the next sched start!
 * Just add to end time?
 *
 * 2) I think one way to handle the problem of losing up to a microsecond
 * every time we call ble_ll_conn_next_event in a loop is to do everything by
 * keeping track of last anchor point. Would need last anchor usecs too. I guess
 * we could also keep last anchor usecs as a uint32 or something and when we
 * do the next event keep track of the residual using a different ticks to
 * usecs calculation. Not sure.
 */

/*
 * XXX: How should we deal with a late connection event? We need to determine
 * what we want to do under the following cases:
 *  1) The current connection event has not ended but a schedule item starts
 */

/* Global LL connection parameters */
struct ble_ll_conn_global_params g_ble_ll_conn_params;

#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)

/* This is a dummy structure we use for the empty PDU */
struct ble_ll_empty_pdu
{
    struct os_mbuf om;
    struct os_mbuf_pkthdr pkt_hdr;
    struct ble_mbuf_hdr ble_hdr;
};

/* We cannot have more than 254 connections given our current implementation */
#if (MYNEWT_VAL(BLE_MAX_CONNECTIONS) >= 255)
    #error "Maximum # of connections is 254"
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
/* Global connection complete event. Used when initiating */
uint8_t *g_ble_ll_conn_comp_ev;
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV_SYNC_TRANSFER)
/* Global default sync transfer params */
struct ble_ll_conn_sync_transfer_params g_ble_ll_conn_sync_transfer_params;
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
struct ble_ll_conn_create_sm g_ble_ll_conn_create_sm;
#endif

/* Pointer to current connection */
struct ble_ll_conn_sm *g_ble_ll_conn_cur_sm;

/* Connection state machine array */
struct ble_ll_conn_sm g_ble_ll_conn_sm[MYNEWT_VAL(BLE_MAX_CONNECTIONS)];

/* List of active connections */
struct ble_ll_conn_active_list g_ble_ll_conn_active_list;

/* List of free connections */
struct ble_ll_conn_free_list g_ble_ll_conn_free_list;

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
static uint16_t g_ble_ll_conn_css_next_slot = BLE_LL_CONN_CSS_NO_SLOT;
struct ble_ll_conn_css_list g_ble_ll_conn_css_list;
#endif

STATS_SECT_START(ble_ll_conn_stats)
    STATS_SECT_ENTRY(cant_set_sched)
    STATS_SECT_ENTRY(conn_ev_late)
    STATS_SECT_ENTRY(wfr_expirations)
    STATS_SECT_ENTRY(handle_not_found)
    STATS_SECT_ENTRY(no_conn_sm)
    STATS_SECT_ENTRY(no_free_conn_sm)
    STATS_SECT_ENTRY(rx_data_pdu_no_conn)
    STATS_SECT_ENTRY(rx_data_pdu_bad_aa)
    STATS_SECT_ENTRY(periph_rxd_bad_conn_req_params)
    STATS_SECT_ENTRY(periph_ce_failures)
    STATS_SECT_ENTRY(data_pdu_rx_dup)
    STATS_SECT_ENTRY(data_pdu_txg)
    STATS_SECT_ENTRY(data_pdu_txf)
    STATS_SECT_ENTRY(conn_req_txd)
    STATS_SECT_ENTRY(l2cap_enqueued)
    STATS_SECT_ENTRY(rx_ctrl_pdus)
    STATS_SECT_ENTRY(rx_l2cap_pdus)
    STATS_SECT_ENTRY(rx_l2cap_bytes)
    STATS_SECT_ENTRY(rx_malformed_ctrl_pdus)
    STATS_SECT_ENTRY(rx_bad_llid)
    STATS_SECT_ENTRY(tx_ctrl_pdus)
    STATS_SECT_ENTRY(tx_ctrl_bytes)
    STATS_SECT_ENTRY(tx_l2cap_pdus)
    STATS_SECT_ENTRY(tx_l2cap_bytes)
    STATS_SECT_ENTRY(tx_empty_pdus)
    STATS_SECT_ENTRY(mic_failures)
    STATS_SECT_ENTRY(sched_start_in_idle)
    STATS_SECT_ENTRY(sched_end_in_idle)
    STATS_SECT_ENTRY(conn_event_while_tmo)
STATS_SECT_END
STATS_SECT_DECL(ble_ll_conn_stats) ble_ll_conn_stats;

STATS_NAME_START(ble_ll_conn_stats)
    STATS_NAME(ble_ll_conn_stats, cant_set_sched)
    STATS_NAME(ble_ll_conn_stats, conn_ev_late)
    STATS_NAME(ble_ll_conn_stats, wfr_expirations)
    STATS_NAME(ble_ll_conn_stats, handle_not_found)
    STATS_NAME(ble_ll_conn_stats, no_conn_sm)
    STATS_NAME(ble_ll_conn_stats, no_free_conn_sm)
    STATS_NAME(ble_ll_conn_stats, rx_data_pdu_no_conn)
    STATS_NAME(ble_ll_conn_stats, rx_data_pdu_bad_aa)
    STATS_NAME(ble_ll_conn_stats, periph_rxd_bad_conn_req_params)
    STATS_NAME(ble_ll_conn_stats, periph_ce_failures)
    STATS_NAME(ble_ll_conn_stats, data_pdu_rx_dup)
    STATS_NAME(ble_ll_conn_stats, data_pdu_txg)
    STATS_NAME(ble_ll_conn_stats, data_pdu_txf)
    STATS_NAME(ble_ll_conn_stats, conn_req_txd)
    STATS_NAME(ble_ll_conn_stats, l2cap_enqueued)
    STATS_NAME(ble_ll_conn_stats, rx_ctrl_pdus)
    STATS_NAME(ble_ll_conn_stats, rx_l2cap_pdus)
    STATS_NAME(ble_ll_conn_stats, rx_l2cap_bytes)
    STATS_NAME(ble_ll_conn_stats, rx_malformed_ctrl_pdus)
    STATS_NAME(ble_ll_conn_stats, rx_bad_llid)
    STATS_NAME(ble_ll_conn_stats, tx_ctrl_pdus)
    STATS_NAME(ble_ll_conn_stats, tx_ctrl_bytes)
    STATS_NAME(ble_ll_conn_stats, tx_l2cap_pdus)
    STATS_NAME(ble_ll_conn_stats, tx_l2cap_bytes)
    STATS_NAME(ble_ll_conn_stats, tx_empty_pdus)
    STATS_NAME(ble_ll_conn_stats, mic_failures)
    STATS_NAME(ble_ll_conn_stats, sched_start_in_idle)
    STATS_NAME(ble_ll_conn_stats, sched_end_in_idle)
    STATS_NAME(ble_ll_conn_stats, conn_event_while_tmo)
STATS_NAME_END(ble_ll_conn_stats)

static void ble_ll_conn_event_end(struct ble_npl_event *ev);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_CTRL_TO_HOST_FLOW_CONTROL)
struct ble_ll_conn_cth_flow {
    bool enabled;
    uint16_t max_buffers;
    uint16_t num_buffers;
};

static struct ble_ll_conn_cth_flow g_ble_ll_conn_cth_flow;

static struct ble_npl_event g_ble_ll_conn_cth_flow_error_ev;

static bool
ble_ll_conn_cth_flow_is_enabled(void)
{
    return g_ble_ll_conn_cth_flow.enabled;
}

static bool
ble_ll_conn_cth_flow_alloc_credit(struct ble_ll_conn_sm *connsm)
{
    struct ble_ll_conn_cth_flow *cth = &g_ble_ll_conn_cth_flow;
    os_sr_t sr;

    OS_ENTER_CRITICAL(sr);

    if (!cth->num_buffers) {
        OS_EXIT_CRITICAL(sr);
        return false;
    }

    connsm->cth_flow_pending++;
    cth->num_buffers--;

    OS_EXIT_CRITICAL(sr);

    return true;
}

static void
ble_ll_conn_cth_flow_free_credit(struct ble_ll_conn_sm *connsm, uint16_t credits)
{
    struct ble_ll_conn_cth_flow *cth = &g_ble_ll_conn_cth_flow;
    os_sr_t sr;

    OS_ENTER_CRITICAL(sr);

    /*
     * It's not quite clear what we should do if host gives back more credits
     * that we have allocated. For now let's just set invalid values back to
     * sane values and continue.
     */

    cth->num_buffers += credits;
    if (cth->num_buffers > cth->max_buffers) {
        cth->num_buffers = cth->max_buffers;
    }

    if (connsm->cth_flow_pending < credits) {
        connsm->cth_flow_pending = 0;
    } else {
        connsm->cth_flow_pending -= credits;
    }

    OS_EXIT_CRITICAL(sr);
}

static void
ble_ll_conn_cth_flow_error_fn(struct ble_npl_event *ev)
{
    struct ble_hci_ev *hci_ev;
    struct ble_hci_ev_command_complete *hci_ev_cp;
    uint16_t opcode;

    hci_ev = ble_transport_alloc_evt(0);
    if (!hci_ev) {
        /* Not much we can do anyway... */
        return;
    }

    /*
     * We are here in case length of HCI_Host_Number_Of_Completed_Packets was
     * invalid. We will send an error back to host and we can only hope host is
     * reasonable and will do some actions to recover, e.g. it should disconnect
     * all connections to guarantee that all credits are back in pool and we're
     * back in sync (although spec does not really say what should happen).
     */

    opcode = BLE_HCI_OP(BLE_HCI_OGF_CTLR_BASEBAND,
                        BLE_HCI_OCF_CB_HOST_NUM_COMP_PKTS);

    hci_ev->opcode = BLE_HCI_EVCODE_COMMAND_COMPLETE;
    hci_ev->length = sizeof(*hci_ev_cp);

    hci_ev_cp = (void *)hci_ev->data;
    hci_ev_cp->num_packets = BLE_LL_CFG_NUM_HCI_CMD_PKTS;
    hci_ev_cp->opcode = htole16(opcode);
    hci_ev_cp->status = BLE_ERR_INV_HCI_CMD_PARMS;

    ble_ll_hci_event_send(hci_ev);
}

void
ble_ll_conn_cth_flow_set_buffers(uint16_t num_buffers)
{
    BLE_LL_ASSERT(num_buffers);

    g_ble_ll_conn_cth_flow.max_buffers = num_buffers;
    g_ble_ll_conn_cth_flow.num_buffers = num_buffers;
}

bool
ble_ll_conn_cth_flow_enable(bool enabled)
{
    struct ble_ll_conn_cth_flow *cth = &g_ble_ll_conn_cth_flow;

    if (cth->enabled == enabled) {
        return true;
    }

    if (!SLIST_EMPTY(&g_ble_ll_conn_active_list)) {
        return false;
    }

    cth->enabled = enabled;

    return true;
}

void
ble_ll_conn_cth_flow_process_cmd(const uint8_t *cmdbuf)
{
    const struct ble_hci_cmd *cmd;
    const struct ble_hci_cb_host_num_comp_pkts_cp *cp;
    struct ble_ll_conn_sm *connsm;
    int i;

    cmd = (const void *)cmdbuf;
    cp = (const void *)cmd->data;

    if (cmd->length != sizeof(cp->handles) + cp->handles * sizeof(cp->h[0])) {
        ble_ll_event_add(&g_ble_ll_conn_cth_flow_error_ev);
        return;
    }

    for (i = 0; i < cp->handles; i++) {
        /*
         * It's probably ok that we do not have active connection with given
         * handle - this can happen if disconnection already happened in LL but
         * host sent credits back before processing disconnection event. In such
         * case we can simply ignore command for that connection since credits
         * are returned by LL already.
         */
        connsm = ble_ll_conn_find_by_handle(cp->h[i].handle);
        if (connsm) {
            ble_ll_conn_cth_flow_free_credit(connsm, cp->h[i].count);
        }
    }
}
#endif

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
static void
ble_ll_conn_css_update_list(struct ble_ll_conn_sm *connsm)
{
    struct ble_ll_conn_sm *e;
    struct ble_ll_conn_sm *e_last;
    struct ble_ll_conn_sm *e_insert_after = NULL;
    bool found_to_insert = false;

    if (SLIST_FIRST(&g_ble_ll_conn_css_list) == connsm) {
        SLIST_REMOVE_HEAD(&g_ble_ll_conn_css_list, css_sle);
    } else {
        e_last = NULL;
        SLIST_FOREACH(e, &g_ble_ll_conn_css_list, css_sle) {
            if (e == connsm) {
                SLIST_NEXT(e_last, css_sle) = SLIST_NEXT(e, css_sle);
                break;
            }
            e_last = e;
        }
    }

    if (SLIST_EMPTY(&g_ble_ll_conn_css_list)) {
        SLIST_INSERT_HEAD(&g_ble_ll_conn_css_list, connsm, css_sle);
        return;
    }

    e_last = NULL;
    SLIST_FOREACH(e, &g_ble_ll_conn_css_list, css_sle) {
        if (e->css_slot_idx > connsm->css_slot_idx) {
            found_to_insert = true;
            e_insert_after = e_last;
            break;
        }

        e_last = e;
    }

    if (found_to_insert) {
        if (e_insert_after) {
            SLIST_INSERT_AFTER(e_last, connsm, css_sle);
        } else {
            SLIST_INSERT_HEAD(&g_ble_ll_conn_css_list, connsm, css_sle);
        }
    } else {
        SLIST_INSERT_AFTER(e_last, connsm, css_sle);
    }
}

void
ble_ll_conn_css_set_next_slot(uint16_t slot_idx)
{
    g_ble_ll_conn_css_next_slot = slot_idx;
}

uint16_t
ble_ll_conn_css_get_next_slot(void)
{
    struct ble_ll_conn_sm *connsm;
    uint16_t slot_idx = 0;

    if (g_ble_ll_conn_css_next_slot != BLE_LL_CONN_CSS_NO_SLOT) {
        return g_ble_ll_conn_css_next_slot;
    }

    /* CSS connections are sorted in active conn list so just need to find 1st
     * free value.
     */
    SLIST_FOREACH(connsm, &g_ble_ll_conn_css_list, css_sle) {
        if ((connsm->css_slot_idx != slot_idx) &&
            (connsm->css_slot_idx_pending != slot_idx)) {
            break;
        }
        slot_idx++;
    }

    if (slot_idx >= ble_ll_sched_css_get_period_slots()) {
        slot_idx = BLE_LL_CONN_CSS_NO_SLOT;
    }

    return slot_idx;
}

int
ble_ll_conn_css_is_slot_busy(uint16_t slot_idx)
{
    struct ble_ll_conn_sm *connsm;

    if (g_ble_ll_conn_css_next_slot == slot_idx) {
        return 1;
    }

    SLIST_FOREACH(connsm, &g_ble_ll_conn_css_list, css_sle) {
        if ((connsm->css_slot_idx == slot_idx) ||
            (connsm->css_slot_idx_pending == slot_idx)) {
            return 1;
        }
    }

    return 0;
}

int
ble_ll_conn_css_move(struct ble_ll_conn_sm *connsm, uint16_t slot_idx)
{
    int16_t slot_diff;
    uint32_t offset;
    int rc;

    /* Assume connsm and slot_idx are valid */
    BLE_LL_ASSERT(connsm->conn_state != BLE_LL_CONN_STATE_IDLE);
    BLE_LL_ASSERT(connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL);
    BLE_LL_ASSERT((slot_idx < ble_ll_sched_css_get_period_slots()) ||
                  (slot_idx != BLE_LL_CONN_CSS_NO_SLOT));

    slot_diff = slot_idx - connsm->css_slot_idx;
    if (slot_diff < 0) {
        slot_diff += ble_ll_sched_css_get_period_slots();
    }

    offset = slot_diff * ble_ll_sched_css_get_slot_us() / BLE_LL_CONN_ITVL_USECS;

    if (offset >= 0xffff) {
        return -1;
    }

    rc = ble_ll_conn_move_anchor(connsm, offset);
    if (!rc) {
        connsm->css_slot_idx_pending = slot_idx;
    }

    return rc;
}
#endif

struct ble_ll_conn_sm *
ble_ll_conn_find_by_peer_addr(const uint8_t *addr, uint8_t addr_type)
{
    struct ble_ll_conn_sm *connsm;

    SLIST_FOREACH(connsm, &g_ble_ll_conn_active_list, act_sle) {
        if (!memcmp(&connsm->peer_addr, addr, BLE_DEV_ADDR_LEN) &&
            !((connsm->peer_addr_type ^ addr_type) & 1)) {
            return connsm;
        }
    }

    return NULL;
}

#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
static inline int
ble_ll_conn_phy_should_update(uint8_t pref_mask, uint8_t curr_mask)
{
#if MYNEWT_VAL(BLE_LL_CONN_PHY_PREFER_2M)
    /* Should change to 2M if preferred, but not active */
    if ((pref_mask & BLE_PHY_MASK_2M) && (curr_mask != BLE_PHY_MASK_2M)) {
        return 1;
    }
#endif

    /* Should change to active phy is not preferred */
    if ((curr_mask & pref_mask) == 0) {
        return 1;
    }

    return 0;
}

int
ble_ll_conn_phy_update_if_needed(struct ble_ll_conn_sm *connsm)
{
    if (!ble_ll_conn_phy_should_update(connsm->phy_data.pref_mask_tx,
                                       CONN_CUR_TX_PHY_MASK(connsm)) &&
        !ble_ll_conn_phy_should_update(connsm->phy_data.pref_mask_rx,
                                       CONN_CUR_RX_PHY_MASK(connsm))) {
        return -1;
    }

    connsm->phy_data.pref_mask_tx_req = connsm->phy_data.pref_mask_tx;
    connsm->phy_data.pref_mask_rx_req = connsm->phy_data.pref_mask_rx;

    ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_PHY_UPDATE, NULL);

    return 0;
}
#endif

void
ble_ll_conn_itvl_to_ticks(uint32_t itvl, uint32_t *itvl_ticks,
                          uint8_t *itvl_usecs)
{
    *itvl_ticks = ble_ll_tmr_u2t_r(itvl * BLE_LL_CONN_ITVL_USECS, itvl_usecs);
}

/**
 * Get the event buffer allocated to send the connection complete event
 * when we are initiating.
 *
 * @return uint8_t*
 */
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
static uint8_t *
ble_ll_init_get_conn_comp_ev(void)
{
    uint8_t *evbuf;

    evbuf = g_ble_ll_conn_comp_ev;
    BLE_LL_ASSERT(evbuf != NULL);
    g_ble_ll_conn_comp_ev = NULL;

    return evbuf;
}
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
/**
 * Called to determine if the received PDU is an empty PDU or not.
 */
static int
ble_ll_conn_is_empty_pdu(uint8_t *rxbuf)
{
    int rc;
    uint8_t llid;

    llid = rxbuf[0] & BLE_LL_DATA_HDR_LLID_MASK;
    if ((llid == BLE_LL_LLID_DATA_FRAG) && (rxbuf[1] == 0)) {
        rc = 1;
    } else {
        rc = 0;
    }
    return rc;
}
#endif

/**
 * Called to return the currently running connection state machine end time.
 * Always called when interrupts are disabled.
 *
 * @return int 0: s1 is not least recently used. 1: s1 is least recently used
 */
int
ble_ll_conn_is_lru(struct ble_ll_conn_sm *s1, struct ble_ll_conn_sm *s2)
{
    int rc;

    /* Set time that we last serviced the schedule */
    if (LL_TMR_LT(s1->last_scheduled, s2->last_scheduled)) {
        rc = 1;
    } else {
        rc = 0;
    }

    return rc;
}

/**
 * Called to return the currently running connection state machine end time.
 * Always called when interrupts are disabled.
 *
 * @return uint32_t
 */
uint32_t
ble_ll_conn_get_ce_end_time(void)
{
    uint32_t ce_end_time;

    if (g_ble_ll_conn_cur_sm) {
        ce_end_time = g_ble_ll_conn_cur_sm->ce_end_time;
    } else {
        ce_end_time = ble_ll_tmr_get();
    }
    return ce_end_time;
}

/**
 * Called when connection state machine needs to halt. This function will:
 *  -> Disable the PHY, which will prevent any transmit/receive interrupts.
 *  -> Disable the wait for response timer, if running.
 *  -> Remove the connection state machine from the scheduler.
 *  -> Sets the Link Layer state to standby.
 *  -> Sets the current state machine to NULL.
 *
 *  NOTE: the ordering of these function calls is important! We have to stop
 *  the PHY and remove the schedule item before we can set the state to
 *  standby and set the current state machine pointer to NULL.
 */
static void
ble_ll_conn_halt(void)
{
    ble_phy_disable();
    ble_ll_state_set(BLE_LL_STATE_STANDBY);
    g_ble_ll_conn_cur_sm = NULL;
}

/**
 * Called when the current connection state machine is no longer being used.
 */
static void
ble_ll_conn_current_sm_over(struct ble_ll_conn_sm *connsm)
{

    ble_ll_conn_halt();

    /*
     * NOTE: the connection state machine may be NULL if we are calling
     * this when we are ending the connection. In that case, there is no
     * need to post to the LL the connection event end event
     */
    if (connsm) {
        ble_ll_event_add(&connsm->conn_ev_end);
    }
}

/**
 * Given a handle, find an active connection matching the handle
 *
 * @param handle
 *
 * @return struct ble_ll_conn_sm*
 */
struct ble_ll_conn_sm *
ble_ll_conn_find_by_handle(uint16_t handle)
{
    struct ble_ll_conn_sm *connsm;

    connsm = NULL;
    if ((handle != 0) && (handle <= MYNEWT_VAL(BLE_MAX_CONNECTIONS))) {
        connsm = &g_ble_ll_conn_sm[handle - 1];
        if (connsm->conn_state == BLE_LL_CONN_STATE_IDLE) {
            connsm = NULL;
        }
    }
    return connsm;
}

/**
 * Get a connection state machine.
 */
struct ble_ll_conn_sm *
ble_ll_conn_sm_get(void)
{
    struct ble_ll_conn_sm *connsm;

    connsm = STAILQ_FIRST(&g_ble_ll_conn_free_list);
    if (connsm) {
        STAILQ_REMOVE_HEAD(&g_ble_ll_conn_free_list, free_stqe);
    } else {
        STATS_INC(ble_ll_conn_stats, no_free_conn_sm);
    }

    return connsm;
}

static uint8_t
ble_ll_conn_calc_dci_csa1(struct ble_ll_conn_sm *conn)
{
    uint8_t curchan;
    uint8_t remap_index;
    uint8_t bitpos;

    /* Get next unmapped channel */
    curchan = conn->last_unmapped_chan + conn->hop_inc;
    if (curchan > BLE_PHY_NUM_DATA_CHANS) {
        curchan -= BLE_PHY_NUM_DATA_CHANS;
    }

    /* Save unmapped channel */
    conn->last_unmapped_chan = curchan;

    /* Is this a valid channel? */
    bitpos = 1 << (curchan & 0x07);
    if (conn->chan_map[curchan >> 3] & bitpos) {
        return curchan;
    }

    /* Calculate remap index */
    remap_index = curchan % conn->chan_map_used;

    return ble_ll_utils_chan_map_remap(conn->chan_map, remap_index);
}

/**
 * Determine data channel index to be used for the upcoming/current
 * connection event
 *
 * @param conn
 * @param latency Used only for CSA #1
 *
 * @return uint8_t
 */
uint8_t
ble_ll_conn_calc_dci(struct ble_ll_conn_sm *conn, uint16_t latency)
{
    uint8_t index;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CSA2)
    if (conn->flags.csa2) {
        return ble_ll_utils_dci_csa2(conn->event_cntr, conn->channel_id,
                                     conn->chan_map_used, conn->chan_map);
    }
#endif

    index = conn->data_chan_index;

    while (latency > 0) {
        index = ble_ll_conn_calc_dci_csa1(conn);
        latency--;
    }

    return index;
}

/**
 * Called when we are in the connection state and the wait for response timer
 * fires off.
 *
 * Context: Interrupt
 */
void
ble_ll_conn_wfr_timer_exp(void)
{
    struct ble_ll_conn_sm *connsm;

    connsm = g_ble_ll_conn_cur_sm;
    ble_ll_conn_current_sm_over(connsm);
    STATS_INC(ble_ll_conn_stats, wfr_expirations);
}

/**
 * Callback for peripheral when it transmits a data pdu and the connection event
 * ends after the transmission.
 *
 * Context: Interrupt
 *
 * @param sch
 *
 */
static void
ble_ll_conn_wait_txend(void *arg)
{
    struct ble_ll_conn_sm *connsm;

    connsm = (struct ble_ll_conn_sm *)arg;
    ble_ll_conn_current_sm_over(connsm);
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
static void
ble_ll_conn_start_rx_encrypt(void *arg)
{
    struct ble_ll_conn_sm *connsm;

    connsm = (struct ble_ll_conn_sm *)arg;
    connsm->flags.encrypted = 1;
    ble_phy_encrypt_enable(connsm->enc_data.enc_block.cipher_text);
    ble_phy_encrypt_iv_set(connsm->enc_data.iv);
    ble_phy_encrypt_counter_set(connsm->enc_data.rx_pkt_cntr,
                                !CONN_IS_CENTRAL(connsm));
}

#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
static void
ble_ll_conn_start_rx_unencrypt(void *arg)
{
    struct ble_ll_conn_sm *connsm;

    connsm = (struct ble_ll_conn_sm *)arg;
    connsm->flags.encrypted = 0;
    ble_phy_encrypt_disable();
}
#endif

static void
ble_ll_conn_txend_encrypt(void *arg)
{
    struct ble_ll_conn_sm *connsm;

    connsm = (struct ble_ll_conn_sm *)arg;
    connsm->flags.encrypted = 1;
    ble_ll_conn_current_sm_over(connsm);
}

#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
static void
ble_ll_conn_rxend_unencrypt(void *arg)
{
    struct ble_ll_conn_sm *connsm;

    connsm = (struct ble_ll_conn_sm *)arg;
    connsm->flags.encrypted = 0;
    ble_ll_conn_current_sm_over(connsm);
}
#endif

static void
ble_ll_conn_continue_rx_encrypt(void *arg)
{
    struct ble_ll_conn_sm *connsm;

    connsm = (struct ble_ll_conn_sm *)arg;
    ble_phy_encrypt_counter_set(connsm->enc_data.rx_pkt_cntr,
                                !CONN_IS_CENTRAL(connsm));
}
#endif

/**
 * Returns the cputime of the next scheduled item on the scheduler list or
 * when the current connection will start its next interval (whichever is
 * earlier). This API is called when determining at what time we should end
 * the current connection event. The current connection event must end before
 * the next scheduled item. However, the current connection itself is not
 * in the scheduler list! Thus, we need to calculate the time at which the
 * next connection will start (the schedule start time; not the anchor point)
 * and not overrun it.
 *
 * Context: Interrupt
 *
 * @param connsm
 *
 * @return uint32_t
 */
static uint32_t
ble_ll_conn_get_next_sched_time(struct ble_ll_conn_sm *connsm)
{
    uint32_t ce_end;
    uint32_t next_sched_time;
    uint8_t rem_us;

    /* Calculate time at which next connection event will start */
    /* NOTE: We dont care if this time is tick short. */
    ce_end = connsm->anchor_point + connsm->conn_itvl_ticks -
        g_ble_ll_sched_offset_ticks;
    rem_us = connsm->anchor_point_usecs;
    ble_ll_tmr_add_u(&ce_end, &rem_us, connsm->conn_itvl_usecs);

    ce_end -= ble_ll_tmr_u2t_up(MYNEWT_VAL(BLE_LL_CONN_EVENT_END_MARGIN));

    if (connsm->max_ce_len_ticks) {
        if (LL_TMR_LT(connsm->anchor_point + connsm->max_ce_len_ticks, ce_end)) {
            ce_end = connsm->anchor_point + connsm->max_ce_len_ticks;
        }
    }

    if (ble_ll_sched_next_time(&next_sched_time)) {
        if (LL_TMR_LT(next_sched_time, ce_end)) {
            ce_end = next_sched_time;
        }
    }

    return ce_end;
}

/**
 * Called to check if certain connection state machine flags have been
 * set.
 *
 * @param connsm
 */
static void
ble_ll_conn_chk_csm_flags(struct ble_ll_conn_sm *connsm)
{
    uint8_t update_status;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
    if (connsm->flags.encrypt_ltk_req) {
        /*
         * Send Long term key request event to host. If masked, we need to
         * send a REJECT_IND.
         */
        if (ble_ll_hci_ev_ltk_req(connsm)) {
            ble_ll_ctrl_reject_ind_send(connsm, BLE_LL_CTRL_ENC_REQ,
                                        BLE_ERR_PINKEY_MISSING);
        }
        connsm->flags.encrypt_ltk_req = 0;
    }
#endif

    /*
     * There are two cases where this flag gets set:
     * 1) A connection update procedure was started and the event counter
     * has passed the instant.
     * 2) We successfully sent the reject reason.
     */
    if (connsm->flags.conn_update_host_w4event) {
        update_status = BLE_ERR_SUCCESS;
        if (IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_CONN_UPDATE)) {
            ble_ll_ctrl_proc_stop(connsm, BLE_LL_CTRL_PROC_CONN_UPDATE);
        } else {
            if (IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_CONN_PARAM_REQ)) {
                ble_ll_ctrl_proc_stop(connsm, BLE_LL_CTRL_PROC_CONN_PARAM_REQ);
                update_status = connsm->reject_reason;
            }
        }
        ble_ll_hci_ev_conn_update(connsm, update_status);
        connsm->flags.conn_update_host_w4event = 0;
    }

    /* Check if we need to send PHY update complete event */
#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    if (connsm->flags.phy_update_host_w4event) {
        if (!ble_ll_hci_ev_phy_update(connsm, BLE_ERR_SUCCESS)) {
            /* Sent event. Clear flag */
            connsm->flags.phy_update_host_w4event = 0;
        }
    }
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    if (connsm->flags.subrate_ind_txd) {
        ble_ll_conn_subrate_set(connsm, &connsm->subrate_trans);
        connsm->subrate_trans.subrate_factor = 0;
        ble_ll_ctrl_proc_stop(connsm, BLE_LL_CTRL_PROC_SUBRATE_UPDATE);
        connsm->flags.subrate_ind_txd = 0;
        connsm->flags.subrate_host_req = 0;
    }
#endif /* BLE_LL_CTRL_SUBRATE_IND */
#endif /* BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE */
}

/**
 * Called when we want to send a data channel pdu inside a connection event.
 *
 * Context: interrupt
 *
 * @param connsm
 *
 * @return int 0: success; otherwise failure to transmit
 */
static uint16_t
ble_ll_conn_adjust_pyld_len(struct ble_ll_conn_sm *connsm, uint16_t pyld_len)
{
    uint16_t max_pyld_len;
    uint16_t ret;

#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    uint8_t phy_mode;

    if (connsm->phy_tx_transition) {
        phy_mode = ble_ll_phy_to_phy_mode(connsm->phy_tx_transition,
                                          connsm->phy_data.pref_opts);
    } else {
        phy_mode = connsm->phy_data.tx_phy_mode;
    }

    max_pyld_len = ble_ll_pdu_max_tx_octets_get(connsm->eff_max_tx_time,
                                                phy_mode);

#else
    max_pyld_len = ble_ll_pdu_max_tx_octets_get(connsm->eff_max_tx_time,
                                                BLE_PHY_MODE_1M);
#endif

    ret = pyld_len;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
    if (connsm->flags.encrypted) {
        max_pyld_len -= BLE_LL_DATA_MIC_LEN;
    }
#endif

    if (ret > connsm->eff_max_tx_octets) {
        ret = connsm->eff_max_tx_octets;
    }

    if (ret > max_pyld_len) {
        ret = max_pyld_len;
    }

    return ret;
}

static int
ble_ll_conn_tx_pdu(struct ble_ll_conn_sm *connsm)
{
    int rc;
    uint8_t md;
    uint8_t hdr_byte;
    uint8_t end_transition;
    uint8_t cur_txlen;
    uint16_t next_txlen;
    uint16_t cur_offset;
    uint16_t pktlen;
    uint32_t next_event_time;
    uint32_t ticks;
    struct os_mbuf *m;
    struct ble_mbuf_hdr *ble_hdr;
    struct os_mbuf_pkthdr *pkthdr = NULL;
    struct os_mbuf_pkthdr *nextpkthdr;
    struct ble_ll_empty_pdu empty_pdu;
    ble_phy_tx_end_func txend_func;
    int tx_phy_mode;
    uint8_t llid;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
    int is_ctrl;
    uint8_t opcode;
#endif

    /* For compiler warnings... */
    ble_hdr = NULL;
    m = NULL;
    md = 0;
    hdr_byte = BLE_LL_LLID_DATA_FRAG;

    if (connsm->flags.terminate_ind_rxd) {
        /* We just received terminate indication.
         * Just send empty packet as an ACK
         */
        connsm->flags.empty_pdu_txd = 1;
        goto conn_tx_pdu;
    }

    /*
     * We need to check if we are retrying a pdu or if there is a pdu on
     * the transmit queue.
     */
    pkthdr = STAILQ_FIRST(&connsm->conn_txq);
    if (!connsm->cur_tx_pdu && !connsm->flags.empty_pdu_txd && !pkthdr) {
        connsm->flags.empty_pdu_txd = 1;
        goto conn_tx_pdu;
    }

    /*
     * If we dont have a pdu we have previously transmitted, take it off
     * the connection transmit queue
     */
    cur_offset = 0;
    if (!connsm->cur_tx_pdu && !connsm->flags.empty_pdu_txd) {
        /* Convert packet header to mbuf */
        m = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);
        nextpkthdr = STAILQ_NEXT(pkthdr, omp_next);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
        /*
         * If we are encrypting, we are only allowed to send certain
         * kinds of LL control PDU's. If none is enqueued, send empty pdu!
         *
         * In Slave role, we are allowed to send unencrypted packets until
         * LL_ENC_RSP is sent.
         */
        if (((connsm->enc_data.enc_state > CONN_ENC_S_ENCRYPTED) &&
             CONN_IS_CENTRAL(connsm)) ||
            ((connsm->enc_data.enc_state > CONN_ENC_S_ENC_RSP_TO_BE_SENT) &&
             CONN_IS_PERIPHERAL(connsm))) {
            if (!ble_ll_ctrl_enc_allowed_pdu_tx(pkthdr)) {
                connsm->flags.empty_pdu_txd = 1;
                goto conn_tx_pdu;
            }

            /*
             * We will allow a next packet if it itself is allowed or we are
             * a peripheral and we are sending the START_ENC_RSP. The central has
             * to wait to receive the START_ENC_RSP from the peripheral before
             * packets can be let go.
             */
            if (nextpkthdr && !ble_ll_ctrl_enc_allowed_pdu_tx(nextpkthdr)
                && (CONN_IS_CENTRAL(connsm) ||
                    !ble_ll_ctrl_is_start_enc_rsp(m))) {
                nextpkthdr = NULL;
            }
        }
#endif
        /* Take packet off queue*/
        STAILQ_REMOVE_HEAD(&connsm->conn_txq, omp_next);
        ble_hdr = BLE_MBUF_HDR_PTR(m);

        /*
         * We dequeued new packet for transmission.
         * If this is a data PDU we need to calculate payload length we can send
         * over current PHY. Effectively, this determines fragmentation of packet
         * into PDUs.
         * If this is a control PDU we send complete PDU as only data PDU can be
         * fragmented. We assume that checks (i.e. if remote supports such PDU)
         * were already performed before putting packet on queue.
         */
        llid = ble_hdr->txinfo.hdr_byte & BLE_LL_DATA_HDR_LLID_MASK;
        pktlen = pkthdr->omp_len;
        if (llid == BLE_LL_LLID_CTRL) {
            cur_txlen = pktlen;
            ble_ll_ctrl_tx_start(connsm, m);
        } else {
            cur_txlen = ble_ll_conn_adjust_pyld_len(connsm, pktlen);
        }
        ble_hdr->txinfo.pyld_len = cur_txlen;

        /* NOTE: header was set when first enqueued */
        hdr_byte = ble_hdr->txinfo.hdr_byte;
        connsm->cur_tx_pdu = m;
    } else {
        nextpkthdr = pkthdr;
        if (connsm->cur_tx_pdu) {
            m = connsm->cur_tx_pdu;
            ble_hdr = BLE_MBUF_HDR_PTR(m);
            pktlen = OS_MBUF_PKTLEN(m);
            cur_txlen = ble_hdr->txinfo.pyld_len;
            cur_offset = ble_hdr->txinfo.offset;
            if (cur_offset == 0) {
                hdr_byte = ble_hdr->txinfo.hdr_byte & BLE_LL_DATA_HDR_LLID_MASK;
            }
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
            if (connsm->enc_data.enc_state > CONN_ENC_S_ENCRYPTED) {
                /* We will allow a next packet if it itself is allowed */
                pkthdr = OS_MBUF_PKTHDR(connsm->cur_tx_pdu);
                if (nextpkthdr && !ble_ll_ctrl_enc_allowed_pdu_tx(nextpkthdr)
                    && (CONN_IS_CENTRAL(connsm) ||
                        !ble_ll_ctrl_is_start_enc_rsp(connsm->cur_tx_pdu))) {
                    nextpkthdr = NULL;
                }
            }
#endif
        } else {
            /* Empty PDU here. NOTE: header byte gets set later */
            pktlen = 0;
            cur_txlen = 0;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
            if (connsm->enc_data.enc_state > CONN_ENC_S_ENCRYPTED) {
                /* We will allow a next packet if it itself is allowed */
                if (nextpkthdr && !ble_ll_ctrl_enc_allowed_pdu_tx(nextpkthdr)) {
                    nextpkthdr = NULL;
                }
            }
#endif
        }
    }

    /*
     * Set the more data data flag if we have more data to send and we
     * have not been asked to terminate
     */
    if (nextpkthdr || ((cur_offset + cur_txlen) < pktlen)) {
        /* Get next event time */
        next_event_time = ble_ll_conn_get_next_sched_time(connsm);

        /* XXX: TODO: need to check this with phy update procedure. There are
           limitations if we have started update */

        /*
         * Dont bother to set the MD bit if we cannot do the following:
         *  -> wait IFS, send the current frame.
         *  -> wait IFS, receive a maximum size frame.
         *  -> wait IFS, send the next frame.
         *  -> wait IFS, receive a maximum size frame.
         *
         *  For peripheral:
         *  -> wait IFS, send current frame.
         *  -> wait IFS, receive maximum size frame.
         *  -> wait IFS, send next frame.
         */
        if ((cur_offset + cur_txlen) < pktlen) {
            next_txlen = pktlen - (cur_offset + cur_txlen);
        } else {
            next_txlen = connsm->eff_max_tx_octets;
        }
        if (next_txlen > connsm->eff_max_tx_octets) {
            next_txlen = connsm->eff_max_tx_octets;
        }

        /*
         * XXX: this calculation is based on using the current time
         * and assuming the transmission will occur an IFS time from
         * now. This is not the most accurate especially if we have
         * received a frame and we are replying to it.
         */
#if BLE_LL_BT5_PHY_SUPPORTED
        tx_phy_mode = connsm->phy_data.tx_phy_mode;
#else
        tx_phy_mode = BLE_PHY_MODE_1M;
#endif

        ticks = (BLE_LL_IFS * 3) + connsm->ota_max_rx_time +
                ble_ll_pdu_us(next_txlen, tx_phy_mode) +
                ble_ll_pdu_us(cur_txlen, tx_phy_mode);

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
        if (connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
            ticks += (BLE_LL_IFS + connsm->ota_max_rx_time);
        }
#endif

        ticks = ble_ll_tmr_u2t(ticks);
        if (LL_TMR_LT(ble_ll_tmr_get() + ticks, next_event_time)) {
            md = 1;
        }
    }

    /* If we send an empty PDU we need to initialize the header */
conn_tx_pdu:
    if (connsm->flags.empty_pdu_txd) {
        /*
         * This looks strange, but we dont use the data pointer in the mbuf
         * when we have an empty pdu.
         */
        m = (struct os_mbuf *)&empty_pdu;
        m->om_data = (uint8_t *)&empty_pdu;
        m->om_data += BLE_MBUF_MEMBLOCK_OVERHEAD;
        ble_hdr = &empty_pdu.ble_hdr;
        ble_hdr->txinfo.flags = 0;
        ble_hdr->txinfo.offset = 0;
        ble_hdr->txinfo.pyld_len = 0;
    }

    /* Set tx seqnum */
    if (connsm->tx_seqnum) {
        hdr_byte |= BLE_LL_DATA_HDR_SN_MASK;
    }

    /* If we have more data, set the bit */
    if (md) {
        hdr_byte |= BLE_LL_DATA_HDR_MD_MASK;
    }

    /* Set NESN (next expected sequence number) bit */
    if (connsm->next_exp_seqnum) {
        hdr_byte |= BLE_LL_DATA_HDR_NESN_MASK;
    }

    /* Set the header byte in the outgoing frame */
    ble_hdr->txinfo.hdr_byte = hdr_byte;

    /*
     * If we are a peripheral, check to see if this transmission will end the
     * connection event. We will end the connection event if we have
     * received a valid frame with the more data bit set to 0 and we dont
     * have more data.
     *
     * XXX: for a peripheral, we dont check to see if we can:
     *  -> wait IFS, rx frame from central (either big or small).
     *  -> wait IFS, send empty pdu or next pdu.
     *
     *  We could do this. Now, we just keep going and hope that we dont
     *  overrun next scheduled item.
     */
    if ((connsm->flags.terminate_ind_rxd) ||
        (CONN_IS_PERIPHERAL(connsm) && (md == 0) &&
         (connsm->cons_rxd_bad_crc == 0) &&
         ((connsm->last_rxd_hdr_byte & BLE_LL_DATA_HDR_MD_MASK) == 0) &&
         !ble_ll_ctrl_is_terminate_ind(hdr_byte, m->om_data[0]))) {
        /* We will end the connection event */
        end_transition = BLE_PHY_TRANSITION_NONE;
        txend_func = ble_ll_conn_wait_txend;
    } else {
        /* Wait for a response here */
        end_transition = BLE_PHY_TRANSITION_TX_RX;
        txend_func = NULL;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
    llid = ble_hdr->txinfo.hdr_byte & BLE_LL_DATA_HDR_LLID_MASK;
    if (llid == BLE_LL_LLID_CTRL) {
        is_ctrl = 1;
        opcode = m->om_data[0];
    } else {
        is_ctrl = 0;
        opcode = 0;
    }

    if (is_ctrl && (opcode == BLE_LL_CTRL_START_ENC_RSP)) {
        /*
         * Both central and peripheral send the START_ENC_RSP encrypted and receive
         * encrypted
         */
        connsm->flags.encrypted = 1;
        connsm->enc_data.tx_encrypted = 1;
        ble_phy_encrypt_enable(connsm->enc_data.enc_block.cipher_text);
        ble_phy_encrypt_iv_set(connsm->enc_data.iv);
        ble_phy_encrypt_counter_set(connsm->enc_data.tx_pkt_cntr,
                                    CONN_IS_CENTRAL(connsm));
        if (txend_func == NULL) {
            txend_func = ble_ll_conn_continue_rx_encrypt;
        }
    } else if (is_ctrl && (opcode == BLE_LL_CTRL_START_ENC_REQ)) {
        /*
         * Only the peripheral sends this and it gets sent unencrypted but
         * we receive encrypted
         */
        connsm->flags.encrypted = 0;
        connsm->enc_data.enc_state = CONN_ENC_S_START_ENC_RSP_WAIT;
        connsm->enc_data.tx_encrypted = 0;
        ble_phy_encrypt_disable();
        if (txend_func == NULL) {
            txend_func = ble_ll_conn_start_rx_encrypt;
        } else {
            txend_func = ble_ll_conn_txend_encrypt;
        }
    } else if (is_ctrl && (opcode == BLE_LL_CTRL_PAUSE_ENC_RSP)) {
        /*
         * The peripheral sends the PAUSE_ENC_RSP encrypted. The central sends
         * it unencrypted (note that link was already set unencrypted).
         */
        switch (connsm->conn_role) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
        case BLE_LL_CONN_ROLE_CENTRAL:
            connsm->flags.encrypted = 0;
            connsm->enc_data.enc_state = CONN_ENC_S_PAUSED;
            connsm->enc_data.tx_encrypted = 0;
            ble_phy_encrypt_disable();
            break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
        case BLE_LL_CONN_ROLE_PERIPHERAL:
            connsm->flags.encrypted = 1;
            connsm->enc_data.tx_encrypted = 1;
            ble_phy_encrypt_enable(connsm->enc_data.enc_block.cipher_text);
            ble_phy_encrypt_iv_set(connsm->enc_data.iv);
            ble_phy_encrypt_counter_set(connsm->enc_data.tx_pkt_cntr,
                                        CONN_IS_CENTRAL(connsm));
            if (txend_func == NULL) {
                txend_func = ble_ll_conn_start_rx_unencrypt;
            } else {
                txend_func = ble_ll_conn_rxend_unencrypt;
            }
            break;
#endif
        default:
            BLE_LL_ASSERT(0);
            break;
        }
    } else {
        /* If encrypted set packet counter */
        if (connsm->flags.encrypted) {
            connsm->enc_data.tx_encrypted = 1;
            ble_phy_encrypt_counter_set(connsm->enc_data.tx_pkt_cntr,
                                        CONN_IS_CENTRAL(connsm));
            if (txend_func == NULL) {
                txend_func = ble_ll_conn_continue_rx_encrypt;
            }
        }
    }
#endif

    /* Set transmit end callback */
    ble_phy_set_txend_cb(txend_func, connsm);
    rc = ble_phy_tx(ble_ll_tx_mbuf_pducb, m, end_transition);
    if (!rc) {
        /* Log transmit on connection state */
        cur_txlen = ble_hdr->txinfo.pyld_len;
        ble_ll_trace_u32x2(BLE_LL_TRACE_ID_CONN_TX, cur_txlen,
                           ble_hdr->txinfo.offset);

        /* Set last transmitted MD bit */
        connsm->flags.last_txd_md = md;

        /* Increment packets transmitted */
        if (connsm->flags.empty_pdu_txd) {
            if (connsm->flags.terminate_ind_rxd) {
                connsm->flags.terminate_ind_rxd_acked = 1;
            }
            STATS_INC(ble_ll_conn_stats, tx_empty_pdus);
        } else if ((hdr_byte & BLE_LL_DATA_HDR_LLID_MASK) == BLE_LL_LLID_CTRL) {
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
            connsm->has_nonempty_pdu = 1;
#endif
            STATS_INC(ble_ll_conn_stats, tx_ctrl_pdus);
            STATS_INCN(ble_ll_conn_stats, tx_ctrl_bytes, cur_txlen);
        } else {
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
            connsm->has_nonempty_pdu = 1;
#endif
            STATS_INC(ble_ll_conn_stats, tx_l2cap_pdus);
            STATS_INCN(ble_ll_conn_stats, tx_l2cap_bytes, cur_txlen);
        }
    }
    return rc;
}

/**
 * Schedule callback for start of connection event.
 *
 * Context: Interrupt
 *
 * @param sch
 *
 * @return int 0: scheduled item is still running. 1: schedule item is done.
 */
static int
ble_ll_conn_event_start_cb(struct ble_ll_sched_item *sch)
{
    int rc = 0;
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    uint32_t usecs;
#endif
    uint32_t start;
    struct ble_ll_conn_sm *connsm;

    /* XXX: note that we can extend end time here if we want. Look at this */

    /* Set current connection state machine */
    connsm = (struct ble_ll_conn_sm *)sch->cb_arg;
    g_ble_ll_conn_cur_sm = connsm;
    BLE_LL_ASSERT(connsm);

    /* In rare cases 1st connection event is fired before LL finished processing
     * new connection. In such case just skip this connection event and LL will
     * reschedule to next connection event.
     */
    if (connsm->conn_state == BLE_LL_CONN_STATE_IDLE) {
        ble_ll_conn_current_sm_over(connsm);
        return BLE_LL_SCHED_STATE_DONE;
    }

    /* Log connection event start */
    ble_ll_trace_u32(BLE_LL_TRACE_ID_CONN_EV_START, connsm->conn_handle);

    /* Disable whitelisting as connections do not use it */
    ble_ll_whitelist_disable();

    /* Set LL state */
    ble_ll_state_set(BLE_LL_STATE_CONNECTION);

    /* Set channel */
    ble_phy_setchan(connsm->data_chan_index, connsm->access_addr,
                    connsm->crcinit);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    ble_phy_resolv_list_disable();
#endif

#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    ble_phy_mode_set(connsm->phy_data.tx_phy_mode, connsm->phy_data.rx_phy_mode);
#endif

    /* Set the power */
    ble_ll_tx_power_set(g_ble_ll_tx_power);

    switch (connsm->conn_role) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    case BLE_LL_CONN_ROLE_CENTRAL:
        /* Set start time of transmission */
        start = sch->start_time + g_ble_ll_sched_offset_ticks;
        rc = ble_phy_tx_set_start_time(start, sch->remainder);
        if (!rc) {
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
            if (connsm->flags.encrypted) {
                ble_phy_encrypt_enable(connsm->enc_data.enc_block.cipher_text);
                ble_phy_encrypt_iv_set(connsm->enc_data.iv);
                ble_phy_encrypt_counter_set(connsm->enc_data.tx_pkt_cntr, 1);
            } else {
                ble_phy_encrypt_disable();
            }
#endif
            rc = ble_ll_conn_tx_pdu(connsm);
            if (!rc) {
                rc = BLE_LL_SCHED_STATE_RUNNING;
            } else {
                /* Inform LL task of connection event end */
                rc = BLE_LL_SCHED_STATE_DONE;
            }
        } else {
            STATS_INC(ble_ll_conn_stats, conn_ev_late);
            rc = BLE_LL_SCHED_STATE_DONE;
        }
        break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    case BLE_LL_CONN_ROLE_PERIPHERAL:
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
        if (connsm->flags.encrypted) {
            ble_phy_encrypt_enable(connsm->enc_data.enc_block.cipher_text);
            ble_phy_encrypt_iv_set(connsm->enc_data.iv);
            ble_phy_encrypt_counter_set(connsm->enc_data.rx_pkt_cntr, 1);
        } else {
            ble_phy_encrypt_disable();
        }
#endif

        /* XXX: what is this really for the peripheral? */
        start = sch->start_time + g_ble_ll_sched_offset_ticks;
        rc = ble_phy_rx_set_start_time(start, sch->remainder);
        if (rc) {
            /* End the connection event as we have no more buffers */
            STATS_INC(ble_ll_conn_stats, periph_ce_failures);
            rc = BLE_LL_SCHED_STATE_DONE;
        } else {
            /*
             * Set flag that tells peripheral to set last anchor point if a packet
             * has been received.
             */
            connsm->flags.periph_set_last_anchor = 1;

            /*
             * Set the wait for response time. The anchor point is when we
             * expect the central to start transmitting. Worst-case, we expect
             * to hear a reply within the anchor point plus:
             *  -> current tx window size
             *  -> current window widening amount (includes +/- 16 usec jitter)
             *  -> Amount of time it takes to detect packet start.
             *  -> Some extra time (16 usec) to insure timing is OK
             */

            /*
             * For the 32 kHz crystal, the amount of usecs we have to wait
             * is not from the anchor point; we have to account for the time
             * from when the receiver is enabled until the anchor point. The
             * time we start before the anchor point is this:
             *   -> current window widening.
             *   -> up to one 32 kHz tick since we discard remainder.
             *   -> Up to one tick since the usecs to ticks calc can be off
             *   by up to one tick.
             * NOTES:
             * 1) the 61 we add is for the two ticks mentioned above.
             * 2) The address rx time and jitter is accounted for in the
             * phy function
             */
            usecs = connsm->periph_cur_tx_win_usecs + 61 +
                    (2 * connsm->periph_cur_window_widening);
            ble_phy_wfr_enable(BLE_PHY_WFR_ENABLE_RX, 0, usecs);
            /* Set next wakeup time to connection event end time */
            rc = BLE_LL_SCHED_STATE_RUNNING;
        }
        break;
#endif
    default:
        BLE_LL_ASSERT(0);
        break;
    }

    if (rc == BLE_LL_SCHED_STATE_DONE) {
        ble_ll_conn_current_sm_over(connsm);
    }

    /* Set time that we last serviced the schedule */
    connsm->last_scheduled = ble_ll_tmr_get();
    return rc;
}

/**
 * Called to determine if the device is allowed to send the next pdu in the
 * connection event. This will always return 'true' if we are a peripheral. If we
 * are a central, we must be able to send the next fragment and get a minimum
 * sized response from the peripheral.
 *
 * Context: Interrupt context (rx end isr).
 *
 * @param connsm
 * @param begtime   Time at which IFS before pdu transmission starts
 *
 * @return int 0: not allowed to send 1: allowed to send
 */
static int
ble_ll_conn_can_send_next_pdu(struct ble_ll_conn_sm *connsm, uint32_t begtime,
                              uint32_t add_usecs)
{
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    int rc;
    uint16_t rem_bytes;
    uint32_t ticks;
    uint32_t usecs;
    uint32_t next_sched_time;
    struct os_mbuf *txpdu;
    struct os_mbuf_pkthdr *pkthdr;
    struct ble_mbuf_hdr *txhdr;
    uint32_t allowed_usecs;
    int tx_phy_mode;

#if BLE_LL_BT5_PHY_SUPPORTED
    tx_phy_mode = connsm->phy_data.tx_phy_mode;
#else
    tx_phy_mode = BLE_PHY_MODE_1M;
#endif

    rc = 1;
    if (connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
        /* Get next scheduled item time */
        next_sched_time = ble_ll_conn_get_next_sched_time(connsm);

        txpdu = connsm->cur_tx_pdu;
        if (!txpdu) {
            pkthdr = STAILQ_FIRST(&connsm->conn_txq);
            if (pkthdr) {
                txpdu = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);
            }
        } else {
            pkthdr = OS_MBUF_PKTHDR(txpdu);
        }

        /* XXX: TODO: need to check this with phy update procedure. There are
           limitations if we have started update */
        if (txpdu) {
            txhdr = BLE_MBUF_HDR_PTR(txpdu);
            rem_bytes = pkthdr->omp_len - txhdr->txinfo.offset;
            if (rem_bytes > connsm->eff_max_tx_octets) {
                rem_bytes = connsm->eff_max_tx_octets;
            }
            usecs = ble_ll_pdu_us(rem_bytes, tx_phy_mode);
        } else {
            /* We will send empty pdu (just a LL header) */
            usecs = ble_ll_pdu_us(0, tx_phy_mode);
        }
        usecs += (BLE_LL_IFS * 2) + connsm->ota_max_rx_time;

        ticks = (uint32_t)(next_sched_time - begtime);
        allowed_usecs = ble_ll_tmr_t2u(ticks);
        if ((usecs + add_usecs) >= allowed_usecs) {
            rc = 0;
        }
    }

    return rc;
#else
    return 1;
#endif
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_PING)
/**
 * Callback for the Authenticated payload timer. This function is called
 * when the authenticated payload timer expires. When the authenticated
 * payload timeout expires, we should
 *  -> Send the authenticated payload timeout event.
 *  -> Start the LE ping procedure.
 *  -> Restart the timer.
 *
 * @param arg
 */
void
ble_ll_conn_auth_pyld_timer_cb(struct ble_npl_event *ev)
{
    struct ble_ll_conn_sm *connsm;

    connsm = (struct ble_ll_conn_sm *)ble_npl_event_get_arg(ev);
    ble_ll_auth_pyld_tmo_event_send(connsm);
    ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_LE_PING, NULL);
    ble_ll_conn_auth_pyld_timer_start(connsm);
}

/**
 * Start (or restart) the authenticated payload timer
 *
 * @param connsm
 */
void
ble_ll_conn_auth_pyld_timer_start(struct ble_ll_conn_sm *connsm)
{
    int32_t tmo;

    /* Timeout in is in 10 msec units */
    tmo = (int32_t)BLE_LL_CONN_AUTH_PYLD_OS_TMO(connsm->auth_pyld_tmo);
    ble_npl_callout_reset(&connsm->auth_pyld_timer, tmo);
}
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
static void
ble_ll_conn_central_common_init(struct ble_ll_conn_sm *connsm)
{

    /* Set central role */
    connsm->conn_role = BLE_LL_CONN_ROLE_CENTRAL;

    /* Set default ce parameters */

    /*
     * XXX: for now, we need twice the transmit window as our calculations
     * for the transmit window offset could be off.
     */
    connsm->tx_win_size = BLE_LL_CONN_TX_WIN_MIN + 1;
    connsm->tx_win_off = 0;
    connsm->central_sca = BLE_LL_SCA_ENUM;

    /* Hop increment is a random value between 5 and 16. */
    connsm->hop_inc = (ble_ll_rand() % 12) + 5;

    /* Set channel map to map requested by host */
    connsm->chan_map_used = g_ble_ll_data.chan_map_used;
    memcpy(connsm->chan_map, g_ble_ll_data.chan_map, BLE_LL_CHAN_MAP_LEN);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
    connsm->acc_subrate_min = g_ble_ll_conn_params.acc_subrate_min;
    connsm->acc_subrate_max = g_ble_ll_conn_params.acc_subrate_max;
    connsm->acc_max_latency = g_ble_ll_conn_params.acc_max_latency;
    connsm->acc_cont_num = g_ble_ll_conn_params.acc_cont_num;
    connsm->acc_supervision_tmo = g_ble_ll_conn_params.acc_supervision_tmo;
#endif

    /*  Calculate random access address and crc initialization value */
    connsm->access_addr = ble_ll_utils_calc_aa();
    connsm->crcinit = ble_ll_rand() & 0xffffff;

    /* Set initial schedule callback */
    connsm->conn_sch.sched_cb = ble_ll_conn_event_start_cb;
}
/**
 * Called when a create connection command has been received. This initializes
 * a connection state machine in the central role.
 *
 * NOTE: Must be called before the state machine is started
 *
 * @param connsm
 * @param hcc
 */
void
ble_ll_conn_central_init(struct ble_ll_conn_sm *connsm,
                         struct ble_ll_conn_create_scan *cc_scan,
                         struct ble_ll_conn_create_params *cc_params)
{

    ble_ll_conn_central_common_init(connsm);

    connsm->own_addr_type = cc_scan->own_addr_type;
    memcpy(&connsm->peer_addr, &cc_scan->peer_addr, BLE_DEV_ADDR_LEN);
    connsm->peer_addr_type = cc_scan->peer_addr_type;

    connsm->conn_itvl = cc_params->conn_itvl;
    connsm->conn_itvl_ticks = cc_params->conn_itvl_ticks;
    connsm->conn_itvl_usecs = cc_params->conn_itvl_usecs;
    connsm->periph_latency = cc_params->conn_latency;
    connsm->supervision_tmo = cc_params->supervision_timeout;
    connsm->max_ce_len_ticks = ble_ll_tmr_u2t_up(cc_params->max_ce_len * BLE_LL_CONN_CE_USECS);
}
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_DATA_LEN_EXT)
int
ble_ll_conn_set_data_len(struct ble_ll_conn_sm *connsm,
                         uint16_t tx_octets, uint16_t tx_time,
                         uint16_t rx_octets, uint16_t rx_time)
{
    int init_dle = 0;

    /* Note: octets/time shall be checked by caller! */

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    /* Keep original values requested by host since we may want to recalculate
     * after PHY changes between coded and uncoded.
     */
    connsm->host_req_max_tx_time = tx_time;
    connsm->host_req_max_rx_time = rx_time;

    /* If peer does not support coded, we cannot use value larger than 2120us */
    if (!ble_ll_conn_rem_feature_check(connsm, BLE_LL_FEAT_LE_CODED_PHY)) {
        tx_time = MIN(tx_time, BLE_LL_CONN_SUPP_TIME_MAX_UNCODED);
        rx_time = MIN(rx_time, BLE_LL_CONN_SUPP_TIME_MAX_UNCODED);
    }
#endif

    if (connsm->max_tx_time != tx_time) {
        connsm->max_tx_time = tx_time;
        init_dle = 1;
    }

    if (connsm->max_tx_octets != tx_octets) {
        connsm->max_tx_octets = tx_octets;
        init_dle = 1;
    }

    if (rx_time && (connsm->max_rx_time != rx_time)) {
        connsm->max_rx_time = rx_time;
        init_dle = 1;
    }

    if (rx_octets && (connsm->max_rx_octets != rx_octets)) {
        connsm->max_rx_octets = rx_octets;
        init_dle = 1;
    }

    if (init_dle) {
        ble_ll_ctrl_initiate_dle(connsm, false);
    }

    return 0;
}
#endif

#if (BLE_LL_BT5_PHY_SUPPORTED == 1)

static void
ble_ll_conn_set_phy(struct ble_ll_conn_sm *connsm, int tx_phy, int rx_phy)
{

    struct ble_ll_conn_phy_data *phy_data = &connsm->phy_data;

    phy_data->rx_phy_mode = ble_ll_phy_to_phy_mode(rx_phy,
                                                   BLE_HCI_LE_PHY_CODED_ANY);
    phy_data->cur_rx_phy = rx_phy;

    phy_data->tx_phy_mode = ble_ll_phy_to_phy_mode(tx_phy,
                                                   BLE_HCI_LE_PHY_CODED_ANY);
    phy_data->cur_tx_phy = tx_phy;

}

static void
ble_ll_conn_init_phy(struct ble_ll_conn_sm *connsm, int phy)
{
    struct ble_ll_conn_global_params *conngp;

    /* Always initialize symmetric PHY - controller can change this later */
    ble_ll_conn_set_phy(connsm, phy, phy);

    /* Update data length management to match initial PHY */
    conngp = &g_ble_ll_conn_params;
    connsm->max_tx_octets = conngp->conn_init_max_tx_octets;
    connsm->max_rx_octets = conngp->supp_max_rx_octets;
    if (phy == BLE_PHY_CODED) {
        connsm->max_tx_time = conngp->conn_init_max_tx_time_coded;
        connsm->max_rx_time = BLE_LL_CONN_SUPP_TIME_MAX_CODED;
        connsm->rem_max_tx_time = BLE_LL_CONN_SUPP_TIME_MIN_CODED;
        connsm->rem_max_rx_time = BLE_LL_CONN_SUPP_TIME_MIN_CODED;
        /* Assume peer does support coded */
        ble_ll_conn_rem_feature_add(connsm, BLE_LL_FEAT_LE_CODED_PHY);
    } else {
        connsm->max_tx_time = conngp->conn_init_max_tx_time_uncoded;
        connsm->max_rx_time = BLE_LL_CONN_SUPP_TIME_MAX_UNCODED;
        connsm->rem_max_tx_time = BLE_LL_CONN_SUPP_TIME_MIN_UNCODED;
        connsm->rem_max_rx_time = BLE_LL_CONN_SUPP_TIME_MIN_UNCODED;
    }
    connsm->eff_max_tx_time = connsm->rem_max_tx_time;
    connsm->eff_max_rx_time = connsm->rem_max_rx_time;
    connsm->rem_max_tx_octets = BLE_LL_CONN_SUPP_BYTES_MIN;
    connsm->rem_max_rx_octets = BLE_LL_CONN_SUPP_BYTES_MIN;
    connsm->eff_max_tx_octets = BLE_LL_CONN_SUPP_BYTES_MIN;
    connsm->eff_max_rx_octets = BLE_LL_CONN_SUPP_BYTES_MIN;
}

#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
static void
ble_ll_conn_create_set_params(struct ble_ll_conn_sm *connsm, uint8_t phy)
{
    struct ble_ll_conn_create_params *cc_params;

    cc_params = &g_ble_ll_conn_create_sm.params[phy - 1];

    connsm->periph_latency = cc_params->conn_latency;
    connsm->supervision_tmo = cc_params->supervision_timeout;

    connsm->conn_itvl = cc_params->conn_itvl;
    connsm->conn_itvl_ticks = cc_params->conn_itvl_ticks;
    connsm->conn_itvl_usecs = cc_params->conn_itvl_usecs;
}
#endif
#endif

static void
ble_ll_conn_set_csa(struct ble_ll_conn_sm *connsm, bool chsel)
{
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CSA2)
    if (chsel) {
        connsm->flags.csa2 = 1;
        connsm->channel_id = ((connsm->access_addr & 0xffff0000) >> 16) ^
                              (connsm->access_addr & 0x0000ffff);

        /* calculate the next data channel */
        connsm->data_chan_index = ble_ll_conn_calc_dci(connsm, 0);
        return;
    }
#endif

    connsm->last_unmapped_chan = 0;

    /* calculate the next data channel */
    connsm->data_chan_index = ble_ll_conn_calc_dci(connsm, 1);
}

/**
 * Create a new connection state machine. This is done once per
 * connection when the HCI command "create connection" is issued to the
 * controller or when a peripheral receives a connect request.
 *
 * Context: Link Layer task
 *
 * @param connsm
 */
void
ble_ll_conn_sm_new(struct ble_ll_conn_sm *connsm)
{
    struct ble_ll_conn_global_params *conn_params;

    /* Reset following elements */
    memset(&connsm->flags, 0, sizeof(connsm->flags));
    connsm->event_cntr = 0;
    connsm->conn_state = BLE_LL_CONN_STATE_IDLE;
    connsm->disconnect_reason = 0;
    connsm->rxd_disconnect_reason = 0;
    connsm->conn_features = BLE_LL_CONN_INITIAL_FEATURES;
    memset(connsm->remote_features, 0, sizeof(connsm->remote_features));
    connsm->vers_nr = 0;
    connsm->comp_id = 0;
    connsm->sub_vers_nr = 0;
    connsm->reject_reason = BLE_ERR_SUCCESS;
    connsm->conn_rssi = BLE_LL_CONN_UNKNOWN_RSSI;
    connsm->inita_identity_used = 0;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
    connsm->subrate_base_event = 0;
    connsm->subrate_factor = 1;
    connsm->cont_num = 0;
    connsm->cont_num_left = 0;
    connsm->has_nonempty_pdu = 0;
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV_SYNC_TRANSFER)
    connsm->sync_transfer_sync_timeout = g_ble_ll_conn_sync_transfer_params.sync_timeout_us;
    connsm->sync_transfer_mode = g_ble_ll_conn_sync_transfer_params.mode;
    connsm->sync_transfer_skip = g_ble_ll_conn_sync_transfer_params.max_skip;
#endif

    /* XXX: TODO set these based on PHY that started connection */
#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    connsm->phy_data.cur_tx_phy = BLE_PHY_1M;
    connsm->phy_data.cur_rx_phy = BLE_PHY_1M;
    connsm->phy_data.tx_phy_mode = BLE_PHY_MODE_1M;
    connsm->phy_data.rx_phy_mode = BLE_PHY_MODE_1M;
    connsm->phy_data.pref_mask_tx_req = 0;
    connsm->phy_data.pref_mask_rx_req = 0;
    connsm->phy_data.pref_mask_tx = g_ble_ll_data.ll_pref_tx_phys;
    connsm->phy_data.pref_mask_rx = g_ble_ll_data.ll_pref_rx_phys;
    connsm->phy_data.pref_opts = 0;
    connsm->phy_tx_transition = 0;
#endif

    /* Reset current control procedure */
    connsm->cur_ctrl_proc = BLE_LL_CTRL_PROC_IDLE;
    connsm->pending_ctrl_procs = 0;

    /*
     * Set handle in connection update procedure to 0. If the handle
     * is non-zero it means that the host initiated the connection
     * parameter update request and the rest of the parameters are valid.
     */
    connsm->conn_param_req.handle = 0;

    /* Connection end event */
    ble_npl_event_init(&connsm->conn_ev_end, ble_ll_conn_event_end, connsm);

    /* Initialize transmit queue and ack/flow control elements */
    STAILQ_INIT(&connsm->conn_txq);
    connsm->cur_tx_pdu = NULL;
    connsm->tx_seqnum = 0;
    connsm->next_exp_seqnum = 0;
    connsm->cons_rxd_bad_crc = 0;
    connsm->last_rxd_sn = 1;
    connsm->completed_pkts = 0;

    /* initialize data length mgmt */
    conn_params = &g_ble_ll_conn_params;
    connsm->max_tx_octets = conn_params->conn_init_max_tx_octets;
    connsm->max_rx_octets = conn_params->supp_max_rx_octets;
    connsm->max_tx_time = conn_params->conn_init_max_tx_time;
    connsm->max_rx_time = conn_params->supp_max_rx_time;
    connsm->rem_max_tx_time = BLE_LL_CONN_SUPP_TIME_MIN;
    connsm->rem_max_rx_time = BLE_LL_CONN_SUPP_TIME_MIN;
    connsm->eff_max_tx_time = BLE_LL_CONN_SUPP_TIME_MIN;
    connsm->eff_max_rx_time = BLE_LL_CONN_SUPP_TIME_MIN;
    connsm->ota_max_rx_time = BLE_LL_CONN_SUPP_TIME_MIN;
    connsm->rem_max_tx_octets = BLE_LL_CONN_SUPP_BYTES_MIN;
    connsm->rem_max_rx_octets = BLE_LL_CONN_SUPP_BYTES_MIN;
    connsm->eff_max_tx_octets = BLE_LL_CONN_SUPP_BYTES_MIN;
    connsm->eff_max_rx_octets = BLE_LL_CONN_SUPP_BYTES_MIN;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    connsm->host_req_max_tx_time = 0;
    connsm->host_req_max_rx_time = 0;
#endif

    /* Reset encryption data */
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
    memset(&connsm->enc_data, 0, sizeof(struct ble_ll_conn_enc_data));
    connsm->enc_data.enc_state = CONN_ENC_S_UNENCRYPTED;
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_PING)
    connsm->auth_pyld_tmo = BLE_LL_CONN_DEF_AUTH_PYLD_TMO;
    connsm->flags.le_ping_supp = 1;
#endif

    /* Add to list of active connections */
    SLIST_INSERT_HEAD(&g_ble_ll_conn_active_list, connsm, act_sle);

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    if (ble_ll_sched_css_is_enabled() &&
        (connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL)) {
        ble_ll_conn_css_update_list(connsm);
    }
#endif
}

void
ble_ll_conn_update_eff_data_len(struct ble_ll_conn_sm *connsm)
{
    int ota_max_rx_time_calc = 0;
    int send_event;
    uint16_t eff_time;
    uint16_t eff_bytes;
    uint16_t ota_time;
    uint8_t phy_mode;

    /* Assume no event sent */
    send_event = 0;

    /* See if effective times have changed */
    eff_time = MIN(connsm->rem_max_tx_time, connsm->max_rx_time);
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    if (connsm->phy_data.cur_rx_phy == BLE_PHY_CODED) {
        eff_time = MAX(eff_time, BLE_LL_CONN_SUPP_TIME_MIN_CODED);
    }
#endif
    if (eff_time != connsm->eff_max_rx_time) {
        connsm->eff_max_rx_time = eff_time;
        ota_max_rx_time_calc = 1;
        send_event = 1;
    }
    eff_time = MIN(connsm->rem_max_rx_time, connsm->max_tx_time);
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    if (connsm->phy_data.cur_tx_phy == BLE_PHY_CODED) {
        eff_time = MAX(eff_time, BLE_LL_CONN_SUPP_TIME_MIN_CODED);
    }
#endif
    if (eff_time != connsm->eff_max_tx_time) {
        connsm->eff_max_tx_time = eff_time;
        send_event = 1;
    }
    eff_bytes = MIN(connsm->rem_max_tx_octets, connsm->max_rx_octets);
    if (eff_bytes != connsm->eff_max_rx_octets) {
        connsm->eff_max_rx_octets = eff_bytes;
        ota_max_rx_time_calc = 1;
        send_event = 1;
    }
    eff_bytes = MIN(connsm->rem_max_rx_octets, connsm->max_tx_octets);
    if (eff_bytes != connsm->eff_max_tx_octets) {
        connsm->eff_max_tx_octets = eff_bytes;
        send_event = 1;
    }

    /* If effective rx octets and/or time value changes, we need to calculate
     * actual OTA max rx time, i.e. lesser of effective max rx time and rx time
     * of PDU containing max rx octets of payload. This is then used to calculate
     * connection events timings.
     */
    if (ota_max_rx_time_calc) {
#if BLE_LL_BT5_PHY_SUPPORTED
        phy_mode = ble_ll_phy_to_phy_mode(connsm->phy_data.cur_rx_phy,
                                          BLE_HCI_LE_PHY_CODED_S8_PREF);
#else
        phy_mode = BLE_PHY_MODE_1M;
#endif
        ota_time = ble_ll_pdu_us(connsm->eff_max_rx_octets, phy_mode);
        connsm->ota_max_rx_time = MIN(ota_time, connsm->eff_max_rx_time);
    }

    if (send_event) {
        ble_ll_hci_ev_datalen_chg(connsm);
    }
}

/**
 * Called when a connection is terminated
 *
 * Context: Link Layer task.
 *
 * @param connsm
 * @param ble_err
 */
void
ble_ll_conn_end(struct ble_ll_conn_sm *connsm, uint8_t ble_err)
{
    struct os_mbuf *m;
    struct os_mbuf_pkthdr *pkthdr;
    os_sr_t sr;

    /* Remove scheduler events just in case */
    ble_ll_sched_rmv_elem(&connsm->conn_sch);

    /* In case of the supervision timeout we shall make sure
     * that there is no ongoing connection event. It could happen
     * because we scheduled connection event before checking connection timeout.
     * If connection event managed to start, let us drop it.
     */
    OS_ENTER_CRITICAL(sr);
    if (g_ble_ll_conn_cur_sm == connsm) {
        ble_ll_conn_halt();
        STATS_INC(ble_ll_conn_stats, conn_event_while_tmo);
    }
    OS_EXIT_CRITICAL(sr);

    /* Stop any control procedures that might be running */
    ble_npl_callout_stop(&connsm->ctrl_proc_rsp_timer);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_PING)
    ble_npl_callout_stop(&connsm->auth_pyld_timer);
#endif

    /* Remove from the active connection list */
    SLIST_REMOVE(&g_ble_ll_conn_active_list, connsm, ble_ll_conn_sm, act_sle);

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    if (ble_ll_sched_css_is_enabled() &&
        (connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL)) {
        /* If current connection was reference for CSS, we need to find another
         * one. It does not matter which one we'll pick.
         */
        OS_ENTER_CRITICAL(sr);
        SLIST_REMOVE(&g_ble_ll_conn_css_list, connsm, ble_ll_conn_sm, css_sle);
        if (connsm == g_ble_ll_conn_css_ref) {
            g_ble_ll_conn_css_ref = SLIST_FIRST(&g_ble_ll_conn_css_list);
        }
        OS_EXIT_CRITICAL(sr);
    }
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_CTRL_TO_HOST_FLOW_CONTROL)
    ble_ll_conn_cth_flow_free_credit(connsm, connsm->cth_flow_pending);
#endif

    /* Free the current transmit pdu if there is one. */
    if (connsm->cur_tx_pdu) {
        os_mbuf_free_chain(connsm->cur_tx_pdu);
        connsm->cur_tx_pdu = NULL;
    }

    /* Free all packets on transmit queue */
    while (1) {
        /* Get mbuf pointer from packet header pointer */
        pkthdr = STAILQ_FIRST(&connsm->conn_txq);
        if (!pkthdr) {
            break;
        }
        STAILQ_REMOVE_HEAD(&connsm->conn_txq, omp_next);

        m = (struct os_mbuf *)((uint8_t *)pkthdr - sizeof(struct os_mbuf));
        os_mbuf_free_chain(m);
    }

    /* Make sure events off queue */
    ble_ll_event_remove(&connsm->conn_ev_end);

    /* Connection state machine is now idle */
    connsm->conn_state = BLE_LL_CONN_STATE_IDLE;

    /*
     * If we have features and there's pending HCI command, send an event before
     * disconnection event so it does make sense to host.
     */
    if (connsm->flags.features_host_req &&
        connsm->flags.features_rxd) {
        ble_ll_hci_ev_rd_rem_used_feat(connsm, BLE_ERR_SUCCESS);
        connsm->flags.features_host_req = 0;
    }

    /*
     * If there is still pending read features request HCI command, send an
     * event to complete it.
     */
    if (connsm->flags.features_host_req) {
        ble_ll_hci_ev_rd_rem_used_feat(connsm, ble_err);
        connsm->flags.features_host_req = 0;
    }

    /*
     * We need to send a disconnection complete event. Connection Complete for
     * canceling connection creation is sent from LE Create Connection Cancel
     * Command handler.
     *
     * If the ble error is "success" it means that the reset command was
     * received and we should not send an event.
     */
    if (ble_err && (ble_err != BLE_ERR_UNK_CONN_ID ||
                    connsm->flags.terminate_ind_rxd)) {
        ble_ll_disconn_comp_event_send(connsm, ble_err);
    }

    /* Put connection state machine back on free list */
    STAILQ_INSERT_TAIL(&g_ble_ll_conn_free_list, connsm, free_stqe);

    /* Log connection end */
    ble_ll_trace_u32x3(BLE_LL_TRACE_ID_CONN_END, connsm->conn_handle,
                       connsm->event_cntr, (uint32_t)ble_err);
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV_SYNC_TRANSFER)
void
ble_ll_conn_get_anchor(struct ble_ll_conn_sm *connsm, uint16_t conn_event,
                       uint32_t *anchor, uint8_t *anchor_usecs)
{
    uint32_t itvl;

    itvl = (connsm->conn_itvl * BLE_LL_CONN_ITVL_USECS);

    *anchor = connsm->anchor_point;
    *anchor_usecs = connsm->anchor_point_usecs;

    if ((int16_t)(conn_event - connsm->event_cntr) < 0) {
        itvl *= connsm->event_cntr - conn_event;
        ble_ll_tmr_sub(anchor, anchor_usecs, itvl);
    } else {
        itvl *= conn_event - connsm->event_cntr;
        ble_ll_tmr_add(anchor, anchor_usecs, itvl);
    }
}
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
int
ble_ll_conn_move_anchor(struct ble_ll_conn_sm *connsm, uint16_t offset)
{
    BLE_LL_ASSERT(connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL);

    if (IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_CONN_PARAM_REQ) ||
        IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_CONN_UPDATE)) {
        return -1;
    }

    connsm->conn_update_anchor_offset_req = offset;
    ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_CONN_UPDATE, NULL);

    return 0;
}
#endif

/**
 * Called to move to the next connection event.
 *
 * Context: Link Layer task.
 *
 * @param connsm
 *
 * @return int
 */
static int
ble_ll_conn_next_event(struct ble_ll_conn_sm *connsm)
{
    uint32_t conn_itvl_us;
    uint32_t ce_duration;
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    uint32_t cur_ww;
    uint32_t max_ww;
#endif
    struct ble_ll_conn_upd_req *upd;
    uint8_t skip_anchor_calc = 0;
    uint32_t usecs;
    uint8_t use_periph_latency;
    uint16_t base_event_cntr;
    uint16_t next_event_cntr;
    uint8_t next_is_subrated;
    uint16_t subrate_factor;
    uint16_t event_cntr_diff;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
    struct ble_ll_conn_subrate_params *cstp;
    uint16_t trans_next_event_cntr;
    uint16_t subrate_conn_upd_event_cntr;
#endif
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    uint8_t anchor_calc_for_css = 0;
#endif

    /* XXX: deal with connection request procedure here as well */
    ble_ll_conn_chk_csm_flags(connsm);

    /* If unable to start terminate procedure, start it now */
    if (connsm->disconnect_reason && !connsm->flags.terminate_started) {
        ble_ll_ctrl_terminate_start(connsm);
    }

    if (connsm->flags.terminate_started && CONN_IS_PERIPHERAL(connsm)) {
        /* Some of the devices waits whole connection interval to ACK our
         * TERMINATE_IND sent as a Slave. Since we are here it means we are still waiting for ACK.
         * Make sure we catch it in next connection event.
         */
        connsm->periph_latency = 0;
    }

    next_is_subrated = 1;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
    base_event_cntr = connsm->subrate_base_event;
    subrate_factor = connsm->subrate_factor;

    /* We need to restore remaining continuation events counter if a non-empty
     * PDU was txd/rxd in this connection event. Also we need to set counter to
     * 0 in case there was no valid PDU at subrated event, since we should not
     * use continuation events in such case (i.e. ignore any valid PDUs prior
     * to subrated event).
     *
     * Note that has_nonempty_pdu flag is also cleared here since LL may move to
     * next connection event due to scheduling conflict and there will be no
     * start callback for new event.
     */
    if (connsm->has_nonempty_pdu) {
        connsm->cont_num_left = connsm->cont_num;
        connsm->has_nonempty_pdu = 0;
    } else if (connsm->event_cntr == connsm->subrate_base_event) {
        connsm->cont_num_left = 0;
    }

    if (connsm->cont_num_left > 0) {
        connsm->cont_num_left--;
        next_is_subrated = 0;
    }
#else
    base_event_cntr = connsm->event_cntr;
    subrate_factor = 1;
#endif

    /*
     * XXX: TODO Probably want to add checks to see if we need to start
     * a control procedure here as an instant may have prevented us from
     * starting one.
     */

    /*
     * XXX TODO: I think this is technically incorrect. We can allow peripheral
     * latency if we are doing one of these updates as long as we
     * know that the central has received the ACK to the PDU that set
     * the instant
     */
    /* Set event counter to the next connection event that we will tx/rx in */

    use_periph_latency = next_is_subrated &&
                         connsm->flags.periph_use_latency &&
                         !connsm->flags.conn_update_sched &&
                         !connsm->flags.phy_update_sched &&
                         !connsm->flags.chanmap_update_sched &&
                         connsm->flags.pkt_rxd;

    if (next_is_subrated) {
        next_event_cntr = base_event_cntr + subrate_factor;
        if (use_periph_latency) {
            next_event_cntr += subrate_factor * connsm->periph_latency;
        }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
        /* If we are in subrate transition mode, we should also listen on
         * subrated connection events based on new parameters.
         */
        if (connsm->flags.subrate_trans) {
            BLE_LL_ASSERT(CONN_IS_CENTRAL(connsm));

            cstp = &connsm->subrate_trans;
            trans_next_event_cntr = cstp->subrate_base_event;
            while (INT16_LTE(trans_next_event_cntr, connsm->event_cntr)) {
                trans_next_event_cntr += cstp->subrate_factor;
            }
            cstp->subrate_base_event = trans_next_event_cntr;

            if (INT16_LT(trans_next_event_cntr, next_event_cntr)) {
                next_event_cntr = trans_next_event_cntr;
                next_is_subrated = 0;
            }
        }
#endif
    } else {
        next_event_cntr = connsm->event_cntr + 1;
    }


#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
    /* If connection update is scheduled, peripheral shall listen at instant
     * and one connection event before instant regardless of subrating.
     */
    if (CONN_IS_PERIPHERAL(connsm) &&
        connsm->flags.conn_update_sched &&
        (connsm->subrate_factor > 1)) {
        subrate_conn_upd_event_cntr = connsm->conn_update_req.instant - 1;
        if (connsm->event_cntr == subrate_conn_upd_event_cntr) {
            subrate_conn_upd_event_cntr++;
        }

        if (INT16_GT(next_event_cntr, subrate_conn_upd_event_cntr)) {
            next_event_cntr = subrate_conn_upd_event_cntr;
            next_is_subrated = 0;
        }
    }

    /* Set next connection event as a subrate base event if that connection
     * event is a subrated event, this simplifies calculations later.
     * Note that according to spec base event should only be changed on
     * wrap-around, but since we only use this value internally we can use any
     * valid value.
     */
    if (next_is_subrated ||
        (connsm->subrate_base_event +
         connsm->subrate_factor == next_event_cntr)) {
        connsm->subrate_base_event = next_event_cntr;
    }
#endif

    event_cntr_diff = next_event_cntr - connsm->event_cntr;
    BLE_LL_ASSERT(event_cntr_diff > 0);

    connsm->event_cntr = next_event_cntr;

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    if (ble_ll_sched_css_is_enabled() &&
        connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
        connsm->css_period_idx += event_cntr_diff;

        /* If this is non-reference connection, we calculate anchor point from
         * reference connection instead of using connection interval. This is
         * to make sure connections do not drift over time.
         */
        if (g_ble_ll_conn_css_ref != connsm) {
            anchor_calc_for_css = 1;
            skip_anchor_calc = 1;
        }
    }
#endif

    if (!skip_anchor_calc) {
        /* Calculate next anchor point for connection.
         * We can use pre-calculated values for one interval if latency is 1.
         */
        if (event_cntr_diff == 1) {
            connsm->anchor_point += connsm->conn_itvl_ticks;
            ble_ll_tmr_add_u(&connsm->anchor_point, &connsm->anchor_point_usecs,
                             connsm->conn_itvl_usecs);
        } else {
            conn_itvl_us = connsm->conn_itvl * BLE_LL_CONN_ITVL_USECS;

            ble_ll_tmr_add(&connsm->anchor_point, &connsm->anchor_point_usecs,
                           conn_itvl_us * event_cntr_diff);
        }
    }

    /*
     * If a connection update has been scheduled and the event counter
     * is now equal to the instant, we need to adjust the start of the
     * connection by the the transmit window offset. We also copy in the
     * update parameters as they now should take effect.
     */
    if (connsm->flags.conn_update_sched &&
        (connsm->event_cntr == connsm->conn_update_req.instant)) {

        /* Set flag so we send connection update event */
        upd = &connsm->conn_update_req;
        if (CONN_IS_CENTRAL(connsm) ||
            (CONN_IS_PERIPHERAL(connsm) &&
             IS_PENDING_CTRL_PROC(connsm, BLE_LL_CTRL_PROC_CONN_PARAM_REQ)) ||
            (connsm->conn_itvl != upd->interval) ||
            (connsm->periph_latency != upd->latency) ||
            (connsm->supervision_tmo != upd->timeout)) {
            connsm->flags.conn_update_host_w4event = 1;
        }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
        if (connsm->conn_itvl != upd->interval) {
            connsm->subrate_base_event = connsm->event_cntr;
            connsm->subrate_factor = 1;
            connsm->cont_num = 0;
        }
#endif

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
        if (ble_ll_sched_css_is_enabled() &&
            connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
            BLE_LL_ASSERT(connsm->css_slot_idx_pending !=
                          BLE_LL_CONN_CSS_NO_SLOT);

            /* If we are moving to an earlier slot, we are effectively skipping
             * one period.
             */
            if (connsm->css_slot_idx_pending < connsm->css_slot_idx) {
                connsm->css_period_idx++;
            }

            connsm->css_slot_idx = connsm->css_slot_idx_pending;
            connsm->css_slot_idx_pending = BLE_LL_CONN_CSS_NO_SLOT;

            ble_ll_conn_css_update_list(connsm);

            if (anchor_calc_for_css) {
                ble_ll_sched_css_set_conn_anchor(connsm);
                anchor_calc_for_css = 0;
            }

#if MYNEWT_VAL(BLE_LL_HCI_VS_CONN_STRICT_SCHED)
            ble_ll_hci_ev_send_vs_css_slot_changed(connsm->conn_handle,
                                                   connsm->css_slot_idx);
#endif
        }
#endif

        connsm->supervision_tmo = upd->timeout;
        connsm->periph_latency = upd->latency;
        connsm->tx_win_size = upd->winsize;
        connsm->periph_cur_tx_win_usecs =
            connsm->tx_win_size * BLE_LL_CONN_TX_WIN_USECS;
        connsm->tx_win_off = upd->winoffset;
        connsm->conn_itvl = upd->interval;

        ble_ll_conn_itvl_to_ticks(connsm->conn_itvl, &connsm->conn_itvl_ticks,
                                  &connsm->conn_itvl_usecs);

        if (connsm->conn_param_req.handle != 0) {
            connsm->max_ce_len_ticks = ble_ll_tmr_u2t_up(connsm->conn_param_req.max_ce_len * BLE_LL_CONN_CE_USECS);
            connsm->conn_param_req.handle = 0;
        }

        if (upd->winoffset != 0) {
            usecs = upd->winoffset * BLE_LL_CONN_ITVL_USECS;
            ble_ll_tmr_add(&connsm->anchor_point, &connsm->anchor_point_usecs,
                           usecs);
        }

        /* Reset the starting point of the connection supervision timeout */
        connsm->last_rxd_pdu_cputime = connsm->anchor_point;

        /* Reset update scheduled flag */
        connsm->flags.conn_update_sched = 0;
    }

    /*
     * If there is a channel map request pending and we have reached the
     * instant, change to new channel map. Note there is a special case here.
     * If we received a channel map update with an instant equal to the event
     * counter, when we get here the event counter has already been
     * incremented by 1. That is why we do a signed comparison and change to
     * new channel map once the event counter equals or has passed channel
     * map update instant.
     */
    if (connsm->flags.chanmap_update_sched &&
        ((int16_t)(connsm->chanmap_instant - connsm->event_cntr) <= 0)) {

        /* XXX: there is a chance that the control packet is still on
         * the queue of the central. This means that we never successfully
         * transmitted update request. Would end up killing connection
           on peripheral side. Could ignore it or see if still enqueued. */
        connsm->chan_map_used =
            ble_ll_utils_chan_map_used_get(connsm->req_chanmap);
        memcpy(connsm->chan_map, connsm->req_chanmap, BLE_LL_CHAN_MAP_LEN);

        connsm->flags.chanmap_update_sched = 0;

        ble_ll_ctrl_proc_stop(connsm, BLE_LL_CTRL_PROC_CHAN_MAP_UPD);

        /* XXX: host could have resent channel map command. Need to
           check to make sure we dont have to restart! */
    }

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    if (anchor_calc_for_css) {
        ble_ll_sched_css_set_conn_anchor(connsm);
    }
#endif

#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    if (connsm->flags.phy_update_sched &&
        (connsm->event_cntr == connsm->phy_instant)) {

        /* Set cur phy to new phy */
        if (connsm->phy_data.new_tx_phy) {
            connsm->phy_data.cur_tx_phy = connsm->phy_data.new_tx_phy;
            connsm->phy_data.tx_phy_mode =
                                ble_ll_phy_to_phy_mode(connsm->phy_data.cur_tx_phy,
                                                   connsm->phy_data.pref_opts);
        }

        if (connsm->phy_data.new_rx_phy) {
            connsm->phy_data.cur_rx_phy = connsm->phy_data.new_rx_phy;
            connsm->phy_data.rx_phy_mode =
                                ble_ll_phy_to_phy_mode(connsm->phy_data.cur_rx_phy,
                                                   connsm->phy_data.pref_opts);
        }

        /* Clear flags and set flag to send event at next instant */
        connsm->flags.phy_update_sched = 0;
        connsm->flags.phy_update_host_w4event = 1;

        ble_ll_ctrl_phy_update_proc_complete(connsm);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
        /* Recalculate effective connection parameters */
        ble_ll_conn_update_eff_data_len(connsm);

        /*
         * If PHY in either direction was changed to coded, we assume that peer
         * does support LE Coded PHY even if features were not exchanged yet.
         * This means that MaxRxTime can be updated to supported max and we need
         * initiate DLE to notify peer about the change.
         */
        if (((connsm->phy_data.cur_tx_phy == BLE_PHY_CODED) ||
             (connsm->phy_data.cur_rx_phy == BLE_PHY_CODED)) &&
            !ble_ll_conn_rem_feature_check(connsm, BLE_LL_FEAT_LE_CODED_PHY)) {
            ble_ll_conn_rem_feature_add(connsm, BLE_LL_FEAT_LE_CODED_PHY);
            connsm->max_rx_time = BLE_LL_CONN_SUPP_TIME_MAX_CODED;
            ble_ll_ctrl_initiate_dle(connsm, false);
        }
#endif
    }
#endif

    /* Calculate data channel index of next connection event */
    connsm->data_chan_index = ble_ll_conn_calc_dci(connsm, event_cntr_diff);

    /*
     * If we are trying to terminate connection, check if next wake time is
     * passed the termination timeout. If so, no need to continue with
     * connection as we will time out anyway.
     */
    if (connsm->flags.terminate_started) {
        if ((int32_t)(connsm->terminate_timeout - connsm->anchor_point) <= 0) {
            return -1;
        }
    }

    /*
     * Calculate ce end time. For a peripheral, we need to add window widening
     * and the transmit window if we still have one.
     */
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    /* If css is enabled, use slot duration instead of conn_init_slots for
     * reservation.
     */
    if (ble_ll_sched_css_is_enabled() &&
        connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
        ce_duration = ble_ll_tmr_u2t(ble_ll_sched_css_get_slot_us());
    } else {
        ce_duration = ble_ll_tmr_u2t(MYNEWT_VAL(BLE_LL_CONN_INIT_SLOTS) *
                                     BLE_LL_SCHED_USECS_PER_SLOT);
    }
#else
    ce_duration = ble_ll_tmr_u2t(MYNEWT_VAL(BLE_LL_CONN_INIT_SLOTS) *
                                     BLE_LL_SCHED_USECS_PER_SLOT);
#endif

#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    if (connsm->conn_role == BLE_LL_CONN_ROLE_PERIPHERAL) {

        cur_ww = ble_ll_utils_calc_window_widening(connsm->anchor_point,
                                                   connsm->last_anchor_point,
                                                   connsm->central_sca);
        max_ww = (connsm->conn_itvl * (BLE_LL_CONN_ITVL_USECS/2)) - BLE_LL_IFS;
        if (cur_ww >= max_ww) {
            return -1;
        }
        cur_ww += BLE_LL_JITTER_USECS;
        connsm->periph_cur_window_widening = cur_ww;
        ce_duration += ble_ll_tmr_u2t(cur_ww +
                                      connsm->periph_cur_tx_win_usecs);
    }
#endif
    ce_duration -= g_ble_ll_sched_offset_ticks;
    connsm->ce_end_time = connsm->anchor_point + ce_duration;

    return 0;
}

/**
 * Called when a connection has been created. This function will
 *  -> Set the connection state to created.
 *  -> Start the connection supervision timer
 *  -> Set the Link Layer state to connection.
 *  -> Send a connection complete event.
 *
 *  See Section 4.5.2 Vol 6 Part B
 *
 *  Context: Link Layer
 *
 * @param connsm
 *
 * @ return 0: connection NOT created. 1: connection created
 */
static int
ble_ll_conn_created(struct ble_ll_conn_sm *connsm, struct ble_mbuf_hdr *rxhdr)
{
    int rc;
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    uint8_t *evbuf;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    uint32_t usecs;
#endif

    /* XXX: TODO this assumes we received in 1M phy */

    /* Set state to created */
    connsm->conn_state = BLE_LL_CONN_STATE_CREATED;

    /* Clear packet received flag */
    connsm->flags.pkt_rxd = 0;

    /* Consider time created the last scheduled time */
    connsm->last_scheduled = ble_ll_tmr_get();

    /*
     * Set the last rxd pdu time since this is where we want to start the
     * supervision timer from.
     */
    connsm->last_rxd_pdu_cputime = connsm->last_scheduled;

    /*
     * Set first connection event time. If peripheral the endtime is the receive end
     * time of the connect request. The actual connection starts 1.25 msecs plus
     * the transmit window offset from the end of the connection request.
     */
    rc = 1;
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    if (connsm->conn_role == BLE_LL_CONN_ROLE_PERIPHERAL) {
        /*
         * With a 32.768 kHz crystal we dont care about the remaining usecs
         * when setting last anchor point. The only thing last anchor is used
         * for is to calculate window widening. The effect of this is
         * negligible.
         */
        connsm->last_anchor_point = rxhdr->beg_cputime;

        usecs = rxhdr->rem_usecs + 1250 +
                (connsm->tx_win_off * BLE_LL_CONN_TX_WIN_USECS) +
                ble_ll_pdu_us(BLE_CONNECT_REQ_LEN,
                                       rxhdr->rxinfo.phy_mode);

        if (rxhdr->rxinfo.channel < BLE_PHY_NUM_DATA_CHANS) {
            switch (rxhdr->rxinfo.phy) {
            case BLE_PHY_1M:
            case BLE_PHY_2M:
                usecs += 1250;
                break;
            case BLE_PHY_CODED:
                usecs += 2500;
                break;
            default:
                BLE_LL_ASSERT(0);
                break;
            }
        }

        /* Anchor point is cputime. */
        connsm->anchor_point = rxhdr->beg_cputime;
        connsm->anchor_point_usecs = 0;
        ble_ll_tmr_add(&connsm->anchor_point, &connsm->anchor_point_usecs,
                       usecs);

        connsm->periph_cur_tx_win_usecs =
            connsm->tx_win_size * BLE_LL_CONN_TX_WIN_USECS;
        connsm->ce_end_time = connsm->anchor_point +
                              ble_ll_tmr_u2t(MYNEWT_VAL(BLE_LL_CONN_INIT_SLOTS) *
                                             BLE_LL_SCHED_USECS_PER_SLOT +
                                             connsm->periph_cur_tx_win_usecs) + 1;

        /* Start the scheduler for the first connection event */
        while (ble_ll_sched_conn_periph_new(connsm)) {
            if (ble_ll_conn_next_event(connsm)) {
                STATS_INC(ble_ll_conn_stats, cant_set_sched);
                rc = 0;
                break;
            }
        }
    }
#endif

    /* Send connection complete event to inform host of connection */
    if (rc) {
#if (BLE_LL_BT5_PHY_SUPPORTED == 1) && MYNEWT_VAL(BLE_LL_CONN_PHY_INIT_UPDATE)
        /*
         * If we have default phy preferences and they are different than
         * the current PHY's in use, start update procedure.
         */
        /*
         * XXX: should we attempt to start this without knowing if
         * the other side can support it?
         */
        if (!ble_ll_conn_phy_update_if_needed(connsm)) {
            connsm->flags.phy_update_self_initiated = 1;
        }
#endif
        switch (connsm->conn_role) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
        case BLE_LL_CONN_ROLE_CENTRAL:
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
            if (ble_ll_sched_css_is_enabled()) {
                ble_ll_sched_css_update_anchor(connsm);
                ble_ll_conn_css_set_next_slot(BLE_LL_CONN_CSS_NO_SLOT);
            }
#endif

            evbuf = ble_ll_init_get_conn_comp_ev();
            ble_ll_conn_comp_event_send(connsm, BLE_ERR_SUCCESS, evbuf, NULL);
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CSA2)
            ble_ll_hci_ev_le_csa(connsm);
#endif

            /*
             * Initiate features exchange
             *
             * XXX we do this only as a central as it was observed that sending
             * LL_PERIPH_FEATURE_REQ after connection breaks some recent iPhone
             * models; for peripheral just assume central will initiate features xchg
             * if it has some additional features to use.
             */
            ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_FEATURE_XCHG,
                                   NULL);
            break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
        case BLE_LL_CONN_ROLE_PERIPHERAL:
            ble_ll_adv_send_conn_comp_ev(connsm, rxhdr);
            break;
#endif
        default:
            BLE_LL_ASSERT(0);
            break;
        }
    }

    return rc;
}

/**
 * Called upon end of connection event
 *
 * Context: Link-layer task
 *
 * @param void *arg Pointer to connection state machine
 *
 */
static void
ble_ll_conn_event_end(struct ble_npl_event *ev)
{
    uint8_t ble_err;
    uint32_t tmo;
    struct ble_ll_conn_sm *connsm;

    ble_ll_rfmgmt_release();

    /* Better be a connection state machine! */
    connsm = (struct ble_ll_conn_sm *)ble_npl_event_get_arg(ev);
    BLE_LL_ASSERT(connsm);

    /* Log event end */
    ble_ll_trace_u32x2(BLE_LL_TRACE_ID_CONN_EV_END, connsm->conn_handle,
                       connsm->event_cntr);

    ble_ll_scan_chk_resume();

    /* If we have transmitted the terminate IND successfully, we are done */
    if ((connsm->flags.terminate_ind_txd) ||
        (connsm->flags.terminate_ind_rxd &&
         connsm->flags.terminate_ind_rxd_acked)) {
        if (connsm->flags.terminate_ind_txd) {
            ble_err = BLE_ERR_CONN_TERM_LOCAL;
        } else {
            /* Make sure the disconnect reason is valid! */
            ble_err = connsm->rxd_disconnect_reason;
            if (ble_err == 0) {
                ble_err = BLE_ERR_REM_USER_CONN_TERM;
            }
        }
        ble_ll_conn_end(connsm, ble_err);
        return;
    }

    /* Remove any connection end events that might be enqueued */
    ble_ll_event_remove(&connsm->conn_ev_end);

    /*
     * If we have received a packet, we can set the current transmit window
     * usecs to 0 since we dont need to listen in the transmit window.
     */
    if (connsm->flags.pkt_rxd) {
        connsm->periph_cur_tx_win_usecs = 0;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_PING)
    /*
     * If we are encrypted and have passed the authenticated payload timeout
     * we need to send an event to tell the host. Unfortunately, I think we
     * need one of these per connection and we have to set this timer
     * fairly accurately. So we need to another event in the connection.
     * This sucks.
     *
     * The way this works is that whenever the timer expires it just gets reset
     * and we send the autheticated payload timeout event. Note that this timer
     * should run even when encryption is paused.
     * XXX: what should be here? Was there code here that got deleted?
     */
#endif

    /* Move to next connection event */
    if (ble_ll_conn_next_event(connsm)) {
        ble_ll_conn_end(connsm, BLE_ERR_CONN_TERM_LOCAL);
        return;
    }

    /* Reset "per connection event" variables */
    connsm->cons_rxd_bad_crc = 0;
    connsm->flags.pkt_rxd = 0;

    /* See if we need to start any control procedures */
    ble_ll_ctrl_chk_proc_start(connsm);

    /* Set initial schedule callback */
    connsm->conn_sch.sched_cb = ble_ll_conn_event_start_cb;

    /* XXX: I think all this fine for when we do connection updates, but
       we may want to force the first event to be scheduled. Not sure */
    /* Schedule the next connection event */
    while (ble_ll_sched_conn_reschedule(connsm)) {
        if (ble_ll_conn_next_event(connsm)) {
            ble_ll_conn_end(connsm, BLE_ERR_CONN_TERM_LOCAL);
            return;
        }
    }

    /*
     * This is definitely not perfect but hopefully will be fine in regards to
     * the specification. We check the supervision timer at connection event
     * end. If the next connection event is going to start past the supervision
     * timeout we end the connection here. I guess this goes against the spec
     * in two ways:
     * 1) We are actually causing a supervision timeout before the time
     * specified. However, this is really a moot point because the supervision
     * timeout would have expired before we could possibly receive a packet.
     * 2) We may end the supervision timeout a bit later than specified as
     * we only check this at event end and a bad CRC could cause us to continue
     * the connection event longer than the supervision timeout. Given that two
     * bad CRC's consecutively ends the connection event, I dont regard this as
     * a big deal but it could cause a slightly longer supervision timeout.
     */
    if (connsm->conn_state == BLE_LL_CONN_STATE_CREATED) {
        tmo = (uint32_t)connsm->conn_itvl * BLE_LL_CONN_ITVL_USECS * 6UL;
        ble_err = BLE_ERR_CONN_ESTABLISHMENT;
    } else {
        tmo = connsm->supervision_tmo * BLE_HCI_CONN_SPVN_TMO_UNITS * 1000UL;
        ble_err = BLE_ERR_CONN_SPVN_TMO;
    }
    /* XXX: Convert to ticks to usecs calculation instead??? */
    tmo = ble_ll_tmr_u2t(tmo);
    if ((int32_t)(connsm->anchor_point - connsm->last_rxd_pdu_cputime) >= tmo) {
        ble_ll_conn_end(connsm, ble_err);
        return;
    }

    /* If we have completed packets, send an event */
    ble_ll_conn_num_comp_pkts_event_send(connsm);

    /* If we have features and there's pending HCI command, send an event */
    if (connsm->flags.features_host_req &&
        connsm->flags.features_rxd) {
        ble_ll_hci_ev_rd_rem_used_feat(connsm, BLE_ERR_SUCCESS);
        connsm->flags.features_host_req = 0;
    }
}

/**
 * Update the connection request PDU with the address type and address of
 * advertiser we are going to send connect request to.
 *
 * @param m
 * @param adva
 * @param addr_type     Address type of ADVA from received advertisement.
 * @param inita
 * @param inita_type     Address type of INITA from received advertisement.

 * @param txoffset      The tx window offset for this connection
 */
void
ble_ll_conn_prepare_connect_ind(struct ble_ll_conn_sm *connsm,
                                struct ble_ll_scan_pdu_data *pdu_data,
                                struct ble_ll_scan_addr_data *addrd,
                                uint8_t channel)
{
    uint8_t hdr;
    uint8_t *addr;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    struct ble_ll_resolv_entry *rl;
#endif

    hdr = BLE_ADV_PDU_TYPE_CONNECT_IND;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CSA2)
    /* We need CSA2 bit only for legacy connect */
    if (channel >= BLE_PHY_NUM_DATA_CHANS) {
        hdr |= BLE_ADV_PDU_HDR_CHSEL;
    }
#endif

    if (addrd->adva_type) {
        /* Set random address */
        hdr |= BLE_ADV_PDU_HDR_RXADD_MASK;
    }

    if (addrd->targeta) {
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
        if (addrd->targeta_resolved) {
            if (connsm->own_addr_type > BLE_OWN_ADDR_RANDOM) {
                /* If TargetA was resolved we should reply with a different RPA
                 * in InitA (see Core 5.3, Vol 6, Part B, 6.4).
                 */
                BLE_LL_ASSERT(addrd->rpa_index >= 0);
                rl = &g_ble_ll_resolv_list[addrd->rpa_index];
                hdr |= BLE_ADV_PDU_HDR_TXADD_RAND;
                ble_ll_resolv_get_priv_addr(rl, 1, pdu_data->inita);
            } else {
                /* Host does not want us to use RPA so use identity */
                if ((connsm->own_addr_type & 1) == 0) {
                    memcpy(pdu_data->inita, g_dev_addr, BLE_DEV_ADDR_LEN);
                } else {
                    hdr |= BLE_ADV_PDU_HDR_TXADD_RAND;
                    memcpy(pdu_data->inita, g_random_addr, BLE_DEV_ADDR_LEN);
                }
            }
        } else {
            memcpy(pdu_data->inita, addrd->targeta, BLE_DEV_ADDR_LEN);
            if (addrd->targeta_type) {
                hdr |= BLE_ADV_PDU_HDR_TXADD_RAND;
            }
        }
#else
        memcpy(pdu_data->inita, addrd->targeta, BLE_DEV_ADDR_LEN);
        if (addrd->targeta_type) {
            hdr |= BLE_ADV_PDU_HDR_TXADD_RAND;
        }
#endif
    } else {
        /* Get pointer to our device address */
        if ((connsm->own_addr_type & 1) == 0) {
            addr = g_dev_addr;
        } else {
            hdr |= BLE_ADV_PDU_HDR_TXADD_RAND;
            addr = g_random_addr;
        }

    /* XXX: do this ahead of time? Calculate the local rpa I mean */
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
        if ((connsm->own_addr_type > BLE_HCI_ADV_OWN_ADDR_RANDOM) &&
            (addrd->rpa_index >= 0)) {
            /* We are using RPA and advertiser was on our resolving list, so
             * we'll use RPA to reply (see Core 5.3, Vol 6, Part B, 6.4).
             */
            rl = &g_ble_ll_resolv_list[addrd->rpa_index];
            if (rl->rl_has_local) {
                hdr |= BLE_ADV_PDU_HDR_TXADD_RAND;
                ble_ll_resolv_get_priv_addr(rl, 1, pdu_data->inita);
                addr = NULL;
            }
        }
#endif

        if (addr) {
            memcpy(pdu_data->inita, addr, BLE_DEV_ADDR_LEN);
            /* Identity address used */
            connsm->inita_identity_used = 1;
        }
    }

    memcpy(pdu_data->adva, addrd->adva, BLE_DEV_ADDR_LEN);

    pdu_data->hdr_byte = hdr;
}

uint8_t
ble_ll_conn_tx_connect_ind_pducb(uint8_t *dptr, void *pducb_arg, uint8_t *hdr_byte)
{
    struct ble_ll_conn_sm *connsm;
    struct ble_ll_scan_pdu_data *pdu_data;

    connsm = pducb_arg;
    /*
     * pdu_data was prepared just before starting TX and is expected to be
     * still valid here
     */
    pdu_data = ble_ll_scan_get_pdu_data();

    memcpy(dptr, pdu_data->inita, BLE_DEV_ADDR_LEN);
    memcpy(dptr + BLE_DEV_ADDR_LEN, pdu_data->adva, BLE_DEV_ADDR_LEN);

    dptr += 2 * BLE_DEV_ADDR_LEN;

    put_le32(dptr, connsm->access_addr);
    dptr[4] = (uint8_t)connsm->crcinit;
    dptr[5] = (uint8_t)(connsm->crcinit >> 8);
    dptr[6] = (uint8_t)(connsm->crcinit >> 16);
    dptr[7] = connsm->tx_win_size;
    put_le16(dptr + 8, connsm->tx_win_off);
    put_le16(dptr + 10, connsm->conn_itvl);
    put_le16(dptr + 12, connsm->periph_latency);
    put_le16(dptr + 14, connsm->supervision_tmo);
    memcpy(dptr + 16, &connsm->chan_map, BLE_LL_CHAN_MAP_LEN);
    dptr[21] = connsm->hop_inc | (connsm->central_sca << 5);

    *hdr_byte = pdu_data->hdr_byte;

    return 34;
}

/**
 * Called when a schedule item overlaps the currently running connection
 * event. This generally should not happen, but if it does we stop the
 * current connection event to let the schedule item run.
 *
 * NOTE: the phy has been disabled as well as the wfr timer before this is
 * called.
 */
void
ble_ll_conn_event_halt(void)
{
    ble_ll_state_set(BLE_LL_STATE_STANDBY);
    if (g_ble_ll_conn_cur_sm) {
        g_ble_ll_conn_cur_sm->flags.pkt_rxd = 0;
        ble_ll_event_add(&g_ble_ll_conn_cur_sm->conn_ev_end);
        g_ble_ll_conn_cur_sm = NULL;
    }
}

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
int
ble_ll_conn_send_connect_req(struct os_mbuf *rxpdu,
                             struct ble_ll_scan_addr_data *addrd,
                             uint8_t ext)
{
    struct ble_ll_conn_sm *connsm;
    struct ble_mbuf_hdr *rxhdr;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    uint8_t phy;
#endif
    int rc;

    connsm = g_ble_ll_conn_create_sm.connsm;
    rxhdr = BLE_MBUF_HDR_PTR(rxpdu);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    if (ext) {
#if BLE_LL_BT5_PHY_SUPPORTED
        phy = rxhdr->rxinfo.phy;
#else
        phy = BLE_PHY_1M;
#endif
        ble_ll_conn_create_set_params(connsm, phy);
    }
#endif

    if (ble_ll_sched_conn_central_new(connsm, rxhdr, 0)) {
        return -1;
    }

    ble_ll_conn_prepare_connect_ind(connsm, ble_ll_scan_get_pdu_data(), addrd,
                                    rxhdr->rxinfo.channel);

    ble_phy_set_txend_cb(NULL, NULL);
    rc = ble_phy_tx(ble_ll_conn_tx_connect_ind_pducb, connsm,
                    ext ? BLE_PHY_TRANSITION_TX_RX : BLE_PHY_TRANSITION_NONE);
    if (rc) {
        ble_ll_conn_send_connect_req_cancel();
        return -1;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV) && BLE_LL_BT5_PHY_SUPPORTED
    if (ext) {
        ble_ll_conn_init_phy(connsm, phy);
    }
#endif

    return 0;
}

void
ble_ll_conn_send_connect_req_cancel(void)
{
    struct ble_ll_conn_sm *connsm;

    connsm = g_ble_ll_conn_create_sm.connsm;

    ble_ll_sched_rmv_elem(&connsm->conn_sch);
}

static void
ble_ll_conn_central_start(uint8_t phy, uint8_t csa,
                          struct ble_ll_scan_addr_data *addrd, uint8_t *targeta)
{
    struct ble_ll_conn_sm *connsm;

    connsm = g_ble_ll_conn_create_sm.connsm;
    g_ble_ll_conn_create_sm.connsm = NULL;

    connsm->peer_addr_type = addrd->adv_addr_type;
    memcpy(connsm->peer_addr, addrd->adv_addr, 6);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    if (addrd->adva_resolved) {
        BLE_LL_ASSERT(addrd->rpa_index >= 0);
        connsm->peer_addr_resolved = 1;
        ble_ll_resolv_set_peer_rpa(addrd->rpa_index, addrd->adva);
        ble_ll_scan_set_peer_rpa(addrd->adva);
    } else {
        connsm->peer_addr_resolved = 0;
    }

    if (addrd->targeta_resolved) {
        BLE_LL_ASSERT(addrd->rpa_index >= 0);
        BLE_LL_ASSERT(targeta);
    }
#endif

    ble_ll_conn_set_csa(connsm, csa);
#if BLE_LL_BT5_PHY_SUPPORTED
    ble_ll_conn_init_phy(connsm, phy);
#endif
    ble_ll_conn_created(connsm, NULL);
}

void
ble_ll_conn_created_on_legacy(struct os_mbuf *rxpdu,
                              struct ble_ll_scan_addr_data *addrd,
                              uint8_t *targeta)
{
    uint8_t *rxbuf;
    uint8_t csa;

    rxbuf = rxpdu->om_data;
    csa = rxbuf[0] & BLE_ADV_PDU_HDR_CHSEL_MASK;

    ble_ll_conn_central_start(BLE_PHY_1M, csa, addrd, targeta);
}
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
void
ble_ll_conn_created_on_aux(struct os_mbuf *rxpdu,
                           struct ble_ll_scan_addr_data *addrd,
                           uint8_t *targeta)
{
#if BLE_LL_BT5_PHY_SUPPORTED
    struct ble_mbuf_hdr *rxhdr;
#endif
    uint8_t phy;

#if BLE_LL_BT5_PHY_SUPPORTED
    rxhdr = BLE_MBUF_HDR_PTR(rxpdu);
    phy = rxhdr->rxinfo.phy;
#else
    phy = BLE_PHY_1M;
#endif

    ble_ll_conn_central_start(phy, 1, addrd, targeta);
}
#endif
#endif /* BLE_LL_CFG_FEAT_LL_EXT_ADV */

/**
 * Function called when a timeout has occurred for a connection. There are
 * two types of timeouts: a connection supervision timeout and control
 * procedure timeout.
 *
 * Context: Link Layer task
 *
 * @param connsm
 * @param ble_err
 */
void
ble_ll_conn_timeout(struct ble_ll_conn_sm *connsm, uint8_t ble_err)
{
    int was_current;
    os_sr_t sr;

    was_current = 0;
    OS_ENTER_CRITICAL(sr);
    if (g_ble_ll_conn_cur_sm == connsm) {
        ble_ll_conn_current_sm_over(NULL);
        was_current = 1;
    }
    OS_EXIT_CRITICAL(sr);

    /* Check if we need to resume scanning */
    if (was_current) {
        ble_ll_scan_chk_resume();
    }

    ble_ll_conn_end(connsm, ble_err);
}

/**
 * Called when a data channel PDU has started that matches the access
 * address of the current connection. Note that the CRC of the PDU has not
 * been checked yet.
 *
 * Context: Interrupt
 *
 * @param rxhdr
 */
int
ble_ll_conn_rx_isr_start(struct ble_mbuf_hdr *rxhdr, uint32_t aa)
{
    struct ble_ll_conn_sm *connsm;

    /*
     * Disable wait for response timer since we receive a response. We dont
     * care if this is the response we were waiting for or not; the code
     * called at receive end will deal with ending the connection event
     * if needed
     */
    connsm = g_ble_ll_conn_cur_sm;
    if (connsm) {
        /* Double check access address. Better match connection state machine */
        if (aa != connsm->access_addr) {
            STATS_INC(ble_ll_conn_stats, rx_data_pdu_bad_aa);
            ble_ll_state_set(BLE_LL_STATE_STANDBY);
            ble_ll_event_add(&connsm->conn_ev_end);
            g_ble_ll_conn_cur_sm = NULL;
            return -1;
        }

        /* Set connection handle in mbuf header */
        rxhdr->rxinfo.handle = connsm->conn_handle;

        /* Set flag denoting we have received a packet in connection event */
        connsm->flags.pkt_rxd = 1;

        /* Connection is established */
        connsm->conn_state = BLE_LL_CONN_STATE_ESTABLISHED;

        /* Set anchor point (and last) if 1st rxd frame in connection event */
        if (connsm->flags.periph_set_last_anchor) {
            connsm->flags.periph_set_last_anchor = 0;
            connsm->last_anchor_point = rxhdr->beg_cputime;
            connsm->anchor_point = connsm->last_anchor_point;
            connsm->anchor_point_usecs = rxhdr->rem_usecs;
        }
    }
    return 1;
}

/**
 * Called from the Link Layer task when a data PDU has been received
 *
 * Context: Link layer task
 *
 * @param rxpdu Pointer to received pdu
 * @param rxpdu Pointer to ble mbuf header of received pdu
 */
void
ble_ll_conn_rx_data_pdu(struct os_mbuf *rxpdu, struct ble_mbuf_hdr *hdr)
{
    uint8_t hdr_byte;
    uint8_t rxd_sn;
    uint8_t *rxbuf;
    uint8_t llid;
    uint16_t acl_len;
    uint16_t acl_hdr;
    struct ble_ll_conn_sm *connsm;

    /* Packets with invalid CRC are not sent to LL */
    BLE_LL_ASSERT(BLE_MBUF_HDR_CRC_OK(hdr));

    /* XXX: there is a chance that the connection was thrown away and
       re-used before processing packets here. Fix this. */
    /* We better have a connection state machine */
    connsm = ble_ll_conn_find_by_handle(hdr->rxinfo.handle);
    if (!connsm) {
       STATS_INC(ble_ll_conn_stats, no_conn_sm);
       goto conn_rx_data_pdu_end;
    }

    /* Check state machine */
    ble_ll_conn_chk_csm_flags(connsm);

    /* Validate rx data pdu */
    rxbuf = rxpdu->om_data;
    hdr_byte = rxbuf[0];
    acl_len = rxbuf[1];
    llid = hdr_byte & BLE_LL_DATA_HDR_LLID_MASK;
    rxd_sn = hdr_byte & BLE_LL_DATA_HDR_SN_MASK;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
    if (BLE_MBUF_HDR_MIC_FAILURE(hdr)) {
        /* MIC failure is expected on retransmissions since packet counter does
         * not match, so we simply ignore retransmitted PDU with MIC failure as
         * they do not have proper decrypted contents.
         */
        if (rxd_sn != connsm->last_rxd_sn) {
            STATS_INC(ble_ll_conn_stats, mic_failures);
            ble_ll_conn_timeout(connsm, BLE_ERR_CONN_TERM_MIC);
        }
        goto conn_rx_data_pdu_end;
    }
#endif

    /*
     * Check that the LLID and payload length are reasonable.
     * Empty payload is only allowed for LLID == 01b.
     *  */
    if ((llid == 0) || ((acl_len == 0) && (llid != BLE_LL_LLID_DATA_FRAG))) {
        STATS_INC(ble_ll_conn_stats, rx_bad_llid);
        goto conn_rx_data_pdu_end;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
    /* Check if PDU is allowed when encryption is started. If not,
     * terminate connection.
     *
     * Reference: Core 5.0, Vol 6, Part B, 5.1.3.1
     */
    if ((connsm->enc_data.enc_state > CONN_ENC_S_PAUSE_ENC_RSP_WAIT &&
         CONN_IS_CENTRAL(connsm)) ||
        (connsm->enc_data.enc_state >= CONN_ENC_S_ENC_RSP_TO_BE_SENT &&
         CONN_IS_PERIPHERAL(connsm))) {
        if (!ble_ll_ctrl_enc_allowed_pdu_rx(rxpdu)) {
            ble_ll_conn_timeout(connsm, BLE_ERR_CONN_TERM_MIC);
            goto conn_rx_data_pdu_end;
        }
    }
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_PING)
    /*
     * Reset authenticated payload timeout if valid MIC. NOTE: we dont
     * check the MIC failure bit as that would have terminated the
     * connection
     */
    if ((connsm->enc_data.enc_state == CONN_ENC_S_ENCRYPTED) &&
        connsm->flags.le_ping_supp && (acl_len != 0)) {
        ble_ll_conn_auth_pyld_timer_start(connsm);
    }
#endif

    /* Update RSSI */
    connsm->conn_rssi = hdr->rxinfo.rssi - ble_ll_rx_gain();

    /*
     * If we are a peripheral, we can only start to use peripheral latency
     * once we have received a NESN of 1 from the central
     */
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    if (connsm->conn_role == BLE_LL_CONN_ROLE_PERIPHERAL) {
        if (hdr_byte & BLE_LL_DATA_HDR_NESN_MASK) {
            connsm->flags.periph_use_latency = 1;
        }
    }
#endif

    /*
     * Discard the received PDU if the sequence number is the same
     * as the last received sequence number
     */
    if (rxd_sn == connsm->last_rxd_sn) {
       STATS_INC(ble_ll_conn_stats, data_pdu_rx_dup);
       goto conn_rx_data_pdu_end;
   }

    /* Update last rxd sn */
    connsm->last_rxd_sn = rxd_sn;

    /* No need to do anything if empty pdu */
    if ((llid == BLE_LL_LLID_DATA_FRAG) && (acl_len == 0)) {
        goto conn_rx_data_pdu_end;
    }

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
    connsm->has_nonempty_pdu = 1;
#endif

    if (llid == BLE_LL_LLID_CTRL) {
        /* Process control frame */
        STATS_INC(ble_ll_conn_stats, rx_ctrl_pdus);
        if (ble_ll_ctrl_rx_pdu(connsm, rxpdu)) {
            STATS_INC(ble_ll_conn_stats, rx_malformed_ctrl_pdus);
        }
    } else {
        /* Count # of received l2cap frames and byes */
        STATS_INC(ble_ll_conn_stats, rx_l2cap_pdus);
        STATS_INCN(ble_ll_conn_stats, rx_l2cap_bytes, acl_len);

        /* NOTE: there should be at least two bytes available */
        BLE_LL_ASSERT(OS_MBUF_LEADINGSPACE(rxpdu) >= 2);
        os_mbuf_prepend(rxpdu, 2);
        rxbuf = rxpdu->om_data;

        acl_hdr = (llid << 12) | connsm->conn_handle;
        put_le16(rxbuf, acl_hdr);
        put_le16(rxbuf + 2, acl_len);
        ble_transport_to_hs_acl(rxpdu);
    }

    /* NOTE: we dont free the mbuf since we handed it off! */
    return;

    /* Free buffer */
conn_rx_data_pdu_end:
#if MYNEWT_PKG_apache_mynewt_nimble__nimble_transport_common_hci_ipc
    if (hdr->rxinfo.flags & BLE_MBUF_HDR_F_CONN_CREDIT_INT) {
        hci_ipc_put(HCI_IPC_TYPE_ACL);
    }
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_CTRL_TO_HOST_FLOW_CONTROL)
    /* Need to give credit back if we allocated one for this PDU */
    if (hdr->rxinfo.flags & BLE_MBUF_HDR_F_CONN_CREDIT) {
        ble_ll_conn_cth_flow_free_credit(connsm, 1);
    }
#endif

    os_mbuf_free_chain(rxpdu);
}

/**
 * Called when a packet has been received while in the connection state.
 *
 * Context: Interrupt
 *
 * @param rxpdu
 * @param crcok
 *
 * @return int
 *       < 0: Disable the phy after reception.
 *      == 0: Success. Do not disable the PHY.
 *       > 0: Do not disable PHY as that has already been done.
 */
int
ble_ll_conn_rx_isr_end(uint8_t *rxbuf, struct ble_mbuf_hdr *rxhdr)
{
    int rc;
    uint8_t hdr_byte;
    uint8_t hdr_sn;
    uint8_t hdr_nesn;
    uint8_t conn_sn;
    uint8_t conn_nesn;
    uint8_t reply = 0;
    uint16_t rem_bytes;
    uint8_t opcode = 0;
    uint8_t rx_pyld_len;
    uint32_t begtime;
    uint32_t add_usecs;
    struct os_mbuf *txpdu;
    struct ble_ll_conn_sm *connsm;
    struct os_mbuf *rxpdu = NULL;
    struct ble_mbuf_hdr *txhdr;
    int rx_phy_mode;
    bool alloc_rxpdu = true;

    rc = -1;
    connsm = g_ble_ll_conn_cur_sm;

    /* Retrieve the header and payload length */
    hdr_byte = rxbuf[0];
    rx_pyld_len = rxbuf[1];

    /*
     * No need to alloc rxpdu for packets with invalid CRC, we would throw them
     * away instantly from LL anyway.
     */
    if (!BLE_MBUF_HDR_CRC_OK(rxhdr)) {
        alloc_rxpdu = false;
    }

#if MYNEWT_PKG_apache_mynewt_nimble__nimble_transport_common_hci_ipc
    /* If IPC transport is used, make sure there is buffer available on app side
     * for this PDU. We'll just nak in LL if there are no free buffers.
     */
    if (alloc_rxpdu && BLE_LL_LLID_IS_DATA(hdr_byte) && (rx_pyld_len > 0)) {
        if (hci_ipc_get(HCI_IPC_TYPE_ACL)) {
            rxhdr->rxinfo.flags |= BLE_MBUF_HDR_F_CONN_CREDIT_INT;
        } else {
            alloc_rxpdu = false;
        }
    }
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_CTRL_TO_HOST_FLOW_CONTROL)
    /*
     * If flow control is enabled, we need to have credit available for each
     * non-empty data packet that LL may send to host. If there are no credits
     * available, we don't need to allocate buffer for this packet so LL will
     * nak it.
     */
    if (alloc_rxpdu && ble_ll_conn_cth_flow_is_enabled() &&
        BLE_LL_LLID_IS_DATA(hdr_byte) && (rx_pyld_len > 0)) {
        if (ble_ll_conn_cth_flow_alloc_credit(connsm)) {
            rxhdr->rxinfo.flags |= BLE_MBUF_HDR_F_CONN_CREDIT;
        } else {
#if MYNEWT_PKG_apache_mynewt_nimble__nimble_transport_common_hci_ipc
            /* Need to return app buffer to pool since we won't use it */
            hci_ipc_put(HCI_IPC_TYPE_ACL);
#endif
            alloc_rxpdu = false;
        }
    }
#endif

    /*
     * We need to attempt to allocate a buffer here. The reason we do this
     * now is that we should not ack the packet if we have no receive
     * buffers available. We want to free up our transmit PDU if it was
     * acked, but we should not ack the received frame if we cant hand it up.
     * NOTE: we hand up empty pdu's to the LL task!
     */
    if (alloc_rxpdu) {
        rxpdu = ble_ll_rxpdu_alloc(rx_pyld_len + BLE_LL_PDU_HDR_LEN);
    }

    /*
     * We should have a current connection state machine. If we dont, we just
     * hand the packet to the higher layer to count it.
     */
    if (!connsm) {
        STATS_INC(ble_ll_conn_stats, rx_data_pdu_no_conn);
        goto conn_exit;
    }

    /*
     * Calculate the end time of the received PDU. NOTE: this looks strange
     * but for the 32768 crystal we add the time it takes to send the packet
     * to the 'additional usecs' field to save some calculations.
     */
    begtime = rxhdr->beg_cputime;
#if BLE_LL_BT5_PHY_SUPPORTED
    rx_phy_mode = connsm->phy_data.rx_phy_mode;
#else
    rx_phy_mode = BLE_PHY_MODE_1M;
#endif
    add_usecs = rxhdr->rem_usecs +
                ble_ll_pdu_us(rx_pyld_len, rx_phy_mode);

    /*
     * Check the packet CRC. A connection event can continue even if the
     * received PDU does not pass the CRC check. If we receive two consecutive
     * CRC errors we end the connection event.
     */
    if (!BLE_MBUF_HDR_CRC_OK(rxhdr)) {
        /*
         * Increment # of consecutively received CRC errors. If more than
         * one we will end the connection event.
         */
        ++connsm->cons_rxd_bad_crc;
        if (connsm->cons_rxd_bad_crc >= 2) {
            reply = 0;
        } else {
            switch (connsm->conn_role) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
            case BLE_LL_CONN_ROLE_CENTRAL:
                reply = connsm->flags.last_txd_md;
                break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
            case BLE_LL_CONN_ROLE_PERIPHERAL:
                /* A peripheral always responds with a packet */
                reply = 1;
                break;
#endif
            default:
                BLE_LL_ASSERT(0);
                break;
            }
        }
    } else {
        /* Reset consecutively received bad crcs (since this one was good!) */
        connsm->cons_rxd_bad_crc = 0;

        /* Set last valid received pdu time (resets supervision timer) */
        connsm->last_rxd_pdu_cputime = begtime + ble_ll_tmr_u2t(add_usecs);

        /* Set last received header byte */
        connsm->last_rxd_hdr_byte = hdr_byte;

        if (BLE_LL_LLID_IS_CTRL(hdr_byte)) {
            opcode = rxbuf[2];
        }

        /*
         * If SN bit from header does not match NESN in connection, this is
         * a resent PDU and should be ignored.
         */
        hdr_sn = hdr_byte & BLE_LL_DATA_HDR_SN_MASK;
        conn_nesn = connsm->next_exp_seqnum;
        if (rxpdu && ((hdr_sn && conn_nesn) || (!hdr_sn && !conn_nesn))) {
            connsm->next_exp_seqnum ^= 1;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
            if (connsm->flags.encrypted && !ble_ll_conn_is_empty_pdu(rxbuf)) {
                ++connsm->enc_data.rx_pkt_cntr;
            }
#endif
        }

        ble_ll_trace_u32x2(BLE_LL_TRACE_ID_CONN_RX, connsm->tx_seqnum,
                           !!(hdr_byte & BLE_LL_DATA_HDR_NESN_MASK));

        /*
         * Check NESN bit from header. If same as tx seq num, the transmission
         * is acknowledged. Otherwise we need to resend this PDU.
         */
        if (connsm->flags.empty_pdu_txd || connsm->cur_tx_pdu) {
            hdr_nesn = hdr_byte & BLE_LL_DATA_HDR_NESN_MASK;
            conn_sn = connsm->tx_seqnum;
            if ((hdr_nesn && conn_sn) || (!hdr_nesn && !conn_sn)) {
                /* We did not get an ACK. Must retry the PDU */
                STATS_INC(ble_ll_conn_stats, data_pdu_txf);
            } else {
                /* Transmit success */
                connsm->tx_seqnum ^= 1;
                STATS_INC(ble_ll_conn_stats, data_pdu_txg);

                /* If we transmitted the empty pdu, clear flag */
                if (connsm->flags.empty_pdu_txd) {
                    connsm->flags.empty_pdu_txd = 0;
                    goto chk_rx_terminate_ind;
                }

                /*
                 * Determine if we should remove packet from queue or if there
                 * are more fragments to send.
                 */
                txpdu = connsm->cur_tx_pdu;
                if (txpdu) {
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
                    if (connsm->enc_data.tx_encrypted) {
                        ++connsm->enc_data.tx_pkt_cntr;
                    }
#endif
                    txhdr = BLE_MBUF_HDR_PTR(txpdu);
                    if ((txhdr->txinfo.hdr_byte & BLE_LL_DATA_HDR_LLID_MASK)
                        == BLE_LL_LLID_CTRL) {
                        connsm->cur_tx_pdu = NULL;
                        /* Note: the mbuf is freed by this call */
                        rc = ble_ll_ctrl_tx_done(txpdu, connsm);
                        if (rc) {
                            /* Means we transmitted a TERMINATE_IND */
                            goto conn_exit;
                        } else {
                            goto chk_rx_terminate_ind;
                        }
                    }

                    /* Increment offset based on number of bytes sent */
                    txhdr->txinfo.offset += txhdr->txinfo.pyld_len;
                    if (txhdr->txinfo.offset >= OS_MBUF_PKTLEN(txpdu)) {
                        /* If l2cap pdu, increment # of completed packets */
                        if (txhdr->txinfo.pyld_len != 0) {
#if (BLETEST_THROUGHPUT_TEST == 1)
                            bletest_completed_pkt(connsm->conn_handle);
#endif
                            ++connsm->completed_pkts;
                            if (connsm->completed_pkts > 2) {
                                ble_ll_event_add(&g_ble_ll_data.ll_comp_pkt_ev);
                            }
                        }
                        os_mbuf_free_chain(txpdu);
                        connsm->cur_tx_pdu = NULL;
                    } else {
                        rem_bytes = OS_MBUF_PKTLEN(txpdu) - txhdr->txinfo.offset;
                        /* Adjust payload for max TX time and octets */

#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
                        if (BLE_LL_LLID_IS_CTRL(hdr_byte) &&
                            CONN_IS_PERIPHERAL(connsm) &&
                            (opcode == BLE_LL_CTRL_PHY_UPDATE_IND)) {
                            connsm->phy_tx_transition =
                                    ble_ll_ctrl_phy_tx_transition_get(rxbuf[3]);
                        }
#endif

                        rem_bytes = ble_ll_conn_adjust_pyld_len(connsm, rem_bytes);
                        txhdr->txinfo.pyld_len = rem_bytes;
                    }
                }
            }
        }

        /* Should we continue connection event? */
        /* If this is a TERMINATE_IND, we have to reply */
chk_rx_terminate_ind:
        /* If we received a terminate IND, we must set some flags */
        if (BLE_LL_LLID_IS_CTRL(hdr_byte) &&
            (opcode == BLE_LL_CTRL_TERMINATE_IND) &&
            (rx_pyld_len == (1 + BLE_LL_CTRL_TERMINATE_IND_LEN))) {
            connsm->flags.terminate_ind_rxd = 1;
            connsm->rxd_disconnect_reason = rxbuf[3];
        }

        switch (connsm->conn_role) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
        case BLE_LL_CONN_ROLE_CENTRAL:
            reply = connsm->flags.last_txd_md || (hdr_byte & BLE_LL_DATA_HDR_MD_MASK);
            break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
        case BLE_LL_CONN_ROLE_PERIPHERAL:
            /* A peripheral always replies */
            reply = 1;
            break;
#endif
        default:
            BLE_LL_ASSERT(0);
            break;
        }
    }

    /* If reply flag set, send data pdu and continue connection event */
    rc = -1;
    if (rx_pyld_len && connsm->flags.encrypted) {
        rx_pyld_len += BLE_LL_DATA_MIC_LEN;
    }
    if (reply && ble_ll_conn_can_send_next_pdu(connsm, begtime, add_usecs)) {
        rc = ble_ll_conn_tx_pdu(connsm);
    }

conn_exit:
    /* Copy the received pdu and hand it up */
    if (rxpdu) {
        ble_phy_rxpdu_copy(rxbuf, rxpdu);
        ble_ll_rx_pdu_in(rxpdu);
    }

    /* Send link layer a connection end event if over */
    if (rc) {
        ble_ll_conn_current_sm_over(connsm);
    }

    return rc;
}

/**
 * Called to adjust payload length to fit into max effective octets and TX time
 * on current PHY.
 */
/**
 * Called to enqueue a packet on the transmit queue of a connection. Should
 * only be called by the controller.
 *
 * Context: Link Layer
 *
 *
 * @param connsm
 * @param om
 */
void
ble_ll_conn_enqueue_pkt(struct ble_ll_conn_sm *connsm, struct os_mbuf *om,
                        uint8_t hdr_byte, uint16_t length)
{
    os_sr_t sr;
    struct os_mbuf_pkthdr *pkthdr;
    struct ble_mbuf_hdr *ble_hdr;
    int lifo;

    /* Set mbuf length and packet length if a control PDU */
    if (hdr_byte == BLE_LL_LLID_CTRL) {
        om->om_len = length;
        OS_MBUF_PKTHDR(om)->omp_len = length;
    }

    /* Set BLE transmit header */
    ble_hdr = BLE_MBUF_HDR_PTR(om);
    ble_hdr->txinfo.flags = 0;
    ble_hdr->txinfo.offset = 0;
    ble_hdr->txinfo.hdr_byte = hdr_byte;

    /*
     * Initial payload length is calculate when packet is dequeued, there's no
     * need to do this now.
     */

    lifo = 0;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_ENCRYPTION)
    if (connsm->enc_data.enc_state > CONN_ENC_S_ENCRYPTED) {
        uint8_t llid;

        /*
         * If this is one of the following types we need to insert it at
         * head of queue.
         */
        llid = ble_hdr->txinfo.hdr_byte & BLE_LL_DATA_HDR_LLID_MASK;
        if (llid == BLE_LL_LLID_CTRL) {
            switch (om->om_data[0]) {
            case BLE_LL_CTRL_TERMINATE_IND:
            case BLE_LL_CTRL_REJECT_IND:
            case BLE_LL_CTRL_REJECT_IND_EXT:
            case BLE_LL_CTRL_START_ENC_REQ:
            case BLE_LL_CTRL_START_ENC_RSP:
                lifo = 1;
                break;
            case BLE_LL_CTRL_PAUSE_ENC_RSP:
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
                if (connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
                    lifo = 1;
                }
#endif
                break;
            case BLE_LL_CTRL_ENC_REQ:
            case BLE_LL_CTRL_ENC_RSP:
                /* If encryption has been paused, we don't want to send any packets from the
                 * TX queue, as they would go unencrypted.
                 */
                if (connsm->enc_data.enc_state == CONN_ENC_S_PAUSED) {
                    lifo = 1;
                }
                break;
            default:
                break;
            }
        }
    }
#endif

    /* Add to transmit queue for the connection */
    pkthdr = OS_MBUF_PKTHDR(om);
    OS_ENTER_CRITICAL(sr);
    if (lifo) {
        STAILQ_INSERT_HEAD(&connsm->conn_txq, pkthdr, omp_next);
    } else {
        STAILQ_INSERT_TAIL(&connsm->conn_txq, pkthdr, omp_next);
    }
    OS_EXIT_CRITICAL(sr);
}

/**
 * Data packet from host.
 *
 * Context: Link Layer task
 *
 * @param om
 * @param handle
 * @param length
 *
 * @return int
 */
void
ble_ll_conn_tx_pkt_in(struct os_mbuf *om, uint16_t handle, uint16_t length)
{
    uint8_t hdr_byte;
    uint16_t conn_handle;
    uint16_t pb;
    struct ble_ll_conn_sm *connsm;

    /* See if we have an active matching connection handle */
    conn_handle = handle & 0x0FFF;
    connsm = ble_ll_conn_find_by_handle(conn_handle);
    if (connsm) {
        /* Construct LL header in buffer (NOTE: pb already checked) */
        pb = handle & 0x3000;
        if (pb == 0) {
            hdr_byte = BLE_LL_LLID_DATA_START;
        } else {
            hdr_byte = BLE_LL_LLID_DATA_FRAG;
        }

        /* Add to total l2cap pdus enqueue */
        STATS_INC(ble_ll_conn_stats, l2cap_enqueued);

        /* Clear flags field in BLE header */
        ble_ll_conn_enqueue_pkt(connsm, om, hdr_byte, length);
    } else {
        /* No connection found! */
        STATS_INC(ble_ll_conn_stats, handle_not_found);
        os_mbuf_free_chain(om);
    }
}
#endif

void
ble_ll_conn_chan_map_update(void)
{
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    struct ble_ll_conn_sm *connsm;

    /* Perform channel map update */
    SLIST_FOREACH(connsm, &g_ble_ll_conn_active_list, act_sle) {
        if (connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) {
            ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_CHAN_MAP_UPD, NULL);
        }
    }
#endif
}

#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
/**
 * Called when a device has received a connect request while advertising and
 * the connect request has passed the advertising filter policy and is for
 * us. This will start a connection in the peripheral role assuming that we dont
 * already have a connection with this device and that the connect request
 * parameters are valid.
 *
 * Context: Link Layer
 *
 * @param rxbuf Pointer to received Connect Request PDU
 *
 * @return 0: connection not started; 1 connecton started
 */
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
int
ble_ll_conn_periph_start(uint8_t *rxbuf, uint8_t pat, struct ble_mbuf_hdr *rxhdr,
                         bool force_csa2)
{
    int rc;
    uint32_t temp;
    uint32_t crcinit;
    uint8_t *inita;
    uint8_t *dptr;
    struct ble_ll_conn_sm *connsm;

    /* Ignore the connection request if we are already connected*/
    inita = rxbuf + BLE_LL_PDU_HDR_LEN;
    SLIST_FOREACH(connsm, &g_ble_ll_conn_active_list, act_sle) {
        if (!memcmp(&connsm->peer_addr, inita, BLE_DEV_ADDR_LEN)) {
            if (rxbuf[0] & BLE_ADV_PDU_HDR_TXADD_MASK) {
                if (connsm->peer_addr_type & 1) {
                    return 0;
                }
            } else {
                if ((connsm->peer_addr_type & 1) == 0) {
                    return 0;
                }
            }
        }
    }

    /* Allocate a connection. If none available, dont do anything */
    connsm = ble_ll_conn_sm_get();
    if (connsm == NULL) {
        return 0;
    }

    /* Set the pointer at the start of the connection data */
    dptr = rxbuf + BLE_LL_CONN_REQ_ADVA_OFF + BLE_DEV_ADDR_LEN;

    /* Set connection state machine information */
    connsm->access_addr = get_le32(dptr);
    crcinit = dptr[6];
    crcinit = (crcinit << 8) | dptr[5];
    crcinit = (crcinit << 8) | dptr[4];
    connsm->crcinit = crcinit;
    connsm->tx_win_size = dptr[7];
    connsm->tx_win_off = get_le16(dptr + 8);
    connsm->conn_itvl = get_le16(dptr + 10);
    connsm->periph_latency = get_le16(dptr + 12);
    connsm->supervision_tmo = get_le16(dptr + 14);
    memcpy(&connsm->chan_map, dptr + 16, BLE_LL_CHAN_MAP_LEN);
    connsm->hop_inc = dptr[21] & 0x1F;
    connsm->central_sca = dptr[21] >> 5;

    /* Error check parameters */
    if ((connsm->tx_win_off > connsm->conn_itvl) ||
        (connsm->conn_itvl < BLE_HCI_CONN_ITVL_MIN) ||
        (connsm->conn_itvl > BLE_HCI_CONN_ITVL_MAX) ||
        (connsm->tx_win_size < BLE_LL_CONN_TX_WIN_MIN) ||
        (connsm->periph_latency > BLE_LL_CONN_PERIPH_LATENCY_MAX) ||
        (connsm->hop_inc < 5) || (connsm->hop_inc > 16)) {
        goto err_periph_start;
    }

    /* Slave latency cannot cause a supervision timeout */
    temp = (connsm->periph_latency + 1) * (connsm->conn_itvl * 2) *
           BLE_LL_CONN_ITVL_USECS;
    if ((connsm->supervision_tmo * 10000) <= temp ) {
        goto err_periph_start;
    }

    /*
     * The transmit window must be less than or equal to the lesser of 10
     * msecs or the connection interval minus 1.25 msecs.
     */
    temp = connsm->conn_itvl - 1;
    if (temp > 8) {
        temp = 8;
    }
    if (connsm->tx_win_size > temp) {
        goto err_periph_start;
    }

    /* Set the address of device that we are connecting with */
    memcpy(&connsm->peer_addr, inita, BLE_DEV_ADDR_LEN);
    connsm->peer_addr_type = pat;

    /* Calculate number of used channels; make sure it meets min requirement */
    connsm->chan_map_used = ble_ll_utils_chan_map_used_get(connsm->chan_map);
    if (connsm->chan_map_used < 2) {
        goto err_periph_start;
    }

    ble_ll_conn_itvl_to_ticks(connsm->conn_itvl, &connsm->conn_itvl_ticks,
                              &connsm->conn_itvl_usecs);

    /* Start the connection state machine */
    connsm->conn_role = BLE_LL_CONN_ROLE_PERIPHERAL;
    ble_ll_conn_sm_new(connsm);

#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    /* Use the same PHY as we received CONNECT_REQ on */
    ble_ll_conn_init_phy(connsm, rxhdr->rxinfo.phy);
#endif

    ble_ll_conn_set_csa(connsm,
                        force_csa2 || (rxbuf[0] & BLE_ADV_PDU_HDR_CHSEL_MASK));

    /* Set initial schedule callback */
    connsm->conn_sch.sched_cb = ble_ll_conn_event_start_cb;
    rc = ble_ll_conn_created(connsm, rxhdr);
    if (!rc) {
        SLIST_REMOVE(&g_ble_ll_conn_active_list, connsm, ble_ll_conn_sm, act_sle);
        STAILQ_INSERT_TAIL(&g_ble_ll_conn_free_list, connsm, free_stqe);
    }
    return rc;

err_periph_start:
    STAILQ_INSERT_TAIL(&g_ble_ll_conn_free_list, connsm, free_stqe);
    STATS_INC(ble_ll_conn_stats, periph_rxd_bad_conn_req_params);
    return 0;
}
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
int
ble_ll_conn_subrate_req_hci(struct ble_ll_conn_sm *connsm,
                            struct ble_ll_conn_subrate_req_params *srp)
{
    uint32_t t1, t2;

    if ((srp->subrate_min < 0x0001) || (srp->subrate_min > 0x01f4) ||
        (srp->subrate_max < 0x0001) || (srp->subrate_max > 0x01f4) ||
        (srp->max_latency > 0x01f3) || (srp->cont_num > 0x01f3) ||
        (srp->supervision_tmo < 0x000a) || (srp->supervision_tmo > 0x0c80)) {
        return -EINVAL;
    }

    if (srp->subrate_max * (srp->max_latency + 1) > 500) {
        return -EINVAL;
    }

    t1 = connsm->conn_itvl * srp->subrate_max * (srp->max_latency + 1) *
         BLE_LL_CONN_ITVL_USECS;
    t2 = srp->supervision_tmo * BLE_HCI_CONN_SPVN_TMO_UNITS * 1000 / 2;
    if (t1 > t2) {
        return -EINVAL;
    }

    if (srp->subrate_max < srp->subrate_min) {
        return -EINVAL;
    }

    if (srp->cont_num >= srp->subrate_max) {
        return -EINVAL;
    }

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    if ((connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL) &&
        !ble_ll_conn_rem_feature_check(connsm,
                                       BLE_LL_FEAT_CONN_SUBRATING_HOST)) {
        return -ENOTSUP;
    }
#endif

    if (connsm->cur_ctrl_proc == BLE_LL_CTRL_PROC_CONN_PARAM_REQ) {
        return -EBUSY;
    }

    switch (connsm->conn_role) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    case BLE_LL_CONN_ROLE_CENTRAL:
        connsm->subrate_trans.subrate_factor = srp->subrate_max;
        connsm->subrate_trans.subrate_base_event = connsm->event_cntr;
        connsm->subrate_trans.periph_latency = srp->max_latency;
        connsm->subrate_trans.cont_num = srp->cont_num;
        connsm->subrate_trans.supervision_tmo = srp->supervision_tmo;
        connsm->flags.subrate_host_req = 1;
        ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_SUBRATE_UPDATE, NULL);
        break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    case BLE_LL_CONN_ROLE_PERIPHERAL:
        connsm->subrate_req = *srp;
        connsm->flags.subrate_host_req = 1;
        ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_SUBRATE_REQ, NULL);
        break;
#endif
    default:
        BLE_LL_ASSERT(0);
    }


    return 0;
}

int
ble_ll_conn_subrate_req_llcp(struct ble_ll_conn_sm *connsm,
                             struct ble_ll_conn_subrate_req_params *srp)
{
    BLE_LL_ASSERT(connsm->conn_role == BLE_LL_CONN_ROLE_CENTRAL);

    if ((srp->subrate_min < 0x0001) || (srp->subrate_min > 0x01f4) ||
        (srp->subrate_max < 0x0001) || (srp->subrate_max > 0x01f4) ||
        (srp->max_latency > 0x01f3) || (srp->cont_num > 0x01f3) ||
        (srp->supervision_tmo < 0x000a) || (srp->supervision_tmo > 0x0c80)) {
        return -EINVAL;
    }

    if (connsm->cur_ctrl_proc == BLE_LL_CTRL_PROC_CONN_PARAM_REQ) {
        return -EBUSY;
    }

    if ((srp->max_latency > connsm->acc_max_latency) ||
        (srp->supervision_tmo > connsm->acc_supervision_tmo) ||
        (srp->subrate_max < connsm->acc_subrate_min) ||
        (srp->subrate_min > connsm->acc_subrate_max) ||
        ((connsm->conn_itvl * BLE_LL_CONN_ITVL_USECS * srp->subrate_min *
          (srp->max_latency + 1)) * 2 >= srp->supervision_tmo *
                                         BLE_HCI_CONN_SPVN_TMO_UNITS * 1000)) {
        return -EINVAL;
    }

    connsm->subrate_trans.subrate_factor = MIN(connsm->acc_subrate_max,
                                               srp->subrate_max);
    connsm->subrate_trans.subrate_base_event = connsm->event_cntr;
    connsm->subrate_trans.periph_latency = MIN(connsm->acc_max_latency,
                                               srp->max_latency);
    connsm->subrate_trans.cont_num = MIN(MAX(connsm->acc_cont_num,
                                             srp->cont_num),
                                         connsm->subrate_trans.subrate_factor - 1);
    connsm->subrate_trans.supervision_tmo = MIN(connsm->supervision_tmo,
                                                srp->supervision_tmo);

    ble_ll_ctrl_proc_start(connsm, BLE_LL_CTRL_PROC_SUBRATE_UPDATE, NULL);

    return 0;
}

void
ble_ll_conn_subrate_set(struct ble_ll_conn_sm *connsm,
                        struct ble_ll_conn_subrate_params *sp)
{
    int16_t event_cntr_diff;
    int16_t subrate_events_diff;
    uint8_t send_ev;

    /* Assume parameters were checked by caller */

    send_ev = connsm->flags.subrate_host_req ||
              (connsm->subrate_factor != sp->subrate_factor) ||
              (connsm->periph_latency != sp->periph_latency) ||
              (connsm->cont_num != sp->cont_num) ||
              (connsm->supervision_tmo != sp->supervision_tmo);

    connsm->subrate_factor = sp->subrate_factor;
    connsm->subrate_base_event = sp->subrate_base_event;
    connsm->periph_latency = sp->periph_latency;
    connsm->cont_num = sp->cont_num;
    connsm->supervision_tmo = sp->supervision_tmo;

    /* Let's update subrate base event to "latest" one */
    event_cntr_diff = connsm->event_cntr - connsm->subrate_base_event;
    subrate_events_diff = event_cntr_diff / connsm->subrate_factor;
    connsm->subrate_base_event += connsm->subrate_factor * subrate_events_diff;

    if (send_ev) {
        ble_ll_hci_ev_subrate_change(connsm, 0);
    }
}
#endif

#define MAX_TIME_UNCODED(_maxbytes) \
    ble_ll_pdu_us(_maxbytes + BLE_LL_DATA_MIC_LEN, BLE_PHY_MODE_1M);
#define MAX_TIME_CODED(_maxbytes) \
    ble_ll_pdu_us(_maxbytes + BLE_LL_DATA_MIC_LEN, BLE_PHY_MODE_CODED_125KBPS);

/**
 * Called to reset the connection module. When this function is called the
 * scheduler has been stopped and the phy has been disabled. The LL should
 * be in the standby state.
 *
 * Context: Link Layer task
 */
void
ble_ll_conn_module_reset(void)
{
    uint8_t max_phy_pyld;
    uint16_t maxbytes;
    struct ble_ll_conn_sm *connsm;
    struct ble_ll_conn_global_params *conn_params;

    /* Kill the current one first (if one is running) */
    if (g_ble_ll_conn_cur_sm) {
        connsm = g_ble_ll_conn_cur_sm;
        g_ble_ll_conn_cur_sm = NULL;
        ble_ll_conn_end(connsm, BLE_ERR_SUCCESS);
    }

#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    /* Free the global connection complete event if there is one */
    if (g_ble_ll_conn_comp_ev) {
        ble_transport_free(g_ble_ll_conn_comp_ev);
        g_ble_ll_conn_comp_ev = NULL;
    }

    /* Reset connection we are attempting to create */
    g_ble_ll_conn_create_sm.connsm = NULL;
#endif

    /* Now go through and end all the connections */
    while (1) {
        connsm = SLIST_FIRST(&g_ble_ll_conn_active_list);
        if (!connsm) {
            break;
        }
        ble_ll_conn_end(connsm, BLE_ERR_SUCCESS);
    }

    /* Get the maximum supported PHY PDU size from the PHY */
    max_phy_pyld = ble_phy_max_data_pdu_pyld();

    /* Configure the global LL parameters */
    conn_params = &g_ble_ll_conn_params;

    maxbytes = MIN(MYNEWT_VAL(BLE_LL_SUPP_MAX_RX_BYTES), max_phy_pyld);
    conn_params->supp_max_rx_octets = maxbytes;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    conn_params->supp_max_rx_time = MAX_TIME_CODED(maxbytes);
#else
    conn_params->supp_max_rx_time = MAX_TIME_UNCODED(maxbytes);
#endif

    maxbytes = MIN(MYNEWT_VAL(BLE_LL_SUPP_MAX_TX_BYTES), max_phy_pyld);
    conn_params->supp_max_tx_octets = maxbytes;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LE_CODED_PHY)
    conn_params->supp_max_tx_time = MAX_TIME_CODED(maxbytes);
#else
    conn_params->supp_max_tx_time = MAX_TIME_UNCODED(maxbytes);
#endif

    maxbytes = MIN(MYNEWT_VAL(BLE_LL_CONN_INIT_MAX_TX_BYTES), max_phy_pyld);
    conn_params->conn_init_max_tx_octets = maxbytes;
    conn_params->conn_init_max_tx_time = MAX_TIME_UNCODED(maxbytes);
    conn_params->conn_init_max_tx_time_uncoded = MAX_TIME_UNCODED(maxbytes);
    conn_params->conn_init_max_tx_time_coded = MAX_TIME_CODED(maxbytes);

    conn_params->sugg_tx_octets = BLE_LL_CONN_SUPP_BYTES_MIN;
    conn_params->sugg_tx_time = BLE_LL_CONN_SUPP_TIME_MIN;

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_ENHANCED_CONN_UPDATE)
    conn_params->acc_subrate_min = 0x0001;
    conn_params->acc_subrate_max = 0x0001;
    conn_params->acc_max_latency = 0x0000;
    conn_params->acc_cont_num = 0x0000;
    conn_params->acc_supervision_tmo = 0x0c80;
#endif

    /* Reset statistics */
    STATS_RESET(ble_ll_conn_stats);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV_SYNC_TRANSFER)
    /* reset default sync transfer params */
    g_ble_ll_conn_sync_transfer_params.max_skip = 0;
    g_ble_ll_conn_sync_transfer_params.mode = 0;
    g_ble_ll_conn_sync_transfer_params.sync_timeout_us = 0;
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_CTRL_TO_HOST_FLOW_CONTROL)
    g_ble_ll_conn_cth_flow.enabled = false;
    g_ble_ll_conn_cth_flow.max_buffers = 1;
    g_ble_ll_conn_cth_flow.num_buffers = 1;
#endif

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    g_ble_ll_conn_css_next_slot = BLE_LL_CONN_CSS_NO_SLOT;
#endif
}

/* Initialize the connection module */
void
ble_ll_conn_module_init(void)
{
    int rc;
    uint16_t i;
    struct ble_ll_conn_sm *connsm;

    /* Initialize list of active connections */
    SLIST_INIT(&g_ble_ll_conn_active_list);
    STAILQ_INIT(&g_ble_ll_conn_free_list);

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    SLIST_INIT(&g_ble_ll_conn_css_list);
#endif

    /*
     * Take all the connections off the free memory pool and add them to
     * the free connection list, assigning handles in linear order. Note:
     * the specification allows a handle of zero; we just avoid using it.
     */
    connsm = &g_ble_ll_conn_sm[0];
    for (i = 0; i < MYNEWT_VAL(BLE_MAX_CONNECTIONS); ++i) {

        memset(connsm, 0, sizeof(struct ble_ll_conn_sm));
        connsm->conn_handle = i + 1;
        STAILQ_INSERT_TAIL(&g_ble_ll_conn_free_list, connsm, free_stqe);

        /* Initialize fixed schedule elements */
        connsm->conn_sch.sched_type = BLE_LL_SCHED_TYPE_CONN;
        connsm->conn_sch.cb_arg = connsm;

        ble_ll_ctrl_init_conn_sm(connsm);

        ++connsm;
    }

    /* Register connection statistics */
    rc = stats_init_and_reg(STATS_HDR(ble_ll_conn_stats),
                            STATS_SIZE_INIT_PARMS(ble_ll_conn_stats, STATS_SIZE_32),
                            STATS_NAME_INIT_PARMS(ble_ll_conn_stats),
                            "ble_ll_conn");
    BLE_LL_ASSERT(rc == 0);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_CTRL_TO_HOST_FLOW_CONTROL)
    ble_npl_event_init(&g_ble_ll_conn_cth_flow_error_ev,
                       ble_ll_conn_cth_flow_error_fn, NULL);
#endif

    /* Call reset to finish reset of initialization */
    ble_ll_conn_module_reset();
}

#endif
#endif
