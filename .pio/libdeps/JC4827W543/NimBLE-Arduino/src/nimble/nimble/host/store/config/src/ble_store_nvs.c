/*
* Copyright 2015-2019 Espressif Systems (Shanghai) PTE LTD
*
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

#ifdef ESP_PLATFORM

#include "nimble/porting/nimble/include/syscfg/syscfg.h"

#if MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST)

#include <string.h>
#include <esp_system.h>
#include "nimble/porting/nimble/include/sysinit/sysinit.h"
#include "nimble/nimble/host/include/host/ble_hs.h"
#include "nimble/nimble/host/store/config/include/store/config/ble_store_config.h"
#include "ble_store_config_priv.h"
#include "esp_log.h"
#include "nvs.h"
#include "nimble/nimble/host/src/ble_hs_resolv_priv.h"


#define NIMBLE_NVS_STR_NAME_MAX_LEN              16
#define NIMBLE_NVS_PEER_SEC_KEY                  "peer_sec"
#define NIMBLE_NVS_OUR_SEC_KEY                   "our_sec"
#define NIMBLE_NVS_CCCD_SEC_KEY                  "cccd_sec"
#define NIMBLE_NVS_CSFC_SEC_KEY                  "csfc_sec"
#define NIMBLE_NVS_PEER_RECORDS_KEY              "p_dev_rec"
#define NIMBLE_NVS_NAMESPACE                     "nimble_bond"

#if MYNEWT_VAL(ENC_ADV_DATA)
#define NIMBLE_NVS_EAD_SEC_KEY                   "ead_sec"
#endif

#define NIMBLE_NVS_LOCAL_IRK_KEY                "local_irk"
#define NIMBLE_NVS_RPA_RECORDS_KEY               "rpa_rec"
typedef uint32_t nvs_handle_t;

static const char *LOG_TAG = "NIMBLE_NVS";

/*****************************************************************************
 * $ MISC                                                                    *
 *****************************************************************************/

static void
get_nvs_key_string(int obj_type, int index, char *key_string)
{
    if (obj_type == BLE_STORE_OBJ_TYPE_PEER_DEV_REC) {
        sprintf(key_string, "%s_%d", NIMBLE_NVS_PEER_RECORDS_KEY, index);
    } else {
        if (obj_type == BLE_STORE_OBJ_TYPE_PEER_SEC) {
            sprintf(key_string, "%s_%d", NIMBLE_NVS_PEER_SEC_KEY, index);
        } else if (obj_type == BLE_STORE_OBJ_TYPE_OUR_SEC) {
            sprintf(key_string, "%s_%d", NIMBLE_NVS_OUR_SEC_KEY, index);
#if MYNEWT_VAL(ENC_ADV_DATA)
        } else if (obj_type == BLE_STORE_OBJ_TYPE_ENC_ADV_DATA) {
            sprintf(key_string, "%s_%d", NIMBLE_NVS_EAD_SEC_KEY, index);
#endif
        } else if (obj_type == BLE_STORE_OBJ_TYPE_LOCAL_IRK) {
            sprintf(key_string, "%s_%d", NIMBLE_NVS_LOCAL_IRK_KEY, index);

        } else if (obj_type == BLE_STORE_OBJ_TYPE_PEER_ADDR){
            sprintf(key_string, "%s_%d", NIMBLE_NVS_RPA_RECORDS_KEY, index);
        } else if (obj_type == BLE_STORE_OBJ_TYPE_CCCD) {
            sprintf(key_string, "%s_%d", NIMBLE_NVS_CCCD_SEC_KEY, index);
        } else {
            sprintf(key_string, "%s_%d", NIMBLE_NVS_CSFC_SEC_KEY, index);
        }
    }
}

/* compares values at two addresses of size = item_size
* @Returns               index if entries match
*                       -1 if mismatch
*/
static int
get_nvs_matching_index(void *nvs_val, void *db_list, int db_num, size_t
                       item_size)
{
    uint8_t *db_item = (uint8_t *)db_list;
    int i;

    for (i = 0; i < db_num; i++) {
        if (memcmp(nvs_val, db_item, item_size) == 0) {
            /* Key matches with the one in RAM database */
            return i;
        }
        db_item += item_size;
    }
    return -1;
}

static int
get_nvs_max_obj_value(int obj_type)
{
    /* If host based privacy is enabled */
    if (obj_type == BLE_STORE_OBJ_TYPE_PEER_DEV_REC) {
        return (MYNEWT_VAL(BLE_STORE_MAX_BONDS) + 1);
    } else {
        if (obj_type == BLE_STORE_OBJ_TYPE_CCCD) {
            return MYNEWT_VAL(BLE_STORE_MAX_CCCDS);
        } else if (obj_type == BLE_STORE_OBJ_TYPE_CSFC) {
            return MYNEWT_VAL(BLE_STORE_MAX_CSFCS);
#if MYNEWT_VAL(ENC_ADV_DATA)
        } else if (obj_type == BLE_STORE_OBJ_TYPE_ENC_ADV_DATA) {
            return MYNEWT_VAL(BLE_STORE_MAX_EADS);
#endif
        } else {
            return MYNEWT_VAL(BLE_STORE_MAX_BONDS);
        }
    }
}

