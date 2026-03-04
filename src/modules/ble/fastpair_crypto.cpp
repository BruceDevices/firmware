#include "fastpair_crypto.h"
#include <mbedtls/entropy.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ccm.h>
#include "esp_random.h"

FastPairCrypto::FastPairCrypto() {
    mbedtls_ecp_group_init(&grp);
    mbedtls_mpi_init(&d);
    mbedtls_ecp_point_init(&Q);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_aes_init(&aes_ctx);

    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);

    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, 
                         (const uint8_t*)"fastpair", 8);

    mbedtls_entropy_free(&entropy);
    mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
}

FastPairCrypto::~FastPairCrypto() {
    mbedtls_ecp_group_free(&grp);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_point_free(&Q);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_aes_free(&aes_ctx);
}

bool FastPairCrypto::generateValidKeyPair(uint8_t* public_key, size_t* pub_len) {
    int ret = mbedtls_ecdh_gen_public(&grp, &d, &Q, 
                                      mbedtls_ctr_drbg_random, &ctr_drbg);
    if(ret != 0) {
        return false;
    }

    if(*pub_len >= 65) {
        size_t olen;
        ret = mbedtls_ecp_point_write_binary(&grp, &Q, 
                                            MBEDTLS_ECP_PF_UNCOMPRESSED,
                                            &olen, public_key, *pub_len);
        if(ret == 0 && olen == 65) {
            *pub_len = 65;
            return true;
        }
    }

    return false;
}

bool FastPairCrypto::generateEphemeralKeyPair(uint8_t* public_key, uint8_t* private_key) {
    mbedtls_mpi priv;
    mbedtls_ecp_point pub;

    mbedtls_mpi_init(&priv);
    mbedtls_ecp_point_init(&pub);

    int ret = mbedtls_ecdh_gen_public(&grp, &priv, &pub, 
                                      mbedtls_ctr_drbg_random, &ctr_drbg);

    if(ret == 0) {
        size_t olen;
        ret = mbedtls_ecp_point_write_binary(&grp, &pub, 
                                            MBEDTLS_ECP_PF_UNCOMPRESSED,
                                            &olen, public_key, 65);

        if(ret == 0 && olen == 65) {
            mbedtls_mpi_write_binary(&priv, private_key, 32);
        }
    }

    mbedtls_mpi_free(&priv);
    mbedtls_ecp_point_free(&pub);

    return (ret == 0);
}

bool FastPairCrypto::ecdhComputeSharedSecret(const uint8_t* private_key, 
                                            const uint8_t* peer_public_key,
                                            uint8_t* shared_secret) {
    mbedtls_mpi priv;
    mbedtls_ecp_point peer_pub;

    mbedtls_mpi_init(&priv);
    mbedtls_ecp_point_init(&peer_pub);

    mbedtls_mpi_read_binary(&priv, private_key, 32);

    int ret = mbedtls_ecp_point_read_binary(&grp, &peer_pub, 
                                           peer_public_key, 65);
    if(ret != 0) {
        mbedtls_mpi_free(&priv);
        mbedtls_ecp_point_free(&peer_pub);
        return false;
    }

    mbedtls_mpi z;
    mbedtls_mpi_init(&z);

    ret = mbedtls_ecdh_compute_shared(&grp, &z, &peer_pub, &priv, 
                                      mbedtls_ctr_drbg_random, &ctr_drbg);

    if(ret == 0) {
        mbedtls_mpi_write_binary(&z, shared_secret, 32);
    }

    mbedtls_mpi_free(&z);
    mbedtls_mpi_free(&priv);
    mbedtls_ecp_point_free(&peer_pub);

    return (ret == 0);
}

