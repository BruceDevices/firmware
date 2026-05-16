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

#ifndef H_BLE_LL_SCHED_
#define H_BLE_LL_SCHED_

#ifdef __cplusplus
extern "C" {
#endif

/* Time per BLE scheduler slot */
#define BLE_LL_SCHED_USECS_PER_SLOT   (1250)

/*
 * Worst case time needed for scheduled advertising item. This is the longest
 * possible time to receive a scan request and send a scan response (with the
 * appropriate IFS time between them). This number is calculated using the
 * following formula: IFS + SCAN_REQ + IFS + SCAN_RSP = 150 + 176 + 150 + 376.
 * Note: worst case time to tx adv, rx scan req and send scan rsp is 1228 usecs.
 * This assumes maximum sized advertising PDU and scan response PDU.
 *
 * For connectable advertising events no scan request is allowed. In this case
 * we just need to receive a connect request PDU: IFS + CONNECT_REQ = 150 + 352.
 * Note: worst-case is 376 + 150 + 352 = 878 usecs
 *
 * NOTE: The advertising PDU transmit time is NOT included here since we know
 * how long that will take (worst-case is 376 usecs).
 */
#define BLE_LL_SCHED_ADV_MAX_USECS          (852)
#define BLE_LL_SCHED_DIRECT_ADV_MAX_USECS   (502)
#define BLE_LL_SCHED_MAX_ADV_PDU_USECS      (376)

/*
 * This is the offset from the start of the scheduled item until the actual
 * tx/rx should occur, in ticks.
 */
extern uint8_t g_ble_ll_sched_offset_ticks;

/*
 * This is the number of slots needed to transmit and receive a maximum
 * size PDU, including an IFS time before each. The actual time is
 * 2120 usecs for tx/rx and 150 for IFS = 4540 usecs.
 */
#define BLE_LL_SCHED_MAX_TXRX_SLOT  (4 * BLE_LL_SCHED_USECS_PER_SLOT)

/* BLE scheduler errors */
#define BLE_LL_SCHED_ERR_OVERLAP    (1)

/* Types of scheduler events */
#define BLE_LL_SCHED_TYPE_ADV       (1)
#define BLE_LL_SCHED_TYPE_SCAN      (2)
#define BLE_LL_SCHED_TYPE_CONN      (3)
#define BLE_LL_SCHED_TYPE_DTM       (5)
#define BLE_LL_SCHED_TYPE_PERIODIC  (6)
#define BLE_LL_SCHED_TYPE_SYNC      (7)
#define BLE_LL_SCHED_TYPE_SCAN_AUX  (8)
#define BLE_LL_SCHED_TYPE_BIG       (9)
#if MYNEWT_VAL(BLE_LL_EXT)
#define BLE_LL_SCHED_TYPE_EXTERNAL  (255)
#endif

/* Return values for schedule callback. */
#define BLE_LL_SCHED_STATE_RUNNING  (0)
#define BLE_LL_SCHED_STATE_DONE     (1)

/* Callback function */
struct ble_ll_sched_item;
typedef int (*sched_cb_func)(struct ble_ll_sched_item *sch);
typedef void (*sched_remove_cb_func)(struct ble_ll_sched_item *sch);

typedef int (* ble_ll_sched_preempt_cb_t)(struct ble_ll_sched_item *sch,
                                          struct ble_ll_sched_item *item);


/*
 * Schedule item
 *  sched_type: This is the type of the schedule item.
 *  enqueued: Flag denoting if item is on the scheduler list. 0: no, 1:yes
 *  remainder: # of usecs from offset till tx/rx should occur
 *  txrx_offset: Number of ticks from start time until tx/rx should occur.
 *
 */
struct ble_ll_sched_item
{
    uint8_t         sched_type;
#if MYNEWT_VAL(BLE_LL_EXT)
    uint8_t         sched_ext_type;
#endif
    uint8_t         enqueued;
    uint8_t         remainder;
    uint32_t        start_time;
    uint32_t        end_time;
    void            *cb_arg;
    sched_cb_func   sched_cb;
    TAILQ_ENTRY(ble_ll_sched_item) link;
};

/* Initialize the scheduler */
int ble_ll_sched_init(void);

int ble_ll_sched_insert(struct ble_ll_sched_item *sch, uint32_t max_delay,
                        ble_ll_sched_preempt_cb_t preempt_cb);
void ble_ll_sched_restart(void);

/* Remove item(s) from schedule */
int ble_ll_sched_rmv_elem(struct ble_ll_sched_item *sch);

void ble_ll_sched_rmv_elem_type(uint8_t type, sched_remove_cb_func remove_cb);

/* Schedule a new master connection */
struct ble_ll_conn_sm;
int ble_ll_sched_conn_central_new(struct ble_ll_conn_sm *connsm,
                                  struct ble_mbuf_hdr *ble_hdr, uint8_t pyld_len);

/* Schedule a new slave connection */
int ble_ll_sched_conn_periph_new(struct ble_ll_conn_sm *connsm);

struct ble_ll_adv_sm;
typedef void ble_ll_sched_adv_new_cb(struct ble_ll_adv_sm *advsm,
                                     uint32_t sch_start, void *arg);

/* Schedule a new advertising event */
int ble_ll_sched_adv_new(struct ble_ll_sched_item *sch,
                         ble_ll_sched_adv_new_cb cb, void *arg);

/* Schedule periodic advertising event */
int ble_ll_sched_periodic_adv(struct ble_ll_sched_item *sch, bool first_event);

int ble_ll_sched_sync_reschedule(struct ble_ll_sched_item *sch, uint32_t ww_us);
int ble_ll_sched_sync(struct ble_ll_sched_item *sch);

/* Reschedule an advertising event */
int ble_ll_sched_adv_reschedule(struct ble_ll_sched_item *sch,
                                uint32_t max_delay_ticks);

/* Reschedule and advertising pdu */
int ble_ll_sched_adv_resched_pdu(struct ble_ll_sched_item *sch);

/* Reschedule a connection that had previously been scheduled or that is over */
int ble_ll_sched_conn_reschedule(struct ble_ll_conn_sm * connsm);

/**
 * Called to determine when the next scheduled event will occur.
 *
 * If there are not scheduled events this function returns 0; otherwise it
 * returns 1 and *next_event_time is set to the start time of the next event.
 *
 * @param next_event_time cputime at which next scheduled event will occur
 *
 * @return int 0: No events are scheduled 1: there is an upcoming event
 */
int ble_ll_sched_next_time(uint32_t *next_event_time);

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)
int ble_ll_sched_scan_aux(struct ble_ll_sched_item *sch);
#endif

