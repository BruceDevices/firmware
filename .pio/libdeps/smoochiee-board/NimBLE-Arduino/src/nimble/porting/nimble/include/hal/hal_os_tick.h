/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup HAL
 * @{
 *   @defgroup HALOsTick HAL OS Tick
 *   @{
 */

#ifndef H_HAL_OS_TICK_
#define H_HAL_OS_TICK_

#ifdef __cplusplus
extern "C" {
#endif

#include "nimble/porting/nimble/include/os/os.h"

typedef long os_time_t;

void hal_rtc_intr_init(void);

/**
 * Set up the periodic timer to interrupt at a frequency of 'os_ticks_per_sec'.
 * 'prio' is the cpu-specific priority of the periodic timer interrupt.
 *
 * @param os_ticks_per_sec Frequency of the OS tick timer
 * @param prio             Priority of the OS tick timer
 */
void bleonly_os_tick_init(uint32_t os_ticks_per_sec);

/**
 * Halt CPU for up to 'n' ticks.
 *
 * @param n The number of ticks to halt the CPU for
 */
void os_tick_idle(os_time_t n);


#ifdef __cplusplus
}
#endif

#endif /* H_HAL_OS_TICK_ */

/**
 *   @} HALOsTick
 * @} HAL
 */
