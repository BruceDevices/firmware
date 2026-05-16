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

#include "nimble/porting/nimble/include/syscfg/syscfg.h"

#if MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST)

#include <inttypes.h>
#include <string.h>

#include "nimble/porting/nimble/include/sysinit/sysinit.h"
#include "nimble/nimble/host/include/host/ble_hs.h"
#include "base64/base64.h"
#include "nimble/nimble/host/store/config/include/store/config/ble_store_config.h"
#include "ble_store_config_priv.h"
#include "ble_bond_nvs.h"

#define BLE_STORE_CONFIG_SEC_ENCODE_SZ      \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_sec))

#define BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ  \
    (MYNEWT_VAL(BLE_STORE_MAX_BONDS) * BLE_STORE_CONFIG_SEC_ENCODE_SZ + 1)

#define BLE_STORE_CONFIG_CCCD_ENCODE_SZ     \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_cccd))

#define BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ \
    (MYNEWT_VAL(BLE_STORE_MAX_CCCDS) * BLE_STORE_CONFIG_CCCD_ENCODE_SZ + 1)

#define BLE_STORE_CONFIG_CSFC_ENCODE_SZ     \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_csfc))

#define BLE_STORE_CONFIG_CSFC_SET_ENCODE_SZ     \
    (MYNEWT_VAL(BLE_STORE_MAX_CSFCS) * BLE_STORE_CONFIG_CSFC_ENCODE_SZ + 1)

#if MYNEWT_VAL(ENC_ADV_DATA)
#define BLE_STORE_CONFIG_EAD_ENCODE_SZ     \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_ead))

#define BLE_STORE_CONFIG_EAD_SET_ENCODE_SZ \
    (MYNEWT_VAL(BLE_STORE_MAX_EADS) * BLE_STORE_CONFIG_EAD_ENCODE_SZ + 1)
#endif

#define BLE_STORE_CONFIG_RPA_REC_ENCODE_SZ \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_rpa_rec))

#define BLE_STORE_CONFIG_RPA_REC_SET_ENCODE_SZ \
    (MYNEWT_VAL(BLE_STORE_MAX_BONDS) * BLE_STORE_CONFIG_RPA_REC_ENCODE_SZ + 1)

#define BLE_STORE_CONFIG_IRK_ENCODE_SZ     \
    BASE64_ENCODE_SIZE(sizeof (struct ble_store_value_local_irk))

#define BLE_STORE_CONFIG_IRK_SET_ENCODE_SZ \
    (MYNEWT_VAL(BLE_STORE_MAX_BONDS) * BLE_STORE_CONFIG_IRK_ENCODE_SZ + 1)

#define CCCD_LABEL      0x64636363 /* cccd */
#define CSFC_LABEL      0x63667363 /* csfc */
#define RPA_REC_LABEL   0x72617072 /* rpar */
#define IRK_LABEL       0x6b72696f /* oirk */
#define EAD_LABEL       0x00646165 /* ead */
#define PEER_SEC_LABEL  0x63657370 /* psec */
#define OUR_SEC_LABEL   0x6365736f /* osec */

static void
ble_store_config_serialize_arr(const void *arr, int obj_sz, int num_objs,
                               char *out_buf, int buf_sz)
{
    int arr_size;

    arr_size = obj_sz * num_objs;
    assert(arr_size <= buf_sz);

    base64_encode(arr, arr_size, out_buf, 1);
}

static int
ble_store_config_deserialize_arr(const char *enc,
                                 void *out_arr,
                                 int obj_sz,
                                 int *out_num_objs)
{
    int len;

    len = base64_decode(enc, out_arr);
    if (len < 0) {
        return OS_EINVAL;
    }

    *out_num_objs = len / obj_sz;
    return 0;
}

static int
ble_store_config_persist_sec_set(uint32_t setting_name,
                                 const struct ble_store_value_sec *secs,
                                 int num_secs)
{
    char buf[BLE_STORE_CONFIG_SEC_SET_ENCODE_SZ];
    int rc;

    ble_store_config_serialize_arr(secs, sizeof *secs, num_secs,
                                   buf, sizeof buf);
    rc = ble_bond_nvs_save_entry(setting_name, buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }

    return 0;
}

