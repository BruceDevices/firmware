// SPDX-License-Identifier: GPL-3.0-or-later
//
// RTL433 for Bruce. Ported from the Flipper Zero firmware RTL433 app
// (applications_user/rtl433, GPL-3.0-or-later, rtl_433 contributors):
// the pulse capture/windowing pipeline and the dedicated decode worker
// mirror the original rtl_radio.c / rtl_engine.c design, retargeted from
// Flipper's subghz async RX to Bruce's native RfRxSession (RMT on GDO0 /
// GPIO ISR) and Bruce's native capture-list UI.
//
// Requires the lib/RTL433 decoder library (full rtl_433 catalogue).
#if !defined(LITE_VERSION) && !defined(RTL433_DISABLED)
#include "rf_rtl433.h"
#include "core/mykeyboard.h"
#include "core/scrollableTextArea.h"
#include "core/sd_functions.h"
#include "protocols/rf_decoder.h"
#include "protocols/rf_presets.h"
#include "rf_scan.h"  // rf_draw_capture_list / rf_clear_nav_state
#include "rf_utils.h" // initRfModule / deinitRfModule
#include "rtl_capture.h"
#include "rtl_engine.h"
#include <globals.h>
#include <algorithm>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace {
// ---------------------------------------------------------------------------
// Tuning constants (mirroring the original app where applicable).
// ---------------------------------------------------------------------------
constexpr size_t RTL_MAX_HISTORY = 32;      // capture list entries
constexpr size_t RTL_MAX_HISTORY_BYTES = 96 * 1024;
constexpr size_t RTL_QUEUE_FRAMES = 3;      // pending decode bursts
constexpr size_t RTL_QUEUE_RESULTS = 16;
constexpr size_t RTL_CAPTURE_SYMBOLS = 2048; // RMT symbols per burst
constexpr unsigned RTL_DECODER_STACK = 32 * 1024;
constexpr int RTL_FSK_THRESHOLD_DBM = -75;  // FSK carrier gate
constexpr uint32_t RTL_FSK_HOLD_US = 5000;  // carrier release hold
constexpr uint32_t RTL_PRETRIGGER_US = 4000; // buffered FSK prefix
constexpr unsigned RTL_PRETRIGGER_EDGES = 256;
constexpr uint32_t RTL_WINDOW_US = 250000;  // snapshot cadence
constexpr uint32_t RTL_DWELL_US = 2000000;  // auto-scan dwell per profile
constexpr uint32_t RTL_HOP_US = 3000000;    // frequency-hop dwell

// Bruce presets stand in for the original receiver profiles: OOK 270 kHz,
// OOK 650 kHz, FSK dev 2.38k, FSK dev 47.6k.
constexpr int RTL_PRESET_AUTO = -1;
const char *rtl_preset_names[4] = {"Ook270Async", "Ook650Async", "2FSKDev238Async", "2FSKDev476Async"};
const char *rtl_preset_labels[4] = {"AM270", "AM650", "FM238", "FM476"};
constexpr bool rtl_preset_fsk[4] = {false, false, true, true};
// Auto-scan order: most sensors are OOK 650/270; FSK profiles follow.
constexpr int rtl_auto_order[4] = {1, 0, 3, 2};

// ---------------------------------------------------------------------------
// Decode pipeline. Capture runs on Bruce's main loop; decoding runs on a
// dedicated FreeRTOS task — the full rtl_433 catalogue needs far more stack
// than the Arduino loop task provides (the original app likewise used a
// dedicated RTLDecode thread with a large stack).
// ---------------------------------------------------------------------------
struct RtlResult {
    String key;   // protocol + model + id + channel identity
    String label; // short row for the capture list
    String text;  // all decoded fields
    String json;  // full JSON record
    unsigned hits = 1;
    uint32_t frequency = 0;
    int rssi = 0;
    bool fsk = false;
    uint32_t time_ms = 0;
    size_t bytes() const { return key.length() + label.length() + text.length() + json.length(); }
};

RtlEngine rtlEngine;
QueueHandle_t rtlFrameQueue = nullptr;
QueueHandle_t rtlResultQueue = nullptr;
SemaphoreHandle_t rtlDoneSemaphore = nullptr;
TaskHandle_t rtlDecodeTask = nullptr;
std::atomic<bool> rtlRunning{false};
std::atomic<bool> rtlWindowPending{false};

