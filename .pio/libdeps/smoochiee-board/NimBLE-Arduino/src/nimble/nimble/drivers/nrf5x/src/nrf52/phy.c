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

#if defined(ARDUINO_ARCH_NRF5) && defined(NRF52_SERIES)

#include <stdint.h>
#include <nrfx.h>
#include <nimble/nimble/controller/include/controller/ble_fem.h>
#include "nimble/nimble/drivers/nrf5x/src/phy_priv.h"

#if PHY_USE_DEBUG
void
phy_debug_init(void)
{
#if PHY_USE_DEBUG_1
    phy_gpiote_configure(PHY_GPIOTE_DEBUG_1,
                         MYNEWT_VAL(BLE_PHY_DBG_TIME_TXRXEN_READY_PIN));

    nrf_ppi_channel_endpoint_setup(NRF_PPI, 17,
                                   (uint32_t)&(NRF_RADIO->EVENTS_READY),
                                   (uint32_t)&(NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_DEBUG_1]));
    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH17_Msk);

    /* CH[20] and PPI CH[21] are on to trigger TASKS_TXEN or TASKS_RXEN */
    nrf_ppi_fork_endpoint_setup(NRF_PPI, 20,
                                (uint32_t)&(NRF_GPIOTE->TASKS_SET[PHY_GPIOTE_DEBUG_1]));
    nrf_ppi_fork_endpoint_setup(NRF_PPI, 21,
                                (uint32_t)&(NRF_GPIOTE->TASKS_SET[PHY_GPIOTE_DEBUG_1]));
#endif

#if PHY_USE_DEBUG_2
    phy_gpiote_configure(PHY_GPIOTE_DEBUG_2,
                         MYNEWT_VAL(BLE_PHY_DBG_TIME_ADDRESS_END_PIN));

    /* CH[26] and CH[27] are always on for EVENT_ADDRESS and EVENT_END */
    nrf_ppi_fork_endpoint_setup(NRF_PPI, 26,
                                (uint32_t)&(NRF_GPIOTE->TASKS_SET[PHY_GPIOTE_DEBUG_2]));
    nrf_ppi_fork_endpoint_setup(NRF_PPI, 27,
                                (uint32_t)&(NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_DEBUG_2]));
#endif

#if PHY_USE_DEBUG_3
    phy_gpiote_configure(PHY_GPIOTE_DEBUG_3, MYNEWT_VAL(BLE_PHY_DBG_TIME_WFR_PIN));

#if NRF52840_XXAA
    nrf_ppi_channel_endpoint_setup(NRF_PPI, 18,
                                   (uint32_t)&(NRF_RADIO->EVENTS_RXREADY),
                                   (uint32_t)&(NRF_GPIOTE->TASKS_SET[PHY_GPIOTE_DEBUG_3]));
#else
    nrf_ppi_channel_endpoint_setup(NRF_PPI, 18,
                                   (uint32_t)&(NRF_RADIO->EVENTS_READY),
                                   (uint32_t)&(NRF_GPIOTE->TASKS_SET[GIDX_DEBUG_3]));
#endif
    nrf_ppi_channel_endpoint_setup(NRF_PPI, 19,
                                   (uint32_t)&(NRF_RADIO->EVENTS_DISABLED),
                                   (uint32_t)&(NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_DEBUG_3]));
    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH18_Msk | PPI_CHEN_CH19_Msk);

    /* CH[4] and CH[5] are always on for wfr */
    nrf_ppi_fork_endpoint_setup(NRF_PPI, 4,
                                (uint32_t)&(NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_DEBUG_3]));
    nrf_ppi_fork_endpoint_setup(NRF_PPI, 5,
                                (uint32_t)&(NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_DEBUG_3]));
#endif
}
#endif /* PHY_USE_DEBUG */

#if PHY_USE_FEM
void
phy_fem_init(void)
{
#if PHY_USE_FEM_SINGLE_GPIO
#if PHY_USE_FEM_PA
    phy_gpiote_configure(PHY_GPIOTE_FEM, MYNEWT_VAL(BLE_FEM_PA_GPIO));
#else
    phy_gpiote_configure(PHY_GPIOTE_FEM, MYNEWT_VAL(BLE_FEM_LNA_GPIO));
#endif
    NRF_PPI->CH[6].TEP = (uint32_t)&(NRF_GPIOTE->TASKS_SET[PHY_GPIOTE_FEM]);
    NRF_PPI->CH[7].TEP = (uint32_t)&(NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM]);
