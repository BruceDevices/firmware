#ifndef PORT_ESP32_CONFIG_H_
#define PORT_ESP32_CONFIG_H_

// libssh-build-local/config.h

/* Name of package */
#define PACKAGE "libssh"

/* Version number of package */
#define VERSION "0.11.4"

#define SYSCONFDIR "etc"

/* Global configuration directory */
/* #undef USR_GLOBAL_CONF_DIR */
#define GLOBAL_CONF_DIR "/etc/ssh"

/* Global bind configuration file path */
#define GLOBAL_BIND_CONFIG "/etc/ssh/libssh_server_config"

/* Global client configuration file path */
#define GLOBAL_CLIENT_CONFIG "/etc/ssh/ssh_config"

/************************** HEADER FILES *************************/

/* Define to 1 if you have the <argp.h> header file. */
#define HAVE_ARGP_H 1

/* Define to 1 if you have the <aprpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the <glob.h> header file. */
#define HAVE_GLOB_H 1

/* Define to 1 if you have the <valgrind/valgrind.h> header file. */
/* #undef HAVE_VALGRIND_VALGRIND_H */

/* Define to 1 if you have the <pty.h> header file. */
#define HAVE_PTY_H 1

/* Define to 1 if you have the <utmp.h> header file. */
#define HAVE_UTMP_H 1

/* Define to 1 if you have the <util.h> header file. */
/* #undef HAVE_UTIL_H */

/* Define to 1 if you have the <libutil.h> header file. */
/* #undef HAVE_LIBUTIL_H */

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/utime.h> header file. */
/* #undef HAVE_SYS_UTIME_H */

/* Define to 1 if you have the <io.h> header file. */
/* #undef HAVE_IO_H */

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <ifaddrs.h> header file. */
// #undef HAVE_IFADDRS_H

/* Define to 1 if you have the <openssl/aes.h> header file. */
/* #undef HAVE_OPENSSL_AES_H */

/* Define to 1 if you have the <wspiapi.h> header file. */
/* #undef HAVE_WSPIAPI_H */

/* Define to 1 if you have the <openssl/des.h> header file. */
/* #undef HAVE_OPENSSL_DES_H */

/* Define to 1 if you have the <openssl/ecdh.h> header file. */
/* #undef HAVE_OPENSSL_ECDH_H */

/* Define to 1 if you have the <openssl/ec.h> header file. */
/* #undef HAVE_OPENSSL_EC_H */

/* Define to 1 if you have the <openssl/ecdsa.h> header file. */
/* #undef HAVE_OPENSSL_ECDSA_H */

/* Define to 1 if you have the <pthread.h> header file. */
/* #undef HAVE_PTHREAD_H */

/* Define to 1 if you have elliptic curve cryptography in openssl */
/* #undef HAVE_OPENSSL_ECC */

/* Define to 1 if you have elliptic curve cryptography in gcrypt */
/* #undef HAVE_GCRYPT_ECC */

/* Define to 1 if you have elliptic curve cryptography */
#define HAVE_ECC 1

/* Define to 1 if you have gl_flags as a glob_t struct member */
#define HAVE_GLOB_GL_FLAGS_MEMBER 1

/* Define to 1 if you have gcrypt with ChaCha20/Poly1305 support */
/* #undef HAVE_GCRYPT_CHACHA_POLY */

/*************************** FUNCTIONS ***************************/

/* Define to 1 if you have the `EVP_chacha20' function. */
/* #undef HAVE_OPENSSL_EVP_CHACHA20 */

/* Define to 1 if you have the `EVP_KDF_CTX_new_id' or `EVP_KDF_CTX_new` function. */
/* #undef HAVE_OPENSSL_EVP_KDF_CTX */

