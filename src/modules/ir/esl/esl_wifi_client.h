#pragma once

#include "esl_wifi.h"

#include <stddef.h>
#include <stdint.h>

/* Prompt Bruce STA connect if down. Error + false if still disconnected. */
bool esl_wifi_client_ensure_sta(void);

/* GET {base}/plugins with UA TagTinker-WiFi/2.0. No TX. */
bool esl_wifi_client_fetch_plugins(EslWifiManifest *out);

/* GET {base}/render/{id}?w=&h=&accent=&key=val. Caller frees *out_body.
 * HTTP non-200 / short body / bad header → displayError, false, no TX. */
bool esl_wifi_client_fetch_render(const char *plugin_id, uint16_t w, uint16_t h,
                                  uint8_t accent, const char *const *keys,
                                  const char *const *values, uint8_t n_params,
                                  EslWifiRenderHeader *out_hdr, uint8_t **out_body,
                                  size_t *out_len);
