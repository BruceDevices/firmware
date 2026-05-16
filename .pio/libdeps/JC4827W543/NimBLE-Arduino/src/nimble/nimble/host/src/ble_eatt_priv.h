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

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/porting/nimble/include/os/os_mbuf.h"
#include "nimble/nimble/host/include/host/ble_l2cap.h"

#ifndef BLE_EATT_H_
#define BLE_EATT_H_

typedef int (* ble_eatt_att_rx_fn)(uint16_t conn_handle, uint16_t cid, struct os_mbuf **rx_buf);

#define BLE_EATT_PSM    (0x0027)

#define BLE_GATT_OP_SERVER 0xF1
#define BLE_GATT_OP_DUMMY  0xF2

#if MYNEWT_VAL(BLE_EATT_CHAN_NUM) > 0
void ble_eatt_init(ble_eatt_att_rx_fn att_rx_fn);
uint16_t ble_eatt_get_available_chan_cid(uint16_t conn_handle, uint8_t op);
void ble_eatt_release_chan(uint16_t conn_handle, uint8_t op);
int ble_eatt_tx(uint16_t conn_handle, uint16_t cid, struct os_mbuf *txom);
#else
static inline void
ble_eatt_init(ble_eatt_att_rx_fn att_rx_fn)
{
}

static inline void
ble_eatt_release_chan(uint16_t conn_handle, uint8_t op)
{

}

static inline uint16_t
ble_eatt_get_available_chan_cid(uint16_t conn_handle, uint8_t op)
{
    return BLE_L2CAP_CID_ATT;
}

#endif
#endif
