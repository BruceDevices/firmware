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
#include "nimble/nimble/host/include/host/ble_hs.h"
#include "ble_hs_conn_priv.h"
#include "ble_hs_priv.h"
#include "ble_gattc_cache_priv.h"

#include "nimble/porting/nimble/include/nimble/storage_port.h"
#include "nimble/nimble/host/include/host/ble_gatt.h"
#ifdef ESP_PLATFORM
#include "nimble/esp_port/port/include/esp_nimble_mem.h"
#endif

#define GATT_CACHE_PREFIX "gatt_"
#define INVALID_ADDR_NUM 0xff
#define MAX_DEVICE_IN_CACHE 50
#define MAX_ADDR_LIST_CACHE_BUF 2048

#if MYNEWT_VAL(BLE_GATT_CACHING)
static const char *cache_key = "gattc_cache_key";
static const char *cache_addr = "cache_addr_tab";
static uint8_t ble_gattc_cache_find_addr(ble_addr_t addr);
static uint8_t ble_gattc_cache_find_hash(uint8_t * hash_key);
static uint8_t svc_end_handle;
struct cache_fn_mapping cache_fn;

typedef struct {
    /*Save the service data in the list according to the address */
    cache_handle_t cache_fp;
    bool is_open;
    ble_addr_t addr;
    uint8_t hash_key[16];
} cache_addr_info_t;

typedef struct {
    /* Save the address list in the cache */
    cache_handle_t addr_fp;
    bool is_open;
    uint8_t num_addr;
    cache_addr_info_t cache_addr[MAX_DEVICE_IN_CACHE];
} cache_env_t;

static cache_env_t *cache_env = NULL;

static void
print_hash_key(uint8_t * hash_key)
{
    MODLOG_DFLT(DEBUG, "Hash Key : ");

    for (int i = 0; i < 16; i++) {
        MODLOG_DFLT(DEBUG, "%x ", hash_key[i]);
    }
}

static void
print_addr(ble_addr_t addr)
{
    MODLOG_DFLT(DEBUG, "Peer address: ");
    for (int i = 0; i < 6; i++) {
        MODLOG_DFLT(DEBUG, "%d ", addr.val[i]);
    }
}

static void
getFilename(char *buffer, uint8_t * hash)
{
    sprintf(buffer, "%s%02x%02x%02x%02x", GATT_CACHE_PREFIX,
            hash[0], hash[1], hash[2], hash[3]);
}

static int
cacheErase(cache_handle_t handle)
{
    if (cache_fn.erase_all) {
        cache_fn.erase_all(handle);
    }
    return -1;
}

static int
cacheWrite(cache_handle_t handle, const char * key, const void* value, size_t length)
{
    if (cache_fn.write) {
        return cache_fn.write(handle, key, value, length);
    }
    return -1;
}

static int
cacheRead(cache_handle_t handle, const char * key, void* out_value, size_t* length)
{
    if (cache_fn.read) {
        return cache_fn.read(handle, key, out_value, length);
    }
    return -1;
}

static void
cacheClose(ble_addr_t addr)
{
    uint8_t index = 0;
    if ((index = ble_gattc_cache_find_addr(addr)) != INVALID_ADDR_NUM) {
        if (cache_env->cache_addr[index].is_open) {
            if (cache_fn.close) {
                cache_fn.close(cache_env->cache_addr[index].cache_fp);
            }
            cache_env->cache_addr[index].is_open = false;
        }
    }
}

static bool
cacheOpen(ble_addr_t addr, bool to_save, uint8_t *index)
{
    char fname[255] = {0};
    int status = -1;
    uint8_t hash_key[16] = {0};

    if ((*index = ble_gattc_cache_find_addr(addr)) != INVALID_ADDR_NUM) {
        if (cache_env->cache_addr[*index].is_open) {
            return true;
        } else {
            memcpy(hash_key, cache_env->cache_addr[*index].hash_key, sizeof(uint8_t) * 16);
            getFilename(fname, hash_key);
            if (cache_fn.open) {
                if ((status = cache_fn.open(fname, READWRITE, &cache_env->cache_addr[*index].cache_fp)) == 0) {
                    /* Set the open flag to TRUE when success to open the hash file. */
                    cache_env->cache_addr[*index].is_open = true;
                }
            }
        }
    }
    return ((status == 0) ? true : false);
}

