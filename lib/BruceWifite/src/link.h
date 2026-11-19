/* WIFITE driver transport; GPL-3.0-or-later. */
#pragma once
#include "core.h"
#include <esp_err.h>
typedef struct {
  bool (*accept)(const uint8_t bssid[6]);
  void (*ready)(void);
  void (*receive)(const uint8_t src[6], const uint8_t *eapol, size_t n);
} WfLinkCallbacks;
/* Calls below run on the firmware WLAN worker, while holding its radio lease.
 * Callbacks run on the driver task and must not block or call Wi-Fi APIs. */
esp_err_t wf_link_install(const WfLinkCallbacks *callbacks);
esp_err_t wf_link_connect(const WfTarget *target);
esp_err_t wf_link_send(const uint8_t *ethernet, size_t n);
void wf_link_disconnect(void);
void wf_link_uninstall(void);
