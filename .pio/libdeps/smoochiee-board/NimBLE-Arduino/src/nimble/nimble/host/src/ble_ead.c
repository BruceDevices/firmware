/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "nimble/porting/nimble/include/os/queue.h"
#include "nimble/nimble/host/include/host/ble_ead.h"
#include "nimble/nimble/host/include/host/ble_aes_ccm.h"

#if MYNEWT_VAL(ENC_ADV_DATA)

#ifdef __cplusplus
extern "C" {
#endif

static uint8_t ble_ead_aad[] = {0xEA};

static int ble_ead_rand(void *buf, size_t len)
{
    int rc;
    rc = ble_hs_hci_util_rand(buf, len);
    if (rc != 0) {
        return -1;
    }
    return 0;
}

static int ble_ead_generate_randomizer(uint8_t randomizer[BLE_EAD_RANDOMIZER_SIZE])
{
    int err;
    err = ble_ead_rand(randomizer, BLE_EAD_RANDOMIZER_SIZE);

    if (err != 0) {
        return err;
    }

    randomizer[4] |= 1 << BLE_EAD_RANDOMIZER_DIRECTION_BIT;
    return 0;
}


static int ble_ead_generate_nonce(const uint8_t iv[BLE_EAD_IV_SIZE],
                                  const uint8_t randomizer[BLE_EAD_RANDOMIZER_SIZE], uint8_t *nonce)
{
    uint8_t new_randomizer[BLE_EAD_RANDOMIZER_SIZE];

    if (randomizer == NULL) {
        int err;
        err = ble_ead_generate_randomizer(new_randomizer);
        if (err != 0) {
            BLE_HS_LOG(DEBUG, "Failed to generate Randomizer");
            return -1;
        }
        randomizer = new_randomizer;
    }

    memcpy(&nonce[0], randomizer, BLE_EAD_RANDOMIZER_SIZE);
    memcpy(&nonce[BLE_EAD_RANDOMIZER_SIZE], iv, BLE_EAD_IV_SIZE);

    return 0;
}

static int ead_encrypt(const uint8_t session_key[BLE_EAD_KEY_SIZE], const uint8_t iv[BLE_EAD_IV_SIZE],
                       const uint8_t randomizer[BLE_EAD_RANDOMIZER_SIZE], const uint8_t *payload,
                       size_t payload_size, uint8_t *encrypted_payload)
{
    int err;
    uint8_t nonce[BLE_EAD_NONCE_SIZE];

    /** Nonce is concatenation of Randomizer and IV */
    err = ble_ead_generate_nonce(iv, randomizer, nonce);
    if (err != 0) {
        return -1;
    }

    /** Copying Randomizer to the start of encrypted advertisment data */
    memcpy(encrypted_payload, nonce, BLE_EAD_RANDOMIZER_SIZE);

    err = ble_aes_ccm_encrypt(session_key, nonce, payload, payload_size, ble_ead_aad, BLE_EAD_AAD_SIZE,
                              &encrypted_payload[BLE_EAD_RANDOMIZER_SIZE], BLE_EAD_MIC_SIZE);
    
    if (err != 0) {
        BLE_HS_LOG(DEBUG, "Failed to encrypt the payload (ble_ccm_encrypt err %d)", err);
        return -1;
    }

    return 0;
}

int ble_ead_encrypt(const uint8_t session_key[BLE_EAD_KEY_SIZE], const uint8_t iv[BLE_EAD_IV_SIZE],
                    const uint8_t *payload, size_t payload_size, uint8_t *encrypted_payload)
{
    if (session_key == NULL) {
        BLE_HS_LOG(DEBUG, "session_key is NULL");
        return BLE_HS_EINVAL;
    }

    if (iv == NULL) {
        BLE_HS_LOG(DEBUG, "iv is NULL");
        return BLE_HS_EINVAL;
    }

    if (payload == NULL) {
        BLE_HS_LOG(DEBUG, "payload is NULL");
        return BLE_HS_EINVAL;
    }

    if (encrypted_payload == NULL) {
        BLE_HS_LOG(DEBUG, "encrypted_payload is NULL");
        return BLE_HS_EINVAL;
    }

    if (payload_size == 0) {
        BLE_HS_LOG(DEBUG, "payload_size is set to 0. The encrypted result will only contain the "
                   "Randomizer and the MIC.");
    }

    return ead_encrypt(session_key, iv, NULL, payload, payload_size, encrypted_payload);
}

static int ead_decrypt(const uint8_t session_key[BLE_EAD_KEY_SIZE], const uint8_t iv[BLE_EAD_IV_SIZE],
                       const uint8_t *encrypted_payload, size_t encrypted_payload_size,
                       uint8_t *payload)
{
    int err;
    uint8_t nonce[BLE_EAD_NONCE_SIZE];
    const uint8_t *encrypted_ad_data = &encrypted_payload[BLE_EAD_RANDOMIZER_SIZE];
    size_t encrypted_ad_data_size = encrypted_payload_size - BLE_EAD_RANDOMIZER_SIZE;
    size_t payload_size = encrypted_ad_data_size - BLE_EAD_MIC_SIZE;

    const uint8_t *randomizer = encrypted_payload;

    err = ble_ead_generate_nonce(iv, randomizer, nonce);
    if (err != 0) {
        return -1;
    }

    err = ble_aes_ccm_decrypt(session_key, nonce, encrypted_ad_data, payload_size, ble_ead_aad,
                              BLE_EAD_AAD_SIZE, payload, BLE_EAD_MIC_SIZE);

    if (err != 0) {
        BLE_HS_LOG(DEBUG, "Failed to decrypt the data (ble_ccm_decrypt err %d)", err);
        return -1;
    }

    return 0;
}

int ble_ead_decrypt(const uint8_t session_key[BLE_EAD_KEY_SIZE], const uint8_t iv[BLE_EAD_IV_SIZE],
                    const uint8_t *encrypted_payload, size_t encrypted_payload_size,
                    uint8_t *payload)
{
    if (session_key == NULL) {
        BLE_HS_LOG(DEBUG, "session_key is NULL");
        return BLE_HS_EINVAL;
    }

    if (iv == NULL) {
        BLE_HS_LOG(DEBUG, "iv is NULL");
        return BLE_HS_EINVAL;
    }

    if (encrypted_payload == NULL) {
        BLE_HS_LOG(DEBUG, "encrypted_payload is NULL");
        return BLE_HS_EINVAL;
    }

    if (payload == NULL) {
        BLE_HS_LOG(DEBUG, "payload is NULL");
        return BLE_HS_EINVAL;
    }

    if (encrypted_payload_size < BLE_EAD_RANDOMIZER_SIZE + BLE_EAD_MIC_SIZE) {
        BLE_HS_LOG(DEBUG, "encrypted_payload_size is not large enough.");
        return BLE_HS_EINVAL;
    } else if (encrypted_payload_size == BLE_EAD_RANDOMIZER_SIZE + BLE_EAD_MIC_SIZE) {
        BLE_HS_LOG(WARN, "encrypted_payload_size not large enough to contain encrypted data.");
    }

    return ead_decrypt(session_key, iv, encrypted_payload, encrypted_payload_size, payload);
}

#endif /* ENC_ADV_DATA */

#ifdef __cplusplus
}
#endif
