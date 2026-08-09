#include "dual_detect.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/settings.h"
#include "core/type_convertion.h"
#include "modules/ir/custom_ir.h"
#include "modules/ir/ir_read.h"
#include "modules/rf/rf_scan.h"
#include "modules/rf/rf_send.h"
#include "modules/rf/rf_utils.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <math.h>
#include <globals.h>

// ============================================================================
// RF + IR dual-band detector.
//
// Spawns two FreeRTOS tasks that listen AT THE SAME TIME:
//   - RF  : CC1101 RMT RX (or M5 GPIO ISR) via RfRxSession.
//   - IR  : GPIO ISR via IrRead::loop_headless.
// The first band that fires stops; you then Replay / Save / Discard each
// capture from the result menu. ESC always exits back to the IR menu.
//
// Optional AUTO-SAVE mode (toggle in [NEXT] -> "Auto-save"):
// every capture is stored automatically under /BruceIR or /BruceRF with an
// auto-generated name - no filename keyboard, no menu interaction. A signal is
// only written once the NEXT, DIFFERENT one arrives (repeats of the same
// signal are skipped - one file per distinct button); the last capture is
// flushed on the next capture or when leaving the detector. A short result
// flash confirms each capture. The Replay/Save/Discard menu is skipped while
// this is on.
// ============================================================================

