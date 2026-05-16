/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup HAL
 * @{
 *   @defgroup HALWatchdog HAL Watchdog
 *   @{
 */

#ifndef _HAL_WATCHDOG_H_
#define _HAL_WATCHDOG_H_

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set a recurring watchdog timer to fire no sooner than in 'expire_secs'
 * seconds. Watchdog should be tickled periodically with a frequency
 * smaller than 'expire_secs'. Watchdog needs to be then started with
 * a call to :c:func:`hal_watchdog_enable()`.
 *
 * @param expire_msecs		Watchdog timer expiration time in msecs
 *
 * @return			< 0 on failure; on success return the actual
 *                              expiration time as positive value
 */
int hal_watchdog_init(uint32_t expire_msecs);

/**
 * Starts the watchdog.
 */
void hal_watchdog_enable(void);

/**
 * Tickles the watchdog.   This needs to be done periodically, before
 * the value configured in :c:func:`hal_watchdog_init()` expires.
 */
void hal_watchdog_tickle(void);

#ifdef __cplusplus
}
#endif

#endif /* _HAL_WATCHDOG_H_ */

/**
 *   @} HALWatchdog
 * @} HAL
 */
