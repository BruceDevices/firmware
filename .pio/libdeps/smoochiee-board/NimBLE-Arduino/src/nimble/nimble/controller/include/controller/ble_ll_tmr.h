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

#ifndef H_BLE_LL_TMR_
#define H_BLE_LL_TMR_

#include "nimble/porting/nimble/include/os/os_cputime.h"
#include "nimble/nimble/controller/include/controller/ble_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USECS_PER_TICK      ((1000000 + MYNEWT_VAL(OS_CPUTIME_FREQ) - 1) / \
                             MYNEWT_VAL(OS_CPUTIME_FREQ))

#define LL_TMR_LT(_t1, _t2)     ((int32_t)((_t1) - (_t2)) < 0)
#define LL_TMR_GT(_t1, _t2)     ((int32_t)((_t1) - (_t2)) > 0)
#define LL_TMR_GEQ(_t1, _t2)    ((int32_t)((_t1) - (_t2)) >= 0)
#define LL_TMR_LEQ(_t1, _t2)    ((int32_t)((_t1) - (_t2)) <= 0)

typedef void (ble_ll_tmr_cb)(void *arg);

struct ble_ll_tmr {
    struct hal_timer t;
};

static inline uint32_t
ble_ll_tmr_get(void)
{
    return os_cputime_get32();
}

static inline uint32_t
ble_ll_tmr_t2u(uint32_t ticks)
{
#if MYNEWT_VAL(OS_CPUTIME_FREQ) == 31250
    return ticks * 32;
#endif

    return os_cputime_ticks_to_usecs(ticks);
}

static inline uint32_t
ble_ll_tmr_u2t(uint32_t usecs)
{
#if MYNEWT_VAL(OS_CPUTIME_FREQ) == 31250
    return usecs / 32;
#endif
#if MYNEWT_VAL(OS_CPUTIME_FREQ) == 32768
    if (usecs <= 31249) {
        return (usecs * 137439) / 4194304;
    }
#endif

    return os_cputime_usecs_to_ticks(usecs);
}

static inline uint32_t
ble_ll_tmr_u2t_up(uint32_t usecs)
{
    return ble_ll_tmr_u2t(usecs + (USECS_PER_TICK - 1));
}

static inline uint32_t
ble_ll_tmr_u2t_r(uint32_t usecs, uint8_t *rem_us)
{
    uint32_t ticks;

    ticks = ble_ll_tmr_u2t(usecs);
    *rem_us = usecs - ble_ll_tmr_t2u(ticks);
    if (*rem_us == USECS_PER_TICK) {
        *rem_us = 0;
        ticks++;
    }

    return ticks;
}

static inline void
ble_ll_tmr_add(uint32_t *ticks, uint8_t *rem_us, uint32_t usecs)
{
    uint32_t t_ticks;
    uint8_t t_rem_us;

    t_ticks = ble_ll_tmr_u2t_r(usecs, &t_rem_us);

    *ticks += t_ticks;
    *rem_us += t_rem_us;
    if (*rem_us >= USECS_PER_TICK) {
        *rem_us -= USECS_PER_TICK;
        *ticks += 1;
    }
}

static inline void
ble_ll_tmr_add_u(uint32_t *ticks, uint8_t *rem_us, uint8_t usecs)
{
    BLE_LL_ASSERT(usecs < USECS_PER_TICK);

    *rem_us += usecs;
    if (*rem_us >= USECS_PER_TICK) {
        *rem_us -= USECS_PER_TICK;
        *ticks += 1;
    }
}

static inline void
ble_ll_tmr_sub(uint32_t *ticks, uint8_t *rem_us, uint32_t usecs)
{
    uint32_t t_ticks;
    uint8_t t_rem_us;

    if (usecs <= *rem_us) {
        *rem_us -= usecs;
        return;
    }

    usecs -= *rem_us;
    *rem_us = 0;

    t_ticks = ble_ll_tmr_u2t_r(usecs, &t_rem_us);
    if (t_rem_us) {
        t_ticks += 1;
        *rem_us = USECS_PER_TICK - t_rem_us;
    }

    *ticks -= t_ticks;
}

static inline void
ble_ll_tmr_init(struct ble_ll_tmr *tmr, ble_ll_tmr_cb *cb, void *arg)
{
    os_cputime_timer_init(&tmr->t, cb, arg);
}

static inline void
ble_ll_tmr_start(struct ble_ll_tmr *tmr, uint32_t tgt)
{
    os_cputime_timer_start(&tmr->t, tgt);
}

static inline void
ble_ll_tmr_stop(struct ble_ll_tmr *tmr)
{
    os_cputime_timer_stop(&tmr->t);
}

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_LL_TMR_ */