int
ble_store_config_persist_our_secs(void)
{
    int rc;

    rc = ble_store_config_persist_sec_set(OUR_SEC_LABEL,
                                          ble_store_config_our_secs,
                                          ble_store_config_num_our_secs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
ble_store_config_persist_peer_secs(void)
{
    int rc;

    rc = ble_store_config_persist_sec_set(PEER_SEC_LABEL,
                                          ble_store_config_peer_secs,
                                          ble_store_config_num_peer_secs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int
ble_store_config_persist_cccds(void)
{
    char buf[BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ];
    int rc;

    ble_store_config_serialize_arr(ble_store_config_cccds,
                                   sizeof *ble_store_config_cccds,
                                   ble_store_config_num_cccds,
                                   buf,
                                   sizeof buf);
    rc = ble_bond_nvs_save_entry(CCCD_LABEL, buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }

    return 0;
}

int
ble_store_config_persist_csfcs(void)
{
    char buf[BLE_STORE_CONFIG_CSFC_SET_ENCODE_SZ];
    int rc;

    ble_store_config_serialize_arr(ble_store_config_csfcs,
                                   sizeof *ble_store_config_csfcs,
                                   ble_store_config_num_csfcs,
                                   buf,
                                   sizeof buf);
    rc = ble_bond_nvs_save_entry(CSFC_LABEL, buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }

    return 0;
}

#if MYNEWT_VAL(ENC_ADV_DATA)
int
ble_store_config_persist_eads(void)
{
    char buf[BLE_STORE_CONFIG_CCCD_SET_ENCODE_SZ];
    int rc;
    ble_store_config_serialize_arr(ble_store_config_eads,
                                   sizeof *ble_store_config_eads,
                                   ble_store_config_num_eads,
                                   buf,
                                   sizeof buf);

    rc = ble_bond_nvs_save_entry(EAD_LABEL, buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }
    return 0;
}
#endif

int
ble_store_config_persist_rpa_recs(void)
{
    char buf[BLE_STORE_CONFIG_RPA_REC_SET_ENCODE_SZ];
    int rc;
    ble_store_config_serialize_arr(ble_store_config_rpa_recs,
                                   sizeof *ble_store_config_rpa_recs,
                                   ble_store_config_num_rpa_recs,
                                   buf,
                                   sizeof buf);
    rc = ble_bond_nvs_save_entry(RPA_REC_LABEL, buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }
    return 0;
}

int
ble_store_config_persist_local_irk(void) {
    char buf[BLE_STORE_CONFIG_IRK_SET_ENCODE_SZ];
    int rc;

    ble_store_config_serialize_arr(ble_store_config_local_irks,
                                   sizeof *ble_store_config_local_irks,
                                   ble_store_config_num_local_irks,
                                   buf, sizeof buf);

    rc = ble_bond_nvs_save_entry(IRK_LABEL, buf);
    if (rc != 0) {
        return BLE_HS_ESTORE_FAIL;
    }

    return 0;
}

void
ble_store_config_conf_init(void)
{
    uint32_t val_addr = 0;
    int rc = 0;

    rc = ble_bond_nvs_get_entry(OUR_SEC_LABEL, &val_addr);
    if (rc == 0) {
        rc = ble_store_config_deserialize_arr(
                (char*)val_addr,
                ble_store_config_our_secs,
                sizeof *ble_store_config_our_secs,
                &ble_store_config_num_our_secs);
        if (rc != 0) {
            BLE_HS_LOG(ERROR, "our_sec restore error rc=%d\n", rc);
            return;
        }

        rc = ble_bond_nvs_get_entry(PEER_SEC_LABEL, &val_addr);
        if (rc == 0) {
            rc = ble_store_config_deserialize_arr(
                    (char*)val_addr,
                    ble_store_config_peer_secs,
                    sizeof *ble_store_config_peer_secs,
                    &ble_store_config_num_peer_secs);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "peer_sec restore error rc=%d\n", rc);
                return;
            }

        } else {
            /* If we have a security entry for our security but not a peer
             * we should assume something wrong with the store so delete it.
             */
            BLE_HS_LOG(ERROR, "peer info not found\n");
            ble_store_clear();
            return;
        }

        rc = ble_bond_nvs_get_entry(CCCD_LABEL, &val_addr);
        if (rc == 0) {
            rc = ble_store_config_deserialize_arr(
                    (char*)val_addr,
                    ble_store_config_cccds,
                    sizeof *ble_store_config_cccds,
                    &ble_store_config_num_cccds);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "cccd restore error rc=%d\n", rc);
                return;
            }
        }

        rc = ble_bond_nvs_get_entry(CSFC_LABEL, &val_addr);
        if (rc == 0) {
            rc = ble_store_config_deserialize_arr(
                    (char*)val_addr,
                    ble_store_config_csfcs,
                    sizeof *ble_store_config_csfcs,
                    &ble_store_config_num_csfcs);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "cfsc restore error rc=%d\n", rc);
                return;
            }
        }

#if MYNEWT_VAL(ENC_ADV_DATA)
        rc = ble_bond_nvs_get_entry(EAD_LABEL, &val_addr);
        if (rc == 0) {
            rc = ble_store_config_deserialize_arr(
                    (char*)val_addr,
                    ble_store_config_eads,
                    sizeof *ble_store_config_eads,
                    &ble_store_config_num_eads);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "ead restore error rc=%d\n", rc);
                return;
            }
        }
#endif
        rc = ble_bond_nvs_get_entry(RPA_REC_LABEL, &val_addr);
        if (rc == 0) {
            rc = ble_store_config_deserialize_arr(
                    (char*)val_addr,
                    ble_store_config_rpa_recs,
                    sizeof *ble_store_config_rpa_recs,
                    &ble_store_config_num_rpa_recs);
            if (rc != 0) {
                BLE_HS_LOG(ERROR, "rpa_rec restore error rc=%d\n", rc);
                return;
            }
        }
    } else {
        /* If we have a security entry for our security but not a peer
         * we should assume something wrong with the store so delete it.
         */
        BLE_HS_LOG(ERROR, "our_sec info not found\n");
        ble_store_clear();
        return;
    }

    rc = ble_bond_nvs_get_entry(IRK_LABEL, &val_addr);
    if (rc == 0) {
        rc = ble_store_config_deserialize_arr(
                (char*)val_addr,
                ble_store_config_local_irks,
                sizeof *ble_store_config_local_irks,
                &ble_store_config_num_local_irks);
        if (rc != 0) {
            BLE_HS_LOG(ERROR, "irk restore error rc=%d\n", rc);
            return;
        }
    }
}

#endif /* MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST) */
#endif /* ESP_PLATFORM */
