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

#ifndef H_BLE_LL_SCAN_AUX_
#define H_BLE_LL_SCAN_AUX_

#ifdef __cplusplus
extern "C" {
#endif

#if MYNEWT_VAL(BLE_LL_CFG_FEAT_LL_EXT_ADV)

struct ble_ll_scan_aux_data;

void ble_ll_scan_aux_init(void);
int ble_ll_scan_aux_sched(struct ble_ll_scan_aux_data *aux, uint32_t pdu_ticks,
                          uint8_t pdu_rem_us, uint32_t aux_ptr);
int ble_ll_scan_aux_rx_isr_start(uint8_t pdu_type, struct ble_mbuf_hdr *rxhdr);
int ble_ll_scan_aux_rx_isr_end(struct os_mbuf *rxpdu, uint8_t crcok);
void ble_ll_scan_aux_rx_pkt_in(struct os_mbuf *rxpdu, struct ble_mbuf_hdr *rxhdr);

void ble_ll_scan_aux_break(struct ble_ll_scan_aux_data *aux);
void ble_ll_scan_aux_wfr_timer_exp(void);
void ble_ll_scan_aux_halt(void);
void ble_ll_scan_aux_sched_remove(struct ble_ll_sched_item *sch);

int ble_ll_scan_aux_rx_isr_end_on_ext(struct ble_ll_scan_sm *scansm,
                                      struct os_mbuf *rxpdu);
void ble_ll_scan_aux_pkt_in_on_ext(struct os_mbuf *rxpdu,
                                   struct ble_mbuf_hdr *rxhdr);

#endif

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_LL_SCAN_AUX_ */
