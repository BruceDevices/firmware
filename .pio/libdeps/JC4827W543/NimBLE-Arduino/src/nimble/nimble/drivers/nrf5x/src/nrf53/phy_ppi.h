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

#ifndef H_PHY_PPI_
#define H_PHY_PPI_

#define DPPI_CH_PUB(_ch)        (((DPPI_CH_ ## _ch) & 0xff) | (1 << 31))
#define DPPI_CH_SUB(_ch)        (((DPPI_CH_ ## _ch) & 0xff) | (1 << 31))
#define DPPI_CH_UNSUB(_ch)      (((DPPI_CH_ ## _ch) & 0xff) | (0 << 31))
#define DPPI_CH_MASK(_ch)       (1 << (DPPI_CH_ ## _ch))

/* Channels 0..5 are always used.
 * Channels 6 and 7 are used for PA/LNA (optionally).
 * Channels 7..9 are used for GPIO debugging (optionally).
 */

#define DPPI_CH_TIMER0_EVENTS_COMPARE_0         0
#define DPPI_CH_TIMER0_EVENTS_COMPARE_3         1
#define DPPI_CH_RADIO_EVENTS_END                2
#define DPPI_CH_RADIO_EVENTS_BCMATCH            3
#define DPPI_CH_RADIO_EVENTS_ADDRESS            4
#define DPPI_CH_RTC0_EVENTS_COMPARE_0           5
#define DPPI_CH_TIMER0_EVENTS_COMPARE_2         6
#define DPPI_CH_RADIO_EVENTS_DISABLED           7
#define DPPI_CH_RADIO_EVENTS_READY              8
#define DPPI_CH_RADIO_EVENTS_RXREADY            9

#define DPPI_CH_ENABLE_ALL  (DPPIC_CHEN_CH0_Msk | DPPIC_CHEN_CH1_Msk | \
                             DPPIC_CHEN_CH2_Msk | DPPIC_CHEN_CH3_Msk | \
                             DPPIC_CHEN_CH4_Msk | DPPIC_CHEN_CH5_Msk)

#define DPPI_CH_MASK_FEM    (DPPI_CH_MASK(TIMER0_EVENTS_COMPARE_2) | \
                             DPPI_CH_MASK(RADIO_EVENTS_DISABLED))

static inline void
phy_ppi_rtc0_compare0_to_timer0_start_enable(void)
{
    NRF_TIMER0->SUBSCRIBE_START = DPPI_CH_SUB(RTC0_EVENTS_COMPARE_0);
}

static inline void
phy_ppi_rtc0_compare0_to_timer0_start_disable(void)
{
    NRF_TIMER0->SUBSCRIBE_START = DPPI_CH_UNSUB(RTC0_EVENTS_COMPARE_0);
    NRF_TIMER0->SUBSCRIBE_CAPTURE[3] = DPPI_CH_UNSUB(RADIO_EVENTS_ADDRESS);
    NRF_RADIO->SUBSCRIBE_DISABLE = DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_3);
}

static inline void
phy_ppi_timer0_compare0_to_radio_txen_enable(void)
{
    NRF_RADIO->SUBSCRIBE_TXEN = DPPI_CH_SUB(TIMER0_EVENTS_COMPARE_0);
}

static inline void
phy_ppi_timer0_compare0_to_radio_txen_disable(void)
{
    NRF_RADIO->SUBSCRIBE_TXEN = DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_0);
}

static inline void
phy_ppi_timer0_compare0_to_radio_rxen_enable(void)
{
    NRF_RADIO->SUBSCRIBE_RXEN = DPPI_CH_SUB(TIMER0_EVENTS_COMPARE_0);
}

static inline void
phy_ppi_timer0_compare0_to_radio_rxen_disable(void)
{
    NRF_RADIO->SUBSCRIBE_RXEN = DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_0);
}

static inline void
phy_ppi_radio_address_to_ccm_crypt_enable(void)
{
    NRF_CCM->SUBSCRIBE_CRYPT = DPPI_CH_SUB(RADIO_EVENTS_ADDRESS);
}

static inline void
phy_ppi_radio_address_to_ccm_crypt_disable(void)
{
    NRF_CCM->SUBSCRIBE_CRYPT = DPPI_CH_UNSUB(RADIO_EVENTS_ADDRESS);
}

static inline void
phy_ppi_radio_bcmatch_to_aar_start_enable(void)
{
    NRF_AAR->SUBSCRIBE_START = DPPI_CH_SUB(RADIO_EVENTS_BCMATCH);
}

static inline void
phy_ppi_radio_bcmatch_to_aar_start_disable(void)
{
    NRF_AAR->SUBSCRIBE_START = DPPI_CH_UNSUB(RADIO_EVENTS_BCMATCH);
}

static inline void
phy_ppi_wfr_enable(void)
{
    NRF_TIMER0->SUBSCRIBE_CAPTURE[3] = DPPI_CH_SUB(RADIO_EVENTS_ADDRESS);
    NRF_RADIO->SUBSCRIBE_DISABLE = DPPI_CH_SUB(TIMER0_EVENTS_COMPARE_3);
}

static inline void
phy_ppi_wfr_disable(void)
{
    NRF_TIMER0->SUBSCRIBE_CAPTURE[3] = DPPI_CH_UNSUB(RADIO_EVENTS_ADDRESS);
    NRF_RADIO->SUBSCRIBE_DISABLE = DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_3);
}

static inline void
phy_ppi_fem_disable(void)
{
#if PHY_USE_FEM_SINGLE_GPIO
    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_FEM] =
        DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_3);
#else
#if PHY_USE_FEM_PA
    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_FEM_PA] =
        DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_2);
#endif
#if PHY_USE_FEM_LNA
    NRF_GPIOTE->SUBSCRIBE_SET[PHY_GPIOTE_FEM_LNA] =
        DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_2);
#endif
#endif
}

static inline void
phy_ppi_disable(void)
{
    NRF_TIMER0->SUBSCRIBE_START = DPPI_CH_UNSUB(RTC0_EVENTS_COMPARE_0);
    NRF_TIMER0->SUBSCRIBE_CAPTURE[3] = DPPI_CH_UNSUB(RADIO_EVENTS_ADDRESS);
    NRF_RADIO->SUBSCRIBE_DISABLE = DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_3);
    NRF_RADIO->SUBSCRIBE_TXEN = DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_0);
    NRF_RADIO->SUBSCRIBE_RXEN = DPPI_CH_UNSUB(TIMER0_EVENTS_COMPARE_0);
    NRF_AAR->SUBSCRIBE_START = DPPI_CH_UNSUB(RADIO_EVENTS_BCMATCH);
    NRF_CCM->SUBSCRIBE_CRYPT = DPPI_CH_UNSUB(RADIO_EVENTS_ADDRESS);

    phy_ppi_fem_disable();
}

#endif /* H_PHY_PPI_ */