void
ble_gattc_cacheReset(ble_addr_t *addr)
{
    uint8_t index = 0;
    if (addr == NULL) {
        BLE_HS_LOG(ERROR, "%s Cannot reset cache with null addr", __func__);
        return;
    }

    if ((index = ble_gattc_cache_find_addr(*addr)) != INVALID_ADDR_NUM) {
        if (cache_env->cache_addr[index].is_open) {
            cacheErase(cache_env->cache_addr[index].cache_fp);

            if (cache_fn.close) {
                cache_fn.close(cache_env->cache_addr[index].cache_fp);
            }
            cache_env->cache_addr[index].is_open = false;
            BLE_HS_LOG(DEBUG, "%s erased peer entry from NVS");
        } else {
            cacheOpen(*addr, false, &index);
            if (index == INVALID_ADDR_NUM) {
                BLE_HS_LOG(ERROR, "%s INVALID ADDR NUM", __func__);
                return;
            }
            if (cache_env->cache_addr[index].is_open) {
                cacheErase(cache_env->cache_addr[index].cache_fp);
                if (cache_fn.close) {
                    cache_fn.close(cache_env->cache_addr[index].cache_fp);
                }
                cache_env->cache_addr[index].is_open = false;
                BLE_HS_LOG(DEBUG, "%s erased peer entry from NVS");
            } else {
                BLE_HS_LOG(ERROR, "%s cacheOpen failed", __func__);
                return;
            }
        }
        if (cache_env->num_addr == 0) {
            BLE_HS_LOG(ERROR, "%s cache addr list error", __func__);
            return;
        }

        uint8_t num = cache_env->num_addr;

        /* Delete the server_bda in the addr_info list. */
        for (uint8_t i = index; i < (num - 1); i++) {
            memcpy(&cache_env->cache_addr[i], &cache_env->cache_addr[i + 1],
                   sizeof(cache_addr_info_t));
        }

        /* Reduced the number address counter also */
        cache_env->num_addr--;

        /* Update addr list to storage flash */
        if (cache_env->num_addr > 0) {
            uint8_t *p_buf = nimble_platform_mem_malloc(MAX_ADDR_LIST_CACHE_BUF);
            if (!p_buf) {
                BLE_HS_LOG(ERROR, "%s malloc error", __func__);
                return;
            }

            uint16_t length = cache_env->num_addr * (sizeof(ble_addr_t) + sizeof(uint8_t) * 16);

            for (uint8_t i = 0; i < cache_env->num_addr; i++) {

                /* Copy the address to the buffer. */
                memcpy((p_buf + i * (sizeof(ble_addr_t) + sizeof(uint8_t) * 16)),
                       &cache_env->cache_addr[i].addr, sizeof(ble_addr_t));

                /* Copy the hash key to the buffer.*/
                memcpy(p_buf + i * (sizeof(ble_addr_t) + sizeof(uint8_t) * 16) + sizeof(ble_addr_t),
                       cache_env->cache_addr[i].hash_key, sizeof(uint8_t) * 16);
            }

            if (cache_env->is_open) {
                if (cacheWrite(cache_env->addr_fp, cache_key, p_buf, length) != 0) {
                    BLE_HS_LOG(INFO, "%s, storage set blob failed", __func__);
                }
            }
            nimble_platform_mem_free(p_buf);

        } else {
            if (cache_env->is_open) {
                cacheErase(cache_env->addr_fp);
                if (cache_fn.close) {
                    cache_fn.close(cache_env->addr_fp);
                }
                cache_env->is_open = false;
                BLE_HS_LOG(DEBUG, "%s erased entire cache from NVS");
            } else {
                BLE_HS_LOG(INFO, "cache_env status is error");
            }
        }
    }
}