static String fieldText(const data_t *data, const char *key) {
    const data_t *field = rtl_field(data, key);
    if (!field) return "";
    char buf[32];
    rtl_value_text(field, buf, sizeof buf);
    return String(buf);
}

// Called from the decode task by the rtl_433 engine for every decoded record.
// The engine frees `data` right after this returns, so everything needed is
// copied into a heap-allocated result and handed to the UI via a queue.
static void rtl_output(void *ctx, const r_device *decoder, const data_t *data, const RtlMeta *meta) {
    (void)ctx;
    if (!decoder || !data || !meta) return;

    RtlResult *result = new RtlResult();
    if (!result) return;

    const String model = fieldText(data, "model");
    result->key =
        String(decoder->name) + "|" + model + "|" + fieldText(data, "id") + "|" + fieldText(data, "channel");

    // Short label for the capture list: model, id/channel, then a few of the
    // most common sensor readings.
    String label = model.isEmpty() ? String(decoder->name) : model;
    const String id = fieldText(data, "id");
    if (!id.isEmpty()) label += " " + id;
    const String channel = fieldText(data, "channel");
    if (!channel.isEmpty()) label += " CH" + channel;
    struct Named {
        const char *key, *suffix;
    };
    static const Named named[] = {
        {"temperature_C", "C"},   {"temperature_F", "F"},  {"humidity", "%"},
        {"pressure_hPa", "hPa"},  {"pressure_kPa", "kPa"}, {"wind_avg_km_h", "km/h"},
        {"wind_avg_m_s", "m/s"},  {"rain_mm", "mm"},       {"depth_cm", "cm"},
        {"moisture", "%"},        {"uv", "UV"},            {"lux", "lx"},
    };
    unsigned added = 0;
    for (const auto &n : named) {
        if (added >= 3) break;
        const String value = fieldText(data, n.key);
        if (!value.isEmpty()) {
            label += " " + value + n.suffix;
            added++;
        }
    }

    String text = "Model: " + String(decoder->name) + "\n";
    for (const data_t *f = data; f; f = f->next) {
        char buf[64];
        rtl_value_text(f, buf, sizeof buf);
        text += String(f->key) + ": " + buf + "\n";
    }
    text += "Frequency: " + String((unsigned long)meta->frequency) + " Hz\n";
    text += "RSSI: " + String(meta->rssi) + " dBm\n";
    text += "Modulation: " + String(meta->fsk ? "FSK" : "OOK") + "\n";

    char *json = rtl_json(decoder, data, meta);
    if (json) {
        result->json = json;
        free(json);
    }

    result->label = label;
    result->text = text;
    result->frequency = meta->frequency;
    result->rssi = meta->rssi;
    result->fsk = meta->fsk;
    result->time_ms = (uint32_t)meta->time_ms;

    if (xQueueSend(rtlResultQueue, &result, 0) != pdTRUE) delete result;
}

static void rtl_yield(void *ctx) {
    (void)ctx;
    vTaskDelay(1);
}

static void decodeTask(void *) {
    for (;;) {
        RtlFrame *frame = nullptr;
        if (xQueueReceive(rtlFrameQueue, &frame, pdMS_TO_TICKS(250)) != pdTRUE) {
            if (!rtlRunning) break;
            continue;
        }
        if (!frame) break; // sentinel: stop requested
        rtl_engine_decode(&rtlEngine, &frame->pulses, &frame->meta);
        if (frame->window) rtlWindowPending = false;
        free(frame);
    }
    xSemaphoreGive(rtlDoneSemaphore);
    vTaskDelete(nullptr);
}

// ---------------------------------------------------------------------------
// The app. One static instance: the capture frame holds pulse_data_t
// (~10 KiB), which must not live on the Arduino loop task's small stack.
// ---------------------------------------------------------------------------
class Rtl433 {
    // --- settings ----------------------------------------------------------
    const float frequencies[6] = {315.0f, 433.92f, 433.65f, 434.25f, 434.42f, 868.35f};
    int frequencyIndex = 1;            // 433.92 MHz
    int presetIndex = RTL_PRESET_AUTO; // -1 = auto, else 0..3
    bool hopping = false;
    bool logToSd = false;

    // --- history -----------------------------------------------------------
    std::vector<RtlResult> history;
    int selected = 0, scrollOffset = 0;
    bool listDirty = true;

