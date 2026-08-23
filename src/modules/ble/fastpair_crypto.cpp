#if !defined(LITE_VERSION)
#include "fastpair_crypto.h"
#include "esp_random.h"
#include <mbedtls/ccm.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/hkdf.h>
#include <cstring>

//=============================================================================
// Constructor / Destructor
//=============================================================================

FastPairCrypto::FastPairCrypto() {
    initialized = false;
    memset(&ctx, 0, sizeof(ctx));
    mbedtls_ecp_group_init(&grp);
    mbedtls_ecp_point_init(&Q);
    mbedtls_mpi_init(&d);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_aes_init(&aes_ctx);
    ctx.version = FP_VERSION_2;
    ctx.sequence_number = 0;
    ctx.handshake_complete = false;
    ctx.encrypted = false;
    init();
}

FastPairCrypto::~FastPairCrypto() {
    deinit();
    mbedtls_ecp_group_free(&grp);
    mbedtls_ecp_point_free(&Q);
    mbedtls_mpi_free(&d);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    mbedtls_aes_free(&aes_ctx);
}

//=============================================================================
// Initialization
//=============================================================================

bool FastPairCrypto::init() {
    if (initialized) return true;

    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, 
                         (const uint8_t *)"fastpair_crypto_v2", 19);
    mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    
    initialized = true;
    return true;
}

void FastPairCrypto::deinit() {
    memset(&ctx, 0, sizeof(ctx));
    initialized = false;
}

bool FastPairCrypto::isInitialized() {
    return initialized;
}

//=============================================================================
// Key Generation
//=============================================================================

bool FastPairCrypto::generateKeyPairInternal(uint8_t *public_key, size_t *pub_len) {
    if (!initialized) {
        if (!init()) return false;
    }

    mbedtls_mpi d_local;
    mbedtls_ecp_point Q_local;
    mbedtls_mpi_init(&d_local);
    mbedtls_ecp_point_init(&Q_local);

    int ret = mbedtls_ecdh_gen_public(&grp, &d_local, &Q_local, 
                                      mbedtls_ctr_drbg_random, &ctr_drbg);

    if (ret == 0 && *pub_len >= FASTPAIR_PUBLIC_KEY_LEN) {
        size_t olen;
        ret = mbedtls_ecp_point_write_binary(&grp, &Q_local, 
                                             MBEDTLS_ECP_PF_UNCOMPRESSED, 
                                             &olen, public_key, *pub_len);
        if (ret == 0 && olen == FASTPAIR_PUBLIC_KEY_LEN) {
            *pub_len = FASTPAIR_PUBLIC_KEY_LEN;
            // Store in context
            memcpy(ctx.local_public_key, public_key, FASTPAIR_PUBLIC_KEY_LEN);
            mbedtls_mpi_write_binary(&d_local, ctx.local_private_key, FASTPAIR_PRIVATE_KEY_LEN);
        }
    }

    mbedtls_mpi_free(&d_local);
    mbedtls_ecp_point_free(&Q_local);
    return (ret == 0);
}

bool FastPairCrypto::generateKeyPair(uint8_t *public_key, size_t *pub_len) {
    return generateKeyPairInternal(public_key, pub_len);
}

bool FastPairCrypto::generateEphemeralKeyPair(uint8_t *public_key, uint8_t *private_key) {
    if (!initialized) {
        if (!init()) return false;
    }

    mbedtls_mpi d_priv;
    mbedtls_ecp_point Q_pub;
    mbedtls_mpi_init(&d_priv);
    mbedtls_ecp_point_init(&Q_pub);

    int ret = mbedtls_ecdh_gen_public(&grp, &d_priv, &Q_pub, 
                                      mbedtls_ctr_drbg_random, &ctr_drbg);

    if (ret == 0) {
        size_t olen;
        ret = mbedtls_ecp_point_write_binary(&grp, &Q_pub, 
                                             MBEDTLS_ECP_PF_UNCOMPRESSED, 
                                             &olen, public_key, FASTPAIR_PUBLIC_KEY_LEN);
        if (ret == 0 && olen == FASTPAIR_PUBLIC_KEY_LEN) {
            mbedtls_mpi_write_binary(&d_priv, private_key, FASTPAIR_PRIVATE_KEY_LEN);
        }
    }

    mbedtls_mpi_free(&d_priv);
    mbedtls_ecp_point_free(&Q_pub);
    return (ret == 0);
}

