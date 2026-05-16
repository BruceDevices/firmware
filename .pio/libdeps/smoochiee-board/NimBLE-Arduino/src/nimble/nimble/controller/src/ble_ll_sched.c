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
#include <assert.h>
#include <string.h>
#include "nimble/nimble/drivers/nrf51/include/ble/xcvr.h"
#include "nimble/nimble/controller/include/controller/ble_phy.h"
#include "nimble/nimble/controller/include/controller/ble_ll.h"
#include "nimble/nimble/controller/include/controller/ble_ll_pdu.h"
#include "nimble/nimble/controller/include/controller/ble_ll_sched.h"
#include "nimble/nimble/controller/include/controller/ble_ll_adv.h"
#include "nimble/nimble/controller/include/controller/ble_ll_scan.h"
#include "nimble/nimble/controller/include/controller/ble_ll_scan_aux.h"
#include "nimble/nimble/controller/include/controller/ble_ll_rfmgmt.h"
#include "nimble/nimble/controller/include/controller/ble_ll_trace.h"
#include "nimble/nimble/controller/include/controller/ble_ll_tmr.h"
#include "nimble/nimble/controller/include/controller/ble_ll_sync.h"
#include "nimble/nimble/controller/include/controller/ble_ll_iso_big.h"
#if MYNEWT_VAL(BLE_LL_EXT)
#include "nimble/nimble/controller/include/controller/ble_ll_ext.h"
#endif
#include "ble_ll_priv.h"
#include "ble_ll_conn_priv.h"

#define BLE_LL_SCHED_MAX_DELAY_ANY      (0x7fffffff)

static struct ble_ll_tmr g_ble_ll_sched_timer;

uint8_t g_ble_ll_sched_offset_ticks;

#if (BLE_LL_SCHED_DEBUG == 1)
int32_t g_ble_ll_sched_max_late;
int32_t g_ble_ll_sched_max_early;
#endif

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
struct ble_ll_sched_css {
    uint8_t enabled;
#if !MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_FIXED)
    uint32_t slot_us;
    uint32_t period_slots;
#endif
    uint32_t period_anchor_ticks;
    uint8_t period_anchor_rem_us;
    uint8_t period_anchor_idx;
    uint16_t period_anchor_slot_idx;
};

static struct ble_ll_sched_css g_ble_ll_sched_css = {
#if !MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_FIXED)
    .slot_us = MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_SLOT_US),
    .period_slots = MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_PERIOD_SLOTS),
#endif
};
#endif

/* XXX: TODO:
 *  1) Add some accounting to the schedule code to see how late we are
 *  (min/max?)
 *
 *  2) Need to determine how we really want to handle the case when we execute
 *  a schedule item but there is a current event. We could:
 *      -> Reschedule the schedule item and let current event finish
 *      -> Kill the current event and run the scheduled item.
 *      -> Disable schedule timer while in an event; could cause us to be late.
 *      -> Wait for current event to finish hoping it does before schedule item.
 */

/* Queue for timers */
static TAILQ_HEAD(ll_sched_qhead, ble_ll_sched_item) g_ble_ll_sched_q;
static uint8_t g_ble_ll_sched_q_head_changed;

static int
preempt_any(struct ble_ll_sched_item *sch,
            struct ble_ll_sched_item *item)
{
    return 1;
}

static int
preempt_none(struct ble_ll_sched_item *sch,
             struct ble_ll_sched_item *item)
{
    return 0;
}

static int
preempt_any_except_conn(struct ble_ll_sched_item *sch,
                        struct ble_ll_sched_item *item)
{
    BLE_LL_ASSERT(sch->sched_type == BLE_LL_SCHED_TYPE_CONN);

    if (item->sched_type != BLE_LL_SCHED_TYPE_CONN) {
        return 1;
    }

    return ble_ll_conn_is_lru(sch->cb_arg, item->cb_arg);
}

static inline int
ble_ll_sched_check_overlap(struct ble_ll_sched_item *sch1,
                           struct ble_ll_sched_item *sch2)
{
    /* Note: item ranges are defined as [start, end) so items do not overlap
     *       if one item starts at the same time as another ends.
     */
    return LL_TMR_GT(sch1->end_time, sch2->start_time) &&
           LL_TMR_GT(sch2->end_time, sch1->start_time);
}

static void
ble_ll_sched_preempt(struct ble_ll_sched_item *sch,
                     struct ble_ll_sched_item *first)
{
    struct ble_ll_sched_item *entry;
    struct ble_ll_sched_item *next;
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    struct ble_ll_conn_sm *connsm;
#endif

    entry = first;

