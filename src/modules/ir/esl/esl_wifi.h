/* Worker manifest + render-header parse for WiFi Plugins.
 *
 * No network. Field names match Flipper's companion JSON decoder in
 * TagTinker/esp32-wifi-fw/main/main.c. URL builders match cloud_client.c. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESL_WIFI_DEFAULT_URL "https://tagtinker.jhackerr.workers.dev"
#define ESL_WIFI_UA "TagTinker-WiFi/2.0"
#define ESL_WIFI_MAX_PLUGINS 8
#define ESL_WIFI_MAX_PARAMS 6
#define ESL_WIFI_MAX_OPTIONS 8

enum {
    ESL_WIFI_PARAM_STRING = 0,
    ESL_WIFI_PARAM_INT = 1,
    ESL_WIFI_PARAM_ENUM = 2,
    ESL_WIFI_PARAM_BOOL = 3,
};

typedef struct {
    uint16_t width, height, row_stride;
    uint8_t planes;
} EslWifiRenderHeader;

/* Flipper TagTinkerWifiPlugin / TtWifiParam field sizes. */
typedef struct {
    char key[24];
    char label[24];
    uint8_t type;
    char default_value[64];
    uint8_t option_count;
    char options[ESL_WIFI_MAX_OPTIONS][24];
    int32_t int_min;
    int32_t int_max;
} EslWifiParam;

typedef struct {
    char id[24];
    char name[40];
    char description[64];
    uint8_t accent_modes;
    uint8_t param_count;
    EslWifiParam params[ESL_WIFI_MAX_PARAMS];
} EslWifiPlugin;

typedef struct {
    uint8_t count;
    EslWifiPlugin plugins[ESL_WIFI_MAX_PLUGINS];
} EslWifiManifest;

/* LE u16 w, u16 h, u8 planes, u8 reserved, u16 stride.
 * Rejects planes other than 1 or 2, or zero w/h/stride. */
bool esl_wifi_parse_render_header(const uint8_t hdr[8], EslWifiRenderHeader *out);

uint32_t esl_wifi_plane_bytes(const EslWifiRenderHeader *hdr);

/* Root object with a "plugins" array. Caps: 8 plugins, 6 params, 8 options. */
bool esl_wifi_parse_manifest(const char *json, EslWifiManifest *out);

/* Flipper url_append_escaped: alnum / -_. pass through; else %XX lowercase. */
void esl_wifi_url_append_escaped(char *dst, size_t cap, size_t *pos,
                                 const char *s);

bool esl_wifi_build_plugins_url(char *dst, size_t cap, const char *base);

/* accent: 1=red, 2=yellow, else none. id and values are escaped. */
bool esl_wifi_build_render_url(char *dst, size_t cap, const char *base,
                               const char *plugin_id, uint16_t w, uint16_t h,
                               uint8_t accent, const char *const *keys,
                               const char *const *values, uint8_t n_params);

#ifdef __cplusplus
}
#endif
