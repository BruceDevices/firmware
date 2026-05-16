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

#include <stdint.h>
#include <nimble/porting/nimble/include/syscfg/syscfg.h>
#include <nimble/porting/nimble/include/sysinit/sysinit.h>
#include <nimble/porting/nimble/include/os/os_mbuf.h>
#include <nimble/porting/nimble/include/os/os_mempool.h>
#include <nimble/nimble/include/nimble/ble.h>
#include <nimble/nimble/include/nimble/hci_common.h>
#include <nimble/nimble/transport/include/nimble/transport.h>
#if BLE_TRANSPORT_IPC
#include <nimble/nimble/transport/common/hci_ipc/include/nimble/transport/hci_ipc.h>
#endif
#ifdef ESP_PLATFORM
#include "nimble/esp_port/port/include/esp_nimble_mem.h"
#endif

int os_msys_buf_alloc(void);
void os_msys_buf_free(void);

#define OMP_FLAG_FROM_HS        (0x01)
#define OMP_FLAG_FROM_LL        (0x02)
#define OMP_FLAG_FROM_MASK      (0x03)

#if MYNEWT_VAL(BLE_HS_FLOW_CTRL) || \
    MYNEWT_VAL(BLE_LL_CFG_FEAT_CTRL_TO_HOST_FLOW_CONTROL) || \
    (!MYNEWT_VAL(BLE_HOST) && BLE_TRANSPORT_IPC_ON_HS)
#define POOL_CMD_COUNT      (2)
#else
#define POOL_CMD_COUNT      (1)
#endif
#define POOL_CMD_SIZE       (258)

#define POOL_EVT_COUNT      (MYNEWT_VAL(BLE_TRANSPORT_EVT_COUNT))
#define POOL_EVT_LO_COUNT   (MYNEWT_VAL(BLE_TRANSPORT_EVT_DISCARDABLE_COUNT))
#define POOL_EVT_SIZE       (MYNEWT_VAL(BLE_TRANSPORT_EVT_SIZE))

#if MYNEWT_VAL_CHOICE(BLE_TRANSPORT_LL, native) && \
   MYNEWT_VAL_CHOICE(BLE_TRANSPORT_HS, native)
#define POOL_ACL_COUNT      (0)
#define POOL_ISO_COUNT      (0)
#elif !MYNEWT_VAL_CHOICE(BLE_TRANSPORT_LL, native) && \
      !MYNEWT_VAL_CHOICE(BLE_TRANSPORT_HS, native)
#define POOL_ACL_COUNT      ((MYNEWT_VAL(BLE_TRANSPORT_ACL_FROM_HS_COUNT)) + \
                             (MYNEWT_VAL(BLE_TRANSPORT_ACL_FROM_LL_COUNT)))
#define POOL_ISO_COUNT      ((MYNEWT_VAL(BLE_TRANSPORT_ISO_FROM_HS_COUNT)) + \
                             (MYNEWT_VAL(BLE_TRANSPORT_ISO_FROM_LL_COUNT)))
#elif MYNEWT_VAL_CHOICE(BLE_TRANSPORT_LL, native)
#define POOL_ACL_COUNT      (MYNEWT_VAL(BLE_TRANSPORT_ACL_FROM_HS_COUNT))
#define POOL_ISO_COUNT      (MYNEWT_VAL(BLE_TRANSPORT_ISO_FROM_HS_COUNT))
#else
#define POOL_ACL_COUNT      (MYNEWT_VAL(BLE_TRANSPORT_ACL_FROM_LL_COUNT))
#define POOL_ISO_COUNT      (MYNEWT_VAL(BLE_TRANSPORT_ISO_FROM_LL_COUNT))
#endif
#define POOL_ACL_SIZE       (OS_ALIGN( MYNEWT_VAL(BLE_TRANSPORT_ACL_SIZE) + \
                                       BLE_MBUF_MEMBLOCK_OVERHEAD +         \
                                       BLE_HCI_DATA_HDR_SZ, OS_ALIGNMENT))