/* Define to 1 if you have the `FIPS_mode' function. */
/* #undef HAVE_OPENSSL_FIPS_MODE */

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the `_snprintf' function. */
/* #undef HAVE__SNPRINTF */

/* Define to 1 if you have the `_snprintf_s' function. */
/* #undef HAVE__SNPRINTF_S */

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `_vsnprintf' function. */
/* #undef HAVE__VSNPRINTF */

/* Define to 1 if you have the `_vsnprintf_s' function. */
/* #undef HAVE__VSNPRINTF_S */

/* Define to 1 if you have the `isblank' function. */
#define HAVE_ISBLANK 1

/* Define to 1 if you have the `strncpy' function. */
#define HAVE_STRNCPY 1

/* Define to 1 if you have the `strndup' function. */
#define HAVE_STRNDUP 1

/* Define to 1 if you have the `cfmakeraw' function. */
#define HAVE_CFMAKERAW 1

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the `poll' function. */
// #undef HAVE_POLL

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `clock_gettime' function. */
// #undef HAVE_CLOCK_GETTIME

/* Define to 1 if you have the `ntohll' function. */
/* #undef HAVE_NTOHLL */

/* Define to 1 if you have the `htonll' function. */
/* #undef HAVE_HTONLL */

/* Define to 1 if you have the `strtoull' function. */
// #define HAVE_STRTOULL 1

/* Define to 1 if you have the `__strtoull' function. */
/* #undef HAVE___STRTOULL */

/* Define to 1 if you have the `_strtoui64' function. */
/* #undef HAVE__STRTOUI64 */

/* Define to 1 if you have the `glob' function. */
// #undef HAVE_GLOB

/* Define to 1 if you have the `explicit_bzero' function. */
// #undef HAVE_EXPLICIT_BZERO

/* Define to 1 if you have the `memset_s' function. */
/* #undef HAVE_MEMSET_S */

/* Define to 1 if you have the `SecureZeroMemory' function. */
/* #undef HAVE_SECURE_ZERO_MEMORY */

/* Define to 1 if you have the `cmocka_set_test_filter' function. */
/* #undef HAVE_CMOCKA_SET_TEST_FILTER */

/* Define to 1 if we have support for blowfish */
/* #undef HAVE_BLOWFISH */

/*************************** LIBRARIES ***************************/

/* Define to 1 if you have the `crypto' library (-lcrypto). */
/* #undef HAVE_LIBCRYPTO */

/* Define to 1 if you have the `gcrypt' library (-lgcrypt). */
/* #undef HAVE_LIBGCRYPT */

/* Define to 1 if you have the 'mbedTLS' library (-lmbedtls). */
#define HAVE_LIBMBEDCRYPTO 1

/* Define to 1 if you have the `pthread' library (-lpthread). */
// #undef HAVE_PTHREAD

/* Define to 1 if you have the `cmocka' library (-lcmocka). */
/* #undef HAVE_CMOCKA */

/**************************** OPTIONS ****************************/

#define HAVE_GCC_THREAD_LOCAL_STORAGE 1
/* #undef HAVE_MSC_THREAD_LOCAL_STORAGE */

// #undef HAVE_FALLTHROUGH_ATTRIBUTE
#define HAVE_UNUSED_ATTRIBUTE 1
#define HAVE_WEAK_ATTRIBUTE 1

#define HAVE_CONSTRUCTOR_ATTRIBUTE 1
#define HAVE_DESTRUCTOR_ATTRIBUTE 1

#define HAVE_GCC_VOLATILE_MEMORY_PROTECTION 1

// #define HAVE_COMPILER__FUNC__ 1
#define HAVE_COMPILER__FUNCTION__ 1

/* #undef HAVE_GCC_BOUNDED_ATTRIBUTE */

/* Define to 1 if you want to enable GSSAPI */
/* #undef WITH_GSSAPI */

/* Define to 1 if you want to enable ZLIB */
/* #undef WITH_ZLIB */

/* Define to 1 if you want to enable SFTP */
/* #undef WITH_SFTP */

/* Define to 1 if you want to enable server support */
#define WITH_SERVER 1

