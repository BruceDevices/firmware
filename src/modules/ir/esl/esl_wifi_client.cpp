#include "esl_wifi_client.h"

#include "core/display.h"
#include "core/wifi/wifi_common.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <globals.h>
#include <new>
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

static HTTPClient *esl_wifi_new_http(NetworkClientSecure **out_secure) {
    *out_secure = new (std::nothrow) NetworkClientSecure();
    HTTPClient *http = new (std::nothrow) HTTPClient();
    if (*out_secure == nullptr || http == nullptr) {
        delete *out_secure;
        delete http;
        *out_secure = nullptr;
        return nullptr;
    }
    (*out_secure)->setInsecure();
    return http;
}

static void esl_wifi_free_http(HTTPClient *http, NetworkClientSecure *secure) {
    if (http != nullptr) {
        http->end();
        delete http;
    }
    delete secure;
}

static const char *esl_wifi_http_plugins(EslWifiManifest *out, const char *url) {
    NetworkClientSecure *secure = nullptr;
    HTTPClient *http = esl_wifi_new_http(&secure);
    if (http == nullptr) return "plugin fetch failed";

    http->setConnectTimeout(15000);
    http->setTimeout(20000);
    http->setUserAgent(ESL_WIFI_UA);
    if (!http->begin(*secure, url)) {
        esl_wifi_free_http(http, secure);
        return "plugin fetch failed";
    }

    const int code = http->GET();
    Serial.printf("esl wifi plugins GET %d heap %u\n", code, (unsigned)ESP.getFreeHeap());
    if (code != HTTP_CODE_OK) {
        esl_wifi_free_http(http, secure);
        return "plugin fetch failed";
    }

    String body = http->getString();
    esl_wifi_free_http(http, secure);
    if (!esl_wifi_parse_manifest(body.c_str(), out)) {
        return "plugin JSON parse failed";
    }
    return nullptr;
}

/* writeToStream sink. One-shot Stream::readBytes on NetworkClientSecure
 * stops at the first TLS record: read() returns -1 while the next record
 * is in flight, and NetworkClient::readBytes treats that as EOF. */
class EslHttpSink : public Stream {
public:
    uint8_t *buf;
    size_t cap;
    size_t len;

    EslHttpSink(uint8_t *b, size_t c) : buf(b), cap(c), len(0) {}

    size_t write(uint8_t c) override {
        if (len >= cap) return 0;
        buf[len++] = c;
        return 1;
    }
    size_t write(const uint8_t *d, size_t n) override {
        if (d == nullptr || n == 0) return 0;
        if (len + n > cap) n = cap - len;
        memcpy(buf + len, d, n);
        len += n;
        return n;
    }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
};

static const char *esl_wifi_http_render(const char *url, EslWifiRenderHeader *hdr, uint8_t **out_body,
                                        size_t *out_len) {
    NetworkClientSecure *secure = nullptr;
    HTTPClient *http = esl_wifi_new_http(&secure);
    if (http == nullptr) return "Render failed";

    http->setConnectTimeout(15000);
    http->setTimeout(25000);
    http->setUserAgent(ESL_WIFI_UA);
    if (!http->begin(*secure, url)) {
        esl_wifi_free_http(http, secure);
        return "Render failed";
    }

    const int code = http->GET();
    const int clen = http->getSize();
    Serial.printf("esl wifi render GET %d clen %d heap %u\n", code, clen, (unsigned)ESP.getFreeHeap());
    if (code != HTTP_CODE_OK) {
        esl_wifi_free_http(http, secure);
        return "Render failed";
    }

    size_t cap = 8u + ESL_WIFI_RENDER_MAX;
    if (clen > 8 && (uint32_t)clen <= cap) cap = (size_t)clen;

    uint8_t *all = (uint8_t *)ps_malloc(cap);
    if (all == nullptr) all = (uint8_t *)malloc(cap);
    if (all == nullptr) {
        esl_wifi_free_http(http, secure);
        return "Render failed";
    }

    EslHttpSink sink(all, cap);
    const int written = http->writeToStream(&sink);
    esl_wifi_free_http(http, secure);
    Serial.printf("esl wifi render bytes %d/%u\n", written, (unsigned)sink.len);

    if (written < 8 || sink.len < 8) {
        free(all);
        return "short header";
    }
    if (!esl_wifi_parse_render_header(all, hdr)) {
        free(all);
        return "bad header";
    }

    const uint32_t nbytes = esl_wifi_plane_bytes(hdr);
    if (nbytes == 0u || nbytes > ESL_WIFI_RENDER_MAX) {
        free(all);
        return "bad header";
    }
    if (sink.len < 8u + nbytes) {
        free(all);
        return "short body";
    }

    uint8_t *body = (uint8_t *)ps_malloc(nbytes);
    if (body == nullptr) body = (uint8_t *)malloc(nbytes);
    if (body == nullptr) {
        free(all);
        return "Render failed";
    }
    memcpy(body, all + 8, nbytes);
    free(all);

    *out_body = body;
    *out_len = (size_t)nbytes;
    return nullptr;
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

    const char *err = esl_wifi_http_plugins(out, url);
    if (err != nullptr) {
        displayError(err, true);
        return false;
    }
    return true;
}

bool esl_wifi_client_fetch_render(const char *plugin_id, uint16_t w, uint16_t h, uint8_t accent,
                                  const char *const *keys, const char *const *values, uint8_t n_params,
                                  EslWifiRenderHeader *out_hdr, uint8_t **out_body, size_t *out_len) {
    if (out_hdr == nullptr || out_body == nullptr || out_len == nullptr) {
        return false;
    }
    *out_body = nullptr;
    *out_len = 0;
    memset(out_hdr, 0, sizeof *out_hdr);

    if (!esl_wifi_client_ensure_sta()) return false;

    char url[768];
    if (!esl_wifi_build_render_url(url, sizeof url, ESL_WIFI_DEFAULT_URL, plugin_id, w, h, accent, keys,
                                   values, n_params)) {
        displayError("Render failed", true);
        return false;
    }

    const char *err = esl_wifi_http_render(url, out_hdr, out_body, out_len);
    if (err != nullptr) {
        displayError(err, true);
        return false;
    }
    return true;
}