namespace {

const float DUAL_FREQS[] = {315.0f, 433.92f, 868.35f, 915.0f};
const int DUAL_WINDOW_MIN = 5;
const int DUAL_WINDOW_MAX = 15;

enum { PRESS_NONE = 0, PRESS_ESC, PRESS_NEXT };

typedef struct {
    int window;            // listen window in seconds
    bool raw;              // capture raw on both bands
    String rf;             // .sub file content or ""
    String ir;             // IR file content or ""
    SemaphoreHandle_t sem; // counting(2), given once per finished task
    volatile bool abort;   // set by the caller to stop listening early
} DualCtx;

// --- Detector state ---------------------------------------------------------
bool _rfRaw = false;
bool _irRaw = false;
int _win = 8;
int _rfHits = 0;
int _irHits = 0;
int _cycles = 0;
int _specX = 0, _specY = 0, _specW = 0, _specH = 0;

// --- Signal queue (optional) ---------------------------------------------------
// When enabled, every capture is kept in a bounded history so a signal that
// arrives right after another one is not lost: the newest shows the result menu
// while the previous ones stay available (replay/save) from the queue browser.
struct QueuedSignal {
    String kind; // "RF" or "IR"
    String content;
};
const int MAX_QUEUE = 10;
bool _queueMode = false;
std::vector<QueuedSignal> _queue;

// --- Auto-save (optional) ------------------------------------------------------
// When enabled every capture is saved automatically under /BruceIR or /BruceRF
// with an auto-generated name. A capture is only written once a NEW one arrives
// (the previous signal is flushed then) or when the detector is left, so the
// just-captured signal stays "in hand" briefly without any menu interaction.
bool _autoSave = false;
String _pendingKind = "";
String _pendingContent = "";
String _lastFp = ""; // fingerprint of the pending capture (repeat detection)

uint16_t _rfColor() {
    return bruceConfig.priColor;
}

uint16_t _irColor() {
    if (bruceConfig.secColor != bruceConfig.bgColor) return bruceConfig.secColor;
    return getComplementaryColor(bruceConfig.bgColor);
}

uint16_t _dimColor() {
    return getColorVariation(bruceConfig.bgColor, 15);
}

// Human-readable name of the active IR RX pin, from the board's IR_RX_PINS list
// (same list the Settings -> IR module selector uses).
String _irRxName() {
    const std::vector<std::pair<String, int>> pins = IR_RX_PINS;
    for (auto &p : pins) {
        if (p.second == bruceConfigPins.irRx) return String(p.first);
    }
    return "GPIO" + String(bruceConfigPins.irRx);
}

// True when a single-pin RF module would listen on the SAME pin as the IR
// receiver (e.g. Cardputer defaults: rfRx = GROVE_SCL = RXLED = GPIO1). In that
// case the RMT RF receiver grabs the same demodulated IR burst and captures it
// as "RF RAW", so the user has to press the remote twice. A CC1101 receives on
// GDO0 (not rfRx) and is never affected.
bool _rfIrConflict() {
    return (bruceConfigPins.rfModule != CC1101_SPI_MODULE) && (bruceConfigPins.rfRx == bruceConfigPins.irRx);
}

// Forward declaration (drawSpectrumFrame is defined after the listener tasks).
void drawSpectrumFrame(int frame);
// Forward declaration (queue browser is defined after the capture helpers).
bool _queueMenu();

// --- RF capture (decode, else RAW fallback, in the same window) -------------
String _hex64(uint64_t v) {
    char buf[64] = {0};
    decimalToHexString(v, buf);
    return String(buf);
}

String _formatRf(const RfCodes &codes, float frequency) {
    String out = rf_subghz_header(frequency);
    if (codes.protocol == "RAW") {
        String preset = codes.preset;
        if (preset == "" || preset == "1") preset = "FuriHalSubGhzPresetOok270Async";
        else if (preset == "2") preset = "FuriHalSubGhzPresetOok650Async";
        out += "Preset: " + preset + "\n";
        out += "Protocol: RAW\n";
        out += "RAW_Data: " + codes.data;
    } else {
        String preset = codes.preset;
        if (preset == "") preset = "FuriHalSubGhzPresetOok270Async";
        String protocol = codes.protocol;
        if (protocol == "") protocol = "RcSwitch";
        out += "Preset: " + preset + "\n";
        out += "Protocol: " + protocol + "\n";
        out += "Bit: " + String(codes.Bit) + "\n";
        out += "Key: " + _hex64(codes.key) + "\n";
        if (codes.fix != 0) { // KeeLoq rolling-code extras
            out += "Manufacture: " + codes.mf_name + "\n";
            out += "Serial: " + _hex64(codes.serial) + "\n";
            out += "Button: " + String(codes.btn) + "\n";
            out += "Counter: " + String(codes.cnt) + "\n";
        }
        out += "TE: " + String(codes.te) + "\n";
    }
    return out;
}

String _rfCapture(int window, bool forceRaw, volatile bool *abort) {
    float frequency = bruceConfigPins.rfFreq;
    if (!initRfModule("rx", frequency)) return "";
    RfRxSession rx;
    if (!rx.begin()) {
        deinitRfModule();
        return "";
    }

    RfCodes received;
    uint32_t deadline = millis() + (uint32_t)window * 1000UL;
    while (millis() < deadline) {
        if (abort != nullptr && *abort) break;
        std::vector<int> durations;
        if (rx.poll(durations)) {
            bool decoded =
                (!forceRaw) && (rf_try_keeloq(durations, received) || rf_decode_ook(durations, received));
            String _data;
            bool hasCrc = false;
            uint64_t crc = 0;
            std::vector<int> indexed;
            int rawBits = 0, rawTe = 0;
            int transitions = rf_build_raw(durations, _data, hasCrc, crc, indexed, rawBits, rawTe);

            if (decoded) {
                received.frequency = long(frequency * 1000000);
                received.data = _data;
                rx.end();
                deinitRfModule();
                return _formatRf(received, frequency);
            } else if (transitions > 20) {
                // Undecodable signal: keep it as RAW so it can still be replayed.
                RfCodes raw;
                raw.frequency = long(frequency * 1000000);
                raw.protocol = "RAW";
                raw.preset = "Ook270Async";
                raw.te = rawTe;
                raw.data = _data;
                rx.end();
                deinitRfModule();
                return _formatRf(raw, frequency);
            }
            // else: noise / too few transitions, keep listening
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    rx.end();
    deinitRfModule();
    return "";
}

// --- FreeRTOS listener tasks --------------------------------------------------
// Tasks MUST end with vTaskDelete(NULL): ESP-IDF aborts ("FreeRTOS Task ... should
// not return, Aborting now!") a task function that returns normally. The RAII
// destructors (RfRxSession end, IrRead/IRrecv disableIRIn + timerEnd) run inside
// their scopes BEFORE the self-delete, so nothing is left armed on a freed stack.
static void _rfTask(void *arg) {
    DualCtx *ctx = (DualCtx *)arg;
    ctx->rf = _rfCapture(ctx->window, ctx->raw, &ctx->abort); // cleans up internally
    xSemaphoreGive(ctx->sem);
    vTaskDelete(NULL);
}

static void _irTask(void *arg) {
    DualCtx *ctx = (DualCtx *)arg;
    {
        IrRead ir(true, ctx->raw);
        ctx->ir = ir.loop_headless(ctx->window, &ctx->abort);
    } // ~IrRead() runs disableIRIn() + frees the IRrecv timer here
    xSemaphoreGive(ctx->sem);
    vTaskDelete(NULL);
}

// Listens to both bands for one window. Returns the key press (PRESS_*), or
// PRESS_NONE. Fills res.rf / res.ir with the captured file contents ("" = none).
int _dualListen(DualCtx &res, bool bothRaw) {
    bool conflict = _rfIrConflict();
    int nTasks = conflict ? 1 : 2; // conflicted boards listen IR-only (see _rfIrConflict)
    DualCtx ctx;
    ctx.window = _win;
    ctx.raw = bothRaw;
    ctx.sem = xSemaphoreCreateCounting(2, 0);
    ctx.abort = false;
    TaskHandle_t rfTask = NULL, irTask = NULL;
    if (ctx.sem == NULL) return PRESS_NONE;

    bool ok;
    if (conflict) {
        // IR and RF would both listen on the same pin: the RMT RF receiver grabs
        // the same demodulated IR burst and captures it as "RF RAW" first. Skip
        // the RF task so the first press is always caught.
        ok = xTaskCreate(_irTask, "dualIR", 16384, &ctx, 2, &irTask) == pdPASS;
    } else {
        ok = (xTaskCreate(_rfTask, "dualRF", 16384, &ctx, 2, &rfTask) == pdPASS) &&
             (xTaskCreate(_irTask, "dualIR", 16384, &ctx, 2, &irTask) == pdPASS);
    }
    if (!ok) {
        // Out of task RAM: listen sequentially so the feature still works.
        if (rfTask != NULL) vTaskDelete(rfTask);
        if (irTask != NULL) vTaskDelete(irTask);
        if (!conflict) ctx.rf = _rfCapture(ctx.window, bothRaw, nullptr);
        IrRead ir(true, bothRaw);
        ctx.ir = ir.loop_headless(ctx.window);
        vSemaphoreDelete(ctx.sem);
        res.rf = ctx.rf;
        res.ir = ctx.ir;
        return PRESS_NONE;
    }

    int press = PRESS_NONE;
    unsigned long lastAnim = 0;
    int frame = 0;
    // Wait until the listening task(s) finished OR one of them captured
    // something. The first band that fires stops the detector (see header
    // comment): the other band's task is aborted below so the result shows
    // immediately instead of only after the full listen window (e.g. RF still
    // listening for its 8s while IR already caught the remote press).
    while (uxSemaphoreGetCount(ctx.sem) < nTasks && ctx.rf == "" && ctx.ir == "") {
        if (check(EscPress)) press = PRESS_ESC;
        else if (check(NextPress)) press = PRESS_NEXT;
        if (press != PRESS_NONE) {
            ctx.abort = true;
            uint32_t w = millis() + 3000; // give the tasks up to 3s to wind down
            while (uxSemaphoreGetCount(ctx.sem) < nTasks && millis() < w) vTaskDelay(10 / portTICK_PERIOD_MS);
            break;
        }
        if (millis() - lastAnim > 80) {
            lastAnim = millis();
            drawSpectrumFrame(frame++);
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    // A band fired: stop the other one (its *_capture loop checks ctx.abort) and
    // wait for it to wind down so its RAII destructors run before we return.
    if (ctx.rf != "" || ctx.ir != "") {
        ctx.abort = true;
        uint32_t w = millis() + 3000;
        while (uxSemaphoreGetCount(ctx.sem) < nTasks && millis() < w) vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // The listener(s) finished (or we gave up waiting): the tasks self-deleted
    // via vTaskDelete(NULL) after their destructors ran, so there are no task
    // handles left to reclaim (they are dangling once deleted).
    for (int i = 0; i < nTasks; i++) xSemaphoreTake(ctx.sem, pdMS_TO_TICKS(2000));
    vSemaphoreDelete(ctx.sem);

    res.rf = ctx.rf;
    res.ir = ctx.ir;
    return press;
}

// --- Listen / result screens ---------------------------------------------------
void drawListenScreen() {
    int W = tftWidth;
    int H = tftHeight;
    uint16_t bg = bruceConfig.bgColor;
    uint16_t rf = _rfColor();
    uint16_t ir = _irColor();

    tft.fillScreen(bg);
    tft.setTextColor(rf, bg);
    tft.setTextSize(1);
    tft.drawString("RF + IR DETECT", 4, 2, 1);

    int pw = W - 8;
    int ph = 24;
    int rfY = 14;
    int irY = rfY + ph + 4;

    tft.fillRoundRect(4, rfY, pw, ph, 4, rf);
    tft.setTextColor(bg, rf);
    tft.setTextSize(2);
    String rfMod = (bruceConfigPins.rfModule == CC1101_SPI_MODULE) ? "CC1101" : "M5RF";
    tft.drawString("RF " + String(bruceConfigPins.rfFreq, 2) + "MHz [" + rfMod + "]", 10, rfY + 1, 1);
    tft.setTextSize(1);
    if (_rfIrConflict())
        tft.drawString("RF DISABLED (same pin as IR)", 10, rfY + ph - 11, 1);
    else
        tft.drawString("LISTENING " + String(_rfRaw ? "RAW" : "AUTO"), 10, rfY + ph - 11, 1);

    tft.fillRoundRect(4, irY, pw, ph, 4, ir);
    tft.setTextColor(bg, ir);
    tft.setTextSize(2);
    tft.drawString("IR 38kHz " + String(_irRaw ? "RAW" : "AUTO"), 10, irY + 1, 1);
    tft.setTextSize(1);
    tft.drawString("LISTENING [" + _irRxName() + "]", 10, irY + ph - 11, 1);

    _specX = 4;
    _specY = irY + ph + 6;
    _specW = W - 8;
    _specH = H - _specY - 24;
    if (_specH < 8) _specH = 8;

    tft.setTextColor(_dimColor(), bg);
    tft.setTextSize(1);
    tft.drawString(
        "RF:" + String(_rfHits) + " IR:" + String(_irHits) + " Q:" + String(_queue.size()) +
            (_queueMode ? "*" : "") + " AS:" + String(_autoSave ? "ON" : "OFF") + " c:" + String(_cycles),
        4, H - 20, 1
    );
    tft.drawString("[NEXT] options   [ESC] quit", 4, H - 10, 1);
}

void drawSpectrumFrame(int frame) {
    if (_specW <= 0) return;
    uint16_t bg = bruceConfig.bgColor;
    uint16_t rf = _rfColor();
    uint16_t ir = _irColor();
    tft.fillRect(_specX, _specY, _specW, _specH, bg);

    int bars = 16;
    float bw = (float)_specW / bars;
    int baseY = _specY + _specH;
    for (int i = 0; i < bars; i++) {
        float hgtF = _specH * (0.22f + 0.68f * fabsf(sinf(frame * 0.7f + i * 0.8f)));
        if (hgtF > _specH) hgtF = _specH;
        int hgt = (int)hgtF;
        if (hgt < 1) hgt = 1;
        uint16_t col = (i % 3 == 1) ? rf : ((i % 3 == 2) ? ir : _dimColor());
        tft.fillRect(_specX + (int)(i * bw), baseY - hgt, (int)bw - 2, hgt, col);
    }
    int scanX = _specX + (int)(_specW * ((frame % 12) / 12.0f));
    tft.drawFastVLine(scanX, _specY, _specH, getColorVariation(bg, 40));
    tft.drawFastHLine(_specX, baseY, _specW, _dimColor());
}

// --- Capture description (result screen) ---------------------------------------
String _field(const String &content, const String &key) {
    String k = key + ":";
    int idx = content.indexOf(k);
    if (idx < 0) return "";
    int eol = content.indexOf('\n', idx);
    String v = content.substring(idx + k.length(), eol < 0 ? (int)content.length() : eol);
    v.trim();
    return v;
}

int _countTokens(const String &s) {
    int n = 0;
    bool inTok = false;
    for (size_t i = 0; i < s.length(); i++) {
        char ch = s.charAt(i);
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') inTok = false;
        else if (!inTok) {
            inTok = true;
            n++;
        }
    }
    return n;
}

// Stable identity of a capture, used by auto-save to skip repeats: two presses
// of the same button produce the same fingerprint, a different button produces
// a different one. Decoded signals are identified by their protocol fields,
// raw captures by the raw pulse data itself.
String _fingerprint(const String &kind, const String &content) {
    String fp = kind + "|";
    if (kind == "RF") {
        fp += _field(content, "Frequency") + "|" + _field(content, "Protocol") + "|" +
              _field(content, "Bit") + "|" + _field(content, "Key") + "|" +
              _field(content, "TE") + "|" + _field(content, "RAW_Data");
    } else {
        fp += _field(content, "type") + "|" + _field(content, "protocol") + "|" +
              _field(content, "address") + "|" + _field(content, "command") + "|" +
              _field(content, "bits") + "|" + _field(content, "value") + "|" +
              _field(content, "data");
    }
    return fp;
}

std::vector<String> _describeRF(const String &content) {
    std::vector<String> out;
    String freq = _field(content, "Frequency");
    String proto = _field(content, "Protocol");
    String bit = _field(content, "Bit");
    String key = _field(content, "Key");
    String te = _field(content, "TE");
    String raw = _field(content, "RAW_Data");
    if (freq != "") out.push_back("Freq: " + String(freq.toDouble() / 1000000.0, 3) + " MHz");
    else out.push_back("Freq: " + String(bruceConfigPins.rfFreq, 2) + " MHz");
    out.push_back("Protocol: " + String(proto != "" ? proto : (raw != "" ? "RAW" : "?")));
    if (bit != "") out.push_back("Bits: " + bit);
    if (key != "") out.push_back("Key: " + key);
    if (te != "" && te != "0") out.push_back("TE: " + te);
    if (raw != "") {
        String trimmed = raw;
        if (trimmed.length() > 30) trimmed = trimmed.substring(0, 30) + "...";
        out.push_back("RAW: " + trimmed);
    }
    return out;
}

std::vector<String> _describeIR(const String &content) {
    std::vector<String> out;
    String type = _field(content, "type");
    String proto = _field(content, "protocol");
    String addr = _field(content, "address");
    String cmd = _field(content, "command");
    String bits = _field(content, "bits");
    String val = _field(content, "value");
    String data = _field(content, "data");
    out.push_back("Type: " + String(type != "" ? type : "?"));
    if (proto != "") out.push_back("Protocol: " + proto);
    if (addr != "") out.push_back("Address: " + addr);
    if (cmd != "") out.push_back("Command: " + cmd);
    if (bits != "") out.push_back("Bits: " + bits);
    if (val != "") out.push_back("Value: " + val);
    if (data != "") out.push_back("Pulses: " + String(_countTokens(data)));
    return out;
}

void drawResultScreen(const String &kind, const String &content) {
    uint16_t bg = bruceConfig.bgColor;
    uint16_t band = (kind == "RF") ? _rfColor() : _irColor();
    tft.fillScreen(bg);
    tft.setTextColor(band, bg);
    tft.setTextSize(2);
    tft.drawString(kind + " SIGNAL!", 4, 2, 1);
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bg);
    int y = 26;
    std::vector<String> lines = (kind == "RF") ? _describeRF(content) : _describeIR(content);
    for (size_t i = 0; i < lines.size() && y < tftHeight - 14; i++) {
        tft.drawString(lines[i], 4, y, 1);
        y += 11;
    }
    tft.setTextColor(_dimColor(), bg);
    tft.drawString(
        _autoSave ? "AUTO-SAVE: next signal saves this" : "choose action...", 4, tftHeight - 10, 1
    );
}

// --- Save / Replay / Result menu ------------------------------------------------
// Replaces Flipper's "Filetype: IR signals file" header with Bruce's so the IR
// replay path (txIrFile) accepts the file.
String _normalizeIR(const String &content) {
    const char *hdr = "Filetype: IR signals file";
    if (content.startsWith(hdr)) {
        return "Filetype: Bruce IR File" + content.substring(strlen(hdr));
    }
    return content;
}

void _saveCapture(const String &kind, const String &content) {
    if (!sdcardMounted) {
        displayError("No SD card - save disabled", true);
        return;
    }
    FS *fs = nullptr;
    if (!getFsStorage(fs)) {
        displayError("No storage available", true);
        return;
    }
    String folder = (kind == "RF") ? "/BruceRF" : "/BruceIR";
    String ext = (kind == "RF") ? ".sub" : ".ir";
    if (!fs->exists(folder)) fs->mkdir(folder);

    String name = keyboard(kind == "RF" ? "rf_signal" : "ir_signal", 24, "File name:");
    if (name == "\x1B") return;
    name.trim();
    if (name == "") return;

    String target = name;
    int i = 1;
    while (fs->exists(folder + "/" + target + ext)) {
        target = name + "_" + String(i++);
    }

    String body = (kind == "IR") ? _normalizeIR(content) : content;
    File f = fs->open(folder + "/" + target + ext, FILE_WRITE);
    if (!f) {
        displayError("Error saving file", true);
        return;
    }
    f.print(body);
    if (!body.endsWith("\n")) f.println();
    f.close();
    displaySuccess("Saved " + folder + "/" + target + ext, true);
}

// Saves a capture to /BruceRF or /BruceIR with an auto-generated name
// (ir_capture, ir_capture_2, ...). SD-gated like _saveCapture. Non-blocking
// feedback (waitKeyPress=false) so the listen loop is not interrupted.
void _autoSaveCapture(const String &kind, const String &content) {
    if (!sdcardMounted) {
        displayWarning("Auto-save: no SD card", false);
        return;
    }
    FS *fs = nullptr;
    if (!getFsStorage(fs)) {
        displayWarning("Auto-save: no storage", false);
        return;
    }
    String folder = (kind == "RF") ? "/BruceRF" : "/BruceIR";
    String ext = (kind == "RF") ? ".sub" : ".ir";
    if (!fs->exists(folder)) fs->mkdir(folder);

    String base = (kind == "RF") ? "rf_capture" : "ir_capture";
    String target = base;
    int i = 1;
    while (fs->exists(folder + "/" + target + ext)) target = base + "_" + String(i++);

    String body = (kind == "IR") ? _normalizeIR(content) : content;
    File f = fs->open(folder + "/" + target + ext, FILE_WRITE);
    if (!f) {
        displayWarning("Auto-save failed", false);
        return;
    }
    f.print(body);
    if (!body.endsWith("\n")) f.println();
    f.close();
    displaySuccess("Auto-saved " + target + ext, false);
}

// Flushes (saves) the pending capture, if any. Clears it either way.
void _autoSavePending() {
    if (_pendingKind == "") return;
    String kind = _pendingKind;
    String content = _pendingContent;
    _pendingKind = "";
    _pendingContent = "";
    _autoSaveCapture(kind, content);
}

// Auto-save flow for a fresh capture: a DIFFERENT signal saves the previous
// pending one first; a repeat of the same signal only replaces the pending one
// (nothing new is written - one file per distinct button). Either way the new
// capture becomes the pending one and gets a short result flash.
void _autoSaveFlow(const String &kind, const String &content) {
    String fp = _fingerprint(kind, content);
    if (_pendingKind != "" && fp != _lastFp) _autoSavePending();
    _lastFp = fp;
    _pendingKind = kind;
    _pendingContent = content;
    drawResultScreen(kind, content);
    vTaskDelay(pdMS_TO_TICKS(900));
}

void _replayCapture(const String &kind, const String &content) {
    FS *fs = nullptr;
    if (!getFsStorage(fs)) {
        displayError("No storage available", true);
        return;
    }
    String tmp = (kind == "RF") ? "/_dualdetect.sub" : "/_dualdetect.ir";
    String body = (kind == "IR") ? _normalizeIR(content) : content;

    File f = fs->open(tmp, FILE_WRITE);
    if (!f) {
        displayError("Replay write failed", true);
        return;
    }
    f.print(body);
    f.close();

    bool ok = false;
    if (kind == "RF") {
        RfCodes data;
        ok = readSubFile(fs, tmp, data) && txSubFile(data, true);
    } else {
        ok = txIrFile(fs, tmp, true);
    }
    fs->remove(tmp);
    if (ok) displaySuccess("Replayed " + kind + " signal", true);
    else displayError("Replay failed", true);
}

// Shows the captured signal info and the Replay / Save / Discard menu.
// Loops back to the menu after a Replay (or Save) so the user can replay the
// signal again, save it, or discard it. "Save Signal" is offered only when an
// SD card is mounted (T-Embed); boards without SD (M5StickS3) can only replay.
// Returns true when the user chose to leave the detector.
bool _handleCapture(const String &kind, const String &content) {
    drawResultScreen(kind, content);
    vTaskDelay(pdMS_TO_TICKS(1200));
    while (true) {
        bool exit = false;
        bool discard = false;
        std::vector<Option> options;
        options.emplace_back("Replay", [&]() { _replayCapture(kind, content); });
        if (sdcardMounted) options.emplace_back("Save Signal", [&]() { _saveCapture(kind, content); });
        if (_queueMode && !_queue.empty())
            options.emplace_back(
                "View Queue (" + String(_queue.size()) + ")", [&]() { _queueMenu(); }
            );
        options.emplace_back("Discard", [&]() { discard = true; });
        options.emplace_back("Exit to Main Menu", [&]() { exit = true; });
        int idx = loopOptions(options, MENU_TYPE_SUBMENU, (kind + " Signal").c_str());
        if (idx == -1) return false; // ESC -> discard and keep listening
        if (discard) return false;
        if (exit) return true;
        // Replay or Save ran: loop back to the menu to redo the replay or pick another action
    }
}

// --- Signal queue browser -------------------------------------------------------
// Short label for a queued capture (band + first descriptor line).
String _queueLabel(const QueuedSignal &q) {
    std::vector<String> d = (q.kind == "RF") ? _describeRF(q.content) : _describeIR(q.content);
    String label = q.kind + " " + (d.size() > 0 ? d[0] : "?");
    if (label.length() > 34) label = label.substring(0, 33) + "~";
    return label;
}

// Per-item action menu (same Replay/Save as a fresh capture, plus Remove).
// Returns true when the item was removed from the queue.
bool _queueItemMenu(size_t idx) {
    while (true) {
        bool remove = false, back = false;
        std::vector<Option> options;
        options.emplace_back("Replay", [&]() { _replayCapture(_queue[idx].kind, _queue[idx].content); });
        if (sdcardMounted)
            options.emplace_back("Save Signal", [&]() { _saveCapture(_queue[idx].kind, _queue[idx].content); });
        options.emplace_back("Remove", [&]() { remove = true; });
        options.emplace_back("Back", [&]() { back = true; });
        int sel = loopOptions(options, MENU_TYPE_SUBMENU, (_queue[idx].kind + " Signal").c_str());
        if (sel == -1 || back) return false; // ESC/Back -> back to the queue browser
        if (remove) {
            _queue.erase(_queue.begin() + idx);
            return true;
        }
        // Replay/Save ran: loop back to this item's actions
    }
}

// Browsed the captured-history. Returns true when the user chose to leave the
// detector ("Exit to Main Menu"), false to resume listening.
bool _queueMenu() {
    while (true) {
        if (_queue.empty()) {
            displayInfo("Queue is empty", true);
            return false;
        }
        bool exitMain = false, close = false;
        std::vector<Option> options;
        for (size_t i = 0; i < _queue.size(); i++) {
            options.emplace_back(String(i + 1) + ". " + _queueLabel(_queue[i]), [&]() {
                // selection is handled after loopOptions via `sel`
            });
        }
        options.emplace_back("Discard All", [&]() {
            _queue.clear();
            close = true;
        });
        options.emplace_back("Close", [&]() { close = true; });
        options.emplace_back("Exit to Main Menu", [&]() { exitMain = true; });
        int sel = loopOptions(options, MENU_TYPE_SUBMENU, ("Queue (" + String(_queue.size()) + ")").c_str());
        if (sel == -1) return false; // ESC -> resume listening
        if (exitMain) return true;
        if (close) return false;
        if (sel >= 0 && sel < (int)_queue.size()) _queueItemMenu((size_t)sel);
        // loop: rebuild the list (a queue item may have been removed)
    }
}

// --- Options sub-menus ------------------------------------------------------------
void _freqMenu() {
    std::vector<Option> options;
    for (float f : DUAL_FREQS) {
        options.emplace_back(String(f, 3) + " MHz", [f]() { bruceConfigPins.rfFreq = f; });
    }
    // Full list of common sub-GHz frequencies (same as Settings -> RF freq).
    options.emplace_back("All frequencies...", [&]() {
        std::vector<Option> sub;
        int arraySize = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);
        for (int i = 0; i < arraySize; i++) {
            sub.emplace_back(
                String(subghz_frequency_list[i], 3) + " MHz",
                [i]() { bruceConfigPins.rfFreq = subghz_frequency_list[i]; }
            );
        }
        loopOptions(sub, MENU_TYPE_SUBMENU, "All frequencies");
    });
    options.emplace_back("Manual", [&]() {
        String v = num_keyboard(String(bruceConfigPins.rfFreq, 2), 8, "Frequency (MHz)");
        float f = v.toFloat();
        if (f > 0) bruceConfigPins.rfFreq = f;
    });
    loopOptions(options, MENU_TYPE_SUBMENU, "RF Frequency");
}

void _winMenu() {
    std::vector<Option> options;
    for (int i = DUAL_WINDOW_MIN; i <= DUAL_WINDOW_MAX; i++) {
        options.emplace_back(
            String(i) + (i == 1 ? " second" : " seconds"), [i]() { _win = i; }
        );
    }
    loopOptions(options, MENU_TYPE_SUBMENU, "Listen window");
}

// Returns true when the user asked to exit the detector, false to resume listening.
bool _optionsMenu() {
    while (true) {
        bool start = false, exit = false;
        std::vector<Option> options = {
            {"Freq: " + String(bruceConfigPins.rfFreq, 2) + " MHz", [&]() { _freqMenu(); }},
            {"Window: " + String(_win) + "s", [&]() { _winMenu(); }},
            {"RF mode: " + String(_rfRaw ? "RAW" : "AUTO"), [&]() { _rfRaw = !_rfRaw; }},
            {"IR mode: " + String(_irRaw ? "RAW" : "AUTO"), [&]() { _irRaw = !_irRaw; }},
            {"IR module: " + _irRxName(), [&]() { gsetIrRxPin(true); }}, // external IR module (e.g. M5 IR Mod on Grove)
            {"RF module: " + String(bruceConfigPins.rfModule == CC1101_SPI_MODULE ? "CC1101" : "M5 RF433"),
             [&]() { setRFModuleMenu(); }}, // external CC1101 on boards without an internal one
            {"RF pin: GPIO" + String(bruceConfigPins.rfRx),
             [&]() { gsetRfRxPin(true); }}, // resolves the same-pin-as-IR conflict on-board
            {"Signal queue: " + String(_queueMode ? "ON" : "OFF"), [&]() {
                 _queueMode = !_queueMode;
                 if (!_queueMode) _queue.clear(); // history is only kept while the feature is on
             }},
            {"Auto-save: " + String(_autoSave ? "ON" : "OFF"), [&]() {
                 _autoSave = !_autoSave;
                 if (!_autoSave) { // forget the unsaved capture when switching off
                     _pendingKind = "";
                     _pendingContent = "";
                     _lastFp = "";
                 }
             }},
            {"Close Menu", [&]() { start = true; }},
            {"Exit to Main Menu", [&]() { exit = true; }},
        };
        if (_queueMode)
            options.insert(
                options.end() - 2,
                Option("Queue: " + String(_queue.size()) + " signals", [&]() { _queueMenu(); })
            );
        int idx = loopOptions(options, MENU_TYPE_SUBMENU, "RF+IR Detect");
        if (idx == -1) return false; // ESC -> back to listening
        if (exit) return true;
        if (start) return false;
        // toggles/sub-menus selected: rebuild the list with the new labels
    }
}

} // namespace

void dualDetect() {
    _rfHits = 0;
    _irHits = 0;
    _cycles = 0;
    _pendingKind = "";
    _pendingContent = "";
    _lastFp = "";

    while (true) {
        _cycles++;
        drawListenScreen();

        bool bothRaw = _rfRaw && _irRaw;
        DualCtx res;
        int press = _dualListen(res, bothRaw);
        if (press == PRESS_ESC) {
            // Auto-save: don't lose the last capture when leaving the detector.
            if (_autoSave) _autoSavePending();
            return;
        }

        // When a band was in AUTO mode and got nothing decodable, give one more
        // window in RAW mode (captures unknown protocols too).
        if (press == PRESS_NONE) {
            bool rfNeed = (!_rfRaw) && res.rf == "";
            bool irNeed = (!_irRaw) && res.ir == "";
            if (rfNeed || irNeed) {
                DualCtx res2;
                int press2 = _dualListen(res2, true);
                if (press2 == PRESS_ESC) {
                    if (_autoSave) _autoSavePending();
                    return;
                }
                if (res.rf == "") res.rf = res2.rf;
                if (res.ir == "") res.ir = res2.ir;
                if (press2 != PRESS_NONE) press = press2;
            }
        }

        // Queue mode keeps every capture in a bounded history: the newest shows
        // the result menu while the previous ones stay available from the queue.
        if (_queueMode) {
            if (res.rf != "") _queue.push_back({"RF", res.rf});
            if (res.ir != "") _queue.push_back({"IR", res.ir});
            while ((int)_queue.size() > MAX_QUEUE) _queue.erase(_queue.begin());
        }

        // Auto-save mode: no blocking menu - each new signal saves the previous
        // one automatically and flashes the result, then keeps listening.
        if (_autoSave) {
            if (res.rf != "") {
                _rfHits++;
                _autoSaveFlow("RF", res.rf);
            }
            if (res.ir != "") {
                _irHits++;
                _autoSaveFlow("IR", res.ir);
            }
            if (press == PRESS_NEXT) {
                if (_optionsMenu()) {
                    if (_autoSave) _autoSavePending();
                    return;
                }
                continue;
            }
            continue;
        }

        if (res.rf != "") {
            _rfHits++;
            if (_handleCapture("RF", res.rf)) return;
        }
        if (res.ir != "") {
            _irHits++;
            if (_handleCapture("IR", res.ir)) return;
        }

        if (press == PRESS_NEXT) {
            if (_optionsMenu()) return;
            continue;
        }
    }
}
