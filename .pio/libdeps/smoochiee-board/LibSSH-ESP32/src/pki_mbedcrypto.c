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

#ifdef HAVE_LIBMBEDCRYPTO
#include <mbedtls/pk.h>
#include <mbedtls/error.h>
#include "mbedcrypto-compat.h"

#include "libssh/priv.h"
#include "libssh/pki.h"
#include "libssh/pki_priv.h"
#include "libssh/buffer.h"
#include "libssh/bignum.h"
#include "libssh/misc.h"

#define MAX_PASSPHRASE_SIZE 1024
#define MAX_KEY_SIZE 32

void pki_key_clean(ssh_key key)
{
    if (key == NULL)
        return;

    if (key->pk != NULL) {
        mbedtls_pk_free(key->pk);
        SAFE_FREE(key->pk);
    }

    if (key->ecdsa != NULL) {
        mbedtls_ecdsa_free(key->ecdsa);
        SAFE_FREE(key->ecdsa);
    }
}

ssh_string pki_private_key_to_pem(const ssh_key key, const char *passphrase,
        ssh_auth_callback auth_fn, void *auth_data)
{
    (void) key;
    (void) passphrase;
    (void) auth_fn;
    (void) auth_data; return NULL;
}

static int pki_key_ecdsa_to_nid(mbedtls_ecdsa_context *ecdsa)
{
    mbedtls_ecp_group_id id;

    id = ecdsa->MBEDTLS_PRIVATE(grp.id);
    if (id == MBEDTLS_ECP_DP_SECP256R1) {
        return NID_mbedtls_nistp256;
    } else if (id == MBEDTLS_ECP_DP_SECP384R1) {
        return NID_mbedtls_nistp384;
    } else if (id == MBEDTLS_ECP_DP_SECP521R1) {
        return NID_mbedtls_nistp521;
    }

    return -1;
}

static enum ssh_keytypes_e pki_key_ecdsa_to_key_type(mbedtls_ecdsa_context *ecdsa)
{
    int nid;

    nid = pki_key_ecdsa_to_nid(ecdsa);

    switch (nid) {
        case NID_mbedtls_nistp256:
            return SSH_KEYTYPE_ECDSA_P256;
        case NID_mbedtls_nistp384:
            return SSH_KEYTYPE_ECDSA_P384;
        case NID_mbedtls_nistp521:
            return SSH_KEYTYPE_ECDSA_P521;
        default:
            return SSH_KEYTYPE_UNKNOWN;
    }
}

ssh_key pki_private_key_from_base64(const char *b64_key, const char *passphrase,
        ssh_auth_callback auth_fn, void *auth_data)
{
    ssh_key key = NULL;
    mbedtls_pk_context *pk = NULL;
    mbedtls_pk_type_t mbed_type;
    int valid;
    /* mbedtls pk_parse_key expects strlen to count the 0 byte */
    size_t b64len = strlen(b64_key) + 1;
    unsigned char tmp[MAX_PASSPHRASE_SIZE] = {0};
#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_ctr_drbg_context *ctr_drbg = ssh_get_mbedtls_ctr_drbg_context();
#endif

    pk = malloc(sizeof(mbedtls_pk_context));
    if (pk == NULL) {
        goto fail;
    }
    mbedtls_pk_init(pk);

    if (passphrase == NULL) {
        if (auth_fn) {
            valid = auth_fn("Passphrase for private key:",
                            (char *)tmp,
                            MAX_PASSPHRASE_SIZE,
                            0,
                            0,
                            auth_data);
            if (valid < 0) {
                goto fail;
            }
            valid = mbedtls_pk_parse_key(
                pk,
                (const unsigned char *)b64_key,
                b64len,
                tmp,
                strnlen((const char *)tmp, MAX_PASSPHRASE_SIZE)
#if MBEDTLS_VERSION_MAJOR > 2
                    ,
                mbedtls_ctr_drbg_random,
                ctr_drbg
#endif
            );
        } else {
            valid = mbedtls_pk_parse_key(pk,
                                         (const unsigned char *)b64_key,
                                         b64len,
                                         NULL,
                                         0
#if MBEDTLS_VERSION_MAJOR > 2
                                         ,
                                         mbedtls_ctr_drbg_random,
                                         ctr_drbg
#endif
            );
        }
    } else {
        valid = mbedtls_pk_parse_key(pk,
                                     (const unsigned char *)b64_key,
                                     b64len,
                                     (const unsigned char *)passphrase,
                                     strnlen(passphrase, MAX_PASSPHRASE_SIZE)
#if MBEDTLS_VERSION_MAJOR > 2
                                         ,
                                     mbedtls_ctr_drbg_random,
                                     ctr_drbg
#endif
        );
    }
    if (valid != 0) {
        char error_buf[100];
        mbedtls_strerror(valid, error_buf, 100);
        SSH_LOG(SSH_LOG_WARN, "Parsing private key %s", error_buf);
        goto fail;
    }

    mbed_type = mbedtls_pk_get_type(pk);

    key = ssh_key_new();
    if (key == NULL) {
        goto fail;
    }

    switch (mbed_type) {
    case MBEDTLS_PK_RSA:
    case MBEDTLS_PK_RSA_ALT:
        key->pk = pk;
        pk = NULL;
        key->type = SSH_KEYTYPE_RSA;
        break;
    case MBEDTLS_PK_ECKEY:
    case MBEDTLS_PK_ECDSA: {
        /* type will be set later */
        mbedtls_ecp_keypair *keypair = mbedtls_pk_ec(*pk);

        key->ecdsa = malloc(sizeof(mbedtls_ecdsa_context));
        if (key->ecdsa == NULL) {
            goto fail;
        }

        mbedtls_ecdsa_init(key->ecdsa);
        mbedtls_ecdsa_from_keypair(key->ecdsa, keypair);
        key->pk = pk;

        key->ecdsa_nid = pki_key_ecdsa_to_nid(key->ecdsa);

        /* pki_privatekey_type_from_string always returns P256 for ECDSA
         * keys, so we need to figure out the correct type here */
        key->type = pki_key_ecdsa_to_key_type(key->ecdsa);
        if (key->type == SSH_KEYTYPE_UNKNOWN) {
            SSH_LOG(SSH_LOG_TRACE, "Invalid private key.");
            goto fail;
        }
        break;
    }
    default:
        SSH_LOG(SSH_LOG_WARN,
                "Unknown or invalid private key type %d",
                mbed_type);
        return NULL;
    }

    key->type_c = ssh_key_type_to_char(key->type);
    key->flags = SSH_KEY_FLAG_PRIVATE | SSH_KEY_FLAG_PUBLIC;

    return key;
fail:
    ssh_key_free(key);
    if (pk != NULL) {
        mbedtls_pk_free(pk);
        SAFE_FREE(pk);
    }
    return NULL;
}

int pki_privkey_build_rsa(ssh_key key,
                          ssh_string n,
                          ssh_string e,
                          ssh_string d,
                          UNUSED_PARAM(ssh_string iqmp),
                          ssh_string p,
                          ssh_string q)
{
    mbedtls_rsa_context *rsa = NULL;
    const mbedtls_pk_info_t *pk_info = NULL;
    int rc;

    key->pk = malloc(sizeof(mbedtls_pk_context));
    if (key->pk == NULL) {
        return SSH_ERROR;
    }

    mbedtls_pk_init(key->pk);
    pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA);
    mbedtls_pk_setup(key->pk, pk_info);

    rc = mbedtls_pk_can_do(key->pk, MBEDTLS_PK_RSA);
    if (rc == 0) {
        goto fail;
    }

    rsa = mbedtls_pk_rsa(*key->pk);
    rc = mbedtls_rsa_import_raw(rsa,
                                ssh_string_data(n), ssh_string_len(n),
                                ssh_string_data(p), ssh_string_len(p),
                                ssh_string_data(q), ssh_string_len(q),
                                ssh_string_data(d), ssh_string_len(d),
                                ssh_string_data(e), ssh_string_len(e));
    if (rc != 0) {
        SSH_LOG(SSH_LOG_TRACE, "Failed to import private RSA key");
        goto fail;
    }

    rc = mbedtls_rsa_complete(rsa);
    if (rc != 0) {
        SSH_LOG(SSH_LOG_TRACE, "Failed to complete private RSA key");
        goto fail;
    }

    rc = mbedtls_rsa_check_privkey(rsa);
    if (rc != 0) {
        SSH_LOG(SSH_LOG_TRACE, "Inconsistent private RSA key");
        goto fail;
    }

    return SSH_OK;

