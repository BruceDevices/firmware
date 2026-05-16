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

#include <stddef.h>
#include <errno.h>
#include "ble_hs_priv.h"
#include "nimble/nimble/host/include/host/ble_att.h"

#if NIMBLE_BLE_CONNECT

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

static uint16_t ble_att_preferred_mtu_val;

/** Dispatch table for incoming ATT requests.  Sorted by op code. */
typedef int ble_att_rx_fn(uint16_t conn_handle, uint16_t cid, struct os_mbuf **om);
struct ble_att_rx_dispatch_entry {
    uint8_t bde_op;
    ble_att_rx_fn *bde_fn;
};

/** Dispatch table for incoming ATT commands.  Must be ordered by op code. */
static const struct ble_att_rx_dispatch_entry ble_att_rx_dispatch[] = {
    { BLE_ATT_OP_ERROR_RSP,            ble_att_clt_rx_error },
    { BLE_ATT_OP_MTU_REQ,              ble_att_svr_rx_mtu },
    { BLE_ATT_OP_MTU_RSP,              ble_att_clt_rx_mtu },
    { BLE_ATT_OP_FIND_INFO_REQ,        ble_att_svr_rx_find_info },
    { BLE_ATT_OP_FIND_INFO_RSP,        ble_att_clt_rx_find_info },
    { BLE_ATT_OP_FIND_TYPE_VALUE_REQ,  ble_att_svr_rx_find_type_value },
    { BLE_ATT_OP_FIND_TYPE_VALUE_RSP,  ble_att_clt_rx_find_type_value },
    { BLE_ATT_OP_READ_TYPE_REQ,        ble_att_svr_rx_read_type },
    { BLE_ATT_OP_READ_TYPE_RSP,        ble_att_clt_rx_read_type },
    { BLE_ATT_OP_READ_REQ,             ble_att_svr_rx_read },
    { BLE_ATT_OP_READ_RSP,             ble_att_clt_rx_read },
    { BLE_ATT_OP_READ_BLOB_REQ,        ble_att_svr_rx_read_blob },
    { BLE_ATT_OP_READ_BLOB_RSP,        ble_att_clt_rx_read_blob },
    { BLE_ATT_OP_READ_MULT_REQ,        ble_att_svr_rx_read_mult },
    { BLE_ATT_OP_READ_MULT_RSP,        ble_att_clt_rx_read_mult },
    { BLE_ATT_OP_READ_GROUP_TYPE_REQ,  ble_att_svr_rx_read_group_type },
    { BLE_ATT_OP_READ_GROUP_TYPE_RSP,  ble_att_clt_rx_read_group_type },
    { BLE_ATT_OP_WRITE_REQ,            ble_att_svr_rx_write },
    { BLE_ATT_OP_WRITE_RSP,            ble_att_clt_rx_write },
    { BLE_ATT_OP_PREP_WRITE_REQ,       ble_att_svr_rx_prep_write },
    { BLE_ATT_OP_PREP_WRITE_RSP,       ble_att_clt_rx_prep_write },
    { BLE_ATT_OP_EXEC_WRITE_REQ,       ble_att_svr_rx_exec_write },
    { BLE_ATT_OP_EXEC_WRITE_RSP,       ble_att_clt_rx_exec_write },
    { BLE_ATT_OP_NOTIFY_REQ,           ble_att_svr_rx_notify },
    { BLE_ATT_OP_INDICATE_REQ,         ble_att_svr_rx_indicate },
    { BLE_ATT_OP_INDICATE_RSP,         ble_att_clt_rx_indicate },
    { BLE_ATT_OP_READ_MULT_VAR_REQ,    ble_att_svr_rx_read_mult_var },
    { BLE_ATT_OP_READ_MULT_VAR_RSP,    ble_att_clt_rx_read_mult_var },
    { BLE_ATT_OP_NOTIFY_MULTI_REQ,     ble_att_svr_rx_notify_multi },
    { BLE_ATT_OP_WRITE_CMD,            ble_att_svr_rx_write_no_rsp },
    { BLE_ATT_OP_SIGNED_WRITE_CMD,     ble_att_svr_rx_signed_write },
};

