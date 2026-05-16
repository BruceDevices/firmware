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

#ifndef _HCI_IPC_H_
#define _HCI_IPC_H_

#include <stdint.h>

#define HCI_IPC_TYPE_CMD                0x01
#define HCI_IPC_TYPE_ACL                0x02
#define HCI_IPC_TYPE_EVT                0x04
#define HCI_IPC_TYPE_EVT_DISCARDABLE    0x05
#define HCI_IPC_TYPE_EVT_IN_CMD         0x06
#define HCI_IPC_TYPE_ISO                0x07

struct __attribute__((packed)) hci_ipc_hdr {
    uint8_t type;
    uint16_t length;
};

struct hci_ipc_sm {
    struct hci_ipc_hdr hdr;
    uint8_t hdr_len;
    uint16_t rem_len;
    uint16_t buf_len;

    union {
        uint8_t *buf;
        struct os_mbuf *om;
    };
};

struct hci_ipc_shm {
    uint16_t n2a_num_acl;
    uint16_t n2a_num_evt;
    uint16_t n2a_num_evt_disc;
};

void hci_ipc_init(volatile struct hci_ipc_shm *shm, struct hci_ipc_sm *sm);
int hci_ipc_rx(struct hci_ipc_sm *sm, const uint8_t *buf, uint16_t len);

extern void hci_ipc_atomic_put(volatile uint16_t *num);
extern uint16_t hci_ipc_atomic_get(volatile uint16_t *num);

/* Just to optimize static inlines below, do not use directly! */
extern volatile struct hci_ipc_shm *g_ipc_shm;

static inline int
hci_ipc_get(uint8_t type)
{
    volatile struct hci_ipc_shm *shm = g_ipc_shm;

    switch (type) {
    case HCI_IPC_TYPE_ACL:
        return hci_ipc_atomic_get(&shm->n2a_num_acl);
    case HCI_IPC_TYPE_EVT:
        return hci_ipc_atomic_get(&shm->n2a_num_evt);
    case HCI_IPC_TYPE_EVT_DISCARDABLE:
        return hci_ipc_atomic_get(&shm->n2a_num_evt_disc);
    }

    return 0;
}

static inline void
hci_ipc_put(uint8_t type)
{
    volatile struct hci_ipc_shm *shm = g_ipc_shm;

    switch (type) {
    case HCI_IPC_TYPE_ACL:
        hci_ipc_atomic_put(&shm->n2a_num_acl);
        break;
    case HCI_IPC_TYPE_EVT:
        hci_ipc_atomic_put(&shm->n2a_num_evt);
        break;
    case HCI_IPC_TYPE_EVT_DISCARDABLE:
        hci_ipc_atomic_put(&shm->n2a_num_evt_disc);
        break;
    }
}

#endif /* _HCI_IPC_H_ */