static uint8_t
ble_gattc_cache_find_addr(ble_addr_t addr)
{
    uint8_t addr_index = 0;
    uint8_t num = cache_env->num_addr;
    cache_addr_info_t *addr_info = &cache_env->cache_addr[0];

    for (addr_index = 0; addr_index < num; addr_index++, addr_info++) {
        if (!memcmp(&addr_info->addr, &addr, sizeof(ble_addr_t))) {
            return addr_index;
        }
    }

    return INVALID_ADDR_NUM;
}

static uint8_t
ble_gattc_cache_find_hash(uint8_t * hash_key)
{
    uint8_t index = 0;
    uint8_t num = cache_env->num_addr;
    cache_addr_info_t *addr_info;
    for (index = 0; index < num; index++) {
        addr_info = &cache_env->cache_addr[index];
        if (memcmp(addr_info->hash_key, hash_key, sizeof(uint8_t) * 16) == 0) {
            return index;
        }
    }

    return INVALID_ADDR_NUM;
}

static void check_uuid(ble_uuid_any_t *uuid)
{
    ble_uuid16_t u16;
    ble_uuid32_t u32;
    switch (uuid->u.type) {
    case BLE_UUID_TYPE_16:
        u16 = *(const ble_uuid16_t *)uuid;
        memset(uuid->u128.value, 0, sizeof(uuid->u128.value));
        uuid->u16.u.type = u16.u.type;
        uuid->u16.value = u16.value;
        break;
    case BLE_UUID_TYPE_32:
        u32 = *(const ble_uuid32_t *)uuid;
        memset(uuid->u128.value, 0, sizeof(uuid->u128.value));
        uuid->u32.u.type = u32.u.type;
        uuid->u32.value = u32.value;
        break;
    case BLE_UUID_TYPE_128:
        break;
    default:
        assert(0);
        break;
    }
}

static void
ble_gattc_fill_nv_attr_entry(ble_uuid_any_t uuid, uint16_t s_handle, uint16_t e_handle,
                             ble_gatt_attr_type attr_type, bool is_primary,
                             struct ble_gatt_nv_attr *nv_attr, int index, uint8_t properties)
{
    ble_uuid_copy(&nv_attr[index].uuid, &uuid.u);
    check_uuid(&nv_attr[index].uuid);
    nv_attr[index].s_handle = s_handle;
    nv_attr[index].e_handle = e_handle;
    nv_attr[index].is_primary = is_primary;
    nv_attr[index].attr_type = attr_type;
    nv_attr[index].properties = properties;

    if (s_handle >= svc_end_handle) {
        svc_end_handle = s_handle;
    }

    BLE_HS_LOG(DEBUG, "uuid = %x, s = %d, e = %d, is_primary = %d, attr_type = %d, prop =%d",
               uuid.u16.value, s_handle, e_handle, is_primary, attr_type, properties);
}

static void
ble_gattc_fill_nv_attr(struct ble_gattc_cache_conn *peer, size_t num_attr, struct ble_gatt_nv_attr *nv_attr)
{
    struct ble_gattc_cache_conn_svc *svc;
    struct ble_gattc_cache_conn_chr *chr;
    struct ble_gattc_cache_conn_dsc *dsc;

    int index = 0;
    int svc_index = 0;

    SLIST_FOREACH(svc, &peer->svcs, next) {
        ble_gattc_fill_nv_attr_entry(svc->svc.uuid, svc->svc.start_handle, svc->svc.end_handle,
                                     BLE_GATT_ATTR_TYPE_SRVC, svc->type == BLE_GATT_SVC_TYPE_PRIMARY,
                                     nv_attr, index, 0);
        svc_index = index;
        index++;
        SLIST_FOREACH(chr, &svc->chrs, next) {
            ble_gattc_fill_nv_attr_entry(chr->chr.uuid, chr->chr.val_handle,
                                         chr->chr.def_handle, BLE_GATT_ATTR_TYPE_CHAR, false,
                                         nv_attr, index, chr->chr.properties);
            index++;
            SLIST_FOREACH(dsc, &chr->dscs, next) {
                ble_gattc_fill_nv_attr_entry(dsc->dsc.uuid, dsc->dsc.handle, 0,
                                             BLE_GATT_ATTR_TYPE_CHAR_DESCR, false, nv_attr, index,
                                             0);
                index++;
            }
        }

        if (svc->svc.end_handle == 65535) {
            nv_attr[svc_index].e_handle = svc_end_handle;
        }
    }
}

