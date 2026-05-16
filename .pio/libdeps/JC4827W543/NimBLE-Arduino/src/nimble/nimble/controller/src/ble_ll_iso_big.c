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

#include <errno.h>
#include <stdint.h>
#include <nimble/porting/nimble/include/syscfg/syscfg.h>
#include <nimble/nimble/include/nimble/ble.h>
#include <nimble/nimble/include/nimble/hci_common.h>
#include <nimble/nimble/transport/include/nimble/transport.h>
#include <nimble/nimble/controller/include/controller/ble_ll.h>
#include <nimble/nimble/controller/include/controller/ble_ll_adv.h>
#include <nimble/nimble/controller/include/controller/ble_ll_crypto.h>
#include <nimble/nimble/controller/include/controller/ble_ll_hci.h>
#include <nimble/nimble/controller/include/controller/ble_ll_isoal.h>
#include <nimble/nimble/controller/include/controller/ble_ll_iso_big.h>
#include <nimble/nimble/controller/include/controller/ble_ll_pdu.h>
#include <nimble/nimble/controller/include/controller/ble_ll_sched.h>
#include <nimble/nimble/controller/include/controller/ble_ll_rfmgmt.h>
#include <nimble/nimble/controller/include/controller/ble_ll_tmr.h>
#include <nimble/nimble/controller/include/controller/ble_ll_utils.h>
#include <nimble/nimble/controller/include/controller/ble_ll_whitelist.h>
#include "ble_ll_priv.h"

#if MYNEWT_VAL(BLE_LL_ISO_BROADCASTER)

/* XXX make those configurable */
#define BIG_POOL_SIZE           (10)
#define BIS_POOL_SIZE           (10)

#define BIG_HANDLE_INVALID      (0xff)

#define BIG_CONTROL_ACTIVE_CHAN_MAP     1
#define BIG_CONTROL_ACTIVE_TERM         2

struct ble_ll_iso_big;

struct ble_ll_iso_bis {
    struct ble_ll_iso_big *big;
    uint8_t num;
    uint16_t conn_handle;

    uint32_t aa;
    uint32_t crc_init;
    uint16_t chan_id;
    uint8_t iv[8];

    struct {
        uint16_t prn_sub_lu;
        uint16_t remap_idx;

        uint8_t subevent_num;
        uint8_t n;
        uint8_t g;
    } tx;

    struct ble_ll_isoal_mux mux;
    uint16_t num_completed_pkt;

    STAILQ_ENTRY(ble_ll_iso_bis) bis_q_next;
};

STAILQ_HEAD(ble_ll_iso_bis_q, ble_ll_iso_bis);

struct big_params {
    uint8_t nse; /* 1-31 */
    uint8_t bn; /* 1-7, mandatory 1 */
    uint8_t irc; /* 1-15  */
    uint8_t pto; /* 0-15, mandatory 0 */
    uint32_t sdu_interval;
    uint16_t iso_interval;
    uint16_t max_transport_latency;
    uint16_t max_sdu;
    uint8_t max_pdu;
    uint8_t phy;
    uint8_t interleaved : 1;
    uint8_t framed : 1;
    uint8_t encrypted : 1;
    uint8_t broadcast_code[16];
};

struct ble_ll_iso_big {
    struct ble_ll_adv_sm *advsm;

    uint8_t handle;
    uint8_t num_bis;
    uint16_t iso_interval;
    uint16_t bis_spacing;
    uint16_t sub_interval;
    uint8_t phy;
    uint8_t max_pdu;
    uint16_t max_sdu;
    uint16_t mpt;
    uint8_t bn; /* 1-7, mandatory 1 */
    uint8_t pto; /* 0-15, mandatory 0 */
    uint8_t irc; /* 1-15  */
    uint8_t nse; /* 1-31 */
    uint8_t interleaved : 1;
    uint8_t framed : 1;
    uint8_t encrypted : 1;
    uint8_t giv[8];
    uint8_t gskd[16];
    uint8_t gsk[16];
    uint8_t iv[8];
    uint8_t gc;

    uint32_t sdu_interval;

    uint32_t ctrl_aa;
    uint16_t crc_init;
    uint8_t chan_map[BLE_LL_CHAN_MAP_LEN];
    uint8_t chan_map_used;

    uint8_t biginfo[33];

    uint64_t big_counter;
    uint64_t bis_counter;

    uint32_t sync_delay;
    uint32_t event_start;
    uint8_t event_start_us;
    struct ble_ll_sched_item sch;
    struct ble_npl_event event_done;

    struct {
        uint16_t subevents_rem;
        struct ble_ll_iso_bis *bis;
    } tx;

    struct ble_ll_iso_bis_q bis_q;

    uint8_t cstf : 1;
    uint8_t cssn : 4;
    uint8_t control_active : 3;
    uint16_t control_instant;

    uint8_t chan_map_new_pending : 1;
    uint8_t chan_map_new[BLE_LL_CHAN_MAP_LEN];

    uint8_t term_pending : 1;
    uint8_t term_reason : 7;
};

static struct ble_ll_iso_big big_pool[BIG_POOL_SIZE];
static struct ble_ll_iso_bis bis_pool[BIS_POOL_SIZE];
static uint8_t big_pool_free = BIG_POOL_SIZE;
static uint8_t bis_pool_free = BIS_POOL_SIZE;

static struct ble_ll_iso_big *big_pending;
static struct ble_ll_iso_big *big_active;

struct ble_ll_iso_bis *
ble_ll_iso_big_find_bis_by_handle(uint16_t conn_handle)
{
    struct ble_ll_iso_bis *bis;
    uint8_t bis_idx;

    if (!BLE_LL_CONN_HANDLE_IS_BIS(conn_handle)) {
        return NULL;
    }

    bis_idx = BLE_LL_CONN_HANDLE_IDX(conn_handle);

    if (bis_idx >= BIS_POOL_SIZE) {
        return NULL;
    }

    bis = &bis_pool[bis_idx];
    if (!bis->big) {
        return NULL;
    }

    return bis;
}