/*****************************************************************************
 * $ NVS                                                                     *
 *****************************************************************************/
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
static int
get_nvs_peer_record(char *key_string, struct ble_hs_dev_records *p_dev_rec)
{
    esp_err_t err;
    size_t required_size = 0;
    nvs_handle_t nimble_handle;

    err = nvs_open(NIMBLE_NVS_NAMESPACE, NVS_READWRITE, &nimble_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS open operation failed");
        return BLE_HS_ESTORE_FAIL;
    }

    err = nvs_get_blob(nimble_handle, key_string, NULL, &required_size);

    /* if Address pointer for value is NULL, filling of value not needed */
    if (err != ESP_OK || p_dev_rec == NULL) {
        goto end;
    }

    err = nvs_get_blob(nimble_handle, key_string, p_dev_rec,
                       &required_size);

end:
    nvs_close(nimble_handle);
    return err;
}
#endif

static int
get_nvs_db_value(int obj_type, char *key_string, union ble_store_value *val)
{
    esp_err_t err;
    size_t required_size = 0;
    nvs_handle_t nimble_handle;

    err = nvs_open(NIMBLE_NVS_NAMESPACE, NVS_READWRITE, &nimble_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS open operation failed");
        return BLE_HS_ESTORE_FAIL;
    }

    err = nvs_get_blob(nimble_handle, key_string, NULL, &required_size);

    /* if Address pointer for value is NULL, filling of value not needed */
    if (err != ESP_OK || val == NULL) {
        goto end;
    }

    if (obj_type == BLE_STORE_OBJ_TYPE_CCCD) {
        err = nvs_get_blob(nimble_handle, key_string, &val->cccd,
                           &required_size);
    } else if (obj_type == BLE_STORE_OBJ_TYPE_CSFC) {
        err = nvs_get_blob(nimble_handle, key_string, &val->csfc,
                           &required_size);
#if MYNEWT_VAL(ENC_ADV_DATA)
    } else if (obj_type == BLE_STORE_OBJ_TYPE_ENC_ADV_DATA) {
        err = nvs_get_blob(nimble_handle, key_string, &val->ead,
                           &required_size);
#endif
    } else if (obj_type == BLE_STORE_OBJ_TYPE_LOCAL_IRK) {
        err = nvs_get_blob (nimble_handle, key_string, &val->local_irk,
                      &required_size);

    } else if(obj_type == BLE_STORE_OBJ_TYPE_PEER_ADDR){
         err = nvs_get_blob(nimble_handle, key_string, &val->rpa_rec,
                           &required_size);

    } else {
        err = nvs_get_blob(nimble_handle, key_string, &val->sec,
                           &required_size);
    }

end:
    nvs_close(nimble_handle);
    return err;
}

/* Finds empty index or total count or index to be deleted in NVS database
* This function serves 3 different purposes depending upon 'empty' and `value`
* arguments.
* @ returns             - empty NVS index, if empty = 1
*                       - count of NVS database, if empty = 0, value = NULL
*                       - index that does not match with RAM db, if empty = 0 &
*                         value has valid database address.
*/
static int
get_nvs_db_attribute(int obj_type, bool empty, void *value, int num_value)
{
    union ble_store_value cur = {0};
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
    struct ble_hs_dev_records p_dev_rec = {0};
#endif
    esp_err_t err;
    int i, count = 0, max_limit = 0;
    char key_string[NIMBLE_NVS_STR_NAME_MAX_LEN];

    max_limit = get_nvs_max_obj_value(obj_type);

    for (i = 1; i <= max_limit; i++) {
        get_nvs_key_string(obj_type, i, key_string);

#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
        if (obj_type != BLE_STORE_OBJ_TYPE_PEER_DEV_REC) {
#endif
            err = get_nvs_db_value(obj_type, key_string, &cur);
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
        } else {
            err = get_nvs_peer_record(key_string, &p_dev_rec);
        }
#endif
        /* Check if the user is searching for empty index to write to */
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            if (empty) {
                ESP_LOGD(LOG_TAG, "Empty NVS index found = %d for obj_type = %d", i, obj_type);
                return i;
            }
        } else if (err == ESP_OK) {
            count++;
            /* If user has provided value, then the purpose is to find
             * non-matching entry from NVS */
            if (value) {
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
                if (obj_type == BLE_STORE_OBJ_TYPE_PEER_DEV_REC) {
                    err = get_nvs_matching_index(&p_dev_rec.peer_sec,
                                                 &((struct ble_hs_dev_records *)value)->peer_sec,
                                                 num_value,
                                                 sizeof(struct ble_hs_peer_sec));
                } else
#endif
                {
                    if (obj_type == BLE_STORE_OBJ_TYPE_CCCD) {
                        err = get_nvs_matching_index(&cur.sec, value, num_value,
                                                     sizeof(struct ble_store_value_cccd));
                    } else if (obj_type == BLE_STORE_OBJ_TYPE_CSFC) {
                        err = get_nvs_matching_index(&cur.csfc, value, num_value,
                                                     sizeof(struct ble_store_value_csfc));
#if MYNEWT_VAL(ENC_ADV_DATA)
                    } else if (obj_type == BLE_STORE_OBJ_TYPE_ENC_ADV_DATA) {
                        err = get_nvs_matching_index(&cur.sec, value, num_value,
                                                     sizeof(struct ble_store_value_ead));
#endif
                   } else if (obj_type == BLE_STORE_OBJ_TYPE_LOCAL_IRK) {
                       err = get_nvs_matching_index(&cur.local_irk, value, num_value,
                                                    sizeof(struct ble_store_value_local_irk));

                   } else if (obj_type == BLE_STORE_OBJ_TYPE_PEER_ADDR){
                        err = get_nvs_matching_index(&cur.rpa_rec,value,num_value,
                                                     sizeof(struct ble_store_value_rpa_rec));
                    } else {
                        err = get_nvs_matching_index(&cur.cccd, value, num_value,
                                                     sizeof(struct ble_store_value_sec));
                    }
                }
                /* If found non-matching/odd entry of NVS with entries in the
                 * internal database, return NVS index so can be deleted */
                if (err == -1 && !empty) {
                    return i;
                }
            }
        } else {
            ESP_LOGE(LOG_TAG, "NVS read operation failed while fetching size !!");
            return -1;
        }
    }

    if (empty == 0) {
        return count;
    } else {
        return (max_limit + 1);
    }
}

