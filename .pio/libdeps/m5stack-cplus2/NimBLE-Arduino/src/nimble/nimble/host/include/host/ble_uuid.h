/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_BLE_UUID_
#define H_BLE_UUID_

/**
 * @brief Bluetooth UUID
 * @defgroup bt_uuid Bluetooth UUID
 * @ingroup bt_host
 * @{
 */

#include <inttypes.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct os_mbuf;

/** Type of UUID */
enum {
    /** 16-bit UUID (BT SIG assigned) */
    BLE_UUID_TYPE_16 = 16,

    /** 32-bit UUID (BT SIG assigned) */
    BLE_UUID_TYPE_32 = 32,

    /** 128-bit UUID */
    BLE_UUID_TYPE_128 = 128,
};

/** Generic UUID type, to be used only as a pointer */
typedef struct {
    /** Type of the UUID */
    uint8_t type;
} ble_uuid_t;

/** 16-bit UUID */
typedef struct {
    /** Generic UUID structure */
    ble_uuid_t u;

    /** 16-bit UUID value */
    uint16_t value;
} ble_uuid16_t;

/** 32-bit UUID */
typedef struct {
    /** Generic UUID structure */
    ble_uuid_t u;

    /** 32-bit UUID value */
    uint32_t value;
} ble_uuid32_t;

/** 128-bit UUID */
typedef struct {
    /** Generic UUID structure */
    ble_uuid_t u;

    /** 128-bit UUID value  */
    uint8_t value[16];
} ble_uuid128_t;

/** Universal UUID type, to be used for any-UUID static allocation */
typedef union {
    /** Generic UUID structure */
    ble_uuid_t u;

    /** 16-bit UUID structure */
    ble_uuid16_t u16;

    /** 32-bit UUID structure */
    ble_uuid32_t u32;

    /** 128-bit UUID structure */
    ble_uuid128_t u128;
} ble_uuid_any_t;

/**
 * @brief Macro for initializing a 16-bit UUID.
 *
 * This macro initializes a 16-bit UUID with the provided value.
 *
 * @param uuid16    The value of the 16-bit UUID.
 *
 * @return          The initialized 16-bit UUID structure.
 */
#define BLE_UUID16_INIT(uuid16)         \
    {                                   \
        .u = {                          \
            .type = BLE_UUID_TYPE_16,   \
        },                              \
        .value = (uuid16),              \
    }

/**
 * @brief Macro for initializing a 32-bit UUID.
 *
 * This macro initializes a 32-bit UUID with the provided value.
 *
 * @param uuid32    The value of the 32-bit UUID.
 *
 * @return          The initialized 32-bit UUID structure.
 */
#define BLE_UUID32_INIT(uuid32)         \
    {                                   \
        .u = {                          \
            .type = BLE_UUID_TYPE_32,   \
        },                              \
        .value = (uuid32),              \
    }

#define BLE_UUID128_INIT(uuid128 ...)   \
    {                                   \
        .u = {                          \
            .type = BLE_UUID_TYPE_128,  \
        },                              \
        .value = { uuid128 },           \
    }

/**
 * @brief Macro for declaring a pointer to a 16-bit UUID structure initialized with a specific 16-bit UUID value.
 *
 * @param uuid16    The 16-bit UUID value to initialize the structure with.
 *
 * @return          Pointer to a `ble_uuid_t` structure.
 */
#define BLE_UUID16_DECLARE(uuid16) \
    ((ble_uuid_t *) (&(ble_uuid16_t) BLE_UUID16_INIT(uuid16)))

/**
 * @brief Macro for declaring a pointer to a 32-bit UUID structure initialized with a specific 32-bit UUID value.
 *
 * @param uuid32    The 32-bit UUID value to initialize the structure with.
 *
 * @return          Pointer to a `ble_uuid_t` structure.
 */
#define BLE_UUID32_DECLARE(uuid32) \
    ((ble_uuid_t *) (&(ble_uuid32_t) BLE_UUID32_INIT(uuid32)))