fail:
    mbedtls_pk_free(key->pk);
    SAFE_FREE(key->pk);
    return SSH_ERROR;
}

int pki_pubkey_build_rsa(ssh_key key, ssh_string e, ssh_string n)
{
    mbedtls_rsa_context *rsa = NULL;
    const mbedtls_pk_info_t *pk_info = NULL;
#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_mpi N;
    mbedtls_mpi E;
#endif
    int rc;

    key->pk = malloc(sizeof(mbedtls_pk_context));
    if (key->pk == NULL) {
        return SSH_ERROR;
    }

    mbedtls_pk_init(key->pk);
    pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA);
    mbedtls_pk_setup(key->pk, pk_info);

    rc = mbedtls_pk_can_do(key->pk, MBEDTLS_PK_RSA);
    if (rc == 0) {
        goto fail;
    }

    rsa = mbedtls_pk_rsa(*key->pk);
#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&E);

    rc = mbedtls_mpi_read_binary(&N, ssh_string_data(n),
                                 ssh_string_len(n));
#else
    rc = mbedtls_mpi_read_binary(&rsa->N, ssh_string_data(n),
                                 ssh_string_len(n));
#endif
    if (rc != 0) {
        goto fail;
    }
#if MBEDTLS_VERSION_MAJOR > 2
    rc = mbedtls_mpi_read_binary(&E, ssh_string_data(e),
                                 ssh_string_len(e));
#else
    rc = mbedtls_mpi_read_binary(&rsa->E, ssh_string_data(e),
                                 ssh_string_len(e));
#endif
    if (rc != 0) {
        goto fail;
    }

#if MBEDTLS_VERSION_MAJOR > 2
    rc = mbedtls_rsa_import(rsa, &N, NULL, NULL, NULL, &E);
    if (rc != 0) {
        goto fail;
    }

    rc = mbedtls_rsa_complete(rsa);
    if (rc != 0) {
        goto fail;
    }

#else
    rsa->len = (mbedtls_mpi_bitlen(&rsa->N) + 7) >> 3;
#endif
    rc = SSH_OK;
    goto exit;
fail:
    rc = SSH_ERROR;
    mbedtls_pk_free(key->pk);
    SAFE_FREE(key->pk);
exit:
#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_mpi_free(&N);
    mbedtls_mpi_free(&E);
#endif
    return rc;
}

ssh_key pki_key_dup(const ssh_key key, int demote)
{
    ssh_key new = NULL;
    int rc;
    const mbedtls_pk_info_t *pk_info = NULL;
#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_mpi N;
    mbedtls_mpi E;
    mbedtls_mpi D;
    mbedtls_mpi P;
    mbedtls_mpi Q;
#endif

    new = ssh_key_new();
    if (new == NULL) {
        return NULL;
    }

    new->type = key->type;
    new->type_c = key->type_c;
    if (demote) {
        new->flags = SSH_KEY_FLAG_PUBLIC;
    } else {
        new->flags = key->flags;
    }

#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&E);
    mbedtls_mpi_init(&D);
    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
#endif

    switch(key->type) {
        case SSH_KEYTYPE_RSA: {
            mbedtls_rsa_context *rsa, *new_rsa;

            new->pk = malloc(sizeof(mbedtls_pk_context));
            if (new->pk == NULL) {
                goto fail;
            }

            mbedtls_pk_init(new->pk);
            pk_info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA);
            mbedtls_pk_setup(new->pk, pk_info);

            if (!mbedtls_pk_can_do(key->pk, MBEDTLS_PK_RSA) ||
                !mbedtls_pk_can_do(new->pk, MBEDTLS_PK_RSA)) {
                goto fail;
            }

            rsa = mbedtls_pk_rsa(*key->pk);
            new_rsa = mbedtls_pk_rsa(*new->pk);

            if (!demote && (key->flags & SSH_KEY_FLAG_PRIVATE)) {
#if MBEDTLS_VERSION_MAJOR > 2
                rc = mbedtls_rsa_export(rsa, &N, &P, &Q, &D, &E);
                if (rc != 0) {
                    goto fail;
                }
                rc = mbedtls_rsa_import(new_rsa, &N, &P, &Q, &D, &E);
                if (rc != 0) {
                    goto fail;
                }
#else
                rc = mbedtls_mpi_copy(&new_rsa->N, &rsa->N);
                if (rc != 0) {
                    goto fail;
                }

                rc = mbedtls_mpi_copy(&new_rsa->E, &rsa->E);
                if (rc != 0) {
                    goto fail;
                }

                new_rsa->len = (mbedtls_mpi_bitlen(&new_rsa->N) + 7) >> 3;

                rc = mbedtls_mpi_copy(&new_rsa->D, &rsa->D);
                if (rc != 0) {
                    goto fail;
                }

                rc = mbedtls_mpi_copy(&new_rsa->P, &rsa->P);
                if (rc != 0) {
                    goto fail;
                }

                rc = mbedtls_mpi_copy(&new_rsa->Q, &rsa->Q);
                if (rc != 0) {
                    goto fail;
                }

                rc = mbedtls_mpi_copy(&new_rsa->DP, &rsa->DP);
                if (rc != 0) {
                    goto fail;
                }

                rc = mbedtls_mpi_copy(&new_rsa->DQ, &rsa->DQ);
                if (rc != 0) {
                    goto fail;
                }

                rc = mbedtls_mpi_copy(&new_rsa->QP, &rsa->QP);
                if (rc != 0) {
                    goto fail;
                }
#endif
            } else {
#if MBEDTLS_VERSION_MAJOR > 2
                rc = mbedtls_rsa_export(rsa, &N, NULL, NULL, NULL, &E);
                if (rc != 0) {
                    goto fail;
                }
                rc = mbedtls_rsa_import(new_rsa, &N, NULL, NULL, NULL, &E);
                if (rc != 0) {
                    goto fail;
                }
#else
                rc = mbedtls_mpi_copy(&new_rsa->N, &rsa->N);
                if (rc != 0) {
                    goto fail;
                }

                rc = mbedtls_mpi_copy(&new_rsa->E, &rsa->E);
                if (rc != 0) {
                    goto fail;
                }

                new_rsa->len = (mbedtls_mpi_bitlen(&new_rsa->N) + 7) >> 3;
#endif
            }

#if MBEDTLS_VERSION_MAJOR > 2
            rc = mbedtls_rsa_complete(new_rsa);
            if (rc != 0) {
                goto fail;
            }
#endif

            break;
        }
        case SSH_KEYTYPE_ECDSA_P256:
        case SSH_KEYTYPE_ECDSA_P384:
        case SSH_KEYTYPE_ECDSA_P521:
            new->ecdsa_nid = key->ecdsa_nid;

            new->ecdsa = malloc(sizeof(mbedtls_ecdsa_context));

            if (new->ecdsa == NULL) {
                goto fail;
            }

            mbedtls_ecdsa_init(new->ecdsa);

            if (demote && ssh_key_is_private(key)) {
                rc = mbedtls_ecp_copy(&new->ecdsa->MBEDTLS_PRIVATE(Q),
                                &key->ecdsa->MBEDTLS_PRIVATE(Q));
                if (rc != 0) {
                    goto fail;
                }

                rc = mbedtls_ecp_group_copy(&new->ecdsa->MBEDTLS_PRIVATE(grp),
                                &key->ecdsa->MBEDTLS_PRIVATE(grp));
                if (rc != 0) {
                    goto fail;
                }
            } else {
                mbedtls_ecdsa_from_keypair(new->ecdsa, key->ecdsa);
            }

            break;
        case SSH_KEYTYPE_ED25519:
            rc = pki_ed25519_key_dup(new, key);
            if (rc != SSH_OK) {
                goto fail;
            }
            break;
        default:
            goto fail;
    }

    goto cleanup;

