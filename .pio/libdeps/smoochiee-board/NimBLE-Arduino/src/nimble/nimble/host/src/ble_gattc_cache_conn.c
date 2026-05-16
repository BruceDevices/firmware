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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "nimble/nimble/host/include/host/ble_hs.h"
#include "ble_hs_priv.h"
#include "ble_gattc_cache_priv.h"

#if MYNEWT_VAL(BLE_GATT_CACHING)
/* Gatt Procedure macros */
#define BLE_GATT_OP_DISC_ALL_SVCS               1
#define BLE_GATT_OP_DISC_SVC_UUID               2
#define BLE_GATT_OP_FIND_INC_SVCS               3
#define BLE_GATT_OP_DISC_ALL_CHRS               4
#define BLE_GATT_OP_DISC_CHR_UUID               5
#define BLE_GATT_OP_DISC_ALL_DSCS               6

#define CHECK_CACHE_CONN_STATE(cache_state, cb, cb_arg, opcode, \
                                s_handle, e_handle, p_uuid) \
    op = &conn->pending_op; \
    switch(cache_state) { \
    case SVC_DISC_IN_PROGRESS: \
        if((void*)ble_gattc_cache_conn_svc_disced == cb) { \
            return BLE_HS_EINVAL; \
        } \
        ble_gattc_cache_conn_fill_op(op, s_handle, e_handle, p_uuid, cb, \
                                     cb_arg, opcode); \
        return 0; \
    case CHR_DISC_IN_PROGRESS: \
       if((void*)ble_gattc_cache_conn_chr_disced == cb) { \
           return BLE_HS_EINVAL; \
       } \
        ble_gattc_cache_conn_fill_op(op, s_handle, e_handle, p_uuid, cb, \
                                     cb_arg, opcode); \
        return 0; \
    case INC_DISC_IN_PROGRESS: \
       if((void *)ble_gattc_cache_conn_inc_disced == cb) { \
           return BLE_HS_EINVAL; \
        } \
        ble_gattc_cache_conn_fill_op(op, s_handle, e_handle, p_uuid, cb, \
                                     cb_arg, opcode); \
        return 0; \
    case DSC_DISC_IN_PROGRESS: \
       if((void*)ble_gattc_cache_conn_dsc_disced == cb) { \
           return BLE_HS_EINVAL; \
       } \
        ble_gattc_cache_conn_fill_op(op, s_handle, e_handle, p_uuid, cb, \
                                     cb_arg, opcode); \
        return 0; \
    case VERIFY_IN_PROGRESS: \
        ble_gattc_cache_conn_fill_op(op, s_handle, e_handle, p_uuid, cb, \
                                     cb_arg, opcode); \
        return 0; \
    break; \
    case CACHE_INVALID: \
        ble_gattc_cache_conn_fill_op(op, s_handle, e_handle, p_uuid, cb, \
                                     cb_arg, opcode); \
        /* start discovery here */ \
        BLE_HS_LOG(INFO, "Cache not valid"); \
        rc = ble_gattc_cache_conn_disc(conn); \
        return rc; \
    case CACHE_VERIFIED: \
        ble_gattc_cache_conn_fill_op(op, s_handle, e_handle, p_uuid, cb, \
                                     cb_arg, opcode); \
        break; \
    }
#define BLE_SVC_GATT_CHR_SERVICE_CHANGED_UUID16     0x2a05
static void *ble_gattc_cache_conn_svc_mem;
static struct os_mempool ble_gattc_cache_conn_svc_pool;

static void *ble_gattc_cache_conn_chr_mem;
static struct os_mempool ble_gattc_cache_conn_chr_pool;

static void *ble_gattc_cache_conn_dsc_mem;
static struct os_mempool ble_gattc_cache_conn_dsc_pool;

static void *ble_gattc_cache_conn_mem;
static struct os_mempool ble_gattc_cache_conn_pool;
static SLIST_HEAD(, ble_gattc_cache_conn) ble_gattc_cache_conns;

static struct ble_gattc_cache_conn_svc *
ble_gattc_cache_conn_svc_find_range(struct ble_gattc_cache_conn *ble_gattc_cache_conn, uint16_t attr_handle);

static struct ble_gattc_cache_conn_svc *
ble_gattc_cache_conn_svc_find(struct ble_gattc_cache_conn *ble_gattc_cache_conn, uint16_t svc_start_handle,
                              struct ble_gattc_cache_conn_svc **out_prev);

int
ble_gattc_cache_conn_svc_is_empty(const struct ble_gattc_cache_conn_svc *svc);

uint16_t
ble_gattc_cache_conn_chr_end_handle(const struct ble_gattc_cache_conn_svc *svc, const struct ble_gattc_cache_conn_chr *chr);

int
ble_gattc_cache_conn_chr_is_empty(const struct ble_gattc_cache_conn_svc *svc, const struct ble_gattc_cache_conn_chr *chr);

static struct ble_gattc_cache_conn_chr *
ble_gattc_cache_conn_chr_find(const struct ble_gattc_cache_conn_svc *svc, uint16_t chr_def_handle,
                              struct ble_gattc_cache_conn_chr **out_prev);

static void
ble_gattc_cache_conn_disc_chrs(struct ble_gattc_cache_conn *ble_gattc_cache_conn);

static void
ble_gattc_cache_conn_disc_incs(struct ble_gattc_cache_conn *ble_gattc_cache_conn);

static void
ble_gattc_cache_conn_disc_dscs(struct ble_gattc_cache_conn *peer);

struct ble_gattc_cache_conn *
ble_gattc_cache_conn_find(uint16_t conn_handle)
{
    struct ble_gattc_cache_conn *ble_gattc_cache_conn;

    SLIST_FOREACH(ble_gattc_cache_conn, &ble_gattc_cache_conns, next) {
        if (ble_gattc_cache_conn->conn_handle == conn_handle) {
            return ble_gattc_cache_conn;
        }
    }

    return NULL;
}

struct ble_gattc_cache_conn *
ble_gattc_cache_conn_find_by_addr(ble_addr_t peer_addr)
{
    struct ble_gattc_cache_conn *ble_gattc_cache_conn;
    SLIST_FOREACH(ble_gattc_cache_conn, &ble_gattc_cache_conns, next) {
        if (memcmp(&ble_gattc_cache_conn->ble_gattc_cache_conn_addr, &peer_addr, sizeof(peer_addr)) == 0) {
            return ble_gattc_cache_conn;
        }
    }
    return NULL;
}

static struct ble_gattc_cache_conn_dsc *
ble_gattc_cache_conn_dsc_find_prev(const struct ble_gattc_cache_conn_chr *chr, uint16_t dsc_handle)
{
    struct ble_gattc_cache_conn_dsc *prev;
    struct ble_gattc_cache_conn_dsc *dsc;

    prev = NULL;
    SLIST_FOREACH(dsc, &chr->dscs, next) {
        if (dsc->dsc.handle >= dsc_handle) {
            break;
        }

        prev = dsc;
    }

    return prev;
}

static struct ble_gattc_cache_conn_dsc *
ble_gattc_cache_conn_dsc_find(const struct ble_gattc_cache_conn_chr *chr, uint16_t dsc_handle,
                              struct ble_gattc_cache_conn_dsc **out_prev)
{
    struct ble_gattc_cache_conn_dsc *prev;
    struct ble_gattc_cache_conn_dsc *dsc;

    prev = ble_gattc_cache_conn_dsc_find_prev(chr, dsc_handle);
    if (prev == NULL) {
        dsc = SLIST_FIRST(&chr->dscs);
    } else {
        dsc = SLIST_NEXT(prev, next);
    }

    if (dsc != NULL && dsc->dsc.handle != dsc_handle) {
        dsc = NULL;
    }

    if (out_prev != NULL) {
        *out_prev = prev;
    }
    return dsc;
}