bool FastPairCrypto::generateValidKeyPair(uint8_t *public_key, size_t *pub_len) {
    return generateKeyPair(public_key, pub_len);
}

//=============================================================================
// ECDH Operations
//=============================================================================

bool FastPairCrypto::computeSharedSecretInternal(const uint8_t *private_key, 
                                                  const uint8_t *peer_public_key, 
                                                  uint8_t *shared_secret) {
    if (!initialized) {
        if (!init()) return false;
    }

    mbedtls_mpi priv;
    mbedtls_ecp_point peer_pub;
    mbedtls_mpi z;

    mbedtls_mpi_init(&priv);
    mbedtls_ecp_point_init(&peer_pub);
    mbedtls_mpi_init(&z);

    mbedtls_mpi_read_binary(&priv, private_key, FASTPAIR_PRIVATE_KEY_LEN);

    int ret = mbedtls_ecp_point_read_binary(&grp, &peer_pub, peer_public_key, FASTPAIR_PUBLIC_KEY_LEN);
    if (ret != 0) {
        mbedtls_mpi_free(&priv);
        mbedtls_ecp_point_free(&peer_pub);
        mbedtls_mpi_free(&z);
        return false;
    }

    ret = mbedtls_ecdh_compute_shared(&grp, &z, &peer_pub, &priv, 
                                      mbedtls_ctr_drbg_random, &ctr_drbg);

    if (ret == 0) {
        mbedtls_mpi_write_binary(&z, shared_secret, FASTPAIR_SHARED_SECRET_LEN);
    }

    mbedtls_mpi_free(&priv);
    mbedtls_ecp_point_free(&peer_pub);
    mbedtls_mpi_free(&z);
    return (ret == 0);
}

bool FastPairCrypto::ecdhComputeSharedSecret(const uint8_t *private_key, 
                                             const uint8_t *peer_public_key, 
                                             uint8_t *shared_secret) {
    return computeSharedSecretInternal(private_key, peer_public_key, shared_secret);
}

bool FastPairCrypto::ecdhComputeSharedSecretRaw(const uint8_t *private_key, 
                                                const uint8_t *peer_public_key, 
                                                uint8_t *shared_secret) {
    return computeSharedSecretInternal(private_key, peer_public_key, shared_secret);
}

//=============================================================================
// FastPair Protocol Operations
//=============================================================================

bool FastPairCrypto::performHandshake(const uint8_t *peer_public_key) {
    if (!initialized) {
        if (!init()) return false;
    }

    // Store peer public key
    memcpy(ctx.peer_public_key, peer_public_key, FASTPAIR_PUBLIC_KEY_LEN);

    // Generate our key pair if not already done
    if (ctx.local_public_key[0] == 0) {
        size_t pub_len = FASTPAIR_PUBLIC_KEY_LEN;
        if (!generateKeyPair(ctx.local_public_key, &pub_len)) {
            return false;
        }
    }

    // Compute shared secret
    if (!computeSharedSecretInternal(ctx.local_private_key, peer_public_key, ctx.shared_secret)) {
        return false;
    }

    // Generate nonce
    generateNonce(ctx.nonce);

    // Derive session key
    if (!deriveSessionKey(ctx.shared_secret, ctx.nonce, ctx.session_key)) {
        return false;
    }

    ctx.handshake_complete = true;
    ctx.sequence_number = 0;
    return true;
}

bool FastPairCrypto::createHandshakeMessage(uint8_t *message, size_t *msg_len) {
    if (!ctx.handshake_complete) {
        return false;
    }

    // Format: [type(1)] [public_key(65)] [nonce(16)]
    size_t pos = 0;
    message[pos++] = FP_MSG_KEY_EXCHANGE;
    memcpy(&message[pos], ctx.local_public_key, FASTPAIR_PUBLIC_KEY_LEN);
    pos += FASTPAIR_PUBLIC_KEY_LEN;
    memcpy(&message[pos], ctx.nonce, FASTPAIR_NONCE_LEN);
    pos += FASTPAIR_NONCE_LEN;

    *msg_len = pos;
    return true;
}