fail:
    SSH_KEY_FREE(new);
cleanup:
#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_mpi_free(&N);
    mbedtls_mpi_free(&E);
    mbedtls_mpi_free(&D);
    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&Q);
#endif
    return new;
}

int pki_key_generate_rsa(ssh_key key, int parameter)
{
    int rc;
    const mbedtls_pk_info_t *info = NULL;

    key->pk = malloc(sizeof(mbedtls_pk_context));
    if (key->pk == NULL) {
        return SSH_ERROR;
    }

    mbedtls_pk_init(key->pk);

    info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA);
    rc = mbedtls_pk_setup(key->pk, info);
    if (rc != 0) {
        return SSH_ERROR;
    }

    if (mbedtls_pk_can_do(key->pk, MBEDTLS_PK_RSA)) {
        rc = mbedtls_rsa_gen_key(mbedtls_pk_rsa(*key->pk),
                                 mbedtls_ctr_drbg_random,
                                 ssh_get_mbedtls_ctr_drbg_context(),
                                 parameter,
                                 65537);
        if (rc != 0) {
            mbedtls_pk_free(key->pk);
            return SSH_ERROR;
        }
    }

    return SSH_OK;
}

int pki_key_compare(const ssh_key k1, const ssh_key k2, enum ssh_keycmp_e what)
{
    int rc = 0;
#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_mpi N1;
    mbedtls_mpi N2;
    mbedtls_mpi P1;
    mbedtls_mpi P2;
    mbedtls_mpi Q1;
    mbedtls_mpi Q2;
    mbedtls_mpi E1;
    mbedtls_mpi E2;

    mbedtls_mpi_init(&N1);
    mbedtls_mpi_init(&N2);
    mbedtls_mpi_init(&P1);
    mbedtls_mpi_init(&P2);
    mbedtls_mpi_init(&Q1);
    mbedtls_mpi_init(&Q2);
    mbedtls_mpi_init(&E1);
    mbedtls_mpi_init(&E2);
#endif

    switch (ssh_key_type_plain(k1->type)) {
        case SSH_KEYTYPE_RSA: {
            mbedtls_rsa_context *rsa1, *rsa2;
            if (!mbedtls_pk_can_do(k1->pk, MBEDTLS_PK_RSA) ||
                !mbedtls_pk_can_do(k2->pk, MBEDTLS_PK_RSA)) {
                break;
            }

            if (mbedtls_pk_get_type(k1->pk) != mbedtls_pk_get_type(k2->pk) ||
                mbedtls_pk_get_bitlen(k1->pk) !=
                    mbedtls_pk_get_bitlen(k2->pk)) {
                rc = 1;
                goto cleanup;
            }

            if (what == SSH_KEY_CMP_PUBLIC) {
#if MBEDTLS_VERSION_MAJOR > 2
                rsa1 = mbedtls_pk_rsa(*k1->pk);
                rc = mbedtls_rsa_export(rsa1, &N1, NULL, NULL, NULL, &E1);
                if (rc != 0) {
                    rc = 1;
                    goto cleanup;
                }

                rsa2 = mbedtls_pk_rsa(*k2->pk);
                rc = mbedtls_rsa_export(rsa2, &N2, NULL, NULL, NULL, &E2);
                if (rc != 0) {
                    rc = 1;
                    goto cleanup;
                }

                if (mbedtls_mpi_cmp_mpi(&N1, &N2) != 0) {
                    rc = 1;
                    goto cleanup;
                }

                if (mbedtls_mpi_cmp_mpi(&E1, &E2) != 0) {
                    rc = 1;
                    goto cleanup;
                }
#else
                rsa1 = mbedtls_pk_rsa(*k1->pk);
                rsa2 = mbedtls_pk_rsa(*k2->pk);
                if (mbedtls_mpi_cmp_mpi(&rsa1->N, &rsa2->N) != 0) {
                    rc = 1;
                    goto cleanup;
                }

                if (mbedtls_mpi_cmp_mpi(&rsa1->E, &rsa2->E) != 0) {
                    rc = 1;
                    goto cleanup;
                }
#endif
            } else if (what == SSH_KEY_CMP_PRIVATE) {
#if MBEDTLS_VERSION_MAJOR > 2
                rsa1 = mbedtls_pk_rsa(*k1->pk);
                rc = mbedtls_rsa_export(rsa1, &N1, &P1, &Q1, NULL, &E1);
                if (rc != 0) {
                    rc = 1;
                    goto cleanup;
                }

                rsa2 = mbedtls_pk_rsa(*k2->pk);
                rc = mbedtls_rsa_export(rsa2, &N2, &P2, &Q2, NULL, &E2);
                if (rc != 0) {
                    rc = 1;
                    goto cleanup;
                }

                if (mbedtls_mpi_cmp_mpi(&N1, &N2) != 0) {
                    rc = 1;
                    goto cleanup;
                }

                if (mbedtls_mpi_cmp_mpi(&E1, &E2) != 0) {
                    rc = 1;
                    goto cleanup;
                }

                if (mbedtls_mpi_cmp_mpi(&P1, &P2) != 0) {
                    rc = 1;
                    goto cleanup;
                }

                if (mbedtls_mpi_cmp_mpi(&Q1, &Q2) != 0) {
                    rc = 1;
                    goto cleanup;
                }
#else
                rsa1 = mbedtls_pk_rsa(*k1->pk);
                rsa2 = mbedtls_pk_rsa(*k2->pk);
                if (mbedtls_mpi_cmp_mpi(&rsa1->N, &rsa2->N) != 0) {
                    rc = 1;
                    goto cleanup;
                }

                if (mbedtls_mpi_cmp_mpi(&rsa1->E, &rsa2->E) != 0) {
                    rc = 1;
                    goto cleanup;
                }

                if (mbedtls_mpi_cmp_mpi(&rsa1->P, &rsa2->P) != 0) {
                    rc = 1;
                    goto cleanup;
                }

                if (mbedtls_mpi_cmp_mpi(&rsa1->Q, &rsa2->Q) != 0) {
                    rc = 1;
                    goto cleanup;
                }
#endif
            }
            break;
        }
        case SSH_KEYTYPE_ECDSA_P256:
        case SSH_KEYTYPE_ECDSA_P384:
        case SSH_KEYTYPE_ECDSA_P521:
        case SSH_KEYTYPE_SK_ECDSA: {
            mbedtls_ecp_keypair *ecdsa1 = k1->ecdsa;
            mbedtls_ecp_keypair *ecdsa2 = k2->ecdsa;

            if (ecdsa1->MBEDTLS_PRIVATE(grp).id !=
                ecdsa2->MBEDTLS_PRIVATE(grp).id) {
                rc = 1;
                goto cleanup;
            }

            if (mbedtls_mpi_cmp_mpi(&ecdsa1->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X),
                &ecdsa2->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(X)))
            {
                rc = 1;
                goto cleanup;
            }

            if (mbedtls_mpi_cmp_mpi(&ecdsa1->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Y),
                &ecdsa2->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Y)))
            {
                rc = 1;
                goto cleanup;
            }

            if (mbedtls_mpi_cmp_mpi(&ecdsa1->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Z),
                &ecdsa2->MBEDTLS_PRIVATE(Q).MBEDTLS_PRIVATE(Z)))
            {
                rc = 1;
                goto cleanup;
            }

            if (what == SSH_KEY_CMP_PRIVATE) {
                if (mbedtls_mpi_cmp_mpi(&ecdsa1->MBEDTLS_PRIVATE(d),
                    &ecdsa2->MBEDTLS_PRIVATE(d)))
                {
                    rc = 1;
                    goto cleanup;
                }
            }

            break;
        }
        case SSH_KEYTYPE_ED25519:
        case SSH_KEYTYPE_SK_ED25519:
            /* ed25519 keys handled globally */
            rc = 1;
            break;
        default:
            rc = 1;
	    break;
    }