    do {
        next = TAILQ_NEXT(entry, link);

        TAILQ_REMOVE(&g_ble_ll_sched_q, entry, link);
        entry->enqueued = 0;

        switch (entry->sched_type) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL) || MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
            case BLE_LL_SCHED_TYPE_CONN:
                connsm = (struct ble_ll_conn_sm *)entry->cb_arg;
                ble_ll_event_add(&connsm->conn_ev_end);
                break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_BROADCASTER)
            case BLE_LL_SCHED_TYPE_ADV:
                ble_ll_adv_event_rmvd_from_sched(entry->cb_arg);
                break;
#endif
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV) && MYNEWT_VAL(BLE_LL_ROLE_OBSERVER)
            case BLE_LL_SCHED_TYPE_SCAN_AUX:
                ble_ll_scan_aux_break(entry->cb_arg);
                break;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV)
#if MYNEWT_VAL(BLE_LL_ROLE_BROADCASTER)
            case BLE_LL_SCHED_TYPE_PERIODIC:
                ble_ll_adv_periodic_rmvd_from_sched(entry->cb_arg);
                break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_OBSERVER)
            case BLE_LL_SCHED_TYPE_SYNC:
                ble_ll_sync_rmvd_from_sched(entry->cb_arg);
                break;
#endif
#endif
#endif
#if MYNEWT_VAL(BLE_LL_ISO_BROADCASTER)
        case BLE_LL_SCHED_TYPE_BIG:
            /* FIXME sometimes it may be useful to preempt... */
            BLE_LL_ASSERT(0);
            break;
#endif
#if MYNEWT_VAL(BLE_LL_EXT)
            case BLE_LL_SCHED_TYPE_EXTERNAL:
                ble_ll_ext_sched_removed(entry);
                break;
#endif
            default:
                BLE_LL_ASSERT(0);
                break;
        }

        entry = next;
    } while (entry != sch);
}

static inline void
ble_ll_sched_q_head_changed(void)
{
    if (g_ble_ll_sched_q_head_changed) {
        return;
    }

    g_ble_ll_sched_q_head_changed = 1;

    ble_ll_tmr_stop(&g_ble_ll_sched_timer);
}

void
ble_ll_sched_restart(void)
{
    struct ble_ll_sched_item *first;

    if (!g_ble_ll_sched_q_head_changed) {
        return;
    }

    g_ble_ll_sched_q_head_changed = 0;

    first = TAILQ_FIRST(&g_ble_ll_sched_q);

    ble_ll_rfmgmt_sched_changed(first);

    if (first) {
        ble_ll_tmr_start(&g_ble_ll_sched_timer, first->start_time);
    }
}

int
ble_ll_sched_insert(struct ble_ll_sched_item *sch, uint32_t max_delay,
                    ble_ll_sched_preempt_cb_t preempt_cb)
{
    struct ble_ll_sched_item *preempt_first;
    struct ble_ll_sched_item *first;
    struct ble_ll_sched_item *entry;
    uint32_t max_start_time;
    uint32_t duration;

    OS_ASSERT_CRITICAL();

    preempt_first = NULL;

    max_start_time = sch->start_time + max_delay;
    duration = sch->end_time - sch->start_time;

    first = TAILQ_FIRST(&g_ble_ll_sched_q);
    if (!first) {
        TAILQ_INSERT_HEAD(&g_ble_ll_sched_q, sch, link);
        sch->enqueued = 1;
        goto done;
    }

    TAILQ_FOREACH(entry, &g_ble_ll_sched_q, link) {
        if (LL_TMR_LEQ(sch->end_time, entry->start_time)) {
            TAILQ_INSERT_BEFORE(entry, sch, link);
            sch->enqueued = 1;
            goto done;
        }

        /* If current item overlaps our item check if we can preempt. If we
         * cannot preempt, move our item past current item and see if it's
         * still within allowed range.
         */

        if (ble_ll_sched_check_overlap(sch, entry)) {
            if (preempt_cb(sch, entry)) {
                if (!preempt_first) {
                    preempt_first = entry;
                }
            } else {
                preempt_first = NULL;
                /*
                 * For the 32768 Hz crystal in nrf chip, 1 tick is 30.517us.
                 * The connection state machine use anchor point to store the
                 * cpu ticks and anchor_point_usec to store the remainder.
                 * Therefore, to compensate the inaccuracy of the crystal, the
                 * ticks of anchor_point will be add with 1 once the value of
                 * anchor_point_usec exceed 31. If two connections have same
                 * connection interval, the time difference between the two
                 * start of schedule item will decreased 1, which lead to
                 * an overlap. To prevent this from happenning, we set the
                 * start_time of sch to 1 cpu tick after the end_time of entry.
                 */
                sch->start_time = entry->end_time + 1;

                if ((max_delay == 0) || LL_TMR_GEQ(sch->start_time,
                                                    max_start_time)) {
                    sch->enqueued = 0;
                    goto done;
                }

                sch->end_time = sch->start_time + duration;
            }
        }
    }

    if (!entry) {
        TAILQ_INSERT_TAIL(&g_ble_ll_sched_q, sch, link);
        sch->enqueued = 1;
    }

done:
    if (preempt_first) {
        BLE_LL_ASSERT(sch->enqueued);
        ble_ll_sched_preempt(sch, preempt_first);
    }

    /* Pause scheduler if inserted as 1st item, we do not want to miss this
     * one. Caller should restart outside critical section.
     */
    if (TAILQ_FIRST(&g_ble_ll_sched_q) == sch) {
        BLE_LL_ASSERT(sch->enqueued);
        ble_ll_sched_q_head_changed();
    }

    return sch->enqueued ? 0 : -1;
}

