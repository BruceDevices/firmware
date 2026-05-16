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

#if defined(ARDUINO_ARCH_NRF5) && defined(NRF53)

#include <stdint.h>
#include <nrfx.h>
#include <nimble/nimble/controller/include/controller/ble_fem.h>
#include "nimble/nimble/drivers/nrf5x/src/phy_priv.h"

/*
 * When the radio is operated on high voltage (see VREQCTRL - Voltage request
 * control on page 62 for how to control voltage), the output power is increased
 * by 3 dB. I.e. if the TXPOWER value is set to 0 dBm and high voltage is
 * requested using VREQCTRL, the output power will be +3
 * */
#define NRF_TXPOWER_VREQH 3

#if PHY_USE_DEBUG
void
phy_debug_init(void)
{
#if PHY_USE_DEBUG_1
    phy_gpiote_configure(PHY_GPIOTE_DEBUG_1,
                         MYNEWT_VAL(BLE_PHY_DBG_TIME_TXRXEN_READY_PIN));

    NRF_RADIO->PUBLISH_READY = DPPI_CH_PUB(RADIO_EVENTS_READY);
    NRF_DPPIC->CHENSET = DPPI_CH_MASK(RADIO_EVENTS_READY);

    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_DEBUG_1] = DPPI_CH_SUB(TIMER0_EVENTS_COMPARE_0);
    NRF_GPIOTE->SUBSCRIBE_CLR[PHY_GPIOTE_DEBUG_1] = DPPI_CH_SUB(RADIO_EVENTS_READY);
#endif

#if PHY_USE_DEBUG_2
    phy_gpiote_configure(PHY_GPIOTE_DEBUG_2,
                         MYNEWT_VAL(BLE_PHY_DBG_TIME_ADDRESS_END_PIN));

    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_DEBUG_2] = DPPI_CH_SUB(RADIO_EVENTS_ADDRESS);
    NRF_GPIOTE->SUBSCRIBE_CLR[PHY_GPIOTE_DEBUG_2] = DPPI_CH_SUB(RADIO_EVENTS_END);
#endif

#if PHY_USE_DEBUG_3
    phy_gpiote_configure(PHY_GPIOTE_DEBUG_3, MYNEWT_VAL(BLE_PHY_DBG_TIME_WFR_PIN));

    NRF_RADIO->PUBLISH_RXREADY = DPPI_CH_PUB(RADIO_EVENTS_READY);
    NRF_RADIO->PUBLISH_DISABLED = DPPI_CH_PUB(RADIO_EVENTS_DISABLED);
    NRF_DPPIC->CHENSET = DPPI_CH_MASK(RADIO_EVENTS_RXREADY) |
                         DPPI_CH_MASK(RADIO_EVENTS_DISABLED);

    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_DEBUG_3] = DPPI_CH_SUB(RADIO_EVENTS_RXREADY);

    /* TODO figure out how (if?) to subscribe task to multiple DPPI channels
     * Currently only last one is working. Also using multiple GPIOTE for same
     * PIN doesn't work...
     */
    NRF_GPIOTE->SUBSCRIBE_CLR[PHY_GPIOTE_DEBUG_3] = DPPI_CH_SUB(RADIO_EVENTS_DISABLED);
    NRF_GPIOTE->SUBSCRIBE_CLR[PHY_GPIOTE_DEBUG_3] = DPPI_CH_SUB(RADIO_EVENTS_ADDRESS);
    NRF_GPIOTE->SUBSCRIBE_CLR[PHY_GPIOTE_DEBUG_3] = DPPI_CH_SUB(TIMER0_EVENTS_COMPARE_3);

#endif
}
#endif /* PHY_USE_DEBUG */

