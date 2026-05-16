/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef _BLE_AES_CCM_
#define _BLE_AES_CCM_

#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/porting/nimble/include/os/queue.h"
#include "nimble/nimble/host/include/host/ble_hs.h"

#if MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
#include "nimble/ext/tinycrypt/include/tinycrypt/aes.h"
#else
#include "nimble/ext/tinycrypt/include/tinycrypt/aes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if MYNEWT_VAL(ENC_ADV_DATA)

const char *ble_aes_ccm_hex(const void *buf, size_t len);
int ble_aes_ccm_encrypt_be(const uint8_t *key, const uint8_t *plaintext, uint8_t *enc_data);
int ble_aes_ccm_decrypt(const uint8_t key[16], uint8_t nonce[13], const uint8_t *enc_data,
                        size_t len, const uint8_t *aad, size_t aad_len,
                        uint8_t *plaintext, size_t mic_size);
int ble_aes_ccm_encrypt(const uint8_t key[16], uint8_t nonce[13], const uint8_t *enc_data,
                        size_t len, const uint8_t *aad, size_t aad_len,
                        uint8_t *plaintext, size_t mic_size);

#endif /* ENC_ADV_DATA */

#ifdef __cplusplus
}
#endif

#endif /* _BLE_AES_CCM_ */