static struct ble_gattc_cache_conn_chr *
ble_gattc_cache_conn_chr_find_range(const struct ble_gattc_cache_conn_svc *svc, uint16_t dsc_handle)
{
    struct ble_gattc_cache_conn_chr *chr;
    struct ble_gattc_cache_conn_chr *prev = NULL;

    SLIST_FOREACH(chr, &svc->chrs, next) {
        if (chr->chr.val_handle <= (dsc_handle)) {
            prev = chr;
        } else {
            break;
        }
    }
    return prev;
}

int
ble_gattc_cache_conn_dsc_add(ble_addr_t peer_addr, uint16_t chr_val_handle,
                             const struct ble_gatt_dsc *gatt_dsc)
{
    struct ble_gattc_cache_conn_dsc *prev;
    struct ble_gattc_cache_conn_dsc *dsc;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn_chr *chr;
    struct ble_gattc_cache_conn *peer;

    peer = ble_gattc_cache_conn_find_by_addr(peer_addr);
    if (peer == NULL) {
        BLE_HS_LOG(ERROR, "Conn not found");
        return BLE_HS_EUNKNOWN;
    }

    svc = ble_gattc_cache_conn_svc_find_range(peer, gatt_dsc->handle);
    if (svc == NULL) {
        /* Can't find service for discovered descriptor; this shouldn't
         * happen.
         */
        assert(0);
        return BLE_HS_EUNKNOWN;
    }

    if (chr_val_handle == 0) {
        chr = ble_gattc_cache_conn_chr_find_range(svc, gatt_dsc->handle);
    } else {
        chr = ble_gattc_cache_conn_chr_find(svc, chr_val_handle, NULL);
    }

    if (chr == NULL) {
        /* Can't find characteristic for discovered descriptor; this shouldn't
         * happen.
         */
        BLE_HS_LOG(ERROR, "Couldn't find characteristc for dsc handle = %d", gatt_dsc->handle);
        assert(0);
        return BLE_HS_EUNKNOWN;
    }

    dsc = ble_gattc_cache_conn_dsc_find(chr, gatt_dsc->handle, &prev);
    if (dsc != NULL) {
        BLE_HS_LOG(DEBUG, "Descriptor already discovered.");
        /* Descriptor already discovered. */
        return 0;
    }

    dsc = os_memblock_get(&ble_gattc_cache_conn_dsc_pool);
    if (dsc == NULL) {
        /* Out of memory. */
        return BLE_HS_ENOMEM;
    }
    memset(dsc, 0, sizeof * dsc);

    dsc->dsc = *gatt_dsc;

    if (prev == NULL) {
        SLIST_INSERT_HEAD(&chr->dscs, dsc, next);
    } else {
        SLIST_NEXT(prev, next) = dsc;
    }

    BLE_HS_LOG(DEBUG, "Descriptor added with handle = %d", dsc->dsc.handle);

    return 0;
}

uint16_t
ble_gattc_cache_conn_chr_end_handle(const struct ble_gattc_cache_conn_svc *svc, const struct ble_gattc_cache_conn_chr *chr)
{
    const struct ble_gattc_cache_conn_chr *next_chr;

    next_chr = SLIST_NEXT(chr, next);
    if (next_chr != NULL) {
        return next_chr->chr.def_handle - 1;
    } else {
        return svc->svc.end_handle;
    }
}

int
ble_gattc_cache_conn_chr_is_empty(const struct ble_gattc_cache_conn_svc *svc, const struct ble_gattc_cache_conn_chr *chr)
{
    return ble_gattc_cache_conn_chr_end_handle(svc, chr) <= chr->chr.val_handle;
}

static struct ble_gattc_cache_conn_chr *
ble_gattc_cache_conn_chr_find_prev(const struct ble_gattc_cache_conn_svc *svc, uint16_t chr_val_handle)
{
    struct ble_gattc_cache_conn_chr *prev;
    struct ble_gattc_cache_conn_chr *chr;

    prev = NULL;
    SLIST_FOREACH(chr, &svc->chrs, next) {
        if (chr->chr.val_handle >= chr_val_handle) {
            break;
        }

        prev = chr;
    }

    return prev;
}

static struct ble_gattc_cache_conn_chr *
ble_gattc_cache_conn_chr_find(const struct ble_gattc_cache_conn_svc *svc, uint16_t chr_val_handle,
                              struct ble_gattc_cache_conn_chr **out_prev)
{
    struct ble_gattc_cache_conn_chr *prev;
    struct ble_gattc_cache_conn_chr *chr;

    prev = ble_gattc_cache_conn_chr_find_prev(svc, chr_val_handle);
    if (prev == NULL) {
        chr = SLIST_FIRST(&svc->chrs);
    } else {
        chr = SLIST_NEXT(prev, next);
    }

    if (chr != NULL && chr->chr.val_handle != chr_val_handle) {
        chr = NULL;
    }

    if (out_prev != NULL) {
        *out_prev = prev;
    }
    return chr;
}

static void
ble_gattc_cache_conn_chr_delete(struct ble_gattc_cache_conn_chr *chr)
{
    struct ble_gattc_cache_conn_dsc *dsc;

    while ((dsc = SLIST_FIRST(&chr->dscs)) != NULL) {
        SLIST_REMOVE_HEAD(&chr->dscs, next);
        os_memblock_put(&ble_gattc_cache_conn_dsc_pool, dsc);
    }

    os_memblock_put(&ble_gattc_cache_conn_chr_pool, chr);
}

static int
ble_gattc_cache_conn_db_hash_read(uint16_t conn_handle,
                                  const struct ble_gatt_error *error,
                                  struct ble_gatt_attr *attr,
                                  void *arg)
{
    uint16_t res;
    struct ble_gattc_cache_conn *peer;

    peer = arg;
    if (error->status != 0) {
        res = error->status;
        return res;
    }
    res = ble_hs_mbuf_to_flat(attr->om, peer->database_hash, sizeof(uint8_t) * 16, NULL);
    return res;
}

const struct ble_gattc_cache_conn_svc *
ble_gattc_cache_conn_svc_find_uuid(const struct ble_gattc_cache_conn *ble_gattc_cache_conn, const ble_uuid_t *uuid)
{
    const struct ble_gattc_cache_conn_svc *svc;

    SLIST_FOREACH(svc, &ble_gattc_cache_conn->svcs, next) {
        if (ble_uuid_cmp(&svc->svc.uuid.u, uuid) == 0) {
            return svc;
        }
    }

    return NULL;
}

static const struct ble_gattc_cache_conn_chr *
ble_gattc_cache_conn_chr_find_uuid(const struct ble_gattc_cache_conn *ble_gattc_cache_conn, const ble_uuid_t *svc_uuid,
                                   const ble_uuid_t *chr_uuid)
{
    const struct ble_gattc_cache_conn_svc *svc;
    const struct ble_gattc_cache_conn_chr *chr;

    svc = ble_gattc_cache_conn_svc_find_uuid(ble_gattc_cache_conn, svc_uuid);
    if (svc == NULL) {
        return NULL;
    }

    SLIST_FOREACH(chr, &svc->chrs, next) {
        if (ble_uuid_cmp(&chr->chr.uuid.u, chr_uuid) == 0) {
            return chr;
        }
    }

    return NULL;
}