/* Deletes NVS value at given index
* @Returns               0 on success,
*                       -1 on NVS memory access failure
*/
static int
ble_nvs_delete_value(int obj_type, int8_t index)
{
    esp_err_t err;
    nvs_handle_t nimble_handle;
    char key_string[NIMBLE_NVS_STR_NAME_MAX_LEN];

    if (index > get_nvs_max_obj_value(obj_type)) {
        ESP_LOGE(LOG_TAG, "Invalid index provided to delete");
        return BLE_HS_EUNKNOWN;
    }

    err = nvs_open(NIMBLE_NVS_NAMESPACE, NVS_READWRITE, &nimble_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS open operation failed !!");
        return BLE_HS_ESTORE_FAIL;
    }

    get_nvs_key_string(obj_type, index, key_string);

    /* Erase the key with given index */
    err = nvs_erase_key(nimble_handle, key_string);
    if (err != ESP_OK) {
        goto error;
    }
    err = nvs_commit(nimble_handle);
    if (err != ESP_OK) {
        goto error;
    }

    nvs_close(nimble_handle);
    return 0;
error:
    nvs_close(nimble_handle);
    return BLE_HS_ESTORE_FAIL;
}

static int
ble_nvs_write_key_value(char *key, const void *value, size_t required_size)
{
    nvs_handle_t nimble_handle;
    esp_err_t err;

    err = nvs_open(NIMBLE_NVS_NAMESPACE, NVS_READWRITE, &nimble_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS open operation failed !!");
        return BLE_HS_ESTORE_FAIL;
    }

    err = nvs_set_blob(nimble_handle, key, value, required_size);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS write operation failed !!");
        goto error;
    }

    /* NVS commit and close */
    err = nvs_commit(nimble_handle);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS commit operation failed !!");
        goto error;
    }

    nvs_close(nimble_handle);
    return 0;
error:
    nvs_close(nimble_handle);
    return BLE_HS_ESTORE_FAIL;
}

