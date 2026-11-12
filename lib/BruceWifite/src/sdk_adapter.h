/* WirelessPi WPS transport adapter, GPL-3.0-or-later. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <esp_err.h>
typedef struct {
  uint32_t scans, seen, connecting, ready;
  esp_err_t error;
} WfSdkStatus;
/* Configure only after esp_wifi_wps_enable(), before esp_wifi_wps_start().
 * Clear only after esp_wifi_wps_disable() has joined the WPS task. */
esp_err_t wf_sdk_prepare(const uint8_t bssid[6], uint8_t channel,
                        void (*ready)(void),
                        void (*receive)(const uint8_t *, const uint8_t *, size_t));
void wf_sdk_clear(void);
void wf_sdk_status(WfSdkStatus *out);
