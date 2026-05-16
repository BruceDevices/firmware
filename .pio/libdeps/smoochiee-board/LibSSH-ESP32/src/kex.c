/*
 * kex.c - key exchange
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2008 by Aris Adamantiadis
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "libssh/libssh.h"
#include "libssh/priv.h"
#include "libssh/buffer.h"
#include "libssh/dh.h"
#ifdef WITH_GEX
#include "libssh/dh-gex.h"
#endif /* WITH_GEX */
#include "libssh/kex.h"
#include "libssh/session.h"
#include "libssh/ssh2.h"
#include "libssh/string.h"
#include "libssh/curve25519.h"
#include "libssh/knownhosts.h"
#include "libssh/misc.h"
#include "libssh/pki.h"
#include "libssh/bignum.h"
#include "libssh/token.h"

#ifdef HAVE_BLOWFISH
# define BLOWFISH ",blowfish-cbc"
#else
# define BLOWFISH ""
#endif

#ifdef HAVE_LIBGCRYPT
# define AES "aes256-gcm@openssh.com,aes128-gcm@openssh.com," \
             "aes256-ctr,aes192-ctr,aes128-ctr"
# define AES_CBC ",aes256-cbc,aes192-cbc,aes128-cbc"
# define DES_SUPPORTED ",3des-cbc"

#elif defined(HAVE_LIBMBEDCRYPTO)
# ifdef MBEDTLS_GCM_C
#  define GCM "aes256-gcm@openssh.com,aes128-gcm@openssh.com,"
# else
#  define GCM ""
# endif /* MBEDTLS_GCM_C */
# define AES GCM "aes256-ctr,aes192-ctr,aes128-ctr"
# define AES_CBC ",aes256-cbc,aes192-cbc,aes128-cbc"
# define DES_SUPPORTED ",3des-cbc"

#elif defined(HAVE_LIBCRYPTO)
# ifdef HAVE_OPENSSL_AES_H
#  define GCM "aes256-gcm@openssh.com,aes128-gcm@openssh.com,"
#  define AES GCM "aes256-ctr,aes192-ctr,aes128-ctr"
#  define AES_CBC ",aes256-cbc,aes192-cbc,aes128-cbc"
# else /* HAVE_OPENSSL_AES_H */
#  define AES ""
#  define AES_CBC ""
# endif /* HAVE_OPENSSL_AES_H */

# define DES_SUPPORTED ",3des-cbc"
#endif /* HAVE_LIBCRYPTO */

#ifdef WITH_ZLIB
#define ZLIB "none,zlib@openssh.com,zlib"
#define ZLIB_DEFAULT "none,zlib@openssh.com"
#else
#define ZLIB "none"
#define ZLIB_DEFAULT "none"
#endif /* WITH_ZLIB */

#ifdef HAVE_CURVE25519
#define CURVE25519 "curve25519-sha256,curve25519-sha256@libssh.org,"
#else
#define CURVE25519 ""
#endif /* HAVE_CURVE25519 */

#ifdef HAVE_ECC
#define ECDH "ecdh-sha2-nistp256,ecdh-sha2-nistp384,ecdh-sha2-nistp521,"
#define EC_HOSTKEYS "ecdsa-sha2-nistp521," \
                    "ecdsa-sha2-nistp384," \
                    "ecdsa-sha2-nistp256,"
#define EC_SK_HOSTKEYS "sk-ecdsa-sha2-nistp256@openssh.com,"
#define EC_FIPS_PUBLIC_KEY_ALGOS "ecdsa-sha2-nistp521-cert-v01@openssh.com," \
                                 "ecdsa-sha2-nistp384-cert-v01@openssh.com," \
                                 "ecdsa-sha2-nistp256-cert-v01@openssh.com,"
#define EC_PUBLIC_KEY_ALGORITHMS EC_FIPS_PUBLIC_KEY_ALGOS \
                                 "sk-ecdsa-sha2-nistp256-cert-v01@openssh.com,"
#else
#define ECDH ""
#define EC_HOSTKEYS ""
#define EC_SK_HOSTKEYS ""
#define EC_FIPS_PUBLIC_KEY_ALGOS ""
#define EC_PUBLIC_KEY_ALGORITHMS ""
#endif /* HAVE_ECC */

#ifdef WITH_INSECURE_NONE
#define NONE ",none"
#else
#define NONE
#endif /* WITH_INSECURE_NONE */

#define HOSTKEYS "ssh-ed25519," \
                 EC_HOSTKEYS \
                 "sk-ssh-ed25519@openssh.com," \
                 EC_SK_HOSTKEYS \
                 "rsa-sha2-512," \
                 "rsa-sha2-256," \
                 "ssh-rsa"
#define DEFAULT_HOSTKEYS "ssh-ed25519," \
                         EC_HOSTKEYS \
                         "sk-ssh-ed25519@openssh.com," \
                         EC_SK_HOSTKEYS \
                         "rsa-sha2-512," \
                         "rsa-sha2-256"

#define PUBLIC_KEY_ALGORITHMS "ssh-ed25519-cert-v01@openssh.com," \
                              "sk-ssh-ed25519-cert-v01@openssh.com," \
                              EC_PUBLIC_KEY_ALGORITHMS \
                              "rsa-sha2-512-cert-v01@openssh.com," \
                              "rsa-sha2-256-cert-v01@openssh.com," \
                              "ssh-rsa-cert-v01@openssh.com," \
                              HOSTKEYS
#define DEFAULT_PUBLIC_KEY_ALGORITHMS "ssh-ed25519-cert-v01@openssh.com," \
                                      EC_PUBLIC_KEY_ALGORITHMS \
                                      "rsa-sha2-512-cert-v01@openssh.com," \
                                      "rsa-sha2-256-cert-v01@openssh.com," \
                                      DEFAULT_HOSTKEYS

#ifdef WITH_GEX
#define GEX_SHA256 "diffie-hellman-group-exchange-sha256,"
#define GEX_SHA1 "diffie-hellman-group-exchange-sha1,"
#else
#define GEX_SHA256
#define GEX_SHA1
#endif /* WITH_GEX */

#define CHACHA20 "chacha20-poly1305@openssh.com,"

#define DEFAULT_KEY_EXCHANGE \
    CURVE25519 \
    ECDH \
    "diffie-hellman-group18-sha512,diffie-hellman-group16-sha512," \
    GEX_SHA256 \
    "diffie-hellman-group14-sha256" \

#define KEY_EXCHANGE_SUPPORTED \
    GEX_SHA1 \
    DEFAULT_KEY_EXCHANGE \
    ",diffie-hellman-group14-sha1,diffie-hellman-group1-sha1"

/* RFC 8308 */
#define KEX_EXTENSION_CLIENT "ext-info-c"
/* Strict kex mitigation against CVE-2023-48795 */
#define KEX_STRICT_CLIENT "kex-strict-c-v00@openssh.com"
#define KEX_STRICT_SERVER "kex-strict-s-v00@openssh.com"

/* Allowed algorithms in FIPS mode */
#define FIPS_ALLOWED_CIPHERS "aes256-gcm@openssh.com,"\
                             "aes256-ctr,"\
                             "aes256-cbc,"\
                             "aes128-gcm@openssh.com,"\
                             "aes128-ctr,"\
                             "aes128-cbc"

#define FIPS_ALLOWED_HOSTKEYS EC_HOSTKEYS \
                              "rsa-sha2-512," \
                              "rsa-sha2-256"

#define FIPS_ALLOWED_PUBLIC_KEY_ALGORITHMS EC_FIPS_PUBLIC_KEY_ALGOS \
                                           "rsa-sha2-512-cert-v01@openssh.com," \
                                           "rsa-sha2-256-cert-v01@openssh.com," \
                                           FIPS_ALLOWED_HOSTKEYS

#define FIPS_ALLOWED_KEX "ecdh-sha2-nistp256,"\
                         "ecdh-sha2-nistp384,"\
                         "ecdh-sha2-nistp521,"\
                         "diffie-hellman-group-exchange-sha256,"\
                         "diffie-hellman-group14-sha256,"\
                         "diffie-hellman-group16-sha512,"\
                         "diffie-hellman-group18-sha512"

#define FIPS_ALLOWED_MACS "hmac-sha2-256-etm@openssh.com,"\
                          "hmac-sha1-etm@openssh.com,"\
                          "hmac-sha2-512-etm@openssh.com,"\
                          "hmac-sha2-256,"\
                          "hmac-sha1,"\
                          "hmac-sha2-512"