/* To write key value in NVS.
* @Returns              0 if success
*                       BLE_HS_ESTORE_FAIL if failure
*                       BLE_HS_ESTORE_CAP if no space in NVS
*/
static int
ble_store_nvs_write(int obj_type, const union ble_store_value *val)
{
    char key_string[NIMBLE_NVS_STR_NAME_MAX_LEN];
    int8_t write_key_index = 0;

    write_key_index = get_nvs_db_attribute(obj_type, 1, NULL, 0);
    if (write_key_index == -1) {
        ESP_LOGE(LOG_TAG, "NVS operation failed !!");
        return BLE_HS_ESTORE_FAIL;
    } else if (write_key_index > get_nvs_max_obj_value(obj_type)) {

        /* bare-bone config code will take care of capacity overflow event,
         * however another check added for consistency */
        ESP_LOGD(LOG_TAG, "NVS size overflow.");
        return BLE_HS_ESTORE_CAP;
    }

    get_nvs_key_string(obj_type, write_key_index, key_string);

    if (obj_type == BLE_STORE_OBJ_TYPE_CCCD) {
        return ble_nvs_write_key_value(key_string, &val->cccd, sizeof(struct
                                       ble_store_value_cccd));
    } else if (obj_type == BLE_STORE_OBJ_TYPE_CSFC) {
        return ble_nvs_write_key_value(key_string, &val->csfc, sizeof(struct
                                       ble_store_value_csfc));
#if MYNEWT_VAL(ENC_ADV_DATA)
    } else if (obj_type == BLE_STORE_OBJ_TYPE_ENC_ADV_DATA) {
        return ble_nvs_write_key_value(key_string, &val->ead, sizeof(struct
                                       ble_store_value_ead));
#endif
    } else if (obj_type == BLE_STORE_OBJ_TYPE_LOCAL_IRK) {
        return ble_nvs_write_key_value(key_string, &val->local_irk, sizeof(struct
                                   ble_store_value_local_irk));

    } else if (obj_type == BLE_STORE_OBJ_TYPE_PEER_ADDR) {
        return ble_nvs_write_key_value(key_string, &val->rpa_rec, sizeof(struct
                                       ble_store_value_rpa_rec));

    } else {
        return ble_nvs_write_key_value(key_string, &val->sec, sizeof(struct
                                       ble_store_value_sec));
    }
}

#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
/* If Host based privacy is enabled */
static int
ble_store_nvs_peer_records(int obj_type, const struct ble_hs_dev_records *p_dev_rec)
{
    char key_string[NIMBLE_NVS_STR_NAME_MAX_LEN];
    int8_t write_key_index = 0;

    write_key_index = get_nvs_db_attribute(obj_type, 1, NULL, 0);
    if (write_key_index == -1) {
        ESP_LOGE(LOG_TAG, "NVS operation failed !!");
        return BLE_HS_ESTORE_FAIL;
    } else if (write_key_index > get_nvs_max_obj_value(obj_type)) {

        /* bare-bone config code will take care of capacity overflow event,
         * however another check added for consistency */
        ESP_LOGD(LOG_TAG, "NVS size overflow.");
        return BLE_HS_ESTORE_CAP;
    }

    get_nvs_key_string(obj_type, write_key_index, key_string);

    return ble_nvs_write_key_value(key_string, p_dev_rec, sizeof(struct
                                   ble_hs_dev_records));
}
#endif

static int
populate_db_from_nvs(int obj_type, void *dst, int *db_num)
{
    uint8_t *db_item = (uint8_t *)dst;
    union ble_store_value cur = {0};
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
    struct ble_hs_dev_records p_dev_rec = {0};
#endif

    esp_err_t err;
    int i;
    char key_string[NIMBLE_NVS_STR_NAME_MAX_LEN];

    for (i = 1; i <= get_nvs_max_obj_value(obj_type); i++) {
        get_nvs_key_string(obj_type, i, key_string);

#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
        if (obj_type != BLE_STORE_OBJ_TYPE_PEER_DEV_REC) {
#endif
            err = get_nvs_db_value(obj_type, key_string, &cur);
            if (err == ESP_ERR_NVS_NOT_FOUND) {
                continue;
            } else if (err != ESP_OK) {
                ESP_LOGE(LOG_TAG, "NVS read operation failed !!");
                return -1;
            }
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
        } else {
            err = get_nvs_peer_record(key_string, &p_dev_rec);
            if (err == ESP_ERR_NVS_NOT_FOUND) {
                continue;
            } else if (err != ESP_OK) {
                ESP_LOGE(LOG_TAG, "NVS read operation failed !!");
                return -1;
            }
        }

        /* NVS index has data, fill up the ram db with it */
        if (obj_type == BLE_STORE_OBJ_TYPE_PEER_DEV_REC) {
            ESP_LOGD(LOG_TAG, "Peer dev records filled from NVS index = %d", i);
            memcpy(db_item, &p_dev_rec, sizeof(struct ble_hs_dev_records));
            db_item += sizeof(struct ble_hs_dev_records);
            (*db_num)++;
        } else
#endif
        {
            if (obj_type == BLE_STORE_OBJ_TYPE_CCCD) {
                ESP_LOGD(LOG_TAG, "CCCD in RAM is filled up from NVS index = %d", i);
                memcpy(db_item, &cur.cccd, sizeof(struct ble_store_value_cccd));
                db_item += sizeof(struct ble_store_value_cccd);
                (*db_num)++;
            } else if (obj_type == BLE_STORE_OBJ_TYPE_CSFC) {
                ESP_LOGD(LOG_TAG, "CSFC in RAM is filled up from NVS index = %d", i);
                memcpy(db_item, &cur.csfc, sizeof(struct ble_store_value_csfc));
                db_item += sizeof(struct ble_store_value_csfc);
                (*db_num)++;
#if MYNEWT_VAL(ENC_ADV_DATA)
            } else if (obj_type == BLE_STORE_OBJ_TYPE_ENC_ADV_DATA) {
                  ESP_LOGD(LOG_TAG, "EAD in RAM is filled up from NVS index = %d", i);
                  memcpy(db_item, &cur.ead, sizeof(struct ble_store_value_ead));
                  db_item += sizeof(struct ble_store_value_ead);
                  (*db_num)++;
#endif
           } else if(obj_type == BLE_STORE_OBJ_TYPE_LOCAL_IRK) {
                  ESP_LOGD(LOG_TAG, "Local IRK in RAM is filled up from NVS index = %d", i);
                  memcpy(db_item, &cur.local_irk, sizeof(struct ble_store_value_local_irk));
                  db_item += sizeof(struct ble_store_value_local_irk);
                  (*db_num)++;

            } else if(obj_type == BLE_STORE_OBJ_TYPE_PEER_ADDR) {
                  ESP_LOGD(LOG_TAG, "RPA_REC in RAM is filled up from NVS index = %d", i);
                  memcpy(db_item, &cur.rpa_rec, sizeof(struct ble_store_value_rpa_rec));
                  db_item += sizeof(struct ble_store_value_rpa_rec);
                  (*db_num)++;
            } else {
                ESP_LOGD(LOG_TAG, "KEY in RAM is filled up from NVS index = %d", i);
                memcpy(db_item, &cur.sec, sizeof(struct ble_store_value_sec));
                db_item += sizeof(struct ble_store_value_sec);
                (*db_num)++;
            }
        }
    }
    return 0;
}