struct ble_ll_isoal_mux *
ble_ll_iso_big_find_mux_by_handle(uint16_t conn_handle)
{
    struct ble_ll_iso_bis *bis;

    bis = ble_ll_iso_big_find_bis_by_handle(conn_handle);
    if (bis) {
        return &bis->mux;
    }

    return NULL;
}

int
ble_ll_iso_big_last_tx_timestamp_get(struct ble_ll_iso_bis *bis,
                                     uint16_t *packet_seq_num, uint32_t *timestamp)
{
    struct ble_ll_iso_big *big;

    big = bis->big;

    *packet_seq_num = big->bis_counter;
    *timestamp = (uint64_t)big->event_start * 1000000 / 32768 +
                 big->event_start_us;

    return 0;
}

static void
ble_ll_iso_big_biginfo_calc(struct ble_ll_iso_big *big, uint32_t seed_aa)
{
    uint8_t *buf;

    buf = big->biginfo;

    /* big_offset, big_offset_units, iso_interval, num_bis */
    put_le32(buf, (big->num_bis << 27) | (big->iso_interval << 15));
    buf += 4;

    /* nse, bn */
    *(uint8_t *)buf = (big->bn << 5) | (big->nse);
    buf += 1;

    /* sub_interval, pto */
    put_le24(buf,(big->pto << 20) | (big->sub_interval));
    buf += 3;

    /* bis_spacing, irc */
    put_le24(buf, (big->irc << 20) | (big->bis_spacing));
    buf += 3;

    /* max_pdu, rfu */
    put_le16(buf, big->max_pdu);
    buf += 2;

    /* seed_access_address */
    put_le32(buf, seed_aa);
    buf += 4;

    /* sdu_interval, max_sdu */
    put_le32(buf, (big->max_sdu << 20) | (big->sdu_interval));
    buf += 4;

    /* base_crc_init */
    put_le16(buf, big->crc_init);
    buf += 2;

    /* chm, phy */
    memcpy(buf, big->chan_map, 5);
    buf[4] |= (big->phy - 1) << 5;
    buf += 5;

    /* bis_payload_cnt, framing */
    memset(buf, 0x00, 5);
}

int
ble_ll_iso_big_biginfo_copy(struct ble_ll_iso_big *big, uint8_t *dptr,
                            uint32_t base_ticks, uint8_t base_rem_us)
{
    uint8_t *dptr_start;
    uint64_t counter;
    uint32_t offset_us;
    uint32_t offset;
    uint32_t d_ticks;
    uint8_t d_rem_us;

    dptr_start = dptr;
    counter = big->bis_counter;

    d_ticks = big->event_start - base_ticks;
    d_rem_us = big->event_start_us;
    ble_ll_tmr_sub(&d_ticks, &d_rem_us, base_rem_us);

    offset_us = ble_ll_tmr_t2u(d_ticks) + d_rem_us;
    if (offset_us <= 600) {
        counter += big->bn;
        offset_us += big->iso_interval * 1250;
    }
    if (offset_us >= 491460) {
        offset = 0x4000 | (offset_us / 300);
    } else {
        offset = offset_us / 30;
    }

    *dptr++ = ble_ll_iso_big_biginfo_len(big) - 1;
    *dptr++ = 0x2c;

    memcpy(dptr, big->biginfo, 33);
    put_le32(dptr, get_le32(dptr) | (offset & 0x7fff));
    dptr += 28;

    *dptr++ = counter & 0xff;
    *dptr++ = (counter >> 8) & 0xff;
    *dptr++ = (counter >> 16) & 0xff;
    *dptr++ = (counter >> 24) & 0xff;
    *dptr++ = (counter >> 32) & 0xff;

    if (big->encrypted) {
        memcpy(dptr, big->giv, 8);
        dptr += 8;
        memcpy(dptr, big->gskd, 16);
        dptr += 16;
    }

    return dptr - dptr_start;
}

int
ble_ll_iso_big_biginfo_len(struct ble_ll_iso_big *big)
{
    return 2 + sizeof(big->biginfo) +
           (big->encrypted ? sizeof(big->giv) + sizeof(big->gskd) : 0);
}

static void
ble_ll_iso_big_free(struct ble_ll_iso_big *big)
{
    struct ble_ll_iso_bis *bis;

    if (big->handle == BIG_HANDLE_INVALID) {
        return;
    }

    big->handle = BIG_HANDLE_INVALID;
    ble_ll_sched_rmv_elem(&big->sch);
    ble_npl_eventq_remove(&g_ble_ll_data.ll_evq, &big->event_done);

    STAILQ_FOREACH(bis, &big->bis_q, bis_q_next) {
        ble_ll_isoal_mux_free(&bis->mux);
        bis->big = NULL;
        bis_pool_free++;
    }

    big_pool_free++;
}

static void
ble_ll_iso_big_terminate_complete(struct ble_ll_iso_big *big)
{
    struct ble_hci_ev *hci_ev;
    struct ble_hci_ev_le_subev_terminate_big_complete *evt;
    uint8_t big_handle;
    uint8_t reason;

    big_handle = big->handle;
    reason = big->term_reason;

    ble_ll_iso_big_free(big);

    hci_ev = ble_transport_alloc_evt(0);
    if (!hci_ev) {
        BLE_LL_ASSERT(0);
        return;
    }
    hci_ev->opcode = BLE_HCI_EVCODE_LE_META;
    hci_ev->length = sizeof(*evt);

    evt = (void *)hci_ev->data;
    memset(evt, 0, hci_ev->length);
    evt->subev_code = BLE_HCI_LE_SUBEV_TERMINATE_BIG_COMPLETE;
    evt->big_handle = big_handle;
    evt->reason = reason;

    ble_transport_to_hs_evt(hci_ev);
}

