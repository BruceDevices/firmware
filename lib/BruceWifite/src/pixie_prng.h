/*
 * pixiewps: offline WPS brute-force utility that exploits low entropy PRNGs
 *
 * Copyright (c) 2015-2017, wiire <wi7ire@gmail.com>
 * SPDX-License-Identifier: GPL-3.0+
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */


#pragma once
#include <stdint.h>
struct ralink_randstate {
	uint32_t sreg;
};

static unsigned char ralink_randbyte(struct ralink_randstate *state)
{
	unsigned char r = 0;
	for (int i = 0; i < 8; i++) {
#if defined(__mips__) || defined(__mips)
		const uint32_t lsb_mask = -(state->sreg & 1);
		state->sreg ^= lsb_mask & 0x80000057;
		state->sreg >>= 1;
		state->sreg |= lsb_mask & 0x80000000;
		r = (r << 1) | (lsb_mask & 1);
#else
		unsigned char result;
		if (state->sreg & 0x00000001) {
			state->sreg = ((state->sreg ^ 0x80000057) >> 1) | 0x80000000;
			result = 1;
		}
		else {
			state->sreg = state->sreg >> 1;
			result = 0;
		}
		r = (r << 1) | result;
#endif
	}
	return r;
}

static void ralink_randstate_restore(struct ralink_randstate *state, uint8_t r)
{
	for (int i = 0; i < 8; i++) {
		const unsigned char result = r & 1;
		r = r >> 1;
		if (result) {
			state->sreg = (((state->sreg) << 1) ^ 0x80000057) | 0x00000001;
		}
		else {
			state->sreg = state->sreg << 1;
		}
	}
}

static unsigned char ralink_randbyte_backwards(struct ralink_randstate *state)
{
	unsigned char r = 0;
	for (int i = 0; i < 8; i++) {
		unsigned char result;
		if (state->sreg & 0x80000000) {
			state->sreg = ((state->sreg << 1) ^ 0x80000057) | 0x00000001;
			result = 1;
		}
		else {
			state->sreg = state->sreg <<  1;
			result = 0;
		}
		r |= result << i;
	}
	return r;
}

/* Simplest */
static uint32_t ecos_rand_simplest(uint32_t *seed)
{
	*seed = (*seed * 1103515245) + 12345; /* Permutate seed */
	return *seed;
}

/* Simple, Linear congruential generator */
static uint32_t ecos_rand_simple(uint32_t *seed)
{
	uint32_t s = *seed;
	uint32_t uret;

	s = (s * 1103515245) + 12345;          /* Permutate seed */
	uret = s & 0xffe00000;                 /* Use top 11 bits */
	s = (s * 1103515245) + 12345;          /* Permutate seed */
	uret += (s & 0xfffc0000) >> 11;        /* Use top 14 bits */
	s = (s * 1103515245) + 12345;          /* Permutate seed */
	uret += (s & 0xfe000000) >> (11 + 14); /* Use top 7 bits */

	*seed = s;
	return uret;
}

/* Mersenne-Knuth */
static uint32_t ecos_rand_knuth(uint32_t *seed)
{
	#define MM 2147483647 /* Mersenne prime */
	#define AA 48271      /* This does well in the spectral test */
	#define QQ 44488      /* MM / AA */
	#define RR 3399       /* MM % AA, important that RR < QQ */

	*seed = AA * (*seed % QQ) - RR * (*seed / QQ);
	if (*seed & 0x80000000)
		*seed += MM;

	return *seed;
}

/*
 * Based on the code of user @1yura.
 *
 * See glibc_random.c for a better understanding of the code.
 */

#include <stdint.h>

static const uint32_t glibc_seed_tbl[31 + 3] = {
		0x0128e83b, 0x00dafa31, 0x009f4828, 0x00f66443, 0x00bee24d, 0x00817005, 0x00cb918f,
		0x00a64845, 0x0069c3cf, 0x00a76dbd, 0x0090a848, 0x0057025f, 0x0089126c, 0x007d9a8f,
		0x0048252a, 0x006fb2d4, 0x006ccc15, 0x003c5744, 0x005a998f, 0x005df917, 0x0032ed77,
		0x00492688, 0x0050e901, 0x002b5f57, 0x003acd0b, 0x00456b7a, 0x0025413d, 0x002f11f4,
		0x003b564d, 0x00203f14, 0x002589fc, 0x003283f8, 0x001c17e4, 0x001dd823
};

static inline uint32_t *glibc_fast_nonce(uint32_t seed, uint32_t *dest)
{
	uint32_t word0 = 0, word1 = 0, word2 = 0, word3 = 0;

#ifdef PWPS_UNERRING
	if      (seed == 0x7fffffff) seed = 0x13f835f3;
	else if (seed == 0xfffffffe) seed = 0x5df735f1;
#endif

	for (int j = 0; j < 31; j++) {
		word0 += seed * glibc_seed_tbl[j + 3];
		word1 += seed * glibc_seed_tbl[j + 2];
		word2 += seed * glibc_seed_tbl[j + 1];
		word3 += seed * glibc_seed_tbl[j + 0];

		/* This does: seed = (16807LL * seed) % 0x7fffffff
		   using the sum of digits method which works for mod N, base N+1 */
		uint64_t p = 16807ULL * seed;
		p = (p >> 31) + (p & 0x7fffffff);
		seed = (p >> 31) + (p & 0x7fffffff);
#if 0 /* Same as PWPS_UNERRING */
		if (seed == 0x7fffffff) seed = 0;
#endif
	}
	dest[0] = word0 >> 1;
	dest[1] = word1 >> 1;
	dest[2] = word2 >> 1;
	dest[3] = word3 >> 1;
	return dest;
}

static inline uint32_t glibc_fast_seed(uint32_t seed)
{
	uint32_t word0 = 0;

#ifdef PWPS_UNERRING
	if      (seed == 0x7fffffff) seed = 0x13f835f3;
	else if (seed == 0xfffffffe) seed = 0x5df735f1;
#endif

	for (int j = 3; j < 31 + 3 - 1; j++) {
		word0 += seed * glibc_seed_tbl[j];

		/* This does: seed = (16807LL * seed) % 0x7fffffff
		   using the sum of digits method which works for mod N, base N+1 */
		uint64_t p = 16807ULL * seed;
		p = (p >> 31) + (p & 0x7fffffff);
		seed = (p >> 31) + (p & 0x7fffffff);
#if 0 /* Same as PWPS_UNERRING */
		if (seed == 0x7fffffff) seed = 0;
#endif
	}
	return (word0 + seed * glibc_seed_tbl[33]) >> 1;
}