#define POOL_ISO_SIZE       (OS_ALIGN(MYNEWT_VAL(BLE_TRANSPORT_ISO_SIZE) + \
                                      BLE_MBUF_MEMBLOCK_OVERHEAD +         \
                                      BLE_HCI_DATA_HDR_SZ, OS_ALIGNMENT))

#if !SOC_ESP_NIMBLE_CONTROLLER || !CONFIG_BT_CONTROLLER_ENABLED

#ifdef ESP_PLATFORM
static os_membuf_t *pool_cmd_buf;
static struct os_mempool pool_cmd;

static os_membuf_t *pool_evt_buf;
static struct os_mempool pool_evt;

static os_membuf_t *pool_evt_lo_buf;
static struct os_mempool pool_evt_lo;

#if POOL_ACL_COUNT > 0
static os_membuf_t *pool_acl_buf;
static struct os_mempool_ext pool_acl;
static struct os_mbuf_pool mpool_acl;
#endif

#if POOL_ISO_COUNT > 0
static os_membuf_t *pool_iso_buf;
static struct os_mempool_ext pool_iso;
static struct os_mbuf_pool mpool_iso;
#endif

#else // !ESP_PLATFORM

static uint8_t pool_cmd_buf[ OS_MEMPOOL_BYTES(POOL_CMD_COUNT, POOL_CMD_SIZE) ];
static struct os_mempool pool_cmd;

static uint8_t pool_evt_buf[ OS_MEMPOOL_BYTES(POOL_EVT_COUNT, POOL_EVT_SIZE) ];
static struct os_mempool pool_evt;

static uint8_t pool_evt_lo_buf[ OS_MEMPOOL_BYTES(POOL_EVT_LO_COUNT, POOL_EVT_SIZE) ];
static struct os_mempool pool_evt_lo;

#if POOL_ACL_COUNT > 0
static uint8_t pool_acl_buf[ OS_MEMPOOL_BYTES(POOL_ACL_COUNT, POOL_ACL_SIZE) ];
static struct os_mempool_ext pool_acl;
static struct os_mbuf_pool mpool_acl;
#endif

#if POOL_ISO_COUNT > 0
static uint8_t pool_iso_buf[ OS_MEMPOOL_BYTES(POOL_ISO_COUNT, POOL_ISO_SIZE) ];
static struct os_mempool_ext pool_iso;
static struct os_mbuf_pool mpool_iso;
#endif

#endif // ESP_PLATFORM

static os_mempool_put_fn *transport_put_acl_from_ll_cb;

void *
ble_transport_alloc_cmd(void)
{
    return os_memblock_get(&pool_cmd);
}

static void *
try_alloc_evt(struct os_mempool *mp)
{
#if BLE_TRANSPORT_IPC_ON_LL
    uint8_t type;
#endif
    void *buf;

#if BLE_TRANSPORT_IPC_ON_LL
    if (mp == &pool_evt) {
        type = HCI_IPC_TYPE_EVT;
    } else {
        type = HCI_IPC_TYPE_EVT_DISCARDABLE;
    }

    if (!hci_ipc_get(type)) {
        return NULL;
    }
#endif

    buf = os_memblock_get(mp);

#if BLE_TRANSPORT_IPC_ON_LL
    if (!buf) {
        hci_ipc_put(type);
    }
#endif

    return buf;
}

void *
ble_transport_alloc_evt(int discardable)
{
    void *buf;

    if (discardable) {
        buf = try_alloc_evt(&pool_evt_lo);
    } else {
        buf = try_alloc_evt(&pool_evt);
        if (!buf) {
            buf = try_alloc_evt(&pool_evt_lo);
        }
    }

    return buf;
}

