#ifndef __BLE_P4_COMPAT_H__
#define __BLE_P4_COMPAT_H__

// -----------------------------------------------------------------------------
// ESP32-P4 BLE TX-power compatibility shim.
//
// The P4 has no native Bluetooth controller; its BLE stack runs host-only
// (NimBLE) over the ESP-Hosted co-processor. The legacy controller TX-power API
// from esp_bt.h / esp_gap_ble_api.h (esp_power_level_t, ESP_PWR_LVL_*,
// esp_ble_tx_power_set, esp_ble_power_type_t) is therefore not available on P4,
// and NimBLE 2.5.1 removes its esp_power_level_t overloads for this target.
//
// Bruce's BLE modules still speak that legacy API, so map it onto NimBLE's
// modern dBm-based NimBLEDevice::setPower(int8_t). We define the ESP_PWR_LVL_*
// constants as their real dBm values (they only exist here, so there is no
// clash with the vendor enum), which makes setPower() receive correct dBm.
// -----------------------------------------------------------------------------
#ifdef CONFIG_IDF_TARGET_ESP32P4
#include <NimBLEDevice.h>
#include <esp_err.h>

typedef int8_t esp_power_level_t;
typedef int    esp_ble_power_type_t;

enum {
    ESP_PWR_LVL_N12 = -12,
    ESP_PWR_LVL_N9 = -9,
    ESP_PWR_LVL_N6 = -6,
    ESP_PWR_LVL_N3 = -3,
    ESP_PWR_LVL_N0 = 0,
    ESP_PWR_LVL_P3 = 3,
    ESP_PWR_LVL_P6 = 6,
    ESP_PWR_LVL_P9 = 9,
    ESP_PWR_LVL_P12 = 12,
    ESP_PWR_LVL_P15 = 15,
    ESP_PWR_LVL_P18 = 18,
    ESP_PWR_LVL_P20 = 20,
    ESP_PWR_LVL_P21 = 21,
};

#define ESP_BLE_PWR_TYPE_ADV 0
#define ESP_BLE_PWR_TYPE_SCAN 1
#define ESP_BLE_PWR_TYPE_DEFAULT 2

static inline esp_err_t esp_ble_tx_power_set(esp_ble_power_type_t /*type*/, esp_power_level_t level) {
    return NimBLEDevice::setPower(static_cast<int8_t>(level)) ? ESP_OK : ESP_FAIL;
}
#endif // CONFIG_IDF_TARGET_ESP32P4

#endif // __BLE_P4_COMPAT_H__