/* Gets the database in RAM filled up with keys stored in NVS. The sequence of
 * the keys in database may get lost.
 */
static int
ble_nvs_restore_sec_keys(void)
{
    esp_err_t err;
    int flag = 0;
    extern uint16_t ble_store_config_our_bond_count;
    extern uint16_t ble_store_config_peer_bond_count;
    extern int ble_store_config_compare_bond_count(const void *a, const void *b);

#if MYNEWT_VAL(BLE_STORE_MAX_BONDS)
    err = populate_db_from_nvs(BLE_STORE_OBJ_TYPE_OUR_SEC, ble_store_config_our_secs,
                               &ble_store_config_num_our_secs);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS operation failed for 'our sec'");
        return err;
    }
    ESP_LOGD(LOG_TAG, "ble_store_config_our_secs restored %d bonds", ble_store_config_num_our_secs);

    err = populate_db_from_nvs(BLE_STORE_OBJ_TYPE_PEER_SEC, ble_store_config_peer_secs,
                               &ble_store_config_num_peer_secs);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS operation failed for 'peer sec'");
        return err;
    }

    for (int i = 0; i < MYNEWT_VAL(BLE_STORE_MAX_BONDS) - 1; i++) {
        if ((ble_store_config_our_secs[i].bond_count > ble_store_config_our_secs[i+1].bond_count)
            || (ble_store_config_peer_secs[i].bond_count > ble_store_config_peer_secs[i+1].bond_count)) {
                flag = 1;
                break;
        }
    }

    if (flag) {

        qsort(ble_store_config_our_secs, ble_store_config_num_our_secs,
            sizeof(struct ble_store_value_sec), ble_store_config_compare_bond_count);

        qsort(ble_store_config_peer_secs, ble_store_config_num_peer_secs,
            sizeof(struct ble_store_value_sec), ble_store_config_compare_bond_count);
    }

    ble_store_config_our_bond_count = ble_store_config_our_secs[ble_store_config_num_our_secs - 1].bond_count;
    ble_store_config_peer_bond_count = ble_store_config_peer_secs[ble_store_config_num_peer_secs - 1].bond_count;

    ESP_LOGD(LOG_TAG, "ble_store_config_peer_secs restored %d bonds",
             ble_store_config_num_peer_secs);
#endif

#if MYNEWT_VAL(BLE_STORE_MAX_CCCDS)
    err = populate_db_from_nvs(BLE_STORE_OBJ_TYPE_CCCD, ble_store_config_cccds,
                               &ble_store_config_num_cccds);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS operation failed for 'CCCD'");
        return err;
    }
    ESP_LOGD(LOG_TAG, "ble_store_config_cccds restored %d bonds",
             ble_store_config_num_cccds);
#endif

#if MYNEWT_VAL(BLE_STORE_MAX_CSFCS)
    err = populate_db_from_nvs(BLE_STORE_OBJ_TYPE_CSFC, ble_store_config_csfcs,
                               &ble_store_config_num_csfcs);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS operation failed for 'CSFC'");
        return err;
    }
    ESP_LOGD(LOG_TAG, "ble_store_config_csfcs restored %d bonds",
             ble_store_config_num_csfcs);
#endif

#if MYNEWT_VAL(ENC_ADV_DATA)
    err = populate_db_from_nvs(BLE_STORE_OBJ_TYPE_ENC_ADV_DATA, ble_store_config_eads,
                               &ble_store_config_num_eads);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS operation failed for 'EAD'");
        return err;
    }
    ESP_LOGD(LOG_TAG, "ble_store_config_eads restored %d bonds",
             ble_store_config_num_eads);