/* Stop the scheduler */
void ble_ll_sched_stop(void);

#if MYNEWT_VAL(BLE_LL_DTM)
int ble_ll_sched_dtm(struct ble_ll_sched_item *sch);
#endif

#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED)
#if !MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_FIXED)
void ble_ll_sched_css_set_params(uint32_t slot_us, uint32_t period_slots);
#endif
void ble_ll_sched_css_set_enabled(uint8_t enabled);
void ble_ll_sched_css_update_anchor(struct ble_ll_conn_sm *connsm);
void ble_ll_sched_css_set_conn_anchor(struct ble_ll_conn_sm *connsm);
#if MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_FIXED)
static inline bool
ble_ll_sched_css_is_enabled(void)
{
    return true;
}

static inline uint32_t
ble_ll_sched_css_get_slot_us(void)
{
    return MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_SLOT_US);
}

static inline uint32_t
ble_ll_sched_css_get_period_slots(void)
{
    return MYNEWT_VAL(BLE_LL_CONN_STRICT_SCHED_PERIOD_SLOTS);
}

static inline uint32_t
ble_ll_sched_css_get_conn_interval_us(void)
{
    return ble_ll_sched_css_get_period_slots() *
           ble_ll_sched_css_get_slot_us() / 1250;
}
#else
bool ble_ll_sched_css_is_enabled(void);
uint32_t ble_ll_sched_css_get_slot_us(void);
uint32_t ble_ll_sched_css_get_period_slots(void);
uint32_t ble_ll_sched_css_get_conn_interval_us(void);
#endif
#endif

#if MYNEWT_VAL(BLE_LL_ISO_BROADCASTER)
int ble_ll_sched_iso_big(struct ble_ll_sched_item *sch, int first);
#endif /* BLE_LL_ISO_BROADCASTER */

#ifdef __cplusplus
}
#endif

#endif /* H_LL_SCHED_ */