bool FastPairCrypto::parseHandshakeMessage(const uint8_t *message, size_t msg_len) {
    if (msg_len < 1 + FASTPAIR_PUBLIC_KEY_LEN + FASTPAIR_NONCE_LEN) {
        return false;
    }

    size_t pos = 0;
    uint8_t type = message[pos++];
    if (type != FP_MSG_KEY_EXCHANGE) {
        return false;
    }

    memcpy(ctx.peer_public_key, &message[pos], FASTPAIR_PUBLIC_KEY_LEN);
    pos += FASTPAIR_PUBLIC_KEY_LEN;
    memcpy(ctx.nonce, &message[pos], FASTPAIR_NONCE_LEN);

    return true;
}

bool FastPairCrypto::createSecureMessage(const uint8_t *data, size_t data_len, 
                                          uint8_t *encrypted, size_t *out_len) {
    if (!ctx.handshake_complete || !ctx.encrypted) {
        return false;
    }

    // Format: [type(1)] [seq(4)] [encrypted_data] [tag(8)]
    size_t pos = 0;
    encrypted[pos++] = FP_MSG_SECURE;

    // Sequence number
    encrypted[pos++] = (ctx.sequence_number >> 24) & 0xFF;
    encrypted[pos++] = (ctx.sequence_number >> 16) & 0xFF;
    encrypted[pos++] = (ctx.sequence_number >> 8) & 0xFF;
    encrypted[pos++] = ctx.sequence_number & 0xFF;

    uint8_t tag[FASTPAIR_TAG_LEN];
    if (!fastPairEncryptWithTag(ctx.session_key, ctx.nonce, data, data_len, 
                                &encrypted[pos], tag)) {
        return false;
    }
    pos += data_len;
    memcpy(&encrypted[pos], tag, FASTPAIR_TAG_LEN);
    pos += FASTPAIR_TAG_LEN;

    *out_len = pos;
    ctx.sequence_number++;
    return true;
}

bool FastPairCrypto::parseSecureMessage(const uint8_t *encrypted, size_t enc_len, 
                                        uint8_t *data, size_t *out_len) {
    if (enc_len < 1 + 4 + FASTPAIR_TAG_LEN) {
        return false;
    }

    size_t pos = 0;
    uint8_t type = encrypted[pos++];
    if (type != FP_MSG_SECURE) {
        return false;
    }

    uint32_t seq = (encrypted[pos] << 24) | (encrypted[pos+1] << 16) | 
                   (encrypted[pos+2] << 8) | encrypted[pos+3];
    pos += 4;

    size_t data_len = enc_len - pos - FASTPAIR_TAG_LEN;
    const uint8_t *ciphertext = &encrypted[pos];
    const uint8_t *tag = &encrypted[pos + data_len];

    if (!fastPairDecryptWithTag(ctx.session_key, ctx.nonce, ciphertext, data_len, tag, data)) {
        return false;
    }

    *out_len = data_len;
    ctx.sequence_number = seq + 1;
    return true;
}

//=============================================================================
// Key Derivation
//=============================================================================

bool FastPairCrypto::deriveSessionKey(const uint8_t *shared_secret, const uint8_t *nonce, uint8_t *session_key) {
    return hkdf(shared_secret, FASTPAIR_SHARED_SECRET_LEN,
                nonce, FASTPAIR_NONCE_LEN,
                (const uint8_t*)"FastPair Session Key", 22,
                session_key, FASTPAIR_AES_KEY_LEN);
}

bool FastPairCrypto::deriveAccountKey(const uint8_t *shared_secret, const uint8_t *nonce, uint8_t *account_key) {
    return deriveAccountKeyInternal(shared_secret, nonce, account_key);
}

bool FastPairCrypto::deriveAccountKeyInternal(const uint8_t *shared_secret, const uint8_t *nonce, uint8_t *account_key) {
    return hkdf(shared_secret, FASTPAIR_SHARED_SECRET_LEN,
                nonce, FASTPAIR_NONCE_LEN,
                (const uint8_t*)"FastPair Account Key", 21,
                account_key, FASTPAIR_ACCOUNT_KEY_LEN);
}