int
ble_gattc_cache_conn_chr_add(ble_addr_t peer_addr, uint16_t svc_start_handle,
                             const struct ble_gatt_chr *gatt_chr)
{
    struct ble_gattc_cache_conn_chr *prev;
    struct ble_gattc_cache_conn_chr *chr;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn *peer;

    peer = ble_gattc_cache_conn_find_by_addr(peer_addr);
    if (peer == NULL) {
        BLE_HS_LOG(ERROR, "Peer not found");
        return BLE_HS_EUNKNOWN;
    }

    if (svc_start_handle == 0) {
        svc = ble_gattc_cache_conn_svc_find_range(peer, gatt_chr->val_handle);
    } else {
        svc = ble_gattc_cache_conn_svc_find(peer, svc_start_handle, NULL);
    }

    if (svc == NULL) {
        /* Can't find service for discovered characteristic; this shouldn't
         * happen.
         */
        assert(0);
        return BLE_HS_EUNKNOWN;
    }

    chr = ble_gattc_cache_conn_chr_find(svc, gatt_chr->def_handle, &prev);
    if (chr != NULL) {
        /* Characteristic already discovered. */
        return 0;
    }

    chr = os_memblock_get(&ble_gattc_cache_conn_chr_pool);
    if (chr == NULL) {
        /* Out of memory. */
        return BLE_HS_ENOMEM;
    }
    memset(chr, 0, sizeof * chr);

    chr->chr = *gatt_chr;

    if (prev == NULL) {
        SLIST_INSERT_HEAD(&svc->chrs, chr, next);
    } else {
        SLIST_NEXT(prev, next) = chr;
    }

    /* ble_gattc_db_hash_chr_present(chr->chr.uuid.u16); */

    BLE_HS_LOG(DEBUG, "Characteristc added with def_handle = %d and val_handle = %d",
               chr->chr.def_handle, chr->chr.val_handle);

    return 0;
}

int
ble_gattc_cache_conn_svc_is_empty(const struct ble_gattc_cache_conn_svc *svc)
{
    return svc->svc.end_handle <= svc->svc.start_handle;
}

static struct ble_gattc_cache_conn_svc *
ble_gattc_cache_conn_svc_find_prev(struct ble_gattc_cache_conn *peer, uint16_t svc_start_handle)
{
    struct ble_gattc_cache_conn_svc *prev;
    struct ble_gattc_cache_conn_svc *svc;

    prev = NULL;
    SLIST_FOREACH(svc, &peer->svcs, next) {
        if (svc->svc.start_handle >= svc_start_handle) {
            break;
        }

        prev = svc;
    }

    return prev;
}

static struct ble_gattc_cache_conn_svc *
ble_gattc_cache_conn_svc_find(struct ble_gattc_cache_conn *ble_gattc_cache_conn, uint16_t svc_start_handle,
                              struct ble_gattc_cache_conn_svc **out_prev)
{
    struct ble_gattc_cache_conn_svc *prev;
    struct ble_gattc_cache_conn_svc *svc;

    prev = ble_gattc_cache_conn_svc_find_prev(ble_gattc_cache_conn, svc_start_handle);
    if (prev == NULL) {
        svc = SLIST_FIRST(&ble_gattc_cache_conn->svcs);
    } else {
        svc = SLIST_NEXT(prev, next);
    }

    if (svc != NULL && svc->svc.start_handle != svc_start_handle) {
        svc = NULL;
    }

    if (out_prev != NULL) {
        *out_prev = prev;
    }
    return svc;
}

static struct ble_gattc_cache_conn_svc *
ble_gattc_cache_conn_svc_find_range(struct ble_gattc_cache_conn *ble_gattc_cache_conn, uint16_t attr_handle)
{
    struct ble_gattc_cache_conn_svc *svc;

    SLIST_FOREACH(svc, &ble_gattc_cache_conn->svcs, next) {
        if (svc->svc.start_handle <= attr_handle &&
                svc->svc.end_handle >= attr_handle) {

            return svc;
        }
    }

    return NULL;
}

const struct ble_gattc_cache_conn_dsc *
ble_gattc_cache_conn_dsc_find_uuid(const struct ble_gattc_cache_conn *ble_gattc_cache_conn, const ble_uuid_t *svc_uuid,
                                   const ble_uuid_t *chr_uuid, const ble_uuid_t *dsc_uuid)
{
    const struct ble_gattc_cache_conn_chr *chr;
    const struct ble_gattc_cache_conn_dsc *dsc;

    chr = ble_gattc_cache_conn_chr_find_uuid(ble_gattc_cache_conn, svc_uuid, chr_uuid);
    if (chr == NULL) {
        return NULL;
    }

    SLIST_FOREACH(dsc, &chr->dscs, next) {
        if (ble_uuid_cmp(&dsc->dsc.uuid.u, dsc_uuid) == 0) {
            return dsc;
        }
    }

    return NULL;
}

int
ble_gattc_cache_conn_inc_add(ble_addr_t peer_addr, const struct ble_gatt_svc *gatt_svc)
{
    struct ble_gattc_cache_conn_svc *prev;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn *peer;

    peer = ble_gattc_cache_conn_find_by_addr(peer_addr);
    if (peer == NULL) {
        BLE_HS_LOG(ERROR, "Peer not found");
        return BLE_HS_EUNKNOWN;
    }

    svc = ble_gattc_cache_conn_svc_find(peer, gatt_svc->start_handle, &prev);
    if (svc != NULL) {
        /* Inc already discovered. */
        return 0;
    }

    svc = os_memblock_get(&ble_gattc_cache_conn_svc_pool);
    if (svc == NULL) {
        /* Out of memory. */
        return BLE_HS_ENOMEM;
    }
    memset(svc, 0, sizeof * svc);

    svc->type = BLE_GATT_SVC_TYPE_SECONDARY;
    svc->svc = *gatt_svc;
    SLIST_INIT(&svc->chrs);

    if (prev == NULL) {
        SLIST_INSERT_HEAD(&peer->svcs, svc, next);
    } else {
        SLIST_INSERT_AFTER(prev, svc, next);
    }

    BLE_HS_LOG(DEBUG, "Inc added with start_handle = %d and end_handle = %d",
               gatt_svc->start_handle, gatt_svc->end_handle);

    return 0;
}

int
ble_gattc_cache_conn_svc_add(ble_addr_t peer_addr, const struct ble_gatt_svc *gatt_svc)
{
    struct ble_gattc_cache_conn_svc *prev;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn *peer;

    peer = ble_gattc_cache_conn_find_by_addr(peer_addr);
    if (peer == NULL) {
        BLE_HS_LOG(ERROR, "Peer not found");
        return BLE_HS_EUNKNOWN;
    }

    svc = ble_gattc_cache_conn_svc_find(peer, gatt_svc->start_handle, &prev);
    if (svc != NULL) {
        /* Service already discovered. */
        return 0;
    }

    svc = os_memblock_get(&ble_gattc_cache_conn_svc_pool);
    if (svc == NULL) {
        /* Out of memory. */
        return BLE_HS_ENOMEM;
    }
    memset(svc, 0, sizeof * svc);

    svc->type = BLE_GATT_SVC_TYPE_PRIMARY;
    svc->svc = *gatt_svc;
    SLIST_INIT(&svc->chrs);

    if (prev == NULL) {
        SLIST_INSERT_HEAD(&peer->svcs, svc, next);
    } else {
        SLIST_INSERT_AFTER(prev, svc, next);
    }

    BLE_HS_LOG(DEBUG, "Service added with start_handle = %d and end_handle = %d",
               gatt_svc->start_handle, gatt_svc->end_handle);

    return 0;
}

static void
ble_gattc_cache_conn_svc_delete(struct ble_gattc_cache_conn_svc *svc)
{
    struct ble_gattc_cache_conn_chr *chr;

    while ((chr = SLIST_FIRST(&svc->chrs)) != NULL) {
        SLIST_REMOVE_HEAD(&svc->chrs, next);
        ble_gattc_cache_conn_chr_delete(chr);
    }

    os_memblock_put(&ble_gattc_cache_conn_svc_pool, svc);
}

size_t
ble_gattc_cache_conn_get_db_size(struct ble_gattc_cache_conn *peer)
{
    if (peer == NULL) {
        return 0;
    }

    size_t db_size = 0;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn_chr *chr;
    struct ble_gattc_cache_conn_dsc *dsc;

    SLIST_FOREACH(svc, &peer->svcs, next) {
        db_size++;
        SLIST_FOREACH(chr, &svc->chrs, next) {
            db_size++;
            SLIST_FOREACH(dsc, &chr->dscs, next) {
                db_size++;
            }
        }
    }

    return db_size;
}