cleanup:
#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_mpi_free(&N1);
    mbedtls_mpi_free(&N2);
    mbedtls_mpi_free(&P1);
    mbedtls_mpi_free(&P2);
    mbedtls_mpi_free(&Q1);
    mbedtls_mpi_free(&Q2);
    mbedtls_mpi_free(&E1);
    mbedtls_mpi_free(&E2);
#endif
    return rc;
}

ssh_string make_ecpoint_string(const mbedtls_ecp_group *g, const
        mbedtls_ecp_point *p)
{
    ssh_string s = NULL;
    size_t len = 1;
    int rc;

    s = ssh_string_new(len);
    if (s == NULL) {
        return NULL;
    }

    rc = mbedtls_ecp_point_write_binary(g, p, MBEDTLS_ECP_PF_UNCOMPRESSED,
                &len, ssh_string_data(s), ssh_string_len(s));
    if (rc == MBEDTLS_ERR_ECP_BUFFER_TOO_SMALL) {
        SSH_STRING_FREE(s);

        s = ssh_string_new(len);
        if (s == NULL) {
            return NULL;
        }

        rc = mbedtls_ecp_point_write_binary(g, p, MBEDTLS_ECP_PF_UNCOMPRESSED,
                &len, ssh_string_data(s), ssh_string_len(s));
    }

    if (rc != 0) {
        SSH_STRING_FREE(s);
        return NULL;
    }

    if (len != ssh_string_len(s)) {
        SSH_STRING_FREE(s);
        return NULL;
    }

    return s;
}

static const char* pki_key_ecdsa_nid_to_char(int nid)
{
    switch (nid) {
        case NID_mbedtls_nistp256:
            return "nistp256";
        case NID_mbedtls_nistp384:
            return "nistp384";
        case NID_mbedtls_nistp521:
            return "nistp521";
        default:
            break;
    }

    return "unknown";
}

ssh_string pki_key_to_blob(const ssh_key key, enum ssh_key_e type)
{
    ssh_buffer buffer = NULL;
    ssh_string type_s = NULL;
    ssh_string e = NULL;
    ssh_string n = NULL;
    ssh_string p = NULL;
    ssh_string q = NULL;
    ssh_string d = NULL;
    ssh_string iqmp = NULL;
    ssh_string str = NULL;
    int rc;
#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_mpi E = {0};
    mbedtls_mpi N = {0};
    mbedtls_mpi D = {0};
    mbedtls_mpi IQMP = {0};
    mbedtls_mpi P = {0};
    mbedtls_mpi Q = {0};

    mbedtls_mpi_init(&E);
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&D);
    mbedtls_mpi_init(&IQMP);
    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
#endif

    buffer = ssh_buffer_new();
    if (buffer == NULL) {
        return NULL;
    }
    /* The buffer will contain sensitive information. Make sure it is erased */
    ssh_buffer_set_secure(buffer);

    if (key->cert != NULL) {
        rc = ssh_buffer_add_buffer(buffer, key->cert);
        if (rc < 0) {
            SSH_BUFFER_FREE(buffer);
            return NULL;
        }

        goto makestring;
    }

    type_s = ssh_string_from_char(key->type_c);
    if (type_s == NULL) {
        SSH_BUFFER_FREE(buffer);
        return NULL;
    }

    rc = ssh_buffer_add_ssh_string(buffer, type_s);
    SSH_STRING_FREE(type_s);
    if (rc < 0) {
        SSH_BUFFER_FREE(buffer);
        return NULL;
    }

    switch (key->type) {
    case SSH_KEYTYPE_RSA: {
        mbedtls_rsa_context *rsa = NULL;
        mbedtls_mpi *E_ptr = NULL, *N_ptr = NULL;

        if (mbedtls_pk_can_do(key->pk, MBEDTLS_PK_RSA) == 0) {
            SSH_BUFFER_FREE(buffer);
            return NULL;
        }

        rsa = mbedtls_pk_rsa(*key->pk);
#if MBEDTLS_VERSION_MAJOR > 2
        rc = mbedtls_rsa_export(rsa, &N, NULL, NULL, NULL, &E);
        if (rc != 0) {
            goto out;
        }

        E_ptr = &E;
        N_ptr = &N;
#else
        E_ptr = &rsa->E;
        N_ptr = &rsa->N;
#endif

        e = ssh_make_bignum_string(E_ptr);
        if (e == NULL) {
            goto out;
        }

        n = ssh_make_bignum_string(N_ptr);
        if (n == NULL) {
            goto out;
        }

        if (type == SSH_KEY_PUBLIC) {
            /* The N and E parts are swapped in the public key export ! */
            rc = ssh_buffer_add_ssh_string(buffer, e);
            if (rc < 0) {
                goto out;
            }

            rc = ssh_buffer_add_ssh_string(buffer, n);
            if (rc < 0) {
                goto out;
            }
        } else if (type == SSH_KEY_PRIVATE) {
            mbedtls_mpi *P_ptr = NULL, *Q_ptr = NULL, *D_ptr = NULL;
            mbedtls_mpi *IQMP_ptr = NULL;

            rc = ssh_buffer_add_ssh_string(buffer, n);
            if (rc < 0) {
                goto out;
            }

            rc = ssh_buffer_add_ssh_string(buffer, e);
            if (rc < 0) {
                goto out;
            }

#if MBEDTLS_VERSION_MAJOR > 2
            rc = mbedtls_rsa_export(rsa, NULL, &P, &Q, &D, NULL);
            if (rc != 0) {
                goto out;
            }

            rc = mbedtls_rsa_export_crt(rsa, NULL, NULL, &IQMP);
            if (rc != 0) {
                goto out;
            }

            P_ptr = &P;
            Q_ptr = &Q;
            D_ptr = &D;
            IQMP_ptr = &IQMP;
#else
            P_ptr = &rsa->P;
            Q_ptr = &rsa->Q;
            D_ptr = &rsa->D;
            IQMP_ptr = &rsa->QP;
#endif

            p = ssh_make_bignum_string(P_ptr);
            if (p == NULL) {
                goto out;
            }

            q = ssh_make_bignum_string(Q_ptr);
            if (q == NULL) {
                goto out;
            }

            d = ssh_make_bignum_string(D_ptr);
            if (d == NULL) {
                goto out;
            }

            iqmp = ssh_make_bignum_string(IQMP_ptr);
            if (iqmp == NULL) {
                goto out;
            }

            rc = ssh_buffer_add_ssh_string(buffer, d);
            if (rc < 0) {
                goto out;
            }

            rc = ssh_buffer_add_ssh_string(buffer, iqmp);
            if (rc < 0) {
                goto out;
            }

            rc = ssh_buffer_add_ssh_string(buffer, p);
            if (rc < 0) {
                goto out;
            }

            rc = ssh_buffer_add_ssh_string(buffer, q);
            if (rc < 0) {
                goto out;
            }
        }
        break;
    }
    case SSH_KEYTYPE_ECDSA_P256:
    case SSH_KEYTYPE_ECDSA_P384:
    case SSH_KEYTYPE_ECDSA_P521:
    case SSH_KEYTYPE_SK_ECDSA:
        type_s =
            ssh_string_from_char(pki_key_ecdsa_nid_to_char(key->ecdsa_nid));
        if (type_s == NULL) {
            SSH_BUFFER_FREE(buffer);
            return NULL;
        }

        rc = ssh_buffer_add_ssh_string(buffer, type_s);
        SSH_STRING_FREE(type_s);
        if (rc < 0) {
            SSH_BUFFER_FREE(buffer);
            return NULL;
        }

        e = make_ecpoint_string(&key->ecdsa->MBEDTLS_PRIVATE(grp),
                                &key->ecdsa->MBEDTLS_PRIVATE(Q));

        if (e == NULL) {
            SSH_BUFFER_FREE(buffer);
            return NULL;
        }

        rc = ssh_buffer_add_ssh_string(buffer, e);
        if (rc < 0) {
            goto out;
        }

        if (type == SSH_KEY_PRIVATE) {
            d = ssh_make_bignum_string(&key->ecdsa->MBEDTLS_PRIVATE(d));

            if (d == NULL) {
                SSH_BUFFER_FREE(buffer);
                goto out;
            }

            rc = ssh_buffer_add_ssh_string(buffer, d);
            if (rc < 0) {
                goto out;
            }
        } else if (key->type == SSH_KEYTYPE_SK_ECDSA) {
            /* public key can contain certificate sk information */
            rc = ssh_buffer_add_ssh_string(buffer, key->sk_application);
            if (rc < 0) {
                goto out;
            }
        }
        break;
    case SSH_KEYTYPE_ED25519:
    case SSH_KEYTYPE_SK_ED25519:
        if (type == SSH_KEY_PUBLIC) {
            rc = pki_ed25519_public_key_to_blob(buffer, key);
            if (rc == SSH_ERROR) {
                goto out;
            }
            /* public key can contain certificate sk information */
            if (key->type == SSH_KEYTYPE_SK_ED25519) {
                rc = ssh_buffer_add_ssh_string(buffer, key->sk_application);
                if (rc < 0) {
                    goto out;
                }
            }
        } else {
            rc = pki_ed25519_private_key_to_blob(buffer, key);
            if (rc == SSH_ERROR) {
                goto out;
            }
        }
        break;
    default:
        goto out;
    }
