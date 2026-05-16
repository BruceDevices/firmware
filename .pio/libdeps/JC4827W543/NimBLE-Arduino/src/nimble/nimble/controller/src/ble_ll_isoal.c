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

#ifndef ESP_PLATFORM

#include <stdint.h>
#include <nimble/porting/nimble/include/syscfg/syscfg.h>
#include <nimble/nimble/include/nimble/hci_common.h>
#include <nimble/nimble/controller/include/controller/ble_ll.h>
#include <nimble/nimble/controller/include/controller/ble_ll_isoal.h>
#include <nimble/nimble/controller/include/controller/ble_ll_iso_big.h>

#if MYNEWT_VAL(BLE_LL_ISO)

STAILQ_HEAD(ble_ll_iso_tx_q, os_mbuf_pkthdr);

static struct ble_npl_event ll_isoal_tx_pkt_in;
static struct ble_ll_iso_tx_q ll_isoal_tx_q;

void
ble_ll_isoal_mux_init(struct ble_ll_isoal_mux *mux, uint8_t max_pdu,
                      uint32_t iso_interval_us, uint32_t sdu_interval_us,
                      uint8_t bn, uint8_t pte)
{
    memset(mux, 0, sizeof(*mux));

    mux->max_pdu = max_pdu;
    /* Core 5.3, Vol 6, Part G, 2.1 */
    mux->sdu_per_interval = iso_interval_us / sdu_interval_us;
    mux->pdu_per_sdu = bn / mux->sdu_per_interval;

    mux->sdu_per_event = (1 + pte) * mux->sdu_per_interval;

    STAILQ_INIT(&mux->sdu_q);
}

void
ble_ll_isoal_mux_free(struct ble_ll_isoal_mux *mux)
{
    struct os_mbuf_pkthdr *pkthdr;
    struct os_mbuf *om;
    struct os_mbuf *om_next;

    pkthdr = STAILQ_FIRST(&mux->sdu_q);
    while (pkthdr) {
        om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);

        while (om) {
            om_next = SLIST_NEXT(om, om_next);
            os_mbuf_free(om);
            om = om_next;
        }

        STAILQ_REMOVE_HEAD(&mux->sdu_q, omp_next);
        pkthdr = STAILQ_FIRST(&mux->sdu_q);
    }

    STAILQ_INIT(&mux->sdu_q);
}

static void
ble_ll_isoal_mux_tx_pkt_in(struct ble_ll_isoal_mux *mux, struct os_mbuf *om,
                           uint8_t pb, uint32_t timestamp)
{
    struct os_mbuf_pkthdr *pkthdr;
    struct ble_mbuf_hdr *blehdr;
    os_sr_t sr;

    BLE_LL_ASSERT(mux);

    switch (pb) {
    case BLE_HCI_ISO_PB_FIRST:
        BLE_LL_ASSERT(!mux->frag);
        mux->frag = om;
        om = NULL;
        break;
    case BLE_HCI_ISO_PB_CONTINUATION:
        BLE_LL_ASSERT(mux->frag);
        os_mbuf_concat(mux->frag, om);
        om = NULL;
        break;
    case BLE_HCI_ISO_PB_COMPLETE:
        BLE_LL_ASSERT(!mux->frag);
        break;
    case BLE_HCI_ISO_PB_LAST:
        BLE_LL_ASSERT(mux->frag);
        os_mbuf_concat(mux->frag, om);
        om = mux->frag;
        mux->frag = NULL;
        break;
    default:
        BLE_LL_ASSERT(0);
        break;
    }

    if (!om) {
        return;
    }

    blehdr = BLE_MBUF_HDR_PTR(om);
    blehdr->txiso.packet_seq_num = ++mux->sdu_counter;

    OS_ENTER_CRITICAL(sr);
    pkthdr = OS_MBUF_PKTHDR(om);
    STAILQ_INSERT_TAIL(&mux->sdu_q, pkthdr, omp_next);
    OS_EXIT_CRITICAL(sr);
}

int
ble_ll_isoal_mux_event_start(struct ble_ll_isoal_mux *mux, uint32_t timestamp)
{
    struct os_mbuf_pkthdr *pkthdr;
    uint8_t num_sdu;

    num_sdu = mux->sdu_per_event;

    pkthdr = STAILQ_FIRST(&mux->sdu_q);
    while (pkthdr && num_sdu--) {
        pkthdr = STAILQ_NEXT(pkthdr, omp_next);
    }

    mux->sdu_in_event = mux->sdu_per_event - num_sdu;
    mux->event_tx_timestamp = timestamp;

    return mux->sdu_in_event;
}

