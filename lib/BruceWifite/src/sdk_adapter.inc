/* Included into the pinned ESP-IDF esp_wps.c, after its definitions.
 * Uses the real SDK WPS task, initialization, scan and association flow.
 * Only peer selection and EAP delivery are adapted for a station registrar.
 * WirelessPi changes: GPL-3.0-or-later; SDK retains Apache-2.0 attribution. */
static bool wf_sdk_scan(struct wps_scan_ie *scan)
{
    struct wps_sm *sm = gWpsSm;
    if (!sm || !sm->is_wps_scan || !scan || !scan->bssid ||
        !scan->wps || !scan->ssid || scan->ssid[1] > 32 ||
        os_memcmp(scan->bssid, wf_sdk_bssid, 6) ||
        (wps_get_status() != WPS_STATUS_DISABLE &&
         wps_get_status() != WPS_STATUS_SCANNING))
        return false;
    if (!scan->rsn && !scan->wpa && (scan->capinfo & WLAN_CAPABILITY_PRIVACY))
        return false;
    esp_wifi_enable_sta_privacy_internal();
    os_memset(sm->creds[0].ssid, 0, sizeof(sm->creds[0].ssid));
    os_memcpy(sm->creds[0].ssid, scan->ssid + 2, scan->ssid[1]);
    sm->creds[0].ssid_len = scan->ssid[1];
    os_memcpy(sm->bssid, scan->bssid, 6);
    sm->channel = scan->chan;
    sm->discover_ssid_cnt = 1;
    sm->wps_pbc_overlap = false;
    __atomic_fetch_add(&wf_sdk_seen, 1, __ATOMIC_RELAXED);
    return true;
}

esp_err_t wf_sdk_prepare(const uint8_t bssid[6], uint8_t channel,
                        void (*ready)(void),
                        void (*receive)(const uint8_t *, const uint8_t *, size_t))
{
    if (!gWpsSm || !s_wps_enabled || !bssid || !ready || !receive ||
        channel < 1 || channel > 14 || wf_sdk_active())
        return ESP_ERR_INVALID_STATE;
    /* Use the SDK's complete registrar IEs rather than a hand-built probe IE. */
    struct wpabuf *probe = wps_build_probe_req_ie(DEV_PW_DEFAULT,
        gWpsSm->dev, gWpsSm->uuid, WPS_REQ_REGISTRAR, 0, NULL);
    struct wpabuf *assoc = wps_build_assoc_req_ie(WPS_REQ_REGISTRAR);
    esp_err_t err = ESP_ERR_NO_MEM;
    if (probe && assoc) {
        err = esp_wifi_set_appie_internal(WIFI_APPIE_WPS_PR,
            wpabuf_mhead(probe), wpabuf_len(probe), 0);
        if (err == ESP_OK)
            err = esp_wifi_set_appie_internal(WIFI_APPIE_WPS_AR,
                wpabuf_mhead(assoc), wpabuf_len(assoc), 0);
    }
    wpabuf_free(probe);
    wpabuf_free(assoc);
    if (err != ESP_OK)
        return err;
    os_memcpy(wf_sdk_bssid, bssid, 6);
    wf_sdk_channel = channel;
    wf_sdk_on_ready = ready;
    wf_sdk_on_receive = receive;
    __atomic_store_n(&wf_sdk_scans, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&wf_sdk_seen, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&wf_sdk_connecting, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&wf_sdk_ready, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&wf_sdk_error, ESP_OK, __ATOMIC_RELAXED);
    __atomic_store_n(&wf_sdk_enabled, true, __ATOMIC_RELEASE);
    return ESP_OK;
}

void wf_sdk_clear(void)
{
    /* Called only after the SDK has removed callbacks and joined its task. */
    __atomic_store_n(&wf_sdk_enabled, false, __ATOMIC_RELEASE);
}

void wf_sdk_status(WfSdkStatus *out)
{
    if (!out) return;
    out->scans = __atomic_load_n(&wf_sdk_scans, __ATOMIC_RELAXED);
    out->seen = __atomic_load_n(&wf_sdk_seen, __ATOMIC_RELAXED);
    out->connecting = __atomic_load_n(&wf_sdk_connecting, __ATOMIC_RELAXED);
    out->ready = __atomic_load_n(&wf_sdk_ready, __ATOMIC_RELAXED);
    out->error = __atomic_load_n(&wf_sdk_error, __ATOMIC_RELAXED);
}