static void
ble_ll_iso_big_chan_map_update_complete(struct ble_ll_iso_big *big)
{
    memcpy(big->chan_map, big->chan_map_new, BLE_LL_CHAN_MAP_LEN);
    big->chan_map_used = ble_ll_utils_chan_map_used_get(big->chan_map);
}

static void
ble_ll_iso_big_update_event_start(struct ble_ll_iso_big *big)
{
    os_sr_t sr;

    OS_ENTER_CRITICAL(sr);
    big->event_start = big->sch.start_time + g_ble_ll_sched_offset_ticks;
    big->event_start_us = big->sch.remainder;
    OS_EXIT_CRITICAL(sr);
}

static void
ble_ll_iso_big_event_done(struct ble_ll_iso_big *big)
{
    struct ble_ll_iso_bis *bis;
    struct ble_hci_ev *hci_ev;
    struct ble_hci_ev_num_comp_pkts *hci_ev_ncp;
    int num_completed_pkt;
    int rc;

    ble_ll_rfmgmt_release();

    if (big->big_counter == 0) {
        BLE_LL_ASSERT(big == big_pending);
        ble_ll_iso_big_hci_evt_complete();
    }

    hci_ev = ble_transport_alloc_evt(1);
    if (hci_ev) {
        hci_ev->opcode = BLE_HCI_EVCODE_NUM_COMP_PKTS;
        hci_ev->length = sizeof(*hci_ev_ncp);
        hci_ev_ncp = (void *)hci_ev->data;
        hci_ev_ncp->count = 0;
    }

    STAILQ_FOREACH(bis, &big->bis_q, bis_q_next) {
        num_completed_pkt = ble_ll_isoal_mux_event_done(&bis->mux);
        if (hci_ev && num_completed_pkt) {
            hci_ev_ncp->completed[hci_ev_ncp->count].handle =
                htole16(bis->conn_handle);
            hci_ev_ncp->completed[hci_ev_ncp->count].packets =
                htole16(num_completed_pkt + bis->num_completed_pkt);
            bis->num_completed_pkt = 0;
            hci_ev_ncp->count++;
        } else {
            bis->num_completed_pkt += num_completed_pkt;
        }
    }

    if (hci_ev) {
        if (hci_ev_ncp->count) {
            hci_ev->length = sizeof(*hci_ev_ncp) +
                             hci_ev_ncp->count * sizeof(hci_ev_ncp->completed[0]);
            ble_transport_to_hs_evt(hci_ev);
        } else {
            ble_transport_free(hci_ev);
        }
    }

    big->sch.start_time = big->event_start;
    big->sch.remainder = big->event_start_us;

    do {
        big->big_counter++;
        big->bis_counter += big->bn;

        if (big->control_active &&
            (big->control_instant == (uint16_t)big->big_counter)) {
            switch (big->control_active) {
            case BIG_CONTROL_ACTIVE_TERM:
                ble_ll_iso_big_terminate_complete(big);
                return;
            case BIG_CONTROL_ACTIVE_CHAN_MAP:
                ble_ll_iso_big_chan_map_update_complete(big);
                break;
            default:
                BLE_LL_ASSERT(0);
                break;
            }

            big->control_active = 0;
            big->cstf = 0;
        }

        if (!big->control_active) {
            if (big->term_pending) {
                big->term_pending = 0;
                big->control_active = BIG_CONTROL_ACTIVE_TERM;
            } else if (big->chan_map_new_pending) {
                memcpy(big->chan_map_new, g_ble_ll_data.chan_map,
                       BLE_LL_CHAN_MAP_LEN);
                big->chan_map_new_pending = 0;
                big->control_active = BIG_CONTROL_ACTIVE_CHAN_MAP;
            }

            if (big->control_active) {
                big->control_instant = big->big_counter + 6;
                big->cstf = 1;
                big->cssn += 1;
            }
        }

        /* XXX precalculate some data here? */

        ble_ll_tmr_add(&big->sch.start_time, &big->sch.remainder,
                       big->iso_interval * 1250);
        big->sch.end_time = big->sch.start_time +
                            ble_ll_tmr_u2t_up(big->sync_delay) + 1;
        big->sch.start_time -= g_ble_ll_sched_offset_ticks;

        if (big->control_active) {
            /* XXX fixme */
            big->sch.end_time += 10;
        }

        /* XXX this should always succeed since we preempt anything for now */
        rc = ble_ll_sched_iso_big(&big->sch, 0);
        assert(rc == 0);
    } while (rc < 0);

    ble_ll_iso_big_update_event_start(big);
}

static void
ble_ll_iso_big_event_done_ev(struct ble_npl_event *ev)
{
    struct ble_ll_iso_big *big;

    big = ble_npl_event_get_arg(ev);

    ble_ll_iso_big_event_done(big);
}

static void
ble_ll_iso_big_event_done_to_ll(struct ble_ll_iso_big *big)
{
    big_active = NULL;
    ble_ll_state_set(BLE_LL_STATE_STANDBY);
    ble_npl_eventq_put(&g_ble_ll_data.ll_evq, &big->event_done);
}