static int
ble_gattc_cache_addr_save(uint8_t *out_index, ble_addr_t addr, uint8_t * hash_key)
{
    int rc;
    uint8_t num = ++(cache_env->num_addr);
    uint8_t index = 0;
    uint8_t insert_ind = 0;
    uint8_t i = 0;
    uint8_t *p_buf;

    p_buf = nimble_platform_mem_malloc(MAX_ADDR_LIST_CACHE_BUF);
    if (p_buf == NULL) {
        return BLE_HS_ENOMEM;
    }

    /* Check the address list has the same hash key or not */
    if (ble_gattc_cache_find_hash(hash_key) != INVALID_ADDR_NUM) {
        BLE_HS_LOG(DEBUG, "Hash key already present in the cache list");

        if ((index = ble_gattc_cache_find_addr(addr)) != INVALID_ADDR_NUM) {
            BLE_HS_LOG(DEBUG, "tBD address already present in the cache list");

            /* If the bd_addr already in the address list, update the hash key in it. */
            insert_ind = index;
        } else {
            /*
                If the bd_addr isn't in the address list, added the bd_addr to the last of the
                address list.
             */
            if(num > MYNEWT_VAL(BLE_GATT_CACHING_MAX_CONNS)) {
                nimble_platform_mem_free(p_buf);
                return BLE_HS_ENOMEM;
            }
            BLE_HS_LOG(DEBUG, "BD addr not present");
            insert_ind = num - 1;
        }
    } else {
        BLE_HS_LOG(DEBUG, "Hash key not present, saving new data");
        if(num > MYNEWT_VAL(BLE_GATT_CACHING_MAX_CONNS)) {
            nimble_platform_mem_free(p_buf);
            return BLE_HS_ENOMEM;
        }
        insert_ind = num - 1;
    }

    print_hash_key(hash_key);
    print_addr(addr);

    memcpy(cache_env->cache_addr[insert_ind].hash_key, hash_key, sizeof(uint8_t) * 16);
    memcpy(&cache_env->cache_addr[insert_ind].addr, &addr, sizeof(ble_addr_t));

    cache_handle_t *fp = &cache_env->addr_fp;
    uint16_t length = num * (sizeof(ble_addr_t) + (sizeof(uint8_t) * 16));

    for (i = 0; i < num; i++) {

        memcpy(p_buf + i * (sizeof(ble_addr_t) + sizeof(uint8_t) * 16),
               &cache_env->cache_addr[i].addr, sizeof(ble_addr_t));

        memcpy(p_buf + i * (sizeof(ble_addr_t) + sizeof(uint8_t) * 16) + sizeof(ble_addr_t),
               cache_env->cache_addr[i].hash_key, sizeof(uint8_t) * 16);
    }

    if (cache_env->is_open) {
        BLE_HS_LOG(DEBUG, "NVS Opened already");

        rc = cacheWrite(cache_env->addr_fp, cache_key, p_buf, length);
        if (rc != 0) {
            BLE_HS_LOG(ERROR, "storage set blob fail, err %d", rc);
        }
    } else {
        rc = cache_fn.open(cache_addr, READWRITE, fp);

        if (rc == 0) {
            cache_env->is_open = true;
            rc = cacheWrite(cache_env->addr_fp, cache_key, p_buf, length);

            if (rc != 0) {
                BLE_HS_LOG(ERROR, "storage set blob fail, err %d", rc);
            }
        } else {
            BLE_HS_LOG(ERROR, "Line = %d, storage flash open fail, err_code = %x",
                       __LINE__, rc);
        }
    }
    nimble_platform_mem_free(p_buf);
    if(out_index) {
        *out_index = insert_ind;
    }
    return rc;
}

