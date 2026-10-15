#include "esl_wifi.h"
#include "test_util.h"
#include <string.h>

static void wr_le16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)(v >> 8);
}

static void test_constants(void) {
    CHECK_STR(ESL_WIFI_DEFAULT_URL, "https://tagtinker.jhackerr.workers.dev");
    CHECK_STR(ESL_WIFI_UA, "TagTinker-WiFi/2.0");
    CHECK_EQ(ESL_WIFI_MAX_PLUGINS, 8);
    CHECK_EQ(ESL_WIFI_MAX_PARAMS, 6);
    CHECK_EQ(ESL_WIFI_MAX_OPTIONS, 8);
    CHECK_EQ(ESL_WIFI_PARAM_STRING, 0);
    CHECK_EQ(ESL_WIFI_PARAM_INT, 1);
    CHECK_EQ(ESL_WIFI_PARAM_ENUM, 2);
    CHECK_EQ(ESL_WIFI_PARAM_BOOL, 3);
}

static void test_parse_render_header_ok(void) {
    uint8_t hdr[8] = {0};
    EslWifiRenderHeader out;

    /* Glass Color 2.6: 296×152, 1 plane, BMP stride 40. */
    wr_le16(&hdr[0], 296);
    wr_le16(&hdr[2], 152);
    hdr[4] = 1;
    hdr[5] = 0xAB; /* reserved ignored */
    wr_le16(&hdr[6], 40);
    CHECK(esl_wifi_parse_render_header(hdr, &out));
    CHECK_EQ(out.width, 296);
    CHECK_EQ(out.height, 152);
    CHECK_EQ(out.planes, 1);
    CHECK_EQ(out.row_stride, 40);
    CHECK_EQ(esl_wifi_plane_bytes(&out), 40u * 152u * 1u);

    /* Two-plane wire-sized source. */
    wr_le16(&hdr[0], 152);
    wr_le16(&hdr[2], 296);
    hdr[4] = 2;
    wr_le16(&hdr[6], 20);
    CHECK(esl_wifi_parse_render_header(hdr, &out));
    CHECK_EQ(out.width, 152);
    CHECK_EQ(out.height, 296);
    CHECK_EQ(out.planes, 2);
    CHECK_EQ(out.row_stride, 20);
    CHECK_EQ(esl_wifi_plane_bytes(&out), 20u * 296u * 2u);
}

static void test_parse_render_header_reject(void) {
    uint8_t hdr[8] = {0};
    EslWifiRenderHeader out;
    wr_le16(&hdr[0], 296);
    wr_le16(&hdr[2], 152);
    hdr[4] = 1;
    wr_le16(&hdr[6], 40);

    CHECK(esl_wifi_parse_render_header(hdr, &out));

    hdr[4] = 0;
    CHECK(!esl_wifi_parse_render_header(hdr, &out));
    hdr[4] = 3;
    CHECK(!esl_wifi_parse_render_header(hdr, &out));
    hdr[4] = 1;

    wr_le16(&hdr[0], 0);
    CHECK(!esl_wifi_parse_render_header(hdr, &out));
    wr_le16(&hdr[0], 296);

    wr_le16(&hdr[2], 0);
    CHECK(!esl_wifi_parse_render_header(hdr, &out));
    wr_le16(&hdr[2], 152);

    wr_le16(&hdr[6], 0);
    CHECK(!esl_wifi_parse_render_header(hdr, &out));
    wr_le16(&hdr[6], 40);

    CHECK(!esl_wifi_parse_render_header(NULL, &out));
    CHECK(!esl_wifi_parse_render_header(hdr, NULL));
}

/* Keys match Flipper companion emit_plugin_from_json in
 * TagTinker/esp32-wifi-fw/main/main.c (id, name, description, accent_modes,
 * params[].key/label/type/default/options/min/max). */
