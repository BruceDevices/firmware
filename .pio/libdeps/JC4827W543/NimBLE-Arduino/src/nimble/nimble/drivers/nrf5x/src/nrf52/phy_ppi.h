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

#if defined(ARDUINO_ARCH_NRF5) && defined(NRF52_SERIES)

#include <nrf_ppi.h>

static inline void
phy_ppi_rtc0_compare0_to_timer0_start_enable(void)
{
    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH31_Msk);
}

static inline void
phy_ppi_rtc0_compare0_to_timer0_start_disable(void)
{
    nrf_ppi_channels_disable(NRF_PPI, PPI_CHEN_CH31_Msk);
}

static inline void
phy_ppi_timer0_compare0_to_radio_txen_enable(void)
{
    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH20_Msk);
}

static inline void
phy_ppi_timer0_compare0_to_radio_txen_disable(void)
{
    nrf_ppi_channels_disable(NRF_PPI, PPI_CHEN_CH20_Msk);
}

static inline void
phy_ppi_timer0_compare0_to_radio_rxen_enable(void)
{
    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH21_Msk);
}

static inline void
phy_ppi_timer0_compare0_to_radio_rxen_disable(void)
{
    nrf_ppi_channels_disable(NRF_PPI, PPI_CHEN_CH21_Msk);
}

static inline void
phy_ppi_radio_bcmatch_to_aar_start_enable(void)
{
    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH23_Msk);
}

static inline void
phy_ppi_radio_bcmatch_to_aar_start_disable(void)
{
    nrf_ppi_channels_disable(NRF_PPI, PPI_CHEN_CH23_Msk);
}

static inline void
phy_ppi_radio_address_to_ccm_crypt_enable(void)
{
    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH25_Msk);
}

static inline void
phy_ppi_radio_address_to_ccm_crypt_disable(void)
{
    nrf_ppi_channels_disable(NRF_PPI, PPI_CHEN_CH25_Msk);
}

static inline void
phy_ppi_wfr_enable(void)
{
    nrf_ppi_channels_enable(NRF_PPI, PPI_CHEN_CH4_Msk | PPI_CHEN_CH5_Msk);
}

static inline void
phy_ppi_wfr_disable(void)
{
    nrf_ppi_channels_disable(NRF_PPI, PPI_CHEN_CH4_Msk | PPI_CHEN_CH5_Msk);
}

static inline void
phy_ppi_fem_disable(void)
{
    nrf_ppi_channels_disable(NRF_PPI, PPI_CHEN_CH6_Msk | PPI_CHEN_CH7_Msk);
}

static inline void
phy_ppi_disable(void)
{
    nrf_ppi_channels_disable(NRF_PPI, PPI_CHEN_CH4_Msk | PPI_CHEN_CH5_Msk |
                                      PPI_CHEN_CH6_Msk | PPI_CHEN_CH7_Msk |
                                      PPI_CHEN_CH20_Msk | PPI_CHEN_CH21_Msk |
                                      PPI_CHEN_CH23_Msk | PPI_CHEN_CH25_Msk |
                                      PPI_CHEN_CH31_Msk);
}

#endif /* H_PHY_PPI_ */
#endif /* ARDUINO_ARCH_NRF5 && NRF52_SERIES */