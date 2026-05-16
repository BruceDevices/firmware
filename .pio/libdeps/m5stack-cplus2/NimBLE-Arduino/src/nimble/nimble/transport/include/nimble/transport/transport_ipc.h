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

#ifndef H_NIMBLE_TRANSPORT_IPC_
#define H_NIMBLE_TRANSPORT_IPC_

#include <nimble/porting/nimble/include/syscfg/syscfg.h>

#ifdef __cplusplus
extern "C" {
#endif


/* NOTE: These APIs shall only be used by IPC transports */

#define BLE_TRANSPORT_IPC \
    MYNEWT_PKG_apache_mynewt_nimble__nimble_transport_common_hci_ipc
#define BLE_TRANSPORT_IPC_ON_HS \
    (BLE_TRANSPORT_IPC && !MYNEWT_VAL(BLE_CONTROLLER))
#define BLE_TRANSPORT_IPC_ON_LL \
    (BLE_TRANSPORT_IPC && MYNEWT_VAL(BLE_CONTROLLER))

/* Free cmd/evt buffer sent over IPC */
void ble_transport_ipc_free(void *buf);

/* Get IPC type for cmd/evt buffer */
uint8_t ble_transport_ipc_buf_evt_type_get(void *buf);

#ifdef __cplusplus
}
#endif

#endif /* H_NIMBLE_TRANSPORT_IPC_ */