int
ble_ll_isoal_mux_event_done(struct ble_ll_isoal_mux *mux)
{
    struct os_mbuf_pkthdr *pkthdr;
    struct ble_mbuf_hdr *blehdr;
    struct os_mbuf *om;
    struct os_mbuf *om_next;
    uint8_t num_sdu;
    int pkt_freed = 0;

    num_sdu = min(mux->sdu_in_event, mux->sdu_per_interval);

    pkthdr = STAILQ_FIRST(&mux->sdu_q);
    if (pkthdr) {
        om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);
        blehdr = BLE_MBUF_HDR_PTR(om);
        mux->last_tx_timestamp = mux->event_tx_timestamp;
        mux->last_tx_packet_seq_num = blehdr->txiso.packet_seq_num;
    }

    while (pkthdr && num_sdu--) {
        om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);

        while (om) {
            om_next = SLIST_NEXT(om, om_next);
            os_mbuf_free(om);
            pkt_freed++;
            om = om_next;
        }

        STAILQ_REMOVE_HEAD(&mux->sdu_q, omp_next);
        pkthdr = STAILQ_FIRST(&mux->sdu_q);
    }

    mux->sdu_in_event = 0;

    return pkt_freed;
}

int
ble_ll_isoal_mux_unframed_get(struct ble_ll_isoal_mux *mux, uint8_t idx,
                              uint8_t *llid, void *dptr)
{
    struct os_mbuf_pkthdr *pkthdr;
    struct os_mbuf *om;
    uint8_t sdu_idx;
    uint8_t pdu_idx;
    uint16_t sdu_offset;
    uint16_t rem_len;
    uint8_t pdu_len;

    sdu_idx = idx / mux->pdu_per_sdu;
    pdu_idx = idx - sdu_idx * mux->pdu_per_sdu;

    if (sdu_idx >= mux->sdu_in_event) {
        *llid = 1;
        return 0;
    }

    pkthdr = STAILQ_FIRST(&mux->sdu_q);
    while (pkthdr && sdu_idx--) {
        pkthdr = STAILQ_NEXT(pkthdr, omp_next);
    }

    if (!pkthdr) {
        *llid = 1;
        return 0;
    }

    om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);
    sdu_offset = pdu_idx * mux->max_pdu;
    rem_len = OS_MBUF_PKTLEN(om) - sdu_offset;

    if ((int32_t)rem_len <= 0) {
        *llid = 1;
        pdu_len = 0;
    } else {
        *llid = (pdu_idx < mux->pdu_per_sdu - 1);
        pdu_len = min(mux->max_pdu, rem_len);
    }

    os_mbuf_copydata(om, sdu_offset, pdu_len, dptr);

    return pdu_len;
}

static void
ble_ll_isoal_tx_pkt_in(struct ble_npl_event *ev)
{
    struct os_mbuf *om;
    struct os_mbuf_pkthdr *pkthdr;
    struct ble_hci_iso *hci_iso;
    struct ble_hci_iso_data *hci_iso_data;
    struct ble_ll_isoal_mux *mux;
    uint16_t data_hdr_len;
    uint16_t handle;
    uint16_t conn_handle;
    uint16_t length;
    uint16_t pb_flag;
    uint16_t ts_flag;
    uint32_t timestamp = 0;
    os_sr_t sr;

    while (STAILQ_FIRST(&ll_isoal_tx_q)) {
        pkthdr = STAILQ_FIRST(&ll_isoal_tx_q);
        om = OS_MBUF_PKTHDR_TO_MBUF(pkthdr);

        OS_ENTER_CRITICAL(sr);
        STAILQ_REMOVE_HEAD(&ll_isoal_tx_q, omp_next);
        OS_EXIT_CRITICAL(sr);

        hci_iso = (void *)om->om_data;

        handle = le16toh(hci_iso->handle);
        conn_handle = BLE_HCI_ISO_CONN_HANDLE(handle);
        pb_flag = BLE_HCI_ISO_PB_FLAG(handle);
        ts_flag = BLE_HCI_ISO_TS_FLAG(handle);
        length = BLE_HCI_ISO_LENGTH(le16toh(hci_iso->length));

        data_hdr_len = 0;
        if ((pb_flag == BLE_HCI_ISO_PB_FIRST) ||
            (pb_flag == BLE_HCI_ISO_PB_COMPLETE)) {
            if (ts_flag) {
                timestamp = get_le32(om->om_data + sizeof(*hci_iso));
                data_hdr_len += sizeof(uint32_t);
            }

            hci_iso_data = (void *)(om->om_data + sizeof(*hci_iso) + data_hdr_len);
            data_hdr_len += sizeof(*hci_iso_data);
        }
        os_mbuf_adj(om, sizeof(*hci_iso) + data_hdr_len);

        if (OS_MBUF_PKTLEN(om) != length - data_hdr_len) {
            os_mbuf_free_chain(om);
            continue;
        }

        switch (BLE_LL_CONN_HANDLE_TYPE(conn_handle)) {
        case BLE_LL_CONN_HANDLE_TYPE_BIS:
            mux = ble_ll_iso_big_find_mux_by_handle(conn_handle);
            ble_ll_isoal_mux_tx_pkt_in(mux, om, pb_flag, timestamp);
            break;
        default:
            os_mbuf_free_chain(om);
            break;
        }
    }
}

