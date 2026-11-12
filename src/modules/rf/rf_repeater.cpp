// Native RF Repeater: capture a burst, preview a decode attempt, then
// replay/save/discard it. Reuses the same proven primitives as the rest of
// the RF module rather than re-implementing capture or transmit:
//   - RfRxSession + rf_build_raw/rf_decode_ook/rf_decode_keeloq (capture+ID)
//   - rfTransmitRawTimings (same zero-terminated signed-duration array the
//     "RAW" protocol branch of sendRfCommand() already builds and sends)
//   - getFsStorage + the standard .sub RAW text format (same as save.cpp)
#include "rf_repeater.h"
#include "protocols/rf_decoder.h"
#include "protocols/rf_encoder.h" // rf_tx_protocol/rf_tx_keeloq: clean re-encode, not a raw replay
#include "protocols/rf_registry.h" // rf_find_protocol
#include "rf_capture_tx.h" // rf_identify_capture/rf_transmit_clean_*/rf_transmit_raw -- shared with rf_subghz.cpp
#include "rf_utils.h"
#include "rf_send.h"
#include "core/display.h"
#include "core/sd_functions.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

namespace {

using MatchKind = RfMatchKind;

float g_freq = 433.92f;
float g_bw = 135.0f;
int g_agc = 3; // Max Sensitivity default -- same reasoning as the weather app:
               // you want to actually capture what you're repeating.
// Defaults confirmed by on-device testing: 3 rapid repeats at 300ms apart
// made a Flipper receiver treat them as interference rather than 3 distinct
// presses. A single clean replay works reliably; if repeat count is raised
// above 1, give receivers at least ~1s between transmissions.
// 1 repeat is a genuine single frame but confirmed (2026-11-11 live test with
// Flipper + HackRF) too short to reliably decode -- a receiver typically
// needs to see a code repeat 2+ times to lock on. 3 is the chosen middle
// ground: long enough to decode reliably, short enough to stay snappy for
// short codes. (The earlier "default to 1" was set before repeats were
// baked into one continuous RMT burst with correct protocol framing; back
// then 3 repeats meant 3 separate TX re-inits, which is what confused
// Flipper, not the repeat count itself.)
int g_repeatCount = 3;
int g_repeatDelayMs = 1000;
int g_prePauseMs = 5000;
int g_saveIndex = 0;
// True hands-off repeater behavior: transmit the instant a capture is
// accepted and immediately resume listening, no SEL/menu interaction at
// all. Default ON since that's this app's whole purpose; turn off in
// Settings if you want to review/save before anything gets sent back out.
bool g_autoRepeat = true;
// Set every time the radio (re)arms for RX (see reinit_rx). The CC1101's
// RX-mode settling transient can produce a short, consistent spurious
// burst on GDO0 right as the receiver comes up; a short grace window
// after each (re)arm filters that out without needing a higher edge-count
// threshold (which would also reject genuinely short protocols, e.g. the
// 10-bit "Linear" entry in the registry, ~21 edges for one repeat).
uint32_t g_armedAtMs = 0;
// Last capture seen, regardless of auto-repeat mode or whether it was
// recognized -- lets the user manually trigger a transmit on demand (SEL ->
// Settings -> Retransmit last capture) instead of needing to catch it live
// or wait on auto-repeat's own timing.
bool g_haveLast = false;
std::vector<int> g_lastDurations;
MatchKind g_lastKind = MatchKind::NONE;
RfCodes g_lastCode;
String g_lastInfo;
// Set by the "Retransmit last capture" Settings entry; the caller (which
// owns the live RfRxSession) performs the actual TX so it can properly
// rx.end()/reinit_rx() around it rather than switching the CC1101 to TX
// mode out from under an armed RX session.
bool g_requestRetransmit = false;
constexpr uint32_t ARM_SETTLE_MS = 400;

const char *agc_label(int agc) {
    switch (agc) {
        case 1: return "AGC C7";
        case 2: return "AGC C3 (test)";
        case 3: return "Max Sensitivity";
        default: return "Default";
    }
}

void apply_agc(int agc) {
    if (agc == 1) ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL2, 0xC7);
    else if (agc == 2) ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL2, 0xC3);
    else if (agc == 3) ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL2, 0x07);
}

