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

#include <string.h>
#include <errno.h>
#include "nimble/nimble/include/nimble/ble.h"
#include "ble_hs_priv.h"
#include "ble_l2cap_priv.h"
#include "ble_l2cap_coc_priv.h"
#include "ble_l2cap_sig_priv.h"

#if MYNEWT_VAL(BLE_L2CAP_COC_MAX_NUM) != 0 && NIMBLE_BLE_CONNECT

#define BLE_L2CAP_SDU_SIZE              2

STAILQ_HEAD(ble_l2cap_coc_srv_list, ble_l2cap_coc_srv);

static struct ble_l2cap_coc_srv_list ble_l2cap_coc_srvs;

static os_membuf_t ble_l2cap_coc_srv_mem[
    OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_L2CAP_COC_MAX_NUM),
                    sizeof(struct ble_l2cap_coc_srv))
];

static struct os_mempool ble_l2cap_coc_srv_pool;

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

static void
ble_l2cap_coc_dbg_assert_srv_not_inserted(struct ble_l2cap_coc_srv *srv)
{
#if MYNEWT_VAL(BLE_HS_DEBUG)
    struct ble_l2cap_coc_srv *cur;

    STAILQ_FOREACH(cur, &ble_l2cap_coc_srvs, next) {
        BLE_HS_DBG_ASSERT(cur != srv);
    }
#endif
}

static struct ble_l2cap_coc_srv *
ble_l2cap_coc_srv_alloc(void)
{
    struct ble_l2cap_coc_srv *srv;

    srv = os_memblock_get(&ble_l2cap_coc_srv_pool);
    if (srv != NULL) {
        memset(srv, 0, sizeof(*srv));
    }

    return srv;
}

int
ble_l2cap_coc_create_server(uint16_t psm, uint16_t mtu,
                            ble_l2cap_event_fn *cb, void *cb_arg)
{
    struct ble_l2cap_coc_srv *srv;

    srv = ble_l2cap_coc_srv_alloc();
    if (!srv) {
        return BLE_HS_ENOMEM;
    }

    srv->psm = psm;
    srv->mtu = mtu;
    srv->cb = cb;
    srv->cb_arg = cb_arg;

    ble_l2cap_coc_dbg_assert_srv_not_inserted(srv);

    STAILQ_INSERT_HEAD(&ble_l2cap_coc_srvs, srv, next);

    return 0;
}

static inline void
ble_l2cap_set_used_cid(uint32_t *cid_mask, int bit)
{
    cid_mask[bit / 32] |= (1 << (bit % 32));
}

static inline void
ble_l2cap_clear_used_cid(uint32_t *cid_mask, int bit)
{
    cid_mask[bit / 32] &= ~(1 << (bit % 32));
}

static inline int
ble_l2cap_get_first_available_bit(uint32_t *cid_mask)
{
    unsigned int i;
    int bit = 0;

    for (i = 0; i < BLE_HS_CONN_L2CAP_COC_CID_MASK_LEN; i++) {
        /* Find first available index by finding first available bit
         * in the mask.
         * Note:
         * a) If bit == 0 means all the bits are used
         * b) this function returns 1 + index
         */
        bit = __builtin_ffs(~(unsigned int) (cid_mask[i]));
        if (bit != 0) {
            break;
        }
    }

    if (i == BLE_HS_CONN_L2CAP_COC_CID_MASK_LEN) {
        return -1;
    }

    return (i * 32 + bit - 1);
}

static int
ble_l2cap_coc_get_cid(uint32_t *cid_mask)
{
    int bit;

    bit = ble_l2cap_get_first_available_bit(cid_mask);
    if (bit < 0) {
        return -1;
    }

    ble_l2cap_set_used_cid(cid_mask, bit);
    return BLE_L2CAP_COC_CID_START + bit;
}

static struct ble_l2cap_coc_srv *
ble_l2cap_coc_srv_find(uint16_t psm)
{
    struct ble_l2cap_coc_srv *cur, *srv;

    srv = NULL;
    STAILQ_FOREACH(cur, &ble_l2cap_coc_srvs, next) {
        if (cur->psm == psm) {
            srv = cur;
            break;
        }
    }

    return srv;
}

static void
ble_l2cap_event_coc_received_data(struct ble_l2cap_chan *chan,
                                  struct os_mbuf *om)
{
    struct ble_l2cap_event event;

    event.type = BLE_L2CAP_EVENT_COC_DATA_RECEIVED;
    event.receive.conn_handle = chan->conn_handle;
    event.receive.chan = chan;
    event.receive.sdu_rx = om;

    chan->cb(&event, chan->cb_arg);
}