    // --- receiver state ----------------------------------------------------
    RfRxSession rx;
    RtlCapture cap;
    RtlFrame frame;
    bool radioActive = false;
    int activePreset = 0; // currently applied preset index
    float activeFrequency = 0.f;
    uint32_t lastEdgeUs = 0, windowAtUs = 0, tuneAtUs = 0, hopAtUs = 0, lastRssiUs = 0;
    int rssi = -127;
    // FSK carrier gating (idle FSK is a noise storm; OOK is not gated).
    bool gated = false, gateClosePending = false;
    uint32_t lowSinceUs = 0;
    struct PreEdge {
        uint32_t endUs;
        int duration; // signed, like RfRxSession output
    };
    PreEdge pretrigger[RTL_PRETRIGGER_EDGES];
    unsigned preHead = 0, preCount = 0;
    unsigned cycle = 0;

    bool cc1101() const { return bruceConfigPins.rfModule == CC1101_SPI_MODULE; }
    bool fskNow() const { return cc1101() && rtl_preset_fsk[activePreset]; }

    void resetCapture() {
        rtl_capture_reset(&cap, &frame);
        lastEdgeUs = windowAtUs = micros();
        preHead = preCount = 0;
    }

    void firstEdgeMeta() {
        frame.meta.frequency = (uint32_t)(activeFrequency * 1000000.f);
        frame.meta.time_ms = millis();
        frame.meta.rssi = rssi;
        frame.meta.fsk = fskNow();
        frame.meta.profile = activePreset;
        frame.meta.profile_known = cc1101();
        frame.meta.offline = false;
    }

    void publish(bool window) {
        if (window) {
            // Coalesce unfinished snapshots while a slow decode is running,
            // and never preview the ungated FSK noise floor.
            if (rtlWindowPending || (!gated && fskNow())) return;
            if (!rtl_capture_window(&cap, &frame)) return;
            rtlWindowPending = true;
        }
        RtlFrame *copy = (RtlFrame *)malloc(sizeof(RtlFrame));
        if (copy) {
            *copy = frame;
            if (xQueueSend(rtlFrameQueue, &copy, 0) != pdTRUE) {
                free(copy);
                if (window) rtlWindowPending = false;
            }
        } else if (window) {
            rtlWindowPending = false;
        }
    }

    void feedEdge(int duration) {
        if (!duration) return;
        const bool level = duration > 0;
        const uint32_t us = duration < 0 ? -(uint32_t)duration : (uint32_t)duration;
        // FSK idle is a noise storm: buffer edges while the carrier gate is
        // closed and replay the recent prefix when it opens.
        if (fskNow() && !gated) {
            pretrigger[preHead] = {micros(), duration};
            preHead = (preHead + 1) % RTL_PRETRIGGER_EDGES;
            if (preCount < RTL_PRETRIGGER_EDGES) preCount++;
            return;
        }
        if (!frame.pulses.num_pulses && !cap.high) firstEdgeMeta();
        if (rtl_capture_feed(&cap, &frame, level, us)) {
            publish(false);
            rtl_capture_reset(&cap, &frame);
            windowAtUs = micros();
        } else if (cap.invalid) {
            rtl_capture_reset(&cap, &frame);
            windowAtUs = micros();
        } else if (frame.pulses.num_pulses >= PD_MAX_PULSES) {
            publish(true);
            rtl_capture_rollover(&cap, &frame);
            windowAtUs = micros();
        }
    }

    void feedBatch(const std::vector<int> &durations) {
        for (int d : durations) feedEdge(d);
        lastEdgeUs = micros();
    }

    // Replays buffered edges that are recent enough to belong to the packet.
    void replayPretrigger(uint32_t nowUs) {
        const unsigned start = (preHead + RTL_PRETRIGGER_EDGES - preCount) % RTL_PRETRIGGER_EDGES;
        for (unsigned i = 0; i < preCount; i++) {
            const PreEdge &e = pretrigger[(start + i) % RTL_PRETRIGGER_EDGES];
            if (nowUs - e.endUs <= RTL_PRETRIGGER_US) feedEdge(e.duration);
        }
        preHead = preCount = 0;
    }

