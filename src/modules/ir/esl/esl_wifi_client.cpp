#include "esl_wifi_client.h"

#include "core/display.h"
#include "core/wifi/wifi_common.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <globals.h>
#include <stdlib.h>
#include <string.h>

#define ESL_WIFI_RENDER_MAX 262144u

bool esl_wifi_client_ensure_sta(void) {
    if (!wifiConnected) wifiConnectMenu();
    if (!wifiConnected) {
        displayError("WiFi not connected", true);
        return false;
    }
    return true;
}

bool esl_wifi_client_fetch_plugins(EslWifiManifest *out) {
    if (out == nullptr) return false;
    memset(out, 0, sizeof *out);
    if (!esl_wifi_client_ensure_sta()) return false;

    char url[256];
    if (!esl_wifi_build_plugins_url(url, sizeof url, ESL_WIFI_DEFAULT_URL)) {
        displayError("plugin fetch failed", true);
        return false;
    }

    HTTPClient http;
    http.setConnectTimeout(15000);
    http.setTimeout(20000);
    http.setUserAgent(ESL_WIFI_UA);
    if (!http.begin(url)) {
        displayError("plugin fetch failed", true);
        return false;
    }

    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        http.end();
        displayError("plugin fetch failed", true);
        return false;
    }

    String body = http.getString();
    http.end();
    if (!esl_wifi_parse_manifest(body.c_str(), out)) {
        displayError("plugin JSON parse failed", true);
        return false;
    }
    return true;
}

bool esl_wifi_client_fetch_render(const char *plugin_id, uint16_t w, uint16_t h,
                                  uint8_t accent, const char *const *keys,
                                  const char *const *values, uint8_t n_params,
                                  EslWifiRenderHeader *out_hdr, uint8_t **out_body,
                                  size_t *out_len) {
    if (out_hdr == nullptr || out_body == nullptr || out_len == nullptr) {
        return false;
    }
    *out_body = nullptr;
    *out_len = 0;
    memset(out_hdr, 0, sizeof *out_hdr);

    if (!esl_wifi_client_ensure_sta()) return false;

    char url[768];
    if (!esl_wifi_build_render_url(url, sizeof url, ESL_WIFI_DEFAULT_URL, plugin_id,
                                   w, h, accent, keys, values, n_params)) {
        displayError("Render failed", true);
        return false;
    }

    HTTPClient http;
    http.setConnectTimeout(15000);
    http.setTimeout(25000);
    http.setUserAgent(ESL_WIFI_UA);
    if (!http.begin(url)) {
        displayError("Render failed", true);
        return false;
    }

    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        http.end();
        displayError("Render failed", true);
        return false;
    }

    NetworkClient *stream = http.getStreamPtr();
    if (stream == nullptr) {
        http.end();
        displayError("short header", true);
        return false;
    }

    uint8_t hdr[8];
    const size_t hdr_got = stream->readBytes(hdr, sizeof hdr);
    if (hdr_got < sizeof hdr) {
        http.end();
        displayError("short header", true);
        return false;
    }
    if (!esl_wifi_parse_render_header(hdr, out_hdr)) {
        http.end();
        displayError("bad header", true);
        return false;
    }

    const uint32_t nbytes = esl_wifi_plane_bytes(out_hdr);
    if (nbytes == 0u || nbytes > ESL_WIFI_RENDER_MAX) {
        http.end();
        displayError("bad header", true);
        return false;
    }

    uint8_t *body = (uint8_t *)ps_malloc(nbytes);
    if (body == nullptr) body = (uint8_t *)malloc(nbytes);
    if (body == nullptr) {
        http.end();
        displayError("Render failed", true);
        return false;
    }

    const size_t got = stream->readBytes(body, nbytes);
    http.end();
    if (got < nbytes) {
        free(body);
        displayError("short body", true);
        return false;
    }

    *out_body = body;
    *out_len = (size_t)nbytes;
    return true;
}