static uint8_t
ble_ll_iso_big_control_pdu_cb(uint8_t *dptr, void *arg, uint8_t *hdr_byte)
{
    struct ble_ll_iso_big *big;
    uint8_t len;

    big = arg;

    /* CSTF shall be set to 0 in Control PDU */
    *hdr_byte = 3 | (big->cssn << 2);

    BLE_LL_ASSERT(big->cstf);
    BLE_LL_ASSERT(big->control_active);

    if (big->encrypted) {
        ble_phy_encrypt_header_mask_set(BLE_LL_PDU_HEADERMASK_BIS);
        ble_phy_encrypt_iv_set(big->iv);
        ble_phy_encrypt_counter_set(big->bis_counter, 1);
    }

    switch (big->control_active) {
    case BIG_CONTROL_ACTIVE_CHAN_MAP:
        dptr[0] = 0x00; /* BIG_CHANNEL_MAP_IND */
        memcpy(&dptr[1], big->chan_map_new, BLE_LL_CHAN_MAP_LEN);
        put_le16(&dptr[6], big->control_instant);
        len = 8;
        break;
    case BIG_CONTROL_ACTIVE_TERM:
        dptr[0] = 0x01; /* BIG_TERMINATE_IND */
        dptr[1] = big->term_reason;
        put_le16(&dptr[2], big->control_instant);
        len = 4;
        break;
    default:
        BLE_LL_ASSERT(0);
        len = 0;
        break;
    }

    return len;
}

static void
ble_ll_iso_big_control_txend_cb(void *arg)
{
    struct ble_ll_iso_big *big;

    big = arg;

    ble_ll_iso_big_event_done_to_ll(big);
}

static int
ble_ll_iso_big_control_tx(struct ble_ll_iso_big *big)
{
    uint16_t chan_idx;
    uint16_t chan_id;
    uint16_t foo, bar;
    int rc;

    chan_id = big->ctrl_aa ^ (big->ctrl_aa >> 16);

    chan_idx = ble_ll_utils_dci_iso_event(big->big_counter, chan_id,
                                          &foo,
                                          big->chan_map_used,
                                          big->chan_map,
                                          &bar);

    ble_phy_set_txend_cb(ble_ll_iso_big_control_txend_cb, big);
    ble_phy_setchan(chan_idx, big->ctrl_aa, big->crc_init << 8);

    rc = ble_phy_tx(ble_ll_iso_big_control_pdu_cb, big, BLE_PHY_TRANSITION_NONE);

    return rc;
}

static uint8_t
ble_ll_iso_big_subevent_pdu_cb(uint8_t *dptr, void *arg, uint8_t *hdr_byte)
{
    struct ble_ll_iso_big *big;
    struct ble_ll_iso_bis *bis;
    int idx;
    uint8_t llid;
    uint8_t pdu_len;

    big = arg;
    bis = big->tx.bis;

    /* Core 5.3, Vol 6, Part B, 4.4.6.6 */
    if (bis->tx.g < big->irc) {
        idx = bis->tx.n;
    } else {
        idx = big->bn * big->pto * (bis->tx.g - big->irc + 1) + bis->tx.n;
    }

    if (big->encrypted) {
        ble_phy_encrypt_header_mask_set(BLE_LL_PDU_HEADERMASK_BIS);
        ble_phy_encrypt_iv_set(bis->iv);
        ble_phy_encrypt_counter_set(big->bis_counter + idx, 1);
    }

#if 1
    pdu_len = ble_ll_isoal_mux_unframed_get(&bis->mux, idx, &llid, dptr);
#else
    llid = 0;
    pdu_len = big->max_pdu;
    /* XXX dummy data for testing */
    memset(dptr, bis->num | (bis->num << 4), pdu_len);
    put_be32(dptr, big->big_counter);
    put_be32(&dptr[4], big->bis_counter + idx);
    dptr[8] = bis->tx.subevent_num;
    dptr[9] = bis->tx.g;
    dptr[10] = bis->tx.n;
    if (bis->tx.g == 0) {
        dptr[11] = 'B';
    } else if (bis->tx.g < big->irc) {
        dptr[11] = 'R';
    } else {
        dptr[11] = 'P';
    }
    dptr[12] = 0xff;
#endif

    *hdr_byte = llid | (big->cssn << 2) | (big->cstf << 5);

    return pdu_len;
}

static int
ble_ll_iso_big_subevent_tx(struct ble_ll_iso_big *big)
{
    struct ble_ll_iso_bis *bis;
    uint16_t chan_idx;
    int to_tx;
    int rc;

    bis = big->tx.bis;

    if (bis->tx.subevent_num == 1) {
        chan_idx = ble_ll_utils_dci_iso_event(big->big_counter, bis->chan_id,
                                              &bis->tx.prn_sub_lu,
                                              big->chan_map_used,
                                              big->chan_map,
                                              &bis->tx.remap_idx);
    } else {
        chan_idx = ble_ll_utils_dci_iso_subevent(bis->chan_id,
                                                 &bis->tx.prn_sub_lu,
                                                 big->chan_map_used,
                                                 big->chan_map,
                                                 &bis->tx.remap_idx);
    }

    ble_phy_setchan(chan_idx, bis->aa, bis->crc_init);

    to_tx = (big->tx.subevents_rem > 1) || big->cstf;

    rc = ble_phy_tx(ble_ll_iso_big_subevent_pdu_cb, big,
                    to_tx ? BLE_PHY_TRANSITION_TX_TX
                          : BLE_PHY_TRANSITION_NONE);
    return rc;
}

static void
ble_ll_iso_big_subevent_txend_cb(void *arg)
{
    struct ble_ll_iso_big *big;
    struct ble_ll_iso_bis *bis;
    int rc;

    big = arg;
    bis = big->tx.bis;

    bis->tx.n++;
    if (bis->tx.n == big->bn) {
        bis->tx.n = 0;
        bis->tx.g++;
    }

    /* Switch to next BIS if interleaved or all subevents for current BIS were
     * transmitted.
     */
    if (big->interleaved || (bis->tx.subevent_num == big->nse)) {
        bis = STAILQ_NEXT(bis, bis_q_next);
        if (!bis) {
            bis = STAILQ_FIRST(&big->bis_q);
        }
        big->tx.bis = bis;
    }

    bis->tx.subevent_num++;
    big->tx.subevents_rem--;

    if (big->tx.subevents_rem > 0) {
        rc = ble_ll_iso_big_subevent_tx(big);
        if (rc) {
            ble_phy_disable();
            ble_ll_iso_big_event_done_to_ll(big);
        }
        return;
    }

    if (big->cstf) {
        rc = ble_ll_iso_big_control_tx(big);
        if (rc) {
            ble_phy_disable();
            ble_ll_iso_big_event_done_to_ll(big);
        }
        return;
    }

    ble_ll_iso_big_event_done_to_ll(big);
}