    void carrierGate(uint32_t nowUs) {
        if (!fskNow() || nowUs - lastRssiUs < 5000) return;
        lastRssiUs = nowUs;
        rssi = ELECHOUSE_cc1101.getRssi();
        if (frame.pulses.num_pulses || cap.high) {
            if (rssi > frame.meta.rssi) frame.meta.rssi = rssi;
        }
        const bool above = rssi >= RTL_FSK_THRESHOLD_DBM;
        if (above) lowSinceUs = 0;
        else if (!lowSinceUs) lowSinceUs = nowUs;
        const bool open = above || (gated && lowSinceUs && nowUs - lowSinceUs < RTL_FSK_HOLD_US);
        gateClosePending = gated && !open;
        if (!gated && open) {
            replayPretrigger(nowUs);
            gated = true;
        }
    }

    void closeGate(uint32_t nowUs) {
        if (rtl_capture_finish(&cap, &frame, RTL_GAP_US)) publish(false);
        rtl_capture_reset(&cap, &frame);
        gated = gateClosePending = false;
        preHead = preCount = 0;
        lastEdgeUs = windowAtUs = nowUs;
    }

    // Retune the radio to a preset/frequency and restart the capture session.
    // An AUTO preset resolves to the current auto-scan step.
    bool tune(int presetIdx, float frequency) {
        rx.end();
        radioActive = false;
        rtl_capture_reset(&cap, &frame);
        rtlWindowPending = false;
        gated = gateClosePending = false;
        preHead = preCount = 0;
        if (presetIdx == RTL_PRESET_AUTO) presetIdx = rtl_auto_order[cycle % 4];
        if (!initRfModule("rx", frequency)) return false;
        const RfPreset *preset = nullptr;
        if (cc1101()) {
            preset = rf_find_preset(String(rtl_preset_names[presetIdx]));
            if (preset) {
                // Same parameter application as the native receive path.
                ELECHOUSE_cc1101.setSidle();
                ELECHOUSE_cc1101.setModulation(preset->modulation);
                ELECHOUSE_cc1101.setDeviation(preset->deviation ? preset->deviation : 1.58f);
                ELECHOUSE_cc1101.setRxBW(preset->rxBW ? preset->rxBW : 270.83f);
                ELECHOUSE_cc1101.setDRate(preset->dataRate ? preset->dataRate : 10.f);
                ELECHOUSE_cc1101.setPktFormat(3);
                ELECHOUSE_cc1101.SetRx();
            }
        }
        if (!rx.begin(true, RTL_CAPTURE_SYMBOLS)) {
            deinitRfModule();
            return false;
        }
        activeFrequency = frequency;
        activePreset = presetIdx;
        rssi = -127;
        lastRssiUs = 0;
        resetCapture();
        tuneAtUs = hopAtUs = micros();
        return true;
    }

    // Advance the auto preset scan (and optional frequency hopping). Retuning
    // is only done while the channel is idle so bursts are not cut.
    void scheduler(uint32_t nowUs) {
        const bool burst = frame.pulses.num_pulses || cap.high || gated;
        const bool idle = !burst && !rtlWindowPending;
        if (cc1101() && presetIndex == RTL_PRESET_AUTO && idle && nowUs - tuneAtUs >= RTL_DWELL_US) {
            cycle++;
            if (!tune(RTL_PRESET_AUTO, frequencies[frequencyIndex])) tuneAtUs = micros(); // retry later
            return;
        }
        if (hopping && cc1101() && idle && nowUs - hopAtUs >= RTL_HOP_US) {
            frequencyIndex = (frequencyIndex + 1) % 6;
            if (!tune(presetIndex, frequencies[frequencyIndex])) tuneAtUs = micros(); // retry later
        }
    }

    // --- history -----------------------------------------------------------
    size_t historyBytes() const {
        size_t bytes = 0;
        for (const auto &r : history) bytes += r.bytes();
        return bytes;
    }

    void drainResults() {
        RtlResult *result = nullptr;
        bool dirty = false;
        while (xQueueReceive(rtlResultQueue, &result, 0) == pdTRUE) {
            if (!result) continue;
            remember(*result);
            delete result;
            dirty = true;
        }
        if (dirty) listDirty = true;
    }

