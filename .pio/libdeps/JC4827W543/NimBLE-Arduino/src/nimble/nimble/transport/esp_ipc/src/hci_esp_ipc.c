/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nimble/porting/nimble/include/syscfg/syscfg.h>
#ifdef ESP_PLATFORM
# if !defined(CONFIG_IDF_TARGET_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3) && \
     !defined(CONFIG_IDF_TARGET_ESP32S3) && CONFIG_BT_BLUEDROID_ENABLED

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <nimble/porting/nimble/include/sysinit/sysinit.h>
#include "nimble/porting/nimble/include/os/os_mbuf.h"
#include "nimble/nimble/transport/include/nimble/transport.h"
#include "nimble/esp_port/port/transport/include/esp_hci_transport.h"
#include "nimble/esp_port/port/transport/include/esp_hci_internal.h"

static int
ble_transport_dummy_host_recv_cb(hci_trans_pkt_ind_t type, uint8_t *data, uint16_t len)
{
    /* Dummy function */
    return 0;
}

static int
ble_transport_host_recv_cb(hci_trans_pkt_ind_t type, uint8_t *data, uint16_t len)
{
    int rc;

    if (type == HCI_ACL_IND) {
        rc = ble_transport_to_hs_acl((struct os_mbuf *)data);
    } else {
        rc = ble_transport_to_hs_evt(data);
    }
    return rc;
}

int
ble_transport_to_ll_cmd_impl(void *buf)
{
    return na_hci_transport_host_cmd_tx(buf, 0);
}

int
ble_transport_to_ll_acl_impl(struct os_mbuf *om)
{
    return na_hci_transport_host_acl_tx((uint8_t *)om, 0);
}

void
ble_transport_ll_init(void)
{
    na_hci_transport_host_callback_register(ble_transport_host_recv_cb);
}

void
ble_transport_ll_deinit(void)
{
    na_hci_transport_host_callback_register(ble_transport_dummy_host_recv_cb);
}

void *
ble_transport_alloc_cmd(void)
{
    return r_ble_hci_trans_buf_alloc(ESP_HCI_INTERNAL_BUF_CMD);
}

void
ble_transport_free(void *buf)
{
    r_ble_hci_trans_buf_free(buf);
}

#endif /* !CONFIG_IDF_TARGET_ESP32 && !CONFIG_IDF_TARGET_ESP32C3 && !CONFIG_IDF_TARGET_ESP32S3 */
#endif /* ESP_PLATFORM */