#endif
    err = populate_db_from_nvs(BLE_STORE_OBJ_TYPE_LOCAL_IRK, ble_store_config_local_irks,
                           &ble_store_config_num_local_irks);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS operation failed for 'Local IRK'");
        return err;
    }
    ESP_LOGD(LOG_TAG, "ble_store_config_local_irks restored %d irks",
             ble_store_config_num_local_irks);

    err = populate_db_from_nvs(BLE_STORE_OBJ_TYPE_PEER_ADDR, ble_store_config_rpa_recs,
                               &ble_store_config_num_rpa_recs);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS operation failed for 'RPA_REC'");
        return err;
    }
    ESP_LOGD(LOG_TAG, "ble_store_config_rpa_recs restored %d bonds",
             ble_store_config_num_rpa_recs);

    return 0;
}

#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
static int
ble_nvs_restore_peer_records(void)
{
    esp_err_t err;
    int ble_store_num_peer_dev_rec = 0;
    struct ble_hs_dev_records *peer_dev_rec = ble_rpa_get_peer_dev_records();

    err = populate_db_from_nvs(BLE_STORE_OBJ_TYPE_PEER_DEV_REC, peer_dev_rec,
                               &ble_store_num_peer_dev_rec);
    if (err != ESP_OK) {
        ESP_LOGE(LOG_TAG, "NVS operation failed fetching 'Peer Dev Records'");
        return err;
    }

    ble_rpa_set_num_peer_dev_records(ble_store_num_peer_dev_rec);
    ESP_LOGD(LOG_TAG, "peer_dev_rec restored %d records", ble_store_num_peer_dev_rec);

    return 0;
}
#endif

#if MYNEWT_VAL(BLE_STORE_MAX_CCCDS)
int ble_store_config_persist_cccds(void)
{
    int nvs_count, nvs_idx;
    union ble_store_value val;

    nvs_count = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_CCCD, 0, NULL, 0);
    if (nvs_count == -1) {
        ESP_LOGE(LOG_TAG, "NVS operation failed while persisting CCCD");
        return BLE_HS_ESTORE_FAIL;
    }

    if (nvs_count < ble_store_config_num_cccds) {

        /* NVS db count less than RAM count, write operation */
        ESP_LOGD(LOG_TAG, "Persisting CCCD value in NVS...");
        val.cccd = ble_store_config_cccds[ble_store_config_num_cccds - 1];
        return ble_store_nvs_write(BLE_STORE_OBJ_TYPE_CCCD, &val);
    } else if (nvs_count > ble_store_config_num_cccds) {
        /* NVS db count more than RAM count, delete operation */
        nvs_idx = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_CCCD, 0,
                                       ble_store_config_cccds, ble_store_config_num_cccds);
        if (nvs_idx == -1) {
            ESP_LOGE(LOG_TAG, "NVS delete operation failed for CCCD");
            return BLE_HS_ESTORE_FAIL;
        }
        ESP_LOGD(LOG_TAG, "Deleting CCCD, nvs idx = %d", nvs_idx);
        return ble_nvs_delete_value(BLE_STORE_OBJ_TYPE_CCCD, nvs_idx);
    }
    return 0;
}
#endif

#if MYNEWT_VAL(BLE_STORE_MAX_CSFCS)
int ble_store_config_persist_csfcs(void)
{
    int nvs_count, nvs_idx;
    union ble_store_value val;

    nvs_count = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_CSFC, 0, NULL, 0);
    if (nvs_count == -1) {
        ESP_LOGE(LOG_TAG, "NVS operation failed while persisting CSFC");
        return BLE_HS_ESTORE_FAIL;
    }

    if (nvs_count < ble_store_config_num_csfcs) {
        /* NVS db count less than RAM count, write operation */
        ESP_LOGD(LOG_TAG, "Persisting CSFC value in NVS...");
        val.csfc = ble_store_config_csfcs[ble_store_config_num_csfcs - 1];
        return ble_store_nvs_write(BLE_STORE_OBJ_TYPE_CSFC, &val);
    } else if (nvs_count > ble_store_config_num_csfcs) {
        /* NVS db count more than RAM count, delete operation */
        nvs_idx = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_CSFC, 0,
                                       ble_store_config_csfcs, ble_store_config_num_csfcs);
        if (nvs_idx == -1) {
            ESP_LOGE(LOG_TAG, "NVS delete operation failed for CSFC");
            return BLE_HS_ESTORE_FAIL;
        }
        ESP_LOGD(LOG_TAG, "Deleting CSFC, nvs idx = %d", nvs_idx);
        return ble_nvs_delete_value(BLE_STORE_OBJ_TYPE_CSFC, nvs_idx);
    }
    return 0;
}
#endif