/*
 * Determines if the schedule item overlaps the currently running schedule
 * item. We only care about connection schedule items
 */
static int
ble_ll_sched_overlaps_current(struct ble_ll_sched_item *sch)
{
    int rc = 0;
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    uint32_t ce_end_time;

    if (ble_ll_state_get() == BLE_LL_STATE_CONNECTION) {
        ce_end_time = ble_ll_conn_get_ce_end_time();
        if (LL_TMR_GT(ce_end_time, sch->start_time)) {
            rc = 1;
        }
    }
#endif
    return rc;
}

int
ble_ll_sched_conn_reschedule(struct ble_ll_conn_sm *connsm)
{
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    struct ble_ll_sched_css *css = &g_ble_ll_sched_css;
#endif
    struct ble_ll_sched_item *sch;
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    uint32_t usecs;
#endif
    os_sr_t sr;
    int rc;

    /* Get schedule element from connection */
    sch = &connsm->conn_sch;

    /* Set schedule start and end times */
    sch->start_time = connsm->anchor_point - g_ble_ll_sched_offset_ticks;
    switch (connsm->conn_role) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL)
    case BLE_LL_CONN_ROLE_CENTRAL:
        sch->remainder = connsm->anchor_point_usecs;
        break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    case BLE_LL_CONN_ROLE_PERIPHERAL:
        usecs = connsm->periph_cur_window_widening;
        sch->start_time -= (ble_ll_tmr_u2t(usecs) + 1);
        sch->remainder = 0;
        break;
#endif
    default:
        BLE_LL_ASSERT(0);
        break;
    }

    sch->end_time = connsm->ce_end_time;

    /* Better be past current time or we just leave */
    if (LL_TMR_LT(sch->start_time, ble_ll_tmr_get())) {
        return -1;
    }

    OS_ENTER_CRITICAL(sr);

    if (ble_ll_sched_overlaps_current(sch)) {
        OS_EXIT_CRITICAL(sr);
        return -1;
    }

    rc = ble_ll_sched_insert(sch, 0, preempt_any_except_conn);
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    /* Store new anchor point for strict scheduling if successfully scheduled
     * reference connection.
     */
    if ((rc == 0) && (connsm == g_ble_ll_conn_css_ref)) {
        css->period_anchor_idx = connsm->css_period_idx;
        css->period_anchor_slot_idx = connsm->css_slot_idx;
        css->period_anchor_ticks = connsm->anchor_point;
        css->period_anchor_rem_us = connsm->anchor_point_usecs;
    }
#endif

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}

/**
 * Called to schedule a connection when the current role is central.
 *
 * Context: Interrupt
 *
 * @param connsm
 * @param ble_hdr
 * @param pyld_len
 *
 * @return int
 */
int
ble_ll_sched_conn_central_new(struct ble_ll_conn_sm *connsm,
                              struct ble_mbuf_hdr *ble_hdr, uint8_t pyld_len)
{
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    struct ble_ll_sched_css *css = &g_ble_ll_sched_css;
    uint8_t rem_us;
#endif
    struct ble_ll_sched_item *sch;
    uint32_t orig_start_time;
    uint32_t earliest_start = 0;
    uint32_t min_win_offset;
    uint32_t max_delay;
    uint32_t adv_rxend;
    bool calc_sch = true;
    os_sr_t sr;
    int rc;

    /* Get schedule element from connection */
    sch = &connsm->conn_sch;