static void
ble_gattc_cache_conn_cache_peer(struct ble_gattc_cache_conn *peer)
{
    /* Cache the discovered peer */
    size_t db_size = ble_gattc_cache_conn_get_db_size(peer);
    ble_gattc_cache_save(peer, db_size);
}

void
ble_gattc_cache_conn_broken(uint16_t conn_handle)
{
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn *conn;

    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        return;
    }
    BLE_HS_DBG_ASSERT(conn != NULL);

    /* clean the cache_conn */
    SLIST_REMOVE(&ble_gattc_cache_conns, conn, ble_gattc_cache_conn, next);

    while ((svc = SLIST_FIRST(&conn->svcs)) != NULL) {
        SLIST_REMOVE_HEAD(&conn->svcs, next);
        ble_gattc_cache_conn_svc_delete(svc);

    }
    os_memblock_put(&ble_gattc_cache_conn_pool, conn);
}

void
ble_gattc_cache_conn_bonding_established(uint16_t conn_handle)
{
    /* update the address of the peer */
    struct ble_hs_conn *conn;
    struct ble_hs_conn_addrs addrs;
    struct ble_gattc_cache_conn *peer;

    peer = ble_gattc_cache_conn_find(conn_handle);
    if (peer == NULL) {
        return;
    }

    ble_hs_lock();

    conn = ble_hs_conn_find(conn_handle);
    BLE_HS_DBG_ASSERT(conn != NULL);
    ble_hs_conn_addrs(conn, &addrs);
    peer->ble_gattc_cache_conn_addr = conn->bhc_peer_addr;

    peer->ble_gattc_cache_conn_addr.type =
        ble_hs_misc_peer_addr_type_to_id(conn->bhc_peer_addr.type);

    ble_hs_unlock();
}

void
ble_gattc_cache_conn_bonding_restored(uint16_t conn_handle)
{
    struct ble_hs_conn *conn;
    struct ble_hs_conn_addrs addrs;
    struct ble_gattc_cache_conn *peer;
    int rc;

    peer = ble_gattc_cache_conn_find(conn_handle);
    if (peer == NULL) {
        return;
    }

    ble_hs_lock();

    conn = ble_hs_conn_find(conn_handle);
    BLE_HS_DBG_ASSERT(conn != NULL);
    ble_hs_conn_addrs(conn, &addrs);
    peer->ble_gattc_cache_conn_addr = conn->bhc_peer_addr;
    peer->ble_gattc_cache_conn_addr.type =
        ble_hs_misc_peer_addr_type_to_id(conn->bhc_peer_addr.type);

    ble_hs_unlock();
    /* try to load if not loaded */
    if (peer->cache_state == CACHE_INVALID) {
        rc = ble_gattc_cache_load(peer->ble_gattc_cache_conn_addr);
        if (rc == 0) {
            /* connection is bonded,
            so it is safe to set the state to
                CACHE_VERIFIED */
            /* if the cache is changed after disconnect
            then the indication will be received */
            peer->cache_state = CACHE_VERIFIED;
        }
    }
    if (peer->cache_state == CACHE_LOADED) {
        peer->cache_state = CACHE_VERIFIED;
    }
}

static void service_sanity_check(struct ble_gattc_cache_conn_svc_list *svcs)
{
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn_chr *chr, *prev = NULL;
    struct ble_gattc_cache_conn_dsc *dsc;
    uint16_t end_handle = 0;
    SLIST_FOREACH(svc, svcs, next) {
        if (svc->svc.end_handle == 65535) {
            SLIST_FOREACH(chr, &svc->chrs, next) {
                end_handle = chr->chr.val_handle;
                prev = chr;
            }
            SLIST_FOREACH(dsc, &prev->dscs, next) {
                end_handle = dsc->dsc.handle;
            }
            svc->svc.end_handle = end_handle;
        }
    }
}

static void
ble_gattc_cache_conn_disc_complete(struct ble_gattc_cache_conn *peer, int rc)
{
    struct ble_gattc_cache_conn_op *op;
    struct ble_hs_conn *hs_conn;
    const struct ble_gattc_cache_conn_chr *chr;
    bool bonded;

    peer->disc_prev_chr_val = 0;
    if (rc == 0) {
        /* discovery complete */
        peer->cache_state = CACHE_VERIFIED;
        service_sanity_check(&peer->svcs);

        /* cache the database only if the connection
           is trusted or database hash exists */
        ble_hs_lock();
        hs_conn = ble_hs_conn_find(peer->conn_handle);
        BLE_HS_DBG_ASSERT(hs_conn != NULL);
        bonded = hs_conn->bhc_sec_state.bonded;
        ble_hs_unlock();

        chr = ble_gattc_cache_conn_chr_find_uuid(peer,
                                                 BLE_UUID16_DECLARE(BLE_GATT_SVC_UUID16),
                                                 BLE_UUID16_DECLARE(BLE_SVC_GATT_CHR_DATABASE_HASH_UUID16));
        if (bonded || chr != NULL) {
            /* persist the cache */
            ble_gattc_cacheReset(&hs_conn->bhc_peer_addr);
            ble_gattc_cache_conn_cache_peer(peer); /* TODO */
        }
    } else {
        peer->cache_state = CACHE_INVALID;
    }
    /* respond to the pending gatt op */
    op = &peer->pending_op;
    if (op->cb) {
        switch (op->cb_type) {
        case BLE_GATT_OP_DISC_ALL_SVCS :
            rc = ble_gattc_cache_conn_search_all_svcs(peer->conn_handle, op->cb, op->cb_arg);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "search service failed");
            }
            break;
        case BLE_GATT_OP_DISC_SVC_UUID :
            rc = ble_gattc_cache_conn_search_svc_by_uuid(peer->conn_handle, &op->uuid, op->cb, op->cb_arg);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "search service by uuid failed");
            }
            break;
        case BLE_GATT_OP_FIND_INC_SVCS :
            rc = ble_gattc_cache_conn_search_inc_svcs(peer->conn_handle, op->start_handle, op->end_handle, op->cb, op->cb_arg);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "search inc failed");
            }
            break;
        case BLE_GATT_OP_DISC_ALL_CHRS :
            rc = ble_gattc_cache_conn_search_all_chrs(peer->conn_handle, op->start_handle, op->end_handle, op->cb, op->cb_arg);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "search all chars failed");
            }
            break;
        case BLE_GATT_OP_DISC_CHR_UUID :
            rc = ble_gattc_cache_conn_search_chrs_by_uuid(peer->conn_handle, op->start_handle, op->end_handle, &op->uuid, op->cb, op->cb_arg);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "search chars by uuid failed");
            }
            break;
        case BLE_GATT_OP_DISC_ALL_DSCS :
            rc = ble_gattc_cache_conn_search_all_dscs(peer->conn_handle, op->start_handle, op->end_handle, op->cb, op->cb_arg);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "search all discs failed");
            }
            break;
        }
    }
}

void
ble_gattc_cache_conn_undisc_all(ble_addr_t peer_addr)
{
    struct ble_gattc_cache_conn * peer = NULL;

    peer = ble_gattc_cache_conn_find_by_addr(peer_addr);
    if (peer == NULL) {
        return;
    }
    ble_gattc_cacheReset(&peer->ble_gattc_cache_conn_addr);

    struct ble_gattc_cache_conn_svc *svc;

    while ((svc = SLIST_FIRST(&peer->svcs)) != NULL) {
        SLIST_REMOVE_HEAD(&peer->svcs, next);
        ble_gattc_cache_conn_svc_delete(svc);
    }
}