static int
ble_l2cap_coc_rx_fn(struct ble_l2cap_chan *chan)
{
    int rc;
    struct os_mbuf **om;
    struct os_mbuf *rx_sdu;
    struct ble_l2cap_coc_endpoint *rx;
    uint16_t om_total;

    /* Create a shortcut to rx_buf */
    om = &chan->rx_buf;
    BLE_HS_DBG_ASSERT(*om != NULL);

    /* Create a shortcut to rx endpoint */
    rx = &chan->coc_rx;
    BLE_HS_DBG_ASSERT(rx != NULL);

    rx_sdu = rx->sdus[chan->coc_rx.current_sdu_idx];
    BLE_HS_DBG_ASSERT(rx_sdu != NULL);

    om_total = OS_MBUF_PKTLEN(*om);

    /* First LE frame */
    if (OS_MBUF_PKTLEN(rx_sdu) == 0) {
        uint16_t sdu_len;

        rc = ble_hs_mbuf_pullup_base(om, BLE_L2CAP_SDU_SIZE);
        if (rc != 0) {
            return rc;
        }

        sdu_len = get_le16((*om)->om_data);

        BLE_HS_LOG(DEBUG, "First LE frame received %d, SDU len: %d\n",
                   om_total, sdu_len + 2);

        /* We should receive payload of size sdu_len + 2 bytes of sdu_len field */
        if (om_total > sdu_len + 2) {
            BLE_HS_LOG(ERROR, "Payload larger than expected (%d>%d)\n",
                       om_total, sdu_len + 2);
            /* Disconnect peer with invalid behaviour */
            rx_sdu = NULL;
            rx->data_offset = 0;
            ble_l2cap_disconnect(chan);
            return BLE_HS_EBADDATA;
        }
        if (sdu_len > rx->mtu) {
            BLE_HS_LOG(INFO, "error: sdu_len > rx->mtu (%d>%d)\n",
                       sdu_len, rx->mtu);

            /* Disconnect peer with invalid behaviour */
            ble_l2cap_disconnect(chan);
            return BLE_HS_EBADDATA;
        }

        BLE_HS_LOG(DEBUG,
                   "sdu_len=%d, received LE frame=%d, credits=%d, current_sdu_idx=%d\n",
                   sdu_len, om_total, rx->credits, chan->coc_rx.current_sdu_idx);

        os_mbuf_adj(*om, BLE_L2CAP_SDU_SIZE);

        rc = os_mbuf_appendfrom(rx_sdu, *om, 0, om_total - BLE_L2CAP_SDU_SIZE);
        if (rc != 0) {
            /* FIXME: User shall give us big enough buffer.
             * need to handle it better
             */
            BLE_HS_LOG(INFO, "Could not append data rc=%d\n", rc);
            assert(0);
        }

        /* In RX case data_offset keeps incoming SDU len */
        rx->data_offset = sdu_len;

    } else {
        BLE_HS_LOG(DEBUG, "Continuation...received %d\n", (*om)->om_len);

        if (OS_MBUF_PKTLEN(rx_sdu) + (*om)->om_len > rx->data_offset) {
            /* Disconnect peer with invalid behaviour */
            BLE_HS_LOG(ERROR, "Payload larger than expected (%d>%d)\n",
                       OS_MBUF_PKTLEN(rx_sdu) + (*om)->om_len, rx->data_offset);
            rx_sdu = NULL;
            rx->data_offset = 0;
            ble_l2cap_disconnect(chan);
            return BLE_HS_EBADDATA;
        }
        rc = os_mbuf_appendfrom(rx_sdu, *om, 0, om_total);
        if (rc != 0) {
            /* FIXME: need to handle it better */
            BLE_HS_LOG(DEBUG, "Could not append data rc=%d\n", rc);
            assert(0);
        }
    }

    rx->credits--;

    if (OS_MBUF_PKTLEN(rx_sdu) == rx->data_offset) {
        struct os_mbuf *sdu_rx = rx_sdu;

        BLE_HS_LOG(DEBUG, "Received sdu_len=%d, credits left=%d\n",
                   OS_MBUF_PKTLEN(rx_sdu), rx->credits);

        /* Lets get back control to os_mbuf to application.
         * Since it this callback application might want to set new sdu
         * we need to prepare space for this. Therefore we need sdu_rx
         */
        rx_sdu = NULL;
        chan->coc_rx.current_sdu_idx =
            (chan->coc_rx.current_sdu_idx + 1) % BLE_L2CAP_SDU_BUFF_CNT;
        rx->data_offset = 0;

        ble_l2cap_event_coc_received_data(chan, sdu_rx);

        return 0;
    }

    /* If we did not received full SDU and credits are 0 it means
     * that remote was sending us not fully filled up LE frames.
     * However, we still have buffer to for next LE Frame so lets give one more
     * credit to peer so it can send us full SDU
     */
    if (chan->disable_auto_credit_update == false && rx->credits == 0) {
        /* Remote did not send full SDU. Lets give him one more credits to do
         * so since we have still buffer to handle it
         */
        rx->credits = 1;
        ble_l2cap_sig_le_credits(chan->conn_handle, chan->scid, rx->credits);
    }

    BLE_HS_LOG(DEBUG,
               "Received partial sdu_len=%d, credits left=%d, current_sdu_idx=%d\n",
               OS_MBUF_PKTLEN(rx_sdu), rx->credits, chan->coc_rx.current_sdu_idx);

    return 0;
}