    void remember(RtlResult &incoming) {
        // Repeated receptions of the same sensor update the existing row and
        // recency instead of filling the list with identical entries.
        for (auto it = history.begin(); it != history.end(); ++it) {
            if (it->key == incoming.key) {
                incoming.hits = it->hits + 1;
                history.erase(it);
                break;
            }
        }
        history.push_back(incoming);
        while (history.size() > RTL_MAX_HISTORY || historyBytes() > RTL_MAX_HISTORY_BYTES)
            history.erase(history.begin());
        if (logToSd) appendLog(history.back());
    }

    void appendLog(const RtlResult &result) {
        FS *fs = nullptr;
        if (!getFsStorage(fs)) return;
        if (!fs->exists("/BruceRF")) fs->mkdir("/BruceRF");
        if (!fs->exists("/BruceRF/RTL433")) fs->mkdir("/BruceRF/RTL433");
        File file = fs->open("/BruceRF/RTL433/log.jsonl", FILE_APPEND);
        if (!file) return;
        file.print(result.json);
        file.print("\n");
        file.close();
    }

    // --- UI ---------------------------------------------------------------
    String captureLabel(int index) const {
        const RtlResult &r = history[index];
        String label = String(index + 1) + ". " + r.label;
        if (r.hits > 1) label += " x" + String(r.hits);
        return label;
    }

    void drawCaptureBackground(const RtlResult *entry = nullptr) {
        std::vector<String> rows;
        int sel = entry ? -1 : std::min(selected, std::max(0, int(history.size()) - 1));
        for (size_t i = 0; i < history.size(); ++i) {
            rows.push_back(captureLabel(int(i)));
            if (entry && history[i].key == entry->key) sel = int(i);
        }
        if (entry && sel < 0) {
            sel = int(rows.size());
            rows.push_back(entry->label);
        }
        String modulation;
        if (!cc1101()) modulation = "OOK";
        else {
            modulation = rtl_preset_labels[activePreset];
            if (presetIndex == RTL_PRESET_AUTO) modulation += " Auto";
        }
        rf_draw_capture_list(
            "RTL433 RX", String(activeFrequency) + "MHz " + modulation,
            "Sensors: " + String(history.size()) + (history.empty() ? "  SEL: settings" : "  SEL: signal"), rows, sel,
            scrollOffset, std::max(size_t(12), rows.size())
        );
        listDirty = false;
    }

    // --- shared UI helpers (same pattern as the native ProtoPirate app) ----
    static int choose(const String &title, const std::vector<String> &items, int initial = 0, uint8_t menuType = MENU_TYPE_SUBMENU) {
        int chosen = -1;
        std::vector<Option> choices;
        for (size_t i = 0; i < items.size(); i++) choices.emplace_back(items[i], [&, i]() { chosen = (int)i; });
        rf_clear_nav_state();
        returnToMenu = false;
        loopOptions(choices, menuType, title.c_str(), initial, false, false, 0, true, menuType == MENU_TYPE_REGULAR);
        rf_clear_nav_state();
        returnToMenu = false;
        return chosen;
    }

    static void view(const String &title, const String &text) {
        ScrollableTextArea area(title);
        area.fromString(text);
        area.show();
        delay(170);
    }

    static String pretty(const String &json) {
        if (json.isEmpty()) return "";
        char *pretty = rtl_pretty_json(json.c_str());
        String out = pretty ? pretty : json;
        free(pretty);
        return out;
    }

    static String safeName(const String &key) {
        String name = "RTL433_" + key;
        name.replace("|", "_");
        name.replace("/", "_");
        name.replace("\\", "_");
        name.replace(":", "_");
        name.replace("*", "_");
        name.replace("?", "_");
        name.replace("\"", "_");
        name.replace("<", "_");
        name.replace(">", "_");
        name.replace(" ", "_");
        return name;
    }

    void saveResult(const RtlResult &entry) {
        if (entry.json.isEmpty()) {
            displayError("No decoded data", true);
            return;
        }
        FS *fs = nullptr;
        if (!getFsStorage(fs)) return;
        if (!fs->exists("/BruceRF")) fs->mkdir("/BruceRF");
        if (!fs->exists("/BruceRF/RTL433")) fs->mkdir("/BruceRF/RTL433");
        File file = createNewFile(fs, "/BruceRF/RTL433", safeName(entry.key) + ".json");
        if (!file) {
            displayError("Cannot create file", true);
            return;
        }
        const String path = file.path();
        const size_t written = file.print(entry.json);
        file.close();
        if (written != entry.json.length()) {
            displayError("Incomplete write", true);
            return;
        }
        displaySuccess("Saved", false);
        view("Saved file", path);
    }