static const char k_fixture[] =
    "{"
    "\"plugins\":["
    "{"
    "\"id\":\"crypto\","
    "\"name\":\"Crypto Price\","
    "\"description\":\"Editorial price card with sparkline\","
    "\"accent_modes\":7,"
    "\"params\":["
    "{\"key\":\"symbol\",\"label\":\"Coin\",\"type\":\"enum\","
    "\"default\":\"BTC\","
    "\"options\":[\"BTC\",\"ETH\",\"SOL\",\"XRP\",\"DOGE\",\"ADA\",\"BNB\","
    "\"LINK\",\"EXTRA\"]},"
    "{\"key\":\"currency\",\"label\":\"Currency\",\"type\":\"enum\","
    "\"default\":\"USD\",\"options\":[\"USD\",\"EUR\",\"GBP\"]},"
    "{\"key\":\"range\",\"label\":\"Range\",\"type\":\"enum\","
    "\"default\":\"24H\",\"options\":[\"24H\",\"7D\",\"30D\"]}"
    "]"
    "},"
    "{"
    "\"id\":\"weather\","
    "\"name\":\"Weather\","
    "\"description\":\"Live weather card with forecast\","
    "\"accent_modes\":7,"
    "\"params\":["
    "{\"key\":\"location\",\"label\":\"Location\",\"type\":\"string\","
    "\"default\":\"Paris\"},"
    "{\"key\":\"units\",\"label\":\"Units\",\"type\":\"enum\","
    "\"default\":\"C\",\"options\":[\"C\",\"F\"]}"
    "]"
    "},"
    "{"
    "\"id\":\"gauge\","
    "\"name\":\"Gauge\","
    "\"description\":\"int and bool params\","
    "\"accent_modes\":1,"
    "\"extra\":\"ignored\","
    "\"params\":["
    "{\"key\":\"level\",\"label\":\"Level\",\"type\":\"int\","
    "\"default\":\"50\",\"min\":0,\"max\":100},"
    "{\"key\":\"bold\",\"label\":\"Bold\",\"type\":\"bool\","
    "\"default\":\"0\"},"
    "{\"key\":\"note\",\"label\":\"Note\",\"type\":\"weird\","
    "\"default\":1}"
    "]"
    "}"
    "]"
    "}";

static void test_parse_manifest_fixture(void) {
    EslWifiManifest m;
    memset(&m, 0xFF, sizeof m);
    CHECK(esl_wifi_parse_manifest(k_fixture, &m));
    CHECK_EQ(m.count, 3);

    CHECK_STR(m.plugins[0].id, "crypto");
    CHECK_STR(m.plugins[0].name, "Crypto Price");
    CHECK_STR(m.plugins[0].description, "Editorial price card with sparkline");
    CHECK_EQ(m.plugins[0].accent_modes, 7);
    CHECK_EQ(m.plugins[0].param_count, 3);
    CHECK_STR(m.plugins[0].params[0].key, "symbol");
    CHECK_STR(m.plugins[0].params[0].label, "Coin");
    CHECK_EQ(m.plugins[0].params[0].type, ESL_WIFI_PARAM_ENUM);
    CHECK_STR(m.plugins[0].params[0].default_value, "BTC");
    /* Worker may advertise more than 8 enum options; Flipper caps at 8. */
    CHECK_EQ(m.plugins[0].params[0].option_count, 8);
    CHECK_STR(m.plugins[0].params[0].options[0], "BTC");
    CHECK_STR(m.plugins[0].params[0].options[7], "LINK");
    CHECK_STR(m.plugins[0].params[1].key, "currency");
    CHECK_STR(m.plugins[0].params[2].key, "range");

    CHECK_STR(m.plugins[1].id, "weather");
    CHECK_STR(m.plugins[1].name, "Weather");
    CHECK_EQ(m.plugins[1].param_count, 2);
    CHECK_EQ(m.plugins[1].params[0].type, ESL_WIFI_PARAM_STRING);
    CHECK_STR(m.plugins[1].params[0].default_value, "Paris");
    CHECK_EQ(m.plugins[1].params[1].type, ESL_WIFI_PARAM_ENUM);
    CHECK_STR(m.plugins[1].params[1].options[1], "F");

    CHECK_STR(m.plugins[2].id, "gauge");
    CHECK_EQ(m.plugins[2].accent_modes, 1);
    CHECK_EQ(m.plugins[2].param_count, 3);
    CHECK_EQ(m.plugins[2].params[0].type, ESL_WIFI_PARAM_INT);
    CHECK_EQ(m.plugins[2].params[0].int_min, 0);
    CHECK_EQ(m.plugins[2].params[0].int_max, 100);
    CHECK_STR(m.plugins[2].params[0].default_value, "50");
    CHECK_EQ(m.plugins[2].params[1].type, ESL_WIFI_PARAM_BOOL);
    CHECK_STR(m.plugins[2].params[1].default_value, "0");
    /* Unknown type → string (Flipper param_type_from). Non-string default → "". */
    CHECK_EQ(m.plugins[2].params[2].type, ESL_WIFI_PARAM_STRING);
    CHECK_STR(m.plugins[2].params[2].default_value, "");
}

