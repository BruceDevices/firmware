/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Simple pattern matching, with '*' and '?' as wildcards.
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

/*
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "libssh_esp32_config.h"

#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>
#ifndef _WIN32
#include <lwip/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

/* for systems without IPv6 support matching should still work */
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#include "libssh/priv.h"

/**
 * @brief Compare a string with a pattern containing wildcards `*` and `?`
 *
 * This function is an iterative replacement for the previously recursive
 * implementation to avoid exponential complexity (DoS) with specific patterns.
 *
 * @param[in]  s        The string to match.
 * @param[in]  pattern  The pattern to match against.
 *
 * @return              1 if the pattern matches, 0 otherwise.
 */
static int match_pattern(const char *s, const char *pattern)
{
    const char *s_star = NULL; /* Position in s when last `*` was met */
    const char *p_star = NULL; /* Position in pattern after last `*` */

    if (s == NULL || pattern == NULL) {
        return 0;
    }

    while (*s) {
        /* Case 1: Exact match or '?' wildcard */
        if (*pattern == *s || *pattern == '?') {
            s++;
            pattern++;
            continue;
        }

        /* Case 2: '*' wildcard */
        if (*pattern == '*') {
            /* Record the position of the star and the current string position.
             * We optimistically assume * matches 0 characters first.
             */
            p_star = ++pattern;
            s_star = s;
            continue;
        }

        /* Case 3: Mismatch */
        if (p_star) {
            /* If we have seen a star previously, backtrack.
             * We restore the pattern to just after the star,
             * but advance the string position (consume one more char for the
             * star).
             * No need to backtrack to previous stars as any match of the last
             * star could be eaten the same way by the previous star.
             */
            pattern = p_star;
            s = ++s_star;
            continue;
        }

        /* Case 4: Mismatch and no star to backtrack to */
        return 0;
    }

    /* Handle trailing stars in the pattern
     * (e.g., pattern "abc*" matching "abc") */
    while (*pattern == '*') {
        pattern++;
    }

    /* If we reached the end of the pattern, it's a match */
    return (*pattern == '\0');
}

/*
 * Tries to match the string against the comma-separated sequence of subpatterns
 * (each possibly preceded by ! to indicate negation).
 * Returns -1 if negation matches, 1 if there is a positive match, 0 if there is
 * no match at all.
 */
int match_pattern_list(const char *string, const char *pattern,
    size_t len, int dolower) {
  char sub[1024];
  int negated;
  int got_positive;
  size_t i, subi;

  got_positive = 0;
  for (i = 0; i < len;) {
    /* Check if the subpattern is negated. */
    if (pattern[i] == '!') {
      negated = 1;
      i++;
    } else {
      negated = 0;
    }

    /*
     * Extract the subpattern up to a comma or end.  Convert the
     * subpattern to lowercase.
     */
    for (subi = 0;
        i < len && subi < sizeof(sub) - 1 && pattern[i] != ',';
        subi++, i++) {
      sub[subi] = dolower && isupper((unsigned char)pattern[i]) ?
        (char)tolower((unsigned char)pattern[i]) : pattern[i];
    }

    /* If subpattern too long, return failure (no match). */
    if (subi >= sizeof(sub) - 1) {
      return 0;
    }

    /* If the subpattern was terminated by a comma, skip the comma. */
    if (i < len && pattern[i] == ',') {
      i++;
    }

    /* Null-terminate the subpattern. */
    sub[subi] = '\0';

    /* Try to match the subpattern against the string. */
    if (match_pattern(string, sub)) {
      if (negated) {
        return -1;        /* Negative */
      } else {
        got_positive = 1; /* Positive */
      }
    }
  }

  /*
   * Return success if got a positive match.  If there was a negative
   * match, we have already returned -1 and never get here.
   */
  return got_positive;
}

/*
 * Tries to match the host name (which must be in all lowercase) against the
 * comma-separated sequence of subpatterns (each possibly preceded by ! to
 * indicate negation).
 * Returns -1 if negation matches, 1 if there is a positive match, 0 if there
 * is no match at all.
 */
int match_hostname(const char *host, const char *pattern, unsigned int len) {
  return match_pattern_list(host, pattern, len, 1);
}

