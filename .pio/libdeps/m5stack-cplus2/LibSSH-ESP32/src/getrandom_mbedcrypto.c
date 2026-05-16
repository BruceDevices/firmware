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
#include "mbedcrypto-compat.h"

mbedtls_ctr_drbg_context ssh_mbedtls_ctr_drbg;

int
ssh_mbedtls_random(void *where, int len, int strong)
{
    int rc = 0;
    if (strong) {
        mbedtls_ctr_drbg_set_prediction_resistance(&ssh_mbedtls_ctr_drbg,
                                                   MBEDTLS_CTR_DRBG_PR_ON);
        rc = mbedtls_ctr_drbg_random(&ssh_mbedtls_ctr_drbg, where, len);
        mbedtls_ctr_drbg_set_prediction_resistance(&ssh_mbedtls_ctr_drbg,
                                                   MBEDTLS_CTR_DRBG_PR_OFF);
    } else {
        rc = mbedtls_ctr_drbg_random(&ssh_mbedtls_ctr_drbg, where, len);
    }

    return !rc;
}

int
ssh_get_random(void *where, int len, int strong)
{
    return ssh_mbedtls_random(where, len, strong);
}