static int
ble_gattc_cache_conn_inc_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                                const struct ble_gatt_svc *service, void *arg)
{
    struct ble_gattc_cache_conn *peer;
    int rc;

    peer = arg;
    assert(peer->conn_handle == conn_handle);

    switch (error->status) {
    case 0:
        rc = ble_gattc_cache_conn_inc_add(peer->ble_gattc_cache_conn_addr, service);
        break;

    case BLE_HS_EDONE:
        /* All services discovered; start discovering incs. */
        ble_gattc_cache_conn_disc_chrs(peer);
        rc = 0;
        break;

    default:
        rc = error->status;
        break;
    }

    if (rc != 0) {
        /* Error; abort discovery. */
        ble_gattc_cache_conn_disc_complete(peer, rc);
    }

    return rc;
}
static int
ble_gattc_cache_conn_svc_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                                const struct ble_gatt_svc *service, void *arg)
{
    struct ble_gattc_cache_conn *peer;
    int rc;

    peer = arg;
    assert(peer->conn_handle == conn_handle);

    switch (error->status) {
    case 0:
        rc = ble_gattc_cache_conn_svc_add(peer->ble_gattc_cache_conn_addr, service);
        break;

    case BLE_HS_EDONE:
        /* All services discovered; start discovering incs. */
        peer->cur_svc = NULL;
        ble_gattc_cache_conn_disc_incs(peer);
        rc = 0;
        break;

    default:
        rc = error->status;
        break;
    }

    if (rc != 0) {
        /* Error; abort discovery. */
        ble_gattc_cache_conn_disc_complete(peer, rc);
    }

    return rc;
}

static int
ble_gattc_cache_conn_disc(struct ble_gattc_cache_conn *peer)
{
    int rc;

    ble_gattc_cache_conn_undisc_all(peer->ble_gattc_cache_conn_addr);

    peer->disc_prev_chr_val = 1;

    BLE_HS_LOG(INFO, "Initiating Remote Service Discovery");
    peer->cache_state = SVC_DISC_IN_PROGRESS;
    rc = ble_gattc_disc_all_svcs(peer->conn_handle, ble_gattc_cache_conn_svc_disced, peer);
    return rc;
}

static int
ble_gattc_cache_conn_on_read(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             struct ble_gatt_attr *attr,
                             void *arg)
{
    uint16_t res;

    if (error->status == BLE_HS_EDONE) {
        /* Ignore Read by UUID follow-up callback */
        return 0;
    }

    if (error->status != 0) {
        res = error->status;
        ble_gattc_cache_conn_disc_complete((struct ble_gattc_cache_conn *)arg, res);
        return res;
    }

    res = ble_gattc_cache_check_hash((struct ble_gattc_cache_conn *)arg, attr->om);
    if (res == 0) {
        BLE_HS_LOG(INFO, "Hash value up to date, skipping Discovery");
        ble_gattc_cache_conn_disc_complete((struct ble_gattc_cache_conn *)arg, res);
        return 0;
    } else {
        res = ble_gattc_cache_conn_disc((struct ble_gattc_cache_conn *)arg);
        return res;
    }
}

int
ble_gattc_cache_conn_create(uint16_t conn_handle, ble_addr_t ble_gattc_cache_conn_addr)
{
    struct ble_gattc_cache_conn *cache_conn;
    int rc;

    /* Make sure the connection handle is unique. */
    cache_conn = ble_gattc_cache_conn_find(conn_handle);
    if (cache_conn != NULL) {
        /* peer is present already */
        /* TODO : validate cache somehow */
        return 0;
    }

    cache_conn = os_memblock_get(&ble_gattc_cache_conn_pool);
    if (cache_conn == NULL) {
        /* Out of memory. */
        return BLE_HS_ENOMEM;
    }

    memset(cache_conn, 0, sizeof(struct ble_gattc_cache_conn));

    /* set the conn as dirty initially as the cache is not built */
    cache_conn->cache_state = CACHE_INVALID;
    cache_conn->conn_handle = conn_handle;
    memcpy(&cache_conn->ble_gattc_cache_conn_addr, &ble_gattc_cache_conn_addr, sizeof(ble_addr_t));
    SLIST_INSERT_HEAD(&ble_gattc_cache_conns, cache_conn, next);

    /* Load cache */
    rc = ble_gattc_cache_load(ble_gattc_cache_conn_addr);
    if (rc == 0) {
        cache_conn->cache_state = CACHE_LOADED;
    }
    return 0;
}

void
ble_gattc_cache_conn_load_hash(ble_addr_t peer_addr, uint8_t *hash_key)
{
    struct ble_gattc_cache_conn *peer;
    peer = ble_gattc_cache_conn_find_by_addr(peer_addr);

    if (peer == NULL) {
        BLE_HS_LOG(ERROR, "%s peer NULL", __func__);
    } else {
        BLE_HS_LOG(DEBUG, "Saved hash for peer");
        memcpy(&peer->database_hash, hash_key, sizeof(uint8_t) * 16);
    }
}

static int
ble_gattc_cache_conn_dsc_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                                uint16_t chr_val_handle, const struct ble_gatt_dsc *dsc,
                                void *arg)
{
    struct ble_gattc_cache_conn *peer;
    int rc;

    peer = arg;
    assert(peer->conn_handle == conn_handle);

    switch (error->status) {
    case 0:
        rc = ble_gattc_cache_conn_dsc_add(peer->ble_gattc_cache_conn_addr, chr_val_handle, dsc);
        break;

    case BLE_HS_EDONE:
        /* All descriptors in this characteristic discovered; start discovering
         * descriptors in the next characteristic.
         */
        if (peer->disc_prev_chr_val > 0) {
            ble_gattc_cache_conn_disc_dscs(peer);
        }
        rc = 0;
        break;

    default:
        /* Error; abort discovery. */
        rc = error->status;
        break;
    }

    if (rc != 0) {
        /* Error; abort discovery. */
        ble_gattc_cache_conn_disc_complete(peer, rc);
    }

    return rc;
}

static void
ble_gattc_cache_conn_disc_dscs(struct ble_gattc_cache_conn *peer)
{
    struct ble_gattc_cache_conn_chr *chr;
    struct ble_gattc_cache_conn_svc *svc;
    int rc;

    /* Search through the list of discovered characteristics for the first
     * characteristic that contains undiscovered descriptors.  Then, discover
     * all descriptors belonging to that characteristic.
     */
    SLIST_FOREACH(svc, &peer->svcs, next) {
        SLIST_FOREACH(chr, &svc->chrs, next) {
            if (!ble_gattc_cache_conn_chr_is_empty(svc, chr) &&
                    SLIST_EMPTY(&chr->dscs) &&
                    (peer->disc_prev_chr_val <= chr->chr.def_handle)) {

                peer->cache_state = DSC_DISC_IN_PROGRESS;
                rc = ble_gattc_disc_all_dscs(peer->conn_handle,
                                             chr->chr.val_handle,
                                             ble_gattc_cache_conn_chr_end_handle(svc, chr),
                                             ble_gattc_cache_conn_dsc_disced, peer);
                if (rc != 0) {
                    ble_gattc_cache_conn_disc_complete(peer, rc);
                }

                peer->disc_prev_chr_val = chr->chr.val_handle;
                return;
            }
        }
    }

    /* All descriptors discovered. */
    ble_gattc_cache_conn_disc_complete(peer, 0);
}

static int
ble_gattc_cache_conn_chr_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
                                const struct ble_gatt_chr *chr, void *arg)
{
    struct ble_gattc_cache_conn *peer;
    int rc;

    peer = arg;
    assert(peer->conn_handle == conn_handle);

    switch (error->status) {
    case 0:
        rc = ble_gattc_cache_conn_chr_add(peer->ble_gattc_cache_conn_addr, peer->cur_svc->svc.start_handle, chr);

        if (chr->uuid.u16.value == BLE_GATTC_DATABASE_HASH_UUID128) {
            rc = ble_gattc_read(peer->conn_handle, chr->val_handle,
                                ble_gattc_cache_conn_db_hash_read, peer);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "Failed to read Database Hash %d", rc);
            }
        }
        break;

    case BLE_HS_EDONE:
        /* All characteristics in this service discovered; start discovering
         * characteristics in the next service.
         */
        if (peer->disc_prev_chr_val > 0) {
            ble_gattc_cache_conn_disc_chrs(peer);
        }
        rc = 0;
        break;

    default:
        rc = error->status;
        break;
    }

    if (rc != 0) {
        /* Error; abort discovery. */
        ble_gattc_cache_conn_disc_complete(peer, rc);
    }

    return rc;
}

