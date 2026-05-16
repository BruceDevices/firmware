/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef ESP_PLATFORM
#include "syscfg/syscfg.h"
#if CONFIG_BT_LE_CONTROLLER_NPL_OS_PORTING_SUPPORT

#include <string.h>
#include <stdio.h>
#include "esp_hci_internal.h"
#include "esp_hci_driver.h"

typedef struct {
    hci_driver_forward_fn *forward_cb;
} hci_driver_vhci_env_t;

static hci_driver_vhci_env_t s_hci_driver_vhci_env;

static int
hci_driver_vhci_controller_tx(hci_driver_data_type_t data_type, uint8_t *data)
{
    /* The length is contained in the data. */
    return s_hci_driver_vhci_env.forward_cb(data_type, data, 0, HCI_DRIVER_DIR_C2H);
}

static int
hci_driver_vhci_host_tx(hci_driver_data_type_t data_type, uint8_t *data, uint32_t length)
{
    return s_hci_driver_vhci_env.forward_cb(data_type, data, length, HCI_DRIVER_DIR_H2C);
}

static int
hci_driver_vhci_tx(hci_driver_data_type_t data_type, uint8_t *data, uint32_t length,
                       hci_driver_direction_t dir)
{
    int rc;

    if (dir == HCI_DRIVER_DIR_C2H) {
        rc = hci_driver_vhci_controller_tx(data_type, data);
    } else {
        rc = hci_driver_vhci_host_tx(data_type, data, length);
    }
    return rc;
}

static int
hci_driver_vhci_init(hci_driver_forward_fn *cb)
{
    s_hci_driver_vhci_env.forward_cb = cb;
    return 0;
}

static void
hci_driver_vhci_deinit(void)
{
    memset(&s_hci_driver_vhci_env, 0, sizeof(hci_driver_vhci_env_t));
}

hci_driver_ops_t na_hci_driver_vhci_ops = {
    .hci_driver_tx = hci_driver_vhci_tx,
    .hci_driver_init = hci_driver_vhci_init,
    .hci_driver_deinit = hci_driver_vhci_deinit,
};

// Prevent linking errors when using arduino + bluedroid with esp32c2
#if defined(CONFIG_IDF_TARGET_ESP32C2) && ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 5, 1)
void adv_stack_enableClearLegacyAdvVsCmd(bool en){}
void scan_stack_enableAdvFlowCtrlVsCmd(bool en){}
void advFilter_stack_enableDupExcListVsCmd(bool en){}
void arr_stack_enableMultiConnVsCmd(bool en){}
void pcl_stack_enableSetRssiThreshVsCmd(bool en){}
void chanSel_stack_enableSetCsaVsCmd(bool en){}
void log_stack_enableLogsRelatedVsCmd(bool en){}
void hci_stack_enableSetVsEvtMaskVsCmd(bool en){}
void winWiden_stack_enableSetConstPeerScaVsCmd(bool en){}
#if CONFIG_IDF_TARGET_ESP32C61_ECO3
void conn_stack_enableSetPrefTxRxCntVsCmd(bool en){}
#endif // CONFIG_IDF_TARGET_ESP32C61_ECO3

void adv_stack_enableScanReqRxdVsEvent(bool en){}
void conn_stack_enableChanMapUpdCompVsEvent(bool en){}
void sleep_stack_enableWakeupVsEvent(bool en){}

#if SOC_ECC_SUPPORTED && ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 4, 0)
void esp_crypto_ecc_lock_acquire(void) {}
void esp_crypto_ecc_lock_release(void) {}
#endif
#endif

#endif
#endif