/**
 * @brief Macro for declaring a pointer to a 128-bit UUID structure initialized with specific 128-bit UUID values.
 *
 * @param uuid128   The 128-bit UUID value to initialize the structure with.
 *
 * @return          Pointer to a `ble_uuid_t` structure.
 */
#define BLE_UUID128_DECLARE(uuid128...) \
    ((ble_uuid_t *) (&(ble_uuid128_t) BLE_UUID128_INIT(uuid128)))

/**
 * @brief Macro for casting a pointer to a `ble_uuid_t` structure to a pointer to 16-bit UUID structure.
 *
 * @param u         Pointer to a `ble_uuid_t` structure.
 *
 * @return          Pointer to a `ble_uuid16_t` structure.
 */
#define BLE_UUID16(u) \
    ((ble_uuid16_t *) (u))

/**
 * @brief Macro for casting a pointer to a `ble_uuid_t` structure to a pointer to 32-bit UUID structure.
 *
 * @param u         Pointer to a `ble_uuid_t` structure.
 *
 * @return          Pointer to a `ble_uuid32_t` structure.
 */
#define BLE_UUID32(u) \
    ((ble_uuid32_t *) (u))

/**
 * @brief Macro for casting a pointer to a `ble_uuid_t` structure to a pointer to 128-bit UUID structure.
 *
 * @param u         Pointer to a `ble_uuid_t` structure.
 *
 * @return          Pointer to a `ble_uuid128_t` structure.
 */
#define BLE_UUID128(u) \
    ((ble_uuid128_t *) (u))

/** Size of buffer needed to store UUID as a string.
 *  Includes trailing \0.
 */
#define BLE_UUID_STR_LEN (37)

/** @brief Constructs a UUID object from a byte array.
 *
 * @param uuid  On success, this gets populated with the constructed UUID.
 * @param buf   The source buffer to parse.
 * @param len   The size of the buffer, in bytes.
 *
 * @return      0 on success, BLE_HS_EINVAL if the source buffer does not contain
 *              a valid UUID.
 */
int ble_uuid_init_from_buf(ble_uuid_any_t *uuid, const void *buf, size_t len);

/** @brief Compares two Bluetooth UUIDs.
 *
 * @param uuid1  The first UUID to compare.
 * @param uuid2  The second UUID to compare.
 *
 * @return       0 if the two UUIDs are equal, nonzero if the UUIDs differ.
 */
int ble_uuid_cmp(const ble_uuid_t *uuid1, const ble_uuid_t *uuid2);

/** @brief Copy Bluetooth UUID
 *
 * @param dst    Destination UUID.
 * @param src    Source UUID.
 */
void ble_uuid_copy(ble_uuid_any_t *dst, const ble_uuid_t *src);

/** @brief Converts the specified UUID to its string representation.
 *
 * Example string representations:
 *     o 16-bit:  0x1234
 *     o 32-bit:  0x12345678
 *     o 128-bit: 12345678-1234-1234-1234-123456789abc
 *
 * @param uuid   The source UUID to convert.
 * @param dst    The destination buffer.
 *
 * @return       A pointer to the supplied destination buffer.
 */
char *ble_uuid_to_str(const ble_uuid_t *uuid, char *dst);

/**
 * @brief Converts the specified UUID string to ble_uuid_any_t representation.
 *        If the UUID is recognised as Bluetooth SIG UUID, it will provide its
 *        32-bit or 16-bit representation.
 *
 * Example 128-bit string representations:
 *     o "12345678-1234-1234-1234-123456789abc"
 *     o "12345678123412341234123456789abc"
 *
 * @param uuid  Destination UUID.
 * @param str   The source string UUID.
 *
 * @return      0 on success,
 *              BLE_HS_EINVAL if the specified UUID string has wrong size or
 *              contains disallowed characters.
 */
int
ble_uuid_from_str(ble_uuid_any_t *uuid, const char *str);

/** @brief Converts the specified 16-bit UUID to a uint16_t.
 *
 * @param uuid   The source UUID to convert.
 *
 * @return       The converted integer on success, NULL if the specified UUID is
 *               not 16 bits.
 */
uint16_t ble_uuid_u16(const ble_uuid_t *uuid);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _BLE_HOST_UUID_H */
