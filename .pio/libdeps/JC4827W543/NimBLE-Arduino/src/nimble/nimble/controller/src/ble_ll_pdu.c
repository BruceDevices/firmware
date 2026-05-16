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

#include <nimble/nimble/controller/include/controller/ble_phy.h>
#include <nimble/nimble/controller/include/controller/ble_ll_pdu.h>

static const uint16_t syncword_len[] = {
    [BLE_PHY_MODE_1M] = (BLE_LL_PDU_PREAMBLE_1M_LEN + BLE_LL_PDU_AA_LEN) * 8,
    [BLE_PHY_MODE_2M] = (BLE_LL_PDU_PREAMBLE_2M_LEN + BLE_LL_PDU_AA_LEN) * 4,
    [BLE_PHY_MODE_CODED_125KBPS] = 80 + 256 + 16 + 24,
    [BLE_PHY_MODE_CODED_500KBPS] = 80 + 256 + 16 + 24,
};

static const uint16_t payload0_len[] = {
    [BLE_PHY_MODE_1M] = (BLE_LL_PDU_PREAMBLE_1M_LEN + BLE_LL_PDU_AA_LEN +
                         BLE_LL_PDU_HEADER_LEN + BLE_LL_PDU_CRC_LEN) * 8,
    [BLE_PHY_MODE_2M] = (BLE_LL_PDU_PREAMBLE_2M_LEN + BLE_LL_PDU_AA_LEN +
                         BLE_LL_PDU_HEADER_LEN + BLE_LL_PDU_CRC_LEN) * 4,
    [BLE_PHY_MODE_CODED_125KBPS] = 80 + 256 + 16 + 24 +
                                   8 * (BLE_LL_PDU_HEADER_LEN * 8 + 24 + 3),
    [BLE_PHY_MODE_CODED_500KBPS] = 80 + 256 + 16 + 24 +
                                   2 * (BLE_LL_PDU_HEADER_LEN * 8 + 24 + 3),
};

static const uint8_t us_per_octet[] = {
    [BLE_PHY_MODE_1M] = 8,
    [BLE_PHY_MODE_2M] = 4,
    [BLE_PHY_MODE_CODED_125KBPS] = 64,
    [BLE_PHY_MODE_CODED_500KBPS] = 16,
};

uint32_t
ble_ll_pdu_syncword_us(uint8_t phy_mode)
{
    return syncword_len[phy_mode];
}

uint32_t
ble_ll_pdu_us(uint8_t payload_len, uint8_t phy_mode)
{
    return payload0_len[phy_mode] + (payload_len * us_per_octet[phy_mode]);
}