    void openEntry(size_t index) {
        const RtlResult &entry = history[index];
        for (;;) {
            drawCaptureBackground(&entry);
            int action = choose(entry.label, {"View decoded fields", "Save JSON to SD", "Back"}, 0, MENU_TYPE_REGULAR);
            if (action < 0 || action == 2) return;
            if (action == 0) view(entry.label, entry.text + "\nJSON:\n" + pretty(entry.json));
            else if (action == 1) saveResult(entry);
        }
    }

    void settings() {
        for (;;) {
            drawCaptureBackground();
            int action = choose(
                "Receiver settings",
                {"Frequency: " + String(frequencies[frequencyIndex]) + " MHz",
                 "Modulation: " + String(presetIndex < 0 ? "Auto" : rtl_preset_labels[presetIndex]),
                 "Frequency hopping: " + String(hopping ? "ON" : "OFF"),
                 "SD logging: " + String(logToSd ? "ON" : "OFF"),
                 "Back"},
                0, MENU_TYPE_REGULAR
            );
            if (action < 0 || action == 4) return;
            if (action == 0) {
                std::vector<String> list;
                for (float f : frequencies) list.push_back(String(f) + " MHz");
                drawCaptureBackground();
                int n = choose("Frequency", list, frequencyIndex, MENU_TYPE_REGULAR);
                if (n >= 0) frequencyIndex = n;
            } else if (action == 1) {
                std::vector<String> list = {"Auto"};
                for (const char *l : rtl_preset_labels) list.push_back(l);
                drawCaptureBackground();
                int n = choose("Modulation", list, presetIndex + 1, MENU_TYPE_REGULAR);
                if (n >= 0) presetIndex = n - 1;
            } else if (action == 2) hopping = !hopping;
            else if (action == 3) logToSd = !logToSd;
        }
    }

    // --- receive loop -------------------------------------------------------
    void receive() {
        const float frequency = cc1101() ? frequencies[frequencyIndex] : bruceConfigPins.rfFreq;
        if (!frequency || frequency < 300 || frequency > 928) {
            displayError("Invalid RX frequency", true);
            return;
        }
        if (!tune(presetIndex, frequency)) {
            displayError("RF init failed", true);
            return;
        }
        selected = history.empty() ? 0 : int(history.size()) - 1;
        scrollOffset = 0;
        listDirty = true;
        rf_clear_nav_state();

        for (;;) {
#ifdef HAS_3_BUTTONS
            if (EscPress && PrevPress) EscPress = false;
#endif
            if (check(EscPress)) {
                rx.end();
                if (radioActive) deinitRfModule();
                radioActive = false;
                return;
            }
            if (check(SelPress)) {
                // Pause the radio while a menu is open (shared SPI bus).
                rx.end();
                if (radioActive) deinitRfModule();
                radioActive = false;
                rf_clear_nav_state();
                returnToMenu = false;
                if (history.empty()) settings();
                else openEntry(size_t(selected));
                if (!tune(presetIndex, cc1101() ? frequencies[frequencyIndex] : bruceConfigPins.rfFreq)) {
                    displayError("RF init failed", true);
                    return;
                }
                continue;
            }
            bool up = check(PrevPress);
            if (check(UpPress)) up = true;
            bool down = check(NextPress);
            if (check(DownPress)) down = true;
            if (!history.empty() && (up || down)) {
                selected = (selected + (up ? -1 : 1) + int(history.size())) % int(history.size());
                listDirty = true;
            }

            const uint32_t nowUs = micros();
            // FSK carrier gate upkeep.
            carrierGate(nowUs);
            if (gateClosePending && !rtlWindowPending) closeGate(nowUs);

            // Capture and feed edges.
            std::vector<int> durations;
            if (rx.poll(durations)) {
                feedBatch(durations);
            } else if ((frame.pulses.num_pulses || cap.high) && nowUs - lastEdgeUs >= RTL_GAP_US) {
                // Quiet channel: finalize the burst with a synthetic gap.
                if (rtl_capture_finish(&cap, &frame, RTL_GAP_US)) publish(false);
                rtl_capture_reset(&cap, &frame);
                lastEdgeUs = windowAtUs = nowUs;
            }
            // Snapshot complete pairs so repeat-dependent frames decode without
            // waiting a full 100 ms gap (as in the original app).
            if ((activePreset < 2 || gated) && !rtlWindowPending && micros() - windowAtUs >= RTL_WINDOW_US)
                publish(true);

            drainResults();
            scheduler(micros());
            if (listDirty) drawCaptureBackground();
            delay(2);
        }
    }