bool reinit_rx(RfRxSession &rx) {
    // RfRxSession::begin() calls setup_rf_rx(), which itself calls
    // initRfModule("rx", bruceConfigPins.rfFreq) again internally -- so an
    // initRfModule() call here, before begin(), gets silently re-run (and
    // its frequency/bandwidth/AGC settings along with it) using Bruce's
    // globally-configured frequency instead of g_freq. Set the global first
    // so that second, hidden init tunes to the right place, then (re)apply
    // our bandwidth/AGC AFTER begin() so they're the last word, not
    // whatever that hidden init left behind.
    bruceConfigPins.rfFreq = g_freq;
    if (!rx.begin()) return false;
    ELECHOUSE_cc1101.setRxBW(g_bw);
    apply_agc(g_agc);
    g_armedAtMs = millis();
    return true;
}

// Thin aliases over the shared helpers in rf_capture_tx.h (also used by
// rf_subghz.cpp's Frequency Analyzer) -- kept under the original names here
// so every call site below is unchanged.
bool transmit_durations(const std::vector<int> &durations, float freq) {
    return rf_transmit_raw(durations, freq);
}
bool transmit_clean_ook(const RfCodes &code, float freq, int repeat) {
    return rf_transmit_clean_ook(code, freq, repeat);
}
bool transmit_clean_keeloq(uint64_t key, float freq, int repeat) {
    return rf_transmit_clean_keeloq(key, freq, repeat);
}

void save_capture(const std::vector<int> &durations, float freq) {
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) return;
    if (!fs->exists("/BruceRF")) {
        if (!fs->mkdir("/BruceRF")) return;
    }
    char filename[48];
    snprintf(filename, sizeof(filename), "/BruceRF/repeater_%d.sub", g_saveIndex++);
    File file = fs->open(filename, FILE_WRITE, true);
    if (!file) return;

    String rawData;
    bool hasCrc = false;
    uint64_t crc = 0;
    std::vector<int> indexed;
    int rawBits = 0, rawTe = 0;
    rf_build_raw(durations, rawData, hasCrc, crc, indexed, rawBits, rawTe);

    String out = rf_subghz_header(freq);
    out += "Preset: Ook270Async\n";
    out += "Protocol: RAW\n";
    out += "RAW_Data: " + rawData;
    file.write((const uint8_t *)out.c_str(), out.length());
    file.close();
    displaySuccess(filename, true);
}

String identify_capture(const std::vector<int> &durations, MatchKind &kind, RfCodes &outCode) {
    return rf_identify_capture(durations, kind, outCode);
}

// Chrome (border/title) is static across almost every redraw; a full
// tft.fillScreen() on every call -- including the ~150ms RSSI-bar heartbeat
// tick while idly listening -- was flashing the whole panel black several
// times a second (the flicker seen in the earlier screen-recording). Now
// only the regions that actually change get cleared+redrawn, and the static
// chrome is (re)drawn only when `fullRedraw` is true -- on first entry and
// after returning from any full-screen UI (Settings) that would have
// overwritten it.
void draw_screen(bool haveCapture, const String &info, int rssi, bool listening, bool fullRedraw = false) {
    if (fullRedraw) {
        tft.fillScreen(bruceConfig.bgColor);
        drawMainBorder();
        tft.setTextSize(FP);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setCursor(10, 26);
        tft.print("RF Repeater");
    }

    tft.setTextSize(FP);
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    tft.fillRect(10, 40, tftWidth - 20, 10, bruceConfig.bgColor);
    tft.setCursor(10, 40);
    tft.print(g_freq, 2);
    tft.print(" MHz");

    // Live RSSI meter so you can see signal strength move in real time while
    // transmitting nearby, instead of only a static number. The bar/number
    // area is self-clearing (opaque fillRect + opaque text), so no separate
    // dirty-rect clear is needed here.
    int barMax = 120;
    int barW = max(0, min(barMax, (int)((rssi + 100) * (barMax / 70.0f))));
    tft.fillRect(10, 50, barMax + 2, 10, getColorVariation(bruceConfig.priColor));
    tft.fillRect(11, 51, barW, 8, bruceConfig.priColor);
    tft.fillRect(10 + barMax + 10, 50, tftWidth - (10 + barMax + 10) - 10, 10, bruceConfig.bgColor);
    tft.setCursor(10 + barMax + 10, 50);
    tft.print(rssi);
    tft.print(" dBm");

    // Info block (two lines) has variable-length content depending on state,
    // so it needs an explicit clear before redraw -- but only this block,
    // not the whole screen.
    tft.fillRect(10, 76, tftWidth - 20, 32, bruceConfig.bgColor);
    tft.setCursor(10, 76);
    if (haveCapture) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.print("Captured:");
        tft.setCursor(10, 92);
        tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
        tft.print(info);
    } else {
        tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
        tft.print(listening ? "Listening..." : "");
        if (listening && g_haveLast) {
            tft.setCursor(10, 92);
            tft.print("Last: ");
            tft.print(g_lastInfo);
        }
    }

    tft.fillRect(10, tftHeight - 32, tftWidth - 20, 16, bruceConfig.bgColor);
    tft.setCursor(10, tftHeight - 32);
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    tft.print(haveCapture ? "SEL: replay/save/discard" : "SEL: settings");

    if (fullRedraw) {
        tft.setCursor(10, tftHeight - 16);
        tft.print("ESC: exit");
    }
}