bool FastPairCrypto::deriveSessionKeyFromSecret(const uint8_t *shared_secret, const uint8_t *salt, uint8_t *session_key) {
    return hkdf(shared_secret, FASTPAIR_SHARED_SECRET_LEN,
                salt, FASTPAIR_NONCE_LEN,
                (const uint8_t*)"FastPair Session Key", 22,
                session_key, FASTPAIR_AES_KEY_LEN);
}

bool FastPairCrypto::deriveAESKeyFromSecret(const uint8_t *shared_secret, const uint8_t *nonce, uint8_t *aes_key) {
    return deriveAESKey(shared_secret, nonce, aes_key);
}

bool FastPairCrypto::deriveAESKey(const uint8_t *shared_secret, const uint8_t *nonce, uint8_t *aes_key) {
    return deriveSessionKey(shared_secret, nonce, aes_key);
}

//=============================================================================
// Encryption
//=============================================================================

bool FastPairCrypto::fastPairEncrypt(const uint8_t *key, const uint8_t *nonce, 
                                     const uint8_t *plaintext, size_t len, 
                                     uint8_t *ciphertext) {
    mbedtls_ccm_context ctx_ccm;
    mbedtls_ccm_init(&ctx_ccm);

    int ret = mbedtls_ccm_setkey(&ctx_ccm, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret != 0) {
        mbedtls_ccm_free(&ctx_ccm);
        return false;
    }

    ret = mbedtls_ccm_encrypt_and_tag(&ctx_ccm, len, nonce, 12, NULL, 0,
                                      plaintext, ciphertext, NULL, 0);

    mbedtls_ccm_free(&ctx_ccm);
    return (ret == 0);
}

bool FastPairCrypto::fastPairDecrypt(const uint8_t *key, const uint8_t *nonce, 
                                     const uint8_t *ciphertext, size_t len, 
                                     uint8_t *plaintext) {
    mbedtls_ccm_context ctx_ccm;
    mbedtls_ccm_init(&ctx_ccm);

    int ret = mbedtls_ccm_setkey(&ctx_ccm, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret != 0) {
        mbedtls_ccm_free(&ctx_ccm);
        return false;
    }

    ret = mbedtls_ccm_auth_decrypt(&ctx_ccm, len, nonce, 12, NULL, 0,
                                   ciphertext, plaintext, NULL, 0);

    mbedtls_ccm_free(&ctx_ccm);
    return (ret == 0);
}

bool FastPairCrypto::fastPairEncryptWithTag(const uint8_t *key, const uint8_t *nonce,
                                            const uint8_t *plaintext, size_t len,
                                            uint8_t *ciphertext, uint8_t *tag) {
    mbedtls_ccm_context ctx_ccm;
    mbedtls_ccm_init(&ctx_ccm);

    int ret = mbedtls_ccm_setkey(&ctx_ccm, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret != 0) {
        mbedtls_ccm_free(&ctx_ccm);
        return false;
    }

    ret = mbedtls_ccm_encrypt_and_tag(&ctx_ccm, len, nonce, 12, NULL, 0,
                                      plaintext, ciphertext, tag, FASTPAIR_TAG_LEN);

    mbedtls_ccm_free(&ctx_ccm);
    return (ret == 0);
}