#if MYNEWT_VAL(ENC_ADV_DATA)
int ble_store_config_persist_eads(void)
{
    int nvs_count, nvs_idx;
    union ble_store_value val;

    nvs_count = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_ENC_ADV_DATA, 0, NULL, 0);
    if (nvs_count == -1) {
        ESP_LOGE(LOG_TAG, "NVS operation failed while persisting EAD");
        return BLE_HS_ESTORE_FAIL;
    }

    if (nvs_count < ble_store_config_num_eads) {
        /* NVS db count less than RAM count, write operation */
        ESP_LOGD(LOG_TAG, "Persisting EAD value in NVS...");
        val.ead = ble_store_config_eads[ble_store_config_num_eads - 1];
        return ble_store_nvs_write(BLE_STORE_OBJ_TYPE_ENC_ADV_DATA, &val);
    } else if (nvs_count > ble_store_config_num_eads) {
        /* NVS db count more than RAM count, delete operation */
        nvs_idx = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_ENC_ADV_DATA, 0,
                                       ble_store_config_eads, ble_store_config_num_eads);
        if (nvs_idx == -1) {
            ESP_LOGE(LOG_TAG, "NVS delete operation failed for EAD");
            return BLE_HS_ESTORE_FAIL;
        }
        ESP_LOGD(LOG_TAG, "Deleting EAD, nvs idx = %d", nvs_idx);
        return ble_nvs_delete_value(BLE_STORE_OBJ_TYPE_ENC_ADV_DATA, nvs_idx);
    }
    return 0;
}
#endif
int ble_store_config_persist_local_irk(void)
{
    int nvs_count, nvs_idx;
    union ble_store_value val;

    nvs_count = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_LOCAL_IRK, 0, NULL, 0);
    if (nvs_count == -1) {
        ESP_LOGE(LOG_TAG, "NVS operation failed while persisting EAD");
        return BLE_HS_ESTORE_FAIL;
    }

    if (nvs_count < ble_store_config_num_local_irks) {
        /* NVS db count less than RAM count, write operation */
        ESP_LOGD(LOG_TAG, "Persisting Local IRK value in NVS...");
        val.local_irk = ble_store_config_local_irks[ble_store_config_num_local_irks-1];
        return ble_store_nvs_write(BLE_STORE_OBJ_TYPE_LOCAL_IRK, &val);
    } else if (nvs_count > ble_store_config_num_local_irks) {
        /* NVS db count more than RAM count, delete operation */
        nvs_idx = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_LOCAL_IRK, 0,
                                       ble_store_config_local_irks, ble_store_config_num_local_irks);
        if (nvs_idx == -1) {
            ESP_LOGE(LOG_TAG, "NVS delete operation failed for Local IRK");
            return BLE_HS_ESTORE_FAIL;
        }
        ESP_LOGD(LOG_TAG, "Deleting Local IRK, nvs idx = %d", nvs_idx);
        return ble_nvs_delete_value(BLE_STORE_OBJ_TYPE_LOCAL_IRK, nvs_idx);
    }
    return 0;
}

int ble_store_config_persist_rpa_recs(void)
{
    int nvs_count, nvs_idx;
    union ble_store_value val;
    nvs_count = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_PEER_ADDR, 0, NULL, 0);
     if (nvs_count == -1) {
        ESP_LOGE(LOG_TAG, "NVS operation failed while persisting RPA_RECS");
        return BLE_HS_ESTORE_FAIL;
    }
    if (nvs_count < ble_store_config_num_rpa_recs) {
        /* NVS db count less than RAM count, write operation */
        ESP_LOGD(LOG_TAG, "Persisting RPA_RECS value in NVS...");
        val.rpa_rec = ble_store_config_rpa_recs[ble_store_config_num_rpa_recs - 1];
        return ble_store_nvs_write(BLE_STORE_OBJ_TYPE_PEER_ADDR, &val);
    } else if (nvs_count > ble_store_config_num_rpa_recs) {
        /* NVS db count more than RAM count, delete operation */
        nvs_idx = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_PEER_ADDR, 0,
                                       ble_store_config_rpa_recs, ble_store_config_num_rpa_recs);
        if (nvs_idx == -1) {
            ESP_LOGE(LOG_TAG, "NVS delete operation failed for RPA_REC");
            return BLE_HS_ESTORE_FAIL;
        }
        ESP_LOGD(LOG_TAG, "Deleting RPA_REC, nvs idx = %d", nvs_idx);
        return ble_nvs_delete_value(BLE_STORE_OBJ_TYPE_PEER_ADDR, nvs_idx);
    }
    return 0;

}