/* NOTE: This is a fixed API and the index is defined by ssh_kex_types_e */
static const char *fips_methods[] = {
    FIPS_ALLOWED_KEX,
    FIPS_ALLOWED_PUBLIC_KEY_ALGORITHMS,
    FIPS_ALLOWED_CIPHERS,
    FIPS_ALLOWED_CIPHERS,
    FIPS_ALLOWED_MACS,
    FIPS_ALLOWED_MACS,
    ZLIB_DEFAULT,
    ZLIB_DEFAULT,
    "",
    "",
    NULL
};

/* NOTE: This is a fixed API and the index is defined by ssh_kex_types_e */
static const char *default_methods[] = {
    DEFAULT_KEY_EXCHANGE,
    DEFAULT_PUBLIC_KEY_ALGORITHMS,
    AES,
    AES,
    "hmac-sha2-256-etm@openssh.com,hmac-sha2-512-etm@openssh.com,hmac-sha2-256,hmac-sha2-512",
    "hmac-sha2-256-etm@openssh.com,hmac-sha2-512-etm@openssh.com,hmac-sha2-256,hmac-sha2-512",
    ZLIB_DEFAULT,
    ZLIB_DEFAULT,
    "",
    "",
    NULL
};

/* NOTE: This is a fixed API and the index is defined by ssh_kex_types_e */
static const char *supported_methods[] = {
  KEY_EXCHANGE_SUPPORTED,
  PUBLIC_KEY_ALGORITHMS,
  AES AES_CBC BLOWFISH DES_SUPPORTED NONE,
  AES AES_CBC BLOWFISH DES_SUPPORTED NONE,
  "hmac-sha2-256-etm@openssh.com,hmac-sha2-512-etm@openssh.com,hmac-sha1-etm@openssh.com,hmac-sha2-256,hmac-sha2-512,hmac-sha1" NONE,
  "hmac-sha2-256-etm@openssh.com,hmac-sha2-512-etm@openssh.com,hmac-sha1-etm@openssh.com,hmac-sha2-256,hmac-sha2-512,hmac-sha1" NONE,
  ZLIB,
  ZLIB,
  "",
  "",
  NULL
};

/* descriptions of the key exchange packet */
static const char *ssh_kex_descriptions[] = {
  "kex algos",
  "server host key algo",
  "encryption client->server",
  "encryption server->client",
  "mac algo client->server",
  "mac algo server->client",
  "compression algo client->server",
  "compression algo server->client",
  "languages client->server",
  "languages server->client",
  NULL
};

const char *ssh_kex_get_default_methods(uint32_t algo)
{
    if (algo >= SSH_KEX_METHODS) {
        return NULL;
    }

    return default_methods[algo];
}

const char *ssh_kex_get_supported_method(uint32_t algo)
{
    if (algo >= SSH_KEX_METHODS) {
        return NULL;
    }

    return supported_methods[algo];
}

const char *ssh_kex_get_description(uint32_t algo) {
  if (algo >= SSH_KEX_METHODS) {
    return NULL;
  }

  return ssh_kex_descriptions[algo];
}

const char *ssh_kex_get_fips_methods(uint32_t algo) {
  if (algo >= SSH_KEX_METHODS) {
    return NULL;
  }

  return fips_methods[algo];
}

/**
 * @internal
 * @brief returns whether the first client key exchange algorithm or
 *        hostkey type matches its server counterpart
 * @returns whether the first client key exchange algorithm or hostkey type
 *          matches its server counterpart
 */
static int cmp_first_kex_algo(const char *client_str,
                              const char *server_str) {
    size_t client_kex_len;
    size_t server_kex_len;

    char *colon = NULL;

    int is_wrong = 1;

    if (client_str == NULL || server_str == NULL) {
        return is_wrong;
    }

    colon = strchr(client_str, ',');
    if (colon == NULL) {
        client_kex_len = strlen(client_str);
    } else {
        client_kex_len = colon - client_str;
    }

    colon = strchr(server_str, ',');
    if (colon == NULL) {
        server_kex_len = strlen(server_str);
    } else {
        server_kex_len = colon - server_str;
    }

    if (client_kex_len != server_kex_len) {
        return is_wrong;
    }

    is_wrong = (strncmp(client_str, server_str, client_kex_len) != 0);

    return is_wrong;
}