static int
handle_compare(const void *s1, const void *s2)
{
    return ((struct ble_gatt_nv_attr *)s1)->s_handle - ((struct ble_gatt_nv_attr *)s2)->s_handle;
}

static void
ble_gatts_sort_nv_attr(struct ble_gatt_nv_attr *nv_attr, size_t num_attr)
{
    qsort(nv_attr, (num_attr), sizeof(struct ble_gatt_nv_attr), handle_compare);
}

void
ble_gattc_cache_save(struct ble_gattc_cache_conn *peer, size_t num_attr)
{
    int rc = 0;
    uint8_t hash_key[16] = {0};
    uint8_t index = INVALID_ADDR_NUM;
    struct ble_gatt_nv_attr *nv_attr;

    nv_attr = (struct ble_gatt_nv_attr *) nimble_platform_mem_malloc(num_attr * sizeof(ble_gatt_nv_attr));
    if (nv_attr == NULL) {
        BLE_HS_LOG(DEBUG, "Failed to allocate memory to nv_attr");
        return;
    }
    memset(nv_attr, 0, num_attr * sizeof(ble_gatt_nv_attr));

    ble_gattc_fill_nv_attr(peer, num_attr, nv_attr);

    ble_gatts_sort_nv_attr(nv_attr, num_attr);

    memcpy(hash_key, &peer->database_hash, sizeof(uint8_t) * 16);

    rc = ble_gattc_cache_addr_save(&index, peer->ble_gattc_cache_conn_addr, hash_key);
    if(rc != 0) {
        /* cannot save address, return */
        BLE_HS_LOG(ERROR, "Failed to save cache %d", rc);
        nimble_platform_mem_free(nv_attr);
        return;
    }

    if (cacheOpen(peer->ble_gattc_cache_conn_addr, true, &index)) {
        BLE_HS_LOG(DEBUG, "Cache Opened already \n\tWriting cache_fp and cache_key on index = %d",
                   index);
        rc = cacheWrite(cache_env->cache_addr[index].cache_fp, cache_key, nv_attr,
                        num_attr * sizeof(struct ble_gatt_nv_attr));
    } else {
        rc = -1;
    }

    BLE_HS_LOG(INFO, "%s() wrote hash_key on index = %d, num_attr = %d, status = %d.", __func__,
               index, num_attr, rc);

    nimble_platform_mem_free(nv_attr);
    cacheClose(peer->ble_gattc_cache_conn_addr);
}

static struct ble_gatt_nv_attr *
ble_gattc_cache_load_nv_attr(uint8_t index, int *num_attr)
{
    int rc;
    size_t length = 0;
    struct ble_gatt_nv_attr *nv_attr;

    cacheRead(cache_env->cache_addr[index].cache_fp, cache_key, NULL, &length);

    *num_attr = length / (sizeof(ble_gatt_nv_attr));

    nv_attr = (struct ble_gatt_nv_attr *) nimble_platform_mem_malloc((*num_attr) * sizeof(struct ble_gatt_nv_attr));
    if (nv_attr == NULL) {
        return NULL;
    }

    rc = cacheRead(cache_env->cache_addr[index].cache_fp, cache_key, nv_attr, &length);

    BLE_HS_LOG(INFO, "%s, rc = %d, length = %d index = %d", __func__, rc, length, index);
    return nv_attr;
}

static int
ble_gattc_add_svc_from_cache(ble_addr_t peer_addr, struct ble_gatt_nv_attr nv_attr)
{
    struct ble_gatt_svc *gatt_svc;
    int rc;

    gatt_svc = (struct ble_gatt_svc *)nimble_platform_mem_malloc(sizeof(struct ble_gatt_svc));
    if (gatt_svc == NULL) {
        return BLE_HS_ENOMEM;
    }

    gatt_svc->start_handle = nv_attr.s_handle;
    gatt_svc->end_handle = nv_attr.e_handle;
    ble_uuid_copy(&gatt_svc->uuid, &nv_attr.uuid.u);

    rc = ble_gattc_cache_conn_svc_add(peer_addr, gatt_svc);
    nimble_platform_mem_free(gatt_svc);
    return rc;
}