    /* XXX:
     * The calculations for the 32kHz crystal bear alot of explanation. The
     * earliest possible time that the central can start the connection with a
     * peripheral is 1.25 msecs from the end of the connection request. The
     * connection request is sent an IFS time from the end of the advertising
     * packet that was received plus the time it takes to send the connection
     * request. At 1 Mbps, this is 1752 usecs, or 57.41 ticks. Using 57 ticks
     * makes us off ~13 usecs. Since we dont want to actually calculate the
     * receive end time tick (this would take too long), we assume the end of
     * the advertising PDU is 'now' (we call os_cputime_get32). We dont know
     * how much time it will take to service the ISR but if we are more than the
     * rx to tx time of the chip we will not be successful transmitting the
     * connect request. All this means is that we presume that the peripheral will
     * receive the connect request later than we expect but no earlier than
     * 13 usecs before (this is important).
     *
     * The code then attempts to schedule the connection at the
     * earliest time although this may not be possible. When the actual
     * schedule start time is determined, the central has to determine if this
     * time is more than a transmit window offset interval (1.25 msecs). The
     * central has to tell the peripheral how many transmit window offsets there are
     * from the earliest possible time to when the actual transmit start will
     * occur. Later in this function you will see the calculation. The actual
     * transmission start has to occur within the transmit window. The transmit
     * window interval is in units of 1.25 msecs and has to be at least 1. To
     * make things a bit easier (but less power efficient for the peripheral), we
     * use a transmit window of 2. We do this because we dont quite know the
     * exact start of the transmission and if we are too early or too late we
     * could miss the transmit window. A final note: the actual transmission
     * start (the anchor point) is sched offset ticks from the schedule start
     * time. We dont add this to the calculation when calculating the window
     * offset. The reason we dont do this is we want to insure we transmit
     * after the window offset we tell the peripheral. For example, say we think
     * we are transmitting 1253 usecs from the earliest start. This would cause
     * us to send a transmit window offset of 1. Since we are actually
     * transmitting earlier than the peripheral thinks we could end up transmitting
     * before the window offset. Transmitting later is fine since we have the
     * transmit window to do so. Transmitting before is bad, since the peripheral
     * wont be listening. We could do better calculation if we wanted to use
     * a transmit window of 1 as opposed to 2, but for now we dont care.
     */
    adv_rxend = ble_ll_tmr_get();
    if (ble_hdr->rxinfo.channel >= BLE_PHY_NUM_DATA_CHANS) {
        /*
         * We received packet on advertising channel which means this is a legacy
         * PDU on 1 Mbps - we do as described above.
         */
        earliest_start = adv_rxend + ble_ll_tmr_u2t(1752);
    } else {
        /*
         * The calculations are similar as above.
         *
         * We received packet on data channel which means this is AUX_ADV_IND
         * received on secondary adv channel. We can schedule first packet at
         * the earliest after "T_IFS + AUX_CONNECT_REQ + transmitWindowDelay".
         * AUX_CONNECT_REQ and transmitWindowDelay times vary depending on which
         * PHY we received on.
         *
         */
        if (ble_hdr->rxinfo.phy == BLE_PHY_1M) {
            /* 150 + 352 + 2500 = 3002us */
            earliest_start = adv_rxend + ble_ll_tmr_u2t(3002);
        } else if (ble_hdr->rxinfo.phy == BLE_PHY_2M) {
            /* 150 + 180 + 2500 = 2830us */
            earliest_start = adv_rxend + ble_ll_tmr_u2t(2830);
        } else if (ble_hdr->rxinfo.phy == BLE_PHY_CODED) {
            /* 150 + 2896 + 3750 = 6796us */
            earliest_start = adv_rxend + ble_ll_tmr_u2t(6796);
        } else {
            BLE_LL_ASSERT(0);
        }
    }

    orig_start_time = earliest_start - g_ble_ll_sched_offset_ticks;

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    if (ble_ll_sched_css_is_enabled()) {
        OS_ENTER_CRITICAL(sr);

        if (!g_ble_ll_conn_css_ref) {
            css->period_anchor_ticks = earliest_start;
            css->period_anchor_rem_us = 0;
            css->period_anchor_idx = 0;
            css->period_anchor_slot_idx = connsm->css_slot_idx;

            connsm->css_period_idx = 0;
            max_delay = connsm->conn_itvl_ticks;
        } else {
            /* Reference connection may be already at next period if it has
             * slot index lower than our, so we first try schedule one period
             * earlier since our slot index in that period may not yet have
             * passed. This avoids scheduling 1st connection event too far in
             * the future, i.e. more than conn interval.
             */
            if (connsm->css_slot_idx > css->period_anchor_slot_idx) {
                connsm->css_period_idx = css->period_anchor_idx - 1;
            } else {
                connsm->css_period_idx = css->period_anchor_idx;
            }
            max_delay = 0;
        }

        /* Calculate anchor point and move to next period if scheduled too
         * early.
         */
        connsm->css_period_idx--;
        do {
            connsm->css_period_idx++;
            ble_ll_sched_css_set_conn_anchor(connsm);
            sch->start_time =
                    connsm->anchor_point - g_ble_ll_sched_offset_ticks;
        } while (LL_TMR_LT(sch->start_time, orig_start_time));

        sch->end_time = connsm->anchor_point;
        sch->remainder = connsm->anchor_point_usecs;

        OS_EXIT_CRITICAL(sr);

        rem_us = sch->remainder;
        ble_ll_tmr_add(&sch->end_time, &rem_us, ble_ll_sched_css_get_slot_us());
        if (rem_us == 0) {
            sch->end_time--;
        }

        calc_sch = false;
    }
#endif