#define BLE_ATT_RX_DISPATCH_SZ \
    (sizeof ble_att_rx_dispatch / sizeof ble_att_rx_dispatch[0])

STATS_SECT_DECL(ble_att_stats) ble_att_stats;
STATS_NAME_START(ble_att_stats)
    STATS_NAME(ble_att_stats, error_rsp_rx)
    STATS_NAME(ble_att_stats, error_rsp_tx)
    STATS_NAME(ble_att_stats, mtu_req_rx)
    STATS_NAME(ble_att_stats, mtu_req_tx)
    STATS_NAME(ble_att_stats, mtu_rsp_rx)
    STATS_NAME(ble_att_stats, mtu_rsp_tx)
    STATS_NAME(ble_att_stats, find_info_req_rx)
    STATS_NAME(ble_att_stats, find_info_req_tx)
    STATS_NAME(ble_att_stats, find_info_rsp_rx)
    STATS_NAME(ble_att_stats, find_info_rsp_tx)
    STATS_NAME(ble_att_stats, find_type_value_req_rx)
    STATS_NAME(ble_att_stats, find_type_value_req_tx)
    STATS_NAME(ble_att_stats, find_type_value_rsp_rx)
    STATS_NAME(ble_att_stats, find_type_value_rsp_tx)
    STATS_NAME(ble_att_stats, read_type_req_rx)
    STATS_NAME(ble_att_stats, read_type_req_tx)
    STATS_NAME(ble_att_stats, read_type_rsp_rx)
    STATS_NAME(ble_att_stats, read_type_rsp_tx)
    STATS_NAME(ble_att_stats, read_req_rx)
    STATS_NAME(ble_att_stats, read_req_tx)
    STATS_NAME(ble_att_stats, read_rsp_rx)
    STATS_NAME(ble_att_stats, read_rsp_tx)
    STATS_NAME(ble_att_stats, read_blob_req_rx)
    STATS_NAME(ble_att_stats, read_blob_req_tx)
    STATS_NAME(ble_att_stats, read_blob_rsp_rx)
    STATS_NAME(ble_att_stats, read_blob_rsp_tx)
    STATS_NAME(ble_att_stats, read_mult_req_rx)
    STATS_NAME(ble_att_stats, read_mult_req_tx)
    STATS_NAME(ble_att_stats, read_mult_rsp_rx)
    STATS_NAME(ble_att_stats, read_mult_rsp_tx)
    STATS_NAME(ble_att_stats, read_group_type_req_rx)
    STATS_NAME(ble_att_stats, read_group_type_req_tx)
    STATS_NAME(ble_att_stats, read_group_type_rsp_rx)
    STATS_NAME(ble_att_stats, read_group_type_rsp_tx)
    STATS_NAME(ble_att_stats, write_req_rx)
    STATS_NAME(ble_att_stats, write_req_tx)
    STATS_NAME(ble_att_stats, write_rsp_rx)
    STATS_NAME(ble_att_stats, write_rsp_tx)
    STATS_NAME(ble_att_stats, prep_write_req_rx)
    STATS_NAME(ble_att_stats, prep_write_req_tx)
    STATS_NAME(ble_att_stats, prep_write_rsp_rx)
    STATS_NAME(ble_att_stats, prep_write_rsp_tx)
    STATS_NAME(ble_att_stats, exec_write_req_rx)
    STATS_NAME(ble_att_stats, exec_write_req_tx)
    STATS_NAME(ble_att_stats, exec_write_rsp_rx)
    STATS_NAME(ble_att_stats, exec_write_rsp_tx)
    STATS_NAME(ble_att_stats, notify_req_rx)
    STATS_NAME(ble_att_stats, notify_req_tx)
    STATS_NAME(ble_att_stats, indicate_req_rx)
    STATS_NAME(ble_att_stats, indicate_req_tx)
    STATS_NAME(ble_att_stats, indicate_rsp_rx)
    STATS_NAME(ble_att_stats, indicate_rsp_tx)
    STATS_NAME(ble_att_stats, multi_notify_req_rx)
    STATS_NAME(ble_att_stats, multi_notify_req_tx)
    STATS_NAME(ble_att_stats, write_cmd_rx)
    STATS_NAME(ble_att_stats, write_cmd_tx)