void
ble_l2cap_coc_set_new_mtu_mps(struct ble_l2cap_chan *chan, uint16_t mtu,
                              uint16_t mps)
{
    chan->my_coc_mps = mps;
    chan->coc_rx.mtu = mtu;
    chan->initial_credits = mtu / chan->my_coc_mps;
    if (mtu % chan->my_coc_mps) {
        chan->initial_credits++;
    }
}

struct ble_l2cap_chan *
ble_l2cap_coc_chan_alloc(struct ble_hs_conn *conn, uint16_t psm, uint16_t mtu,
                         struct os_mbuf *sdu_rx, ble_l2cap_event_fn *cb,
                         void *cb_arg)
{
    struct ble_l2cap_chan *chan;

    chan = ble_l2cap_chan_alloc(conn->bhc_handle);
    if (!chan) {
        return NULL;
    }

    chan->psm = psm;
    chan->cb = cb;
    chan->cb_arg = cb_arg;
    chan->scid = ble_l2cap_coc_get_cid(conn->l2cap_coc_cid_mask);
    chan->my_coc_mps = MYNEWT_VAL(BLE_L2CAP_COC_MPS);
    chan->rx_fn = ble_l2cap_coc_rx_fn;
    chan->coc_rx.mtu = mtu;
    chan->coc_rx.sdus[0] = sdu_rx;
    for (int i = 1; i < BLE_L2CAP_SDU_BUFF_CNT; i++) {
        chan->coc_rx.sdus[i] = NULL;
    }
    chan->coc_rx.current_sdu_idx = 0;

    if (BLE_L2CAP_SDU_BUFF_CNT == 1) {
        chan->coc_rx.next_sdu_alloc_idx = 0;
    } else {
        chan->coc_rx.next_sdu_alloc_idx = chan->coc_rx.sdus[0] == NULL ? 0 : 1;
    }

    /* Number of credits should allow to send full SDU with on given
     * L2CAP MTU
     */
    chan->coc_rx.credits = mtu / chan->my_coc_mps;
    if (mtu % chan->my_coc_mps) {
        chan->coc_rx.credits++;
    }

    chan->initial_credits = chan->coc_rx.credits;
    return chan;
}

int
ble_l2cap_coc_create_srv_chan(struct ble_hs_conn *conn, uint16_t psm,
                              struct ble_l2cap_chan **chan)
{
    struct ble_l2cap_coc_srv *srv;

    /* Check if there is server registered on this PSM */
    srv = ble_l2cap_coc_srv_find(psm);
    if (!srv) {
        return BLE_HS_ENOTSUP;
    }

    *chan = ble_l2cap_coc_chan_alloc(conn, psm, srv->mtu, NULL, srv->cb,
                                     srv->cb_arg);
    if (!*chan) {
        return BLE_HS_ENOMEM;
    }

    return 0;
}

static void
ble_l2cap_event_coc_disconnected(struct ble_l2cap_chan *chan)
{
    struct ble_l2cap_event event = {};

    /* FIXME */
    if (!chan->cb) {
        return;
    }

    event.type = BLE_L2CAP_EVENT_COC_DISCONNECTED;
    event.disconnect.conn_handle = chan->conn_handle;
    event.disconnect.chan = chan;

    chan->cb(&event, chan->cb_arg);
}

void
ble_l2cap_coc_cleanup_chan(struct ble_hs_conn *conn, struct ble_l2cap_chan *chan)
{
    /* PSM 0 is used for fixed channels. */
    if (chan->psm == 0) {
        return;
    }

    ble_l2cap_event_coc_disconnected(chan);

    if (conn && chan->scid) {
        ble_l2cap_clear_used_cid(conn->l2cap_coc_cid_mask,
                                 chan->scid - BLE_L2CAP_COC_CID_START);
    }

    for (int i = 0; i < BLE_L2CAP_SDU_BUFF_CNT; i++) {
        os_mbuf_free_chain(chan->coc_rx.sdus[i]);
    }
    os_mbuf_free_chain(chan->coc_tx.sdus[0]);
}

