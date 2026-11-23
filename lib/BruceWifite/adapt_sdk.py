#!/usr/bin/env python3
"""Generate an auditable, pinned SDK adapter without editing the SDK checkout.

SDK transport remains Espressif esp_wps.c. Only a selected-BSSID registrar
session uses the adapter; ordinary WPS calls retain their existing behavior.
"""
import hashlib
from pathlib import Path
import sys

EXPECTED = 'a251e979423ae78738e2f35833f319c47153379ad3fe67b82b7206e313abf2a3'

SDK_CONFIG_PREFIX = '#include "sdkconfig.h"\n#if CONFIG_MBEDTLS_SHA1_C || CONFIG_MBEDTLS_HARDWARE_SHA\n#define CONFIG_FAST_PBKDF2 1\n#endif\n#if CONFIG_ESP_WIFI_ENABLE_WPA3_SAE\n#define CONFIG_WPA3_SAE 1\n#endif\n#if CONFIG_ESP_WIFI_ENABLE_SAE_PK\n#define CONFIG_SAE_PK 1\n#endif\n#if CONFIG_ESP_WIFI_ENABLE_SAE_H2E\n#define CONFIG_SAE_H2E 1\n#endif\n#if CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT\n#define CONFIG_SAE 1\n#endif\n#if CONFIG_ESP_WIFI_WPS_STRICT\n#define CONFIG_WPS_STRICT 1\n#endif\n#if CONFIG_ESP_WIFI_WPS_RECONNECT_ON_FAIL\n#define CONFIG_WPS_RECONNECT_ON_FAIL 1\n#endif\n#if CONFIG_ESP_WIFI_SUITE_B_192\n#define CONFIG_SUITEB192 1\n#endif\n#if CONFIG_WPA_SUITE_B\n#define CONFIG_SUITEB 1\n#endif\n#if CONFIG_ESP_WIFI_GCMP_SUPPORT\n#define CONFIG_GCMP 1\n#endif\n#if CONFIG_ESP_WIFI_GMAC_SUPPORT\n#define CONFIG_GMAC 1\n#endif\n#if CONFIG_ESP_WIFI_MBO_SUPPORT\n#define CONFIG_MBO 1\n#endif\n#if CONFIG_ESP_WIFI_DPP_SUPPORT\n#define CONFIG_DPP 1\n#endif\n#if CONFIG_ESP_WIFI_11KV_SUPPORT\n#define CONFIG_IEEE80211KV 1\n#endif\n#if CONFIG_ESP_WIFI_RRM_SUPPORT\n#define CONFIG_RRM 1\n#endif\n#if CONFIG_ESP_WIFI_WNM_SUPPORT\n#define CONFIG_WNM 1\n#endif\n#if CONFIG_ESP_WIFI_11R_SUPPORT\n#define CONFIG_IEEE80211R 1\n#endif\n#if CONFIG_ESP_WIFI_TESTING_OPTIONS\n#define CONFIG_TESTING_OPTIONS 1\n#endif\n#if !CONFIG_ESP_WIFI_MBEDTLS_TLS_CLIENT\n#define CONFIG_TLS_INTERNAL_CLIENT 1\n#define CONFIG_TLSV11 1\n#define CONFIG_TLSV12 1\n#define EAP_FAST 1\n#endif\n#if CONFIG_ESP_WIFI_MBEDTLS_CRYPTO\n#define CONFIG_CRYPTO_MBEDTLS 1\n#endif\n#if !CONFIG_MBEDTLS_SHA1_C && !CONFIG_MBEDTLS_HARDWARE_SHA\n#define CONFIG_CRYPTO_INTERNAL 1\n#endif\n#if CONFIG_ESP_WIFI_WPS_SOFTAP_REGISTRAR\n#define CONFIG_WPS_REGISTRAR 1\n#endif\n#if CONFIG_ESP_WIFI_ENABLE_WPA3_OWE_STA\n#define CONFIG_OWE_STA 1\n#endif\n#if CONFIG_ESP_WIFI_EAP_TLS1_3\n#define CONFIG_TLSV13 1\n#endif\n'

