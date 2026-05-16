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

#ifndef H_BLE_GATTC_CACHE_PRIV_
#define H_BLE_GATTC_CACHE_PRIV_

#include "nimble/porting/nimble/include/modlog/modlog.h"
#include "nimble/porting/nimble/include/os/queue.h"
#include "nimble/nimble/host/include/host/ble_gatt.h"
#include "nimble/nimble/include/nimble/ble.h"
#include "nimble/nimble/host/services/gatt/include/services/gatt/ble_svc_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MYNEWT_VAL(BLE_GATT_CACHING)
#define BLE_GATTC_DATABASE_HASH_UUID128             0x2b2a
enum {
    BLE_GATT_ATTR_TYPE_INCL_SRVC,
    BLE_GATT_ATTR_TYPE_CHAR,
    BLE_GATT_ATTR_TYPE_CHAR_DESCR,
    BLE_GATT_ATTR_TYPE_SRVC
};
typedef uint8_t ble_gatt_attr_type;

typedef struct ble_gatt_nv_attr {
    uint16_t s_handle;
    uint16_t e_handle;              /* used for service only */
    ble_gatt_attr_type attr_type;
    ble_uuid_any_t uuid;
    uint8_t properties;             /* used for characteristic only */
    unsigned int is_primary : 1;                /* used for service only */
} ble_gatt_nv_attr;

/* cache conn */
/** ble_gattc_cache_conn. */
struct ble_gattc_cache_conn_dsc {
    SLIST_ENTRY(ble_gattc_cache_conn_dsc) next;
    struct ble_gatt_dsc dsc;
};
SLIST_HEAD(ble_gattc_cache_conn_dsc_list, ble_gattc_cache_conn_dsc);

struct ble_gattc_cache_conn_chr {
    SLIST_ENTRY(ble_gattc_cache_conn_chr) next;
    struct ble_gatt_chr chr;

    struct ble_gattc_cache_conn_dsc_list dscs;
};
SLIST_HEAD(ble_gattc_cache_conn_chr_list, ble_gattc_cache_conn_chr);

struct ble_gattc_cache_conn_svc {
    SLIST_ENTRY(ble_gattc_cache_conn_svc) next;
    /**
     * One of the following:
     *     o BLE_GATT_SVC_TYPE_PRIMARY - primary service
     *     o BLE_GATT_SVC_TYPE_SECONDARY - secondary service
     */
    uint8_t type;
    struct ble_gatt_svc svc;

    struct ble_gattc_cache_conn_chr_list chrs;
};
SLIST_HEAD(ble_gattc_cache_conn_svc_list, ble_gattc_cache_conn_svc);

struct ble_gattc_cache_conn;
typedef void ble_gattc_cache_conn_disc_fn(const struct ble_gattc_cache_conn *ble_gattc_cache_conn, int status, void *arg);
enum {
    CACHE_INVALID = 0,
    CACHE_LOADED,
    CACHE_VERIFIED,
    SVC_DISC_IN_PROGRESS,
    CHR_DISC_IN_PROGRESS,
    INC_DISC_IN_PROGRESS,
    DSC_DISC_IN_PROGRESS,
    VERIFY_IN_PROGRESS
};
struct ble_gattc_cache_conn_op {
    /* cb is used only when the gattc
       request comes while the cache is building */
    uint16_t start_handle;
    uint16_t end_handle;
    ble_uuid_t uuid;
    void *cb;
    void *cb_arg;
    uint8_t cb_type;
};
struct ble_gattc_cache_conn {
    SLIST_ENTRY(ble_gattc_cache_conn) next;

    uint16_t conn_handle;
    ble_addr_t ble_gattc_cache_conn_addr;

    uint8_t database_hash[16];

    /** List of discovered GATT services. */
    struct ble_gattc_cache_conn_svc_list svcs;

    uint8_t cache_state;
    /** Keeps track of where we are in the service discovery process. */
    uint16_t disc_prev_chr_val;
    struct ble_gattc_cache_conn_svc *cur_svc;

