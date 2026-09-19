#pragma once
#if !defined(LITE_VERSION)
#include <Arduino.h>
#include <mbedtls/aes.h>
#include <mbedtls/ccm.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecp.h>
#include <mbedtls/md.h>
#include <mbedtls/entropy.h>

// FastPair Protocol Constants
#define FASTPAIR_PUBLIC_KEY_LEN     65
#define FASTPAIR_PRIVATE_KEY_LEN    32
#define FASTPAIR_SHARED_SECRET_LEN  32
#define FASTPAIR_NONCE_LEN          16
#define FASTPAIR_TAG_LEN            8
#define FASTPAIR_ACCOUNT_KEY_LEN    16
#define FASTPAIR_AES_KEY_LEN        16

// FastPair Version
enum FastPairProtocolVersion {
    FP_VERSION_1 = 1,
    FP_VERSION_2 = 2,
    FP_VERSION_3 = 3
};

// FastPair Message Types
enum FastPairMessageType {
    FP_MSG_HELLO = 0x00,
    FP_MSG_KEY_EXCHANGE = 0x01,
    FP_MSG_SECURE = 0x02,
    FP_MSG_ACK = 0x03,
    FP_MSG_ERROR = 0xFF
};

// FastPair Context
struct FastPairContext {
    uint8_t local_public_key[FASTPAIR_PUBLIC_KEY_LEN];
    uint8_t local_private_key[FASTPAIR_PRIVATE_KEY_LEN];
    uint8_t peer_public_key[FASTPAIR_PUBLIC_KEY_LEN];
    uint8_t shared_secret[FASTPAIR_SHARED_SECRET_LEN];
    uint8_t account_key[FASTPAIR_ACCOUNT_KEY_LEN];
    uint8_t nonce[FASTPAIR_NONCE_LEN];
    uint8_t session_key[FASTPAIR_AES_KEY_LEN];
    uint32_t sequence_number;
    bool handshake_complete;
    bool encrypted;
    FastPairProtocolVersion version;
};

class FastPairCrypto {
private:
    mbedtls_ecp_group grp;
    mbedtls_ecp_point Q;
    mbedtls_mpi d;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    mbedtls_aes_context aes_ctx;
    FastPairContext ctx;
    bool initialized;

    bool generateKeyPairInternal(uint8_t *public_key, size_t *pub_len);
    bool computeSharedSecretInternal(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret);
    bool deriveSessionKey(const uint8_t *shared_secret, const uint8_t *nonce, uint8_t *session_key);
    bool deriveAccountKeyInternal(const uint8_t *shared_secret, const uint8_t *nonce, uint8_t *account_key);
    bool deriveAESKey(const uint8_t *shared_secret, const uint8_t *nonce, uint8_t *aes_key);

public:
    FastPairCrypto();
    ~FastPairCrypto();

    // Initialization
    bool init();
    void deinit();
    bool isInitialized();

    // Key Generation
    bool generateKeyPair(uint8_t *public_key, size_t *pub_len);
    bool generateEphemeralKeyPair(uint8_t *public_key, uint8_t *private_key);
    bool generateValidKeyPair(uint8_t *public_key, size_t *pub_len);

    // ECDH Operations
    bool ecdhComputeSharedSecret(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret);
    bool ecdhComputeSharedSecretRaw(const uint8_t *private_key, const uint8_t *peer_public_key, uint8_t *shared_secret);

    // FastPair Protocol Operations
    bool performHandshake(const uint8_t *peer_public_key);
    bool createHandshakeMessage(uint8_t *message, size_t *msg_len);
    bool parseHandshakeMessage(const uint8_t *message, size_t msg_len);
    bool createSecureMessage(const uint8_t *data, size_t data_len, uint8_t *encrypted, size_t *out_len);
    bool parseSecureMessage(const uint8_t *encrypted, size_t enc_len, uint8_t *data, size_t *out_len);

    // FastPair Key Derivation
    bool deriveAccountKey(const uint8_t *shared_secret, const uint8_t *nonce, uint8_t *account_key);
    bool deriveSessionKeyFromSecret(const uint8_t *shared_secret, const uint8_t *salt, uint8_t *session_key);
    bool deriveAESKeyFromSecret(const uint8_t *shared_secret, const uint8_t *nonce, uint8_t *aes_key);

    // Encryption
    bool fastPairEncrypt(const uint8_t *key, const uint8_t *nonce, const uint8_t *plaintext, size_t len, uint8_t *ciphertext);
    bool fastPairDecrypt(const uint8_t *key, const uint8_t *nonce, const uint8_t *ciphertext, size_t len, uint8_t *plaintext);
    bool fastPairEncryptWithTag(const uint8_t *key, const uint8_t *nonce, const uint8_t *plaintext, size_t len, uint8_t *ciphertext, uint8_t *tag);
    bool fastPairDecryptWithTag(const uint8_t *key, const uint8_t *nonce, const uint8_t *ciphertext, size_t len, const uint8_t *tag, uint8_t *plaintext);

    // Nonce Generation
    void generateNonce(uint8_t *nonce);
    void generateValidNonce(uint8_t *nonce);
    void incrementNonce(uint8_t *nonce, uint32_t increment = 1);

    // Utilities
    bool looksLikeValidPublicKey(const uint8_t *key, size_t len);
    bool validatePublicKey(const uint8_t *key, size_t len);
    bool validatePrivateKey(const uint8_t *key, size_t len);
    bool validateSharedSecret(const uint8_t *secret, size_t len);
    void generatePlausibleSharedSecret(const uint8_t *their_pubkey, uint8_t *output);
    void generatePlausibleAccountKey(const uint8_t *nonce, uint8_t *output);
    void copyPublicKey(const uint8_t *src, uint8_t *dst);
    void copyPrivateKey(const uint8_t *src, uint8_t *dst);
    bool areKeysEqual(const uint8_t *key1, const uint8_t *key2, size_t len);

    // HMAC Operations
    bool hmacSha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *output);
    bool hmacSha256Verify(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, const uint8_t *expected);

    // Hash Operations
    bool sha256Hash(const uint8_t *data, size_t data_len, uint8_t *output);
    bool sha256HashVerify(const uint8_t *data, size_t data_len, const uint8_t *expected);

    // HKDF Operations
    bool hkdfExtract(const uint8_t *salt, size_t salt_len, const uint8_t *ikm, size_t ikm_len, uint8_t *prk);
    bool hkdfExpand(const uint8_t *prk, size_t prk_len, const uint8_t *info, size_t info_len, uint8_t *okm, size_t okm_len);
    bool hkdf(const uint8_t *salt, size_t salt_len, const uint8_t *ikm, size_t ikm_len, const uint8_t *info, size_t info_len, uint8_t *okm, size_t okm_len);

    // Debug
    void hexDump(const char *label, const uint8_t *data, size_t len);
    void printContext();

    // Getters
    FastPairContext *getContext();
    bool isHandshakeComplete();
    bool isEncrypted();
    FastPairProtocolVersion getVersion();
    uint32_t getSequenceNumber();
    const uint8_t* getLocalPublicKey();
    const uint8_t* getPeerPublicKey();
    const uint8_t* getSharedSecret();
    const uint8_t* getAccountKey();
    const uint8_t* getSessionKey();
};
#endif