STATS_NAME_END(ble_att_stats)

static const struct ble_att_rx_dispatch_entry *
ble_att_rx_dispatch_entry_find(uint8_t op)
{
    const struct ble_att_rx_dispatch_entry *entry;
    unsigned int i;

    for (i = 0; i < BLE_ATT_RX_DISPATCH_SZ; i++) {
        entry = ble_att_rx_dispatch + i;
        if (entry->bde_op == op) {
            return entry;
        }

        if (entry->bde_op > op) {
            break;
        }
    }

    return NULL;
}

int
ble_att_conn_chan_find(uint16_t conn_handle, uint16_t cid, struct ble_hs_conn **out_conn,
                       struct ble_l2cap_chan **out_chan)
{
    return ble_hs_misc_conn_chan_find(conn_handle, cid,
                                      out_conn, out_chan);
}

int
ble_att_conn_chan_find_by_psm(uint16_t conn_handle, uint16_t psm,
                              struct ble_hs_conn **out_conn,
                              struct ble_l2cap_chan **out_chan)
{
    return ble_hs_misc_conn_chan_find(conn_handle, psm, out_conn, out_chan);
}

void
ble_att_inc_tx_stat(uint8_t att_op)
{
    switch (att_op) {
    case BLE_ATT_OP_ERROR_RSP:
        STATS_INC(ble_att_stats, error_rsp_tx);
        break;

    case BLE_ATT_OP_MTU_REQ:
        STATS_INC(ble_att_stats, mtu_req_tx);
        break;

    case BLE_ATT_OP_MTU_RSP:
        STATS_INC(ble_att_stats, mtu_rsp_tx);
        break;

    case BLE_ATT_OP_FIND_INFO_REQ:
        STATS_INC(ble_att_stats, find_info_req_tx);
        break;

    case BLE_ATT_OP_FIND_INFO_RSP:
        STATS_INC(ble_att_stats, find_info_rsp_tx);
        break;

    case BLE_ATT_OP_FIND_TYPE_VALUE_REQ:
        STATS_INC(ble_att_stats, find_type_value_req_tx);
        break;

    case BLE_ATT_OP_FIND_TYPE_VALUE_RSP:
        STATS_INC(ble_att_stats, find_type_value_rsp_tx);
        break;

    case BLE_ATT_OP_READ_TYPE_REQ:
        STATS_INC(ble_att_stats, read_type_req_tx);
        break;

    case BLE_ATT_OP_READ_TYPE_RSP:
        STATS_INC(ble_att_stats, read_type_rsp_tx);
        break;

    case BLE_ATT_OP_READ_REQ:
        STATS_INC(ble_att_stats, read_req_tx);
        break;

    case BLE_ATT_OP_READ_RSP:
        STATS_INC(ble_att_stats, read_rsp_tx);
        break;

    case BLE_ATT_OP_READ_BLOB_REQ:
        STATS_INC(ble_att_stats, read_blob_req_tx);
        break;

    case BLE_ATT_OP_READ_BLOB_RSP:
        STATS_INC(ble_att_stats, read_blob_rsp_tx);
        break;

    case BLE_ATT_OP_READ_MULT_REQ:
        STATS_INC(ble_att_stats, read_mult_req_tx);
        break;

    case BLE_ATT_OP_READ_MULT_RSP:
        STATS_INC(ble_att_stats, read_mult_rsp_tx);
        break;

    case BLE_ATT_OP_READ_GROUP_TYPE_REQ:
        STATS_INC(ble_att_stats, read_group_type_req_tx);
        break;

    case BLE_ATT_OP_READ_GROUP_TYPE_RSP:
        STATS_INC(ble_att_stats, read_group_type_rsp_tx);
        break;

    case BLE_ATT_OP_WRITE_REQ:
        STATS_INC(ble_att_stats, write_req_tx);
        break;

    case BLE_ATT_OP_WRITE_RSP:
        STATS_INC(ble_att_stats, write_rsp_tx);
        break;

    case BLE_ATT_OP_PREP_WRITE_REQ:
        STATS_INC(ble_att_stats, prep_write_req_tx);
        break;

    case BLE_ATT_OP_PREP_WRITE_RSP:
        STATS_INC(ble_att_stats, prep_write_rsp_tx);
        break;

    case BLE_ATT_OP_EXEC_WRITE_REQ:
        STATS_INC(ble_att_stats, exec_write_req_tx);
        break;

    case BLE_ATT_OP_EXEC_WRITE_RSP:
        STATS_INC(ble_att_stats, exec_write_rsp_tx);
        break;

    case BLE_ATT_OP_NOTIFY_REQ:
        STATS_INC(ble_att_stats, notify_req_tx);
        break;

    case BLE_ATT_OP_INDICATE_REQ:
        STATS_INC(ble_att_stats, indicate_req_tx);
        break;

    case BLE_ATT_OP_INDICATE_RSP:
        STATS_INC(ble_att_stats, indicate_rsp_tx);
        break;

    case BLE_ATT_OP_NOTIFY_MULTI_REQ:
        STATS_INC(ble_att_stats, multi_notify_req_tx);
        break;

    case BLE_ATT_OP_WRITE_CMD:
        STATS_INC(ble_att_stats, write_cmd_tx);
        break;

    default:
        break;
    }
}