static void
ble_gattc_cache_conn_disc_chrs(struct ble_gattc_cache_conn *peer)
{
    struct ble_gattc_cache_conn_svc *svc;
    int rc;
    /* Search through the list of discovered service for the first service that
     * contains undiscovered characteristics.  Then, discover all
     * characteristics belonging to that service.
     */
    SLIST_FOREACH(svc, &peer->svcs, next) {
        if (!ble_gattc_cache_conn_svc_is_empty(svc) && SLIST_EMPTY(&svc->chrs)) {
            peer->cur_svc = svc;
            peer->cache_state = CHR_DISC_IN_PROGRESS;
            rc = ble_gattc_disc_all_chrs(peer->conn_handle,
                                         svc->svc.start_handle,
                                         svc->svc.end_handle,
                                         ble_gattc_cache_conn_chr_disced, peer);
            if (rc != 0) {
                ble_gattc_cache_conn_disc_complete(peer, rc);
            }
            return;
        }
    }

    /* All characteristics discovered. */
    ble_gattc_cache_conn_disc_dscs(peer);
}

/* Note : confirm peer->cur_svc is set correctly before calling */
static void
ble_gattc_cache_conn_disc_incs(struct ble_gattc_cache_conn *peer)
{
    struct ble_gattc_cache_conn_svc *svc;
    int rc;

    if (peer->cur_svc == NULL) {
        peer->cur_svc = SLIST_FIRST(&peer->svcs);
    } else {
        peer->cur_svc = SLIST_NEXT(peer->cur_svc, next);
        if (peer->cur_svc == NULL) {
            if (peer->disc_prev_chr_val > 0) {
                ble_gattc_cache_conn_disc_chrs(peer);
            }
        }
    }
    peer->cache_state = INC_DISC_IN_PROGRESS;
    svc = peer->cur_svc;
    rc = ble_gattc_find_inc_svcs(peer->conn_handle,
                                 svc->svc.start_handle,
                                 svc->svc.end_handle,
                                 ble_gattc_cache_conn_inc_disced, peer);
    if (rc != 0) {
        ble_gattc_cache_conn_disc_complete(peer, rc);
    }
    return;
}
/* As esp_nimble only supports adding/deleting whole services
 * It is safe to assume that the start_handle and end_handle
 * spans 1 or more services
 */
void
ble_gattc_cache_conn_update(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
    /* All attributes are being rediscovered
       TODO : rediscover only the start_handle to end_handle */
    struct ble_gattc_cache_conn *peer;
    int rc;

    peer = ble_gattc_cache_conn_find(conn_handle);
    if (peer == NULL) {
        BLE_HS_LOG(ERROR, "Cannot find connection with conn_handle %d", conn_handle);
    }

    peer->cache_state = CACHE_INVALID;
    if (MYNEWT_VAL(BLE_GATT_CACHING_DISABLE_AUTO)) {
	    /* Do not automatically re-discover and correct cache */
        return;
    }
    rc = ble_gattc_cache_conn_disc(peer);
    if (rc != 0) {
        peer->cache_state = CACHE_INVALID;
    }
}

uint16_t
ble_gattc_cache_conn_get_svc_changed_handle(uint16_t conn_handle)
{
    struct ble_gattc_cache_conn *peer;
    const struct ble_gattc_cache_conn_chr *chr;

    peer = ble_gattc_cache_conn_find(conn_handle);
    if (peer == NULL) {
        BLE_HS_LOG(ERROR, "Cannot find connection with conn_handle %d", conn_handle);
        return -1;
    }

    /* Check if attr_handle is of service change char */
    chr = ble_gattc_cache_conn_chr_find_uuid(peer, BLE_UUID16_DECLARE(BLE_GATT_SVC_UUID16),
                                             BLE_UUID16_DECLARE(BLE_SVC_GATT_CHR_SERVICE_CHANGED_UUID16));

    if (chr == NULL) {
        BLE_HS_LOG(ERROR, "Cannot find service change characteristic");
        return -1;
    }
    return chr->chr.val_handle;
}

void
ble_gattc_cache_conn_free_mem(void)
{
    free(ble_gattc_cache_conn_mem);
    ble_gattc_cache_conn_mem = NULL;

    free(ble_gattc_cache_conn_svc_mem);
    ble_gattc_cache_conn_svc_mem = NULL;

    free(ble_gattc_cache_conn_chr_mem);
    ble_gattc_cache_conn_chr_mem = NULL;

    free(ble_gattc_cache_conn_dsc_mem);
    ble_gattc_cache_conn_dsc_mem = NULL;
}

int
ble_gattc_cache_conn_init()
{
    int rc;
    int max_ble_gattc_cache_conns;
    int max_svcs;
    int max_chrs;
    int max_dscs;
    void *storage_cb;

    max_ble_gattc_cache_conns  = MYNEWT_VAL(BLE_MAX_CONNECTIONS);
    max_svcs = (MYNEWT_VAL(BLE_MAX_CONNECTIONS)) *
               (MYNEWT_VAL(BLE_GATT_CACHING_MAX_SVCS));
    max_chrs = (MYNEWT_VAL(BLE_MAX_CONNECTIONS)) *
               (MYNEWT_VAL(BLE_GATT_CACHING_MAX_CHRS));
    max_dscs = (MYNEWT_VAL(BLE_MAX_CONNECTIONS)) *
               (MYNEWT_VAL(BLE_GATT_CACHING_MAX_DSCS));
    /* Free memory first in case this function gets called more than once. */
    ble_gattc_cache_conn_free_mem();

    ble_gattc_cache_conn_mem = malloc(
                                   OS_MEMPOOL_BYTES(max_ble_gattc_cache_conns, sizeof(struct ble_gattc_cache_conn)));
    if (ble_gattc_cache_conn_mem == NULL) {
        rc = BLE_HS_ENOMEM;
        goto err;
    }

    rc = os_mempool_init(&ble_gattc_cache_conn_pool, max_ble_gattc_cache_conns,
                         sizeof(struct ble_gattc_cache_conn), ble_gattc_cache_conn_mem,
                         "ble_gattc_cache_conn_pool");
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto err;
    }

    ble_gattc_cache_conn_svc_mem = malloc(
                                       OS_MEMPOOL_BYTES(max_svcs, sizeof(struct ble_gattc_cache_conn_svc)));
    if (ble_gattc_cache_conn_svc_mem == NULL) {
        rc = BLE_HS_ENOMEM;
        goto err;
    }

    rc = os_mempool_init(&ble_gattc_cache_conn_svc_pool, max_svcs,
                         sizeof(struct ble_gattc_cache_conn_svc), ble_gattc_cache_conn_svc_mem,
                         "ble_gattc_cache_conn_svc_pool");
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto err;
    }

    ble_gattc_cache_conn_chr_mem = malloc(
                                       OS_MEMPOOL_BYTES(max_chrs, sizeof(struct ble_gattc_cache_conn_chr)));
    if (ble_gattc_cache_conn_chr_mem == NULL) {
        rc = BLE_HS_ENOMEM;
        goto err;
    }

    rc = os_mempool_init(&ble_gattc_cache_conn_chr_pool, max_chrs,
                         sizeof(struct ble_gattc_cache_conn_chr), ble_gattc_cache_conn_chr_mem,
                         "ble_gattc_cache_conn_chr_pool");
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto err;
    }

    ble_gattc_cache_conn_dsc_mem = malloc(
                                       OS_MEMPOOL_BYTES(max_dscs, sizeof(struct ble_gattc_cache_conn_dsc)));
    if (ble_gattc_cache_conn_dsc_mem == NULL) {
        rc = BLE_HS_ENOMEM;
        goto err;
    }

    rc = os_mempool_init(&ble_gattc_cache_conn_dsc_pool, max_dscs,
                         sizeof(struct ble_gattc_cache_conn_dsc), ble_gattc_cache_conn_dsc_mem,
                         "ble_gattc_cache_conn_dsc_pool");
    if (rc != 0) {
        rc = BLE_HS_EOS;
        goto err;
    }

    storage_cb = NULL;
    rc = ble_gattc_cache_init(storage_cb);
    return 0;