static int
ble_ll_iso_big_event_sched_cb(struct ble_ll_sched_item *sch)
{
    struct ble_ll_iso_big *big;
    struct ble_ll_iso_bis *bis;
#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    uint8_t phy_mode;
#endif
    int rc;

    big = sch->cb_arg;

    ble_ll_state_set(BLE_LL_STATE_BIG);
    big_active = big;

    ble_ll_whitelist_disable();
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PRIVACY)
    ble_phy_resolv_list_disable();
#endif
#if (BLE_LL_BT5_PHY_SUPPORTED == 1)
    phy_mode = ble_ll_phy_to_phy_mode(big->phy, 0);
    ble_phy_mode_set(phy_mode, phy_mode);
#endif

    BLE_LL_ASSERT(!big->framed);

    /* XXX calculate this in advance at the end of previous event? */
    big->tx.subevents_rem = big->num_bis * big->nse;
    STAILQ_FOREACH(bis, &big->bis_q, bis_q_next) {
        ble_ll_isoal_mux_event_start(&bis->mux, (uint64_t)big->event_start *
                                                1000000 / 32768 +
                                                big->event_start_us);

        bis->tx.subevent_num = 0;
        bis->tx.n = 0;
        bis->tx.g = 0;
    }

    /* Select 1st BIS for transmission */
    big->tx.bis = STAILQ_FIRST(&big->bis_q);
    big->tx.bis->tx.subevent_num = 1;

    if (big->interleaved) {
        ble_phy_tifs_txtx_set(big->bis_spacing, 0);
    } else {
        ble_phy_tifs_txtx_set(big->sub_interval, 0);
    }

    rc = ble_phy_tx_set_start_time(sch->start_time + g_ble_ll_sched_offset_ticks,
                                   sch->remainder);
    if (rc) {
        ble_phy_disable();
        ble_ll_iso_big_event_done_to_ll(big);
        return BLE_LL_SCHED_STATE_DONE;
    }

    ble_phy_set_txend_cb(ble_ll_iso_big_subevent_txend_cb, big);

    if (big->encrypted) {
        ble_phy_encrypt_enable(big->gsk);
    } else {
        ble_phy_encrypt_disable();
    }

    rc = ble_ll_iso_big_subevent_tx(big);
    if (rc) {
        ble_phy_disable();
        ble_ll_iso_big_event_done_to_ll(big);
        return BLE_LL_SCHED_STATE_DONE;
    }

    return BLE_LL_SCHED_STATE_RUNNING;
}

static void
ble_ll_iso_big_calculate_gsk(struct ble_ll_iso_big *big,
                             const uint8_t *broadcast_code)
{
    static const uint8_t big1[16] = {0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00,
                                     0x42, 0x49, 0x47, 0x31};
    static const uint8_t big2[4] = {0x42, 0x49, 0x47, 0x32};
    static const uint8_t big3[4] = {0x42, 0x49, 0x47, 0x33};
    uint8_t code[16];
    uint8_t igltk[16];
    uint8_t gltk[16];

    /* broadcast code is lsb-first in hci, we need msb-first */
    swap_buf(code, broadcast_code, 16);

    ble_ll_rand_data_get(big->gskd, sizeof(big->gskd));

    ble_ll_crypto_h7(big1, code, igltk);
    ble_ll_crypto_h6(igltk, big2, gltk);
    ble_ll_crypto_h8(gltk, big->gskd, big3, big->gsk);

    /* need gskd for biginfo, i.e. lsb-first */
    swap_in_place(big->gskd, 16);
}

static void
ble_ll_iso_big_calculate_iv(struct ble_ll_iso_big *big)
{
    struct ble_ll_iso_bis *bis;
    uint8_t *aa;

    ble_ll_rand_data_get(big->giv, sizeof(big->giv));
    STAILQ_FOREACH(bis, &big->bis_q, bis_q_next) {
        memcpy(&bis->iv[4], &big->giv[4], 4);
        aa = (uint8_t *)&bis->aa;
        bis->iv[0] = big->giv[0] ^ aa[0];
        bis->iv[1] = big->giv[1] ^ aa[1];
        bis->iv[2] = big->giv[2] ^ aa[2];
        bis->iv[3] = big->giv[3] ^ aa[3];
    }

    memcpy(&big->iv[4], &big->giv[4], 4);
    aa = (uint8_t *)&big->ctrl_aa;
    big->iv[0] = big->giv[0] ^ aa[0];
    big->iv[1] = big->giv[1] ^ aa[1];
    big->iv[2] = big->giv[2] ^ aa[2];
    big->iv[3] = big->giv[3] ^ aa[3];
}

static int
ble_ll_iso_big_create(uint8_t big_handle, uint8_t adv_handle, uint8_t num_bis,
                      struct big_params *bp)
{
    struct ble_ll_iso_big *big = NULL;
    struct ble_ll_iso_bis *bis;
    struct ble_ll_adv_sm *advsm;
    uint32_t seed_aa;
    uint8_t pte;
    uint8_t gc;
    uint8_t idx;
    int rc;

    if ((big_pool_free == 0) || (bis_pool_free < num_bis)) {
        return -ENOMEM;
    }