static void
ble_att_inc_rx_stat(uint8_t att_op)
{
    switch (att_op) {
    case BLE_ATT_OP_ERROR_RSP:
        STATS_INC(ble_att_stats, error_rsp_rx);
        break;

    case BLE_ATT_OP_MTU_REQ:
        STATS_INC(ble_att_stats, mtu_req_rx);
        break;

    case BLE_ATT_OP_MTU_RSP:
        STATS_INC(ble_att_stats, mtu_rsp_rx);
        break;

    case BLE_ATT_OP_FIND_INFO_REQ:
        STATS_INC(ble_att_stats, find_info_req_rx);
        break;

    case BLE_ATT_OP_FIND_INFO_RSP:
        STATS_INC(ble_att_stats, find_info_rsp_rx);
        break;

    case BLE_ATT_OP_FIND_TYPE_VALUE_REQ:
        STATS_INC(ble_att_stats, find_type_value_req_rx);
        break;

    case BLE_ATT_OP_FIND_TYPE_VALUE_RSP:
        STATS_INC(ble_att_stats, find_type_value_rsp_rx);
        break;

    case BLE_ATT_OP_READ_TYPE_REQ:
        STATS_INC(ble_att_stats, read_type_req_rx);
        break;

    case BLE_ATT_OP_READ_TYPE_RSP:
        STATS_INC(ble_att_stats, read_type_rsp_rx);
        break;

    case BLE_ATT_OP_READ_REQ:
        STATS_INC(ble_att_stats, read_req_rx);
        break;

    case BLE_ATT_OP_READ_RSP:
        STATS_INC(ble_att_stats, read_rsp_rx);
        break;

    case BLE_ATT_OP_READ_BLOB_REQ:
        STATS_INC(ble_att_stats, read_blob_req_rx);
        break;

    case BLE_ATT_OP_READ_BLOB_RSP:
        STATS_INC(ble_att_stats, read_blob_rsp_rx);
        break;

    case BLE_ATT_OP_READ_MULT_REQ:
        STATS_INC(ble_att_stats, read_mult_req_rx);
        break;

    case BLE_ATT_OP_READ_MULT_RSP:
        STATS_INC(ble_att_stats, read_mult_rsp_rx);
        break;

    case BLE_ATT_OP_READ_GROUP_TYPE_REQ:
        STATS_INC(ble_att_stats, read_group_type_req_rx);
        break;

    case BLE_ATT_OP_READ_GROUP_TYPE_RSP:
        STATS_INC(ble_att_stats, read_group_type_rsp_rx);
        break;

    case BLE_ATT_OP_WRITE_REQ:
        STATS_INC(ble_att_stats, write_req_rx);
        break;

    case BLE_ATT_OP_WRITE_RSP:
        STATS_INC(ble_att_stats, write_rsp_rx);
        break;

    case BLE_ATT_OP_PREP_WRITE_REQ:
        STATS_INC(ble_att_stats, prep_write_req_rx);
        break;

    case BLE_ATT_OP_PREP_WRITE_RSP:
        STATS_INC(ble_att_stats, prep_write_rsp_rx);
        break;

    case BLE_ATT_OP_EXEC_WRITE_REQ:
        STATS_INC(ble_att_stats, exec_write_req_rx);
        break;

    case BLE_ATT_OP_EXEC_WRITE_RSP:
        STATS_INC(ble_att_stats, exec_write_rsp_rx);
        break;

    case BLE_ATT_OP_NOTIFY_REQ:
        STATS_INC(ble_att_stats, notify_req_rx);
        break;

    case BLE_ATT_OP_INDICATE_REQ:
        STATS_INC(ble_att_stats, indicate_req_rx);
        break;

    case BLE_ATT_OP_INDICATE_RSP:
        STATS_INC(ble_att_stats, indicate_rsp_rx);
        break;

    case BLE_ATT_OP_NOTIFY_MULTI_REQ:
        STATS_INC(ble_att_stats, multi_notify_req_rx);
        break;

    case BLE_ATT_OP_WRITE_CMD:
        STATS_INC(ble_att_stats, write_cmd_rx);
        break;

    default:
        break;
    }
}