void settings_menu() {
    int option = 0;
    options = {
        {"Frequency: " + String(g_freq, 2) + " MHz",        [&]() { option = 1; }},
        {"RX BW: " + String((int)g_bw) + " kHz",            [&]() { option = 2; }},
        {"AGC: " + String(agc_label(g_agc)),                [&]() { option = 3; }},
        {"Repeat count: " + String(g_repeatCount),          [&]() { option = 4; }},
        {"Repeat delay: " + String(g_repeatDelayMs) + " ms", [&]() { option = 5; }},
        {"Pre-replay pause: " + String(g_prePauseMs / 1000) + "s", [&]() { option = 6; }},
        {"Auto Repeat: " + String(g_autoRepeat ? "ON" : "OFF"), [&]() { g_autoRepeat = !g_autoRepeat; }},
        {"Retransmit last capture" + String(g_haveLast ? "" : " (none yet)"),
         [&]() { g_requestRetransmit = g_haveLast; }},
        {"Back",                                             [&]() { option = 0; }},
    };
    loopOptions(options);

    if (option == 1) {
        int freqOpt = -1;
        options = {
            {"433.92 MHz", [&]() { freqOpt = 0; }}, {"433.82 MHz", [&]() { freqOpt = 1; }},
            {"315 MHz",    [&]() { freqOpt = 2; }}, {"868.35 MHz", [&]() { freqOpt = 3; }},
            {"915 MHz",    [&]() { freqOpt = 4; }}, {"Cancel",     [&]() { freqOpt = -1; }},
        };
        loopOptions(options);
        const float vals[] = {433.92f, 433.82f, 315.0f, 868.35f, 915.0f};
        if (freqOpt >= 0 && freqOpt < 5) g_freq = vals[freqOpt];
    } else if (option == 2) {
        int bwOpt = -1;
        options = {
            {"58 kHz", [&]() { bwOpt = 58; }},   {"135 kHz", [&]() { bwOpt = 135; }},
            {"256 kHz", [&]() { bwOpt = 256; }}, {"406 kHz", [&]() { bwOpt = 406; }},
            {"Cancel", [&]() { bwOpt = -1; }},
        };
        loopOptions(options);
        if (bwOpt > 0) g_bw = (float)bwOpt;
    } else if (option == 3) {
        int agcOpt = -2;
        options = {
            {"Default",                       [&]() { agcOpt = 0; }},
            {"AGC C7",                         [&]() { agcOpt = 1; }},
            {"AGC C3 (test)",                  [&]() { agcOpt = 2; }},
            {"Max Sensitivity (weak signals)", [&]() { agcOpt = 3; }},
            {"Cancel",                         [&]() { agcOpt = -2; }},
        };
        loopOptions(options);
        if (agcOpt != -2) g_agc = agcOpt;
    } else if (option == 4) {
        int cntOpt = -1;
        options = {
            {"1", [&]() { cntOpt = 1; }},   {"3", [&]() { cntOpt = 3; }},
            {"5", [&]() { cntOpt = 5; }},   {"10", [&]() { cntOpt = 10; }},
            {"20", [&]() { cntOpt = 20; }}, {"Cancel", [&]() { cntOpt = -1; }},
        };
        loopOptions(options);
        if (cntOpt > 0) g_repeatCount = cntOpt;
    } else if (option == 5) {
        int delayOpt = -1;
        options = {
            {"100 ms", [&]() { delayOpt = 100; }},   {"300 ms", [&]() { delayOpt = 300; }},
            {"500 ms", [&]() { delayOpt = 500; }},   {"1000 ms", [&]() { delayOpt = 1000; }},
            {"1500 ms", [&]() { delayOpt = 1500; }}, {"2000 ms", [&]() { delayOpt = 2000; }},
            {"Cancel", [&]() { delayOpt = -1; }},
        };
        loopOptions(options);
        if (delayOpt > 0) g_repeatDelayMs = delayOpt;
    } else if (option == 6) {
        int pauseOpt = -1;
        options = {
            {"0s (none)", [&]() { pauseOpt = 0; }},     {"2s", [&]() { pauseOpt = 2000; }},
            {"5s", [&]() { pauseOpt = 5000; }},         {"10s", [&]() { pauseOpt = 10000; }},
            {"Cancel", [&]() { pauseOpt = -1; }},
        };
        loopOptions(options);
        if (pauseOpt >= 0) g_prePauseMs = pauseOpt;
    }
}

