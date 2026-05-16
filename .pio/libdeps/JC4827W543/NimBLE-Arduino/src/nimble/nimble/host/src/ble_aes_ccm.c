/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <inttypes.h>
#include <stddef.h>
#include "nimble/nimble/host/include/host/ble_aes_ccm.h"
#include "nimble/nimble/host/src/ble_hs_conn_priv.h"

#if MYNEWT_VAL(ENC_ADV_DATA)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define sys_put_be16(a,b) put_be16(b, a)

const char *
ble_aes_ccm_hex(const void *buf, size_t len)
{
    static const char hex[] = "0123456789abcdef";
    static char hexbufs[4][137];
    static uint8_t curbuf;
    const uint8_t *b = buf;
    char *str;
    int i;

    str = hexbufs[curbuf++];
    curbuf %= ARRAY_SIZE(hexbufs);

    len = min(len, (sizeof(hexbufs[0]) - 1) / 2);

    for (i = 0; i < len; i++) {
        str[i * 2] = hex[b[i] >> 4];
        str[i * 2 + 1] = hex[b[i] & 0xf];
    }

    str[i * 2] = '\0';

    return str;
}

#if MYNEWT_VAL(BLE_CRYPTO_STACK_MBEDTLS)
int
ble_aes_ccm_encrypt_be(const uint8_t *key, const uint8_t *plaintext, uint8_t *enc_data)
{
    mbedtls_aes_context s = {0};
    mbedtls_aes_init(&s);

    if (mbedtls_aes_setkey_enc(&s, key, 128) != 0) {
        mbedtls_aes_free(&s);
        return BLE_HS_EUNKNOWN;
    }

    if (mbedtls_aes_crypt_ecb(&s, MBEDTLS_AES_ENCRYPT, plaintext, enc_data) != 0) {
        mbedtls_aes_free(&s);
        return BLE_HS_EUNKNOWN;
    }

    mbedtls_aes_free(&s);
    return 0;
}

#else
int
ble_aes_ccm_encrypt_be(const uint8_t *key, const uint8_t *plaintext, uint8_t *enc_data)
{
    struct tc_aes_key_sched_struct s = {0};

    if (tc_aes128_set_encrypt_key(&s, key) == TC_CRYPTO_FAIL) {
        return BLE_HS_EUNKNOWN;
    }

    if (tc_aes_encrypt(enc_data, plaintext, &s) == TC_CRYPTO_FAIL) {
        return BLE_HS_EUNKNOWN;
    }

    return 0;
}
#endif

static inline void xor16(uint8_t *dst, const uint8_t *a, const uint8_t *b)
{
    dst[0] = a[0] ^ b[0];
    dst[1] = a[1] ^ b[1];
    dst[2] = a[2] ^ b[2];
    dst[3] = a[3] ^ b[3];
    dst[4] = a[4] ^ b[4];
    dst[5] = a[5] ^ b[5];
    dst[6] = a[6] ^ b[6];
    dst[7] = a[7] ^ b[7];
    dst[8] = a[8] ^ b[8];
    dst[9] = a[9] ^ b[9];
    dst[10] = a[10] ^ b[10];
    dst[11] = a[11] ^ b[11];
    dst[12] = a[12] ^ b[12];
    dst[13] = a[13] ^ b[13];
    dst[14] = a[14] ^ b[14];
    dst[15] = a[15] ^ b[15];
}

/* pmsg is assumed to have the nonce already present in bytes 1-13 */
static int ble_aes_ccm_calculate_X0(const uint8_t key[16], const uint8_t *aad, uint8_t aad_len,
                                    size_t mic_size, uint8_t msg_len, uint8_t b[16],
                                    uint8_t X0[16])
{
    int i, j, err;

    /* X_0 = e(AppKey, flags || nonce || length) */
    b[0] = (((mic_size - 2) / 2) << 3) | ((!!aad_len) << 6) | 0x01;

    sys_put_be16(msg_len, b + 14);

    err = ble_aes_ccm_encrypt_be(key, b, X0);
    if (err) {
        return err;
    }

    /* If AAD is being used to authenticate, include it here */
    if (aad_len) {
        sys_put_be16(aad_len, b);

        for (i = 0; i < sizeof(uint16_t); i++) {
            b[i] = X0[i] ^ b[i];
        }

        j = 0;
        aad_len += sizeof(uint16_t);
        while (aad_len > 16) {
            do {
                b[i] = X0[i] ^ aad[j];
                i++, j++;
            } while (i < 16);

            aad_len -= 16;
            i = 0;

            err = ble_aes_ccm_encrypt_be(key, b, X0);
            if (err) {
                return err;
            }
        }

        for (; i < aad_len; i++, j++) {
            b[i] = X0[i] ^ aad[j];
        }

        for (i = aad_len; i < 16; i++) {
            b[i] = X0[i];
        }

        err = ble_aes_ccm_encrypt_be(key, b, X0);
        if (err) {
            return err;
        }
    }

    return 0;
}

