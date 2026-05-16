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

#ifndef H_PHY_PRIV_
#define H_PHY_PRIV_

#include <stdint.h>
#include <nrf_gpio.h>
#include <nrf_gpiote.h>


#if defined(NRF52840_XXAA) && MYNEWT_VAL(BLE_PHY_NRF52_HEADERMASK_WORKAROUND)
#define PHY_USE_HEADERMASK_WORKAROUND 1
#endif

#define PHY_USE_DEBUG_1     (MYNEWT_VAL(BLE_PHY_DBG_TIME_TXRXEN_READY_PIN) >= 0)
#define PHY_USE_DEBUG_2     (MYNEWT_VAL(BLE_PHY_DBG_TIME_ADDRESS_END_PIN) >= 0)
#define PHY_USE_DEBUG_3     (MYNEWT_VAL(BLE_PHY_DBG_TIME_WFR_PIN) >= 0)
#define PHY_USE_DEBUG       (PHY_USE_DEBUG_1 || PHY_USE_DEBUG_2 || PHY_USE_DEBUG_3)

#define PHY_USE_FEM_PA      (MYNEWT_VAL(BLE_FEM_PA) != 0)
#define PHY_USE_FEM_LNA     (MYNEWT_VAL(BLE_FEM_LNA) != 0)
#define PHY_USE_FEM         (PHY_USE_FEM_PA || PHY_USE_FEM_LNA)
#define PHY_USE_FEM_SINGLE_GPIO \
    (PHY_USE_FEM && (!PHY_USE_FEM_PA || !PHY_USE_FEM_LNA || \
                     (MYNEWT_VAL(BLE_FEM_PA_GPIO) == \
                      MYNEWT_VAL(BLE_FEM_LNA_GPIO))))

/* GPIOTE indexes, start assigning from last one */
#define PHY_GPIOTE_DEBUG_1  (8 - PHY_USE_DEBUG_1)
#define PHY_GPIOTE_DEBUG_2  (PHY_GPIOTE_DEBUG_1 - PHY_USE_DEBUG_2)
#define PHY_GPIOTE_DEBUG_3  (PHY_GPIOTE_DEBUG_2 - PHY_USE_DEBUG_3)
#if PHY_USE_FEM_SINGLE_GPIO
#define PHY_GPIOTE_FEM      (PHY_GPIOTE_DEBUG_3 - PHY_USE_FEM)
#else
#define PHY_GPIOTE_FEM_PA   (PHY_GPIOTE_DEBUG_3 - PHY_USE_FEM_PA)
#define PHY_GPIOTE_FEM_LNA  (PHY_GPIOTE_FEM_PA - PHY_USE_FEM_LNA)
#endif

static inline void
phy_gpiote_configure(int idx, int pin)
{
    nrf_gpio_cfg_output(pin);
    nrf_gpiote_task_configure(NRF_GPIOTE, idx, pin, GPIOTE_CONFIG_POLARITY_None/*NRF_GPIOTE_POLARITY_NONE*/,
                              NRF_GPIOTE_INITIAL_VALUE_LOW);
    nrf_gpiote_task_enable(NRF_GPIOTE, idx);
}

#if PHY_USE_DEBUG
void phy_debug_init(void);
#endif

#if PHY_USE_FEM
void phy_fem_init(void);
#if PHY_USE_FEM_PA
void phy_fem_enable_pa(void);
#endif
#if PHY_USE_FEM_LNA
void phy_fem_enable_lna(void);
#endif
void phy_fem_disable(void);
#endif

void phy_ppi_init(void);

#ifdef NRF52_SERIES
#include "nimble/nimble/drivers/nrf5x/src/nrf52/phy_ppi.h"
#endif

void phy_txpower_set(int8_t dbm);
int8_t phy_txpower_round(int8_t dbm);

#ifdef NRF52_SERIES
#include "nimble/nimble/drivers/nrf5x/src/nrf52/phy_ppi.h"
#endif
#ifdef NRF53_SERIES
#include "nimble/nimble/drivers/nrf5x/src/nrf52/phy_ppi.h"
#endif

#endif /* H_PHY_PRIV_ */