SSH_PACKET_CALLBACK(ssh_packet_kexinit)
{
    int i, ok;
    struct ssh_crypto_struct *crypto = session->next_crypto;
    int server_kex = session->server;
    ssh_string str = NULL;
    char *strings[SSH_KEX_METHODS] = {0};
    int rc = SSH_ERROR;
    size_t len;

    uint8_t first_kex_packet_follows = 0;
    uint32_t kexinit_reserved = 0;

    (void)type;
    (void)user;

    SSH_LOG(SSH_LOG_TRACE, "KEXINIT received");

    if (session->session_state == SSH_SESSION_STATE_AUTHENTICATED) {
        if (session->dh_handshake_state == DH_STATE_FINISHED) {
            SSH_LOG(SSH_LOG_DEBUG, "Peer initiated key re-exchange");
            /* Reset the sent flag if the re-kex was initiated by the peer */
            session->flags &= ~SSH_SESSION_FLAG_KEXINIT_SENT;
        } else if (session->flags & SSH_SESSION_FLAG_KEXINIT_SENT &&
                   session->dh_handshake_state == DH_STATE_INIT_SENT) {
            /* This happens only when we are sending our-guessed first kex
             * packet right after our KEXINIT packet. */
            SSH_LOG(SSH_LOG_DEBUG, "Received peer kexinit answer.");
        } else if (session->session_state != SSH_SESSION_STATE_INITIAL_KEX) {
            ssh_set_error(session, SSH_FATAL,
                          "SSH_KEXINIT received in wrong state");
            goto error;
        }
    } else if (session->session_state != SSH_SESSION_STATE_INITIAL_KEX) {
        ssh_set_error(session, SSH_FATAL,
                      "SSH_KEXINIT received in wrong state");
        goto error;
    }

    if (server_kex) {
#ifdef WITH_SERVER
        len = ssh_buffer_get_data(packet, crypto->client_kex.cookie, 16);
        if (len != 16) {
            ssh_set_error(session, SSH_FATAL,
                          "ssh_packet_kexinit: no cookie in packet");
            goto error;
        }

        ok = ssh_hashbufin_add_cookie(session, crypto->client_kex.cookie);
        if (ok < 0) {
            ssh_set_error(session, SSH_FATAL,
                          "ssh_packet_kexinit: adding cookie failed");
            goto error;
        }

        ok = server_set_kex(session);
        if (ok == SSH_ERROR) {
            goto error;
        }
#endif /* WITH_SERVER */
    } else {
        len = ssh_buffer_get_data(packet, crypto->server_kex.cookie, 16);
        if (len != 16) {
            ssh_set_error(session, SSH_FATAL,
                          "ssh_packet_kexinit: no cookie in packet");
            goto error;
        }

        ok = ssh_hashbufin_add_cookie(session, crypto->server_kex.cookie);
        if (ok < 0) {
            ssh_set_error(session, SSH_FATAL,
                          "ssh_packet_kexinit: adding cookie failed");
            goto error;
        }

        ok = ssh_set_client_kex(session);
        if (ok == SSH_ERROR) {
            goto error;
        }
    }

    for (i = 0; i < SSH_KEX_METHODS; i++) {
        str = ssh_buffer_get_ssh_string(packet);
        if (str == NULL) {
          goto error;
        }

        rc = ssh_buffer_add_ssh_string(session->in_hashbuf, str);
        if (rc < 0) {
            ssh_set_error(session, SSH_FATAL,
                          "Error adding string in hash buffer");
            goto error;
        }

        strings[i] = ssh_string_to_char(str);
        if (strings[i] == NULL) {
            ssh_set_error_oom(session);
            goto error;
        }
        SSH_STRING_FREE(str);
        str = NULL;
    }

    /* copy the peer kex info into an array of strings */
    if (server_kex) {
#ifdef WITH_SERVER
        for (i = 0; i < SSH_KEX_METHODS; i++) {
            crypto->client_kex.methods[i] = strings[i];
        }
#endif /* WITH_SERVER */
    } else { /* client */
        for (i = 0; i < SSH_KEX_METHODS; i++) {
            crypto->server_kex.methods[i] = strings[i];
        }
    }

    /*
     * Handle the two final fields for the KEXINIT message (RFC 4253 7.1):
     *
     *      boolean      first_kex_packet_follows
     *      uint32       0 (reserved for future extension)
     *
     * Notably if clients set 'first_kex_packet_follows', it is expected
     * that its value is included when computing the session ID (see
     * 'make_sessionid').
     */

    rc = ssh_buffer_get_u8(packet, &first_kex_packet_follows);
    if (rc != 1) {
        goto error;
    }

    rc = ssh_buffer_add_u8(session->in_hashbuf, first_kex_packet_follows);
    if (rc < 0) {
        goto error;
    }

    rc = ssh_buffer_add_u32(session->in_hashbuf, kexinit_reserved);
    if (rc < 0) {
        goto error;
    }

    /*
     * Remember whether 'first_kex_packet_follows' was set and the client
     * guess was wrong: in this case the next SSH_MSG_KEXDH_INIT message
     * must be ignored on the server side.
     * Client needs to start the Key exchange over with the correct method
     */
    if (first_kex_packet_follows || session->send_first_kex_follows) {
        char **client_methods = crypto->client_kex.methods;
        char **server_methods = crypto->server_kex.methods;
        session->first_kex_follows_guess_wrong =
            cmp_first_kex_algo(client_methods[SSH_KEX],
                               server_methods[SSH_KEX]) ||
            cmp_first_kex_algo(client_methods[SSH_HOSTKEYS],
                               server_methods[SSH_HOSTKEYS]);
            SSH_LOG(SSH_LOG_DEBUG, "The initial guess was %s.",
                    session->first_kex_follows_guess_wrong ? "wrong" : "right");
    }

    /*
     * handle the "strict KEX" feature. If supported by peer, then set up the
     * flag and verify packet sequence numbers.
     */
    if (server_kex) {
        ok = match_group(crypto->client_kex.methods[SSH_KEX],
                         KEX_STRICT_CLIENT);
        if (ok) {
            SSH_LOG(SSH_LOG_DEBUG, "Client supports strict kex, enabling.");
            session->flags |= SSH_SESSION_FLAG_KEX_STRICT;
        }
    } else {
        /* client kex */
        ok = match_group(crypto->server_kex.methods[SSH_KEX],
                         KEX_STRICT_SERVER);
        if (ok) {
            SSH_LOG(SSH_LOG_DEBUG, "Server supports strict kex, enabling.");
            session->flags |= SSH_SESSION_FLAG_KEX_STRICT;
        }
    }
#ifdef WITH_SERVER
    if (server_kex) {
        /*
         * If client sent a ext-info-c message in the kex list, it supports
         * RFC 8308 extension negotiation.
         */
        ok = match_group(crypto->client_kex.methods[SSH_KEX],
                         KEX_EXTENSION_CLIENT);
        if (ok) {
            const char *hostkeys = NULL, *wanted_hostkeys = NULL;

            /* The client supports extension negotiation */
            session->extensions |= SSH_EXT_NEGOTIATION;
            /*
             * RFC 8332 Section 3.1: Use for Server Authentication
             * Check what algorithms were provided in the SSH_HOSTKEYS list
             * by the client and enable the respective extensions to provide
             * correct signature in the next packet if RSA is negotiated
             */
            hostkeys = crypto->client_kex.methods[SSH_HOSTKEYS];
            wanted_hostkeys = session->opts.wanted_methods[SSH_HOSTKEYS];
            ok = match_group(hostkeys, "rsa-sha2-512");
            if (ok) {
                /* Check if rsa-sha2-512 is allowed by config */
                if (wanted_hostkeys != NULL) {
                    char *is_allowed = ssh_find_matching(wanted_hostkeys,
                                                         "rsa-sha2-512");
                    if (is_allowed != NULL) {
                        session->extensions |= SSH_EXT_SIG_RSA_SHA512;
                    }
                    SAFE_FREE(is_allowed);
                }
            }
            ok = match_group(hostkeys, "rsa-sha2-256");
            if (ok) {
                /* Check if rsa-sha2-256 is allowed by config */
                if (wanted_hostkeys != NULL) {
                    char *is_allowed = ssh_find_matching(wanted_hostkeys,
                                                         "rsa-sha2-256");
                    if (is_allowed != NULL) {
                        session->extensions |= SSH_EXT_SIG_RSA_SHA256;
                    }
                    SAFE_FREE(is_allowed);
                }
            }

            /*
             * Ensure that the client preference is honored for the case
             * both signature types are enabled.
             */
            if ((session->extensions & SSH_EXT_SIG_RSA_SHA256) &&
                (session->extensions & SSH_EXT_SIG_RSA_SHA512)) {
                char *rsa_sig_ext = NULL;
                session->extensions &= ~(SSH_EXT_SIG_RSA_SHA256 | SSH_EXT_SIG_RSA_SHA512);
                rsa_sig_ext = ssh_find_matching("rsa-sha2-512,rsa-sha2-256",
                                                hostkeys);
                if (rsa_sig_ext == NULL) {
                    goto error; /* should never happen */
                } else if (strcmp(rsa_sig_ext, "rsa-sha2-512") == 0) {
                    session->extensions |= SSH_EXT_SIG_RSA_SHA512;
                } else if (strcmp(rsa_sig_ext, "rsa-sha2-256") == 0) {
                    session->extensions |= SSH_EXT_SIG_RSA_SHA256;
                } else {
                    SAFE_FREE(rsa_sig_ext);
                    goto error; /* should never happen */
                }
                SAFE_FREE(rsa_sig_ext);
            }

            SSH_LOG(SSH_LOG_DEBUG, "The client supports extension "
                    "negotiation. Enabled signature algorithms: %s%s",
                    session->extensions & SSH_EXT_SIG_RSA_SHA256 ? "SHA256" : "",
                    session->extensions & SSH_EXT_SIG_RSA_SHA512 ? " SHA512" : "");
        }
    }
#endif /* WITH_SERVER */

    /* Note, that his overwrites authenticated state in case of rekeying */
    session->session_state = SSH_SESSION_STATE_KEXINIT_RECEIVED;
    /* if we already sent our initial key exchange packet, do not reset the
     * DH state. We will know if we were right with our guess only in
     * dh_handshake_state() */
    if (session->send_first_kex_follows == false) {
        session->dh_handshake_state = DH_STATE_INIT;
    }
    session->ssh_connection_callback(session);
    return SSH_PACKET_USED;

error:
    SSH_STRING_FREE(str);
    for (i = 0; i < SSH_KEX_METHODS; i++) {
        if (server_kex) {
#ifdef WITH_SERVER
            session->next_crypto->client_kex.methods[i] = NULL;
#endif /* WITH_SERVER */
        } else { /* client */
            session->next_crypto->server_kex.methods[i] = NULL;
        }
        SAFE_FREE(strings[i]);
    }

    session->session_state = SSH_SESSION_STATE_ERROR;

    return SSH_PACKET_USED;
}

void ssh_list_kex(struct ssh_kex_struct *kex) {
  int i = 0;

#ifdef DEBUG_CRYPTO
  ssh_log_hexdump("session cookie", kex->cookie, 16);
#endif

  for(i = 0; i < SSH_KEX_METHODS; i++) {
    if (kex->methods[i] == NULL) {
      continue;
    }
    SSH_LOG(SSH_LOG_FUNCTIONS, "%s: %s",
        ssh_kex_descriptions[i], kex->methods[i]);
  }
}

/**
 * @internal
 *
 * @brief selects the hostkey mechanisms to be chosen for the key exchange,
 * as some hostkey mechanisms may be present in known_hosts files.
 *
 * @returns a cstring containing a comma-separated list of hostkey methods.
 *          NULL if no method matches
 */