err:
    ble_gattc_cache_conn_free_mem();
    return rc;
}

/**
 * Returns a pointer to a GATT error object with the specified fields.  The
 * returned object is statically allocated, so this function is not reentrant.
 * This function should only ever be called by the ble_hs task.
 */
static struct ble_gatt_error *
ble_gattc_cache_error(int status, uint16_t att_handle)
{
    static struct ble_gatt_error error;

    /* For consistency, always indicate a handle of 0 on success. */
    if (status == 0 || status == BLE_HS_EDONE) {
        att_handle = 0;
    }

    error.status = status;
    error.att_handle = att_handle;
    return &error;
}

/* gattc discovery apis */

static int ble_gattc_cache_conn_verify(struct ble_gattc_cache_conn *conn)
{
    struct ble_hs_conn *gap_conn;
    int rc;

    if (conn->cache_state == CACHE_VERIFIED) {
        return 0;
    }

    ble_hs_lock();
    gap_conn = ble_hs_conn_find(conn->conn_handle);
    ble_hs_unlock();

    if (gap_conn == NULL) {
        return BLE_HS_ENOTCONN;
    }
    if (conn->cache_state == CACHE_LOADED) {
        if (gap_conn->bhc_sec_state.bonded) {
            conn->cache_state = CACHE_VERIFIED;
            return 0;
        }

        rc = ble_gattc_read_by_uuid(conn->conn_handle, 0x0001, 0xFFFF,
                                    BLE_UUID16_DECLARE(BLE_SVC_GATT_CHR_DATABASE_HASH_UUID16),
                                    ble_gattc_cache_conn_on_read, conn);
        if (rc != 0) {
            /* no way to verify */
            conn->cache_state = CACHE_INVALID;
            return 0;
        }
        conn->cache_state = VERIFY_IN_PROGRESS;
        return 0;
    }
    return 0;
}

static void ble_gattc_cache_search_all_svcs_cb(struct ble_npl_event *ev)
{
    /* return all services */
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn_op *op;
    int status = 0;
    uint16_t conn_handle;
    ble_gatt_disc_svc_fn *dcb;

    conn_handle = *(uint16_t*)ble_npl_event_get_arg(ev);
    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        return;
    }

    ble_npl_event_deinit(&conn->disc_ev);

    op = &conn->pending_op;
    dcb = op->cb;
    SLIST_FOREACH(svc, &conn->svcs, next) {
        if (svc->type == BLE_GATT_SVC_TYPE_PRIMARY) {
            dcb(conn->conn_handle, ble_gattc_cache_error(status, 0), &svc->svc, op->cb_arg);
        }
    }
    status = BLE_HS_EDONE;
    dcb(conn->conn_handle, ble_gattc_cache_error(status, 0), &svc->svc, op->cb_arg);

    return;
}

static void ble_gattc_cache_conn_fill_op(struct ble_gattc_cache_conn_op *op,
                                         uint16_t start_handle,
                                         uint16_t end_handle,
                                         ble_uuid_t uuid,
                                         void *cb,
                                         void *cb_arg,
                                         uint8_t cb_type)
{
    op->cb = cb;
    op->cb_arg = cb_arg;
    op->cb_type = cb_type;
    op->start_handle = start_handle;
    op->end_handle = end_handle;
    op->uuid = uuid;
}

int
ble_gattc_cache_conn_search_all_svcs(uint16_t conn_handle,
                                     ble_gatt_disc_svc_fn *cb, void *cb_arg)
{
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_op *op;
    ble_uuid_t uuid = {0};
    int rc;
    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        BLE_HS_LOG(DEBUG, "No connection in the Cache"
                   "HANDLE=%d\n",
                   conn_handle);
        return BLE_HS_ENOTCONN;
    }

    rc = ble_gattc_cache_conn_verify(conn);
    if (rc != 0) {
        return rc;
    }

    CHECK_CACHE_CONN_STATE(conn->cache_state, cb, cb_arg, BLE_GATT_OP_DISC_ALL_SVCS,
                           0, 0, uuid);
    /* put the event in the queue to mimic the gattc behaviour */
    ble_npl_event_init(&conn->disc_ev, ble_gattc_cache_search_all_svcs_cb, &conn->conn_handle);
    ble_npl_eventq_put((struct ble_npl_eventq *)ble_hs_evq_get(), &conn->disc_ev);
    return 0;
}

static void
ble_gattc_cache_conn_search_svc_by_uuid_cb(struct ble_npl_event *ev)
{
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn_op *op;
    int status = 0;
    uint16_t conn_handle;
    ble_gatt_disc_svc_fn *dcb;

    conn_handle = *(uint16_t*)ble_npl_event_get_arg(ev);
    /* this is to confirm if the connection still exist */
    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        return;
    }

    ble_npl_event_deinit(&conn->disc_ev);

    op = &conn->pending_op;
    dcb = op->cb;
    SLIST_FOREACH(svc, &conn->svcs, next) {
        if (svc->type == BLE_GATT_SVC_TYPE_PRIMARY && ble_uuid_cmp(&svc->svc.uuid.u, &op->uuid) == 0) {
            dcb(conn_handle, ble_gattc_cache_error(status, 0), &svc->svc, op->cb_arg);
        }
    }
    status = BLE_HS_EDONE;
    dcb(conn_handle, ble_gattc_cache_error(status, 0), &svc->svc, op->cb_arg);

    return;
}

int
ble_gattc_cache_conn_search_svc_by_uuid(uint16_t conn_handle, const ble_uuid_t *uuid,
                                        ble_gatt_disc_svc_fn *cb, void *cb_arg)
{
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_op *op;
    int rc;

    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        BLE_HS_LOG(DEBUG, "No connection in the Cache"
                   "HANDLE=%d\n",
                   conn_handle);
        return BLE_HS_ENOTCONN;
    }

    rc = ble_gattc_cache_conn_verify(conn);
    if (rc != 0) {
        return rc;
    }

    CHECK_CACHE_CONN_STATE(conn->cache_state, cb, cb_arg, BLE_GATT_OP_DISC_SVC_UUID,
                           0, 0, *uuid);
    /* put the event in the queue to mimic the gattc behaviour */
    ble_npl_event_init(&conn->disc_ev, ble_gattc_cache_conn_search_svc_by_uuid_cb, &conn->conn_handle);
    ble_npl_eventq_put((struct ble_npl_eventq *)ble_hs_evq_get(), &conn->disc_ev);
    return 0;
}

