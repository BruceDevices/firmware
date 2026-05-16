/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2017 Sartura d.o.o.
 *
 * Author: Juraj Vijtiuk <juraj.vijtiuk@sartura.hr>
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "libssh_esp32_config.h"

#include "libssh/crypto.h"
#include "libssh/wrapper.h"
#include "mbedcrypto-compat.h"

#include <mbedtls/md.h>

SHACTX
sha1_init(void)
{
    SHACTX ctx = NULL;
    int rc;
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);

    if (md_info == NULL) {
        return NULL;
    }

    ctx = malloc(sizeof(mbedtls_md_context_t));
    if (ctx == NULL) {
        return NULL;
    }

    mbedtls_md_init(ctx);

    rc = mbedtls_md_setup(ctx, md_info, 0);
    if (rc != 0) {
        SAFE_FREE(ctx);
        return NULL;
    }

    rc = mbedtls_md_starts(ctx);
    if (rc != 0) {
        SAFE_FREE(ctx);
        return NULL;
    }

    return ctx;
}

void
sha1_ctx_free(SHACTX c)
{
    mbedtls_md_free(c);
    SAFE_FREE(c);
}

int
sha1_update_esp32_port(SHACTX c, const void *data, size_t len)
{
    int rc = mbedtls_md_update(c, data, len);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

int
sha1_final(unsigned char *md, SHACTX c)
{
    int rc = mbedtls_md_finish(c, md);
    sha1_ctx_free(c);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

int
sha1_esp32_port(const unsigned char *digest, size_t len, unsigned char *hash)
{
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    int rc;

    if (md_info == NULL) {
        return SSH_ERROR;
    }
    rc = mbedtls_md(md_info, digest, len, hash);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

SHA256CTX
sha256_init(void)
{
    SHA256CTX ctx = NULL;
    int rc;
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    if (md_info == NULL) {
        return NULL;
    }

    ctx = malloc(sizeof(mbedtls_md_context_t));
    if (ctx == NULL) {
        return NULL;
    }

    mbedtls_md_init(ctx);

    rc = mbedtls_md_setup(ctx, md_info, 0);
    if (rc != 0) {
        SAFE_FREE(ctx);
        return NULL;
    }

    rc = mbedtls_md_starts(ctx);
    if (rc != 0) {
        SAFE_FREE(ctx);
        return NULL;
    }

    return ctx;
}

void
sha256_ctx_free(SHA256CTX c)
{
    mbedtls_md_free(c);
    SAFE_FREE(c);
}

int
sha256_update(SHA256CTX c, const void *data, size_t len)
{
    int rc = mbedtls_md_update(c, data, len);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

int
sha256_final(unsigned char *md, SHA256CTX c)
{
    int rc = mbedtls_md_finish(c, md);
    mbedtls_md_free(c);
    SAFE_FREE(c);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

int
sha256(const unsigned char *digest, size_t len, unsigned char *hash)
{
    int rc;
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        return SSH_ERROR;
    }
    rc = mbedtls_md(md_info, digest, len, hash);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

SHA384CTX
sha384_init(void)
{
    SHA384CTX ctx = NULL;
    int rc;
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA384);

    if (md_info == NULL) {
        return NULL;
    }

    ctx = malloc(sizeof(mbedtls_md_context_t));
    if (ctx == NULL) {
        return NULL;
    }

    mbedtls_md_init(ctx);

    rc = mbedtls_md_setup(ctx, md_info, 0);
    if (rc != 0) {
        SAFE_FREE(ctx);
        return NULL;
    }

    rc = mbedtls_md_starts(ctx);
    if (rc != 0) {
        SAFE_FREE(ctx);
        return NULL;
    }

    return ctx;
}

void
sha384_ctx_free(SHA384CTX c)
{
    mbedtls_md_free(c);
    SAFE_FREE(c);
}

int
sha384_update(SHA384CTX c, const void *data, size_t len)
{
    int rc = mbedtls_md_update(c, data, len);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

int
sha384_final(unsigned char *md, SHA384CTX c)
{
    int rc = mbedtls_md_finish(c, md);
    sha384_ctx_free(c);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

int
sha384(const unsigned char *digest, size_t len, unsigned char *hash)
{
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA384);
    int rc;

    if (md_info == NULL) {
        return SSH_ERROR;
    }
    rc = mbedtls_md(md_info, digest, len, hash);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

SHA512CTX
sha512_init(void)
{
    SHA512CTX ctx = NULL;
    int rc;
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
    if (md_info == NULL) {
        return NULL;
    }

    ctx = malloc(sizeof(mbedtls_md_context_t));
    if (ctx == NULL) {
        return NULL;
    }

    mbedtls_md_init(ctx);

    rc = mbedtls_md_setup(ctx, md_info, 0);
    if (rc != 0) {
        SAFE_FREE(ctx);
        return NULL;
    }

    rc = mbedtls_md_starts(ctx);
    if (rc != 0) {
        SAFE_FREE(ctx);
        return NULL;
    }

    return ctx;
}

void
sha512_ctx_free(SHA512CTX c)
{
    mbedtls_md_free(c);
    SAFE_FREE(c);
}

int
sha512_update(SHA512CTX c, const void *data, size_t len)
{
    int rc = mbedtls_md_update(c, data, len);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

int
sha512_final(unsigned char *md, SHA512CTX c)
{
    int rc = mbedtls_md_finish(c, md);
    sha512_ctx_free(c);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

int
sha512(const unsigned char *digest, size_t len, unsigned char *hash)
{
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
    int rc;

    if (md_info == NULL) {
        return SSH_ERROR;
    }
    rc = mbedtls_md(md_info, digest, len, hash);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

MD5CTX
md5_init(void)
{
    MD5CTX ctx = NULL;
    int rc;
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
    if (md_info == NULL) {
        return NULL;
    }

    ctx = malloc(sizeof(mbedtls_md_context_t));
    if (ctx == NULL) {
        return NULL;
    }

    mbedtls_md_init(ctx);

    rc = mbedtls_md_setup(ctx, md_info, 0);
    if (rc != 0) {
        SAFE_FREE(ctx);
        return NULL;
    }

    rc = mbedtls_md_starts(ctx);
    if (rc != 0) {
        SAFE_FREE(ctx);
        return NULL;
    }

    return ctx;
}

void
md5_ctx_free(MD5CTX c)
{
    mbedtls_md_free(c);
    SAFE_FREE(c);
}

int
md5_update_esp32_port(MD5CTX c, const void *data, size_t len)
{
    int rc = mbedtls_md_update(c, data, len);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}

int
md5_final(unsigned char *md, MD5CTX c)
{
    int rc = mbedtls_md_finish(c, md);
    mbedtls_md_free(c);
    SAFE_FREE(c);
    if (rc != 0) {
        return SSH_ERROR;
    }
    return SSH_OK;
}