makestring:
    str = ssh_string_new(ssh_buffer_get_len(buffer));
    if (str == NULL) {
        goto out;
    }

    rc = ssh_string_fill(str,
                         ssh_buffer_get(buffer),
                         ssh_buffer_get_len(buffer));
    if (rc < 0) {
        ssh_string_burn(str);
        SSH_STRING_FREE(str);
    }

out:
    SSH_BUFFER_FREE(buffer);
    ssh_string_burn(e);
    SSH_STRING_FREE(e);
    ssh_string_burn(n);
    SSH_STRING_FREE(n);
    ssh_string_burn(d);
    SSH_STRING_FREE(d);
    ssh_string_burn(iqmp);
    SSH_STRING_FREE(iqmp);
    ssh_string_burn(p);
    SSH_STRING_FREE(p);
    ssh_string_burn(q);
    SSH_STRING_FREE(q);
#if MBEDTLS_VERSION_MAJOR > 2
    mbedtls_mpi_free(&N);
    mbedtls_mpi_free(&E);
    mbedtls_mpi_free(&D);
    mbedtls_mpi_free(&IQMP);
    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&Q);
#endif

    return str;
}

ssh_string pki_signature_to_blob(const ssh_signature sig)
{
    ssh_string sig_blob = NULL;

    switch(sig->type) {
        case SSH_KEYTYPE_RSA:
            sig_blob = ssh_string_copy(sig->rsa_sig);
            break;
        case SSH_KEYTYPE_ECDSA_P256:
        case SSH_KEYTYPE_ECDSA_P384:
        case SSH_KEYTYPE_ECDSA_P521: {
            ssh_string r = NULL;
            ssh_string s = NULL;
            ssh_buffer b = NULL;
            int rc;

            b = ssh_buffer_new();
            if (b == NULL) {
                return NULL;
            }

            r = ssh_make_bignum_string(sig->ecdsa_sig.r);
            if (r == NULL) {
                SSH_BUFFER_FREE(b);
                return NULL;
            }

            rc = ssh_buffer_add_ssh_string(b, r);
            SSH_STRING_FREE(r);
            if (rc < 0) {
                SSH_BUFFER_FREE(b);
                return NULL;
            }

            s = ssh_make_bignum_string(sig->ecdsa_sig.s);
            if (s == NULL) {
                SSH_BUFFER_FREE(b);
                return NULL;
            }

            rc = ssh_buffer_add_ssh_string(b, s);
            SSH_STRING_FREE(s);
            if (rc < 0) {
                SSH_BUFFER_FREE(b);
                return NULL;
            }

            sig_blob = ssh_string_new(ssh_buffer_get_len(b));
            if (sig_blob == NULL) {
                SSH_BUFFER_FREE(b);
                return NULL;
            }

            rc = ssh_string_fill(sig_blob, ssh_buffer_get(b), ssh_buffer_get_len(b));
            SSH_BUFFER_FREE(b);
            if (rc < 0) {
                SSH_STRING_FREE(sig_blob);
                return NULL;
            }

            break;
        }
        case SSH_KEYTYPE_ED25519:
            sig_blob = pki_ed25519_signature_to_blob(sig);
            break;
        default:
            SSH_LOG(SSH_LOG_TRACE, "Unknown signature key type: %s",
                    sig->type_c);
            return NULL;
    }

    return sig_blob;
}

static ssh_signature pki_signature_from_rsa_blob(const ssh_key pubkey, const
        ssh_string sig_blob, ssh_signature sig)
{
    size_t pad_len = 0;
    char *blob_orig = NULL;
    char *blob_padded_data = NULL;
    ssh_string sig_blob_padded = NULL;

    size_t rsalen = 0;
    size_t len = ssh_string_len(sig_blob);

    if (pubkey->pk == NULL) {
        SSH_LOG(SSH_LOG_TRACE, "Pubkey RSA field NULL");
        goto errout;
    }

    rsalen = mbedtls_pk_get_bitlen(pubkey->pk) / 8;
    if (len > rsalen) {
        SSH_LOG(SSH_LOG_TRACE,
                "Signature is too big: %lu > %lu",
                (unsigned long) len,
                (unsigned long) rsalen);
        goto errout;
    }
#ifdef DEBUG_CRYPTO
    SSH_LOG(SSH_LOG_TRACE, "RSA signature len: %lu", (unsigned long)len);
    ssh_log_hexdump("RSA signature", ssh_string_data(sig_blob), len);
#endif

    if (len == rsalen) {
        sig->rsa_sig = ssh_string_copy(sig_blob);
    } else {
        SSH_LOG(SSH_LOG_DEBUG, "RSA signature len %lu < %lu",
                (unsigned long) len,
                (unsigned long) rsalen);
        pad_len = rsalen - len;

        sig_blob_padded = ssh_string_new(rsalen);
        if (sig_blob_padded == NULL) {
            goto errout;
        }

        blob_padded_data = (char *) ssh_string_data(sig_blob_padded);
        blob_orig = (char *) ssh_string_data(sig_blob);

        explicit_bzero(blob_padded_data, pad_len);
        memcpy(blob_padded_data + pad_len, blob_orig, len);

        sig->rsa_sig = sig_blob_padded;
    }

    return sig;

errout:
    ssh_signature_free(sig);
    return NULL;
}
ssh_signature pki_signature_from_blob(const ssh_key pubkey,
                                      const ssh_string sig_blob,
                                      enum ssh_keytypes_e type,
                                      enum ssh_digest_e hash_type)
{
    ssh_signature sig = NULL;
    int rc;

    if (ssh_key_type_plain(pubkey->type) != type) {
        SSH_LOG(SSH_LOG_TRACE,
                "Incompatible public key provided (%d) expecting (%d)",
                type,
                pubkey->type);
        return NULL;
    }