int ble_store_config_persist_peer_secs(void)
{
    int nvs_count, nvs_idx;
    union ble_store_value val;

    nvs_count = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_PEER_SEC, 0, NULL, 0);
    if (nvs_count == -1) {
        ESP_LOGE(LOG_TAG, "NVS operation failed while persisting peer sec");
        return BLE_HS_ESTORE_FAIL;
    }

    if (nvs_count < ble_store_config_num_peer_secs) {

        /* NVS db count less than RAM count, write operation */
        ESP_LOGD(LOG_TAG, "Persisting peer sec value in NVS...");
        val.sec = ble_store_config_peer_secs[ble_store_config_num_peer_secs - 1];
        return ble_store_nvs_write(BLE_STORE_OBJ_TYPE_PEER_SEC, &val);
    } else if (nvs_count > ble_store_config_num_peer_secs) {
        /* NVS db count more than RAM count, delete operation */
        nvs_idx = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_PEER_SEC, 0,
                                       ble_store_config_peer_secs, ble_store_config_num_peer_secs);
        if (nvs_idx == -1) {
            ESP_LOGE(LOG_TAG, "NVS delete operation failed for peer sec");
            return BLE_HS_ESTORE_FAIL;
        }
        ESP_LOGD(LOG_TAG, "Deleting peer sec, nvs idx = %d", nvs_idx);
        return ble_nvs_delete_value(BLE_STORE_OBJ_TYPE_PEER_SEC, nvs_idx);
    }
    return 0;
}

int ble_store_config_persist_our_secs(void)
{
    int nvs_count, nvs_idx;
    union ble_store_value val;

    nvs_count = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_OUR_SEC, 0, NULL, 0);
    if (nvs_count == -1) {
        ESP_LOGE(LOG_TAG, "NVS operation failed while persisting our sec");
        return BLE_HS_ESTORE_FAIL;
    }

    if (nvs_count < ble_store_config_num_our_secs) {

        /* NVS db count less than RAM count, write operation */
        ESP_LOGD(LOG_TAG, "Persisting our sec value to NVS...");
        val.sec = ble_store_config_our_secs[ble_store_config_num_our_secs - 1];
        return ble_store_nvs_write(BLE_STORE_OBJ_TYPE_OUR_SEC, &val);
    } else if (nvs_count > ble_store_config_num_our_secs) {
        /* NVS db count more than RAM count, delete operation */
        nvs_idx = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_OUR_SEC, 0,
                                       ble_store_config_our_secs, ble_store_config_num_our_secs);
        if (nvs_idx == -1) {
            ESP_LOGE(LOG_TAG, "NVS delete operation failed for our sec");
            return BLE_HS_ESTORE_FAIL;
        }
        ESP_LOGD(LOG_TAG, "Deleting our sec, nvs idx = %d", nvs_idx);
        return ble_nvs_delete_value(BLE_STORE_OBJ_TYPE_OUR_SEC, nvs_idx);
    }
    return 0;
}

#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
int ble_store_persist_peer_records(void)
{
    int nvs_count, nvs_idx;
    struct ble_hs_dev_records peer_rec;
    int ble_store_num_peer_dev_rec = ble_rpa_get_num_peer_dev_records();
    struct ble_hs_dev_records *peer_dev_rec = ble_rpa_get_peer_dev_records();

    nvs_count = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_PEER_DEV_REC, 0, NULL, 0);
    if (nvs_count == -1) {
        ESP_LOGE(LOG_TAG, "NVS operation failed while persisting peer_dev_rec");
        return BLE_HS_ESTORE_FAIL;
    }

    if (nvs_count < ble_store_num_peer_dev_rec) {

        /* NVS db count less than RAM count, write operation */
        ESP_LOGD(LOG_TAG, "Persisting peer dev record to NVS...");
        peer_rec = peer_dev_rec[ble_store_num_peer_dev_rec - 1];
        return ble_store_nvs_peer_records(BLE_STORE_OBJ_TYPE_PEER_DEV_REC, &peer_rec);
    } else if (nvs_count > ble_store_num_peer_dev_rec) {
        /* NVS db count more than RAM count, delete operation */
        nvs_idx = get_nvs_db_attribute(BLE_STORE_OBJ_TYPE_PEER_DEV_REC, 0,
                                       peer_dev_rec,
                                       ble_store_num_peer_dev_rec);
        if (nvs_idx == -1) {
            ESP_LOGE(LOG_TAG, "NVS delete operation failed for peer records");
            return BLE_HS_ESTORE_FAIL;
        }
        ESP_LOGD(LOG_TAG, "Deleting peer record, nvs idx = %d", nvs_idx);
        return ble_nvs_delete_value(BLE_STORE_OBJ_TYPE_PEER_DEV_REC, nvs_idx);
    }
    return 0;
}
#endif

void ble_store_config_conf_init(void)
{
    int err;

    err = ble_nvs_restore_sec_keys();
    if (err != 0) {
        ESP_LOGE(LOG_TAG, "NVS operation failed, can't retrieve the bonding info");
    }
#if MYNEWT_VAL(BLE_HOST_BASED_PRIVACY)
    err = ble_nvs_restore_peer_records();
    if (err != 0) {
        ESP_LOGE(LOG_TAG, "NVS operation failed, can't retrieve the peer records");
    }
#endif
}

/***************************************************************************************/
#endif /* MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST) */
#endif /* ESP_PLATFORM */
