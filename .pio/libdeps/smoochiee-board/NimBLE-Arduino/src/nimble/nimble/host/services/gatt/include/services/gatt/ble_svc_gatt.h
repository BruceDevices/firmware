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

#ifndef H_BLE_SVC_GATT_
#define H_BLE_SVC_GATT_

#include <inttypes.h>
#include "nimble/porting/nimble/include/syscfg/syscfg.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ble_hs_cfg;

#define BLE_SVC_GATT_CHR_SERVICE_CHANGED_UUID16         0x2a05
#define BLE_SVC_GATT_CHR_SERVER_SUPPORTED_FEAT_UUID16   0x2b3a
#define BLE_SVC_GATT_CHR_CLIENT_SUPPORTED_FEAT_UUID16   0x2b29

#define BLE_SVR_GATT_CHR_SVR_SUP_FEAT_EATT_FLAG             0x01

#define BLE_SVR_GATT_CHR_CLT_SUP_FEAT_ROBUST_CACHING_FLAG   0x01
#define BLE_SVR_GATT_CHR_CLT_SUP_FEAT_EATT_FLAG             0x02
#define BLE_SVR_GATT_CHR_CLT_SUP_FEAT_MULTI_NOTIF_FLAG      0x04

#if MYNEWT_VAL(BLE_GATT_CACHING)
#define BLE_SVC_GATT_CHR_DATABASE_HASH_UUID16               0x2b2a

uint16_t ble_svc_gatt_changed_handle(void);
uint16_t ble_svc_gatt_hash_handle(void);
uint16_t ble_svc_gatt_csf_handle(void);
uint8_t ble_svc_gatt_get_csfs(void);
#endif

uint8_t ble_svc_gatt_get_local_cl_supported_feat(void);
void ble_svc_gatt_changed(uint16_t start_handle, uint16_t end_handle);
void ble_svc_gatt_init(void);
void ble_svc_gatt_deinit(void);
#ifdef __cplusplus
}
#endif

#endif
