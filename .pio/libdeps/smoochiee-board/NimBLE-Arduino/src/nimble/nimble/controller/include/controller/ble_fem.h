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

#ifndef H_BLE_FEM_
#define H_BLE_FEM_

#ifdef __cplusplus
extern "C" {
#endif

#include "nimble/porting/nimble/include/syscfg/syscfg.h"

#if MYNEWT_VAL(BLE_FEM_PA)
void ble_fem_pa_init(void);
void ble_fem_pa_enable(void);
void ble_fem_pa_disable(void);
#if MYNEWT_VAL(BLE_FEM_PA_GAIN_TUNABLE)
/* Configures FEM to selected TX power and returns expected PHY TX power */
int ble_fem_pa_tx_power_set(int tx_power);

/* returns rounded FEM TX power */
int ble_fem_pa_tx_power_round(int tx_power);
#endif
#endif

#if MYNEWT_VAL(BLE_FEM_LNA)
void ble_fem_lna_init(void);
void ble_fem_lna_enable(void);
void ble_fem_lna_disable(void);

#if MYNEWT_VAL(BLE_FEM_LNA_GAIN_TUNABLE)
/* Return current value of FEM LNA RX gain (in dBm) */
int ble_fem_lna_rx_gain(void);
#endif

#endif

#if MYNEWT_VAL(BLE_FEM_ANTENNA)
/* 0 sets default antenna, any other value is FEM specific */
int ble_fem_antenna(uint8_t antenna);
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_FEM_ */
