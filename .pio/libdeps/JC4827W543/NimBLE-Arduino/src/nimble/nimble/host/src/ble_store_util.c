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

#include "nimble/nimble/host/include/host/ble_store.h"
#include "ble_hs_priv.h"
#include "ble_hs_resolv_priv.h"

struct ble_store_util_peer_set {
    ble_addr_t *peer_id_addrs;
    int num_peers;
    int max_peers;
    int status;
};

static int
ble_store_util_iter_unique_peer(int obj_type,
                                union ble_store_value *val,
                                void *arg)
{
    struct ble_store_util_peer_set *set;
    int i;

    BLE_HS_DBG_ASSERT(obj_type == BLE_STORE_OBJ_TYPE_OUR_SEC ||
#if MYNEWT_VAL(ENC_ADV_DATA)
                      obj_type == BLE_STORE_OBJ_TYPE_ENC_ADV_DATA ||
#endif
                      obj_type == BLE_STORE_OBJ_TYPE_PEER_SEC || obj_type == BLE_STORE_OBJ_TYPE_LOCAL_IRK);

    set = arg;

    /* Do nothing if this peer is a duplicate. */
    for (i = 0; i < set->num_peers; i++) {
        if (ble_addr_cmp(set->peer_id_addrs + i, &val->sec.peer_addr) == 0) {
            return 0;
        }
    }

    if (set->num_peers >= set->max_peers) {
        /* Overflow; abort the iterate procedure. */
        set->status = BLE_HS_ENOMEM;
        return 1;
    }

    set->peer_id_addrs[set->num_peers] = val->sec.peer_addr;
    set->num_peers++;

    return 0;
}

/**
 * Retrieves the set of peer addresses for which a bond has been established.
 *
 * @param out_peer_id_addrs     On success, the set of bonded peer addresses
 *                                  gets written here.
 * @param out_num_peers         On success, the number of bonds gets written
 *                                  here.
 * @param max_peers             The capacity of the destination buffer.
 *
 * @return                      0 on success;
 *                              BLE_HS_ENOMEM if the destination buffer is too
 *                                  small;
 *                              Other nonzero on error.
 */
int
ble_store_util_bonded_peers(ble_addr_t *out_peer_id_addrs, int *out_num_peers,
                            int max_peers)
{
    struct ble_store_util_peer_set set = {
        .peer_id_addrs = out_peer_id_addrs,
        .num_peers = 0,
        .max_peers = max_peers,
        .status = 0,
    };
    int rc;

    rc = ble_store_iterate(BLE_STORE_OBJ_TYPE_OUR_SEC,
                           ble_store_util_iter_unique_peer,
                           &set);
    if (rc != 0) {
        return rc;
    }

    *out_num_peers = set.num_peers;
    return 0;
}

/**
 * Deletes all entries from the store that are attached to the specified peer
 * address.  This function deletes security entries and CCCD records.
 *
 * @param peer_id_addr          Entries with this peer address get deleted.
 *
 * @return                      0 on success;
 *                              Other nonzero on error.
 */
int
ble_store_util_delete_peer(const ble_addr_t *peer_id_addr)
{
    union ble_store_key key;
    int rc;

    memset(&key, 0, sizeof key);
    key.sec.peer_addr = *peer_id_addr;

    rc = ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_OUR_SEC, &key);
    if (rc != 0) {
        return rc;
    }

    rc = ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_PEER_SEC, &key);
    if (rc != 0) {
        return rc;
    }

    memset(&key, 0, sizeof key);
    key.cccd.peer_addr = *peer_id_addr;

    rc = ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_CCCD, &key);
    if (rc != 0) {
        return rc;
    }

#if MYNEWT_VAL(ENC_ADV_DATA)
    memset(&key, 0, sizeof key);
    key.ead.peer_addr = *peer_id_addr;

    rc = ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_ENC_ADV_DATA, &key);
    if (rc != 0) {
        return rc;
    }
#endif

    memset(&key, 0, sizeof key);
    key.rpa_rec.peer_rpa_addr = *peer_id_addr;

    rc = ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_PEER_ADDR, &key);
    if (rc != 0) {
        return rc;
    }

    memset(&key, 0, sizeof key);
    key.csfc.peer_addr = *peer_id_addr;

    rc = ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_CSFC, &key);
    if (rc != 0) {
        return rc;
    }

#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
    struct ble_hs_dev_records *peer_rec =
                              ble_rpa_find_peer_dev_rec(key.sec.peer_addr.val);

    if (peer_rec != NULL) {
        rc = ble_hs_resolv_list_rmv(peer_rec->peer_sec.peer_addr.type,
                                    peer_rec->peer_sec.peer_addr.val);
        if (rc != 0) {
            /* We can't do anything much here, continue with removing from peer_record  */
            BLE_HS_LOG(DEBUG, "Peer Device was not removed from RL \n");
        }

        rc = ble_rpa_remove_peer_dev_rec(peer_rec);
        if (rc != 0) {
            return rc;
        }
    }
#endif

    return 0;
}