    sig = ssh_signature_new();
    if (sig == NULL) {
        return NULL;
    }

    sig->type = type;
    sig->type_c = ssh_key_signature_to_char(type, hash_type);
    sig->hash_type = hash_type;

    switch(type) {
        case SSH_KEYTYPE_RSA:
            sig = pki_signature_from_rsa_blob(pubkey, sig_blob, sig);
            if (sig == NULL) {
                return NULL;
            }
            break;
        case SSH_KEYTYPE_ECDSA_P256:
        case SSH_KEYTYPE_ECDSA_P384:
        case SSH_KEYTYPE_ECDSA_P521:
        case SSH_KEYTYPE_SK_ECDSA: {
            ssh_buffer b = NULL;
            ssh_string r = NULL;
            ssh_string s = NULL;
            size_t rlen;

            b = ssh_buffer_new();
            if (b == NULL) {
                ssh_signature_free(sig);
                return NULL;
            }

            rc = ssh_buffer_add_data(b, ssh_string_data(sig_blob),
                    ssh_string_len(sig_blob));

            if (rc < 0) {
                SSH_BUFFER_FREE(b);
                ssh_signature_free(sig);
                return NULL;
            }

            r = ssh_buffer_get_ssh_string(b);
            if (r == NULL) {
                SSH_BUFFER_FREE(b);
                ssh_signature_free(sig);
                return NULL;
            }
#ifdef DEBUG_CRYPTO
            ssh_log_hexdump("r", ssh_string_data(r), ssh_string_len(r));
#endif
            sig->ecdsa_sig.r = ssh_make_string_bn(r);
            ssh_string_burn(r);
            SSH_STRING_FREE(r);
            if (sig->ecdsa_sig.r == NULL) {
                SSH_BUFFER_FREE(b);
                ssh_signature_free(sig);
                return NULL;
            }

            s = ssh_buffer_get_ssh_string(b);
            rlen = ssh_buffer_get_len(b);
            SSH_BUFFER_FREE(b);
            if (s == NULL) {
                ssh_signature_free(sig);
                return NULL;
            }

#ifdef DEBUG_CRYPTO
            ssh_log_hexdump("s", ssh_string_data(s), ssh_string_len(s));
#endif
            sig->ecdsa_sig.s = ssh_make_string_bn(s);
            ssh_string_burn(s);
            SSH_STRING_FREE(s);
            if (sig->ecdsa_sig.s == NULL) {
                ssh_signature_free(sig);
                return NULL;
            }

            if (rlen != 0) {
                SSH_LOG(SSH_LOG_TRACE, "Signature has remaining bytes in inner "
                        "sigblob: %lu",
                        (unsigned long)rlen);
                ssh_signature_free(sig);
                return NULL;
            }

            break;
        }
        case SSH_KEYTYPE_ED25519:
        case SSH_KEYTYPE_SK_ED25519:
            rc = pki_signature_from_ed25519_blob(sig, sig_blob);
            if (rc == SSH_ERROR) {
                ssh_signature_free(sig);
                return NULL;
            }
            break;
        default:
            SSH_LOG(SSH_LOG_TRACE, "Unknown signature type");
            return NULL;
    }

    return sig;
}

static ssh_string rsa_do_sign_hash(const unsigned char *digest,
                                   int dlen,
                                   mbedtls_pk_context *privkey,
                                   enum ssh_digest_e hash_type)
{
    ssh_string sig_blob = NULL;
    mbedtls_md_type_t md = 0;
    unsigned char *sig = NULL;
    size_t slen;
    size_t sig_size;
    int ok;

    switch (hash_type) {
    case SSH_DIGEST_SHA1:
        md = MBEDTLS_MD_SHA1;
        break;
    case SSH_DIGEST_SHA256:
        md = MBEDTLS_MD_SHA256;
        break;
    case SSH_DIGEST_SHA512:
        md = MBEDTLS_MD_SHA512;
        break;
    case SSH_DIGEST_AUTO:
    default:
        SSH_LOG(SSH_LOG_TRACE, "Incompatible key algorithm");
        return NULL;
    }

    sig_size = mbedtls_pk_get_bitlen(privkey) / 8;
    sig = malloc(sig_size);
    if (sig == NULL) {
        return NULL;
    }

    ok = mbedtls_pk_sign(privkey,
                         md,
                         digest,
                         dlen,
                         sig,
#if MBEDTLS_VERSION_MAJOR > 2
                         sig_size,
#endif
                         &slen,
                         mbedtls_ctr_drbg_random,
                         ssh_get_mbedtls_ctr_drbg_context());

    if (ok != 0) {
        SAFE_FREE(sig);
        return NULL;
    }

    sig_blob = ssh_string_new(slen);
    if (sig_blob == NULL) {
        SAFE_FREE(sig);
        return NULL;
    }

    ok = ssh_string_fill(sig_blob, sig, slen);
    explicit_bzero(sig, slen);
    SAFE_FREE(sig);
    if (ok < 0) {
        SSH_STRING_FREE(sig_blob);
        return NULL;
    }

    return sig_blob;
}


ssh_signature pki_do_sign_hash(const ssh_key privkey,
                               const unsigned char *hash,
                               size_t hlen,
                               enum ssh_digest_e hash_type)
{
    ssh_signature sig = NULL;
    int rc;

    sig = ssh_signature_new();
    if (sig == NULL) {
        return NULL;
    }

    sig->type = privkey->type;
    sig->type_c = ssh_key_signature_to_char(privkey->type, hash_type);
    sig->hash_type = hash_type;

    switch(privkey->type) {
        case SSH_KEYTYPE_RSA:
            sig->rsa_sig = rsa_do_sign_hash(hash, hlen, privkey->pk, hash_type);
            if (sig->rsa_sig == NULL) {
                ssh_signature_free(sig);
                return NULL;
            }
            break;
        case SSH_KEYTYPE_ECDSA_P256:
        case SSH_KEYTYPE_ECDSA_P384:
        case SSH_KEYTYPE_ECDSA_P521:
            sig->ecdsa_sig.r = bignum_new();
            if (sig->ecdsa_sig.r == NULL) {
                return NULL;
            }

            sig->ecdsa_sig.s = bignum_new();
            if (sig->ecdsa_sig.s == NULL) {
                bignum_safe_free(sig->ecdsa_sig.r);
                return NULL;
            }

            rc = mbedtls_ecdsa_sign(&privkey->ecdsa->MBEDTLS_PRIVATE(grp),
                                    sig->ecdsa_sig.r,
                                    sig->ecdsa_sig.s,
                                    &privkey->ecdsa->MBEDTLS_PRIVATE(d),
                                    hash,
                                    hlen,
                                    mbedtls_ctr_drbg_random,
                                    ssh_get_mbedtls_ctr_drbg_context());
            if (rc != 0) {
                ssh_signature_free(sig);
                return NULL;
            }
            break;
        case SSH_KEYTYPE_ED25519:
            rc = pki_ed25519_sign(privkey, sig, hash, hlen);
            if (rc != SSH_OK) {
                ssh_signature_free(sig);
                return NULL;
            }
            break;
        default:
            ssh_signature_free(sig);
            return NULL;

    }

    return sig;
}

/**
 * @internal
 *
 * @brief Sign the given input data. The digest of to be signed is calculated
 * internally as necessary.
 *
 * @param[in]   privkey     The private key to be used for signing.
 * @param[in]   hash_type   The digest algorithm to be used.
 * @param[in]   input       The data to be signed.
 * @param[in]   input_len   The length of the data to be signed.
 *
 * @return  a newly allocated ssh_signature or NULL on error.
 */
