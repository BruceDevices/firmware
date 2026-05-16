/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 by Aris Adamantiadis
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef WRAPPER_H_
#define WRAPPER_H_

#include <stdbool.h>

#include "libssh_esp32_config.h"
#include "libssh/libssh.h"
#include "libssh/libcrypto.h"
#include "libssh/libgcrypt.h"
#include "libssh/libmbedcrypto.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ssh_kdf_digest {
    SSH_KDF_SHA1=1,
    SSH_KDF_SHA256,
    SSH_KDF_SHA384,
    SSH_KDF_SHA512
};

enum ssh_hmac_e {
  SSH_HMAC_SHA1 = 1,
  SSH_HMAC_SHA256,
  SSH_HMAC_SHA512,
  SSH_HMAC_MD5,
  SSH_HMAC_AEAD_POLY1305,
  SSH_HMAC_AEAD_GCM,
  SSH_HMAC_NONE,
};

enum ssh_des_e {
  SSH_3DES,
  SSH_DES
};

struct ssh_hmac_struct {
  const char* name;
  enum ssh_hmac_e hmac_type;
  bool etm;
};

enum ssh_crypto_direction_e {
    SSH_DIRECTION_IN = 1,
    SSH_DIRECTION_OUT = 2,
    SSH_DIRECTION_BOTH = 3,
};

struct ssh_cipher_struct;
struct ssh_crypto_struct;

typedef struct ssh_mac_ctx_struct *ssh_mac_ctx;
MD5CTX md5_init(void);
void md5_ctx_free(MD5CTX);
int md5_update_esp32_port(MD5CTX c, const void *data, size_t len);
int md5_final(unsigned char *md, MD5CTX c);

SHACTX sha1_init(void);
void sha1_ctx_free(SHACTX);
int sha1_update_esp32_port(SHACTX c, const void *data, size_t len);
int sha1_final(unsigned char *md,SHACTX c);
int sha1_esp32_port(const unsigned char *digest,size_t len, unsigned char *hash);

SHA256CTX sha256_init(void);
void sha256_ctx_free(SHA256CTX);
int sha256_update(SHA256CTX c, const void *data, size_t len);
int sha256_final(unsigned char *md,SHA256CTX c);
int sha256(const unsigned char *digest, size_t len, unsigned char *hash);

SHA384CTX sha384_init(void);
void sha384_ctx_free(SHA384CTX);
int sha384_update(SHA384CTX c, const void *data, size_t len);
int sha384_final(unsigned char *md,SHA384CTX c);
int sha384(const unsigned char *digest, size_t len, unsigned char *hash);

SHA512CTX sha512_init(void);
void sha512_ctx_free(SHA512CTX);
int sha512_update(SHA512CTX c, const void *data, size_t len);
int sha512_final(unsigned char *md,SHA512CTX c);
int sha512(const unsigned char *digest, size_t len, unsigned char *hash);

HMACCTX hmac_init(const void *key,size_t len, enum ssh_hmac_e type);
int hmac_update(HMACCTX c, const void *data, size_t len);
int hmac_final(HMACCTX ctx, unsigned char *hashmacbuf, size_t *len);
size_t hmac_digest_len(enum ssh_hmac_e type);

int ssh_kdf(struct ssh_crypto_struct *crypto,
            unsigned char *key, size_t key_len,
            uint8_t key_type, unsigned char *output,
            size_t requested_len);

int crypt_set_algorithms_client(ssh_session session);
int crypt_set_algorithms_server(ssh_session session);
struct ssh_crypto_struct *crypto_new(void);
void crypto_free(struct ssh_crypto_struct *crypto);

void ssh_reseed(void);
int ssh_crypto_init(void);
void ssh_crypto_finalize(void);

void ssh_cipher_clear(struct ssh_cipher_struct *cipher);
struct ssh_hmac_struct *ssh_get_hmactab(void);
struct ssh_cipher_struct *ssh_get_ciphertab(void);
const char *ssh_hmac_type_to_string(enum ssh_hmac_e hmac_type, bool etm);

#if defined(HAVE_LIBCRYPTO) && OPENSSL_VERSION_NUMBER >= 0x30000000L
int evp_build_pkey(const char* name, OSSL_PARAM_BLD *param_bld, EVP_PKEY **pkey, int selection);
int evp_dup_dsa_pkey(const ssh_key key, ssh_key new_key, int demote);
int evp_dup_rsa_pkey(const ssh_key key, ssh_key new_key, int demote);
int evp_dup_ecdsa_pkey(const ssh_key key, ssh_key new_key, int demote);
#endif /* HAVE_LIBCRYPTO && OPENSSL_VERSION_NUMBER */

#ifdef __cplusplus
}
#endif

#endif /* WRAPPER_H_ */