static void test_parse_manifest_caps_and_defaults(void) {
    /* 9 plugins, 7 params, missing accent_modes / min / max. */
    static const char json[] =
        "{\"plugins\":["
        "{\"id\":\"p0\",\"name\":\"N0\",\"params\":[]},"
        "{\"id\":\"p1\",\"name\":\"N1\",\"params\":[]},"
        "{\"id\":\"p2\",\"name\":\"N2\",\"params\":[]},"
        "{\"id\":\"p3\",\"name\":\"N3\",\"params\":[]},"
        "{\"id\":\"p4\",\"name\":\"N4\",\"params\":[]},"
        "{\"id\":\"p5\",\"name\":\"N5\",\"params\":[]},"
        "{\"id\":\"p6\",\"name\":\"N6\",\"params\":[]},"
        "{\"id\":\"p7\",\"name\":\"N7\",\"params\":[]},"
        "{\"id\":\"p8\",\"name\":\"N8\",\"params\":[]}"
        "]}";
    EslWifiManifest m;
    CHECK(esl_wifi_parse_manifest(json, &m));
    CHECK_EQ(m.count, 8);
    CHECK_STR(m.plugins[0].id, "p0");
    CHECK_STR(m.plugins[7].id, "p7");
    CHECK_EQ(m.plugins[0].accent_modes, 1); /* Flipper default */
    CHECK_EQ(m.plugins[0].param_count, 0);

    static const char many_params[] =
        "{\"plugins\":[{\"id\":\"x\",\"name\":\"X\",\"params\":["
        "{\"key\":\"a\",\"label\":\"A\",\"type\":\"int\"},"
        "{\"key\":\"b\",\"label\":\"B\",\"type\":\"int\"},"
        "{\"key\":\"c\",\"label\":\"C\",\"type\":\"int\"},"
        "{\"key\":\"d\",\"label\":\"D\",\"type\":\"int\"},"
        "{\"key\":\"e\",\"label\":\"E\",\"type\":\"int\"},"
        "{\"key\":\"f\",\"label\":\"F\",\"type\":\"int\"},"
        "{\"key\":\"g\",\"label\":\"G\",\"type\":\"int\"}"
        "]}]}";
    CHECK(esl_wifi_parse_manifest(many_params, &m));
    CHECK_EQ(m.count, 1);
    CHECK_EQ(m.plugins[0].param_count, 6);
    CHECK_STR(m.plugins[0].params[5].key, "f");
    CHECK_EQ(m.plugins[0].params[0].int_min, 0);
    CHECK_EQ(m.plugins[0].params[0].int_max, 100);
}

static void test_parse_manifest_reject(void) {
    EslWifiManifest m;
    CHECK(!esl_wifi_parse_manifest(NULL, &m));
    CHECK(!esl_wifi_parse_manifest("{}", NULL));
    CHECK(!esl_wifi_parse_manifest("", &m));
    CHECK(!esl_wifi_parse_manifest("[1,2]", &m));
    CHECK(!esl_wifi_parse_manifest("{\"plugins\":", &m));
    CHECK(!esl_wifi_parse_manifest("not json", &m));

    CHECK(esl_wifi_parse_manifest("{}", &m));
    CHECK_EQ(m.count, 0);
    CHECK(esl_wifi_parse_manifest("{\"plugins\":[]}", &m));
    CHECK_EQ(m.count, 0);
}

static void test_url_escape_and_build(void) {
    char buf[128];
    size_t pos = 0;
    buf[0] = 0;
    esl_wifi_url_append_escaped(buf, sizeof buf, &pos, "Hello World!");
    CHECK_STR(buf, "Hello%20World%21");

    pos = 0;
    esl_wifi_url_append_escaped(buf, sizeof buf, &pos, "A-Z_a.z09");
    CHECK_STR(buf, "A-Z_a.z09");

    CHECK(esl_wifi_build_plugins_url(buf, sizeof buf, ESL_WIFI_DEFAULT_URL));
    CHECK_STR(buf, "https://tagtinker.jhackerr.workers.dev/plugins");

    const char *keys[] = {"symbol", "city name"};
    const char *vals[] = {"BTC", "São Paulo"};
    CHECK(esl_wifi_build_render_url(buf, sizeof buf, ESL_WIFI_DEFAULT_URL,
                                    "crypto", 296, 152, 1, keys, vals, 2));
    CHECK_STR(buf, "https://tagtinker.jhackerr.workers.dev/render/"
                   "crypto?w=296&h=152&accent=red&symbol=BTC&city%20name=S"
                   "%c3%a3o%20Paulo");

    CHECK(esl_wifi_build_render_url(buf, sizeof buf, ESL_WIFI_DEFAULT_URL,
                                    "weather", 208, 112, 2, NULL, NULL, 0));
    CHECK_STR(buf, "https://tagtinker.jhackerr.workers.dev/render/"
                   "weather?w=208&h=112&accent=yellow");

    CHECK(esl_wifi_build_render_url(buf, sizeof buf, ESL_WIFI_DEFAULT_URL,
                                    "id", 10, 10, 0, NULL, NULL, 0));
    CHECK_STR(buf, "https://tagtinker.jhackerr.workers.dev/render/"
                   "id?w=10&h=10&accent=none");
}

int main(void) {
    test_constants();
    test_parse_render_header_ok();
    test_parse_render_header_reject();
    test_parse_manifest_fixture();
    test_parse_manifest_caps_and_defaults();
    test_parse_manifest_reject();
    test_url_escape_and_build();
    TEST_REPORT("test_wifi");
}