    /* Find free BIG */
    for (idx = 0; idx < BIG_POOL_SIZE; idx++) {
        if (!big && big_pool[idx].handle == BIG_HANDLE_INVALID) {
            big = &big_pool[idx];
        }
        if (big_pool[idx].handle == big_handle) {
            return -EALREADY;
        }
    }

    BLE_LL_ASSERT(big);

    advsm = ble_ll_adv_sync_get(adv_handle);
    if (!advsm) {
        return -ENOENT;
    }

    if (ble_ll_adv_sync_big_add(advsm, big) < 0) {
        return -ENOENT;
    }

    big->advsm = advsm;
    big->handle = big_handle;

    big->crc_init = ble_ll_rand();
    memcpy(big->chan_map, g_ble_ll_data.chan_map, BLE_LL_CHAN_MAP_LEN);
    big->chan_map_used = g_ble_ll_data.chan_map_used;

    big->big_counter = 0;
    big->bis_counter = 0;

    big->cstf = 0;
    big->cssn = 0;
    big->control_active = 0;
    big->control_instant = 0;
    big->chan_map_new_pending = 0;
    big->term_pending = 0;
    big->term_reason = 0;

    /* Calculate number of additional events for pre-transmissions */
    /* Core 5.3, Vol 6, Part B, 4.4.6.6 */
    gc = bp->nse / bp->bn;
    if (bp->irc == gc) {
        pte = 0;
    } else {
        pte = bp->pto * (gc - bp->irc);
    }

    /* Allocate BISes */
    STAILQ_INIT(&big->bis_q);
    big->num_bis = 0;
    for (idx = 0; (big->num_bis < num_bis) && (idx < BIS_POOL_SIZE); idx++) {
        bis = &bis_pool[idx];
        if (bis->big) {
            continue;
        }

        big->num_bis++;
        STAILQ_INSERT_TAIL(&big->bis_q, bis, bis_q_next);

        bis->big = big;
        bis->num = big->num_bis;
        bis->crc_init = (big->crc_init << 8) | (big->num_bis);

        BLE_LL_ASSERT(!big->framed);

        ble_ll_isoal_mux_init(&bis->mux, bp->max_pdu, bp->iso_interval * 1250,
                              bp->sdu_interval, bp->bn, pte);
    }

    big_pool_free--;
    bis_pool_free -= num_bis;

    /* Calculate AA for each BIS and BIG Control. We have to repeat this process
     * until all AAs are valid.
     */
    do {
        rc = 0;

        seed_aa = ble_ll_utils_calc_seed_aa();
        big->ctrl_aa = ble_ll_utils_calc_big_aa(seed_aa, 0);

        if (!ble_ll_utils_verify_aa(big->ctrl_aa)) {
            continue;
        }

        rc = 1;

        STAILQ_FOREACH(bis, &big->bis_q, bis_q_next) {
            bis->aa = ble_ll_utils_calc_big_aa(seed_aa, bis->num);
            if (!ble_ll_utils_verify_aa(bis->aa)) {
                rc = 0;
                break;
            }
            bis->chan_id = bis->aa ^ (bis->aa >> 16);
        }
    } while (rc == 0);

    big->bn = bp->bn;
    big->pto = bp->pto;
    big->irc = bp->irc;
    big->nse = bp->nse;
    big->interleaved = bp->interleaved;
    big->framed = bp->framed;
    big->encrypted = bp->encrypted;
    big->sdu_interval = bp->sdu_interval;
    big->iso_interval = bp->iso_interval;
    big->phy = bp->phy;
    big->max_pdu = bp->max_pdu;
    big->max_sdu = bp->max_sdu;
    /* Core 5.3, Vol 6, Part B, 4.4.6.3 */
    big->mpt = ble_ll_pdu_us(big->max_pdu + (big->encrypted ? 4 : 0),
                             ble_ll_phy_to_phy_mode(big->phy,
                                                    BLE_HCI_LE_PHY_CODED_S8_PREF));

    /* Core 5.3, Vol 6, Part B, 4.4.6.4 */
    if (big->interleaved) {
        big->bis_spacing = big->mpt + BLE_LL_MSS;
        big->sub_interval = big->num_bis * big->bis_spacing;
    } else {
        big->sub_interval = big->mpt + BLE_LL_MSS;
        big->bis_spacing = big->nse * big->sub_interval;
    }
    /* Core 5.3, Vol 6, Part B, 4.4.6.5 */
    big->sync_delay = (big->num_bis - 1) * big->bis_spacing +
                      (big->nse - 1) * big->sub_interval + big->mpt;

    /* Reset PTO if pre-transmissions won't be used */
    big->gc = gc;
    if (big->irc == gc) {
        big->pto = 0;
    }

    if (big->encrypted) {
        ble_ll_iso_big_calculate_gsk(big, bp->broadcast_code);
        ble_ll_iso_big_calculate_iv(big);
    }

    ble_ll_iso_big_biginfo_calc(big, seed_aa);

    /* For now we will schedule complete event as single item. This allows for
     * shortest possible subevent space (150us) but can create sequence of long
     * events that will block scheduler from other activities. To mitigate this
     * we use preempt_none strategy so scheudling is opportunistic and depending
     * on other activities this may create gaps in stream.
     * Eventually we should allow for some more robust scheduling, e.g. per-BIS
     * for sequential arrangement or per-subevent for interleaved, or event
     * per-BIS-subevent but this requires larger subevent space since 150us is
     * not enough for some phys to run scheduler item.
     */