void
ble_att_truncate_to_mtu(const struct ble_l2cap_chan *att_chan,
                        struct os_mbuf *txom)
{
    int32_t extra_len;
    uint16_t mtu;

    mtu = ble_att_chan_mtu(att_chan);
    extra_len = OS_MBUF_PKTLEN(txom) - mtu;
    if (extra_len > 0) {
        os_mbuf_adj(txom, -extra_len);
    }
}

uint16_t
ble_att_mtu_by_cid(uint16_t conn_handle, uint16_t cid)
{
    struct ble_l2cap_chan *chan;
    struct ble_hs_conn *conn;
    uint16_t mtu;
    int rc;

    ble_hs_lock();

    rc = ble_att_conn_chan_find(conn_handle, cid, &conn, &chan);
    if (rc == 0) {
        mtu = ble_att_chan_mtu(chan);
    } else {
        mtu = 0;
    }

    ble_hs_unlock();

    return mtu;
}

uint16_t
ble_att_mtu(uint16_t conn_handle)
{
    return ble_att_mtu_by_cid(conn_handle, BLE_L2CAP_CID_ATT);
}

void
ble_att_set_peer_mtu(struct ble_l2cap_chan *chan, uint16_t peer_mtu)
{
    if (peer_mtu < BLE_ATT_MTU_DFLT) {
        peer_mtu = BLE_ATT_MTU_DFLT;
    }

    chan->peer_mtu = peer_mtu;
}

uint16_t
ble_att_chan_mtu(const struct ble_l2cap_chan *chan)
{
    uint16_t mtu;

#if MYNEWT_VAL(BLE_EATT_CHAN_NUM) > 0
    if (ble_hs_cfg.eatt && chan->psm == BLE_EATT_PSM) {
        /* The ATT_MTU for the Enhanced ATT bearer shall be set to the minimum of the
         * MTU field values of the two devices. Reference:
         * Core v5.4 Vol 3 Part G 5.3.1 ATT_MTU
         */
        return min(chan->coc_tx.mtu, chan->coc_rx.mtu);
    }
#endif

    /* If either side has not exchanged MTU size, use the default.  Otherwise,
     * use the lesser of the two exchanged values.
     */
    if (!(ble_l2cap_is_mtu_req_sent(chan)) ||
        chan->peer_mtu == 0) {

        mtu = BLE_ATT_MTU_DFLT;
    } else {
        mtu = min(chan->my_mtu, chan->peer_mtu);
    }

    BLE_HS_DBG_ASSERT(mtu >= BLE_ATT_MTU_DFLT);

    return mtu;
}

