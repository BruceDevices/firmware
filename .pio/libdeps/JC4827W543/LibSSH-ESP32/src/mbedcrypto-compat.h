#ifndef MBEDCRYPTO_COMPAT_H
#define MBEDCRYPTO_COMPAT_H

/* mbedtls/version.h should be available for both v2 and v3
 * v3 defines the version inside build_info.h so if it isn't defined
 * in version.h we should have v3
 */
#include <mbedtls/version.h>
#include <mbedtls/cipher.h>
#ifdef MBEDTLS_VERSION_MAJOR
#if MBEDTLS_VERSION_MAJOR < 3

static inline size_t mbedtls_cipher_info_get_key_bitlen(
                         const mbedtls_cipher_info_t *info)
{
    if (info == NULL) {
        return 0;
    }
    return info->key_bitlen;
}

static inline size_t mbedtls_cipher_info_get_iv_size(
                         const mbedtls_cipher_info_t *info)
{
    if (info == NULL) {
        return 0;
    }
    return (size_t)info->iv_size;
}

#define MBEDTLS_PRIVATE(X) X
#endif /* MBEDTLS_VERSION_MAJOR < 3 */
#else  /* MBEDTLS_VERSION_MAJOR */
#include <mbedtls/build_info.h>
#if MBEDTLS_VERSION_MAJOR < 3
#define MBEDTLS_PRIVATE(X) X
#endif /* MBEDTLS_VERSION_MAJOR < 3 */
#endif /* MBEDTLS_VERSION_MAJOR */
#endif /* MBEDCRYPTO_COMPAT_H */
