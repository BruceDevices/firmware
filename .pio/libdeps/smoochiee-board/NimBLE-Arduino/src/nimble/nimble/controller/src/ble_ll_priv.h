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

#ifndef H_BLE_LL_PRIV_
#define H_BLE_LL_PRIV_

#ifdef __cplusplus
extern "C" {
#endif

extern int8_t g_ble_ll_tx_power;
extern int8_t g_ble_ll_tx_power_compensation;
extern int8_t g_ble_ll_rx_power_compensation;

int ble_ll_tx_power_round(int tx_power);
void ble_ll_tx_power_set(int tx_power);

static inline int
ble_ll_rx_gain(void)
{
    int gain = g_ble_ll_rx_power_compensation;

#if MYNEWT_VAL(BLE_FEM_LNA)
#if MYNEWT_VAL(BLE_FEM_LNA_GAIN_TUNABLE)
    gain += ble_fem_lna_rx_gain();
#else
    gain += MYNEWT_VAL(BLE_FEM_LNA_GAIN);
#endif
#endif

    return gain;
}

/* if there is any radio related activity enabled
 * (scanning, advertising, connection etc)
 */
#define BLE_LL_BUSY_EXCLUDE_CONNECTIONS 0x01

int ble_ll_is_busy(unsigned int flags);

#ifdef MYNEWT

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/porting/nimble/include/hal/hal_gpio.h"

#define BLE_LL_DEBUG_GPIO_INIT(_name)                                       \
    if (MYNEWT_VAL(BLE_LL_DEBUG_GPIO_ ## _name) >= 0) {                     \
        hal_gpio_init_out(MYNEWT_VAL(BLE_LL_DEBUG_GPIO_ ## _name), 0);      \
    }

#define BLE_LL_DEBUG_GPIO(_name, _val)                                      \
    if (MYNEWT_VAL(BLE_LL_DEBUG_GPIO_ ## _name) >= 0) {                     \
        hal_gpio_write(MYNEWT_VAL(BLE_LL_DEBUG_GPIO_ ## _name), !!(_val));  \
    }

#else
#define BLE_LL_DEBUG_GPIO_INIT(_name)       (void)(0)
#define BLE_LL_DEBUG_GPIO(_name, _val)      (void)(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_LL_PRIV_ */
