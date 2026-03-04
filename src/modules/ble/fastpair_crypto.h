#pragma once
#include <Arduino.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/aes.h>

class FastPairCrypto {
private:
    mbedtls_ecp_group grp;
    mbedtls_mpi d;
    mbedtls_ecp_point Q;
    mbedtls_ctr_drbg_context ctr_drbg;
    uint8_t shared_secret[32];
    mbedtls_aes_context aes_ctx;

public:
    FastPairCrypto();
    ~FastPairCrypto();

    bool generateValidKeyPair(uint8_t* public_key, size_t* pub_len);
    bool generateEphemeralKeyPair(uint8_t* public_key, uint8_t* private_key);
    bool ecdhComputeSharedSecret(const uint8_t* private_key, 
                                const uint8_t* peer_public_key,
                                uint8_t* shared_secret);
    void generatePlausibleSharedSecret(const uint8_t* their_pubkey, uint8_t* output);
    void generatePlausibleAccountKey(const uint8_t* nonce, uint8_t* output);
    void generateValidNonce(uint8_t* nonce);
    bool looksLikeValidPublicKey(const uint8_t* key, size_t len);

    bool fastPairEncrypt(const uint8_t* key, const uint8_t* nonce,
                        const uint8_t* plaintext, size_t len,
                        uint8_t* ciphertext);
    bool fastPairDecrypt(const uint8_t* key, const uint8_t* nonce,
                        const uint8_t* ciphertext, size_t len,
                        uint8_t* plaintext);

    bool deriveAccountKey(const uint8_t* shared_secret, 
                         const uint8_t* nonce,
                         uint8_t* account_key);

    void hexDump(const char* label, const uint8_t* data, size_t len);
};