bool FastPairCrypto::fastPairDecryptWithTag(const uint8_t *key, const uint8_t *nonce,
                                            const uint8_t *ciphertext, size_t len,
                                            const uint8_t *tag, uint8_t *plaintext) {
    mbedtls_ccm_context ctx_ccm;
    mbedtls_ccm_init(&ctx_ccm);

    int ret = mbedtls_ccm_setkey(&ctx_ccm, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (ret != 0) {
        mbedtls_ccm_free(&ctx_ccm);
        return false;
    }

    ret = mbedtls_ccm_auth_decrypt(&ctx_ccm, len, nonce, 12, NULL, 0,
                                   ciphertext, plaintext, tag, FASTPAIR_TAG_LEN);

    mbedtls_ccm_free(&ctx_ccm);
    return (ret == 0);
}

//=============================================================================
// Nonce Generation
//=============================================================================

void FastPairCrypto::generateNonce(uint8_t *nonce) {
    uint32_t time_part = millis();
    memcpy(nonce, &time_part, 4);
    esp_fill_random(&nonce[4], 4);
    for (int i = 8; i < FASTPAIR_NONCE_LEN; i++) {
        nonce[i] = esp_random() & 0xFF;
    }
}

void FastPairCrypto::generateValidNonce(uint8_t *nonce) {
    generateNonce(nonce);
}

void FastPairCrypto::incrementNonce(uint8_t *nonce, uint32_t increment) {
    uint64_t val = 0;
    for (int i = 0; i < FASTPAIR_NONCE_LEN; i++) {
        val = (val << 8) | nonce[i];
    }
    val += increment;
    for (int i = FASTPAIR_NONCE_LEN - 1; i >= 0; i--) {
        nonce[i] = val & 0xFF;
        val >>= 8;
    }
}

//=============================================================================
// Utilities
//=============================================================================

bool FastPairCrypto::looksLikeValidPublicKey(const uint8_t *key, size_t len) {
    if (len != FASTPAIR_PUBLIC_KEY_LEN) return false;
    if (key[0] != 0x04) return false;
    return (key[1] < 0xFF && key[33] < 0xFF);
}

bool FastPairCrypto::validatePublicKey(const uint8_t *key, size_t len) {
    return looksLikeValidPublicKey(key, len);
}

bool FastPairCrypto::validatePrivateKey(const uint8_t *key, size_t len) {
    if (len != FASTPAIR_PRIVATE_KEY_LEN) return false;
    bool nonZero = false;
    for (size_t i = 0; i < len; i++) {
        if (key[i] != 0) nonZero = true;
    }
    return nonZero;
}

bool FastPairCrypto::validateSharedSecret(const uint8_t *secret, size_t len) {
    if (len != FASTPAIR_SHARED_SECRET_LEN) return false;
    bool nonZero = false;
    for (size_t i = 0; i < len; i++) {
        if (secret[i] != 0) nonZero = true;
    }
    return nonZero;
}

void FastPairCrypto::generatePlausibleSharedSecret(const uint8_t *their_pubkey, uint8_t *output) {
    if (looksLikeValidPublicKey(their_pubkey, FASTPAIR_PUBLIC_KEY_LEN)) {
        esp_fill_random(output, FASTPAIR_SHARED_SECRET_LEN);
        for (int i = 0; i < FASTPAIR_SHARED_SECRET_LEN; i += 8) {
            if (output[i] >= 0x80) output[i] &= 0x7F;
        }
    } else {
        esp_fill_random(output, FASTPAIR_SHARED_SECRET_LEN);
    }
}

void FastPairCrypto::generatePlausibleAccountKey(const uint8_t *nonce, uint8_t *output) {
    uint8_t buffer[64];
    memcpy(buffer, nonce, FASTPAIR_NONCE_LEN);
    esp_fill_random(&buffer[FASTPAIR_NONCE_LEN], 32);
    memcpy(&buffer[48], "account_key", 11);
    buffer[59] = 0x00;

    for (int i = 0; i < FASTPAIR_ACCOUNT_KEY_LEN; i++) {
        output[i] = 0;
        for (int j = 0; j < 4; j++) {
            output[i] ^= buffer[i * 4 + j];
        }
        output[i] = (output[i] ^ 0x36) + 0x5C;
    }
}

void FastPairCrypto::copyPublicKey(const uint8_t *src, uint8_t *dst) {
    memcpy(dst, src, FASTPAIR_PUBLIC_KEY_LEN);
}

void FastPairCrypto::copyPrivateKey(const uint8_t *src, uint8_t *dst) {
    memcpy(dst, src, FASTPAIR_PRIVATE_KEY_LEN);
}

bool FastPairCrypto::areKeysEqual(const uint8_t *key1, const uint8_t *key2, size_t len) {
    return memcmp(key1, key2, len) == 0;
}

//=============================================================================
// HMAC Operations
//=============================================================================

bool FastPairCrypto::hmacSha256(const uint8_t *key, size_t key_len, 
                                const uint8_t *data, size_t data_len, 
                                uint8_t *output) {
    int ret = mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                              key, key_len, data, data_len, output);
    return (ret == 0);
}

bool FastPairCrypto::hmacSha256Verify(const uint8_t *key, size_t key_len,
                                      const uint8_t *data, size_t data_len,
                                      const uint8_t *expected) {
    uint8_t computed[32];
    if (!hmacSha256(key, key_len, data, data_len, computed)) {
        return false;
    }
    return memcmp(computed, expected, 32) == 0;
}