#ifndef _WIN32
/**
 * @brief Tries to match the host IPv6 address against a given network address
 * with specified prefix length in CIDR notation.
 *
 * @param[in] host_addr     The host address to verify.
 *
 * @param[in] net_addr      The network id address against which the match is
 *                          being verified
 *
 * @param[in] bits          The prefix length
 *
 * @return 0 on a negative match.
 * @return 1 on a positive match.
 */
static int
cidr_match_6(struct in6_addr *host_addr,
             struct in6_addr *net_addr,
             unsigned int bits)
{
    const uint8_t *a = host_addr->s6_addr;
    const uint8_t *b = net_addr->s6_addr;

    unsigned int byte_whole, bits_left;

    /* The number of a complete byte covered by the prefix */
    byte_whole = bits / 8;

    /*
     * The number of bits remaining in the incomplete (last) byte
     * covered by the prefix
     */
    bits_left = bits % 8;

    if (byte_whole) {
        if (memcmp(a, b, byte_whole) != 0) {
            return 0;
        }
    }

    if (bits_left) {
        if ((a[byte_whole] ^ b[byte_whole]) & (0xFFu << (8 - bits_left))) {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief Tries to match the host IPv4 address against a given network address
 * with specified prefix length in CIDR notation.
 *
 * @param[in] host_addr     The host address to verify.
 *
 * @param[in] net_addr      The network id address against which the match is
 *                          being verified
 *
 * @param[in] bits          The prefix length
 *
 * @return 0 on a negative match.
 * @return 1 on a positive match.
 */
static int
cidr_match_4(struct in_addr *host_addr,
             struct in_addr *net_addr,
             unsigned int bits)
{
    if (bits == 0) {
        /* C99 6.5.7 (3): u32 << 32 is undefined behaviour */
        return 1;
    }

    return !((host_addr->s_addr ^ net_addr->s_addr) &
             htonl((0xFFFFFFFFu << (32 - bits)) & 0xFFFFFFFFu));
}

/**
 * @brief Checks if the mask length is valid according to the address family
 * (IPv4 or IPv6).
 *
 * @param[in] family    The address family (e.g. AF_INET or AF_INET6)
 *
 * @param[in] mask      The subnet mask (prefix)
 *
 * @return true if the mask length does not exceed the maximum valid length
 * according to the address family (IPv4 or IPv6).
 * @return false if the mask length exceeds the maximum valid length
 * or there is no match with IPv4 or IPv6 address family.
 */
static bool
masklen_valid(int family, unsigned int mask)
{
    switch (family) {
    case AF_INET:
        return mask <= 32;
    case AF_INET6:
        return mask <= 128;
    default:
        return false;
    }
}

/**
 * @brief Extracts address family given a network address.
 *
 * @param[in] address   The network address.
 *
 * @return The value of the address family if no errors.
 * @return -1 in case of errors.
 */
static int
get_address_family(const char *address)
{
    struct addrinfo hints, *ai = NULL;
    int rc = -1, rv;

    ZERO_STRUCT(hints);
    if (address == NULL) {
        SSH_LOG(SSH_LOG_TRACE, "Bad arguments");
        goto out;
    }

    hints.ai_flags = AI_NUMERICHOST;
    rv = getaddrinfo(address, NULL, &hints, &ai);
    if (rv != 0) {
        SSH_LOG(SSH_LOG_TRACE,
                "Couldn't get address information - getaddrinfo() failed: %d",
                rv);
        goto out;
    }

    rc = ai->ai_family;
    freeaddrinfo(ai);

out:
    return rc;
}

/**
 * @brief Tries to match the host address against a CIDR list provided
 * by the user. If the host address family is unknown, it can be derived by
 * passing -1 as sa_family argument.
 *
 * It can be also used to validate a CIDR list when the passed address is NULL
 * and sa_family is -1.
 *
 * @param[in] address   The host address to verify (NULL to validate CIDR list).
 *
 * @param[in] addrlist  The CIDR list against which the match is being verified.
 *                      The CIDR list can contain both IPv4 and IPv6 addresses
 *                      and has to be comma separated
 *                      (',' only, space after comma not allowed).
 *
 * @param[in] sa_family The socket address family (e.g. AF_INET or AF_INET6,
 *                      -1 to validate CIDR list or unknown address family).
 *
 * @usage To validate CIDR list: match_cidr_address_list(NULL, addrlist, -1).
 * @usage To verify a match with unknown address family:
 *        match_cidr_address_list(address, addrlist, -1).
 * @return  1 only on positive match.
 * @return  0 on negative match or valid CIDR list.
 * @return  -1 on errors or invalid CIDR list.
 */
int
match_cidr_address_list(const char *address,
                        const char *addrlist,
                        int sa_family)
{
    char *list = NULL, *cp = NULL, *a = NULL, *b = NULL, *sp = NULL;
    char addr_buffer[64], addr[NI_MAXHOST];
    struct in_addr try_addr, match_addr;
    struct in6_addr try_addr6, match_addr6;
    unsigned long mask_len;
    size_t addr_len, tmp_len;
    int rc = 0, r, ai_family;

    ZERO_STRUCT(try_addr);
    ZERO_STRUCT(try_addr6);
    ZERO_STRUCT(match_addr);
    ZERO_STRUCT(match_addr6);

    if (sa_family != AF_INET && sa_family != AF_INET6 && sa_family != -1) {
        SSH_LOG(SSH_LOG_TRACE,
                "Invalid argument: sa_family %d is not valid",
                sa_family);
        return -1;
    }

    if (address != NULL) {
        strncpy(addr, address, NI_MAXHOST - 1);

        /* Remove interface in case of IPv6 address: addr%interface */
        a = strchr(addr, '%');
        if (a != NULL) {
            *a = '\0';
        }

        /*
         * If sa_family is set to -1 and address is not NULL then
         * the socket address family should be derived
         */
        if (sa_family == -1) {
            r = get_address_family(addr);
            if (r == -1) {
                SSH_LOG(SSH_LOG_TRACE,
                        "Failed to derive address family for address "
                        "\"%.100s\"",
                        addr);
                return -1;
            }
            sa_family = r;
        }

        /*
         * Translate host address from dot notation to binary network format
         * according to family type,
         * i.e. IPv4 (store in in_addr) or IPv6 (store in in6_addr)
         */
        if (sa_family == AF_INET) {
            if (inet_pton(AF_INET, addr, &try_addr) == 0) {
                SSH_LOG(SSH_LOG_TRACE,
                        "Couldn't parse IPv4 address \"%.100s\"",
                        addr);
                return -1;
            }
        } else if (sa_family == AF_INET6) {
            if (inet_pton(AF_INET6, addr, &try_addr6) == 0) {
                SSH_LOG(SSH_LOG_TRACE,
                        "Couldn't parse IPv6 address \"%.100s\"",
                        addr);
                return -1;
            }
        } else {
            SSH_LOG(SSH_LOG_TRACE,
                    "Address family %d for address \"%.100s\" "
                    "is not recognized",
                    sa_family,
                    addr);
            return -1;
        }
    }

    b = list = strdup(addrlist);
    if (b == NULL) {
        return -1;
    }

    while ((cp = strsep(&list, ",")) != NULL) {
        if (*cp == '\0') {
            SSH_LOG(SSH_LOG_TRACE, "Empty entry in list \"%.100s\"", b);
            rc = -1;
            break;
        }

        /*
         * Stop junk from reaching address translation. +3 for the "/prefix".
         * INET6_ADDRSTRLEN is 46 and includes space for '\0' terminator. The
         * maximum IPv6 address printable is the one that carries IPv4 too.
         * E.g. ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255 is 46 chars
         * long ('\0' included) and the maximum prefix length possible is 96.
         * This explains why +3. All the other IPv6 addresses with maximum /127
         * prefix length (39 + 4) are covered just by INET6_ADDRSTRLEN itself
         */
        addr_len = strlen(cp);
        if (addr_len > INET6_ADDRSTRLEN + 3) {
            SSH_LOG(SSH_LOG_TRACE,
                    "List entry \"%.100s\" too long: %zu > %d (MAX ALLOWED)",
                    cp,
                    addr_len,
                    INET6_ADDRSTRLEN + 3);
            rc = -1;
            break;
        }

#define VALID_CIDR_CHARS "0123456789abcdefABCDEF.:/"
        tmp_len = strspn(cp, VALID_CIDR_CHARS);
        if (tmp_len != addr_len) {
            SSH_LOG(SSH_LOG_TRACE,
                    "List entry \"%.100s\" contains invalid characters "
                    "-> \"%c\" is an invalid character",
                    cp,
                    cp[tmp_len]);
            rc = -1;
            break;
        }
#undef VALID_CIDR_CHARS

        strncpy(addr_buffer, cp, sizeof(addr_buffer) - 1);
        sp = strchr(addr_buffer, '/');
        if (sp != NULL) {
            *sp = '\0';
            sp++;
            mask_len = strtoul(sp, &cp, 10);
            if (*sp < '0' || *sp > '9' || *cp != '\0') {
                SSH_LOG(SSH_LOG_TRACE, "Error while parsing prefix: %s", sp);
                rc = -1;
                break;
            }
            if (mask_len > 128) {
                SSH_LOG(SSH_LOG_TRACE,
                        "Invalid prefix: %lu exceeds the maximum allowed "
                        "(>128)",
                        mask_len);
                rc = -1;
                break;
            }
        } else {
            SSH_LOG(SSH_LOG_TRACE,
                    "Missing prefix length for list entry \"%.100s\"",
                    addr_buffer);
            rc = -1;
            break;
        }

        ai_family = get_address_family(addr_buffer);
        if (ai_family == -1) {
            SSH_LOG(SSH_LOG_TRACE,
                    "Couldn't get address family for \"%.100s\"",
                    addr_buffer);
            rc = -1;
            break;
        }

        if (ai_family == AF_INET) {
            if (inet_pton(AF_INET, addr_buffer, &match_addr) == 0) {
                SSH_LOG(SSH_LOG_TRACE,
                        "Couldn't parse IPv4 address \"%.100s\"",
                        addr_buffer);
                rc = -1;
                break;
            }
        } else if (ai_family == AF_INET6) {
            if (inet_pton(AF_INET6, addr_buffer, &match_addr6) == 0) {
                SSH_LOG(SSH_LOG_TRACE,
                        "Couldn't parse IPv6 address \"%.100s\"",
                        addr_buffer);
                rc = -1;
                break;
            }
        } else {
            SSH_LOG(SSH_LOG_TRACE,
                    "Address family %d for address \"%.100s\" "
                    "is not recognized",
                    ai_family,
                    addr_buffer);
            rc = -1;
            break;
        }

        if (masklen_valid(ai_family, mask_len) != true) {
            SSH_LOG(SSH_LOG_TRACE,
                    "Invalid mask length %lu for list entry \"%.100s\"",
                    mask_len,
                    addr_buffer);
            rc = -1;
            break;
        }

        /* Verify match between host address and network address*/
        if (((ai_family == AF_INET && sa_family == AF_INET) &&
             cidr_match_4(&try_addr, &match_addr, mask_len)) ||
            ((ai_family == AF_INET6 && sa_family == AF_INET6) &&
             cidr_match_6(&try_addr6, &match_addr6, mask_len))) {
            rc = 1;
            break;
        }
    }
    SAFE_FREE(b);

    return rc;
}
#endif /* _WIN32 */

/**
 * @brief Tries to match an object against a comma separated group of objects
 *
 * The characters '*' and '?' are NOT considered wildcards and an object in the
 * group preceded by a ! does NOT indicate negation. The characters '*', '?'
 * and '!' are treated normally like other characters, only ',' (comma) is
 * treated specially and is considered as a delimiter that separates objects in
 * the group.
 *
 * @param[in] group     Group of objects (comma separated) to match against.
 *
 * @param[in] object    Object to match.
 *
 * @returns             1 if there is a match, 0 if there is no match at all.
 */
int match_group(const char *group, const char *object)
{
    const char *a = NULL;
    const char *z = NULL;

    if (group == NULL || object == NULL) {
        return 0;
    }

    z = group;
    do {
        a = strchr(z, ',');
        if (a == NULL) {
            if (strcmp(z, object) == 0) {
                return 1;
            }
            return 0;
        } else {
            if (strncmp(z, object, a - z) == 0) {
                return 1;
            }
        }
        z = a + 1;
    } while (1);

    /* not reached */
    return 0;
}