#else
#if PHY_USE_FEM_PA
    phy_gpiote_configure(PHY_GPIOTE_FEM_PA, MYNEWT_VAL(BLE_FEM_PA_GPIO));
    NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM_PA] = 1;
#endif
#if PHY_USE_FEM_LNA
    phy_gpiote_configure(PHY_GPIOTE_FEM_LNA, MYNEWT_VAL(BLE_FEM_LNA_GPIO));
    NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM_LNA] = 1;
#endif
#endif /* PHY_USE_FEM_SINGLE_GPIO */

    NRF_PPI->CH[6].EEP = (uint32_t)&(NRF_TIMER0->EVENTS_COMPARE[2]);
    NRF_PPI->CH[7].EEP = (uint32_t)&(NRF_RADIO->EVENTS_DISABLED);

    nrf_ppi_channels_disable(NRF_PPI, PPI_CHEN_CH6_Msk | PPI_CHEN_CH7_Msk);
}

#if PHY_USE_FEM_PA
void
phy_fem_enable_pa(void)
{
    ble_fem_pa_enable();

#if !PHY_USE_FEM_SINGLE_GPIO
    /* Switch FEM channels to control PA */
    NRF_PPI->CH[6].TEP = (uint32_t)&(NRF_GPIOTE->TASKS_SET[PHY_GPIOTE_FEM_PA]);
    NRF_PPI->CH[7].TEP = (uint32_t)&(NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM_PA]);
#endif

    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH6_Msk | PPI_CHEN_CH7_Msk);
}
#endif

#if PHY_USE_FEM_LNA
void
phy_fem_enable_lna(void)
{
    ble_fem_lna_enable();

#if !PHY_USE_FEM_SINGLE_GPIO
    /* Switch FEM channels to control LNA */
    NRF_PPI->CH[6].TEP = (uint32_t)&(NRF_GPIOTE->TASKS_SET[PHY_GPIOTE_FEM_LNA]);
    NRF_PPI->CH[7].TEP = (uint32_t)&(NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM_LNA]);
#endif

    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH6_Msk | PPI_CHEN_CH7_Msk);
}
#endif

void
phy_fem_disable(void)
{
#if PHY_USE_FEM_SINGLE_GPIO
    NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM] = 1;
#else
#if PHY_USE_FEM_PA
    NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM_PA] = 1;
#endif
#if PHY_USE_FEM_LNA
    NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM_LNA] = 1;
#endif
#endif
}
#endif /* PHY_USE_FEM */

void
phy_ppi_init(void)
{
    /* radio_address_to_timer0_capture1 */
    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH26_Msk);
    /* radio_end_to_timer0_capture2 */
    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH27_Msk);

    /*
     * PPI setup.
     * Channel 4: Captures TIMER0 in CC[3] when EVENTS_ADDRESS occurs. Used
     *            to cancel the wait for response timer.
     * Channel 5: TIMER0 CC[3] to TASKS_DISABLE on radio. This is the wait
     *            for response timer.
     */
    nrf_ppi_channel_endpoint_setup(NRF_PPI, NRF_PPI_CHANNEL4,
                                   (uint32_t)&(NRF_RADIO->EVENTS_ADDRESS),
                                   (uint32_t)&(NRF_TIMER0->TASKS_CAPTURE[3]));
    nrf_ppi_channel_endpoint_setup(NRF_PPI, NRF_PPI_CHANNEL5,
                                   (uint32_t)&(NRF_TIMER0->EVENTS_COMPARE[3]),
                                   (uint32_t)&(NRF_RADIO->TASKS_DISABLE));
}

void
phy_txpower_set(int8_t dbm)
{
    NRF_RADIO->TXPOWER = dbm;
}

int8_t
phy_txpower_round(int8_t dbm)
{
/* "Rail" power level if outside supported range */
#ifdef RADIO_TXPOWER_TXPOWER_Pos8dBm
    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos8dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Pos8dBm;
    }
#endif

#ifdef RADIO_TXPOWER_TXPOWER_Pos7dBm
    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos7dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Pos7dBm;
    }
#endif

#ifdef RADIO_TXPOWER_TXPOWER_Pos6dBm
    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos6dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Pos6dBm;
    }
#endif

#ifdef RADIO_TXPOWER_TXPOWER_Pos5dBm
    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos5dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Pos5dBm;
    }
#endif

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos4dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Pos4dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Pos3dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Pos3dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_0dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_0dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg4dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg4dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg8dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg8dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm;
    }

    return (int8_t)RADIO_TXPOWER_TXPOWER_Neg40dBm;
}

#endif /* defined(ARDUINO_ARCH_NRF5) && defined(NRF52_SERIES) */
