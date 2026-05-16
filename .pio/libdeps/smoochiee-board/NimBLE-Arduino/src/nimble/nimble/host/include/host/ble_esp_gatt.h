/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef H_BLE_ESP_GATT_
#define H_BLE_ESP_GATT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Resets the GATT configuration parameters and deallocates the memory of attributes.
 *
 */
void ble_gatts_stop(void);

#ifdef __cplusplus
}
#endif

#endif