static void
ble_l2cap_event_coc_unstalled(struct ble_l2cap_chan *chan, int status)
{
    struct ble_l2cap_event event = {};

    if (!chan->cb) {
        return;
    }

    event.type = BLE_L2CAP_EVENT_COC_TX_UNSTALLED;
    event.tx_unstalled.conn_handle = chan->conn_handle;
    event.tx_unstalled.chan = chan;
    event.tx_unstalled.status = status;

    chan->cb(&event, chan->cb_arg);
}

/* WARNING: this function is called from different task contexts. We expect the
 * host to be locked (ble_hs_lock()) before entering this function! */
static int
ble_l2cap_coc_continue_tx(struct ble_l2cap_chan *chan)
{
    struct ble_l2cap_coc_endpoint *tx;
    uint16_t len;
    uint16_t left_to_send;
    struct os_mbuf *txom;
    struct ble_hs_conn *conn;
    uint16_t sdu_size_offset;
    int rc;

    /* If there is no data to send, just return success */
    tx = &chan->coc_tx;
    if (!tx->sdus[0]) {
        ble_hs_unlock();
        return 0;
    }

    while (tx->credits) {
        sdu_size_offset = 0;

        BLE_HS_LOG(DEBUG, "Available credits %d\n", tx->credits);

        /* lets calculate data we are going to send */
        left_to_send = OS_MBUF_PKTLEN(tx->sdus[0]) - tx->data_offset;

        if (tx->data_offset == 0) {
            sdu_size_offset = BLE_L2CAP_SDU_SIZE;
            left_to_send += sdu_size_offset;
        }

        /* Take into account peer MTU */
        len = min(left_to_send, chan->peer_coc_mps);

        /* Prepare packet */
        txom = ble_hs_mbuf_l2cap_pkt();
        if (!txom) {
            BLE_HS_LOG(DEBUG, "Could not prepare l2cap packet len %d", len);
            rc = BLE_HS_ENOMEM;
            goto failed;
        }

        if (tx->data_offset == 0) {
            /* First packet needs SDU len first. Left to send */
            uint16_t l = htole16(OS_MBUF_PKTLEN(tx->sdus[0]));

            BLE_HS_LOG(DEBUG, "Sending SDU len=%d\n",
                       OS_MBUF_PKTLEN(tx->sdus[0]));
            rc = os_mbuf_append(txom, &l, sizeof(uint16_t));
            if (rc) {
                rc = BLE_HS_ENOMEM;
                BLE_HS_LOG(DEBUG, "Could not append data rc=%d", rc);
                goto failed;
            }
        }

        /* In data_offset we keep track on what we already sent. Need to remember
         * that for first packet we need to decrease data size by 2 bytes for sdu
         * size
         */
        rc = os_mbuf_appendfrom(txom, tx->sdus[0], tx->data_offset,
                                len - sdu_size_offset);
        if (rc) {
            rc = BLE_HS_ENOMEM;
            BLE_HS_LOG(DEBUG, "Could not append data rc=%d", rc);
            goto failed;
        }

        conn = ble_hs_conn_find_assert(chan->conn_handle);
        rc = ble_l2cap_tx(conn, chan, txom);

        if (rc) {
            /* txom is consumed by l2cap */
            txom = NULL;
            goto failed;
        } else {
            tx->credits--;
            tx->data_offset += len - sdu_size_offset;
        }

        BLE_HS_LOG(DEBUG, "Sent %d bytes, credits=%d, to send %d bytes \n",
                   len, tx->credits,
                   OS_MBUF_PKTLEN(tx->sdus[0]) - tx->data_offset);

        if (tx->data_offset == OS_MBUF_PKTLEN(tx->sdus[0])) {
            BLE_HS_LOG(DEBUG, "Complete package sent\n");
            os_mbuf_free_chain(tx->sdus[0]);
            tx->sdus[0] = NULL;
            tx->data_offset = 0;
            break;
        }
    }

    if (tx->sdus[0]) {
        /* Not complete SDU sent, wait for credits */
        tx->flags |= BLE_L2CAP_COC_FLAG_STALLED;
        ble_hs_unlock();
        return BLE_HS_ESTALLED;
    }

    if (tx->flags & BLE_L2CAP_COC_FLAG_STALLED) {
        tx->flags &= ~BLE_L2CAP_COC_FLAG_STALLED;
        ble_hs_unlock();
        ble_l2cap_event_coc_unstalled(chan, 0);
    } else {
        ble_hs_unlock();
    }

    return 0;

failed:
    os_mbuf_free_chain(tx->sdus[0]);
    tx->sdus[0] = NULL;

    os_mbuf_free_chain(txom);
    if (tx->flags & BLE_L2CAP_COC_FLAG_STALLED) {
        tx->flags &= ~BLE_L2CAP_COC_FLAG_STALLED;
        ble_hs_unlock();
        ble_l2cap_event_coc_unstalled(chan, rc);
    } else {
        ble_hs_unlock();
    }

    return rc;
}