static int
ble_gattc_add_inc_from_cache(ble_addr_t peer_addr, struct ble_gatt_nv_attr nv_attr)
{
    struct ble_gatt_svc *gatt_svc;
    int rc;

    gatt_svc = (struct ble_gatt_svc *)nimble_platform_mem_malloc(sizeof(struct ble_gatt_svc));
    if (gatt_svc == NULL) {
        return BLE_HS_ENOMEM;
    }

    gatt_svc->start_handle = nv_attr.s_handle;
    gatt_svc->end_handle = nv_attr.e_handle;
    ble_uuid_copy(&gatt_svc->uuid, &nv_attr.uuid.u);
    rc = ble_gattc_cache_conn_inc_add(peer_addr, gatt_svc);
    nimble_platform_mem_free(gatt_svc);
    return rc;
}

static int
ble_gattc_add_chr_from_cache(ble_addr_t peer_addr, struct ble_gatt_nv_attr nv_attr)
{
    struct ble_gatt_chr *gatt_chr;
    int rc;
    gatt_chr = (struct ble_gatt_chr *)nimble_platform_mem_malloc(sizeof(struct ble_gatt_chr));
    if (gatt_chr == NULL) {
        return BLE_HS_ENOMEM;
    }

    gatt_chr->val_handle = nv_attr.s_handle;
    gatt_chr->def_handle = nv_attr.e_handle;
    ble_uuid_copy(&gatt_chr->uuid, &nv_attr.uuid.u);
    gatt_chr->properties = nv_attr.properties;

    rc = ble_gattc_cache_conn_chr_add(peer_addr, 0, gatt_chr);
    nimble_platform_mem_free(gatt_chr);
    return rc;
}

static int
ble_gattc_add_dsc_from_cache(ble_addr_t peer_addr, struct ble_gatt_nv_attr nv_attr)
{
    struct ble_gatt_dsc *gatt_dsc;
    int rc;
    gatt_dsc = (struct ble_gatt_dsc *)nimble_platform_mem_malloc(sizeof(struct ble_gatt_dsc));
    if (gatt_dsc == NULL) {
        return BLE_HS_ENOMEM;
    }

    gatt_dsc->handle = nv_attr.s_handle;
    ble_uuid_copy(&gatt_dsc->uuid, &nv_attr.uuid.u);

    rc = ble_gattc_cache_conn_dsc_add(peer_addr, 0, gatt_dsc);
    nimble_platform_mem_free(gatt_dsc);
    return rc;
}

int
ble_gattc_cache_load(ble_addr_t peer_addr)
{
    uint8_t index = 0;
    uint8_t cache_index = 0;
    int num_attr = 50;
    int rc = 0;
    struct ble_gatt_nv_attr *nv_attr;

    if (!cacheOpen(peer_addr, true, &index)) {
        BLE_HS_LOG(INFO, "gattc cache open fail");
        return BLE_HS_EINVAL;
    }

    if ((nv_attr = ble_gattc_cache_load_nv_attr(index, &num_attr)) == NULL) {
        BLE_HS_LOG(INFO, "%s, gattc cache nv_attr load fail", __func__);
        return BLE_HS_EINVAL;
    }

    for (int i = 0; i < num_attr; i++) {
        switch (nv_attr[i].attr_type) {
        case BLE_GATT_ATTR_TYPE_SRVC:
            if (nv_attr[i].is_primary) {
                rc = ble_gattc_add_svc_from_cache(peer_addr, nv_attr[i]);
            } else {
                rc = ble_gattc_add_inc_from_cache(peer_addr, nv_attr[i]);
            }
            break;

        case BLE_GATT_ATTR_TYPE_CHAR:
            rc = ble_gattc_add_chr_from_cache(peer_addr, nv_attr[i]);
            break;

        case BLE_GATT_ATTR_TYPE_CHAR_DESCR:
            rc = ble_gattc_add_dsc_from_cache(peer_addr, nv_attr[i]);
            break;

        default:
            break;
        }
    }
    nimble_platform_mem_free(nv_attr);
    cache_index = ble_gattc_cache_find_addr(peer_addr);
    ble_gattc_cache_conn_load_hash(cache_env->cache_addr[cache_index].addr,
                                   cache_env->cache_addr[cache_index].hash_key);
    return rc;
}