#if PHY_USE_FEM
void
phy_fem_init()
{
    /* We can keep clear tasks subscribed and published channels always enabled,
     * it's enough to just (un)subscribe set tasks when needed.
     * TODO: check if this affects power consumption
     */

    NRF_TIMER0->PUBLISH_COMPARE[2] = DPPI_CH_PUB(TIMER0_EVENTS_COMPARE_2);
    NRF_RADIO->PUBLISH_DISABLED = DPPI_CH_PUB(RADIO_EVENTS_DISABLED);

#if PHY_USE_FEM_SINGLE_GPIO
#if PHY_USE_FEM_PA
    phy_gpiote_configure(PHY_GPIOTE_FEM, MYNEWT_VAL(BLE_FEM_PA_GPIO));
#else
    phy_gpiote_configure(PHY_GPIOTE_FEM, MYNEWT_VAL(BLE_FEM_LNA_GPIO));
#endif
    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_FEM] =
        DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_2);
    NRF_GPIOTE->SUBSCRIBE_CLR[PHY_GPIOTE_FEM] =
        DPPI_CH_SUB(RADIO_EVENTS_DISABLED);
    NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM] = 1;
#else
#if PHY_USE_FEM_PA
    phy_gpiote_configure(PHY_GPIOTE_FEM_PA, MYNEWT_VAL(BLE_FEM_PA_GPIO));
    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_FEM_PA] =
        DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_2);
    NRF_GPIOTE->SUBSCRIBE_CLR[PHY_GPIOTE_FEM_PA] =
        DPPI_CH_SUB(RADIO_EVENTS_DISABLED);
    NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM_PA] = 1;
#endif
#if PHY_USE_FEM_LNA
    phy_gpiote_configure(PHY_GPIOTE_FEM_LNA, MYNEWT_VAL(BLE_FEM_LNA_GPIO));
    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_FEM_LNA] =
        DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_2);
    NRF_GPIOTE->SUBSCRIBE_CLR[PHY_GPIOTE_FEM_LNA] =
        DPPI_CH_SUB(RADIO_EVENTS_DISABLED);
    NRF_GPIOTE->TASKS_CLR[PHY_GPIOTE_FEM_LNA] = 1;
#endif
#endif /* PHY_USE_FEM_SINGLE_GPIO */

    NRF_DPPIC->CHENSET = DPPI_CH_MASK_FEM;
}

#if PHY_USE_FEM_PA
void
phy_fem_enable_pa(void)
{
    ble_fem_pa_enable();

#if PHY_USE_FEM_SINGLE_GPIO
    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_FEM] =
        DPPI_CH_SUB(TIMER0_EVENTS_COMPARE_2);
#else
    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_FEM_PA] =
        DPPI_CH_SUB(TIMER0_EVENTS_COMPARE_2);
#endif
}
#endif

#if PHY_USE_FEM_LNA
void
phy_fem_enable_lna(void)
{
    ble_fem_lna_enable();

#if PHY_USE_FEM_SINGLE_GPIO
    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_FEM] =
        DPPI_CH_SUB(TIMER0_EVENTS_COMPARE_2);
#else
    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_FEM_LNA] =
        DPPI_CH_SUB(TIMER0_EVENTS_COMPARE_2);
#endif
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
    /* Publish events */
    NRF_TIMER0->PUBLISH_COMPARE[0] = DPPI_CH_PUB(TIMER0_EVENTS_COMPARE_0);
    NRF_TIMER0->PUBLISH_COMPARE[3] = DPPI_CH_PUB(TIMER0_EVENTS_COMPARE_3);
    NRF_RADIO->PUBLISH_END = DPPI_CH_PUB(RADIO_EVENTS_END);
    NRF_RADIO->PUBLISH_BCMATCH = DPPI_CH_PUB(RADIO_EVENTS_BCMATCH);
    NRF_RADIO->PUBLISH_ADDRESS = DPPI_CH_PUB(RADIO_EVENTS_ADDRESS);
    NRF_RTC0->PUBLISH_COMPARE[0] = DPPI_CH_PUB(RTC0_EVENTS_COMPARE_0);

    /* Enable channels we publish on */
    NRF_DPPIC->CHENSET = DPPI_CH_ENABLE_ALL;

    /* radio_address_to_timer0_capture1 */
    NRF_TIMER0->SUBSCRIBE_CAPTURE[1] = DPPI_CH_SUB(RADIO_EVENTS_ADDRESS);
    /* radio_end_to_timer0_capture2 */
    NRF_TIMER0->SUBSCRIBE_CAPTURE[2] = DPPI_CH_SUB(RADIO_EVENTS_END);
}