char *ssh_client_select_hostkeys(ssh_session session)
{
    const char *wanted = NULL;
    char *wanted_without_certs = NULL;
    char *known_hosts_algorithms = NULL;
    char *known_hosts_ordered = NULL;
    char *new_hostkeys = NULL;
    char *fips_hostkeys = NULL;

    wanted = session->opts.wanted_methods[SSH_HOSTKEYS];
    if (wanted == NULL) {
        if (ssh_fips_mode()) {
            wanted = ssh_kex_get_fips_methods(SSH_HOSTKEYS);
        } else {
            wanted = ssh_kex_get_default_methods(SSH_HOSTKEYS);
        }
    }

    /* This removes the certificate types, unsupported for now */
    wanted_without_certs = ssh_find_all_matching(HOSTKEYS, wanted);
    if (wanted_without_certs == NULL) {
        SSH_LOG(SSH_LOG_TRACE,
                "List of allowed host key algorithms is empty or contains only "
                "unsupported algorithms");
        return NULL;
    }

    SSH_LOG(SSH_LOG_DEBUG,
            "Order of wanted host keys: \"%s\"",
            wanted_without_certs);

    known_hosts_algorithms = ssh_known_hosts_get_algorithms_names(session);
    if (known_hosts_algorithms == NULL) {
        SSH_LOG(SSH_LOG_DEBUG,
                "No key found in known_hosts; "
                "changing host key method to \"%s\"",
                wanted_without_certs);

        return wanted_without_certs;
    }

    SSH_LOG(SSH_LOG_DEBUG,
            "Algorithms found in known_hosts files: \"%s\"",
            known_hosts_algorithms);

    /* Filter and order the keys from known_hosts according to wanted list */
    known_hosts_ordered = ssh_find_all_matching(known_hosts_algorithms,
                                                wanted_without_certs);
    SAFE_FREE(known_hosts_algorithms);
    if (known_hosts_ordered == NULL) {
        SSH_LOG(SSH_LOG_DEBUG,
                "No key found in known_hosts is allowed; "
                "changing host key method to \"%s\"",
                wanted_without_certs);

        return wanted_without_certs;
    }

    /* Append the other supported keys after the preferred ones
     * This function tolerates NULL pointers in parameters */
    new_hostkeys = ssh_append_without_duplicates(known_hosts_ordered,
                                                 wanted_without_certs);
    SAFE_FREE(known_hosts_ordered);
    SAFE_FREE(wanted_without_certs);
    if (new_hostkeys == NULL) {
        ssh_set_error_oom(session);
        return NULL;
    }

    if (ssh_fips_mode()) {
        /* Filter out algorithms not allowed in FIPS mode */
        fips_hostkeys = ssh_keep_fips_algos(SSH_HOSTKEYS, new_hostkeys);
        SAFE_FREE(new_hostkeys);
        if (fips_hostkeys == NULL) {
            SSH_LOG(SSH_LOG_TRACE,
                    "None of the wanted host keys or keys in known_hosts files "
                    "is allowed in FIPS mode.");
            return NULL;
        }
        new_hostkeys = fips_hostkeys;
    }

    SSH_LOG(SSH_LOG_DEBUG,
            "Changing host key method to \"%s\"",
            new_hostkeys);

    return new_hostkeys;
}

/**
 * @brief sets the key exchange parameters to be sent to the server,
 *        in function of the options and available methods.
 */
int ssh_set_client_kex(ssh_session session)
{
    struct ssh_kex_struct *client = &session->next_crypto->client_kex;
    const char *wanted = NULL;
    int ok;
    int i;

    /* Skip if already set, for example for the rekey or when we do the guessing
     * it could have been already used to make some protocol decisions. */
    if (client->methods[0] != NULL) {
        return SSH_OK;
    }

    ok = ssh_get_random(client->cookie, 16, 0);
    if (!ok) {
        ssh_set_error(session, SSH_FATAL, "PRNG error");
        return SSH_ERROR;
    }

    /* Set the list of allowed algorithms in order of preference, if it hadn't
     * been set yet. */
    for (i = 0; i < SSH_KEX_METHODS; i++) {
        if (i == SSH_HOSTKEYS) {
            /* Set the hostkeys in the following order:
             * - First: keys present in known_hosts files ordered by preference
             * - Next: other wanted algorithms ordered by preference */
            client->methods[i] = ssh_client_select_hostkeys(session);
            if (client->methods[i] == NULL) {
                ssh_set_error_oom(session);
                return SSH_ERROR;
            }
            continue;
        }

        wanted = session->opts.wanted_methods[i];
        if (wanted == NULL) {
            if (ssh_fips_mode()) {
                wanted = fips_methods[i];
            } else {
                wanted = default_methods[i];
            }
        }
        client->methods[i] = strdup(wanted);
        if (client->methods[i] == NULL) {
            ssh_set_error_oom(session);
            return SSH_ERROR;
        }
    }

    /* For rekeying, skip the extension negotiation */
    if (session->flags & SSH_SESSION_FLAG_AUTHENTICATED) {
        return SSH_OK;
    }

    ok = ssh_kex_append_extensions(session, client);
    if (ok != SSH_OK){
        return ok;
    }

    return SSH_OK;
}

int ssh_kex_append_extensions(ssh_session session, struct ssh_kex_struct *pkex)
{
    char *kex = NULL;
    char *kex_tmp = NULL;
    size_t kex_len, len;

    /* Here we append ext-info-c and kex-strict-c-v00@openssh.com for client
     * and kex-strict-s-v00@openssh.com for server to the list of kex algorithms
     */
    kex = pkex->methods[SSH_KEX];
    len = strlen(kex);
    if (session->server) {
        /* Comma, nul byte */
        kex_len = len + 1 + strlen(KEX_STRICT_SERVER) + 1;
    } else {
        /* Comma, comma, nul byte */
        kex_len = len + 1 + strlen(KEX_EXTENSION_CLIENT) + 1 +
                  strlen(KEX_STRICT_CLIENT) + 1;
    }
    if (kex_len >= MAX_PACKET_LEN) {
        /* Overflow */
        return SSH_ERROR;
    }
    kex_tmp = realloc(kex, kex_len);
    if (kex_tmp == NULL) {
        ssh_set_error_oom(session);
        return SSH_ERROR;
    }
    if (session->server){
        snprintf(kex_tmp + len, kex_len - len, ",%s", KEX_STRICT_SERVER);
    } else {
        snprintf(kex_tmp + len,
                 kex_len - len,
                 ",%s,%s",
                 KEX_EXTENSION_CLIENT,
                 KEX_STRICT_CLIENT);
    }
    pkex->methods[SSH_KEX] = kex_tmp;
    return SSH_OK;
}

static const char *ssh_find_aead_hmac(const char *cipher)
{
    if (cipher == NULL) {
        return NULL;
    } else if (strcmp(cipher, "chacha20-poly1305@openssh.com") == 0) {
        return "aead-poly1305";
    } else if (strcmp(cipher, "aes256-gcm@openssh.com") == 0) {
        return "aead-gcm";
    } else if (strcmp(cipher, "aes128-gcm@openssh.com") == 0) {
        return "aead-gcm";
    }
    return NULL;
}

static enum ssh_key_exchange_e
kex_select_kex_type(const char *kex)
{
    if (strcmp(kex, "diffie-hellman-group1-sha1") == 0) {
        return SSH_KEX_DH_GROUP1_SHA1;
    } else if (strcmp(kex, "diffie-hellman-group14-sha1") == 0) {
        return SSH_KEX_DH_GROUP14_SHA1;
    } else if (strcmp(kex, "diffie-hellman-group14-sha256") == 0) {
        return SSH_KEX_DH_GROUP14_SHA256;
    } else if (strcmp(kex, "diffie-hellman-group16-sha512") == 0) {
        return SSH_KEX_DH_GROUP16_SHA512;
    } else if (strcmp(kex, "diffie-hellman-group18-sha512") == 0) {
        return SSH_KEX_DH_GROUP18_SHA512;
#ifdef WITH_GEX
    } else if (strcmp(kex, "diffie-hellman-group-exchange-sha1") == 0) {
        return SSH_KEX_DH_GEX_SHA1;
    } else if (strcmp(kex, "diffie-hellman-group-exchange-sha256") == 0) {
        return SSH_KEX_DH_GEX_SHA256;
#endif /* WITH_GEX */
    } else if (strcmp(kex, "ecdh-sha2-nistp256") == 0) {
        return SSH_KEX_ECDH_SHA2_NISTP256;
    } else if (strcmp(kex, "ecdh-sha2-nistp384") == 0) {
        return SSH_KEX_ECDH_SHA2_NISTP384;
    } else if (strcmp(kex, "ecdh-sha2-nistp521") == 0) {
        return SSH_KEX_ECDH_SHA2_NISTP521;
    } else if (strcmp(kex, "curve25519-sha256@libssh.org") == 0) {
        return SSH_KEX_CURVE25519_SHA256_LIBSSH_ORG;
    } else if (strcmp(kex, "curve25519-sha256") == 0) {
        return SSH_KEX_CURVE25519_SHA256;
    }
    /* should not happen. We should be getting only valid names at this stage */
    return 0;
}