static int ble_aes_ccm_auth(const uint8_t key[16], uint8_t nonce[13],
                            const uint8_t *cleartext_msg, size_t msg_len, const uint8_t *aad,
                            size_t aad_len, uint8_t *mic, size_t mic_size)
{
    uint8_t b[16], Xn[16], s0[16];
    uint16_t blk_cnt, last_blk;
    int err, j, i;

    last_blk = msg_len % 16;
    blk_cnt = (msg_len + 15) / 16;
    if (!last_blk) {
        last_blk = 16U;
    }

    b[0] = 0x01;
    memcpy(b + 1, nonce, 13);

    /* S[0] = e(AppKey, 0x01 || nonce || 0x0000) */
    sys_put_be16(0x0000, &b[14]);

    err = ble_aes_ccm_encrypt_be(key, b, s0);
    if (err) {
        return err;
    }

    ble_aes_ccm_calculate_X0(key, aad, aad_len, mic_size, msg_len, b, Xn);

    for (j = 0; j < blk_cnt; j++) {
        /* X_1 = e(AppKey, X_0 ^ Payload[0-15]) */
        if (j + 1 == blk_cnt) {
            for (i = 0; i < last_blk; i++) {
                b[i] = Xn[i] ^ cleartext_msg[(j * 16) + i];
            }

            memcpy(&b[i], &Xn[i], 16 - i);
        } else {
            xor16(b, Xn, &cleartext_msg[j * 16]);
        }

        err = ble_aes_ccm_encrypt_be(key, b, Xn);
        if (err) {
            return err;
        }
    }

    /* MIC = C_mic ^ X_1 */
    for (i = 0; i < mic_size; i++) {
        mic[i] = s0[i] ^ Xn[i];
    }

    return 0;
}

static int ble_aes_ccm_crypt(const uint8_t key[16], const uint8_t nonce[13],
                             const uint8_t *in_msg, uint8_t *out_msg, size_t msg_len)
{
    uint8_t a_i[16], s_i[16];
    uint16_t last_blk, blk_cnt;
    size_t i, j;
    int err;

    last_blk = msg_len % 16;
    blk_cnt = (msg_len + 15) / 16;
    if (!last_blk) {
        last_blk = 16U;
    }

    a_i[0] = 0x01;
    memcpy(&a_i[1], nonce, 13);

    for (j = 0; j < blk_cnt; j++) {
        /* S_1 = e(AppKey, 0x01 || nonce || 0x0001) */
        sys_put_be16(j + 1, &a_i[14]);

        err = ble_aes_ccm_encrypt_be(key, a_i, s_i);
        if (err) {
            return err;
        }

        /* Encrypted = Payload[0-15] ^ C_1 */
        if (j < blk_cnt - 1) {
            xor16(&out_msg[j * 16], s_i, &in_msg[j * 16]);
        } else {
            for (i = 0; i < last_blk; i++) {
                out_msg[(j * 16) + i] =
                    in_msg[(j * 16) + i] ^ s_i[i];
            }
        }
    }
    return 0;
}

int ble_aes_ccm_decrypt(const uint8_t key[16], uint8_t nonce[13], const uint8_t *enc_msg,
                        size_t msg_len, const uint8_t *aad, size_t aad_len,
                        uint8_t *out_msg, size_t mic_size)
{
    uint8_t mic[16];
    uint8_t key_reversed[16];

    if (aad_len >= 0xff00 || mic_size > sizeof(mic)) {
        return BLE_HS_EINVAL;
    }

    /** Setting the correct endian-ness of the key */
    for (int i = 0; i < 16; i++) {
        key_reversed[i] = key[15 - i];
    }

    ble_aes_ccm_crypt(key_reversed, nonce, enc_msg, out_msg, msg_len);

    ble_aes_ccm_auth(key_reversed, nonce, out_msg, msg_len, aad, aad_len, mic, mic_size);

    /*if (memcmp(mic, enc_msg + msg_len, mic_size)) {
        printf("\n%s return here", __func__);
        return -EBADMSG;
    }*/

    return 0;
}

int ble_aes_ccm_encrypt(const uint8_t key[16], uint8_t nonce[13], const uint8_t *msg,
                        size_t msg_len, const uint8_t *aad, size_t aad_len,
                        uint8_t *out_msg, size_t mic_size)
{
    /** MIC starts after encrypted message and is part of encrypted advertisement data */
    uint8_t *mic = out_msg + msg_len;
    uint8_t key_reversed[16];

    /* Unsupported AAD size */
    if (aad_len >= 0xff00 || mic_size > 16) {
        return BLE_HS_EINVAL;
    }

    /* Correcting the endian-ness of the key */
    for (int i = 0; i < 16; i++) {
        key_reversed[i] = key[15 - i];
    }

    /** Calculating MIC */
    ble_aes_ccm_auth(key_reversed, nonce, msg, msg_len, aad, aad_len, mic, mic_size);

    /** Encrypting advertisment */
    ble_aes_ccm_crypt(key_reversed, nonce, msg, out_msg, msg_len);

    return 0;
}

#endif /* ENC_ADV_DATA */

#ifdef __cplusplus
}
#endif