struct os_mbuf *
ble_transport_alloc_acl_from_hs(void)
{
#if POOL_ACL_COUNT > 0
    struct os_mbuf *om;
    struct os_mbuf_pkthdr *pkthdr;
    uint16_t usrhdr_len;

#if MYNEWT_VAL_CHOICE(BLE_TRANSPORT_LL, native)
    usrhdr_len = sizeof(struct ble_mbuf_hdr);
#else
    usrhdr_len = 0;
#endif

    om = os_mbuf_get_pkthdr(&mpool_acl, usrhdr_len);
    if (om) {
        pkthdr = OS_MBUF_PKTHDR(om);
        pkthdr->omp_flags = OMP_FLAG_FROM_HS;
    }

    return om;
#else
    return NULL;
#endif
}

struct os_mbuf *
ble_transport_alloc_iso_from_hs(void)
{
#if POOL_ISO_COUNT > 0
    struct os_mbuf *om;
    struct os_mbuf_pkthdr *pkthdr;
    uint16_t usrhdr_len;

#if MYNEWT_VAL_CHOICE(BLE_TRANSPORT_LL, native)
    usrhdr_len = sizeof(struct ble_mbuf_hdr);
#else
    usrhdr_len = 0;
#endif

    om = os_mbuf_get_pkthdr(&mpool_iso, usrhdr_len);
    if (om) {
        pkthdr = OS_MBUF_PKTHDR(om);
        pkthdr->omp_flags = OMP_FLAG_FROM_HS;
    }

    return om;
#else
    return NULL;
#endif
}

struct os_mbuf *
ble_transport_alloc_acl_from_ll(void)
{
#if POOL_ACL_COUNT > 0
    struct os_mbuf *om;
    struct os_mbuf_pkthdr *pkthdr;

    om = os_mbuf_get_pkthdr(&mpool_acl, 0);
    if (om) {
        pkthdr = OS_MBUF_PKTHDR(om);
        pkthdr->omp_flags = OMP_FLAG_FROM_LL;
    }

    return om;
#else
    return NULL;
#endif
}

struct os_mbuf *
ble_transport_alloc_iso_from_ll(void)
{
#if POOL_ISO_COUNT > 0
    struct os_mbuf *om;
    struct os_mbuf_pkthdr *pkthdr;

    om = os_mbuf_get_pkthdr(&mpool_iso, 0);
    if (om) {
        pkthdr = OS_MBUF_PKTHDR(om);
        pkthdr->omp_flags = OMP_FLAG_FROM_LL;
    }

    return om;
#else
    return NULL;
#endif
}

void
ble_transport_free(void *buf)
{
    if (os_memblock_from(&pool_cmd, buf)) {
        os_memblock_put(&pool_cmd, buf);
    } else if (os_memblock_from(&pool_evt, buf)) {
        os_memblock_put(&pool_evt, buf);
#if BLE_TRANSPORT_IPC
        hci_ipc_put(HCI_IPC_TYPE_EVT);
#endif
    } else if (os_memblock_from(&pool_evt_lo, buf)) {
        os_memblock_put(&pool_evt_lo, buf);
#if BLE_TRANSPORT_IPC
        hci_ipc_put(HCI_IPC_TYPE_EVT_DISCARDABLE);
#endif
    } else {
        assert(0);
    }
}

void
ble_transport_ipc_free(void *buf)
{
    if (os_memblock_from(&pool_cmd, buf)) {
        os_memblock_put(&pool_cmd, buf);
    } else if (os_memblock_from(&pool_evt, buf)) {
        os_memblock_put(&pool_evt, buf);
    } else if (os_memblock_from(&pool_evt_lo, buf)) {
        os_memblock_put(&pool_evt_lo, buf);
    } else {
        assert(0);
    }
}

#ifdef ESP_PLATFORM