    if (calc_sch) {
        sch->start_time = earliest_start - g_ble_ll_sched_offset_ticks;
        sch->end_time = earliest_start +
                        ble_ll_tmr_u2t(MYNEWT_VAL(BLE_LL_CONN_INIT_SLOTS) *
                                       BLE_LL_SCHED_USECS_PER_SLOT);

        min_win_offset = ble_ll_tmr_u2t(
                MYNEWT_VAL(BLE_LL_CONN_INIT_MIN_WIN_OFFSET) *
                BLE_LL_SCHED_USECS_PER_SLOT);
        sch->start_time += min_win_offset;
        sch->end_time += min_win_offset;
        sch->remainder = 0;

        max_delay = connsm->conn_itvl_ticks - min_win_offset;
    }

    OS_ENTER_CRITICAL(sr);

    rc = ble_ll_sched_insert(sch, max_delay, preempt_none);
    if (rc == 0) {
        connsm->tx_win_off = ble_ll_tmr_t2u(sch->start_time - orig_start_time) /
                             BLE_LL_CONN_TX_OFF_USECS;

        connsm->anchor_point = sch->start_time + g_ble_ll_sched_offset_ticks;
        connsm->anchor_point_usecs = sch->remainder;
        connsm->ce_end_time = sch->end_time;
    }

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}

/**
 * Schedules a peripheral connection for the first time.
 *
 * Context: Link Layer
 *
 * @param connsm
 *
 * @return int
 */
int
ble_ll_sched_conn_periph_new(struct ble_ll_conn_sm *connsm)
{
    struct ble_ll_sched_item *sch;
    os_sr_t sr;
    int rc;

    /* Get schedule element from connection */
    sch = &connsm->conn_sch;

    /* Set schedule start and end times */
    /*
     * XXX: for now, we dont care about anchor point usecs for the peripheral. It
     * does not matter if we turn on the receiver up to one tick before w
     * need to. We also subtract one extra tick since the conversion from
     * usecs to ticks could be off by up to 1 tick.
     */
    sch->start_time = connsm->anchor_point - g_ble_ll_sched_offset_ticks -
                      ble_ll_tmr_u2t(connsm->periph_cur_window_widening) - 1;
    sch->end_time = connsm->ce_end_time;
    sch->remainder = 0;

    OS_ENTER_CRITICAL(sr);

    rc = ble_ll_sched_insert(sch, 0, preempt_any);

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV) && MYNEWT_VAL(BLE_LL_ROLE_OBSERVER)
/*
 * Determines if the schedule item overlaps the currently running schedule
 * item. This function cares about connection and sync.
 */
static int
ble_ll_sched_sync_overlaps_current(struct ble_ll_sched_item *sch)
{
    uint32_t end_time;
    uint8_t state;

    state = ble_ll_state_get();
    switch (state) {
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL) || MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    case BLE_LL_STATE_CONNECTION:
        end_time = ble_ll_conn_get_ce_end_time();
        break;
#endif
    case BLE_LL_STATE_SYNC:
        end_time = ble_ll_sync_get_event_end_time();
        break;
    default:
        return 0;
    }

    return LL_TMR_GT(end_time, sch->start_time);
}

int
ble_ll_sched_sync_reschedule(struct ble_ll_sched_item *sch, uint32_t ww_us)
{
    os_sr_t sr;
    int rc = 0;

    /* Adjust start time to include window widening */
    ble_ll_tmr_sub(&sch->start_time, &sch->remainder, ww_us);

    OS_ENTER_CRITICAL(sr);

    if (ble_ll_sched_sync_overlaps_current(sch)) {
        OS_EXIT_CRITICAL(sr);
        return -1;
    }

    rc = ble_ll_sched_insert(sch, 0, preempt_none);

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}

int
ble_ll_sched_sync(struct ble_ll_sched_item *sch)
{
    os_sr_t sr;
    int rc = 0;

    OS_ENTER_CRITICAL(sr);

    rc = ble_ll_sched_insert(sch, 0, preempt_none);

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}
#endif

int
ble_ll_sched_adv_new(struct ble_ll_sched_item *sch, ble_ll_sched_adv_new_cb cb,
                     void *arg)
{
    os_sr_t sr;
    int rc;

    OS_ENTER_CRITICAL(sr);

    rc = ble_ll_sched_insert(sch, BLE_LL_SCHED_MAX_DELAY_ANY,
                             preempt_none);
    BLE_LL_ASSERT(rc == 0);

    cb(sch->cb_arg, sch->start_time, arg);

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}

int
ble_ll_sched_periodic_adv(struct ble_ll_sched_item *sch, bool first_event)
{
    os_sr_t sr;
    int rc;

    OS_ENTER_CRITICAL(sr);

    if (first_event) {
        rc = ble_ll_sched_insert(sch, BLE_LL_SCHED_MAX_DELAY_ANY,
                                 preempt_none);
    } else {
        rc = ble_ll_sched_insert(sch, 0, preempt_any);
    }

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}

