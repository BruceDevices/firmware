// SPDX-License-Identifier: GPL-3.0-or-later
// Derived from ProtoPirate and Flipper Zero; see THIRD_PARTY.md.
#if !defined(LITE_VERSION)
#include "rf_vehicle_support.h"

#define KIA_KEY1 10u
#define KIA_KEY2 11u
#define KIA_KEY3 12u
#define KIA_KEY4 13u

uint64_t kia_mf_key = 0;
uint64_t kia_v6_a_key = 0;
uint64_t kia_v6_b_key = 0;
uint64_t kia_v5_key = 0;

static bool kia_mf_key_loaded = false;
static bool kia_v6_a_key_loaded = false;
static bool kia_v6_b_key_loaded = false;
static bool kia_v5_key_loaded = false;

static void rf_vehicle_keys_reset(void) {
    kia_mf_key = 0;
    kia_v6_a_key = 0;
    kia_v6_b_key = 0;
    kia_v5_key = 0;
    kia_mf_key_loaded = false;
    kia_v6_a_key_loaded = false;
    kia_v6_b_key_loaded = false;
    kia_v5_key_loaded = false;
}

bool rf_vehicle_keys_load(RfStreamEnvironment* environment) {
    rf_vehicle_keys_reset();
    if(!environment) {
        return false;
    }

    RfStreamKeystore* keystore = rf_stream_environment_get_keystore(environment);
    if(!keystore) {
        return false;
    }

    RfStreamKeyArray_t* key_data = rf_stream_keystore_get_data(keystore);
    if(!key_data) {
        return false;
    }

    for
        M_EACH(manufacture_code, *key_data, RfStreamKeyArray_t) {
            switch(manufacture_code->type) {
            case KIA_KEY1:
                kia_mf_key = manufacture_code->key;
                kia_mf_key_loaded = true;
                break;
            case KIA_KEY2:
                kia_v6_a_key = manufacture_code->key;
                kia_v6_a_key_loaded = true;
                break;
            case KIA_KEY3:
                kia_v6_b_key = manufacture_code->key;
                kia_v6_b_key_loaded = true;
                break;
            case KIA_KEY4:
                kia_v5_key = manufacture_code->key;
                kia_v5_key_loaded = true;
                break;
            default:
                break;
            }
        }

    return kia_mf_key_loaded || kia_v6_a_key_loaded || kia_v6_b_key_loaded || kia_v5_key_loaded;
}

uint64_t get_kia_mf_key() {
    return kia_mf_key;
}

uint64_t get_kia_v6_keystore_a() {
    return kia_v6_a_key;
}

uint64_t get_kia_v6_keystore_b() {
    return kia_v6_b_key;
}

uint64_t get_kia_v5_key() {
    return kia_v5_key;
}

bool rf_vehicle_keys_has_kia_mf_key(void) {
    return kia_mf_key_loaded;
}

bool rf_vehicle_keys_has_kia_v6_keystore_a(void) {
    return kia_v6_a_key_loaded;
}

bool rf_vehicle_keys_has_kia_v6_keystore_b(void) {
    return kia_v6_b_key_loaded;
}

bool rf_vehicle_keys_has_kia_v5_key(void) {
    return kia_v5_key_loaded;
}

#endif