#if POOL_ACL_COUNT > 0
static os_error_t
ble_transport_acl_put(struct os_mempool_ext *mpe, void *data, void *arg)
{
    os_error_t err;
    err = 0;

#if MYNEWT_VAL(BLE_TRANSPORT_INT_FLOW_CTL)
    struct os_mbuf *om;
    struct os_mbuf_pkthdr *pkthdr;
    bool from_ll;
#endif

#if MYNEWT_VAL(BLE_HS_FLOW_CTRL)
    if (transport_put_acl_from_ll_cb) {
        err = transport_put_acl_from_ll_cb(mpe, data, arg);
    }
#else
    err = os_memblock_put_from_cb(&mpe->mpe_mp, data);
#endif

#if MYNEWT_VAL(BLE_TRANSPORT_INT_FLOW_CTL)
    om = data;
    pkthdr = OS_MBUF_PKTHDR(om);

    from_ll = (pkthdr->omp_flags & OMP_FLAG_FROM_MASK) == OMP_FLAG_FROM_LL;

    if (from_ll && !err) {
        hci_ipc_put(HCI_IPC_TYPE_ACL);
    }
#endif

    return err;
}
#endif

void ble_buf_free(void)
{
    os_msys_buf_free();

    nimble_platform_mem_free(pool_evt_buf);
    pool_evt_buf = NULL;
    nimble_platform_mem_free(pool_evt_lo_buf);
    pool_evt_lo_buf = NULL;
    nimble_platform_mem_free(pool_cmd_buf);
    pool_cmd_buf = NULL;
#if POOL_ACL_COUNT > 0
    nimble_platform_mem_free(pool_acl_buf);
    pool_acl_buf = NULL;
#endif
#if POOL_ISO_COUNT > 0
    nimble_platform_mem_free(pool_iso_buf);
    pool_iso_buf = NULL;
#endif
}

esp_err_t ble_buf_alloc(void)
{
    if (os_msys_buf_alloc()) {
        return ESP_ERR_NO_MEM;
    }

    pool_evt_buf = (os_membuf_t *) nimble_platform_mem_calloc(1,
                   (sizeof(os_membuf_t) * OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_TRANSPORT_EVT_COUNT),
                           MYNEWT_VAL(BLE_TRANSPORT_EVT_SIZE))));

    pool_evt_lo_buf = (os_membuf_t *) nimble_platform_mem_calloc(1,
                      (sizeof(os_membuf_t) * OS_MEMPOOL_SIZE(MYNEWT_VAL(BLE_TRANSPORT_EVT_DISCARDABLE_COUNT),
                              MYNEWT_VAL(BLE_TRANSPORT_EVT_SIZE))));

    pool_cmd_buf = (os_membuf_t *) nimble_platform_mem_calloc(1,
                   (sizeof(os_membuf_t) * OS_MEMPOOL_SIZE(POOL_CMD_COUNT, POOL_CMD_SIZE)));

#if POOL_ACL_COUNT > 0
    pool_acl_buf = (os_membuf_t *) nimble_platform_mem_calloc(1,
                   (sizeof(os_membuf_t) * OS_MEMPOOL_SIZE(POOL_ACL_COUNT,
                           POOL_ACL_SIZE)));
    if(!pool_acl_buf) {
       ble_buf_free();
       return ESP_ERR_NO_MEM;
    }
#endif
#if POOL_ISO_COUNT > 0
    pool_iso_buf = (os_membuf_t *) nimble_platform_mem_calloc(1,
                    sizeof(os_membuf_t) * OS_MEMPOOL_SIZE(POOL_ISO_COUNT,
                           POOL_ISO_SIZE));
    if(!pool_iso_buf) {
       ble_buf_free();
       return ESP_ERR_NO_MEM;
    }
