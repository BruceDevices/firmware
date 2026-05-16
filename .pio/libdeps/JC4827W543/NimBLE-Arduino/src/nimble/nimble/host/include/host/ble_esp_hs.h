/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef H_BLE_ESP_HS_
#define H_BLE_ESP_HS_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Deinitializes the NimBLE host. This function must be called after the
 * NimBLE host stop procedure is complete.
 */
void ble_hs_deinit(void);

#if MYNEWT_VAL(BLE_QUEUE_CONG_CHECK)
/**
 * Initializes the Bluetooth advertising list and associated mutex lock.
 */
void ble_adv_list_init(void);

/**
 * Deinitializes the Bluetooth advertising list, releasing allocated memory and resources.
 */
void ble_adv_list_deinit(void);

/**
 * Adds a Bluetooth advertising packet to the list.
 */
void ble_adv_list_add_packet(void *data);

/**
 * Returns the count of Bluetooth advertising packets in the list.
 */
uint32_t ble_get_adv_list_length(void);

/**
 *  Clears and refreshes the Bluetooth advertising list.
 */
void ble_adv_list_refresh(void);

/**
 * Checks if a Bluetooth address is present in the advertising list; if not, adds it to the list.
 */
bool ble_check_adv_list(const uint8_t *addr, uint8_t addr_type);
#endif

#ifdef __cplusplus
}
#endif

#endif