/* Define to 1 if you want to enable DH group exchange algorithms */
#define WITH_GEX 1

/* Define to 1 if you want to enable insecure none cipher and MAC */
/* #undef WITH_INSECURE_NONE */

/* Define to 1 if you want to allow libssh to execute arbitrary commands from
 * configuration files or options (match exec, proxy commands and OpenSSH-based
 * proxy-jumps). */
/* #undef WITH_EXEC */

/* Define to 1 if you want to enable blowfish cipher support */
/* #undef WITH_BLOWFISH_CIPHER */

/* Define to 1 if you want to enable debug output for crypto functions */
/* #undef DEBUG_CRYPTO */

/* Define to 1 if you want to enable debug output for packet functions */
/* #undef DEBUG_PACKET */

/* Define to 1 if you want to enable pcap output support (experimental) */
/* #undef WITH_PCAP */

/* Define to 1 if you want to enable calltrace debug output */
#define DEBUG_CALLTRACE 1

/* Define to 1 if you want to enable NaCl support */
/* #undef WITH_NACL */

/* Define to 1 if you want to enable PKCS #11 URI support */
/* #undef WITH_PKCS11_URI */

/* Define to 1 if we want to build a support for PKCS #11 provider. */
/* #undef WITH_PKCS11_PROVIDER */

/*************************** ENDIAN *****************************/

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
/* #undef WORDS_BIGENDIAN */

#ifndef HAVE_STRTOULL
#define HAVE_STRTOULL 1
#endif

#ifndef HAVE_COMPILER__FUNC__
#define HAVE_COMPILER__FUNC__ 1
#endif 

// libssh-src-upstream/include/libssh/config.h

/*
 * config.h - parse the ssh config file
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009-2018    by Andreas Schneider <asn@cryptomilk.org>
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

#ifndef LIBSSH_CONFIG_H_
#define LIBSSH_CONFIG_H_


enum ssh_config_opcode_e {
    /* Unknown opcode */
    SOC_UNKNOWN = -3,
    /* Known and not applicable to libssh */
    SOC_NA = -2,
    /* Known but not supported by current libssh version */
    SOC_UNSUPPORTED = -1,
    SOC_HOST,
    SOC_MATCH,
    SOC_HOSTNAME,
    SOC_PORT,
    SOC_USERNAME,
    SOC_IDENTITY,
    SOC_CIPHERS,
    SOC_MACS,
    SOC_COMPRESSION,
    SOC_TIMEOUT,
    SOC_STRICTHOSTKEYCHECK,
    SOC_KNOWNHOSTS,
    SOC_PROXYCOMMAND,
    SOC_PROXYJUMP,
    SOC_GSSAPISERVERIDENTITY,
    SOC_GSSAPICLIENTIDENTITY,
    SOC_GSSAPIDELEGATECREDENTIALS,
    SOC_INCLUDE,
    SOC_BINDADDRESS,
    SOC_GLOBALKNOWNHOSTSFILE,
    SOC_LOGLEVEL,
    SOC_HOSTKEYALGORITHMS,
    SOC_KEXALGORITHMS,
    SOC_GSSAPIAUTHENTICATION,
    SOC_KBDINTERACTIVEAUTHENTICATION,
    SOC_PASSWORDAUTHENTICATION,
    SOC_PUBKEYAUTHENTICATION,
    SOC_PUBKEYACCEPTEDKEYTYPES,
    SOC_REKEYLIMIT,
    SOC_IDENTITYAGENT,
    SOC_IDENTITIESONLY,
    SOC_CONTROLMASTER,
    SOC_CONTROLPATH,
    SOC_CERTIFICATE,

    SOC_MAX /* Keep this one last in the list */
};
#endif /* LIBSSH_CONFIG_H_ */

// Local port additions

#define NI_MAXHOST 1025

#endif /* PORT_ESP32_CONFIG_H_ */