/** @internal
 * @brief Reverts guessed callbacks set during the dh_handshake()
 * @param session session handle
 * @returns void
 */
static void revert_kex_callbacks(ssh_session session)
{
    switch (session->next_crypto->kex_type) {
    case SSH_KEX_DH_GROUP1_SHA1:
    case SSH_KEX_DH_GROUP14_SHA1:
    case SSH_KEX_DH_GROUP14_SHA256:
    case SSH_KEX_DH_GROUP16_SHA512:
    case SSH_KEX_DH_GROUP18_SHA512:
        ssh_client_dh_remove_callbacks(session);
        break;
#ifdef WITH_GEX
    case SSH_KEX_DH_GEX_SHA1:
    case SSH_KEX_DH_GEX_SHA256:
        ssh_client_dhgex_remove_callbacks(session);
        break;
#endif /* WITH_GEX */
#ifdef HAVE_ECDH
    case SSH_KEX_ECDH_SHA2_NISTP256:
    case SSH_KEX_ECDH_SHA2_NISTP384:
    case SSH_KEX_ECDH_SHA2_NISTP521:
        ssh_client_ecdh_remove_callbacks(session);
        break;
#endif
#ifdef HAVE_CURVE25519
    case SSH_KEX_CURVE25519_SHA256:
    case SSH_KEX_CURVE25519_SHA256_LIBSSH_ORG:
        ssh_client_curve25519_remove_callbacks(session);
        break;
#endif
    }
}

/** @brief Select the different methods on basis of client's and
 * server's kex messages, and watches out if a match is possible.
 */
int ssh_kex_select_methods (ssh_session session)
{
    struct ssh_crypto_struct *crypto = session->next_crypto;
    struct ssh_kex_struct *server = &crypto->server_kex;
    struct ssh_kex_struct *client = &crypto->client_kex;
    char *ext_start = NULL;
    const char *aead_hmac = NULL;
    enum ssh_key_exchange_e kex_type;
    int i;

    /* Here we should drop the extensions from the list so we avoid matching.
     * it. We added it to the end, so we can just truncate the string here */
    if (session->client) {
        ext_start = strstr(client->methods[SSH_KEX], "," KEX_EXTENSION_CLIENT);
        if (ext_start != NULL) {
            ext_start[0] = '\0';
        }
    }
    if (session->server) {
        ext_start = strstr(server->methods[SSH_KEX], "," KEX_STRICT_SERVER);
        if (ext_start != NULL) {
            ext_start[0] = '\0';
        }
    }

    for (i = 0; i < SSH_KEX_METHODS; i++) {
        crypto->kex_methods[i] = ssh_find_matching(server->methods[i],
                                                   client->methods[i]);

        if (i == SSH_MAC_C_S || i == SSH_MAC_S_C) {
            aead_hmac = ssh_find_aead_hmac(crypto->kex_methods[i - 2]);
            if (aead_hmac) {
                free(crypto->kex_methods[i]);
                crypto->kex_methods[i] = strdup(aead_hmac);
            }
        }
        if (crypto->kex_methods[i] == NULL && i < SSH_LANG_C_S) {
            ssh_set_error(session, SSH_FATAL,
                          "kex error : no match for method %s: server [%s], "
                          "client [%s]", ssh_kex_descriptions[i],
                          server->methods[i], client->methods[i]);
            return SSH_ERROR;
        } else if ((i >= SSH_LANG_C_S) && (crypto->kex_methods[i] == NULL)) {
            /* we can safely do that for languages */
            crypto->kex_methods[i] = strdup("");
        }
    }

    /* We can not set this value directly as the old value is needed to revert
     * callbacks if we are client */
    kex_type = kex_select_kex_type(crypto->kex_methods[SSH_KEX]);
    if (session->client && session->first_kex_follows_guess_wrong) {
        SSH_LOG(SSH_LOG_DEBUG, "Our guess was wrong. Restarting the KEX");
        /* We need to remove the wrong callbacks and start kex again */
        revert_kex_callbacks(session);
        session->dh_handshake_state = DH_STATE_INIT;
        session->first_kex_follows_guess_wrong = false;
    }
    crypto->kex_type = kex_type;

    SSH_LOG(SSH_LOG_DEBUG, "Negotiated %s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
            session->next_crypto->kex_methods[SSH_KEX],
            session->next_crypto->kex_methods[SSH_HOSTKEYS],
            session->next_crypto->kex_methods[SSH_CRYPT_C_S],
            session->next_crypto->kex_methods[SSH_CRYPT_S_C],
            session->next_crypto->kex_methods[SSH_MAC_C_S],
            session->next_crypto->kex_methods[SSH_MAC_S_C],
            session->next_crypto->kex_methods[SSH_COMP_C_S],
            session->next_crypto->kex_methods[SSH_COMP_S_C],
            session->next_crypto->kex_methods[SSH_LANG_C_S],
            session->next_crypto->kex_methods[SSH_LANG_S_C]
    );
    return SSH_OK;
}


/* this function only sends the predefined set of kex methods */
int ssh_send_kex(ssh_session session)
{
    struct ssh_kex_struct *kex = (session->server ?
        &session->next_crypto->server_kex :
        &session->next_crypto->client_kex);
    ssh_string str = NULL;
    int i;
    int rc;
    int first_kex_packet_follows = 0;

    /* Only client can initiate the handshake methods we implement. If we
     * already received the peer mechanisms, there is no point in guessing */
    if (session->client &&
        session->session_state != SSH_SESSION_STATE_KEXINIT_RECEIVED &&
        session->send_first_kex_follows) {
        first_kex_packet_follows = 1;
    }

    SSH_LOG(SSH_LOG_TRACE,
            "Sending KEXINIT packet, first_kex_packet_follows = %d",
            first_kex_packet_follows);

    rc = ssh_buffer_pack(session->out_buffer,
                         "bP",
                         SSH2_MSG_KEXINIT,
                         (size_t)16,
                         kex->cookie); /* cookie */
    if (rc != SSH_OK)
        goto error;
    if (ssh_hashbufout_add_cookie(session) < 0) {
        goto error;
    }

    ssh_list_kex(kex);

    for (i = 0; i < SSH_KEX_METHODS; i++) {
        str = ssh_string_from_char(kex->methods[i]);
        if (str == NULL) {
            goto error;
        }

        rc = ssh_buffer_add_ssh_string(session->out_hashbuf, str);
        if (rc < 0) {
            goto error;
        }
        rc = ssh_buffer_add_ssh_string(session->out_buffer, str);
        if (rc < 0) {
            goto error;
        }
        SSH_STRING_FREE(str);
        str = NULL;
    }

    rc = ssh_buffer_pack(session->out_buffer,
                         "bd",
                         first_kex_packet_follows,
                         0);
    if (rc != SSH_OK) {
        goto error;
    }

    /* Prepare also the first_kex_packet_follows and reserved to 0 */
    rc = ssh_buffer_add_u8(session->out_hashbuf, first_kex_packet_follows);
    if (rc < 0) {
        goto error;
    }
    rc = ssh_buffer_add_u32(session->out_hashbuf, 0);
    if (rc < 0) {
        goto error;
    }

    rc = ssh_packet_send(session);
    if (rc == SSH_ERROR) {
        return -1;
    }

    session->flags |= SSH_SESSION_FLAG_KEXINIT_SENT;
    SSH_LOG(SSH_LOG_PACKET, "SSH_MSG_KEXINIT sent");

    /* If we indicated that we are sending the guessed key exchange packet,
     * do it now. The packet is simple, but we need to do some preparations */
    if (first_kex_packet_follows == 1) {
        char *list = kex->methods[SSH_KEX];
        char *colon = strchr(list, ',');
        size_t kex_name_len = colon ? (size_t)(colon - list) : strlen(list);
        char *kex_name = calloc(kex_name_len + 1, 1);
        if (kex_name == NULL) {
            ssh_set_error_oom(session);
            goto error;
        }
        snprintf(kex_name, kex_name_len + 1, "%.*s", (int)kex_name_len, list);
        SSH_LOG(SSH_LOG_TRACE, "Sending the first kex packet for %s", kex_name);

        session->next_crypto->kex_type = kex_select_kex_type(kex_name);
        free(kex_name);

        /* run the first step of the DH handshake */
        session->dh_handshake_state = DH_STATE_INIT;
        if (dh_handshake(session) == SSH_ERROR) {
            goto error;
        }
    }
    return 0;

error:
    ssh_buffer_reinit(session->out_buffer);
    ssh_buffer_reinit(session->out_hashbuf);
    SSH_STRING_FREE(str);

    return -1;
}

