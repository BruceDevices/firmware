/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef H_BLE_EAD_
#define H_BLE_EAD_

#include "nimble/porting/nimble/include/os/queue.h"
#include <inttypes.h>
#include "nimble/porting/nimble/include/syscfg/syscfg.h"
#include "nimble/nimble/host/include/host/ble_gap.h"

#ifdef __cplusplus
extern "C" {
#endif

#if MYNEWT_VAL(ENC_ADV_DATA)

/** Randomizer size in bytes */
#define BLE_EAD_RANDOMIZER_SIZE     5

/** Key size in bytes */
#define BLE_EAD_KEY_SIZE            16

/** Initialisation Vector size in bytes */
#define BLE_EAD_IV_SIZE             8

/** MIC size in bytes */
#define BLE_EAD_MIC_SIZE            4

/** nonce size in bytes */
#define BLE_EAD_NONCE_SIZE 13

/* This value is used to set the directionBit of the CCM  nonce to the MSB of the Randomizer field
 * (see Supplement to the Bluetooth Core Specification v11, Part A 1.23.3)
 */
#define BLE_EAD_RANDOMIZER_DIRECTION_BIT 7

/** Additional Authenticated Data size in bytes */
#define BLE_EAD_AAD_SIZE 1


/** Get the size (in bytes) of the encrypted advertising data for a given
 *  payload size in bytes.
 */
#define BLE_EAD_ENCRYPTED_PAYLOAD_SIZE(payload_size)                                                \
    ((payload_size) + BLE_EAD_RANDOMIZER_SIZE + BLE_EAD_MIC_SIZE)

/** Get the size (in bytes) of the decrypted payload for a given payload size in
 *  bytes.
 */
#define BLE_EAD_DECRYPTED_PAYLOAD_SIZE(encrypted_payload_size)                                      \
    ((encrypted_payload_size) - (BLE_EAD_RANDOMIZER_SIZE + BLE_EAD_MIC_SIZE))

struct key_material {
    uint8_t session_key[BLE_EAD_KEY_SIZE];
    uint8_t iv[BLE_EAD_IV_SIZE];
};

/**
 * @brief Encrypt and authenticate the given advertising data.
 *
 * The resulting data in @p encrypted_payload will look like that:
 * - Randomizer is added in the @ref BLE_EAD_RANDOMIZER_SIZE first bytes;
 * - Encrypted payload is added ( @p payload_size bytes);
 * - MIC is added in the last @ref BLE_EAD_MIC_SIZE bytes.
 *
 * NOTE:
 * - The function must be called each time the RPA is updated or the
 *   data are modified.
 *
 * - The term `advertising structure` is used to describe the advertising
 *   data with the advertising type and the length of those two.
 *
 * @session_key         key of BLE_EAD_KEY_SIZE bytes used for the
 *                      encryption.
 * @iv                  Initialisation Vector used to generate the nonce. It must be
 *                      changed each time the Session Key changes.
 * @payload             Advertising Data to encrypt. Can be multiple advertising
 *                      structures that are concatenated.
 * @payload_size        Size of the Advertising Data to encrypt.
 * @encrypted_payload   Encrypted Ad Data including the Randomizer and
 *                      the MIC. Size must be at least @ref BLE_EAD_RANDOMIZER_SIZE + @p
 *                      payload_size + @ref BLE_EAD_MIC_SIZE. Use @ref
 *                      BLE_EAD_ENCRYPTED_PAYLOAD_SIZE to get the right size.
 *
 * @return              0 on success;
 *                      BLE_HS_EINVAL if the specified value is not
 *                      within the allowed range.
 *                      BLE_HS_ECANCEL if error occurred during the random number
 *                      generation
 */
int ble_ead_encrypt(const uint8_t session_key[BLE_EAD_KEY_SIZE],
                    const uint8_t iv[BLE_EAD_IV_SIZE], const uint8_t *payload,
                    size_t payload_size, uint8_t *encrypted_payload);

/**
 * @brief Decrypt and authenticate the given encrypted advertising data.
 *
 * @note The term `advertising structure` is used to describe the advertising
 *       data with the advertising type and the length of those two.
 *
 * @session_key                 Key of 16 bytes used for the encryption.
 * @iv                          Initialisation Vector used to generate the `nonce`.
 * @encrypted_payload           Encrypted Advertising Data received. This
 *                              should only contain the advertising data from the received
 *                              advertising structure, not the length nor the type.
 * @encrypted_payload_size      Size of the received advertising data in
 *                              bytes. Should be equal to the length field of the received
 *                              advertising structure, minus the size of the type (1 byte).
 * @payload                     Decrypted advertising payload. Use @ref
 *                              BLE_EAD_DECRYPTED_PAYLOAD_SIZE to get the right size.
 *
 * @return                      0 on success;
 *                              BLE_HS_EINVAL if the specified value is not
 *                              within the allowed range.
 */
int ble_ead_decrypt(const uint8_t session_key[BLE_EAD_KEY_SIZE],
                    const uint8_t iv[BLE_EAD_IV_SIZE], const uint8_t *encrypted_payload,
                    size_t encrypted_payload_size, uint8_t *payload);

#endif /* ENC_ADV_DATA */

#ifdef __cplusplus
}
#endif

#endif
