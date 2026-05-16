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

/**
 * This file implements a simple in-RAM key database for BLE host security
 * material and CCCDs.  As this database is only ble_store_ramd in RAM, its
 * contents are lost when the application terminates.
 */

/* This package has been deprecated and you should
 * use the store/config package. For a RAM-only BLE store,
 * use store/config and set BLE_STORE_CONFIG_PERSIST to 0.
 */

#include <inttypes.h>
#include <string.h>

#include "nimble/porting/nimble/include/sysinit/sysinit.h"
#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/nimble/host/include/host/ble_hs.h"
#include "nimble/nimble/host/store/ram/include/store/ram/ble_store_ram.h"

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
static struct ble_store_value_sec
    ble_store_ram_our_secs[MYNEWT_VAL(BLE_STORE_MAX_BONDS)];
#endif

static int ble_store_ram_num_our_secs;

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
static struct ble_store_value_sec
    ble_store_ram_peer_secs[MYNEWT_VAL(BLE_STORE_MAX_BONDS)];
#endif

static int ble_store_ram_num_peer_secs;

#if MYNEWT_VAL(BLE_STORE_MAX_CCCDS)
static struct ble_store_value_cccd
    ble_store_ram_cccds[MYNEWT_VAL(BLE_STORE_MAX_CCCDS)];
#endif

static int ble_store_ram_num_cccds;

#if MYNEWT_VAL(BLE_STORE_MAX_CSFCS)
static struct ble_store_value_csfc
    ble_store_ram_csfcs[MYNEWT_VAL(BLE_STORE_MAX_CSFCS)];
#endif

static int ble_store_ram_num_csfcs;

#if MYNEWT_VAL(ENC_ADV_DATA)
static struct ble_store_value_ead
    ble_store_ram_eads[MYNEWT_VAL(BLE_STORE_MAX_EADS)];
static int ble_store_ram_num_eads;
#endif

/*****************************************************************************
 * $sec                                                                      *
 *****************************************************************************/

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
static void
ble_store_ram_print_value_sec(const struct ble_store_value_sec *sec)
{
    if (sec->ltk_present) {
        BLE_HS_LOG(DEBUG, "ediv=%u rand=%llu authenticated=%d ltk=",
                       sec->ediv, sec->rand_num, sec->authenticated);
        ble_hs_log_flat_buf(sec->ltk, 16);
        BLE_HS_LOG(DEBUG, " ");
    }
    if (sec->irk_present) {
        BLE_HS_LOG(DEBUG, "irk=");
        ble_hs_log_flat_buf(sec->irk, 16);
        BLE_HS_LOG(DEBUG, " ");
    }
    if (sec->csrk_present) {
        BLE_HS_LOG(DEBUG, "csrk=");
        ble_hs_log_flat_buf(sec->csrk, 16);
        BLE_HS_LOG(DEBUG, " ");
    }

    BLE_HS_LOG(DEBUG, "\n");
}
#endif

static void
ble_store_ram_print_key_sec(const struct ble_store_key_sec *key_sec)
{
    if (ble_addr_cmp(&key_sec->peer_addr, BLE_ADDR_ANY)) {
        BLE_HS_LOG(DEBUG, "peer_addr_type=%d peer_addr=",
                       key_sec->peer_addr.type);
        ble_hs_log_flat_buf(key_sec->peer_addr.val, 6);
        BLE_HS_LOG(DEBUG, " ");
    }
}

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
static int
ble_store_ram_find_sec(const struct ble_store_key_sec *key_sec,
                       const struct ble_store_value_sec *value_secs,
                       int num_value_secs)
{
    const struct ble_store_value_sec *cur;
    int i;

    if (!ble_addr_cmp(&key_sec->peer_addr, BLE_ADDR_ANY)) {
        if (key_sec->idx < num_value_secs) {
            return key_sec->idx;
        }
    } else if (key_sec->idx == 0) {
        for (i = 0; i < num_value_secs; i++) {
            cur = &value_secs[i];

            if (!ble_addr_cmp(&cur->peer_addr, &key_sec->peer_addr)) {
                return i;
            }
        }
    }

    return -1;
}
#endif