    /* Schedule 1st event a bit in future */
    /* FIXME schedule 6ms in the future to avoid conflict with periodic
     *       advertising when both are started at the same time; we should
     *       select this value in some smart way later...
     */
    big->sch.start_time = ble_ll_tmr_get() + ble_ll_tmr_u2t(6000);
    big->sch.remainder = 0;
    big->sch.end_time = big->sch.start_time +
                        ble_ll_tmr_u2t_up(big->sync_delay) + 1;
    big->sch.start_time -= g_ble_ll_sched_offset_ticks;

    rc = ble_ll_sched_iso_big(&big->sch, 1);
    if (rc < 0) {
        ble_ll_iso_big_free(big);
        return -EFAULT;
    }

    ble_ll_iso_big_update_event_start(big);

    big_pending = big;

    return 0;
}

static int
ble_ll_iso_big_terminate(uint8_t big_handle, uint8_t reason)
{
    struct ble_ll_iso_big *big = NULL;
    unsigned idx;

    for (idx = 0; idx < BIG_POOL_SIZE; idx++) {
        if (big_pool[idx].handle == big_handle) {
            big = &big_pool[idx];
            break;
        }
    }

    if (!big) {
        return -ENOENT;
    }

    /* Not sure if this is correct, but let's remove BIGInfo now since there's
     * no point for peer to syncing to a BIG that will be disconnected soon.
     */
    ble_ll_adv_sync_big_remove(big->advsm, big);

    big->term_reason = reason;
    big->term_pending = 1;

    return 0;
}

void
ble_ll_iso_big_chan_map_update(void)
{
    struct ble_ll_iso_big *big;
    int idx;

    for (idx = 0; idx < BIG_POOL_SIZE; idx++) {
        big = &big_pool[idx];

        if (big->handle == BIG_HANDLE_INVALID) {
            continue;
        }

        big->chan_map_new_pending = 1;
    }
}

void
ble_ll_iso_big_halt(void)
{
    if (big_active) {
        ble_ll_iso_big_event_done_to_ll(big_active);
    }
}

void
ble_ll_iso_big_hci_evt_complete(void)
{
    struct ble_ll_iso_big *big;
    struct ble_ll_iso_bis *bis;
    struct ble_hci_ev_le_subev_create_big_complete *evt;
    struct ble_hci_ev *hci_ev;
    uint8_t idx;

    big = big_pending;
    big_pending = NULL;

    if (!big) {
        return;
    }

    hci_ev = ble_transport_alloc_evt(0);
    if (!hci_ev) {
        BLE_LL_ASSERT(0);
        /* XXX should we retry later? */
        return;
    }
    hci_ev->opcode = BLE_HCI_EVCODE_LE_META;
    hci_ev->length = sizeof(*evt) + big->num_bis * sizeof(evt->conn_handle[0]);

    evt = (void *)hci_ev->data;
    memset(evt, 0, hci_ev->length);
    evt->subev_code = BLE_HCI_LE_SUBEV_CREATE_BIG_COMPLETE;
    evt->status = 0x00;

    evt->big_handle = big->handle;
    put_le24(evt->big_sync_delay, big->sync_delay);
    /* Core 5.3, Vol 6, Part G, 3.2.2 */
    put_le24(evt->transport_latency_big,
             big->sync_delay +
             (big->pto * (big->nse / big->bn - big->irc) + 1) * big->iso_interval * 1250 -
             big->sdu_interval);
    evt->phy = big->phy;
    evt->nse = big->nse;
    evt->bn = big->bn;
    evt->pto = big->pto;
    evt->irc = big->irc;
    evt->max_pdu = htole16(big->max_pdu);
    evt->iso_interval = htole16(big->iso_interval);
    evt->num_bis = big->num_bis;

    idx = 0;
    STAILQ_FOREACH(bis, &big->bis_q, bis_q_next) {
        evt->conn_handle[idx] = htole16(bis->conn_handle);
        idx++;
    }

    ble_ll_hci_event_send(hci_ev);
}

int
ble_ll_iso_big_hci_create(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_create_big_cp *cmd = (void *)cmdbuf;
    struct big_params bp;
    int rc;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (!IN_RANGE(cmd->big_handle, 0x00, 0xef) ||
        !IN_RANGE(cmd->adv_handle, 0x00, 0xef) ||
        !IN_RANGE(cmd->num_bis, 0x01, 0x1f) ||
        !IN_RANGE(get_le24(cmd->sdu_interval), 0x0000ff, 0x0fffff) ||
        !IN_RANGE(le16toh(cmd->max_sdu), 0x0001, 0x0fff) ||
        !IN_RANGE(le16toh(cmd->max_transport_latency), 0x0005, 0x0fa0) ||
        !IN_RANGE(cmd->rtn, 0x00, 0x1e) ||
        (cmd->packing > 1) || (cmd->framing > 1) || (cmd->encryption) > 1) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    bp.sdu_interval = get_le24(cmd->sdu_interval);
    bp.max_transport_latency = le16toh(cmd->max_transport_latency);
    bp.max_sdu = le16toh(cmd->max_sdu);
    if (cmd->phy & BLE_HCI_LE_PHY_2M_PREF_MASK) {
        bp.phy = BLE_PHY_2M;
    } else if (cmd->phy & BLE_HCI_LE_PHY_1M_PREF_MASK) {
        bp.phy = BLE_PHY_1M;
    } else {
        bp.phy = BLE_PHY_CODED;
    }
    bp.interleaved = cmd->packing;
    bp.framed = cmd->framing;
    bp.encrypted = cmd->encryption;
    memcpy(bp.broadcast_code, cmd->broadcast_code, 16);

    bp.nse = 1;
    bp.bn = 1;
    bp.irc = 1;
    bp.pto = 0;
    bp.iso_interval = bp.sdu_interval / 1250;
    bp.max_pdu = bp.max_sdu;

    rc = ble_ll_iso_big_create(cmd->big_handle, cmd->adv_handle, cmd->num_bis,
                               &bp);
    switch (rc) {
    case 0:
        break;
    case -EINVAL:
        return BLE_ERR_INV_HCI_CMD_PARMS;
    case -ENOMEM:
        return BLE_ERR_CONN_REJ_RESOURCES;
    case -ENOENT:
        return BLE_ERR_UNK_ADV_INDENT;
    default:
        return BLE_ERR_UNSPECIFIED;
    }

    return 0;
}

