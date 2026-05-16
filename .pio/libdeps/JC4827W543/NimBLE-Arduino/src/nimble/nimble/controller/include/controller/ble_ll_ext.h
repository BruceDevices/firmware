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

#ifndef H_BLE_LL_EXT_
#define H_BLE_LL_EXT_

#ifdef __cplusplus
extern "C" {
#endif

#if MYNEWT_VAL(BLE_LL_EXT)

/* Quickstart guide:
 * - create scheduling item with sched_type set to BLE_LL_SCHED_EXTERNAL
 * - use sched_ext_type to differentiate between different types of custom items
 * - insert into scheduler using ble_ll_sched_insert()
 * - set LL state to BLE_LL_STATE_EXTERNAL when item is being executed
 * - set LL state back to BLE_LL_STATE_IDLE when item is done
 */

struct ble_ll_sched_item;

/* Called when LL package is initialized (before ll_task is started) */
void ble_ll_ext_init(void);
/* Called when LL is reset (i.e. HCI_Reset) */
void ble_ll_ext_reset(void);
/* Called when LL is in "external" state and PHY starts to receive a PDU */
int ble_ll_ext_rx_isr_start(uint8_t pdu_type, struct ble_mbuf_hdr *rxhdr);
/* Called when LL is in "external" state and PHY finished to receive a PDU */
int ble_ll_ext_rx_isr_end(uint8_t *rxbuf, struct ble_mbuf_hdr *rxhdr);
/* Called when PDU received in "external" state reaches LL */
void ble_ll_ext_rx_pkt_in(struct os_mbuf *rxpdu, struct ble_mbuf_hdr *rxhdr);
/* Called when LL is in "external" state and was preempted */
void ble_ll_ext_halt(void);
/* Called when LL is in "external" state and PHY failed to receive a PDU */
void ble_ll_ext_wfr_timer_exp(void);
/* Called when an "external" scheduling item was removed from scheduler queue */
void ble_ll_ext_sched_removed(struct ble_ll_sched_item *sch);

#endif

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_LL_EXT_ */