// Manual on-demand transmit of the last capture, triggered from Settings.
// Properly tears down and rearms the live RX session around the transmit,
// same as every other TX path here -- a transmit must never run while the
// RMT RX channel is still armed underneath it.
void perform_manual_retransmit(RfRxSession &rx) {
    if (!g_haveLast) return;
    rx.end();
    // rx.end() only tears down the RMT peripheral; it leaves the CC1101
    // chip itself still actively in RX. deinitRfModule() calls setSidle(),
    // parking the chip in IDLE before TX reprograms the frequency
    // synthesizer -- the stock Scan/Copy app always pairs these two calls
    // for exactly this reason (datasheet: freq programming must happen in
    // IDLE). Skipping this was leaving the radio live-RX while initRfModule
    // ("tx", ...) rewrote its registers underneath it.
    deinitRfModule();
    bool cleanOk = false;
    if (g_lastKind == MatchKind::OOK) cleanOk = transmit_clean_ook(g_lastCode, g_freq, g_repeatCount);
    else if (g_lastKind == MatchKind::KEELOQ) cleanOk = transmit_clean_keeloq(g_lastCode.key, g_freq, g_repeatCount);
    if (g_lastKind == MatchKind::NONE || !cleanOk) {
        for (int i = 0; i < g_repeatCount; i++) {
            transmit_durations(g_lastDurations, g_freq);
            if (i < g_repeatCount - 1) delay(g_repeatDelayMs);
        }
    }
    reinit_rx(rx);
}

} // namespace

