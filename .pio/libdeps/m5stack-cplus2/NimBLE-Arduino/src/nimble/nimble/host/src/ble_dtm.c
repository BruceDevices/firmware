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

#include <nimble/nimble/include/nimble/hci_common.h>
#include <nimble/nimble/host/include/host/ble_dtm.h>
#include <nimble/porting/nimble/include/os/endian.h>
#include "ble_hs_hci_priv.h"

int
ble_dtm_rx_start(const struct ble_dtm_rx_params *params)
{
    struct ble_hci_le_rx_test_v2_cp cmd;

    cmd.rx_chan = params->channel;
    cmd.phy = params->phy;
    cmd.index = params->modulation_index;

    return ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                        BLE_HCI_OCF_LE_RX_TEST_V2),
                             &cmd, sizeof(cmd), NULL, 0);
}

int
ble_dtm_tx_start(const struct ble_dtm_tx_params *params)
{
    struct ble_hci_le_tx_test_v2_cp cmd;

    cmd.tx_chan = params->channel;
    cmd.test_data_len = params->test_data_len;
    cmd.payload = params->payload;
    cmd.phy = params->phy;

    return ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                        BLE_HCI_OCF_LE_TX_TEST_V2),
                             &cmd, sizeof(cmd), NULL, 0);
}

int
ble_dtm_stop(uint16_t *num_packets)
{
    struct ble_hci_le_test_end_rp rsp;
    int rc;

    rc = ble_hs_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE,
                                      BLE_HCI_OCF_LE_TEST_END),
                           NULL, 0, &rsp, sizeof(rsp));

    if (rc) {
        *num_packets = 0;
    } else {
        *num_packets = le16toh(rsp.num_packets);
    }

    return rc;
}