/**
 * Deletes all entries from the store that match the specified key.
 *
 * @param type                  The type of store entry to delete.
 * @param key                   Entries matching this key get deleted.
 *
 * @return                      0 on success;
 *                              Other nonzero on error.
 */
int
ble_store_util_delete_all(int type, const union ble_store_key *key)
{
    int rc;

    do {
        rc = ble_store_delete(type, key);
    } while (rc == 0);

    if (rc != BLE_HS_ENOENT) {
        return rc;
    }

    return 0;
}

static int
ble_store_util_iter_count(int obj_type,
                          union ble_store_value *val,
                          void *arg)
{
    int *count;

    count = arg;
    (*count)++;

    return 0;
}

int
ble_store_util_count(int type, int *out_count)
{
    int rc;

    *out_count = 0;
    rc = ble_store_iterate(type,
                           ble_store_util_iter_count,
                           out_count);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
ble_store_util_delete_oldest_peer(void)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    ble_addr_t peer_id_addrs[MYNEWT_VAL(BLE_STORE_MAX_BONDS)];
    int num_peers;
    int rc;

    rc = ble_store_util_bonded_peers(
            peer_id_addrs, &num_peers,
            sizeof peer_id_addrs / sizeof peer_id_addrs[0]);
    if (rc != 0) {
        return rc;
    }

    if (num_peers == 0) {
        return 0;
    }

    rc = ble_store_util_delete_peer(&peer_id_addrs[0]);
    if (rc != 0) {
        return rc;
    }
#endif
    return 0;
}

#if MYNEWT_VAL(ENC_ADV_DATA)
/**
* Retrieves the set of peer addresses for which a ead has been established.
*
* @param out_peer_id_addrs     On success, the set of ead peer addresses
*                              gets written here.
* @param out_num_peers         On success, the number of eads gets written
*                              here.
* @param max_peers             The capacity of the destination buffer.
*
* @return                      0 on success;
*                              BLE_HS_ENOMEM if the destination buffer is too
*                              small;
*                              Other nonzero on error.
*/
int
ble_store_util_ead_peers(ble_addr_t *out_peer_id_addrs, int *out_num_peers,
                         int max_peers)
{
   struct ble_store_util_peer_set set = {
       .peer_id_addrs = out_peer_id_addrs,
       .num_peers = 0,
       .max_peers = max_peers,
       .status = 0,
   };
   int rc;

   rc = ble_store_iterate(BLE_STORE_OBJ_TYPE_ENC_ADV_DATA,
                          ble_store_util_iter_unique_peer,
                          &set);
   if (rc != 0) {
       return rc;
   }

   *out_num_peers = set.num_peers;
   return 0;
}


int
ble_store_util_delete_ead_oldest_peer(void)
{
   ble_addr_t peer_id_addrs[MYNEWT_VAL(BLE_STORE_MAX_EADS)];
   int num_peers;
   int rc;
   union ble_store_key key;
   memset(&key, 0, sizeof key);

   rc = ble_store_util_ead_peers(
           peer_id_addrs, &num_peers,
           sizeof peer_id_addrs / sizeof peer_id_addrs[0]);
   if (rc != 0) {
       return rc;
   }

   if (num_peers == 0) {
       return 0;
   }

   key.ead.peer_addr = peer_id_addrs[0];
   rc = ble_store_util_delete_all(BLE_STORE_OBJ_TYPE_ENC_ADV_DATA, &key);
   if (rc != 0) {
       return rc;
   }

   return 0;
}
#endif

/**
 * Round-robin status callback.  If a there is insufficient storage capacity
 * for a new record, delete the oldest bond and proceed with the persist
 * operation.
 *
 * Note: This is not the best behavior for an actual product because
 * uninteresting peers could cause important bonds to be deleted.  This is
 * useful for demonstrations and sample apps.
 */
int
ble_store_util_status_rr(struct ble_store_status_event *event, void *arg)
{
    switch (event->event_code) {
    case BLE_STORE_EVENT_OVERFLOW:
        switch (event->overflow.obj_type) {
        case BLE_STORE_OBJ_TYPE_OUR_SEC:
        case BLE_STORE_OBJ_TYPE_PEER_SEC:
        case BLE_STORE_OBJ_TYPE_PEER_ADDR:
            return ble_gap_unpair_oldest_peer();
        case BLE_STORE_OBJ_TYPE_CCCD:
        case BLE_STORE_OBJ_TYPE_CSFC:
            /* Try unpairing oldest peer except current peer */
            return ble_gap_unpair_oldest_except(&event->overflow.value->cccd.peer_addr);
#if MYNEWT_VAL(ENC_ADV_DATA)
        case BLE_STORE_OBJ_TYPE_ENC_ADV_DATA:
            return ble_store_util_delete_ead_oldest_peer();
#endif
        default:
            return BLE_HS_EUNKNOWN;
        }

    case BLE_STORE_EVENT_FULL:
        /* Just proceed with the operation.  If it results in an overflow,
         * we'll delete a record when the overflow occurs.
         */
        return 0;

    default:
        return BLE_HS_EUNKNOWN;
    }
}