void rf_repeater() {
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();

    RfRxSession rx;
    if (!reinit_rx(rx)) {
        Serial.println("[Repeater] CC1101 init failed");
        displayError("CC1101 init failed", true);
        return;
    }
    Serial.printf(
        "[Repeater] started: freq=%.2f bw=%.0f agc=%d rfModule=%d\n", g_freq, g_bw, g_agc,
        (int)bruceConfigPins.rfModule
    );

    std::vector<int> captured;
    bool haveCapture = false;
    String info;
    MatchKind capturedKind = MatchKind::NONE;
    RfCodes capturedCode;
    int rssi = -127;
    uint32_t lastDraw = 0;

    draw_screen(false, "", rssi, true, true);

    while (!check(EscPress)) {
        if (check(SelPress)) {
            if (haveCapture) {
                int action = 0;
                options = {
                    {"Replay x1",                               [&]() { action = 1; }},
                    {"Replay x" + String(g_repeatCount),         [&]() { action = 2; }},
                    {"Save to SD",                               [&]() { action = 3; }},
                    {"Discard",                                  [&]() { action = 4; }},
                    {"Settings",                                 [&]() { action = 5; }},
                    {"Back",                                     [&]() { action = 0; }},
                };
                loopOptions(options);

                if (action == 1 || action == 2) {
                    rx.end();
                    deinitRfModule(); // park CC1101 in IDLE before TX reprograms it (see perform_manual_retransmit)
                    int times = (action == 2) ? g_repeatCount : 1;
                    // Clean paths get `times` passed straight to the encoder
                    // (one continuous burst with the protocol's own correct
                    // inter-repeat gap, like a real remote). Raw fallback has
                    // no such framing, so it's the only case that still
                    // loops externally with our own inter-call delay.
                    bool cleanOk = false;
                    if (capturedKind == MatchKind::OOK) cleanOk = transmit_clean_ook(capturedCode, g_freq, times);
                    else if (capturedKind == MatchKind::KEELOQ) cleanOk = transmit_clean_keeloq(capturedCode.key, g_freq, times);
                    if (capturedKind == MatchKind::NONE || !cleanOk) {
                        for (int i = 0; i < times; i++) {
                            transmit_durations(captured, g_freq);
                            if (i < times - 1) delay(g_repeatDelayMs);
                        }
                    }
                    if (!reinit_rx(rx)) {
                        displayError("RX re-init failed", true);
                        return;
                    }
                    // Always resume listening after a replay with no further
                    // input needed -- that's the point of a repeater. Save to
                    // SD first (SEL -> Save) if you want to keep this one.
                    haveCapture = false;
                    captured.clear();
                    info = "";
                } else if (action == 3) {
                    save_capture(captured, g_freq);
                } else if (action == 4) {
                    haveCapture = false;
                    captured.clear();
                    info = "";
                } else if (action == 5) {
                    float prevFreq = g_freq, prevBw = g_bw;
                    int prevAgc = g_agc;
                    settings_menu();
                    if (g_requestRetransmit) {
                        g_requestRetransmit = false;
                        perform_manual_retransmit(rx);
                    } else if (g_freq != prevFreq || g_bw != prevBw || g_agc != prevAgc) {
                        rx.end();
                        deinitRfModule();
                        if (!reinit_rx(rx)) {
                            displayError("CC1101 re-init failed", true);
                            return;
                        }
                    }
                    // Opening Settings from the capture-review screen implies
                    // you're done with that capture, not that you want to keep
                    // staring at a frozen result. Resume listening instead of
                    // silently sitting on stale data with polling disabled
                    // (the main loop only polls when !haveCapture).
                    haveCapture = false;
                    captured.clear();
                    info = "";
                }
            } else {
                float prevFreq = g_freq, prevBw = g_bw;
                int prevAgc = g_agc;
                settings_menu();
                if (g_requestRetransmit) {
                    g_requestRetransmit = false;
                    perform_manual_retransmit(rx);
                } else if (g_freq != prevFreq || g_bw != prevBw || g_agc != prevAgc) {
                    rx.end();
                    deinitRfModule();
                    if (!reinit_rx(rx)) {
                        displayError("CC1101 re-init failed", true);
                        return;
                    }
                }
            }
            // Settings and the capture-review SEL menu both draw their own
            // full-screen UI via loopOptions(), overwriting our border/title
            // -- restore the full chrome here, not just the dynamic bits.
            draw_screen(haveCapture, info, rssi, !haveCapture, true);
        }

        if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) rssi = ELECHOUSE_cc1101.getRssi();

        if (!haveCapture) {
            std::vector<int> durations;
            bool polled = rx.poll(durations);
            if (polled) {
                Serial.printf("[Repeater] poll() returned %u durations\n", (unsigned)durations.size());
            }
            if (polled && millis() - g_armedAtMs < ARM_SETTLE_MS) {
                Serial.println("[Repeater] ignoring capture inside post-arm settle window");
                polled = false;
            }
            // Reverted to 20 (was briefly raised to 40, which silenced the
            // app entirely -- a real single-frame capture runs ~39 edges,
            // right under that bar). The noise-clobbering problem this was
            // meant to solve is now fixed properly below: only a recognized
            // capture updates g_lastDurations/g_lastKind, so a noise blip
            // can no longer overwrite the real "last capture" regardless of
            // what threshold is used here.
            if (polled && durations.size() > 20) {
                Serial.println("[Repeater] capture accepted, identifying...");
                wakeUpScreen(); // a signal arrived -- light up even if the display had dimmed/blanked while idly listening
                MatchKind kind = MatchKind::NONE;
                RfCodes matched;
                String capInfo = identify_capture(durations, kind, matched);
                bool recognized = (kind != MatchKind::NONE);
                Serial.printf("[Repeater] identified: %s (recognized=%d)\n", capInfo.c_str(), (int)recognized);

                // Only remember a capture as "last" when it's an actual
                // recognized protocol/KeeLoq match. Previously this ran
                // unconditionally, so a stray unrecognized noise blip
                // arriving after a real signal would silently clobber
                // g_lastDurations/g_lastKind -- "Retransmit last capture"
                // in Settings would then resend garbage noise instead of
                // the real signal, with no visible effect on the receiver.
                if (recognized) {
                    g_haveLast = true;
                    g_lastDurations = durations;
                    g_lastKind = kind;
                    g_lastCode = matched;
                    g_lastInfo = capInfo;
                }

                // A length-only "probably real" fallback (previously
                // >=150 edges) turned out to be too permissive in an RF-busy
                // lab with other SDRs/transmitters around: ambient activity
                // from that other gear can easily be long enough to clear
                // any fixed edge-count bar without being anything the user
                // actually sent, causing unwanted auto-transmits. In
                // auto-repeat (hands-off) mode, only transmit on an actual
                // protocol/cipher match -- the one signal that's reliably
                // distinguishable from noise by more than just length. Manual
                // mode (SEL review) still shows raw/unrecognized captures too
                // since nothing there transmits without the user choosing to.
                bool shouldTransmit = recognized;

                if (g_autoRepeat) {
                    if (!shouldTransmit) {
                        Serial.println("[Repeater] unrecognized capture, ignoring (not transmitting)");
                    } else {
                        // Hands-off mode: pause briefly (lets the original
                        // transmitter finish before we key up on the same
                        // frequency), replay, then go straight back to
                        // listening -- no SEL, no freeze, no review step.
                        // (draw_screen only prints `info` when
                        // haveCapture=true, hence passing true here just to
                        // render these status lines.)
                        draw_screen(true, "Captured, waiting " + String(g_prePauseMs / 1000) + "s: " + capInfo, rssi, false);
                        delay(g_prePauseMs);
                        draw_screen(true, "Repeating: " + capInfo, rssi, false);
                        rx.end();
                        deinitRfModule(); // park CC1101 in IDLE before TX reprograms it (see perform_manual_retransmit)
                        // A recognized protocol gets a clean re-synthesized
                        // waveform instead of a noisy raw replay, with
                        // g_repeatCount passed straight to the encoder as one
                        // continuous burst (the protocol's own correct
                        // inter-repeat gap) -- this is what actually fixed
                        // "Flipper detects it but doesn't get a clean read"
                        // for a matched Princeton code. Raw replay (looped
                        // externally, no such framing available) is the
                        // fallback for genuinely uncatalogued (but clearly
                        // real, long) signals.
                        bool cleanOk = false;
                        if (kind == MatchKind::OOK) cleanOk = transmit_clean_ook(matched, g_freq, g_repeatCount);
                        else if (kind == MatchKind::KEELOQ) cleanOk = transmit_clean_keeloq(matched.key, g_freq, g_repeatCount);
                        if (kind == MatchKind::NONE || !cleanOk) {
                            for (int i = 0; i < g_repeatCount; i++) {
                                transmit_durations(durations, g_freq);
                                if (i < g_repeatCount - 1) delay(g_repeatDelayMs);
                            }
                        }
                        if (!reinit_rx(rx)) {
                            displayError("RX re-init failed", true);
                            return;
                        }
                        draw_screen(false, "", rssi, true);
                        lastDraw = millis();
                    }
                } else {
                    captured = durations;
                    haveCapture = true;
                    info = capInfo;
                    capturedKind = kind;
                    capturedCode = matched;
                    draw_screen(true, info, rssi, false);
                }
            }
        }

        uint32_t now = millis();
        static uint32_t lastHeartbeat = 0;
        if (!haveCapture && now - lastHeartbeat > 5000) {
            Serial.printf("[Repeater] listening... rssi=%d\n", rssi);
            lastHeartbeat = now;
        }
        if (!haveCapture && now - lastDraw > 150) {
            draw_screen(false, "", rssi, true);
            lastDraw = now;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    rx.end();
    deinitRfModule();
}