    void catalogue() {
        String text = "rtl_433 decoders compiled in: " + String((unsigned)rtl_catalog_count()) + "\n";
        for (size_t i = 0; i < rtl_catalog_count(); ++i) {
            const r_device *d = rtl_catalog_get(i);
            if (!d || d->disabled == 3 || !d->decode_fn) continue;
            const bool runnable = rtl_modulation_name(d->modulation) != nullptr;
            text += String(d->name) + (d->disabled == 0 && runnable ? "\n" : " (opt)\n");
        }
        view("Protocol catalogue", text);
    }

  public:
    void run() {
        // Engine + worker lifecycle
        rtlFrameQueue = xQueueCreate(RTL_QUEUE_FRAMES, sizeof(RtlFrame *));
        rtlResultQueue = xQueueCreate(RTL_QUEUE_RESULTS, sizeof(RtlResult *));
        rtlDoneSemaphore = xSemaphoreCreateBinary();
        bool engineOk = rtlFrameQueue && rtlResultQueue && rtlDoneSemaphore &&
                        rtl_engine_init(&rtlEngine, rtl_output, rtl_yield, nullptr);
        if (engineOk) {
            rtlRunning = true;
            engineOk = xTaskCreate(decodeTask, "RTLDecode", RTL_DECODER_STACK, nullptr, 2, &rtlDecodeTask) == pdPASS;
        }
        if (!engineOk) {
            displayError("RTL433 engine failed to start", true);
            stopEngine();
            return;
        }

        for (;;) {
            int action = choose(
                "RTL433 / sensor RX",
                {"Receive sensors", "Settings", "Protocol catalogue", "About / limitations", "Exit"},
                0, MENU_TYPE_REGULAR
            );
            if (action < 0 || action == 4) break;
            if (action == 0) receive();
            else if (action == 1) settings();
            else if (action == 2) catalogue();
            else
                view(
                    "About / limitations",
                    "rtl_433 full catalogue on Bruce native RF.\n"
                    "Ported from the Flipper RTL433 app (GPL-3.0).\n\n"
                    "Decodes OOK + FSK sensor protocols via CC1101\n"
                    "(or a plain 433 GPIO receiver, OOK only).\n"
                    "Auto mode cycles AM650/AM270/FM476/FM238.\n"
                    "FSK listening is carrier-gated by RSSI.\n\n"
                    "Credits: rtl_433 contributors, Christian W. Zuckschwerdt,\n"
                    "Benjamin Larsson and the Flipper ESP32 port authors."
                );
        }

        rx.end();
        if (radioActive) deinitRfModule();
        stopEngine();
        returnToMenu = true;
    }

    void stopEngine() {
        rtlRunning = false;
        if (rtlDecodeTask) {
            RtlFrame *sentinel = nullptr;
            xQueueSend(rtlFrameQueue, &sentinel, 0);
            xSemaphoreTake(rtlDoneSemaphore, pdMS_TO_TICKS(10000));
            rtlDecodeTask = nullptr;
        }
        rtl_engine_free(&rtlEngine);
        if (rtlResultQueue) {
            RtlResult *result = nullptr;
            while (xQueueReceive(rtlResultQueue, &result, 0) == pdTRUE) delete result;
            vQueueDelete(rtlResultQueue);
            rtlResultQueue = nullptr;
        }
        if (rtlFrameQueue) {
            RtlFrame *frame = nullptr;
            while (xQueueReceive(rtlFrameQueue, &frame, 0) == pdTRUE) free(frame);
            vQueueDelete(rtlFrameQueue);
            rtlFrameQueue = nullptr;
        }
        if (rtlDoneSemaphore) {
            vSemaphoreDelete(rtlDoneSemaphore);
            rtlDoneSemaphore = nullptr;
        }
    }
};
} // namespace

void rf_rtl433() {
    static Rtl433 app; // holds a ~10 KiB pulse frame; keep off the task stack
    app.run();
}
#endif