static void
ble_att_rx_handle_unknown_request(uint8_t op, uint16_t conn_handle,
                                  uint16_t cid, struct os_mbuf **om)
{
    /* If this is command (bit6 is set to 1), do nothing */
    if (op & 0x40) {
        return;
    }

    os_mbuf_adj(*om, OS_MBUF_PKTLEN(*om));
    ble_att_svr_tx_error_rsp(conn_handle, cid, *om, op, 0,
                             BLE_ATT_ERR_REQ_NOT_SUPPORTED);

    *om = NULL;
}

static void
ble_att_send_outstanding_after_response(uint16_t conn_handle)
{
    struct ble_hs_conn *conn;
    struct ble_l2cap_chan *chan;
    int rc;

    ble_hs_lock();
    rc = ble_hs_misc_conn_chan_find_reqd(conn_handle, BLE_L2CAP_CID_ATT, &conn,
                                         &chan);
    if (rc) {
        return;
    }
    conn->client_att_busy = false;
    ble_att_tx_with_conn(conn, chan, NULL);
    ble_hs_unlock();
}

static int
ble_att_rx_extended(uint16_t conn_handle, uint16_t cid, struct os_mbuf **om)
{
    const struct ble_att_rx_dispatch_entry *entry;
    uint8_t op;
    int rc;

    BLE_HS_DBG_ASSERT(*om != NULL);

    rc = os_mbuf_copydata(*om, 0, 1, &op);
    if (rc != 0) {
        return BLE_HS_EMSGSIZE;
    }

    if (cid == BLE_L2CAP_CID_ATT && ble_att_is_response_op(op)) {
        ble_att_send_outstanding_after_response(conn_handle);
    }

    entry = ble_att_rx_dispatch_entry_find(op);
    if (entry == NULL) {
        ble_att_rx_handle_unknown_request(op, conn_handle, cid, om);
        return BLE_HS_ENOTSUP;
    }

    ble_att_inc_rx_stat(op);

    /* Strip L2CAP ATT header from the front of the mbuf. */
    os_mbuf_adj(*om, 1);

    rc = entry->bde_fn(conn_handle, cid, om);
    if (rc != 0) {
        if (rc == BLE_HS_ENOTSUP) {
            ble_att_rx_handle_unknown_request(op, conn_handle, cid, om);
        }
        return rc;
    }

    return 0;
}

static int
ble_att_rx(struct ble_l2cap_chan *chan)
{
    uint16_t conn_handle;

    conn_handle = ble_l2cap_get_conn_handle(chan);
    if (conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        return BLE_HS_ENOTCONN;
    }

    return ble_att_rx_extended(conn_handle, chan->scid, &chan->rx_buf);
}

uint16_t
ble_att_preferred_mtu(void)
{
    return ble_att_preferred_mtu_val;
}

int
ble_att_set_preferred_mtu(uint16_t mtu)
{
    struct ble_l2cap_chan *chan;
    struct ble_hs_conn *conn;
    int i;

    if (mtu < BLE_ATT_MTU_DFLT) {
        return BLE_HS_EINVAL;
    }
    if (mtu > BLE_ATT_MTU_MAX) {
        return BLE_HS_EINVAL;
    }

    ble_att_preferred_mtu_val = mtu;

    /* Set my_mtu for established connections that haven't exchanged. */
    ble_hs_lock();

    i = 0;
    while ((conn = ble_hs_conn_find_by_idx(i)) != NULL) {
        chan = ble_hs_conn_chan_find_by_scid(conn, BLE_L2CAP_CID_ATT);
        BLE_HS_DBG_ASSERT(chan != NULL);

        if (!(chan->flags & BLE_L2CAP_CHAN_F_TXED_MTU)) {
            chan->my_mtu = mtu;
        }

        i++;
    }

    ble_hs_unlock();

    return 0;
}

struct ble_l2cap_chan *
ble_att_create_chan(uint16_t conn_handle)
{
    struct ble_l2cap_chan *chan;

    chan = ble_l2cap_chan_alloc(conn_handle);
    if (chan == NULL) {
        return NULL;
    }

    chan->scid = BLE_L2CAP_CID_ATT;
    chan->dcid = BLE_L2CAP_CID_ATT;
    chan->my_mtu = ble_att_preferred_mtu_val;
    chan->rx_fn = ble_att_rx;

    return chan;
}

