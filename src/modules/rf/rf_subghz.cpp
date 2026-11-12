// Native SubGHZ app, modeled on Flipper Unleashed's Sub-GHz menu. Built
// almost entirely on top of existing, already-working Bruce RF primitives
// rather than duplicating them:
//   - "Read" / "Read RAW"     -> RFScan(false)/RFScan(true) (the existing
//                                 "Scan/copy" app, pre-seeded into decode-only
//                                 or raw-only mode instead of always starting
//                                 in the member default and relying on the
//                                 in-app Next-press toggle).
//   - "Saved"                  -> sendCustomRF() (existing SD/LittleFS .sub
//                                 browser + readSubFile + txSubFile/
//                                 loopEmulate replay, unchanged).
//   - "Add Manually"           -> genuinely new, but built on readSubFile's
//                                 sibling primitives: the OOK registry
//                                 (rf_find_protocol/rf_protocol_at) for fixed
//                                 codes, KeeloqKeystore + RfCodes::keeloq_step
//                                 for rolling codes, then rfSaveSignal +
//                                 loopEmulate to save/test-transmit exactly
//                                 like a captured signal would be.
//   - "Frequency Analyzer"     -> genuinely new: an interactive version of
//                                 the existing headless rf_scan() sweep
//                                 (same RSSI-peak + two-pass-confirm logic),
//                                 with a live display, stop-on-hit, then a
//                                 capture+identify attempt at that frequency
//                                 using the same RfRxSession/rf_identify_
//                                 capture primitives as the Repeater app.
//   - "Radio Settings"         -> genuinely new, but the same freq/BW/AGC
//                                 settings-menu pattern already used by the
//                                 Repeater and Weather Station apps.
#include "rf_subghz.h"
#include "rf_scan.h"       // RFScan, rfSaveSignal
#include "rf_send.h"       // sendCustomRF, bitList/keyList/etc externs
#include "rf_capture_tx.h" // rf_identify_capture, rf_transmit_clean_*, rf_transmit_raw
#include "protocols/rf_decoder.h"  // RfRxSession, rf_build_raw
#include "protocols/rf_registry.h" // rf_find_protocol, rf_protocol_at/count
#include "protocols/rf_keeloq.h"   // KeeloqKeystore, keeloq_mfcodes_fs
#include "rf_utils.h"
#include "core/display.h"
#include "core/sd_functions.h"
#include "core/mykeyboard.h"      // hex_keyboard/num_keyboard
#include "core/type_convertion.h" // decimalToHexString
#include <ELECHOUSE_CC1101_SRC_DRV.h>

// Defined in rf_send.cpp (plain globals, not exposed via rf_send.h) --
// loopEmulate()'s non-KeeLoq replay branch walks these the same way
// readSubFile() populates them from a multi-code .sub file.
extern std::vector<uint64_t> keyList;
extern std::vector<int> bitList;
// Defined in rf_utils.cpp (plain global, not exposed via rf_utils.h) --
// initRfModule("rx", ...) applies this on top of its ASK/OOK default. Every
// native app here (Repeater, Weather Station, Read/Read RAW) only ever
// listens for OOK; this is the knob needed to try an FSK-modulated signal
// (many automotive keyless-entry/TPMS fobs, unlike most gate/garage OOK
// remotes) -- without it, OOK edge-detection on an FSK carrier produces
// short, inconsistent, noise-looking captures even with strong RSSI, since
// RSSI reflects raw channel power regardless of modulation but the RMT
// OOK-envelope capture has nothing coherent to lock onto.
extern uint8_t bruceScriptRxModulation;
// Same mechanism, for AGC: 0=default, 1=0xC7, 2=0xC3, 3=Max Sensitivity
// (0x07, same preset Repeater/Weather Station use). Exposed as a selectable
// Read/Read RAW setting rather than forced on -- forcing it on earlier
// overloaded/distorted an already-strong signal (confirmed 2026-11-12:
// garbled a clean Princeton capture into 173 noisy edges); a weak signal
// (e.g. an old key fob's dying battery) is the opposite problem and
// benefits from it. Let the user pick per-signal instead of guessing once
// for everyone.
extern uint8_t bruceScriptRxAgc;