static int
ble_store_ram_read_our_sec(const struct ble_store_key_sec *key_sec,
                           struct ble_store_value_sec *value_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    int idx;

    idx = ble_store_ram_find_sec(key_sec, ble_store_ram_our_secs,
                                 ble_store_ram_num_our_secs);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    *value_sec = ble_store_ram_our_secs[idx];
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

static int
ble_store_ram_write_our_sec(const struct ble_store_value_sec *value_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    struct ble_store_key_sec key_sec;
    int idx;

    BLE_HS_LOG(DEBUG, "persisting our sec; ");
    ble_store_ram_print_value_sec(value_sec);

    ble_store_key_from_value_sec(&key_sec, value_sec);
    idx = ble_store_ram_find_sec(&key_sec, ble_store_ram_our_secs,
                                 ble_store_ram_num_our_secs);
    if (idx == -1) {
        if (ble_store_ram_num_our_secs >= MYNEWT_VAL(BLE_STORE_MAX_BONDS)) {
            BLE_HS_LOG(DEBUG, "error persisting our sec; too many entries "
                              "(%d)\n", ble_store_ram_num_our_secs);
            return BLE_HS_ESTORE_CAP;
        }

        idx = ble_store_ram_num_our_secs;
        ble_store_ram_num_our_secs++;
    }

    ble_store_ram_our_secs[idx] = *value_sec;

    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS) || MYNEWT_VAL(BLE_STORE_MAX_CCCDS)
static int
ble_store_ram_delete_obj(void *values, int value_size, int idx,
                         int *num_values)
{
    uint8_t *dst;
    uint8_t *src;
    uint8_t move_count;

    (*num_values)--;
    if (idx < *num_values) {
        dst = values;
        dst += idx * value_size;
        src = dst + value_size;

        move_count = *num_values - idx;
        memmove(dst, src, move_count);
    }

    return 0;
}
#endif

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
static int
ble_store_ram_delete_sec(const struct ble_store_key_sec *key_sec,
                         struct ble_store_value_sec *value_secs,
                         int *num_value_secs)
{
    int idx;
    int rc;

    idx = ble_store_ram_find_sec(key_sec, value_secs, *num_value_secs);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    rc = ble_store_ram_delete_obj(value_secs, sizeof *value_secs, idx,
                                  num_value_secs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
#endif

static int
ble_store_ram_delete_our_sec(const struct ble_store_key_sec *key_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    int rc;

    rc = ble_store_ram_delete_sec(key_sec, ble_store_ram_our_secs,
                                  &ble_store_ram_num_our_secs);
    if (rc != 0) {
        return rc;
    }
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

static int
ble_store_ram_delete_peer_sec(const struct ble_store_key_sec *key_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    int rc;

    rc = ble_store_ram_delete_sec(key_sec, ble_store_ram_peer_secs,
                                  &ble_store_ram_num_peer_secs);
    if (rc != 0) {
        return rc;
    }
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

static int
ble_store_ram_read_peer_sec(const struct ble_store_key_sec *key_sec,
                            struct ble_store_value_sec *value_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    int idx;

    idx = ble_store_ram_find_sec(key_sec, ble_store_ram_peer_secs,
                             ble_store_ram_num_peer_secs);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    *value_sec = ble_store_ram_peer_secs[idx];
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

static int
ble_store_ram_write_peer_sec(const struct ble_store_value_sec *value_sec)
{
#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    struct ble_store_key_sec key_sec;
    int idx;

    BLE_HS_LOG(DEBUG, "persisting peer sec; ");
    ble_store_ram_print_value_sec(value_sec);

    ble_store_key_from_value_sec(&key_sec, value_sec);
    idx = ble_store_ram_find_sec(&key_sec, ble_store_ram_peer_secs,
                                 ble_store_ram_num_peer_secs);
    if (idx == -1) {
        if (ble_store_ram_num_peer_secs >= MYNEWT_VAL(BLE_STORE_MAX_BONDS)) {
            BLE_HS_LOG(DEBUG, "error persisting peer sec; too many entries "
                             "(%d)\n", ble_store_ram_num_peer_secs);
            return BLE_HS_ESTORE_CAP;
        }

        idx = ble_store_ram_num_peer_secs;
        ble_store_ram_num_peer_secs++;
    }

    ble_store_ram_peer_secs[idx] = *value_sec;
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

/*****************************************************************************
 * $cccd                                                                     *
 *****************************************************************************/

#if MYNEWT_VAL(BLE_STORE_MAX_CCCDS)
static int
ble_store_ram_find_cccd(const struct ble_store_key_cccd *key)
{
    struct ble_store_value_cccd *cccd;
    int skipped;
    int i;

    skipped = 0;
    for (i = 0; i < ble_store_ram_num_cccds; i++) {
        cccd = ble_store_ram_cccds + i;

        if (ble_addr_cmp(&key->peer_addr, BLE_ADDR_ANY)) {
            if (ble_addr_cmp(&cccd->peer_addr, &key->peer_addr)) {
                continue;
            }
        }

        if (key->chr_val_handle != 0) {
            if (cccd->chr_val_handle != key->chr_val_handle) {
                continue;
            }
        }

        if (key->idx > skipped) {
            skipped++;
            continue;
        }

        return i;
    }

    return -1;
}
#endif

static int
ble_store_ram_delete_cccd(const struct ble_store_key_cccd *key_cccd)
{
#if MYNEWT_VAL(BLE_STORE_MAX_CCCDS)
    int idx;
    int rc;

    idx = ble_store_ram_find_cccd(key_cccd);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    rc = ble_store_ram_delete_obj(ble_store_ram_cccds,
                                  sizeof *ble_store_ram_cccds,
                                  idx,
                                  &ble_store_ram_num_cccds);
    if (rc != 0) {
        return rc;
    }
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

static int
ble_store_ram_read_cccd(const struct ble_store_key_cccd *key_cccd,
                        struct ble_store_value_cccd *value_cccd)
{
#if MYNEWT_VAL(BLE_STORE_MAX_CCCDS)
    int idx;

    idx = ble_store_ram_find_cccd(key_cccd);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    *value_cccd = ble_store_ram_cccds[idx];

    return 0;
#else
    return BLE_HS_ENOENT;
#endif
}

static int
ble_store_ram_write_cccd(const struct ble_store_value_cccd *value_cccd)
{
#if MYNEWT_VAL(BLE_STORE_MAX_CCCDS)
    struct ble_store_key_cccd key_cccd;
    int idx;

    ble_store_key_from_value_cccd(&key_cccd, value_cccd);
    idx = ble_store_ram_find_cccd(&key_cccd);
    if (idx == -1) {
        if (ble_store_ram_num_cccds >= MYNEWT_VAL(BLE_STORE_MAX_CCCDS)) {
            BLE_HS_LOG(DEBUG, "error persisting cccd; too many entries (%d)\n",
                       ble_store_ram_num_cccds);
            return BLE_HS_ESTORE_CAP;
        }

        idx = ble_store_ram_num_cccds;
        ble_store_ram_num_cccds++;
    }

    ble_store_ram_cccds[idx] = *value_cccd;
    return 0;
#else
    return BLE_HS_ENOENT;
#endif

}

/*****************************************************************************
 * $csfc                                                                     *
 *****************************************************************************/

#if MYNEWT_VAL(BLE_STORE_MAX_CSFCS)
static int
ble_store_ram_find_csfc(const struct ble_store_key_csfc *key)
{
    struct ble_store_value_csfc *csfc;
    int skipped;
    int i;

    skipped = 0;
    for (i = 0; i < ble_store_ram_num_csfcs; i++) {
        csfc = ble_store_ram_csfcs + i;

        if (ble_addr_cmp(&key->peer_addr, BLE_ADDR_ANY)) {
            if (ble_addr_cmp(&csfc->peer_addr, &key->peer_addr)) {
                continue;
            }
        }

        if (key->idx > skipped) {
            skipped++;
            continue;
        }

        return i;
    }

    return -1;
}
#endif

static int
ble_store_ram_delete_csfc(const struct ble_store_key_csfc *key_csfc)
{
#if MYNEWT_VAL(BLE_STORE_MAX_CSFCS)
    int idx;
    int rc;

    idx = ble_store_ram_find_csfc(key_csfc);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    rc = ble_store_ram_delete_obj(ble_store_ram_csfcs,
                                  sizeof *ble_store_ram_csfcs,
                                  idx,
                                  &ble_store_ram_num_csfcs);
    if (rc != 0) {
        return rc;
    }
    return 0;
#else
    return BLE_HS_ENOENT;
#endif
}

static int
ble_store_ram_read_csfc(const struct ble_store_key_csfc *key_csfc,
                        struct ble_store_value_csfc *value_csfc)
{
#if MYNEWT_VAL(BLE_STORE_MAX_CSFCS)
    int idx;

    idx = ble_store_ram_find_csfc(key_csfc);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    *value_csfc = ble_store_ram_csfcs[idx];

    return 0;
#else
    return BLE_HS_ENOENT;
#endif
}

static int
ble_store_ram_write_csfc(const struct ble_store_value_csfc *value_csfc)
{
#if MYNEWT_VAL(BLE_STORE_MAX_CSFCS)
    struct ble_store_key_csfc key_csfc;
    int idx;

    ble_store_key_from_value_csfc(&key_csfc, value_csfc);
    idx = ble_store_ram_find_csfc(&key_csfc);
    if (idx == -1) {
        if (ble_store_ram_num_csfcs >= MYNEWT_VAL(BLE_STORE_MAX_CSFCS)) {
            BLE_HS_LOG(DEBUG, "error persisting csfc; too many entries (%d)\n",
                       ble_store_ram_num_csfcs);
            return BLE_HS_ESTORE_CAP;
        }

        idx = ble_store_ram_num_csfcs;
        ble_store_ram_num_csfcs++;
    }

    ble_store_ram_csfcs[idx] = *value_csfc;
    return 0;
#else
    return BLE_HS_ENOENT;
#endif
}

/*****************************************************************************
 * $ead                                                                     *
 *****************************************************************************/
#if MYNEWT_VAL(ENC_ADV_DATA)
static int
ble_store_ram_find_ead(const struct ble_store_key_ead *key)
{
    struct ble_store_value_ead *ead;
    int skipped;
    int i;

    skipped = 0;
    for (i = 0; i < ble_store_ram_num_eads; i++) {
        ead = ble_store_ram_eads + i;

        if (ble_addr_cmp(&key->peer_addr, BLE_ADDR_ANY)) {
            if (ble_addr_cmp(&ead->peer_addr, &key->peer_addr)) {
                continue;
            }
        }

        if (key->idx > skipped) {
            skipped++;
            continue;
        }

        return i;
    }

    return -1;
}

static int
ble_store_ram_delete_ead(const struct ble_store_key_ead *key_ead)
{
    int idx;
    int rc;

    idx = ble_store_ram_find_ead(key_ead);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }
    rc = ble_store_ram_delete_obj(ble_store_ram_eads,
                                  sizeof *ble_store_ram_eads,
                                  idx,
                                  &ble_store_ram_num_eads);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static int
ble_store_ram_read_ead(const struct ble_store_key_ead *key_ead,
                       struct ble_store_value_ead *value_ead)
{
    int idx;

    idx = ble_store_ram_find_ead(key_ead);
    if (idx == -1) {
        return BLE_HS_ENOENT;
    }

    *value_ead = ble_store_ram_eads[idx];
    return 0;
}

static int
ble_store_ram_write_ead(const struct ble_store_value_ead *value_ead)
{
    struct ble_store_key_ead key_ead;
    int idx;

    ble_store_key_from_value_ead(&key_ead, value_ead);
    idx = ble_store_ram_find_ead(&key_ead);
    if (idx == -1) {
        if (ble_store_ram_num_eads >= MYNEWT_VAL(BLE_STORE_MAX_EADS)) {
            BLE_HS_LOG(DEBUG, "error persisting ead; too many entries (%d)\n",
                       ble_store_ram_num_eads);
            return BLE_HS_ESTORE_CAP;
        }

        idx = ble_store_ram_num_eads;
        ble_store_ram_num_eads++;
    }

    ble_store_ram_eads[idx] = *value_ead;
    return 0;
}
#endif

/*****************************************************************************
 * $api                                                                      *
 *****************************************************************************/

/**
 * Searches the database for an object matching the specified criteria.
 *
 * @return                      0 if a key was found; else BLE_HS_ENOENT.
 */
int
ble_store_ram_read(int obj_type, const union ble_store_key *key,
                   union ble_store_value *value)
{
    int rc;

    switch (obj_type) {
    case BLE_STORE_OBJ_TYPE_PEER_SEC:
        /* An encryption procedure (bonding) is being attempted.  The nimble
         * stack is asking us to look in our key database for a long-term key
         * corresponding to the specified ediv and random number.
         *
         * Perform a key lookup and populate the context object with the
         * result.  The nimble stack will use this key if this function returns
         * success.
         */
        BLE_HS_LOG(DEBUG, "looking up peer sec; ");
        ble_store_ram_print_key_sec(&key->sec);
        BLE_HS_LOG(DEBUG, "\n");
        rc = ble_store_ram_read_peer_sec(&key->sec, &value->sec);
        return rc;

    case BLE_STORE_OBJ_TYPE_OUR_SEC:
        BLE_HS_LOG(DEBUG, "looking up our sec; ");
        ble_store_ram_print_key_sec(&key->sec);
        BLE_HS_LOG(DEBUG, "\n");
        rc = ble_store_ram_read_our_sec(&key->sec, &value->sec);
        return rc;

    case BLE_STORE_OBJ_TYPE_CCCD:
        rc = ble_store_ram_read_cccd(&key->cccd, &value->cccd);
        return rc;

    case BLE_STORE_OBJ_TYPE_CSFC:
        rc = ble_store_ram_read_csfc(&key->csfc, &value->csfc);
        return rc;

#if MYNEWT_VAL(ENC_ADV_DATA)
    case BLE_STORE_OBJ_TYPE_ENC_ADV_DATA:
        rc = ble_store_ram_read_ead(&key->ead, &value->ead);
        return rc;
#endif

    default:
        return BLE_HS_ENOTSUP;
    }
}

/**
 * Adds the specified object to the database.
 *
 * @return                      0 on success; BLE_HS_ESTORE_CAP if the database
 *                                  is full.
 */
int
ble_store_ram_write(int obj_type, const union ble_store_value *val)
{
    int rc;

    switch (obj_type) {
    case BLE_STORE_OBJ_TYPE_PEER_SEC:
        rc = ble_store_ram_write_peer_sec(&val->sec);
        return rc;

    case BLE_STORE_OBJ_TYPE_OUR_SEC:
        rc = ble_store_ram_write_our_sec(&val->sec);
        return rc;

    case BLE_STORE_OBJ_TYPE_CCCD:
        rc = ble_store_ram_write_cccd(&val->cccd);
        return rc;

    case BLE_STORE_OBJ_TYPE_CSFC:
        rc = ble_store_ram_write_csfc(&val->csfc);
        return rc;

#if MYNEWT_VAL(ENC_ADV_DATA)
    case BLE_STORE_OBJ_TYPE_ENC_ADV_DATA:
        rc = ble_store_ram_write_ead(&val->ead);
        return rc;
#endif

    default:
        return BLE_HS_ENOTSUP;
    }
}

int
ble_store_ram_delete(int obj_type, const union ble_store_key *key)
{
    int rc;

    switch (obj_type) {
    case BLE_STORE_OBJ_TYPE_PEER_SEC:
        rc = ble_store_ram_delete_peer_sec(&key->sec);
        return rc;

    case BLE_STORE_OBJ_TYPE_OUR_SEC:
        rc = ble_store_ram_delete_our_sec(&key->sec);
        return rc;

    case BLE_STORE_OBJ_TYPE_CCCD:
        rc = ble_store_ram_delete_cccd(&key->cccd);
        return rc;

    case BLE_STORE_OBJ_TYPE_CSFC:
        rc = ble_store_ram_delete_csfc(&key->csfc);
        return rc;

#if MYNEWT_VAL(ENC_ADV_DATA)
    case BLE_STORE_OBJ_TYPE_ENC_ADV_DATA:
        rc = ble_store_ram_delete_ead(&key->ead);
        return rc;
#endif

    default:
        return BLE_HS_ENOTSUP;
    }
}

void
ble_store_ram_init(void)
{
    /* Ensure this function only gets called by sysinit. */
    SYSINIT_ASSERT_ACTIVE();

    ble_hs_cfg.store_read_cb = ble_store_ram_read;
    ble_hs_cfg.store_write_cb = ble_store_ram_write;
    ble_hs_cfg.store_delete_cb = ble_store_ram_delete;

    /* Re-initialize BSS values in case of unit tests. */
    ble_store_ram_num_our_secs = 0;
    ble_store_ram_num_peer_secs = 0;
    ble_store_ram_num_cccds = 0;
    ble_store_ram_num_csfcs = 0;
#if MYNEWT_VAL(ENC_ADV_DATA)
    ble_store_ram_num_eads = 0;
#endif
}