ssh_signature pki_sign_data(const ssh_key privkey,
                            enum ssh_digest_e hash_type,
                            const unsigned char *input,
                            size_t input_len)
{
    unsigned char hash[SHA512_DIGEST_LEN] = {0};
    const unsigned char *sign_input = NULL;
    uint32_t hlen = 0;
    int rc;

    if (privkey == NULL || !ssh_key_is_private(privkey) || input == NULL) {
        SSH_LOG(SSH_LOG_TRACE, "Bad parameter provided to "
                               "pki_sign_data()");
        return NULL;
    }

    /* Check if public key and hash type are compatible */
    rc = pki_key_check_hash_compatible(privkey, hash_type);
    if (rc != SSH_OK) {
        return NULL;
    }

    switch (hash_type) {
    case SSH_DIGEST_SHA256:
        sha256(input, input_len, hash);
        hlen = SHA256_DIGEST_LEN;
        sign_input = hash;
        break;
    case SSH_DIGEST_SHA384:
        sha384(input, input_len, hash);
        hlen = SHA384_DIGEST_LEN;
        sign_input = hash;
        break;
    case SSH_DIGEST_SHA512:
        sha512(input, input_len, hash);
        hlen = SHA512_DIGEST_LEN;
        sign_input = hash;
        break;
    case SSH_DIGEST_SHA1:
        sha1_esp32_port(input, input_len, hash);
        hlen = SHA_DIGEST_LEN;
        sign_input = hash;
        break;
    case SSH_DIGEST_AUTO:
        if (privkey->type == SSH_KEYTYPE_ED25519) {
            /* SSH_DIGEST_AUTO should only be used with ed25519 */
            sign_input = input;
            hlen = input_len;
            break;
        }
        FALL_THROUGH;
    default:
        SSH_LOG(SSH_LOG_TRACE, "Unknown hash algorithm for type: %d",
                hash_type);
        return NULL;
    }

    return pki_do_sign_hash(privkey, sign_input, hlen, hash_type);
}

/**
 * @internal
 *
 * @brief Verify the signature of a given input. The digest of the input is
 * calculated internally as necessary.
 *
 * @param[in]   signature   The signature to be verified.
 * @param[in]   pubkey      The public key used to verify the signature.
 * @param[in]   input       The signed data.
 * @param[in]   input_len   The length of the signed data.
 *
 * @return  SSH_OK if the signature is valid; SSH_ERROR otherwise.
 */
int pki_verify_data_signature(ssh_signature signature,
                              const ssh_key pubkey,
                              const unsigned char *input,
                              size_t input_len)
{

    unsigned char hash[SHA512_DIGEST_LEN] = {0};
    const unsigned char *verify_input = NULL;
    uint32_t hlen = 0;

    mbedtls_md_type_t md = 0;

    int rc;

    if (pubkey == NULL || ssh_key_is_private(pubkey) || input == NULL ||
        signature == NULL)
    {
        SSH_LOG(SSH_LOG_TRACE, "Bad parameter provided to "
                               "pki_verify_data_signature()");
        return SSH_ERROR;
    }

    /* Check if public key and hash type are compatible */
    rc = pki_key_check_hash_compatible(pubkey, signature->hash_type);
    if (rc != SSH_OK) {
        return SSH_ERROR;
    }

    switch (signature->hash_type) {
    case SSH_DIGEST_SHA256:
        sha256(input, input_len, hash);
        hlen = SHA256_DIGEST_LEN;
        md = MBEDTLS_MD_SHA256;
        verify_input = hash;
        break;
    case SSH_DIGEST_SHA384:
        sha384(input, input_len, hash);
        hlen = SHA384_DIGEST_LEN;
        md = MBEDTLS_MD_SHA384;
        verify_input = hash;
        break;
    case SSH_DIGEST_SHA512:
        sha512(input, input_len, hash);
        hlen = SHA512_DIGEST_LEN;
        md = MBEDTLS_MD_SHA512;
        verify_input = hash;
        break;
    case SSH_DIGEST_SHA1:
        sha1_esp32_port(input, input_len, hash);
        hlen = SHA_DIGEST_LEN;
        md = MBEDTLS_MD_SHA1;
        verify_input = hash;
        break;
    case SSH_DIGEST_AUTO:
        if (pubkey->type == SSH_KEYTYPE_ED25519 ||
            pubkey->type == SSH_KEYTYPE_ED25519_CERT01 ||
            pubkey->type == SSH_KEYTYPE_SK_ED25519 ||
            pubkey->type == SSH_KEYTYPE_ED25519_CERT01)
        {
            verify_input = input;
            hlen = input_len;
            break;
        }
        FALL_THROUGH;
    default:
        SSH_LOG(SSH_LOG_TRACE, "Unknown sig->hash_type: %d",
                signature->hash_type);
        return SSH_ERROR;
    }

    switch (pubkey->type) {
        case SSH_KEYTYPE_RSA:
        case SSH_KEYTYPE_RSA_CERT01:
            rc = mbedtls_pk_verify(pubkey->pk,
                                   md,
                                   hash,
                                   hlen,
                                   ssh_string_data(signature->rsa_sig),
                                   ssh_string_len(signature->rsa_sig));
            if (rc != 0) {
                char error_buf[100];
                mbedtls_strerror(rc, error_buf, 100);
                SSH_LOG(SSH_LOG_TRACE, "RSA error: %s", error_buf);
                return SSH_ERROR;
            }
            break;
        case SSH_KEYTYPE_ECDSA_P256:
        case SSH_KEYTYPE_ECDSA_P384:
        case SSH_KEYTYPE_ECDSA_P521:
        case SSH_KEYTYPE_ECDSA_P256_CERT01:
        case SSH_KEYTYPE_ECDSA_P384_CERT01:
        case SSH_KEYTYPE_ECDSA_P521_CERT01:
        case SSH_KEYTYPE_SK_ECDSA:
        case SSH_KEYTYPE_SK_ECDSA_CERT01:
            rc = mbedtls_ecdsa_verify(&pubkey->ecdsa->MBEDTLS_PRIVATE(grp), hash,
                    hlen, &pubkey->ecdsa->MBEDTLS_PRIVATE(Q),
                    signature->ecdsa_sig.r,
                    signature->ecdsa_sig.s);
            if (rc != 0) {
                char error_buf[100];
                mbedtls_strerror(rc, error_buf, 100);
                SSH_LOG(SSH_LOG_TRACE, "ECDSA error: %s", error_buf);
                return SSH_ERROR;

            }
            break;
        case SSH_KEYTYPE_ED25519:
        case SSH_KEYTYPE_ED25519_CERT01:
        case SSH_KEYTYPE_SK_ED25519:
        case SSH_KEYTYPE_SK_ED25519_CERT01:
            rc = pki_ed25519_verify(pubkey, signature, verify_input, hlen);
            if (rc != SSH_OK) {
                SSH_LOG(SSH_LOG_TRACE, "ED25519 error: Signature invalid");
                return SSH_ERROR;
            }
            break;
        default:
            SSH_LOG(SSH_LOG_TRACE, "Unknown public key type");
            return SSH_ERROR;
    }

    return SSH_OK;
}

const char *pki_key_ecdsa_nid_to_name(int nid)
{
    switch (nid) {
        case NID_mbedtls_nistp256:
            return "ecdsa-sha2-nistp256";
        case NID_mbedtls_nistp384:
            return "ecdsa-sha2-nistp384";
        case NID_mbedtls_nistp521:
            return "ecdsa-sha2-nistp521";
        default:
            break;
    }

    return "unknown";
}

int pki_key_ecdsa_nid_from_name(const char *name)
{
    if (strcmp(name, "nistp256") == 0) {
        return NID_mbedtls_nistp256;
    } else if (strcmp(name, "nistp384") == 0) {
        return NID_mbedtls_nistp384;
    } else if (strcmp(name, "nistp521") == 0) {
        return NID_mbedtls_nistp521;
    }

    return -1;
}