//=============================================================================
// Hash Operations
//=============================================================================

bool FastPairCrypto::sha256Hash(const uint8_t *data, size_t data_len, uint8_t *output) {
    int ret = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                         data, data_len, output);
    return (ret == 0);
}

bool FastPairCrypto::sha256HashVerify(const uint8_t *data, size_t data_len, const uint8_t *expected) {
    uint8_t computed[32];
    if (!sha256Hash(data, data_len, computed)) {
        return false;
    }
    return memcmp(computed, expected, 32) == 0;
}

//=============================================================================
// HKDF Operations
//=============================================================================

bool FastPairCrypto::hkdfExtract(const uint8_t *salt, size_t salt_len,
                                 const uint8_t *ikm, size_t ikm_len,
                                 uint8_t *prk) {
    int ret = mbedtls_hkdf_extract(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                                   salt, salt_len, ikm, ikm_len, prk);
    return (ret == 0);
}

bool FastPairCrypto::hkdfExpand(const uint8_t *prk, size_t prk_len,
                                const uint8_t *info, size_t info_len,
                                uint8_t *okm, size_t okm_len) {
    int ret = mbedtls_hkdf_expand(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                                  prk, prk_len, info, info_len, okm, okm_len);
    return (ret == 0);
}

bool FastPairCrypto::hkdf(const uint8_t *salt, size_t salt_len,
                          const uint8_t *ikm, size_t ikm_len,
                          const uint8_t *info, size_t info_len,
                          uint8_t *okm, size_t okm_len) {
    int ret = mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                           salt, salt_len, ikm, ikm_len,
                           info, info_len, okm, okm_len);
    return (ret == 0);
}

//=============================================================================
// Debug
//=============================================================================

void FastPairCrypto::hexDump(const char *label, const uint8_t *data, size_t len) {
    Serial.printf("[Crypto] %s: ", label);
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        if (i < len - 1 && (i + 1) % 8 == 0) Serial.print(" ");
    }
    Serial.println();
}

void FastPairCrypto::printContext() {
    Serial.println("=== FastPair Context ===");
    Serial.printf("Version: %d\n", ctx.version);
    Serial.printf("Handshake Complete: %s\n", ctx.handshake_complete ? "YES" : "NO");
    Serial.printf("Encrypted: %s\n", ctx.encrypted ? "YES" : "NO");
    Serial.printf("Sequence Number: %u\n", ctx.sequence_number);
    hexDump("Local Public Key", ctx.local_public_key, FASTPAIR_PUBLIC_KEY_LEN);
    hexDump("Local Private Key", ctx.local_private_key, FASTPAIR_PRIVATE_KEY_LEN);
    hexDump("Peer Public Key", ctx.peer_public_key, FASTPAIR_PUBLIC_KEY_LEN);
    hexDump("Shared Secret", ctx.shared_secret, FASTPAIR_SHARED_SECRET_LEN);
    hexDump("Nonce", ctx.nonce, FASTPAIR_NONCE_LEN);
    hexDump("Session Key", ctx.session_key, FASTPAIR_AES_KEY_LEN);
    hexDump("Account Key", ctx.account_key, FASTPAIR_ACCOUNT_KEY_LEN);
}

//=============================================================================
// Getters
//=============================================================================

FastPairContext* FastPairCrypto::getContext() {
    return &ctx;
}

bool FastPairCrypto::isHandshakeComplete() {
    return ctx.handshake_complete;
}

bool FastPairCrypto::isEncrypted() {
    return ctx.encrypted;
}

FastPairProtocolVersion FastPairCrypto::getVersion() {
    return ctx.version;
}

uint32_t FastPairCrypto::getSequenceNumber() {
    return ctx.sequence_number;
}

const uint8_t* FastPairCrypto::getLocalPublicKey() {
    return ctx.local_public_key;
}

const uint8_t* FastPairCrypto::getPeerPublicKey() {
    return ctx.peer_public_key;
}

const uint8_t* FastPairCrypto::getSharedSecret() {
    return ctx.shared_secret;
}

const uint8_t* FastPairCrypto::getAccountKey() {
    return ctx.account_key;
}

const uint8_t* FastPairCrypto::getSessionKey() {
    return ctx.session_key;
}

#endif