int
ble_ll_iso_big_hci_create_test(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_create_big_test_cp *cmd = (void *)cmdbuf;
    struct big_params bp;
    uint32_t iso_interval_us;
    int rc;

    if (len != sizeof(*cmd)) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (!IN_RANGE(cmd->big_handle, 0x00, 0xef) ||
        !IN_RANGE(cmd->adv_handle, 0x00, 0xef) ||
        !IN_RANGE(cmd->num_bis, 0x01, 0x1f) ||
        !IN_RANGE(get_le24(cmd->sdu_interval), 0x0000ff, 0x0fffff) ||
        !IN_RANGE(le16toh(cmd->iso_interval), 0x0004, 0x0c80) ||
        !IN_RANGE(cmd->nse, 0x01, 0x1f) ||
        !IN_RANGE(le16toh(cmd->max_sdu), 0x0001, 0x0fff) ||
        !IN_RANGE(le16toh(cmd->max_pdu), 0x0001, 0x00fb) ||
        /* phy */
        (cmd->packing > 1) || (cmd->framing > 1) ||
        !IN_RANGE(cmd->bn, 0x01, 0x07) ||
        !IN_RANGE(cmd->irc, 0x01, 0x0f) ||
        !IN_RANGE(cmd->pto, 0x00, 0x0f) ||
        (cmd->encryption) > 1) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    bp.nse = cmd->nse;
    bp.bn = cmd->bn;
    bp.irc = cmd->irc;
    bp.pto = cmd->pto;
    bp.sdu_interval = get_le24(cmd->sdu_interval);
    bp.iso_interval = le16toh(cmd->iso_interval);
    bp.max_sdu = le16toh(cmd->max_sdu);
    bp.max_pdu = le16toh(cmd->max_pdu);
    /* TODO verify phy */
    if (cmd->phy & BLE_HCI_LE_PHY_2M_PREF_MASK) {
        bp.phy = BLE_PHY_2M;
    } else if (cmd->phy & BLE_HCI_LE_PHY_1M_PREF_MASK) {
        bp.phy = BLE_PHY_1M;
    } else {
        bp.phy = BLE_PHY_CODED;
    }
    bp.interleaved = cmd->packing;
    bp.framed = cmd->framing;
    bp.encrypted = cmd->encryption;
    memcpy(bp.broadcast_code, cmd->broadcast_code, 16);

    if (bp.nse % bp.bn) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    iso_interval_us = bp.iso_interval * 1250;

    if (!bp.framed) {
        /* sdu_interval shall be an integer multiple of iso_interval */
        if (iso_interval_us % bp.sdu_interval) {
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }

        /* bn shall be an integer multiple of (iso_interval / sdu_interval) */
        if (bp.bn % (iso_interval_us / bp.sdu_interval)) {
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }
    }

    rc = ble_ll_iso_big_create(cmd->big_handle, cmd->adv_handle, cmd->num_bis,
                               &bp);
    switch (rc) {
    case 0:
        break;
    case -EINVAL:
        return BLE_ERR_INV_HCI_CMD_PARMS;
    case -ENOMEM:
        return BLE_ERR_CONN_REJ_RESOURCES;
    case -ENOENT:
        return BLE_ERR_UNK_ADV_INDENT;
    default:
        return BLE_ERR_UNSPECIFIED;
    }

    return 0;
}

int
ble_ll_iso_big_hci_terminate(const uint8_t *cmdbuf, uint8_t len)
{
    const struct ble_hci_le_terminate_big_cp *cmd = (void *)cmdbuf;
    int err;

    err = ble_ll_iso_big_terminate(cmd->big_handle, cmd->reason);
    switch (err) {
    case 0:
        break;
    case -ENOENT:
        return BLE_ERR_UNK_ADV_INDENT;
    default:
        return BLE_ERR_UNSPECIFIED;
    }

    return 0;
}

void
ble_ll_iso_big_init(void)
{
    struct ble_ll_iso_big *big;
    struct ble_ll_iso_bis *bis;
    uint8_t idx;

    memset(big_pool, 0, sizeof(big_pool));
    memset(bis_pool, 0, sizeof(bis_pool));

    for (idx = 0; idx < BIG_POOL_SIZE; idx++) {
        big = &big_pool[idx];

        big->handle = BIG_HANDLE_INVALID;

        big->sch.sched_type = BLE_LL_SCHED_TYPE_BIG;
        big->sch.sched_cb = ble_ll_iso_big_event_sched_cb;
        big->sch.cb_arg = big;

        ble_npl_event_init(&big->event_done, ble_ll_iso_big_event_done_ev, big);
    }

    for (idx = 0; idx < BIS_POOL_SIZE; idx++) {
        bis = &bis_pool[idx];
        bis->conn_handle = BLE_LL_CONN_HANDLE(BLE_LL_CONN_HANDLE_TYPE_BIS, idx);
    }

    big_pool_free = ARRAY_SIZE(big_pool);
    bis_pool_free = ARRAY_SIZE(bis_pool);
}

void
ble_ll_iso_big_reset(void)
{
    struct ble_ll_iso_big *big;
    int idx;

    for (idx = 0; idx < BIG_POOL_SIZE; idx++) {
        big = &big_pool[idx];
        ble_ll_iso_big_free(big);
    }

    ble_ll_iso_big_init();
}

#endif /* BLE_LL_ISO_BROADCASTER */
#endif /* ESP_PLATFORM */