#endif
    if (!pool_evt_buf || !pool_evt_lo_buf || !pool_cmd_buf ) {
        ble_buf_free();
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

#else /* !ESP_PLATFORM */

#if POOL_ACL_COUNT > 0
static os_error_t
ble_transport_acl_put(struct os_mempool_ext *mpe, void *data, void *arg)
{
    struct os_mbuf *om;
    struct os_mbuf_pkthdr *pkthdr;
    bool do_put;
    bool from_ll;
    os_error_t err;

    om = data;
    pkthdr = OS_MBUF_PKTHDR(om);

    do_put = true;
    from_ll = (pkthdr->omp_flags & OMP_FLAG_FROM_MASK) == OMP_FLAG_FROM_LL;
    err = 0;

    if (from_ll && transport_put_acl_from_ll_cb) {
        err = transport_put_acl_from_ll_cb(mpe, data, arg);
        do_put = false;
    }

    if (do_put) {
        err = os_memblock_put_from_cb(&mpe->mpe_mp, data);
    }

#if BLE_TRANSPORT_IPC_ON_HS
    if (from_ll && !err) {
        hci_ipc_put(HCI_IPC_TYPE_ACL);
    }
#endif

    return err;
}
#endif
#endif /* ESP_PLATFORM */

void
ble_transport_init(void)
{
    int rc;

    SYSINIT_ASSERT_ACTIVE();

    rc = os_mempool_init(&pool_cmd, POOL_CMD_COUNT, POOL_CMD_SIZE,
                         pool_cmd_buf, "transport_pool_cmd");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mempool_init(&pool_evt, POOL_EVT_COUNT, POOL_EVT_SIZE,
                         pool_evt_buf, "transport_pool_evt");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mempool_init(&pool_evt_lo, POOL_EVT_LO_COUNT, POOL_EVT_SIZE,
                         pool_evt_lo_buf, "transport_pool_evt_lo");
    SYSINIT_PANIC_ASSERT(rc == 0);

#if POOL_ACL_COUNT > 0
    rc = os_mempool_ext_init(&pool_acl, POOL_ACL_COUNT, POOL_ACL_SIZE,
                             pool_acl_buf, "transport_pool_acl");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mbuf_pool_init(&mpool_acl, &pool_acl.mpe_mp,
                           POOL_ACL_SIZE, POOL_ACL_COUNT);
    SYSINIT_PANIC_ASSERT(rc == 0);

    pool_acl.mpe_put_cb = ble_transport_acl_put;
#endif

#if POOL_ISO_COUNT > 0
    rc = os_mempool_ext_init(&pool_iso, POOL_ISO_COUNT, POOL_ISO_SIZE,
                             pool_iso_buf, "transport_pool_iso");
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mbuf_pool_init(&mpool_iso, &pool_iso.mpe_mp,
                           POOL_ISO_SIZE, POOL_ISO_COUNT);
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif
}

void
ble_transport_deinit(void)
{
    int rc = 0;
#if POOL_ISO_COUNT > 0
    rc = os_mempool_ext_clear(&pool_acl);
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif

    rc = os_mempool_clear(&pool_evt_lo);
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mempool_clear(&pool_evt);
    SYSINIT_PANIC_ASSERT(rc == 0);

    rc = os_mempool_clear(&pool_cmd);
    SYSINIT_PANIC_ASSERT(rc == 0);
}

int
ble_transport_register_put_acl_from_ll_cb(os_mempool_put_fn (*cb))
{
    transport_put_acl_from_ll_cb = cb;

    return 0;
}

#if BLE_TRANSPORT_IPC
uint8_t
ble_transport_ipc_buf_evt_type_get(void *buf)
{
    if (os_memblock_from(&pool_cmd, buf)) {
        return HCI_IPC_TYPE_EVT_IN_CMD;
    } else if (os_memblock_from(&pool_evt, buf)) {
        return HCI_IPC_TYPE_EVT;
    } else if (os_memblock_from(&pool_evt_lo, buf)) {
        return HCI_IPC_TYPE_EVT_DISCARDABLE;
    } else {
        assert(0);
    }
    return 0;
}
#endif
#endif /* !SOC_ESP_NIMBLE_CONTROLLER || !CONFIG_BT_CONTROLLER_ENABLED */
