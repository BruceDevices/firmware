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

#ifndef H_BLE_LL_ISOAL_
#define H_BLE_LL_ISOAL_

#ifdef __cplusplus
extern "C" {
#endif

#if MYNEWT_VAL(BLE_LL_ISO)

struct ble_ll_isoal_mux {
    /* Max PDU length */
    uint8_t max_pdu;
    /* Number of expected SDUs per ISO interval */
    uint8_t sdu_per_interval;
    /* Number of expected PDUs per SDU */
    uint8_t pdu_per_sdu;
    /* Number of SDUs required to fill complete BIG/CIG event (i.e. with pt) */
    uint8_t sdu_per_event;
    /* Number of SDUs available for current event */
    uint8_t sdu_in_event;

    STAILQ_HEAD(, os_mbuf_pkthdr) sdu_q;

    struct os_mbuf *frag;

    uint32_t sdu_counter;

    uint32_t event_tx_timestamp;
    uint32_t last_tx_timestamp;
    uint16_t last_tx_packet_seq_num;
};

void
ble_ll_isoal_mux_init(struct ble_ll_isoal_mux *mux, uint8_t max_pdu,
                      uint32_t iso_interval_us, uint32_t sdu_interval_us,
                      uint8_t bn, uint8_t pte);
void ble_ll_isoal_mux_free(struct ble_ll_isoal_mux *mux);

int ble_ll_isoal_mux_event_start(struct ble_ll_isoal_mux *mux,
                                 uint32_t timestamp);
int ble_ll_isoal_mux_event_done(struct ble_ll_isoal_mux *mux);

int
ble_ll_isoal_mux_unframed_get(struct ble_ll_isoal_mux *mux, uint8_t idx,
                              uint8_t *llid, void *dptr);

/* HCI command handlers */
int ble_ll_isoal_hci_setup_iso_data_path(const uint8_t *cmdbuf, uint8_t cmdlen,
                                         uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_isoal_hci_remove_iso_data_path(const uint8_t *cmdbuf, uint8_t cmdlen,
                                          uint8_t *rspbuf, uint8_t *rsplen);
int ble_ll_isoal_hci_read_tx_sync(const uint8_t *cmdbuf, uint8_t cmdlen,
                                  uint8_t *rspbuf, uint8_t *rsplen);

void ble_ll_isoal_init(void);
void ble_ll_isoal_reset(void);
int ble_ll_isoal_data_in(struct os_mbuf *om);

#endif /* BLE_LL_ISO */

#ifdef __cplusplus
}
#endif

#endif /* H_BLE_LL_ISOAL_ */