void
phy_txpower_set(int8_t dbm)
{
#if MYNEWT_VAL(BLE_PHY_NRF5340_VDDH)
    switch (dbm) {
    case ((int8_t)RADIO_TXPOWER_TXPOWER_0dBm) + NRF_TXPOWER_VREQH:
    case ((int8_t)RADIO_TXPOWER_TXPOWER_Neg1dBm) + NRF_TXPOWER_VREQH:
    case ((int8_t)RADIO_TXPOWER_TXPOWER_Neg2dBm) + NRF_TXPOWER_VREQH:
    case ((int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm) + NRF_TXPOWER_VREQH:
    case ((int8_t)RADIO_TXPOWER_TXPOWER_Neg16dBm) + NRF_TXPOWER_VREQH:
    case ((int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm) + NRF_TXPOWER_VREQH:
    case ((int8_t)RADIO_TXPOWER_TXPOWER_Neg40dBm) + NRF_TXPOWER_VREQH:
        NRF_VREQCTRL->VREGRADIO.VREQH = 1;
        dbm -= NRF_TXPOWER_VREQH;
        break;
    default:
        NRF_VREQCTRL->VREGRADIO.VREQH = 0;
        break;
    }
#endif

    NRF_RADIO->TXPOWER = dbm;
}

int8_t
phy_txpower_round(int8_t dbm)
{
#if MYNEWT_VAL(BLE_PHY_NRF5340_VDDH)
    /* +3 dBm */
    if (dbm >= ((int8_t)RADIO_TXPOWER_TXPOWER_0dBm) + NRF_TXPOWER_VREQH) {
        return ((int8_t)RADIO_TXPOWER_TXPOWER_0dBm) + NRF_TXPOWER_VREQH;
    }

    /* +2 dBm */
    if (dbm >= ((int8_t)RADIO_TXPOWER_TXPOWER_Neg1dBm) + NRF_TXPOWER_VREQH) {
        return ((int8_t)RADIO_TXPOWER_TXPOWER_Neg1dBm) + NRF_TXPOWER_VREQH;
    }

    /* +1 dBm */
    if (dbm >= ((int8_t)RADIO_TXPOWER_TXPOWER_Neg2dBm) + NRF_TXPOWER_VREQH) {
        return ((int8_t)RADIO_TXPOWER_TXPOWER_Neg2dBm) + NRF_TXPOWER_VREQH;
    }
#endif

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_0dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_0dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg1dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg1dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg2dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg2dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg3dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg3dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg4dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg4dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg5dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg5dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg6dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg6dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg7dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg7dBm;
    }

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg8dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg8dBm;
    }

#if MYNEWT_VAL(BLE_PHY_NRF5340_VDDH)
    /* -9 dBm */
    if (dbm >= ((int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm) + NRF_TXPOWER_VREQH) {
        return ((int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm) + NRF_TXPOWER_VREQH;
    }
#endif

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm;
    }

#if MYNEWT_VAL(BLE_PHY_NRF5340_VDDH)
    /* -13 dBm */
    if (dbm >= ((int8_t)RADIO_TXPOWER_TXPOWER_Neg16dBm) + NRF_TXPOWER_VREQH) {
        return ((int8_t)RADIO_TXPOWER_TXPOWER_Neg16dBm) + NRF_TXPOWER_VREQH;
    }
#endif

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg16dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg16dBm;
    }

#if MYNEWT_VAL(BLE_PHY_NRF5340_VDDH)
    /* -17 dBm */
    if (dbm >= ((int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm) + NRF_TXPOWER_VREQH) {
        return ((int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm) + NRF_TXPOWER_VREQH;
    }
#endif

    if (dbm >= (int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm) {
        return (int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm;
    }

#if MYNEWT_VAL(BLE_PHY_NRF5340_VDDH)
    /* -37 dBm */
    if (dbm >= ((int8_t)RADIO_TXPOWER_TXPOWER_Neg40dBm) + NRF_TXPOWER_VREQH) {
        return ((int8_t)RADIO_TXPOWER_TXPOWER_Neg40dBm) + NRF_TXPOWER_VREQH;
    }
#endif

    return (int8_t)RADIO_TXPOWER_TXPOWER_Neg40dBm;
}

#endif /* ARDUINO_ARCH_NRF5 && NRF53 */

