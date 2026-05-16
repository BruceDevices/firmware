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
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "nimble/porting/nimble/include/os/os.h"
#include "nimble/nimble/include/nimble/hci_common.h"
#include "ble_hs_priv.h"

#ifdef ESP_PLATFORM
// #include "bt_common.h"
// #if (BT_HCI_LOG_INCLUDED == TRUE)
// #include "hci_log/bt_hci_log.h"
// #endif // (BT_HCI_LOG_INCLUDED == TRUE)


/*
 * HCI Command Header
 *
 * Comprises the following fields
 *  -> Opcode group field & Opcode command field (2)
 *  -> Parameter Length                          (1)
 *      Length of all the parameters (does not include any part of the hci
 *      command header
 */
#define BLE_HCI_CMD_HDR_LEN                 (3)

static int
ble_hs_hci_cmd_transport(struct ble_hci_cmd *cmd)
{
    int rc;

    rc = ble_transport_to_ll_cmd((uint8_t *)cmd);
    switch (rc) {
    case 0:
        return 0;

    case BLE_ERR_MEM_CAPACITY:
        return BLE_HS_ENOMEM_EVT;

    default:
        return BLE_HS_EUNKNOWN;
    }
}

static int
ble_hs_hci_cmd_send(uint16_t opcode, uint8_t len, const void *cmddata)
{
    struct ble_hci_cmd *cmd;
    uint8_t *buf;
    int rc;

    cmd = (struct ble_hci_cmd *)ble_transport_alloc_cmd();
    BLE_HS_DBG_ASSERT(cmd != NULL);

    buf = (uint8_t *)cmd;
#if !(SOC_ESP_NIMBLE_CONTROLLER) && CONFIG_BT_CONTROLLER_ENABLED
    /* Hack for avoiding memcpy while handling tx pkt to VHCI,
     * keep one byte for type field*/
    buf++;
#endif
    put_le16(buf, opcode);
    buf[2] = len;
    if (len != 0) {
        memcpy(buf + BLE_HCI_CMD_HDR_LEN, cmddata, len);
    }
#if !BLE_MONITOR
    BLE_HS_LOG(DEBUG, "ble_hs_hci_cmd_send: ogf=0x%02x ocf=0x%04x len=%d\n",
               BLE_HCI_OGF(opcode), BLE_HCI_OCF(opcode), len);
    ble_hs_log_flat_buf(buf, len + BLE_HCI_CMD_HDR_LEN);
    BLE_HS_LOG(DEBUG, "\n");
#endif

#if !(SOC_ESP_NIMBLE_CONTROLLER) && CONFIG_BT_CONTROLLER_ENABLED
    buf--;
#endif

// #if ((BT_HCI_LOG_INCLUDED == TRUE) && SOC_ESP_NIMBLE_CONTROLLER && CONFIG_BT_CONTROLLER_ENABLED)
//     uint8_t *data;
// #if !(SOC_ESP_NIMBLE_CONTROLLER) && CONFIG_BT_CONTROLLER_ENABLED
//     data = (uint8_t *)buf + 1;
// #else
//     data = (uint8_t *)buf;
// #endif
//     bt_hci_log_record_hci_data(0x01, data, len + BLE_HCI_CMD_HDR_LEN);
// #endif

    rc = ble_hs_hci_cmd_transport((void *) buf);

    if (rc == 0) {
        STATS_INC(ble_hs_stats, hci_cmd);
    } else {
        BLE_HS_LOG(DEBUG, "ble_hs_hci_cmd_send failure; rc=%d\n", rc);
    }

    return rc;
}
# else /* ! ESP_PLATFORM */
static int
ble_hs_hci_cmd_transport(struct ble_hci_cmd *cmd)
{
    int rc;

    rc = ble_transport_to_ll_cmd(cmd);
    switch (rc) {
    case 0:
        return 0;

    case BLE_ERR_MEM_CAPACITY:
        return BLE_HS_ENOMEM_EVT;

    default:
        return BLE_HS_EUNKNOWN;
    }
}

static int
ble_hs_hci_cmd_send(uint16_t opcode, uint8_t len, const void *cmddata)
{
    struct ble_hci_cmd *cmd;
    int rc;

    cmd = ble_transport_alloc_cmd();
    BLE_HS_DBG_ASSERT(cmd != NULL);

    cmd->opcode = htole16(opcode);
    cmd->length = len;
    if (len != 0) {
        memcpy(cmd->data, cmddata, len);
    }

    rc = ble_hs_hci_cmd_transport(cmd);

    if (rc == 0) {
        STATS_INC(ble_hs_stats, hci_cmd);
    } else {
        BLE_HS_LOG(DEBUG, "ble_hs_hci_cmd_send failure; rc=%d\n", rc);
    }

    return rc;
}

#endif /*ESP_PLATFORM */

int
ble_hs_hci_cmd_send_buf(uint16_t opcode, const void *buf, uint8_t buf_len)
{
    switch (ble_hs_sync_state) {
    case BLE_HS_SYNC_STATE_BAD:
        return BLE_HS_ENOTSYNCED;

    case BLE_HS_SYNC_STATE_BRINGUP:
        if (!ble_hs_is_parent_task()) {
            return BLE_HS_ENOTSYNCED;
        }
        break;

    case BLE_HS_SYNC_STATE_GOOD:
        break;

    default:
        BLE_HS_DBG_ASSERT(0);
        return BLE_HS_EUNKNOWN;
    }

    return ble_hs_hci_cmd_send(opcode, buf_len, buf);
}