int
ble_ll_isoal_hci_setup_iso_data_path(const uint8_t *cmdbuf, uint8_t cmdlen,
                                     uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_setup_iso_data_path_cp *cmd = (const void *)cmdbuf;
    struct ble_hci_le_setup_iso_data_path_rp *rsp = (void *)rspbuf;
    struct ble_ll_iso_bis *bis;
    uint16_t conn_handle;

    conn_handle = le16toh(cmd->conn_handle);
    switch (BLE_LL_CONN_HANDLE_TYPE(conn_handle)) {
    case BLE_LL_CONN_HANDLE_TYPE_BIS:
        bis = ble_ll_iso_big_find_bis_by_handle(conn_handle);
        if (bis) {
            break;
        }
    default:
        return BLE_ERR_UNK_CONN_ID;
    }

    /* Only input for now since we only support BIS */
    if (cmd->data_path_dir) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    /* We do not (yet) support any vendor-specific data path */
    if (cmd->data_path_id) {
        return BLE_ERR_CMD_DISALLOWED;
    }

    rsp->conn_handle = cmd->conn_handle;
    *rsplen = sizeof(*rsp);

    return 0;
}

int
ble_ll_isoal_hci_remove_iso_data_path(const uint8_t *cmdbuf, uint8_t cmdlen,
                                      uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_remove_iso_data_path_cp *cmd = (const void *)cmdbuf;
    struct ble_hci_le_remove_iso_data_path_rp *rsp = (void *)rspbuf;

    /* XXX accepts anything for now */
    rsp->conn_handle = cmd->conn_handle;
    *rsplen = sizeof(*rsp);

    return 0;
}

int
ble_ll_isoal_hci_read_tx_sync(const uint8_t *cmdbuf, uint8_t cmdlen,
                              uint8_t *rspbuf, uint8_t *rsplen)
{
    const struct ble_hci_le_read_iso_tx_sync_cp *cmd = (const void *)cmdbuf;
    struct ble_hci_le_read_iso_tx_sync_rp *rsp = (void *)rspbuf;
    struct ble_ll_isoal_mux *mux;
    uint16_t handle;

    handle = le16toh(cmd->conn_handle);
    switch (BLE_LL_CONN_HANDLE_TYPE(handle)) {
    case BLE_LL_CONN_HANDLE_TYPE_BIS:
        mux = ble_ll_iso_big_find_mux_by_handle(handle);
        if (!mux) {
            return BLE_ERR_UNK_CONN_ID;
        }
        break;
    default:
        return BLE_ERR_UNK_CONN_ID;
    }

    rsp->conn_handle = cmd->conn_handle;
    rsp->packet_seq_num = htole16(mux->last_tx_packet_seq_num);
    rsp->tx_timestamp = htole32(mux->last_tx_timestamp);
    put_le24(rsp->time_offset, 0);

    *rsplen = sizeof(*rsp);

    return 0;
}

void
ble_ll_isoal_init(void)
{
    STAILQ_INIT(&ll_isoal_tx_q);
    ble_npl_event_init(&ll_isoal_tx_pkt_in, ble_ll_isoal_tx_pkt_in, NULL);
}

void
ble_ll_isoal_reset(void)
{
    STAILQ_INIT(&ll_isoal_tx_q);
    ble_npl_eventq_remove(&g_ble_ll_data.ll_evq, &ll_isoal_tx_pkt_in);
}

int
ble_ll_isoal_data_in(struct os_mbuf *om)
{
    struct os_mbuf_pkthdr *hdr;
    os_sr_t sr;

    hdr = OS_MBUF_PKTHDR(om);

    OS_ENTER_CRITICAL(sr);
    STAILQ_INSERT_TAIL(&ll_isoal_tx_q, hdr, omp_next);
    OS_EXIT_CRITICAL(sr);

    ble_npl_eventq_put(&g_ble_ll_data.ll_evq, &ll_isoal_tx_pkt_in);

    return 0;
}

#endif /* BLE_LL_ISO */
#endif /* ESP_PLATFORM */
