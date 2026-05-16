/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 by Aris Adamantiadis
 * Copyrihgt (c) 2018 Red Hat, Inc.
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

#include "libssh_esp32_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "libssh/priv.h"
#include "libssh/crypto.h"
#include "libssh/buffer.h"
#include "libssh/session.h"
#include "libssh/misc.h"
#include "libssh/dh.h"
#include "libssh/ssh2.h"
#include "libssh/pki.h"
#include "libssh/bignum.h"

#include "libssh/string.h"


/* The following implements the SSHKDF for crypto backend that
 * do not have a native implementation */
struct ssh_mac_ctx_struct {
    enum ssh_kdf_digest digest_type;
    union {
        SHACTX sha1_ctx;
        SHA256CTX sha256_ctx;
        SHA384CTX sha384_ctx;
        SHA512CTX sha512_ctx;
    } ctx;
};

static ssh_mac_ctx ssh_mac_ctx_init(enum ssh_kdf_digest type)
{
    ssh_mac_ctx ctx = malloc(sizeof(struct ssh_mac_ctx_struct));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->digest_type = type;
    switch (type) {
    case SSH_KDF_SHA1:
        ctx->ctx.sha1_ctx = sha1_init();
        if (ctx->ctx.sha1_ctx == NULL) {
            goto err;
        }
        return ctx;
    case SSH_KDF_SHA256:
        ctx->ctx.sha256_ctx = sha256_init();
        if (ctx->ctx.sha256_ctx == NULL) {
            goto err;
        }
        return ctx;
    case SSH_KDF_SHA384:
        ctx->ctx.sha384_ctx = sha384_init();
        if (ctx->ctx.sha384_ctx == NULL) {
            goto err;
        }
        return ctx;
    case SSH_KDF_SHA512:
        ctx->ctx.sha512_ctx = sha512_init();
        if (ctx->ctx.sha512_ctx == NULL) {
            goto err;
        }
        return ctx;
    }
err:
    SAFE_FREE(ctx);
    return NULL;
}

static void ssh_mac_ctx_free(ssh_mac_ctx ctx)
{
    if (ctx == NULL) {
        return;
    }

    switch (ctx->digest_type) {
    case SSH_KDF_SHA1:
        sha1_ctx_free(ctx->ctx.sha1_ctx);
        break;
    case SSH_KDF_SHA256:
        sha256_ctx_free(ctx->ctx.sha256_ctx);
        break;
    case SSH_KDF_SHA384:
        sha384_ctx_free(ctx->ctx.sha384_ctx);
        break;
    case SSH_KDF_SHA512:
        sha512_ctx_free(ctx->ctx.sha512_ctx);
        break;
    }
    SAFE_FREE(ctx);
}

static int ssh_mac_update(ssh_mac_ctx ctx, const void *data, size_t len)
{
    switch (ctx->digest_type) {
    case SSH_KDF_SHA1:
        return sha1_update_esp32_port(ctx->ctx.sha1_ctx, data, len);
    case SSH_KDF_SHA256:
        return sha256_update(ctx->ctx.sha256_ctx, data, len);
    case SSH_KDF_SHA384:
        return sha384_update(ctx->ctx.sha384_ctx, data, len);
    case SSH_KDF_SHA512:
        return sha512_update(ctx->ctx.sha512_ctx, data, len);
    }
    return SSH_ERROR;
}

static int ssh_mac_final(unsigned char *md, ssh_mac_ctx ctx)
{
    int rc = SSH_ERROR;

    switch (ctx->digest_type) {
    case SSH_KDF_SHA1:
        rc = sha1_final(md, ctx->ctx.sha1_ctx);
        break;
    case SSH_KDF_SHA256:
        rc = sha256_final(md, ctx->ctx.sha256_ctx);
        break;
    case SSH_KDF_SHA384:
        rc = sha384_final(md, ctx->ctx.sha384_ctx);
        break;
    case SSH_KDF_SHA512:
        rc = sha512_final(md, ctx->ctx.sha512_ctx);
        break;
    }
    SAFE_FREE(ctx);
    return rc;
}

int sshkdf_derive_key(struct ssh_crypto_struct *crypto,
                      unsigned char *key,
                      size_t key_len,
                      uint8_t key_type,
                      unsigned char *output,
                      size_t requested_len)
{
    /* Can't use VLAs with Visual Studio, so allocate the biggest
     * digest buffer we can possibly need */
    unsigned char digest[DIGEST_MAX_LEN];
    size_t output_len = crypto->digest_len;
    ssh_mac_ctx ctx;
    int rc;

    if (DIGEST_MAX_LEN < crypto->digest_len) {
        return -1;
    }

    ctx = ssh_mac_ctx_init(crypto->digest_type);
    if (ctx == NULL) {
        return -1;
    }

    rc = ssh_mac_update(ctx, key, key_len);
    if (rc != SSH_OK) {
        ssh_mac_ctx_free(ctx);
        return -1;
    }
    rc = ssh_mac_update(ctx, crypto->secret_hash, crypto->digest_len);
    if (rc != SSH_OK) {
        ssh_mac_ctx_free(ctx);
        return -1;
    }
    rc = ssh_mac_update(ctx, &key_type, 1);
    if (rc != SSH_OK) {
        ssh_mac_ctx_free(ctx);
        return -1;
    }
    rc = ssh_mac_update(ctx, crypto->session_id, crypto->session_id_len);
    if (rc != SSH_OK) {
        ssh_mac_ctx_free(ctx);
        return -1;
    }
    rc = ssh_mac_final(digest, ctx);
    if (rc != SSH_OK) {
        return -1;
    }

    if (requested_len < output_len) {
        output_len = requested_len;
    }
    memcpy(output, digest, output_len);

    while (requested_len > output_len) {
        ctx = ssh_mac_ctx_init(crypto->digest_type);
        if (ctx == NULL) {
            return -1;
        }
        rc = ssh_mac_update(ctx, key, key_len);
        if (rc != SSH_OK) {
            ssh_mac_ctx_free(ctx);
            return -1;
        }
        rc = ssh_mac_update(ctx, crypto->secret_hash, crypto->digest_len);
        if (rc != SSH_OK) {
            ssh_mac_ctx_free(ctx);
            return -1;
        }
        rc = ssh_mac_update(ctx, output, output_len);
        if (rc != SSH_OK) {
            ssh_mac_ctx_free(ctx);
            return -1;
        }
        rc = ssh_mac_final(digest, ctx);
        if (rc != SSH_OK) {
            return -1;
        }
        if (requested_len < output_len + crypto->digest_len) {
            memcpy(output + output_len, digest, requested_len - output_len);
        } else {
            memcpy(output + output_len, digest, crypto->digest_len);
        }
        output_len += crypto->digest_len;
    }

    return 0;
}