int
ble_gattc_cache_check_hash(struct ble_gattc_cache_conn *peer, struct os_mbuf *om)
{
    if (peer == NULL || om == NULL) {
        BLE_HS_LOG(ERROR, "Check hash failed");
        return -1;
    }
    if (memcmp(peer->database_hash, om->om_data, om->om_len) == 0) {
        return 0;
    }
    return -1;
}

int
ble_gattc_cache_init(void *storage_cb)
{
    /* Point where to store data */
    cache_fn = link_storage_fn(storage_cb);

    cache_handle_t fp;
    int rc = 0;
    uint8_t num_addr;
    size_t length = MAX_ADDR_LIST_CACHE_BUF;

    svc_end_handle = 0;

    uint8_t *p_buf = nimble_platform_mem_malloc(MAX_ADDR_LIST_CACHE_BUF);
    if (p_buf == NULL) {
        BLE_HS_LOG(ERROR, "%s malloc failed!", __func__);
        rc = BLE_HS_ENOMEM;
        return rc;
    }

    cache_env = (cache_env_t *)nimble_platform_mem_malloc(sizeof(cache_env_t));
    if (cache_env == NULL) {
        BLE_HS_LOG(ERROR, "%s malloc failed!", __func__);
        nimble_platform_mem_free(p_buf);
        rc = BLE_HS_ENOMEM;
        return rc;
    }

    memset(cache_env, 0x0, sizeof(cache_env_t));

    if (cache_fn.open) {
        if ((rc = cache_fn.open(cache_addr, READWRITE, &fp)) == 0) {
            cache_env->addr_fp = fp;
            cache_env->is_open = true;

            /* Read previously saved blob if available */
            if ((rc = cacheRead(fp, cache_key, p_buf, &length)) != 0) {
                if (rc != 0) {
                    BLE_HS_LOG(DEBUG, "%s, Line = %d, storage flash get blob data fail, err_code = 0x%x",
                               __func__, __LINE__, rc);
                }
                nimble_platform_mem_free(p_buf);
                return rc;
            }

            num_addr = length / (sizeof(ble_addr_t) + sizeof(uint8_t) * 16);
            cache_env->num_addr = num_addr;
            BLE_HS_LOG(DEBUG, "Number of address loaded = %d", cache_env->num_addr);

            /* Read the address from storage flash to cache address list. */
            for (uint8_t i = 0; i < num_addr; i++) {

                memcpy(&cache_env->cache_addr[i].addr,
                       p_buf + i * (sizeof(ble_addr_t) + sizeof(uint8_t) * 16), sizeof(ble_addr_t));

                memcpy(cache_env->cache_addr[i].hash_key,
                       p_buf + i * (sizeof(ble_addr_t) + sizeof(uint8_t) * 16) + sizeof(ble_addr_t),
                       sizeof(uint8_t) * 16);

                print_addr(cache_env->cache_addr[i].addr);
                print_hash_key(cache_env->cache_addr[i].hash_key);
            }
        }
    } else {
        BLE_HS_LOG(ERROR, "%s, Line = %d, storage flash open fail, err_code = %x", __func__, __LINE__,
                   rc);
        nimble_platform_mem_free(p_buf);
        return rc;
    }

    nimble_platform_mem_free(p_buf);
    return 0;
}
#endif /* MYNEWT_VAL(BLE_GATT_CACHING) */