bool
ble_att_is_request_op(uint8_t opcode)
{
    switch (opcode) {
    case BLE_ATT_OP_MTU_REQ:
    case BLE_ATT_OP_FIND_INFO_REQ:
    case BLE_ATT_OP_FIND_TYPE_VALUE_REQ:
    case BLE_ATT_OP_READ_TYPE_REQ:
    case BLE_ATT_OP_READ_REQ:
    case BLE_ATT_OP_READ_BLOB_REQ:
    case BLE_ATT_OP_READ_MULT_REQ:
    case BLE_ATT_OP_READ_GROUP_TYPE_REQ:
    case BLE_ATT_OP_WRITE_REQ:
    case BLE_ATT_OP_PREP_WRITE_REQ:
    case BLE_ATT_OP_EXEC_WRITE_REQ:
    case BLE_ATT_OP_INDICATE_REQ:
    case BLE_ATT_OP_READ_MULT_VAR_REQ:
        return true;
    }
    return false;
}

bool
ble_att_is_response_op(uint8_t opcode)
{
    switch (opcode) {
    case BLE_ATT_OP_MTU_RSP:
    case BLE_ATT_OP_ERROR_RSP:
    case BLE_ATT_OP_FIND_INFO_RSP:
    case BLE_ATT_OP_FIND_TYPE_VALUE_RSP:
    case BLE_ATT_OP_READ_TYPE_RSP:
    case BLE_ATT_OP_INDICATE_RSP:
    case BLE_ATT_OP_READ_RSP:
    case BLE_ATT_OP_READ_BLOB_RSP:
    case BLE_ATT_OP_READ_MULT_RSP:
    case BLE_ATT_OP_READ_GROUP_TYPE_RSP:
    case BLE_ATT_OP_WRITE_RSP:
    case BLE_ATT_OP_PREP_WRITE_RSP:
    case BLE_ATT_OP_EXEC_WRITE_RSP:
    case BLE_ATT_OP_READ_MULT_VAR_RSP:
        return true;
    }

    return false;
}

bool
ble_att_is_att_pdu_op(uint8_t opcode)
{
    if (ble_att_is_request_op(opcode)) {
        return true;
    }

    if (ble_att_is_response_op(opcode)) {
        return true;
    }

    switch (opcode) {
    case BLE_ATT_OP_NOTIFY_MULTI_REQ:
    case BLE_ATT_OP_WRITE_CMD:
    case BLE_ATT_OP_SIGNED_WRITE_CMD:
    case BLE_ATT_OP_NOTIFY_REQ:
        return true;
    }
    return false;
}

int
ble_att_set_default_bearer_using_cid(uint16_t conn_handle, uint16_t cid) {
#if MYNEWT_VAL(BLE_EATT_CHAN_NUM) > 0
    struct ble_hs_conn * conn;

    ble_hs_lock();
    conn = ble_hs_conn_find(conn_handle);
    ble_hs_unlock();

    if (conn == NULL) {
        return BLE_HS_ENOTCONN;
    }
    conn->default_cid = cid;
    return 0;
#endif
    return BLE_HS_ENOTSUP;
}

uint16_t
ble_att_get_default_bearer_cid(uint16_t conn_handle) {
#if MYNEWT_VAL(BLE_EATT_CHAN_NUM) > 0
    struct ble_hs_conn * conn;

    conn = ble_hs_conn_find(conn_handle);
    if (conn == NULL) {
        return 0;
    }
    return conn->default_cid;
#endif
    return 0;
}

int
ble_att_init(void)
{
    int rc;

    ble_att_preferred_mtu_val = MYNEWT_VAL(BLE_ATT_PREFERRED_MTU);

    rc = stats_init_and_reg(
        STATS_HDR(ble_att_stats), STATS_SIZE_INIT_PARMS(ble_att_stats,
        STATS_SIZE_32), STATS_NAME_INIT_PARMS(ble_att_stats), "ble_att");
    if (rc != 0) {
        return BLE_HS_EOS;
    }

    ble_eatt_init(ble_att_rx_extended);

    return 0;
}

#endif