void
ble_l2cap_coc_le_credits_update(uint16_t conn_handle, uint16_t dcid,
                                uint16_t credits)
{
    struct ble_hs_conn *conn;
    struct ble_l2cap_chan *chan;

    /* remote updated its credits */
    ble_hs_lock();
    conn = ble_hs_conn_find(conn_handle);
    if (!conn) {
        ble_hs_unlock();
        return;
    }

    chan = ble_hs_conn_chan_find_by_dcid(conn, dcid);
    if (!chan) {
        ble_hs_unlock();
        return;
    }

    if (chan->coc_tx.credits + credits > 0xFFFF) {
        BLE_HS_LOG(INFO, "LE CoC credits overflow...disconnecting\n");
        ble_hs_unlock();
        ble_l2cap_sig_disconnect(chan);
        return;
    }

    chan->coc_tx.credits += credits;

    /* leave the host locked on purpose when ble_l2cap_coc_continue_tx() */
    ble_l2cap_coc_continue_tx(chan);
}

int
ble_l2cap_coc_recv_ready(struct ble_l2cap_chan *chan, struct os_mbuf *sdu_rx)
{
    struct ble_hs_conn *conn;
    struct ble_l2cap_chan *c;

    if (!sdu_rx) {
        return BLE_HS_EINVAL;
    }

    if (chan->coc_rx.sdus[0] != NULL &&
        chan->coc_rx.next_sdu_alloc_idx == chan->coc_rx.current_sdu_idx &&
        BLE_L2CAP_SDU_BUFF_CNT != 1) {
        return BLE_HS_EBUSY;
    }

    chan->coc_rx.sdus[chan->coc_rx.next_sdu_alloc_idx] = sdu_rx;
    chan->coc_rx.next_sdu_alloc_idx =
        (chan->coc_rx.next_sdu_alloc_idx + 1) % BLE_L2CAP_SDU_BUFF_CNT;

    ble_hs_lock();
    conn = ble_hs_conn_find_assert(chan->conn_handle);
    c = ble_hs_conn_chan_find_by_scid(conn, chan->scid);
    if (!c) {
        ble_hs_unlock();
        return BLE_HS_ENOENT;
    }

    /* We want to back only that much credits which remote side is missing
     * to be able to send complete SDU.
     */
    if (chan->disable_auto_credit_update == false && chan->coc_rx.credits < c->initial_credits) {
        ble_hs_unlock();
        ble_l2cap_sig_le_credits(chan->conn_handle, chan->scid,
                                 c->initial_credits - chan->coc_rx.credits);
        ble_hs_lock();
        chan->coc_rx.credits = c->initial_credits;
    }

    ble_hs_unlock();

    return 0;
}

/**
 * Transmits a packet over a connection-oriented channel.  This function only
 * consumes the supplied mbuf on success.
 */
int
ble_l2cap_coc_send(struct ble_l2cap_chan *chan, struct os_mbuf *sdu_tx)
{
    struct ble_l2cap_coc_endpoint *tx;


    tx = &chan->coc_tx;

    if (OS_MBUF_PKTLEN(sdu_tx) > tx->mtu) {
        return BLE_HS_EBADDATA;
    }

    ble_hs_lock();
    if (tx->sdus[0]) {
        ble_hs_unlock();
        return BLE_HS_EBUSY;
    }
    tx->sdus[0] = sdu_tx;


    /* leave the host locked on purpose when ble_l2cap_coc_continue_tx() */
    return ble_l2cap_coc_continue_tx(chan);
}

int
ble_l2cap_coc_init(void)
{
    STAILQ_INIT(&ble_l2cap_coc_srvs);

    return os_mempool_init(&ble_l2cap_coc_srv_pool,
                           MYNEWT_VAL(BLE_L2CAP_COC_MAX_NUM),
                           sizeof(struct ble_l2cap_coc_srv),
                           ble_l2cap_coc_srv_mem,
                           "ble_l2cap_coc_srv_pool");
}

#endif