static mbedtls_ecp_group_id pki_key_ecdsa_nid_to_mbed_gid(int nid)
{
    switch (nid) {
        case NID_mbedtls_nistp256:
            return MBEDTLS_ECP_DP_SECP256R1;
        case NID_mbedtls_nistp384:
            return MBEDTLS_ECP_DP_SECP384R1;
        case NID_mbedtls_nistp521:
            return MBEDTLS_ECP_DP_SECP521R1;
    }

    return MBEDTLS_ECP_DP_NONE;
}

int pki_privkey_build_ecdsa(ssh_key key, int nid, ssh_string e, ssh_string exp)
{
    int rc;
    mbedtls_ecp_keypair keypair;
    mbedtls_ecp_group group;
    mbedtls_ecp_point Q;

    key->ecdsa_nid = nid;
    key->type_c = pki_key_ecdsa_nid_to_name(nid);

    key->ecdsa = malloc(sizeof(mbedtls_ecdsa_context));
    if (key->ecdsa == NULL) {
        return SSH_ERROR;
    }

    mbedtls_ecdsa_init(key->ecdsa);
    mbedtls_ecp_keypair_init(&keypair);
    mbedtls_ecp_group_init(&group);
    mbedtls_ecp_point_init(&Q);

    rc = mbedtls_ecp_group_load(&group,
                                pki_key_ecdsa_nid_to_mbed_gid(nid));
    if (rc != 0) {
        goto fail;
    }

    rc = mbedtls_ecp_point_read_binary(&group, &Q, ssh_string_data(e),
                                       ssh_string_len(e));
    if (rc != 0) {
        goto fail;
    }

    rc = mbedtls_ecp_copy(&keypair.MBEDTLS_PRIVATE(Q), &Q);
    if (rc != 0) {
        goto fail;
    }

    rc = mbedtls_ecp_group_copy(&keypair.MBEDTLS_PRIVATE(grp), &group);
    if (rc != 0) {
        goto fail;
    }

    rc = mbedtls_mpi_read_binary(&keypair.MBEDTLS_PRIVATE(d),
                    ssh_string_data(exp),
                    ssh_string_len(exp));
    if (rc != 0) {
        goto fail;
    }

    rc = mbedtls_ecdsa_from_keypair(key->ecdsa, &keypair);
    if (rc != 0) {
        goto fail;
    }

    mbedtls_ecp_point_free(&Q);
    mbedtls_ecp_group_free(&group);
    mbedtls_ecp_keypair_free(&keypair);
    return SSH_OK;

fail:
    mbedtls_ecdsa_free(key->ecdsa);
    mbedtls_ecp_point_free(&Q);
    mbedtls_ecp_group_free(&group);
    mbedtls_ecp_keypair_free(&keypair);
    SAFE_FREE(key->ecdsa);
    return SSH_ERROR;
}

int pki_pubkey_build_ecdsa(ssh_key key, int nid, ssh_string e)
{
    int rc;
    mbedtls_ecp_keypair keypair;
    mbedtls_ecp_group group;
    mbedtls_ecp_point Q;

    key->ecdsa_nid = nid;
    key->type_c = pki_key_ecdsa_nid_to_name(nid);

    key->ecdsa = malloc(sizeof(mbedtls_ecdsa_context));
    if (key->ecdsa == NULL) {
        return SSH_ERROR;
    }

    mbedtls_ecdsa_init(key->ecdsa);
    mbedtls_ecp_keypair_init(&keypair);
    mbedtls_ecp_group_init(&group);
    mbedtls_ecp_point_init(&Q);

    rc = mbedtls_ecp_group_load(&group,
            pki_key_ecdsa_nid_to_mbed_gid(nid));
    if (rc != 0) {
        goto fail;
    }

    rc = mbedtls_ecp_point_read_binary(&group, &Q, ssh_string_data(e),
            ssh_string_len(e));
    if (rc != 0) {
        goto fail;
    }

    rc = mbedtls_ecp_copy(&keypair.MBEDTLS_PRIVATE(Q), &Q);
    if (rc != 0) {
        goto fail;
    }

    rc = mbedtls_ecp_group_copy(&keypair.MBEDTLS_PRIVATE(grp), &group);
    if (rc != 0) {
        goto fail;
    }

    mbedtls_mpi_init(&keypair.MBEDTLS_PRIVATE(d));

    rc = mbedtls_ecdsa_from_keypair(key->ecdsa, &keypair);
    if (rc != 0) {
        goto fail;
    }

    mbedtls_ecp_point_free(&Q);
    mbedtls_ecp_group_free(&group);
    mbedtls_ecp_keypair_free(&keypair);
    return SSH_OK;
fail:
    mbedtls_ecdsa_free(key->ecdsa);
    mbedtls_ecp_point_free(&Q);
    mbedtls_ecp_group_free(&group);
    mbedtls_ecp_keypair_free(&keypair);
    SAFE_FREE(key->ecdsa);
    return SSH_ERROR;
}

int pki_key_generate_ecdsa(ssh_key key, int parameter)
{
    int ok;

    switch (parameter) {
        case 384:
            key->ecdsa_nid = NID_mbedtls_nistp384;
            key->type = SSH_KEYTYPE_ECDSA_P384;
            break;
        case 521:
            key->ecdsa_nid = NID_mbedtls_nistp521;
            key->type = SSH_KEYTYPE_ECDSA_P521;
            break;
        case 256:
        default:
            key->ecdsa_nid = NID_mbedtls_nistp256;
            key->type = SSH_KEYTYPE_ECDSA_P256;
            break;
    }

    key->ecdsa = malloc(sizeof(mbedtls_ecdsa_context));
    if (key->ecdsa == NULL) {
        return SSH_ERROR;
    }

    mbedtls_ecdsa_init(key->ecdsa);

    ok = mbedtls_ecdsa_genkey(key->ecdsa,
                              pki_key_ecdsa_nid_to_mbed_gid(key->ecdsa_nid),
                              mbedtls_ctr_drbg_random,
                              ssh_get_mbedtls_ctr_drbg_context());

    if (ok != 0) {
        mbedtls_ecdsa_free(key->ecdsa);
        SAFE_FREE(key->ecdsa);
    }

    return SSH_OK;
}

int ssh_key_size(ssh_key key)
{
    switch (key->type) {
    case SSH_KEYTYPE_RSA:
    case SSH_KEYTYPE_RSA_CERT01:
    case SSH_KEYTYPE_RSA1:
        return mbedtls_pk_get_bitlen(key->pk);
    case SSH_KEYTYPE_ECDSA_P256:
    case SSH_KEYTYPE_ECDSA_P256_CERT01:
    case SSH_KEYTYPE_SK_ECDSA:
    case SSH_KEYTYPE_SK_ECDSA_CERT01:
        return 256;
    case SSH_KEYTYPE_ECDSA_P384:
    case SSH_KEYTYPE_ECDSA_P384_CERT01:
        return 384;
    case SSH_KEYTYPE_ECDSA_P521:
    case SSH_KEYTYPE_ECDSA_P521_CERT01:
        return 521;
    case SSH_KEYTYPE_ED25519:
    case SSH_KEYTYPE_ED25519_CERT01:
    case SSH_KEYTYPE_SK_ED25519:
    case SSH_KEYTYPE_SK_ED25519_CERT01:
        /* ed25519 keys have fixed size */
        return 255;
    case SSH_KEYTYPE_DSS:   /* deprecated */
    case SSH_KEYTYPE_DSS_CERT01:    /* deprecated */
    case SSH_KEYTYPE_UNKNOWN:
    default:
        return SSH_ERROR;
    }
}

#ifdef WITH_PKCS11_URI
int pki_uri_import(const char *uri_name, ssh_key *key, enum ssh_key_e key_type)
{
    (void) uri_name;
    (void) key;
    (void) key_type;
    SSH_LOG(SSH_LOG_WARN,
            "mbedcrypto does not support PKCS #11");
    return SSH_ERROR;
}
#endif /* WITH_PKCS11_URI */
#endif /* HAVE_LIBMBEDCRYPTO */