/*
 * Key re-exchange (rekey) is triggered by this function.
 * It can not be called again after the rekey is initialized!
 */
int ssh_send_rekex(ssh_session session)
{
    int rc;

    if (session->dh_handshake_state != DH_STATE_FINISHED) {
        /* Rekey/Key exchange is already in progress */
        SSH_LOG(SSH_LOG_PACKET, "Attempting rekey in bad state");
        return SSH_ERROR;
    }

    if (session->current_crypto == NULL) {
        /* No current crypto used -- can not exchange it */
        SSH_LOG(SSH_LOG_PACKET, "No crypto to rekey");
        return SSH_ERROR;
    }

    if (session->client) {
        rc = ssh_set_client_kex(session);
        if (rc != SSH_OK) {
            SSH_LOG(SSH_LOG_PACKET, "Failed to set client kex");
            return rc;
        }
    } else {
#ifdef WITH_SERVER
        rc = server_set_kex(session);
        if (rc == SSH_ERROR) {
            SSH_LOG(SSH_LOG_PACKET, "Failed to set server kex");
            return rc;
        }
#else
        SSH_LOG(SSH_LOG_PACKET, "Invalid session state.");
        return SSH_ERROR;
#endif /* WITH_SERVER */
    }

    session->dh_handshake_state = DH_STATE_INIT;
    rc = ssh_send_kex(session);
    if (rc < 0) {
        SSH_LOG(SSH_LOG_PACKET, "Failed to send kex");
        return rc;
    }

    /* Reset the handshake state */
    session->dh_handshake_state = DH_STATE_INIT_SENT;
    return SSH_OK;
}

/* returns a copy of the provided list if everything is supported,
 * otherwise a new list of the supported algorithms */
char *ssh_keep_known_algos(enum ssh_kex_types_e algo, const char *list)
{
    if (algo > SSH_LANG_S_C) {
        return NULL;
    }

    return ssh_find_all_matching(supported_methods[algo], list);
}

/**
 * @internal
 *
 * @brief Return a newly allocated string containing only the FIPS allowed
 * algorithms from the list.
 *
 * @param[in] algo  The type of the methods to filter
 * @param[in] list  The list to be filtered
 *
 * @return A newly allocated list containing only the FIPS allowed algorithms from
 * the list; NULL in case of error.
 */
char *ssh_keep_fips_algos(enum ssh_kex_types_e algo, const char *list)
{
    if (algo > SSH_LANG_S_C) {
        return NULL;
    }

    return ssh_find_all_matching(fips_methods[algo], list);
}

/**
 * @internal
 *
 * @brief Return a newly allocated string containing the default
 * algorithms plus the algorithms specified in list. If the system
 * runs in fips mode, this will add only fips approved algorithms.
 * Empty list will cause error.
 *
 * @param[in] algo  The type of the methods to filter
 * @param[in] list  The list to be appended
 *
 * @return A newly allocated list containing the default algorithms and the
 * algorithms in list at the end; NULL in case of error.
 */
char *ssh_add_to_default_algos(enum ssh_kex_types_e algo, const char *list)
{
    char *tmp = NULL, *ret = NULL;

    if (algo > SSH_LANG_S_C || list == NULL || list[0] == '\0') {
        return NULL;
    }

    if (ssh_fips_mode()) {
        tmp = ssh_append_without_duplicates(fips_methods[algo], list);
        ret = ssh_find_all_matching(fips_methods[algo], tmp);
    } else {
        tmp = ssh_append_without_duplicates(default_methods[algo], list);
        ret = ssh_find_all_matching(supported_methods[algo], tmp);
    }

    free(tmp);
    return ret;
}

/**
 * @internal
 *
 * @brief Return a newly allocated string containing the default
 * algorithms excluding the algorithms specified in list. If the system
 * runs in fips mode, this will remove from the fips_methods list.
 *
 * @param[in] algo  The type of the methods to filter
 * @param[in] list  The list to be exclude
 *
 * @return A newly allocated list containing the default algorithms without the
 * algorithms in list; NULL in case of error.
 */
char *ssh_remove_from_default_algos(enum ssh_kex_types_e algo, const char *list)
{
    if (algo > SSH_LANG_S_C) {
        return NULL;
    }

    if (list == NULL || list[0] == '\0') {
        if (ssh_fips_mode()) {
            return strdup(fips_methods[algo]);
        } else {
            return strdup(default_methods[algo]);
        }
    }

    if (ssh_fips_mode()) {
        return ssh_remove_all_matching(fips_methods[algo], list);
    } else {
        return ssh_remove_all_matching(default_methods[algo], list);
    }
}

/**
 * @internal
 *
 * @brief Return a newly allocated string containing the default
 * algorithms with prioritized algorithms specified in list. If the
 * algorithms are present in the default list they get prioritized, if not
 * they are added to the front of the default list. If the system
 * runs in fips mode, this will work with the fips_methods list.
 * Empty list will cause error.
 *
 * @param[in] algo  The type of the methods to filter
 * @param[in] list  The list to be pushed to priority
 *
 * @return A newly allocated list containing the default algorithms prioritized
 * with the algorithms in list at the beginning of the list; NULL in case
 * of error.
 */
char *ssh_prefix_default_algos(enum ssh_kex_types_e algo, const char *list)
{
    char *ret = NULL, *tmp = NULL;

    if (algo > SSH_LANG_S_C || list == NULL || list[0] == '\0') {
        return NULL;
    }

    if (ssh_fips_mode()) {
        tmp = ssh_prefix_without_duplicates(fips_methods[algo], list);
        ret = ssh_find_all_matching(fips_methods[algo], tmp);
    } else {
        tmp = ssh_prefix_without_duplicates(default_methods[algo], list);
        ret = ssh_find_all_matching(supported_methods[algo], tmp);
    }

    free(tmp);
    return ret;
}