namespace {

// ---------------------------------------------------------------------------
// Shared settings (Radio Settings menu). g_freq is the fixed-tune target
// used by Add Manually's test-transmit; Frequency Analyzer sweeps regardless
// of g_freq but still uses g_bw/g_agc once it locks onto a candidate and
// tries to capture+decode there. "Read"/"Read RAW" (RFScan) are NOT driven by
// these -- they keep using their own existing in-app Range/Threshold options,
// same as the stock "Scan/copy" entry.
// ---------------------------------------------------------------------------
float g_freq = 433.92f;
float g_bw = 135.0f;
int g_agc = 3; // Max Sensitivity default, same reasoning as Repeater/Weather Station
bool g_autoSave = false;
int g_faBand = 3; // 0=433MHz region, 1=315MHz, 2=868/915MHz, 3=All (sequential)
int g_repeatCount = 3; // matches the Repeater's on-device-confirmed default

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

const char *band_label(int band) {
    switch (band) {
        case 0: return "433 MHz region";
        case 1: return "315 MHz region";
        case 2: return "868/915 MHz region";
        default: return "All (slower)";
    }
}

const char *rx_modulation_label() {
    switch (bruceScriptRxModulation) {
        case 2: return "2-FSK";
        case 3: return "GFSK";
        default: return "OOK/ASK";
    }
}

const char *rx_agc_label() {
    switch (bruceScriptRxAgc) {
        case 1: return "AGC C7";
        case 2: return "AGC C3 (test)";
        case 3: return "Max Sensitivity";
        default: return "Default";
    }
}

void radio_settings_menu() {
    int option = 0;
    options = {
        {"Frequency: " + String(g_freq, 2) + " MHz (Add Manually)", [&]() { option = 1; }},
        {"RX BW: " + String((int)g_bw) + " kHz",                    [&]() { option = 2; }},
        {"AGC: " + String(agc_label(g_agc)),                        [&]() { option = 3; }},
        {"Scan band: " + String(band_label(g_faBand)),              [&]() { option = 4; }},
        {"Repeat count: " + String(g_repeatCount),                  [&]() { option = 5; }},
        {"Auto-save found signals: " + String(g_autoSave ? "ON" : "OFF"), [&]() { g_autoSave = !g_autoSave; }},
        {"RX Modulation: " + String(rx_modulation_label()) + " (Read/Read RAW)", [&]() { option = 6; }},
        {"Read/Read RAW AGC: " + String(rx_agc_label()),            [&]() { option = 7; }},
        {"Back",                                                    [&]() { option = 0; }},
    };
    loopOptions(options);

    if (option == 1) {
        int freqOpt = -1;
        options = {
            {"433.92 MHz", [&]() { freqOpt = 0; }}, {"433.82 MHz", [&]() { freqOpt = 1; }},
            {"315 MHz",    [&]() { freqOpt = 2; }}, {"868.35 MHz", [&]() { freqOpt = 3; }},
            {"915 MHz",    [&]() { freqOpt = 4; }}, {"Custom",     [&]() { freqOpt = 5; }},
            {"Cancel",     [&]() { freqOpt = -1; }},
        };
        loopOptions(options);
        const float vals[] = {433.92f, 433.82f, 315.0f, 868.35f, 915.0f};
        float newFreq = -1.0f;
        if (freqOpt >= 0 && freqOpt < 5) {
            newFreq = vals[freqOpt];
        } else if (freqOpt == 5) {
            // Presets don't cover every band -- e.g. some automotive
            // keyless-entry fobs (confirmed with a 2011 Honda CR-V,
            // 2026-11-12) sit around 312-313 MHz, below the 315 MHz preset.
            String s = num_keyboard(String(g_freq, 2), 8, "Frequency (MHz)");
            if (s != "") newFreq = s.toFloat();
        }
        if (newFreq > 0) {
            g_freq = newFreq;
            // g_freq alone only drives Frequency Analyzer/Add Manually.
            // Read/Read RAW (RFScan) read bruceConfigPins.rfFreq directly
            // and were never affected by this menu at all -- set it too,
            // fixed (not scan-range) mode, so this one settings screen
            // actually controls every SubGHZ screen's tuning.
            bruceConfigPins.setRfFreq(g_freq, 1);
        }
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
        int bandOpt = -1;
        options = {
            {"433 MHz region",        [&]() { bandOpt = 0; }},
            {"315 MHz region",        [&]() { bandOpt = 1; }},
            {"868/915 MHz region",    [&]() { bandOpt = 2; }},
            {"All (slower)",          [&]() { bandOpt = 3; }},
            {"Cancel",                [&]() { bandOpt = -1; }},
        };
        loopOptions(options);
        if (bandOpt >= 0) g_faBand = bandOpt;
    } else if (option == 5) {
        int cntOpt = -1;
        options = {
            {"1", [&]() { cntOpt = 1; }}, {"3", [&]() { cntOpt = 3; }},
            {"5", [&]() { cntOpt = 5; }}, {"10", [&]() { cntOpt = 10; }},
            {"Cancel", [&]() { cntOpt = -1; }},
        };
        loopOptions(options);
        if (cntOpt > 0) g_repeatCount = cntOpt;
    } else if (option == 6) {
        int modOpt = -2;
        options = {
            {"OOK/ASK (most gate/garage remotes)", [&]() { modOpt = 1; }},
            {"2-FSK (many car/TPMS fobs)",          [&]() { modOpt = 2; }},
            {"GFSK",                                [&]() { modOpt = 3; }},
            {"Cancel",                              [&]() { modOpt = -2; }},
        };
        loopOptions(options);
        if (modOpt != -2) bruceScriptRxModulation = (uint8_t)modOpt;
    } else if (option == 7) {
        int agcOpt = -2;
        options = {
            {"Default",                       [&]() { agcOpt = 0; }},
            {"AGC C7",                         [&]() { agcOpt = 1; }},
            {"AGC C3 (test)",                  [&]() { agcOpt = 2; }},
            {"Max Sensitivity (weak signals)", [&]() { agcOpt = 3; }},
            {"Cancel",                         [&]() { agcOpt = -2; }},
        };
        loopOptions(options);
        if (agcOpt != -2) bruceScriptRxAgc = (uint8_t)agcOpt;
    }
}

// ---------------------------------------------------------------------------
// Saved: the stock SD/LittleFS .sub browser already does everything the
// Flipper "Saved" screen needs (browse, rename/delete inline, read, replay).
// ---------------------------------------------------------------------------
void saved_menu() { sendCustomRF(); }

// ---------------------------------------------------------------------------
// Add Manually: generate a valid signal from scratch (no capture needed),
// save it, and offer an immediate test-transmit via the existing loopEmulate
// screen (SEL = send, Next = Counter step/Repeat/Close/Main menu).
// ---------------------------------------------------------------------------
void save_and_emulate(RfCodes &data, bool raw) {
    char hexString[64] = {0};
    decimalToHexString(data.key, hexString);
    rfSaveSignal((float)data.frequency, data, raw, hexString, g_autoSave);
    loopEmulate(data);
}

void add_manually_fixed() {
    const RfProtocolDef *chosen = nullptr;
    options = {};
    int n = rf_protocol_count();
    for (int i = 0; i < n; i++) {
        const RfProtocolDef *def = rf_protocol_at(i);
        if (!def) continue;
        String name = def->name;
        options.emplace_back(name, [&chosen, def]() { chosen = def; });
    }
    options.emplace_back("Cancel", [&chosen]() { chosen = nullptr; });
    loopOptions(options);
    if (!chosen) return;

    String keyStr = hex_keyboard("", 16, "Enter key (hex)");
    if (keyStr == "") return;
    uint64_t key = strtoull(keyStr.c_str(), nullptr, 16);

    RfCodes data;
    data.protocol = chosen->name;
    data.preset = "Ook270Async";
    data.Bit = chosen->bits ? chosen->bits : 24;
    data.te = chosen->te;
    data.key = key;
    data.frequency = (uint32_t)(g_freq * 1000000.0f);

    // loopEmulate()'s non-KeeLoq (data.serial==0) branch replays by walking
    // the global keyList/bitList (the same vectors readSubFile populates for
    // a multi-code .sub) rather than data.key/data.Bit directly.
    keyList.clear();
    bitList.clear();
    keyList.push_back(key);
    bitList.push_back(data.Bit);

    save_and_emulate(data, false);
}

void add_manually_keeloq() {
    KeeloqKeystore keystore(keeloq_mfcodes_fs());
    const std::vector<KeeloqKey> &keys = keystore.get_keys();
    if (keys.empty()) {
        displayError("No /mfcodes keystore found", true);
        return;
    }
    String chosenName = "";
    options = {};
    for (const auto &k : keys) {
        String name = k.mf_name;
        options.emplace_back(name, [&chosenName, name]() { chosenName = name; });
    }
    options.emplace_back("Cancel", [&chosenName]() { chosenName = ""; });
    loopOptions(options);
    if (chosenName == "") return;

    String serialStr = hex_keyboard("", 8, "Enter serial (hex)");
    if (serialStr == "") return;
    uint32_t serial = (uint32_t)strtoul(serialStr.c_str(), nullptr, 16);

    String btnStr = num_keyboard("1", 2, "Button number");
    uint8_t btn = (uint8_t)btnStr.toInt();

    String cntStr = num_keyboard("1", 5, "Starting counter");
    uint16_t cnt = (uint16_t)cntStr.toInt();

    RfCodes data;
    data.mf_name = chosenName;
    data.serial = serial;
    data.btn = btn;
    data.cnt = cnt;
    data.frequency = (uint32_t)(g_freq * 1000000.0f);
    data.preset = "Ook650Async";
    data.protocol = "KeeLoq";
    data.Bit = 64;
    data.fix = (uint32_t)btn << 28 | serial;
    data.keeloq_step(0); // derives the manufacturer key and encodes the hop -> data.key

    save_and_emulate(data, false);
}

void add_manually_menu() {
    options = {
        {"Fixed/Static code",    add_manually_fixed},
        {"KeeLoq (rolling code)", add_manually_keeloq},
        {"Back",                 []() {}},
    };
    loopOptions(options);
}

// ---------------------------------------------------------------------------
// Frequency Analyzer: an interactive version of the existing headless
// rf_scan() sweep (same RSSI-peak-tracking + two-pass-confirm technique), but
// stopping the instant a candidate is confirmed instead of accumulating a
// list, then attempting an actual capture+decode there -- same RfRxSession
// capture mechanics the Repeater app already uses and has been verified
// on-device.
// ---------------------------------------------------------------------------
constexpr uint32_t FA_ARM_SETTLE_MS = 400; // see Repeater: CC1101 RX-mode settling transient
constexpr uint32_t FA_CAPTURE_WINDOW_MS = 5000;

// A continuous fine-step sweep (the original design here, and what the
// headless rf_scan() CLI command also does) spends nearly all its time
// retuned to dead air between the specific channels real sub-GHz remotes
// and sensors actually use. Two independent reference implementations
// (github.com/smolbun/CC1101-Frequency-Analyzer, inspired by Flipper
// Zero's own frequency analyzer worker; github.com/simondankelmann/
// SubMarine's ESP32 firmware, which drives a CC1101 via this exact same
// SmartRC-CC1101-Driver-Lib) both instead scan a short, curated list of
// known ISM channels. Merged from both of those lists plus Bruce's own
// valid-frequency presets.
const float FA_CHANNELS_433[] = {
    387.00f, 390.00f, 418.00f, 430.00f, 430.50f, 431.00f, 431.50f, 433.075f, 433.22f,
    433.42f, 433.657f, 433.82f, 433.889f, 433.92f, 434.075f, 434.177f, 434.19f, 434.39f,
    434.42f, 434.62f, 434.775f, 438.90f, 440.175f, 464.00f, 467.75f,
};
const float FA_CHANNELS_315[] = {
    300.00f, 303.87f, 303.90f, 304.25f, 307.00f, 307.50f, 307.80f, 309.00f, 310.00f,
    312.00f, 312.10f, 312.20f, 313.00f, 313.85f, 314.00f, 314.35f, 314.98f, 315.00f,
    318.00f, 330.00f, 345.00f, 348.00f,
};
const float FA_CHANNELS_868[] = {
    779.00f, 868.30f, 868.35f, 868.40f, 868.80f, 868.865f, 868.95f, 906.40f, 915.00f,
    925.00f, 928.00f,
};

struct FaChannelList {
    const float *channels;
    int count;
};

// A persistent interferer (background noise, a nearby always-on transmitter)
// is by definition the loudest thing in the band on every single sweep pass,
// so the plain "same peak frequency on two consecutive passes" confirmation
// (same technique the stock rf_scan() CLI command uses) locks onto it
// immediately and re-locks onto it again the instant scanning resumes --
// "stuck on the highest ones", confirmed live in a noisy room. A genuine
// button-press signal is transient: it starts, gets captured, and (for a
// static code) usually isn't repeated for a while. So once a confirmed
// candidate frequency fails to yield a decodable capture, temporarily
// exclude that exact spot so the sweep moves on instead of hammering it.
struct FaBlacklistEntry {
    long freqKey; // MHz * 100, so +-0.15MHz matches within one entry
    uint32_t until;
};
std::vector<FaBlacklistEntry> g_faBlacklist;

bool fa_is_blacklisted(float freq) {
    long fk = (long)(freq * 100);
    uint32_t now = millis();
    for (auto &e : g_faBlacklist) {
        if (labs(e.freqKey - fk) <= 15 && now < e.until) return true;
    }
    return false;
}

void fa_blacklist(float freq, uint32_t cooldownMs) {
    long fk = (long)(freq * 100);
    uint32_t until = millis() + cooldownMs;
    for (auto &e : g_faBlacklist) {
        if (labs(e.freqKey - fk) <= 15) {
            e.until = until;
            return;
        }
    }
    if (g_faBlacklist.size() > 16) g_faBlacklist.erase(g_faBlacklist.begin());
    g_faBlacklist.push_back({fk, until});
}

void fa_draw(const String &line1, const String &line2, int rssi, bool fullRedraw) {
    if (fullRedraw) {
        tft.fillScreen(bruceConfig.bgColor);
        drawMainBorder();
        tft.setTextSize(FP);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setCursor(10, 26);
        tft.print("Frequency Analyzer");
    }

    tft.setTextSize(FP);
    tft.fillRect(10, 44, tftWidth - 20, 16, bruceConfig.bgColor);
    tft.setCursor(10, 44);
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    tft.print(line1);

    int barMax = 120;
    int barW = max(0, min(barMax, (int)((rssi + 100) * (barMax / 70.0f))));
    tft.fillRect(10, 64, barMax + 2, 10, getColorVariation(bruceConfig.priColor));
    tft.fillRect(11, 65, barW, 8, bruceConfig.priColor);
    tft.fillRect(10 + barMax + 10, 64, tftWidth - (10 + barMax + 10) - 10, 10, bruceConfig.bgColor);
    tft.setCursor(10 + barMax + 10, 64);
    tft.print(rssi);
    tft.print(" dBm");

    tft.fillRect(10, 84, tftWidth - 20, 32, bruceConfig.bgColor);
    tft.setCursor(10, 84);
    tft.print(line2);

    if (fullRedraw) {
        tft.setCursor(10, tftHeight - 16);
        tft.print("ESC: exit");
    }
}

enum class FaSweepResult { FOUND, EXHAUSTED, SETTINGS, ESCAPED };

// Dwell per channel, sampling RSSI repeatedly and keeping the PEAK over the
// window -- a single instantaneous sample right after retuning (the
// original design here) is NOT how any of the reference implementations
// checked do this: ESP32-Bit-Pirate's measurePeakRssi()/
// analyzeFrequencyActivity() and CC1101-Frequency-Finder's 15ms dwell loop
// both sample repeatedly and take the max.
constexpr uint32_t FA_DWELL_MS = 20;

// Scans a short, curated list of known ISM channels (see FA_CHANNELS_* above)
// instead of a continuous fine-step sweep across the whole band -- a
// continuous sweep spends nearly all its time retuned to dead air between
// the specific frequencies real sub-GHz remotes/sensors actually use, which
// independent reference implementations
// (github.com/smolbun/CC1101-Frequency-Analyzer, inspired by Flipper Zero's
// own frequency analyzer worker; github.com/simondankelmann/SubMarine and
// github.com/geo-tp/ESP32-Bit-Pirate, both driving a CC1101 via this same
// SmartRC-CC1101-Driver-Lib) all avoid by scanning a discrete channel list.
//
// Same RSSI-peak + two-consecutive-passes-agree confirmation rf_scan() uses,
// but returns as soon as a hit is confirmed (rather than accumulating a list
// across the whole range), redraws a live progress line while it runs, and
// checks SEL/Esc every step. maxPasses limits how long this scans ONE list
// before giving up and letting the caller move to the next band -- it used
// to loop internally forever (only Esc or a confirmed hit would ever
// return), which meant "All bands" never actually advanced past the first
// band if nothing was found there, and SEL (settings) did nothing while
// scanning since this loop never checked for it at all.
FaSweepResult fa_sweep_channels(
    const float *channels, int channelCount, float &foundFreq, int &foundRssi, int maxPasses
) {
    int idx = 0;
    float markFreq = 0;
    int markRssi = -100;
    long compareFreq = 0;
    uint32_t lastDraw = 0;

    // Ground-truth instrumentation: the previous "pass done" log only ever
    // printed markRssi, which stays at its -100 init value whenever nothing
    // crosses the -75 threshold -- that told us nothing about what getRssi()
    // was ACTUALLY returning. Track the real min/max/last raw value every
    // step, and print a tick every 500ms so this can be correlated against
    // when a signal was actually being sent.
    int passMin = 127;
    int passMax = -128;
    uint32_t lastTick = 0;
    int passCount = 0;

    while (true) {
        if (check(EscPress)) return FaSweepResult::ESCAPED;
        if (check(SelPress)) return FaSweepResult::SETTINGS;

        float freq = channels[idx];
        // SetRx(freq) (the float overload) does SIDLE -> setMHZ(freq) -> SRX
        // internally -- the same live-retune sequence production tools
        // (ESP32-Bit-Pirate's SubGhzService::tune(), SubMarine's
        // initCC1101()) use.
        ELECHOUSE_cc1101.SetRx(freq);

        uint32_t dwellStart = millis();
        int rssi = -128;
        while (millis() - dwellStart < FA_DWELL_MS) {
            int r = ELECHOUSE_cc1101.getRssi();
            if (r > rssi) rssi = r;
            delayMicroseconds(900);
        }
        if (rssi < passMin) passMin = rssi;
        if (rssi > passMax) passMax = rssi;
        // A blacklisted (still cooling down) frequency is excluded from
        // winning peak-tracking at all -- not just at the confirm step --
        // otherwise it keeps "winning" every single pass and never lets a
        // genuinely different frequency become the new peak.
        if (rssi > -75 && rssi > markRssi && !fa_is_blacklisted(freq)) {
            markRssi = rssi;
            markFreq = freq;
        }

        uint32_t now = millis();
        if (now - lastTick > 500) {
            Serial.printf(
                "[FreqAnalyzer] tick freq=%.3f rssi=%d (pass min=%d max=%d so far)\n", freq, rssi, passMin, passMax
            );
            lastTick = now;
        }
        if (now - lastDraw > 100) {
            fa_draw(
                "Scanning " + String(channels[0], 0) + "-" + String(channels[channelCount - 1], 0) + " MHz",
                "Peak: " + String(markFreq, 3) + " MHz", rssi, false
            );
            lastDraw = now;
        }

        idx++;
        if (idx >= channelCount) {
            idx = 0;
            passCount++;
            Serial.printf(
                "[FreqAnalyzer] pass done (%d/%d): peak=%.3fMHz rssi=%d (raw min=%d max=%d)\n", passCount, maxPasses,
                markFreq, markRssi, passMin, passMax
            );
            passMin = 127;
            passMax = -128;
            if (markRssi > -75) {
                long fr = (long)(markFreq * 100);
                if (fr == compareFreq) {
                    Serial.printf("[FreqAnalyzer] confirmed: %.3f MHz @ %d dBm\n", markFreq, markRssi);
                    foundFreq = markFreq;
                    foundRssi = markRssi;
                    return FaSweepResult::FOUND;
                } else {
                    Serial.printf("[FreqAnalyzer] tentative peak, re-checking next pass: %.3f\n", markFreq);
                    compareFreq = fr;
                }
            }
            markFreq = 0;
            markRssi = -100;
            if (passCount >= maxPasses) {
                Serial.printf("[FreqAnalyzer] channel list exhausted after %d passes, moving on\n", maxPasses);
                return FaSweepResult::EXHAUSTED;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Tunes exactly to foundFreq and tries to capture+identify a signal there
// for up to FA_CAPTURE_WINDOW_MS. Returns true if something was captured
// (durations non-empty); `kind`/`outCode`/`info` describe what, if anything,
// was recognized.
bool fa_capture_at(float foundFreq, std::vector<int> &durations, RfMatchKind &kind, RfCodes &outCode, String &info) {
    Serial.printf("[FreqAnalyzer] capture_at %.2f MHz: arming RX\n", foundFreq);
    bruceConfigPins.rfFreq = foundFreq;
    RfRxSession rx;
    if (!rx.begin()) {
        Serial.println("[FreqAnalyzer] capture_at: rx.begin() failed");
        return false;
    }
    ELECHOUSE_cc1101.setRxBW(g_bw);
    apply_agc(g_agc);
    uint32_t armedAt = millis();

    bool captured = false;
    while (millis() - armedAt < FA_CAPTURE_WINDOW_MS) {
        if (check(EscPress)) break;
        std::vector<int> d;
        if (rx.poll(d)) {
            Serial.printf("[FreqAnalyzer] poll() returned %u durations\n", (unsigned)d.size());
            if (millis() - armedAt < FA_ARM_SETTLE_MS) {
                Serial.println("[FreqAnalyzer] ignoring capture inside post-arm settle window");
                continue;
            }
            if (d.size() > 20) {
                durations = d;
                captured = true;
                break;
            }
        }
        int rssi = ELECHOUSE_cc1101.getRssi();
        fa_draw(
            "Found " + String(foundFreq, 2) + " MHz -- capturing...", "Waiting for a decodable signal", rssi, false
        );
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    rx.end();
    deinitRfModule();

    if (!captured) {
        Serial.println("[FreqAnalyzer] capture_at: timed out, nothing captured");
        return false;
    }
    info = rf_identify_capture(durations, kind, outCode);
    Serial.printf("[FreqAnalyzer] identified: %s\n", info.c_str());
    outCode.frequency = (uint32_t)(foundFreq * 1000000.0f);
    return true;
}

void fa_handle_found(float foundFreq, int foundRssi, const std::vector<int> &durations, RfMatchKind kind, RfCodes &outCode, const String &info) {
    if (g_autoSave) {
        char hexString[64] = {0};
        decimalToHexString(outCode.key, hexString);
        if (kind == RfMatchKind::NONE) {
            String rawData;
            bool hasCrc = false;
            uint64_t crc = 0;
            std::vector<int> indexed;
            int rawBits = 0, rawTe = 0;
            rf_build_raw(durations, rawData, hasCrc, crc, indexed, rawBits, rawTe);
            outCode.data = rawData;
            outCode.preset = "Ook270Async";
            rfSaveSignal(foundFreq, outCode, true, hexString, true);
        } else {
            rfSaveSignal(foundFreq, outCode, false, hexString, true);
        }
        wakeUpScreen();
        fa_draw(
            "Found " + String(foundFreq, 2) + " MHz, auto-saved", info, foundRssi, true
        );
        delay(1500);
        return;
    }

    wakeUpScreen();
    int action = 0;
    options = {
        {"Replay x1",                       [&]() { action = 1; }},
        {"Replay x" + String(g_repeatCount), [&]() { action = 2; }},
        {"Save",                            [&]() { action = 3; }},
        {"Discard / keep scanning",         [&]() { action = 0; }},
    };
    // loopOptions() draws its own full-screen menu immediately, which was
    // overwriting this before it was ever visible -- the found/decoded info
    // never actually got seen. Hold it on screen for a beat first, then
    // keep it as the menu's subtitle so it's still visible while choosing.
    fa_draw("Found " + String(foundFreq, 2) + " MHz, " + String(foundRssi) + " dBm", info, foundRssi, true);
    delay(2000);
    loopOptions(options, MENU_TYPE_REGULAR, info.c_str());

    if (action == 1 || action == 2) {
        int times = (action == 2) ? g_repeatCount : 1;
        bool cleanOk = false;
        if (kind == RfMatchKind::OOK) cleanOk = rf_transmit_clean_ook(outCode, foundFreq, times);
        else if (kind == RfMatchKind::KEELOQ) cleanOk = rf_transmit_clean_keeloq(outCode.key, foundFreq, times);
        if (kind == RfMatchKind::NONE || !cleanOk) rf_transmit_raw(durations, foundFreq);
    } else if (action == 3) {
        char hexString[64] = {0};
        decimalToHexString(outCode.key, hexString);
        if (kind == RfMatchKind::NONE) {
            String rawData;
            bool hasCrc = false;
            uint64_t crc = 0;
            std::vector<int> indexed;
            int rawBits = 0, rawTe = 0;
            rf_build_raw(durations, rawData, hasCrc, crc, indexed, rawBits, rawTe);
            outCode.data = rawData;
            outCode.preset = "Ook270Async";
            rfSaveSignal(foundFreq, outCode, true, hexString, false);
        } else {
            rfSaveSignal(foundFreq, outCode, false, hexString, false);
        }
    }
}

void frequency_analyzer() {
    const int chCount433 = sizeof(FA_CHANNELS_433) / sizeof(float);
    const int chCount315 = sizeof(FA_CHANNELS_315) / sizeof(float);
    const int chCount868 = sizeof(FA_CHANNELS_868) / sizeof(float);
    const FaChannelList list433 = {FA_CHANNELS_433, chCount433};
    const FaChannelList list315 = {FA_CHANNELS_315, chCount315};
    const FaChannelList list868 = {FA_CHANNELS_868, chCount868};
    const FaChannelList bands433[] = {list433};
    const FaChannelList bands315[] = {list315};
    const FaChannelList bands868[] = {list868};
    const FaChannelList bandsAll[] = {list433, list315, list868};

    // Resolves the CURRENT band selection from g_faBand -- called every
    // outer-loop iteration (not just once at the top) so that changing
    // "Scan band" in Settings mid-session actually takes effect on the next
    // sweep instead of being locked in from whatever it was when this
    // screen was first opened.
    auto resolveBands = [&](const FaChannelList *&bands, int &bandCount) {
        switch (g_faBand) {
            case 0: bands = bands433; bandCount = 1; break;
            case 1: bands = bands315; bandCount = 1; break;
            case 2: bands = bands868; bandCount = 1; break;
            default: bands = bandsAll; bandCount = 3; break;
        }
    };
    const FaChannelList *bands;
    int bandCount;
    resolveBands(bands, bandCount);

    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayError("Frequency Analyzer needs the CC1101", true);
        return;
    }
    if (!initRfModule("rx", bands[0].channels[0])) {
        displayError("CC1101 init failed", true);
        return;
    }
    ELECHOUSE_cc1101.setRxBW(256);
    // The sweep was running on the CC1101's default AGC profile, which
    // deliberately cuts the top DVGA/LNA gain steps for better blocking near
    // strong interferers -- fine for Repeater/Weather's narrowband listening,
    // but during a wide sweep it was keeping RSSI pinned to a flat ~-90dBm
    // floor everywhere, real signal or not. Same Max Sensitivity profile
    // those apps default to.
    apply_agc(g_agc);

    fa_draw("Scanning...", "SEL: settings   ESC: exit", -100, true);

    while (!check(EscPress)) {
        if (check(SelPress)) {
            radio_settings_menu();
            resolveBands(bands, bandCount);
            fa_draw("Scanning...", "SEL: settings   ESC: exit", -100, true);
            continue;
        }

        bool found = false;
        bool settingsRequested = false;
        bool escaped = false;
        float foundFreq = 0;
        int foundRssi = -100;
        // 3 passes per channel list before giving up and moving to the next
        // band -- fa_sweep_channels() used to loop internally forever,
        // which meant "All" never actually reached band 2 or 3 unless band
        // 1 had a hit.
        for (int b = 0; b < bandCount && !found && !settingsRequested && !escaped; b++) {
            FaSweepResult r = fa_sweep_channels(bands[b].channels, bands[b].count, foundFreq, foundRssi, 3);
            if (r == FaSweepResult::FOUND) found = true;
            else if (r == FaSweepResult::SETTINGS) settingsRequested = true;
            else if (r == FaSweepResult::ESCAPED) escaped = true;
            // EXHAUSTED: fall through to the next band in this for loop.
        }
        if (escaped) break;
        if (settingsRequested) {
            radio_settings_menu();
            resolveBands(bands, bandCount);
            fa_draw("Scanning...", "SEL: settings   ESC: exit", -100, true);
            continue;
        }
        if (!found) continue; // exhausted every band this cycle -- start over from band 0

        std::vector<int> durations;
        RfMatchKind kind = RfMatchKind::NONE;
        RfCodes outCode;
        String info;
        if (fa_capture_at(foundFreq, durations, kind, outCode, info)) {
            // A real capture happened here -- don't block re-finding this
            // exact spot forever, just long enough to not instantly re-fire
            // on the same signal before the user has dealt with it.
            fa_blacklist(foundFreq, 5000);
            fa_handle_found(foundFreq, foundRssi, durations, kind, outCode, info);
        } else {
            // RSSI crossed the threshold but nothing decodable showed up in
            // the capture window -- almost certainly a persistent noise
            // source, not a button press. Exclude it for longer so the
            // sweep actually moves on to find something else.
            fa_blacklist(foundFreq, 20000);
            wakeUpScreen();
            fa_draw(
                "Energy at " + String(foundFreq, 2) + " MHz, " + String(foundRssi) + " dBm", "No decodable capture -- resuming scan",
                foundRssi, true
            );
            delay(1500);
        }

        // Resume scanning: re-init the RX module at the scan bandwidth.
        if (!initRfModule("rx", bands[0].channels[0])) {
            displayError("CC1101 re-init failed", true);
            return;
        }
        ELECHOUSE_cc1101.setRxBW(256);
    // The sweep was running on the CC1101's default AGC profile, which
    // deliberately cuts the top DVGA/LNA gain steps for better blocking near
    // strong interferers -- fine for Repeater/Weather's narrowband listening,
    // but during a wide sweep it was keeping RSSI pinned to a flat ~-90dBm
    // floor everywhere, real signal or not. Same Max Sensitivity profile
    // those apps default to.
    apply_agc(g_agc);
        fa_draw("Scanning...", "SEL: settings   ESC: exit", -100, true);
    }

    deinitRfModule();
}

} // namespace

void rf_subghz() {
    bool back = false;
    while (!back) {
        options = {
            {"Read",                []() { RFScan(false); }},
            {"Read RAW",            []() { RFScan(true); }},
            {"Saved",               saved_menu},
            {"Add Manually",        add_manually_menu},
            {"Frequency Analyzer",  frequency_analyzer},
            {"Radio Settings",      radio_settings_menu},
            {"Back",                [&]() { back = true; }},
        };
        loopOptions(options);
        if (check(EscPress)) back = true;
    }
}