int
ble_ll_sched_adv_reschedule(struct ble_ll_sched_item *sch,
                            uint32_t max_delay_ticks)
{
    struct ble_ll_sched_item *next;
    uint32_t max_end_time;
    uint32_t rand_ticks;
    os_sr_t sr;
    int rc;

    max_end_time = sch->end_time + max_delay_ticks;

    OS_ENTER_CRITICAL(sr);

    /* Try to schedule as early as possible but no later than max allowed delay.
     * If succeeded, randomize start time to be within max allowed delay from
     * the original start time but make sure it ends before next scheduled item.
     */

    rc = ble_ll_sched_insert(sch, max_delay_ticks, preempt_none);
    if (rc == 0) {
        next = TAILQ_NEXT(sch, link);
        if (next) {
            if (LL_TMR_LT(next->start_time, max_end_time)) {
                max_end_time = next->start_time;
            }
            rand_ticks = max_end_time - sch->end_time;
        } else {
            rand_ticks = max_delay_ticks;
        }

        if (rand_ticks) {
            rand_ticks = ble_ll_rand() % rand_ticks;
        }

        sch->start_time += rand_ticks;
        sch->end_time += rand_ticks;
    }

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}

int
ble_ll_sched_adv_resched_pdu(struct ble_ll_sched_item *sch)
{
    uint8_t lls;
    os_sr_t sr;
    int rc;

    OS_ENTER_CRITICAL(sr);

    lls = ble_ll_state_get();
    switch(lls) {
#if MYNEWT_VAL(BLE_LL_ROLE_BROADCASTER)
    case BLE_LL_STATE_ADV:
        OS_EXIT_CRITICAL(sr);
        return -1;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL) || MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    case BLE_LL_STATE_CONNECTION:
        OS_EXIT_CRITICAL(sr);
        return -1;
#endif
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV) && MYNEWT_VAL(BLE_LL_ROLE_OBSERVER)
    case BLE_LL_STATE_SYNC:
        OS_EXIT_CRITICAL(sr);
        return -1;
#endif
    default:
        break;
    }

    rc = ble_ll_sched_insert(sch, 0, preempt_none);

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}

#if MYNEWT_VAL(BLE_LL_ISO_BROADCASTER)
int
ble_ll_sched_iso_big(struct ble_ll_sched_item *sch, int first)
{
    os_sr_t sr;
    int rc;

    OS_ENTER_CRITICAL(sr);

    if (first) {
        rc = ble_ll_sched_insert(sch, BLE_LL_SCHED_MAX_DELAY_ANY, preempt_none);
    } else {
        /* XXX provide better strategy for preemption */
        rc = ble_ll_sched_insert(sch, 0, preempt_any);
    }

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}
#endif /* BLE_LL_ISO_BROADCASTER */

/**
 * Remove a schedule element
 *
 * @param sched_type
 *
 * @return int 0 - removed, 1 - not in the list
 */
int
ble_ll_sched_rmv_elem(struct ble_ll_sched_item *sch)
{
    uint8_t first_removed;
    os_sr_t sr;
    int rc;

    BLE_LL_ASSERT(sch);

    OS_ENTER_CRITICAL(sr);

    first_removed = 0;

    if (sch->enqueued) {
        if (sch == TAILQ_FIRST(&g_ble_ll_sched_q)) {
            first_removed = 1;
        }

        TAILQ_REMOVE(&g_ble_ll_sched_q, sch, link);
        sch->enqueued = 0;

        rc = 0;
    } else {
        rc = 1;
    }

    if (first_removed) {
        ble_ll_sched_q_head_changed();
    }

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}

void
ble_ll_sched_rmv_elem_type(uint8_t type, sched_remove_cb_func remove_cb)
{
    struct ble_ll_sched_item *first;
    struct ble_ll_sched_item *entry;
    uint8_t first_removed;
    os_sr_t sr;

    OS_ENTER_CRITICAL(sr);

    first = TAILQ_FIRST(&g_ble_ll_sched_q);
    if (first->sched_type == type) {
        first_removed = 1;
    }

    TAILQ_FOREACH(entry, &g_ble_ll_sched_q, link) {
        if (entry->sched_type != type) {
            continue;
        }
        TAILQ_REMOVE(&g_ble_ll_sched_q, entry, link);
        remove_cb(entry);
        entry->enqueued = 0;
    }

    if (first_removed) {
        ble_ll_sched_q_head_changed();
    }

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();
}

/**
 * Executes a schedule item by calling the schedule callback function.
 *
 * Context: Interrupt
 *
 * @param sch Pointer to schedule item
 *
 * @return int 0: schedule item is not over; otherwise schedule item is done.
 */