static void
ble_gattc_cache_conn_search_inc_svcs_cb(struct ble_npl_event *ev)
{
    /* return all included services */
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn_op *op;
    int status = 0;
    uint16_t conn_handle;
    ble_gatt_disc_svc_fn *dcb;

    conn_handle = *(uint16_t*)ble_npl_event_get_arg(ev);
    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        return;
    }

    ble_npl_event_deinit(&conn->disc_ev);

    op = &conn->pending_op;
    dcb = op->cb;
    SLIST_FOREACH(svc, &conn->svcs, next) {
        if (svc->type == BLE_GATT_SVC_TYPE_SECONDARY &&
                (svc->svc.start_handle >= op->start_handle && svc->svc.end_handle <= op->end_handle)) {
            dcb(conn->conn_handle, ble_gattc_cache_error(status, 0), &svc->svc, op->cb_arg);
        }
    }
    status = BLE_HS_EDONE;
    dcb(conn->conn_handle, ble_gattc_cache_error(status, 0), &svc->svc, op->cb_arg);

    return;
}
int
ble_gattc_cache_conn_search_inc_svcs(uint16_t conn_handle, uint16_t start_handle,
                                     uint16_t end_handle,
                                     ble_gatt_disc_svc_fn *cb, void *cb_arg)
{
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_op *op;
    ble_uuid_t uuid = {0};
    int rc;

    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        BLE_HS_LOG(DEBUG, "No connection in the Cache"
                   "HANDLE=%d\n",
                   conn_handle);
        return BLE_HS_ENOTCONN;
    }

    rc = ble_gattc_cache_conn_verify(conn);
    if (rc != 0) {
        return rc;
    }

    CHECK_CACHE_CONN_STATE(conn->cache_state, cb, cb_arg, BLE_GATT_OP_DISC_ALL_CHRS,
                           start_handle, end_handle, uuid);
    /* put the event in the queue to mimic the gattc behaviour */
    ble_npl_event_init(&conn->disc_ev, ble_gattc_cache_conn_search_inc_svcs_cb, &conn->conn_handle);
    ble_npl_eventq_put((struct ble_npl_eventq *)ble_hs_evq_get(), &conn->disc_ev);
    return 0;
}

static void
ble_gattc_cache_conn_search_all_chrs_cb(struct ble_npl_event *ev)
{
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn_chr *chr;
    struct ble_gattc_cache_conn_op *op;
    int status = 0;
    uint16_t conn_handle;
    ble_gatt_chr_fn *dcb;

    conn_handle = *(uint16_t*)ble_npl_event_get_arg(ev);
    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        return;
    }

    ble_npl_event_deinit(&conn->disc_ev);

    op = &conn->pending_op;
    dcb = op->cb;
    svc = ble_gattc_cache_conn_svc_find_range(conn, op->start_handle);
    /* return all chrs */
    SLIST_FOREACH(chr, &svc->chrs, next) {
        dcb(conn_handle, ble_gattc_cache_error(status, 0), &chr->chr, op->cb_arg);
    }
    status = BLE_HS_EDONE;
    dcb(conn_handle, ble_gattc_cache_error(status, 0), &chr->chr, op->cb_arg);

    return;
}

int
ble_gattc_cache_conn_search_all_chrs(uint16_t conn_handle, uint16_t start_handle,
                                     uint16_t end_handle, ble_gatt_chr_fn *cb,
                                     void *cb_arg)
{
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_op *op;
    ble_uuid_t uuid = {0};
    int rc;

    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        BLE_HS_LOG(DEBUG, "No connection in the Cache"
                   "HANDLE=%d\n",
                   conn_handle);
        return BLE_HS_ENOTCONN;
    }

    rc = ble_gattc_cache_conn_verify(conn);
    if (rc != 0) {
        return rc;
    }

    CHECK_CACHE_CONN_STATE(conn->cache_state, cb, cb_arg, BLE_GATT_OP_DISC_ALL_CHRS,
                           start_handle, end_handle, uuid);
    /* put the event in the queue to mimic the gattc behaviour */
    ble_npl_event_init(&conn->disc_ev, ble_gattc_cache_conn_search_all_chrs_cb, &conn->conn_handle);
    ble_npl_eventq_put((struct ble_npl_eventq *)ble_hs_evq_get(), &conn->disc_ev);
    return 0;
}

static void
ble_gattc_cache_conn_search_chrs_by_uuid_cb(struct ble_npl_event *ev)
{
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn_chr *chr;
    struct ble_gattc_cache_conn_op *op;
    int status = 0;
    uint16_t conn_handle;
    ble_gatt_chr_fn *dcb;

    conn_handle = *(uint16_t*)ble_npl_event_get_arg(ev);
    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        return;
    }

    ble_npl_event_deinit(&conn->disc_ev);

    op = &conn->pending_op;
    dcb = op->cb;
    svc = ble_gattc_cache_conn_svc_find_range(conn, op->start_handle);
    /* return all chrs */
    SLIST_FOREACH(chr, &svc->chrs, next) {
        if (ble_uuid_cmp(&chr->chr.uuid.u, &op->uuid) == 0) {
            dcb(conn_handle, ble_gattc_cache_error(status, 0), &chr->chr, op->cb_arg);
        }
    }
    status = BLE_HS_EDONE;
    dcb(conn_handle, ble_gattc_cache_error(status, 0), &chr->chr, op->cb_arg);

    return;
}

int
ble_gattc_cache_conn_search_chrs_by_uuid(uint16_t conn_handle, uint16_t start_handle,
                                         uint16_t end_handle, const ble_uuid_t *uuid,
                                         ble_gatt_chr_fn *cb, void *cb_arg)
{
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_op *op;
    int rc;

    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        BLE_HS_LOG(DEBUG, "No connection in the Cache"
                   "HANDLE=%d\n",
                   conn_handle);
        return BLE_HS_ENOTCONN;
    }

    rc = ble_gattc_cache_conn_verify(conn);
    if (rc != 0) {
        return rc;
    }

    CHECK_CACHE_CONN_STATE(conn->cache_state, cb, cb_arg, BLE_GATT_OP_DISC_CHR_UUID,
                           start_handle, end_handle, *uuid);
    /* put the event in the queue to mimic the gattc behaviour */
    ble_npl_event_init(&conn->disc_ev, ble_gattc_cache_conn_search_chrs_by_uuid_cb, &conn->conn_handle);
    ble_npl_eventq_put((struct ble_npl_eventq *)ble_hs_evq_get(), &conn->disc_ev);
    return 0;
}

static void
ble_gattc_cache_conn_search_all_dscs_cb(struct ble_npl_event *ev)
{
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn_chr *chr;
    struct ble_gattc_cache_conn_dsc *dsc;
    struct ble_gattc_cache_conn_op *op;
    int status = 0;
    uint16_t conn_handle;
    ble_gatt_dsc_fn *dcb;

    conn_handle = *(uint16_t*)ble_npl_event_get_arg(ev);
    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        return;
    }

    ble_npl_event_deinit(&conn->disc_ev);

    op = &conn->pending_op;
    dcb = op->cb;
    svc = ble_gattc_cache_conn_svc_find_range(conn, op->start_handle);
    chr = ble_gattc_cache_conn_chr_find_range(svc, op->start_handle);
    SLIST_FOREACH(dsc, &chr->dscs, next) {
        dcb(conn_handle, ble_gattc_cache_error(status, 0), chr->chr.val_handle, &dsc->dsc, op->cb_arg);
    }
    status = BLE_HS_EDONE;
    dcb(conn_handle, ble_gattc_cache_error(status, 0), chr->chr.val_handle, &dsc->dsc, op->cb_arg);

    return;
}

int
ble_gattc_cache_conn_search_all_dscs(uint16_t conn_handle, uint16_t start_handle,
                                     uint16_t end_handle,
                                     ble_gatt_dsc_fn *cb, void *cb_arg)
{
    struct ble_gattc_cache_conn *conn;
    struct ble_gattc_cache_conn_op *op;
    int rc;
    ble_uuid_t uuid = {0};

    conn = ble_gattc_cache_conn_find(conn_handle);
    if (conn == NULL) {
        BLE_HS_LOG(DEBUG, "No connection in the Cache"
                   "HANDLE=%d\n",
                   conn_handle);
        return BLE_HS_ENOTCONN;
    }

    rc = ble_gattc_cache_conn_verify(conn);
    if (rc != 0) {
        return rc;
    }

    CHECK_CACHE_CONN_STATE(conn->cache_state, cb, cb_arg, BLE_GATT_OP_DISC_ALL_DSCS,
                           start_handle, end_handle, uuid);
    /* put the event in the queue to mimic the gattc behaviour */
    ble_npl_event_init(&conn->disc_ev, ble_gattc_cache_conn_search_all_dscs_cb, &conn->conn_handle);
    ble_npl_eventq_put((struct ble_npl_eventq *)ble_hs_evq_get(), &conn->disc_ev);
    return 0;
}
#endif