void FastPairCrypto::generatePlausibleSharedSecret(const uint8_t* their_pubkey, uint8_t* output) {
    if(looksLikeValidPublicKey(their_pubkey, 65)) {
        esp_fill_random(output, 32);
        for(int i = 0; i < 32; i += 8) {
            if(output[i] >= 0x80) output[i] &= 0x7F;
        }
    } else {
        esp_fill_random(output, 32);
    }
}

void FastPairCrypto::generatePlausibleAccountKey(const uint8_t* nonce, uint8_t* output) {
    uint8_t buffer[64];
    memcpy(buffer, nonce, 16);
    esp_fill_random(&buffer[16], 32);
    memcpy(&buffer[48], "account_key", 11);
    buffer[59] = 0x00;

    for(int i = 0; i < 16; i++) {
        output[i] = 0;
        for(int j = 0; j < 4; j++) {
            output[i] ^= buffer[i * 4 + j];
        }
        output[i] = (output[i] ^ 0x36) + 0x5C;
    }
}

bool FastPairCrypto::deriveAccountKey(const uint8_t* shared_secret, 
                                     const uint8_t* nonce,
                                     uint8_t* account_key) {
    mbedtls_aes_setkey_enc(&aes_ctx, shared_secret, 256);

    uint8_t counter[16] = {0};
    memcpy(counter, nonce, 12);
    counter[15] = 0x01;

    uint8_t input[16] = {0};
    memcpy(input, "account_key", 11);

    // FIXED: Proper mbedtls_aes_crypt_ctr parameters
    size_t nc_off = 0;
    unsigned char stream_block[16] = {0};

    int ret = mbedtls_aes_crypt_ctr(&aes_ctx, 16, &nc_off, counter, 
                                   stream_block, input, account_key);

    return (ret == 0);
}

void FastPairCrypto::generateValidNonce(uint8_t* nonce) {
    uint32_t time_part = millis();
    memcpy(nonce, &time_part, 4);
    esp_fill_random(&nonce[4], 4);

    for(int i = 8; i < 16; i++) {
        nonce[i] = esp_random() & 0xFF;
        if(i == 8) nonce[i] |= 0x80;
    }
}

bool FastPairCrypto::looksLikeValidPublicKey(const uint8_t* key, size_t len) {
    if(len != 65) return false;
    if(key[0] != 0x04) return false;
    return (key[1] < 0xFF && key[33] < 0xFF);
}

bool FastPairCrypto::fastPairEncrypt(const uint8_t* key, const uint8_t* nonce,
                                    const uint8_t* plaintext, size_t len,
                                    uint8_t* ciphertext) {
    mbedtls_ccm_context ctx;
    mbedtls_ccm_init(&ctx);

    int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if(ret != 0) {
        mbedtls_ccm_free(&ctx);
        return false;
    }

    uint8_t tag[8];
    ret = mbedtls_ccm_encrypt_and_tag(&ctx, len, nonce, 12, 
                                     NULL, 0, plaintext, ciphertext, 
                                     tag, 8);

    mbedtls_ccm_free(&ctx);
    return (ret == 0);
}

bool FastPairCrypto::fastPairDecrypt(const uint8_t* key, const uint8_t* nonce,
                                    const uint8_t* ciphertext, size_t len,
                                    uint8_t* plaintext) {
    mbedtls_ccm_context ctx;
    mbedtls_ccm_init(&ctx);

    int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if(ret != 0) {
        mbedtls_ccm_free(&ctx);
        return false;
    }

    uint8_t tag[8];
    memcpy(tag, ciphertext + len, 8);

    ret = mbedtls_ccm_auth_decrypt(&ctx, len, nonce, 12, 
                                  NULL, 0, ciphertext, plaintext, 
                                  tag, 8);

    mbedtls_ccm_free(&ctx);
    return (ret == 0);
}

void FastPairCrypto::hexDump(const char* label, const uint8_t* data, size_t len) {
    Serial.printf("[Crypto] %s: ", label);
    for(size_t i = 0; i < len; i++) {
        if(data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
    }
    Serial.println();
}