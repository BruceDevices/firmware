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

#if defined(ARDUINO_ARCH_NRF5) && defined(NRF53)

#include <assert.h>
#include <string.h>
#include <nimble/porting/nimble/include/syscfg/syscfg.h>
#include <nimble/porting/nimble/include/sysinit/sysinit.h>
#include <nimble/nimble/include/nimble/ble.h>
#include <ipc_nrf5340/ipc_nrf5340.h>
#include <nimble/nimble/transport/include/nimble/transport.h>
#include <nimble/nimble/transport/common/hci_ipc/include/nimble/transport/hci_ipc.h>

#if MYNEWT_VAL(BLE_CONTROLLER)
#define IPC_TX_CHANNEL 0
#define IPC_RX_CHANNEL 1
#else
#define IPC_TX_CHANNEL 1
#define IPC_RX_CHANNEL 0
#endif

static struct hci_ipc_sm g_hci_ipc_sm;

static int
nrf5340_ble_hci_acl_tx(struct os_mbuf *om)
{
    struct hci_ipc_hdr hdr;
    struct os_mbuf *x;
    int rc;

    hdr.type = HCI_IPC_TYPE_ACL;
    hdr.length = 4 + get_le16(&om->om_data[2]);

    rc = ipc_nrf5340_write(IPC_TX_CHANNEL, &hdr, sizeof(hdr), false);
    if (rc == 0) {
        x = om;
        while (x) {
            rc = ipc_nrf5340_write(IPC_TX_CHANNEL, x->om_data, x->om_len, true);
            if (rc < 0) {
                break;
            }
            x = SLIST_NEXT(x, om_next);
        }
    }

    os_mbuf_free_chain(om);

    return (rc < 0) ? BLE_ERR_MEM_CAPACITY : 0;
}

#if !MYNEWT_VAL(BLE_CONTROLLER)
static int
nrf5340_ble_hci_iso_tx(struct os_mbuf *om)
{
    struct hci_ipc_hdr hdr;
    struct os_mbuf *x;
    int rc;

    hdr.type = HCI_IPC_TYPE_ISO;
    hdr.length = 4 + get_le16(&om->om_data[2]);

    rc = ipc_nrf5340_write(IPC_TX_CHANNEL, &hdr, sizeof(hdr), false);
    if (rc == 0) {
        x = om;
        while (x) {
            rc = ipc_nrf5340_write(IPC_TX_CHANNEL, x->om_data, x->om_len, true);
            if (rc < 0) {
                break;
            }
            x = SLIST_NEXT(x, om_next);
        }
    }

    os_mbuf_free_chain(om);

    return (rc < 0) ? BLE_ERR_MEM_CAPACITY : 0;
}
#endif

static void
nrf5340_ble_hci_trans_rx(int channel, void *user_data)
{
    uint8_t *buf;
    int len;

    len = ipc_nrf5340_available_buf(channel, (void **)&buf);
    while (len > 0) {
        len = hci_ipc_rx(&g_hci_ipc_sm, buf, len);
        ipc_nrf5340_consume(channel, len);
        len = ipc_nrf5340_available_buf(channel, (void **)&buf);
    }
}

static void
nrf5340_ble_hci_init(void)
{
    SYSINIT_ASSERT_ACTIVE();

    ipc_nrf5340_recv(IPC_RX_CHANNEL, nrf5340_ble_hci_trans_rx, NULL);
}

#if MYNEWT_VAL(BLE_CONTROLLER)
int
ble_transport_to_hs_evt_impl(void *buf)
{
    struct hci_ipc_hdr hdr;
    uint8_t *hci_ev = buf;
    int rc;

    hdr.type = ble_transport_ipc_buf_evt_type_get(buf);
    hdr.length = 2 + hci_ev[1];

    rc = ipc_nrf5340_write(IPC_TX_CHANNEL, &hdr, sizeof(hdr), false);
    if (rc == 0) {
        rc = ipc_nrf5340_write(IPC_TX_CHANNEL, hci_ev, hdr.length, true);
    }

    ble_transport_ipc_free(buf);

    return (rc < 0) ? BLE_ERR_MEM_CAPACITY : 0;
}

int
ble_transport_to_hs_acl_impl(struct os_mbuf *om)
{
    return nrf5340_ble_hci_acl_tx(om);
}

void
ble_transport_hs_init(void)
{
    volatile struct hci_ipc_shm *shm = ipc_nrf5340_hci_shm_get();

    hci_ipc_init(shm, &g_hci_ipc_sm);
    nrf5340_ble_hci_init();
}
#endif /* BLE_CONTROLLER */

#if !MYNEWT_VAL(BLE_CONTROLLER)
int
ble_transport_to_ll_cmd_impl(void *buf)
{
    struct hci_ipc_hdr hdr;
    uint8_t *cmd = buf;
    int rc;

    hdr.type = HCI_IPC_TYPE_CMD;
    hdr.length = 3 + cmd[2];

    rc = ipc_nrf5340_write(IPC_TX_CHANNEL, &hdr, sizeof(hdr), false);
    if (rc == 0) {
        rc = ipc_nrf5340_write(IPC_TX_CHANNEL, cmd, hdr.length, true);
    }

    ble_transport_ipc_free(buf);

    return (rc < 0) ? BLE_ERR_MEM_CAPACITY :  0;
}

int
ble_transport_to_ll_acl_impl(struct os_mbuf *om)
{
    return nrf5340_ble_hci_acl_tx(om);
}

int
ble_transport_to_ll_iso_impl(struct os_mbuf *om)
{
    return nrf5340_ble_hci_iso_tx(om);
}

void
ble_transport_ll_init(void)
{
    volatile struct hci_ipc_shm *shm = ipc_nrf5340_hci_shm_get();

    hci_ipc_init(shm, &g_hci_ipc_sm);
    nrf5340_ble_hci_init();
}
#endif /* !BLE_CONTROLLER */

uint16_t
hci_ipc_atomic_get(volatile uint16_t *num)
{
    int ret;

    __asm__ volatile (".syntax unified                \n"
                      "1: ldrexh r1, [%[addr]]        \n"
                      "   mov %[ret], r1              \n"
                      "   cmp r1, #0                  \n"
                      "   itte ne                     \n"
                      "   subne r2, r1, #1            \n"
                      "   strexhne r1, r2, [%[addr]]  \n"
                      "   clrexeq                     \n"
                      "   cmp r1, #0                  \n"
                      "   bne 1b                      \n"
                      : [ret] "=&r" (ret)
                      : [addr] "r" (num)
                      : "r1", "r2", "memory");

    return ret;
}

void
hci_ipc_atomic_put(volatile uint16_t *num)
{
    __asm__ volatile (".syntax unified              \n"
                      "1: ldrexh r1, [%[addr]]      \n"
                      "   add r1, r1, #1            \n"
                      "   strexh r2, r1, [%[addr]]  \n"
                      "   cmp r2, #0                \n"
                      "   bne 1b                    \n"
                      :
                      : [addr] "r" (num)
                      : "r1", "r2", "memory");
}

#endif /* ARDUINO_ARCH_NRF5 && NRF53 */