static int
ble_ll_sched_execute_item(struct ble_ll_sched_item *sch)
{
    int rc;
    uint8_t lls;

    lls = ble_ll_state_get();

    ble_ll_trace_u32x3(BLE_LL_TRACE_ID_SCHED, lls, ble_ll_tmr_get(),
                       sch->start_time);

    if (lls == BLE_LL_STATE_STANDBY) {
        goto sched;
    }

    /*
     * This is either an advertising event or connection event start. If
     * we are scanning or initiating just stop it.
     */

    /* We have to disable the PHY no matter what */
    ble_phy_disable();

    switch (lls) {
#if MYNEWT_VAL(BLE_LL_ROLE_OBSERVER)
    case BLE_LL_STATE_SCANNING:
        ble_ll_state_set(BLE_LL_STATE_STANDBY);
        ble_ll_scan_halt();
        break;
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_PERIODIC_ADV)
    case BLE_LL_STATE_SYNC:
        STATS_INC(ble_ll_stats, sched_state_sync_errs);
        ble_ll_sync_halt();
        break;
#endif
#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
    case BLE_LL_STATE_SCAN_AUX:
        ble_ll_state_set(BLE_LL_STATE_STANDBY);
        ble_ll_scan_aux_halt();
        break;
#endif
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_BROADCASTER)
    case BLE_LL_STATE_ADV:
        STATS_INC(ble_ll_stats, sched_state_adv_errs);
        ble_ll_adv_halt();
        break;
#endif
#if MYNEWT_VAL(BLE_LL_ROLE_CENTRAL) || MYNEWT_VAL(BLE_LL_ROLE_PERIPHERAL)
    case BLE_LL_STATE_CONNECTION:
        STATS_INC(ble_ll_stats, sched_state_conn_errs);
        ble_ll_conn_event_halt();
        break;
#endif
#if MYNEWT_VAL(BLE_LL_ISO_BROADCASTER)
    case BLE_LL_STATE_BIG:
        ble_ll_iso_big_halt();
        break;
#endif
#if MYNEWT_VAL(BLE_LL_EXT)
    case BLE_LL_STATE_EXTERNAL:
        ble_ll_ext_halt();
        break;
#endif
    default:
        BLE_LL_ASSERT(0);
        break;
    }

sched:
    BLE_LL_ASSERT(sch->sched_cb);

    BLE_LL_DEBUG_GPIO(SCHED_ITEM, 1);
    rc = sch->sched_cb(sch);
    if (rc != BLE_LL_SCHED_STATE_RUNNING) {
        BLE_LL_DEBUG_GPIO(SCHED_ITEM, 0);
    }

    return rc;
}

/**
 * Run the BLE scheduler. Iterate through all items on the schedule queue.
 *
 * Context: interrupt (scheduler)
 *
 * @return int
 */
static void
ble_ll_sched_run(void *arg)
{
    struct ble_ll_sched_item *sch;

    BLE_LL_DEBUG_GPIO(SCHED_RUN, 1);

    /* Look through schedule queue */
    sch = TAILQ_FIRST(&g_ble_ll_sched_q);
    if (sch) {
#if (BLE_LL_SCHED_DEBUG == 1)
        int32_t dt;

        /* Make sure we have passed the start time of the first event */
        dt = (int32_t)(ble_ll_tmr_get() - sch->start_time);
        if (dt > g_ble_ll_sched_max_late) {
            g_ble_ll_sched_max_late = dt;
        }
        if (dt < g_ble_ll_sched_max_early) {
            g_ble_ll_sched_max_early = dt;
        }
#endif

        /* Remove schedule item and execute the callback */
        TAILQ_REMOVE(&g_ble_ll_sched_q, sch, link);
        sch->enqueued = 0;
        g_ble_ll_sched_q_head_changed = 1;

        ble_ll_sched_execute_item(sch);

        ble_ll_sched_restart();
    }

    BLE_LL_DEBUG_GPIO(SCHED_RUN, 0);
}

/**
 * Called to determine when the next scheduled event will occur.
 *
 * If there are not scheduled events this function returns 0; otherwise it
 * returns 1 and *next_event_time is set to the start time of the next event.
 *
 * @param next_event_time
 *
 * @return int 0: No events are scheduled 1: there is an upcoming event
 */
int
ble_ll_sched_next_time(uint32_t *next_event_time)
{
    int rc;
    os_sr_t sr;
    struct ble_ll_sched_item *first;

    rc = 0;
    OS_ENTER_CRITICAL(sr);
    first = TAILQ_FIRST(&g_ble_ll_sched_q);
    if (first) {
        *next_event_time = first->start_time;
        rc = 1;
    }
    OS_EXIT_CRITICAL(sr);

    return rc;
}

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
int
ble_ll_sched_scan_aux(struct ble_ll_sched_item *sch)
{
    os_sr_t sr;
    int rc;

    OS_ENTER_CRITICAL(sr);

    rc = ble_ll_sched_insert(sch, 0, preempt_none);

    OS_EXIT_CRITICAL(sr);

    ble_ll_sched_restart();

    return rc;
}
#endif