def adapt(source):
    if hashlib.sha256(source).hexdigest() != EXPECTED:
        raise ValueError('WIFITE requires unmodified ESP-IDF v5.5.4 esp_wps.c')
    s = source.decode()
    def replace(old, new):
        nonlocal s
        if s.count(old) != 1:
            raise ValueError('SDK adapter anchor mismatch: ' + old[:80])
        s = s.replace(old, new, 1)

    replace('static wps_factory_information_t *s_factory_info = NULL;', '''static wps_factory_information_t *s_factory_info = NULL;
#include "sdk_adapter.h"
static bool wf_sdk_enabled;
static uint8_t wf_sdk_bssid[6], wf_sdk_channel;
static uint32_t wf_sdk_scans, wf_sdk_seen, wf_sdk_connecting, wf_sdk_ready;
static int wf_sdk_error;
static void (*wf_sdk_on_ready)(void);
static void (*wf_sdk_on_receive)(const uint8_t *, const uint8_t *, size_t);
static bool wf_sdk_active(void) {
    return __atomic_load_n(&wf_sdk_enabled, __ATOMIC_ACQUIRE);
}
static bool wf_sdk_scan(struct wps_scan_ie *scan);''')
    replace('wps_parse_scan_result(struct wps_scan_ie *scan)\n{', '''wps_parse_scan_result(struct wps_scan_ie *scan)
{
    if (wf_sdk_active()) return wf_sdk_scan(scan);''')
    replace('int wps_start_pending(void)\n{', '''int wps_start_pending(void)
{
    if (wf_sdk_active()) {
        esp_wifi_disarm_sta_connection_timer_internal();
        __atomic_fetch_add(&wf_sdk_ready, 1, __ATOMIC_RELAXED);
        wf_sdk_on_ready();
        return ESP_OK;
    }''')
    replace('static void wps_sm_notify_deauth(void)\n{', '''static void wps_sm_notify_deauth(void)
{
    /* External worker owns timeout/retry policy; do not restart enrolment. */
    if (wf_sdk_active()) return;''')
    replace('int wps_sm_rx_eapol(u8 *src_addr, u8 *buf, u32 len)\n{', '''int wps_sm_rx_eapol(u8 *src_addr, u8 *buf, u32 len)
{
    if (wf_sdk_active()) {
        if (src_addr && buf && len >= 4 && len <= 1600 &&
            !os_memcmp(src_addr, wf_sdk_bssid, 6))
            wf_sdk_on_receive(src_addr, buf, len);
        return ESP_OK;
    }''')
    replace('    esp_wifi_promiscuous_scan_start(NULL, wifi_wps_scan_done);', '''    if (wf_sdk_active()) {
        wifi_scan_config_t scan = {0};
        scan.bssid = wf_sdk_bssid;
        scan.channel = wf_sdk_channel;
        scan.show_hidden = true;
        scan.scan_type = WIFI_SCAN_TYPE_ACTIVE;
        scan.scan_time.active.min = 120;
        scan.scan_time.active.max = 300;
        __atomic_fetch_add(&wf_sdk_scans, 1, __ATOMIC_RELAXED);
        int err = esp_wifi_promiscuous_scan_start(&scan, wifi_wps_scan_done);
        if (err != ESP_OK)
            __atomic_store_n(&wf_sdk_error, err, __ATOMIC_RELAXED);
    } else {
        esp_wifi_promiscuous_scan_start(NULL, wifi_wps_scan_done);
    }''')
    replace('''        esp_wifi_set_config(0, &wifi_config);

        wpa_printf(MSG_DEBUG, "WPS: Starting negotiation");
        wifi_config.sta.failure_retry_cnt = 2;
        esp_wifi_connect();
        sm->state = WAIT_START;
        eloop_cancel_timeout(wifi_station_wps_msg_timeout, NULL, NULL);
        eloop_register_timeout(WPS_CONNECT_TIMEOUT_SECS, 0, wifi_station_wps_msg_timeout, NULL, NULL);''', '''        wifi_config.sta.failure_retry_cnt = 2;
        if (wf_sdk_active()) {
            int err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
            if (err == ESP_OK) err = esp_wifi_connect();
            __atomic_store_n(&wf_sdk_error, err, __ATOMIC_RELAXED);
            if (err == ESP_OK)
                __atomic_fetch_add(&wf_sdk_connecting, 1, __ATOMIC_RELAXED);
        } else {
            esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
            esp_wifi_connect();
        }
        sm->state = WAIT_START;
        eloop_cancel_timeout(wifi_station_wps_msg_timeout, NULL, NULL);
        if (!wf_sdk_active())
            eloop_register_timeout(2, 0, wifi_station_wps_msg_timeout, NULL, NULL);''')
    return '// Adapter private headers are pinned to the same ESP-IDF revision as Bruce\'s SDK.\n#include "esp_idf_version.h"\n#if ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(5, 5, 4)\n#error "WiFite SDK adapter requires ESP-IDF 5.5.4; rebase it before upgrading the SDK"\n#endif\n' + SDK_CONFIG_PREFIX + s + '\n#include "sdk_adapter.inc"\n'

if __name__ == '__main__':
    source, dest = map(Path, sys.argv[1:])
    output = adapt(source.read_bytes())
    dest.parent.mkdir(parents=True, exist_ok=True)
    if not dest.exists() or dest.read_text() != output:
        dest.write_text(output)
    print('WIFITE: pinned ESP-IDF WPS adapter generated (SDK checkout unchanged)')