    struct ble_gattc_cache_conn_op pending_op;
    /* event to be posted to inform
    the application about the discovery results */
    struct ble_npl_event disc_ev;
};
/* apis from gatt service */
uint16_t ble_svc_gatt_changed_handle();
uint16_t ble_svc_gatt_hash_handle();
uint16_t ble_svc_gatt_csf_handle();
uint8_t ble_svc_gatt_get_csfs();

#if MYNEWT_VAL(BLE_DYNAMIC_SERVICE)
void ble_svc_gatt_changed(uint16_t start_handle, uint16_t end_handle);
#endif
int ble_gattc_cache_conn_add(uint16_t conn_handle, ble_addr_t ble_gattc_cache_conn_addr);
int ble_gattc_cache_conn_svc_add(ble_addr_t peer_addr, const struct ble_gatt_svc *gatt_svc);
int ble_gattc_cache_conn_inc_add(ble_addr_t peer_addr, const struct ble_gatt_svc *gatt_svc);
int ble_gattc_cache_conn_chr_add(ble_addr_t peer_addr, uint16_t svc_start_handle,
                                 const struct ble_gatt_chr *gatt_chr);
int ble_gattc_cache_conn_dsc_add(ble_addr_t peer_addr, uint16_t chr_val_handle,
                                 const struct ble_gatt_dsc *gatt_dsc);
/**
 * Loads the cache for the connection given by conn_handle
 */
int ble_gattc_cache_conn_create(uint16_t conn_handle, ble_addr_t ble_gattc_cache_conn_addr);

void ble_gattc_cache_conn_load_hash(ble_addr_t peer_addr, uint8_t *hash_key);
void ble_gattc_cache_conn_update(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle);
uint16_t ble_gattc_cache_conn_get_svc_changed_handle(uint16_t conn_handle);

/* cache store */
void ble_gattc_cache_save(struct ble_gattc_cache_conn *peer, size_t num_attr);
int ble_gattc_cache_init(void *storage_cb);
int ble_gattc_cache_load(ble_addr_t peer_addr);
int ble_gattc_cache_check_hash(struct ble_gattc_cache_conn *peer, struct os_mbuf *om);
void ble_gattc_cacheReset(ble_addr_t *addr);
void ble_gattc_cache_conn_broken(uint16_t conn_handle);
void ble_gattc_cache_conn_bonding_established(uint16_t conn_handle);
void ble_gattc_cache_conn_bonding_restored(uint16_t conn_handle);

/* cache search */
int ble_gattc_cache_conn_search_all_svcs(uint16_t conn_handle,
                                         ble_gatt_disc_svc_fn *cb, void *cb_arg);
int ble_gattc_cache_conn_search_svc_by_uuid(uint16_t conn_handle, const ble_uuid_t *uuid,
                                            ble_gatt_disc_svc_fn *cb, void *cb_arg);
int ble_gattc_cache_conn_search_inc_svcs(uint16_t conn_handle, uint16_t start_handle,
                                         uint16_t end_handle,
                                         ble_gatt_disc_svc_fn *cb, void *cb_arg);
int ble_gattc_cache_conn_search_all_chrs(uint16_t conn_handle, uint16_t start_handle,
                                         uint16_t end_handle, ble_gatt_chr_fn *cb,
                                         void *cb_arg);
int ble_gattc_cache_conn_search_chrs_by_uuid(uint16_t conn_handle, uint16_t start_handle,
                                             uint16_t end_handle, const ble_uuid_t *uuid,
                                             ble_gatt_chr_fn *cb, void *cb_arg);
int
ble_gattc_cache_conn_search_all_dscs(uint16_t conn_handle, uint16_t start_handle,
                                     uint16_t end_handle,
                                     ble_gatt_dsc_fn *cb, void *cb_arg);

#ifdef __cplusplus
}
#endif

#endif
#endif