#if MYNEWT_VAL(BLE_LL_DTM)
int ble_ll_sched_dtm(struct ble_ll_sched_item *sch)
{
    os_sr_t sr;
    int rc;

    OS_ENTER_CRITICAL(sr);

    rc = ble_ll_sched_insert(sch, 0, preempt_any);

    OS_EXIT_CRITICAL(sr);

    if (rc == 0) {
        ble_ll_sched_restart();
    }

    return rc;
}
#endif
/**
 * Stop the scheduler
 *
 * Context: Link Layer task
 */
void
ble_ll_sched_stop(void)
{
    ble_ll_tmr_stop(&g_ble_ll_sched_timer);
}

/**
 * Initialize the scheduler. Should only be called once and should be called
 * before any of the scheduler API are called.
 *
 * @return int
 */
int
ble_ll_sched_init(void)
{
    BLE_LL_DEBUG_GPIO_INIT(SCHED_ITEM);
    BLE_LL_DEBUG_GPIO_INIT(SCHED_RUN);

    /*
     * Initialize max early to large negative number. This is used
     * to determine the worst-case "early" time the schedule was called. Dont
     * expect this to be less than -3 or -4.
     */
#if (BLE_LL_SCHED_DEBUG == 1)
    g_ble_ll_sched_max_early = -50000;
#endif

    /*
     * This is the offset from the start of the scheduled item until the actual
     * tx/rx should occur, in ticks. We also "round up" to the nearest tick.
     */
    g_ble_ll_sched_offset_ticks = ble_ll_tmr_u2t_up(XCVR_TX_SCHED_DELAY_USECS);

    /* Initialize cputimer for the scheduler */
    ble_ll_tmr_init(&g_ble_ll_sched_timer, ble_ll_sched_run, NULL);

    g_ble_ll_sched_q_head_changed = 0;

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
    memset(&g_ble_ll_sched_css, 0, sizeof (g_ble_ll_sched_css));
#if !MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_FIXED)
    g_ble_ll_sched_css.slot_us = MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_SLOT_US);
    g_ble_ll_sched_css.period_slots = MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_PERIOD_SLOTS);
#endif
#if !MYNEWT_VAL(BLE_LL_HCI_VS_CONN_STRICT_SCHED)
    g_ble_ll_sched_css.enabled = 1;
#endif
#endif

    return 0;
}

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
#if !MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_FIXED)
void
ble_ll_sched_css_set_params(uint32_t slot_us, uint32_t period_slots)
{
    g_ble_ll_sched_css.slot_us = slot_us;
    g_ble_ll_sched_css.period_slots = period_slots;
}
#endif

void
ble_ll_sched_css_set_enabled(uint8_t enabled)
{
    g_ble_ll_sched_css.enabled = enabled;
}

void
ble_ll_sched_css_update_anchor(struct ble_ll_conn_sm *connsm)
{
    struct ble_ll_sched_css *css = &g_ble_ll_sched_css;

    if (!g_ble_ll_conn_css_ref) {
        g_ble_ll_conn_css_ref = connsm;
        css->period_anchor_ticks = connsm->anchor_point;
        css->period_anchor_rem_us = connsm->anchor_point_usecs;
    }
}

void
ble_ll_sched_css_set_conn_anchor(struct ble_ll_conn_sm *connsm)
{
    struct ble_ll_sched_css *css = &g_ble_ll_sched_css;
    int8_t period_diff;
    int16_t slot_diff;
    int32_t diff;

    period_diff = connsm->css_period_idx - css->period_anchor_idx;
    slot_diff = connsm->css_slot_idx - css->period_anchor_slot_idx;

    diff = (period_diff * ble_ll_sched_css_get_period_slots() + slot_diff) *
           ble_ll_sched_css_get_slot_us();

    connsm->anchor_point = css->period_anchor_ticks;
    connsm->anchor_point_usecs = css->period_anchor_rem_us;

    if (diff < 0) {
        ble_ll_tmr_sub(&connsm->anchor_point, &connsm->anchor_point_usecs,
                       -diff);
    } else if (diff > 0) {
        ble_ll_tmr_add(&connsm->anchor_point, &connsm->anchor_point_usecs,
                       diff);
    }
}

#if !MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_FIXED)
inline bool
ble_ll_sched_css_is_enabled(void)
{
    return g_ble_ll_sched_css.enabled;
}

inline uint32_t
ble_ll_sched_css_get_slot_us(void)
{
    return g_ble_ll_sched_css.slot_us;
}

inline uint32_t
ble_ll_sched_css_get_period_slots(void)
{
    return g_ble_ll_sched_css.period_slots;
}

inline uint32_t
ble_ll_sched_css_get_conn_interval_us(void)
{
    return ble_ll_sched_css_get_period_slots() *
           ble_ll_sched_css_get_slot_us() /
           BLE_LL_CONN_ITVL_USECS;
}
#endif

#endif
#endif