int ssh_make_sessionid(ssh_session session)
{
    ssh_string num = NULL;
    ssh_buffer server_hash = NULL;
    ssh_buffer client_hash = NULL;
    ssh_buffer buf = NULL;
    ssh_string server_pubkey_blob = NULL;
#if !defined(HAVE_LIBCRYPTO) || OPENSSL_VERSION_NUMBER < 0x30000000L
    const_bignum client_pubkey, server_pubkey;
#else
    bignum client_pubkey = NULL, server_pubkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER */
#ifdef WITH_GEX
#if !defined(HAVE_LIBCRYPTO) || OPENSSL_VERSION_NUMBER < 0x30000000L
    const_bignum modulus, generator;
#else
    bignum modulus = NULL, generator = NULL;
#endif /* OPENSSL_VERSION_NUMBER */
#endif /* WITH_GEX */
    int rc = SSH_ERROR;

    buf = ssh_buffer_new();
    if (buf == NULL) {
        return rc;
    }

    rc = ssh_buffer_pack(buf,
                         "ss",
                         session->clientbanner,
                         session->serverbanner);
    if (rc == SSH_ERROR) {
        goto error;
    }

    if (session->client) {
        server_hash = session->in_hashbuf;
        client_hash = session->out_hashbuf;
    } else {
        server_hash = session->out_hashbuf;
        client_hash = session->in_hashbuf;
    }

    rc = ssh_dh_get_next_server_publickey_blob(session, &server_pubkey_blob);
    if (rc != SSH_OK) {
        goto error;
    }

    rc = ssh_buffer_pack(buf,
                         "dPdPS",
                         ssh_buffer_get_len(client_hash),
                         (size_t)ssh_buffer_get_len(client_hash),
                         ssh_buffer_get(client_hash),
                         ssh_buffer_get_len(server_hash),
                         (size_t)ssh_buffer_get_len(server_hash),
                         ssh_buffer_get(server_hash),
                         server_pubkey_blob);
    SSH_STRING_FREE(server_pubkey_blob);
    if (rc != SSH_OK){
        goto error;
    }

    switch(session->next_crypto->kex_type) {
    case SSH_KEX_DH_GROUP1_SHA1:
    case SSH_KEX_DH_GROUP14_SHA1:
    case SSH_KEX_DH_GROUP14_SHA256:
    case SSH_KEX_DH_GROUP16_SHA512:
    case SSH_KEX_DH_GROUP18_SHA512:
        rc = ssh_dh_keypair_get_keys(session->next_crypto->dh_ctx,
                                     DH_CLIENT_KEYPAIR, NULL, &client_pubkey);
        if (rc != SSH_OK) {
            goto error;
        }
        rc = ssh_dh_keypair_get_keys(session->next_crypto->dh_ctx,
                                     DH_SERVER_KEYPAIR, NULL, &server_pubkey);
        if (rc != SSH_OK) {
            goto error;
        }
        rc = ssh_buffer_pack(buf,
                             "BB",
                             client_pubkey,
                             server_pubkey);
        if (rc != SSH_OK) {
            goto error;
        }
#if defined(HAVE_LIBCRYPTO) && OPENSSL_VERSION_NUMBER >= 0x30000000L
        bignum_safe_free(client_pubkey);
        bignum_safe_free(server_pubkey);
#endif /* OPENSSL_VERSION_NUMBER */
        break;
#ifdef WITH_GEX
    case SSH_KEX_DH_GEX_SHA1:
    case SSH_KEX_DH_GEX_SHA256:
        rc = ssh_dh_keypair_get_keys(session->next_crypto->dh_ctx,
                                     DH_CLIENT_KEYPAIR, NULL, &client_pubkey);
        if (rc != SSH_OK) {
            goto error;
        }
        rc = ssh_dh_keypair_get_keys(session->next_crypto->dh_ctx,
                                     DH_SERVER_KEYPAIR, NULL, &server_pubkey);
        if (rc != SSH_OK) {
            goto error;
        }
        rc = ssh_dh_get_parameters(session->next_crypto->dh_ctx,
                                   &modulus, &generator);
        if (rc != SSH_OK) {
            goto error;
        }
        rc = ssh_buffer_pack(buf,
                    "dddBBBB",
                    session->next_crypto->dh_pmin,
                    session->next_crypto->dh_pn,
                    session->next_crypto->dh_pmax,
                    modulus,
                    generator,
                    client_pubkey,
                    server_pubkey);
        if (rc != SSH_OK) {
            goto error;
        }
#if defined(HAVE_LIBCRYPTO) && OPENSSL_VERSION_NUMBER >= 0x30000000L
        bignum_safe_free(modulus);
        bignum_safe_free(generator);
#endif /* OPENSSL_VERSION_NUMBER */
        break;
#endif /* WITH_GEX */
#ifdef HAVE_ECDH
    case SSH_KEX_ECDH_SHA2_NISTP256:
    case SSH_KEX_ECDH_SHA2_NISTP384:
    case SSH_KEX_ECDH_SHA2_NISTP521:
        if (session->next_crypto->ecdh_client_pubkey == NULL ||
            session->next_crypto->ecdh_server_pubkey == NULL) {
            SSH_LOG(SSH_LOG_TRACE, "ECDH parameter missing");
            goto error;
        }
        rc = ssh_buffer_pack(buf,
                             "SS",
                             session->next_crypto->ecdh_client_pubkey,
                             session->next_crypto->ecdh_server_pubkey);
        if (rc != SSH_OK) {
            goto error;
        }
        break;
#endif /* HAVE_ECDH */
#ifdef HAVE_CURVE25519
    case SSH_KEX_CURVE25519_SHA256:
    case SSH_KEX_CURVE25519_SHA256_LIBSSH_ORG:
        rc = ssh_buffer_pack(buf,
                             "dPdP",
                             CURVE25519_PUBKEY_SIZE,
                             (size_t)CURVE25519_PUBKEY_SIZE,
                             session->next_crypto->curve25519_client_pubkey,
                             CURVE25519_PUBKEY_SIZE,
                             (size_t)CURVE25519_PUBKEY_SIZE,
                             session->next_crypto->curve25519_server_pubkey);

        if (rc != SSH_OK) {
            goto error;
        }
        break;
#endif /* HAVE_CURVE25519 */
    }
    rc = ssh_buffer_pack(buf, "B", session->next_crypto->shared_secret);
    if (rc != SSH_OK) {
        goto error;
    }

#ifdef DEBUG_CRYPTO
    ssh_log_hexdump("hash buffer", ssh_buffer_get(buf), ssh_buffer_get_len(buf));
#endif

    /* Set rc for the following switch statement in case we goto error. */
    rc = SSH_ERROR;
    switch (session->next_crypto->kex_type) {
    case SSH_KEX_DH_GROUP1_SHA1:
    case SSH_KEX_DH_GROUP14_SHA1:
#ifdef WITH_GEX
    case SSH_KEX_DH_GEX_SHA1:
#endif /* WITH_GEX */
        session->next_crypto->digest_len = SHA_DIGEST_LENGTH;
        session->next_crypto->digest_type = SSH_KDF_SHA1;
        session->next_crypto->secret_hash = malloc(session->next_crypto->digest_len);
        if (session->next_crypto->secret_hash == NULL) {
            ssh_set_error_oom(session);
            goto error;
        }
        sha1_esp32_port(ssh_buffer_get(buf), ssh_buffer_get_len(buf),
                                   session->next_crypto->secret_hash);
        break;
    case SSH_KEX_DH_GROUP14_SHA256:
    case SSH_KEX_ECDH_SHA2_NISTP256:
    case SSH_KEX_CURVE25519_SHA256:
    case SSH_KEX_CURVE25519_SHA256_LIBSSH_ORG:
#ifdef WITH_GEX
    case SSH_KEX_DH_GEX_SHA256:
#endif /* WITH_GEX */
        session->next_crypto->digest_len = SHA256_DIGEST_LENGTH;
        session->next_crypto->digest_type = SSH_KDF_SHA256;
        session->next_crypto->secret_hash = malloc(session->next_crypto->digest_len);
        if (session->next_crypto->secret_hash == NULL) {
            ssh_set_error_oom(session);
            goto error;
        }
        sha256(ssh_buffer_get(buf), ssh_buffer_get_len(buf),
                                     session->next_crypto->secret_hash);
        break;
    case SSH_KEX_ECDH_SHA2_NISTP384:
        session->next_crypto->digest_len = SHA384_DIGEST_LENGTH;
        session->next_crypto->digest_type = SSH_KDF_SHA384;
        session->next_crypto->secret_hash = malloc(session->next_crypto->digest_len);
        if (session->next_crypto->secret_hash == NULL) {
            ssh_set_error_oom(session);
            goto error;
        }
        sha384(ssh_buffer_get(buf), ssh_buffer_get_len(buf),
                                     session->next_crypto->secret_hash);
        break;
    case SSH_KEX_DH_GROUP16_SHA512:
    case SSH_KEX_DH_GROUP18_SHA512:
    case SSH_KEX_ECDH_SHA2_NISTP521:
        session->next_crypto->digest_len = SHA512_DIGEST_LENGTH;
        session->next_crypto->digest_type = SSH_KDF_SHA512;
        session->next_crypto->secret_hash = malloc(session->next_crypto->digest_len);
        if (session->next_crypto->secret_hash == NULL) {
            ssh_set_error_oom(session);
            goto error;
        }
        sha512(ssh_buffer_get(buf),
               ssh_buffer_get_len(buf),
               session->next_crypto->secret_hash);
        break;
    }

    /* During the first kex, secret hash and session ID are equal. However, after
     * a key re-exchange, a new secret hash is calculated. This hash will not replace
     * but complement existing session id.
     */
    if (!session->next_crypto->session_id) {
        session->next_crypto->session_id = malloc(session->next_crypto->digest_len);
        if (session->next_crypto->session_id == NULL) {
            ssh_set_error_oom(session);
            rc = SSH_ERROR;
            goto error;
        }
        memcpy(session->next_crypto->session_id, session->next_crypto->secret_hash,
                session->next_crypto->digest_len);
	/* Initial length is the same as secret hash */
	session->next_crypto->session_id_len = session->next_crypto->digest_len;
    }
#ifdef DEBUG_CRYPTO
    SSH_LOG(SSH_LOG_DEBUG, "Session hash: \n");
    ssh_log_hexdump("secret hash", session->next_crypto->secret_hash, session->next_crypto->digest_len);
    ssh_log_hexdump("session id", session->next_crypto->session_id, session->next_crypto->session_id_len);
#endif /* DEBUG_CRYPTO */

    rc = SSH_OK;
error:
    SSH_BUFFER_FREE(buf);
    SSH_BUFFER_FREE(client_hash);
    SSH_BUFFER_FREE(server_hash);

    session->in_hashbuf = NULL;
    session->out_hashbuf = NULL;

    SSH_STRING_FREE(num);
#if defined(HAVE_LIBCRYPTO) && OPENSSL_VERSION_NUMBER >= 0x30000000L
    bignum_safe_free(client_pubkey);
    bignum_safe_free(server_pubkey);
#endif /* OPENSSL_VERSION_NUMBER */

    return rc;
}

int ssh_hashbufout_add_cookie(ssh_session session)
{
    int rc;

    session->out_hashbuf = ssh_buffer_new();
    if (session->out_hashbuf == NULL) {
        return -1;
    }

    rc = ssh_buffer_allocate_size(session->out_hashbuf,
            sizeof(uint8_t) + 16);
    if (rc < 0) {
        ssh_buffer_reinit(session->out_hashbuf);
        return -1;
    }

    if (ssh_buffer_add_u8(session->out_hashbuf, 20) < 0) {
        ssh_buffer_reinit(session->out_hashbuf);
        return -1;
    }

    if (session->server) {
        if (ssh_buffer_add_data(session->out_hashbuf,
                    session->next_crypto->server_kex.cookie, 16) < 0) {
            ssh_buffer_reinit(session->out_hashbuf);
            return -1;
        }
    } else {
        if (ssh_buffer_add_data(session->out_hashbuf,
                    session->next_crypto->client_kex.cookie, 16) < 0) {
            ssh_buffer_reinit(session->out_hashbuf);
            return -1;
        }
    }

    return 0;
}

int ssh_hashbufin_add_cookie(ssh_session session, unsigned char *cookie)
{
    int rc;

    session->in_hashbuf = ssh_buffer_new();
    if (session->in_hashbuf == NULL) {
        return -1;
    }

    rc = ssh_buffer_allocate_size(session->in_hashbuf,
            sizeof(uint8_t) + 20 + 16);
    if (rc < 0) {
        ssh_buffer_reinit(session->in_hashbuf);
        return -1;
    }

    if (ssh_buffer_add_u8(session->in_hashbuf, 20) < 0) {
        ssh_buffer_reinit(session->in_hashbuf);
        return -1;
    }
    if (ssh_buffer_add_data(session->in_hashbuf,cookie, 16) < 0) {
        ssh_buffer_reinit(session->in_hashbuf);
        return -1;
    }

    return 0;
}

int ssh_generate_session_keys(ssh_session session)
{
    ssh_string k_string = NULL;
    struct ssh_crypto_struct *crypto = session->next_crypto;
    unsigned char *key = NULL;
    unsigned char *IV_cli_to_srv = NULL;
    unsigned char *IV_srv_to_cli = NULL;
    unsigned char *enckey_cli_to_srv = NULL;
    unsigned char *enckey_srv_to_cli = NULL;
    unsigned char *intkey_cli_to_srv = NULL;
    unsigned char *intkey_srv_to_cli = NULL;
    size_t key_len = 0;
    size_t IV_len = 0;
    size_t enckey_cli_to_srv_len = 0;
    size_t enckey_srv_to_cli_len = 0;
    size_t intkey_cli_to_srv_len = 0;
    size_t intkey_srv_to_cli_len = 0;
    int rc = -1;

    k_string = ssh_make_bignum_string(crypto->shared_secret);
    if (k_string == NULL) {
        ssh_set_error_oom(session);
        goto error;
    }
    /* See RFC4251 Section 5 for the definition of mpint which is the
     * encoding we need to use for key in the SSH KDF */
    key = (unsigned char *)k_string;
    key_len = ssh_string_len(k_string) + 4;

    IV_len = crypto->digest_len;
    if (session->client) {
        enckey_cli_to_srv_len = crypto->out_cipher->keysize / 8;
        enckey_srv_to_cli_len = crypto->in_cipher->keysize / 8;
        intkey_cli_to_srv_len = hmac_digest_len(crypto->out_hmac);
        intkey_srv_to_cli_len = hmac_digest_len(crypto->in_hmac);
    } else {
        enckey_cli_to_srv_len = crypto->in_cipher->keysize / 8;
        enckey_srv_to_cli_len = crypto->out_cipher->keysize / 8;
        intkey_cli_to_srv_len = hmac_digest_len(crypto->in_hmac);
        intkey_srv_to_cli_len = hmac_digest_len(crypto->out_hmac);
    }

    IV_cli_to_srv = malloc(IV_len);
    IV_srv_to_cli = malloc(IV_len);
    enckey_cli_to_srv = malloc(enckey_cli_to_srv_len);
    enckey_srv_to_cli = malloc(enckey_srv_to_cli_len);
    intkey_cli_to_srv = malloc(intkey_cli_to_srv_len);
    intkey_srv_to_cli = malloc(intkey_srv_to_cli_len);
    if (IV_cli_to_srv == NULL || IV_srv_to_cli == NULL ||
        enckey_cli_to_srv == NULL || enckey_srv_to_cli == NULL ||
        intkey_cli_to_srv == NULL || intkey_srv_to_cli == NULL) {
        ssh_set_error_oom(session);
        goto error;
    }

    /* IV */
    rc = ssh_kdf(crypto, key, key_len, 'A', IV_cli_to_srv, IV_len);
    if (rc < 0) {
        goto error;
    }
    rc = ssh_kdf(crypto, key, key_len, 'B', IV_srv_to_cli, IV_len);
    if (rc < 0) {
        goto error;
    }
    /* Encryption Key */
    rc = ssh_kdf(crypto, key, key_len, 'C', enckey_cli_to_srv,
                 enckey_cli_to_srv_len);
    if (rc < 0) {
        goto error;
    }
    rc = ssh_kdf(crypto, key, key_len, 'D', enckey_srv_to_cli,
                 enckey_srv_to_cli_len);
    if (rc < 0) {
        goto error;
    }
    /* Integrity Key */
    rc = ssh_kdf(crypto, key, key_len, 'E', intkey_cli_to_srv,
                 intkey_cli_to_srv_len);
    if (rc < 0) {
        goto error;
    }
    rc = ssh_kdf(crypto, key, key_len, 'F', intkey_srv_to_cli,
                 intkey_srv_to_cli_len);
    if (rc < 0) {
        goto error;
    }

    if (session->client) {
        crypto->encryptIV = IV_cli_to_srv;
        crypto->decryptIV = IV_srv_to_cli;
        crypto->encryptkey = enckey_cli_to_srv;
        crypto->decryptkey = enckey_srv_to_cli;
        crypto->encryptMAC = intkey_cli_to_srv;
        crypto->decryptMAC = intkey_srv_to_cli;
    } else {
        crypto->encryptIV = IV_srv_to_cli;
        crypto->decryptIV = IV_cli_to_srv;
        crypto->encryptkey = enckey_srv_to_cli;
        crypto->decryptkey = enckey_cli_to_srv;
        crypto->encryptMAC = intkey_srv_to_cli;
        crypto->decryptMAC = intkey_cli_to_srv;
    }

#ifdef DEBUG_CRYPTO
    ssh_log_hexdump("Client to Server IV", IV_cli_to_srv, IV_len);
    ssh_log_hexdump("Server to Client IV", IV_srv_to_cli, IV_len);
    ssh_log_hexdump("Client to Server Encryption Key", enckey_cli_to_srv,
                   enckey_cli_to_srv_len);
    ssh_log_hexdump("Server to Client Encryption Key", enckey_srv_to_cli,
                   enckey_srv_to_cli_len);
    ssh_log_hexdump("Client to Server Integrity Key", intkey_cli_to_srv,
                   intkey_cli_to_srv_len);
    ssh_log_hexdump("Server to Client Integrity Key", intkey_srv_to_cli,
                   intkey_srv_to_cli_len);
#endif /* DEBUG_CRYPTO */

    rc = 0;
error:
    ssh_string_burn(k_string);
    SSH_STRING_FREE(k_string);
    if (rc != 0) {
        free(IV_cli_to_srv);
        free(IV_srv_to_cli);
        free(enckey_cli_to_srv);
        free(enckey_srv_to_cli);
        free(intkey_cli_to_srv);
        free(intkey_srv_to_cli);
    }

    return rc;
}
