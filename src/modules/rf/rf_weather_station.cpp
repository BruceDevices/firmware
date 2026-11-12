// Native live weather-sensor monitor (AcuRite-606TX / Nexus / Prologue).
//
// Decode timing/checksum logic ported from the validated JS prototype
// (Bruce_Weather_Live_RX_Fix_v1.0.0's 02_Weather_RF_Live.js), operating
// directly on the captured duration vector instead of round-tripping through
// a RAW-string + regex parse, since this is native code with direct access
// to RfRxSession's output.
#include "rf_weather_station.h"
#include "protocols/rf_decoder.h"
#include "rf_utils.h"
#include "core/display.h"
#include "core/sd_functions.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

namespace {

struct WeatherStation {
    String model;
    int id = 0;
    int channel = 0;
    bool battery = true;
    float temp = 0;
    bool hasHumidity = false;
    int humidity = 0;
    int count = 1;
};

// Settings, RAM-only (reset to these defaults each app launch).
// Weather sensors are typically weak/distant transmitters, so this app
// defaults AGC to Max Sensitivity (see rf_utils.cpp bruceScriptRxAgc==3)
// rather than Bruce's general-purpose default, which trades sensitivity
// for better blocking/selectivity.
float g_freq = 433.92f;
float g_bw = 135.0f;
int g_agc = 3;
bool g_saveCaptures = false;
int g_saveIndex = 0;

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
    // agc==0 (Default): leave whatever initRfModule("rx", ...) already set.
}

void save_capture(const std::vector<int> &durations, float freq, const String &label) {
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) return;
    if (!fs->exists("/BruceRF")) {
        if (!fs->mkdir("/BruceRF")) return;
    }
    char filename[48];
    snprintf(filename, sizeof(filename), "/BruceRF/weather_%d.sub", g_saveIndex++);
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
    out += "Protocol: RAW\n"; // label (model/id) lives only in the filename-adjacent UI, not the .sub format
    out += "RAW_Data: " + rawData;
    file.write((const uint8_t *)out.c_str(), out.length());
    file.close();
}

// Extract fixed-width PPM frames from a capture: a short LOW gap encodes bit
// 0, a long LOW gap encodes bit 1, and any gap at/above `resetGap` ends the
// current frame (only accepted if it reached exactly `bitCount` bits).
// HIGH (positive) durations carry no information for this family of
// protocols and are skipped.
void ppm_extract_frames(
    const std::vector<int> &durations, int shortMin, int shortMax, int longMin, int longMax, int resetGap,
    int bitCount, std::vector<uint64_t> &framesOut
) {
    uint64_t bits = 0;
    int bitLen = 0;
    for (int d : durations) {
        if (d >= 0) continue;
        int gap = -d;
        if (gap >= resetGap) {
            if (bitLen == bitCount) framesOut.push_back(bits);
            bits = 0;
            bitLen = 0;
            continue;
        }
        if (gap >= shortMin && gap < shortMax) {
            bits = (bits << 1) | 0;
            bitLen++;
        } else if (gap >= longMin && gap < longMax) {
            bits = (bits << 1) | 1;
            bitLen++;
        } else {
            if (bitLen == bitCount) framesOut.push_back(bits);
            bits = 0;
            bitLen = 0;
        }
        if (bitLen > bitCount) {
            bits = 0;
            bitLen = 0;
        }
    }
    if (bitLen == bitCount) framesOut.push_back(bits);
}

// Extract bits [a,b) (MSB-first, 0-indexed like the JS .slice(a,b) this
// mirrors) out of a `width`-bit frame value.
uint32_t bitslice(uint64_t frame, int width, int a, int b) {
    int shift = width - b;
    uint32_t mask = (b - a >= 32) ? 0xFFFFFFFFu : ((1u << (b - a)) - 1u);
    return (uint32_t)((frame >> shift) & mask);
}

int32_t signed12(uint32_t n) { return n >= 2048 ? (int32_t)n - 4096 : (int32_t)n; }

// AcuRite-606TX: 32-bit OOK/PPM, 8-bit LFSR checksum (poly 0x98, key 0xF1)
// over the first 3 bytes, verified against the 4th. Self-verifying - no
// repeat count required, unlike Nexus/Prologue below.
bool decode_acurite606(uint64_t frame32, WeatherStation &out) {
    uint8_t b[4];
    for (int i = 0; i < 4; i++) b[i] = (uint8_t)((frame32 >> (8 * (3 - i))) & 0xFF);
    if (!b[0]) return false;
    uint8_t key = 241, sum = 0;
    for (int j = 0; j < 3; j++) {
        for (int n = 7; n >= 0; n--) {
            if ((b[j] >> n) & 1) sum ^= key;
            key = (key & 1) ? ((key >> 1) ^ 152) : (key >> 1);
        }
    }
    if (sum != b[3]) return false;
    int magnitude = ((b[1] & 7) << 8) | b[2];
    float temp = (b[1] & 8) ? -((float)((~magnitude + 1) & 2047)) / 10.0f : magnitude / 10.0f;
    if (temp < -50 || temp > 80) return false;
    out.model = "AcuRite-606TX";
    out.id = b[0];
    out.channel = 0;
    out.battery = (b[1] & 128) == 0;
    out.temp = temp;
    out.hasHumidity = false;
    out.humidity = 0;
    return true;
}

// Nexus: 36-bit PPM, '1111' sync nibble at bits[24:28); repeats ~12x on air,
// require >=3 identical frames (mirrors the JS findRepeated threshold) since
// there's no real checksum, only range-sanity checks.
bool decode_nexus(uint64_t frame36, WeatherStation &out) {
    if (bitslice(frame36, 36, 24, 28) != 0xF) return false;
    uint32_t flags = bitslice(frame36, 36, 8, 12);
    int channel = (flags & 3) + 1;
    float temp = signed12(bitslice(frame36, 36, 12, 24)) / 10.0f;
    uint32_t hum = bitslice(frame36, 36, 28, 36);
    if (channel > 3 || temp < -60 || temp > 80 || hum > 100) return false;
    out.model = (hum == 0) ? "Nexus-T" : "Nexus-TH";
    out.id = bitslice(frame36, 36, 0, 8);
    out.channel = channel;
    out.battery = (flags & 8) != 0;
    out.temp = temp;
    out.hasHumidity = (hum != 0);
    out.humidity = hum;
    return true;
}

// Prologue: 36-bit PPM, type nibble 9 or 5; repeats ~7x, require >=4
// identical frames (mirrors the JS findRepeated threshold).
bool decode_prologue(uint64_t frame36, WeatherStation &out) {
    uint32_t type = bitslice(frame36, 36, 0, 4);
    if (type != 9 && type != 5) return false;
    uint32_t id = bitslice(frame36, 36, 4, 12);
    uint32_t flags = bitslice(frame36, 36, 12, 16);
    float temp = signed12(bitslice(frame36, 36, 16, 28)) / 10.0f;
    uint32_t hum = bitslice(frame36, 36, 28, 36);
    if (temp < -60 || temp > 80 || (hum > 100 && hum != 204)) return false;
    out.model = "Prologue-TH";
    out.id = id;
    out.channel = (flags & 3) + 1;
    out.battery = (flags & 8) != 0;
    out.temp = temp;
    out.hasHumidity = (hum != 204);
    out.humidity = hum;
    return true;
}

struct RepeatEntry {
    int profile;
    uint64_t frame;
    int count;
    uint32_t lastMs;
};

int bump_repeat(std::vector<RepeatEntry> &repeats, int profile, uint64_t frame, uint32_t now) {
    for (auto &r : repeats) {
        if (r.profile == profile && r.frame == frame) {
            r.lastMs = now;
            return ++r.count;
        }
    }
    if (repeats.size() >= 16) repeats.erase(repeats.begin());
    repeats.push_back({profile, frame, 1, now});
    return 1;
}

void draw_weather_screen(const std::vector<WeatherStation> &stations, int rssi, int reads, bool listening) {
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(10, 26);
    tft.print("Weather Station");
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    tft.setCursor(10, 40);
    tft.print(g_freq, 2);
    tft.print(" MHz");

    // Live RSSI meter so you can see signal strength move in real time.
    int barMax = 120;
    int barW = max(0, min(barMax, (int)((rssi + 100) * (barMax / 70.0f))));
    tft.fillRect(10, 50, barMax + 2, 10, getColorVariation(bruceConfig.priColor));
    tft.fillRect(11, 51, barW, 8, bruceConfig.priColor);
    tft.setCursor(10 + barMax + 10, 50);
    tft.print(rssi);
    tft.print(" dBm");

    int y = 68;
    if (stations.empty()) {
        tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
        tft.setCursor(10, y);
        tft.print(listening ? "Listening..." : "No stations yet");
    } else {
        size_t visible = (size_t)max(1, (tftHeight - 56 - 24) / 34);
        for (size_t i = 0; i < stations.size() && i < visible; i++) {
            const auto &s = stations[i];
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setCursor(10, y);
            tft.print(s.model);
            tft.print(" #");
            tft.print(s.id);
            tft.print(" CH");
            tft.print(s.channel);
            tft.print(" x");
            tft.print(s.count);
            y += 15;
            tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
            tft.setCursor(16, y);
            tft.print(s.temp, 1);
            tft.print("C/");
            tft.print(s.temp * 9.0f / 5.0f + 32.0f, 1);
            tft.print("F  ");
            if (s.hasHumidity) {
                tft.print(s.humidity);
                tft.print("%RH  ");
            }
            tft.print(s.battery ? "BATT OK" : "BATT LOW");
            y += 19;
        }
    }
    tft.setTextColor(getColorVariation(bruceConfig.priColor), bruceConfig.bgColor);
    tft.setCursor(10, tftHeight - 16);
    tft.print("SEL settings  ESC exit  n=");
    tft.print(reads);
}

void settings_menu() {
    int option = 0;
    options = {
        {"Frequency: " + String(g_freq, 2) + " MHz", [&]() { option = 1; }},
        {"RX BW: " + String((int)g_bw) + " kHz",     [&]() { option = 2; }},
        {"AGC: " + String(agc_label(g_agc)),         [&]() { option = 3; }},
        {"Save captures: " + String(g_saveCaptures ? "ON" : "OFF"), [&]() { g_saveCaptures = !g_saveCaptures; }},
        {"Back",                                     [&]() { option = 0; }},
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
    }
}

} // namespace

void rf_weather_station() {
    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorder();

    // RfRxSession::begin() calls setup_rf_rx(), which itself calls
    // initRfModule("rx", bruceConfigPins.rfFreq) again internally -- an
    // initRfModule() call here, before begin(), would get silently re-run
    // (and its frequency/bandwidth/AGC along with it) using Bruce's
    // globally-configured frequency instead of g_freq. Set the global first
    // so that hidden init tunes to the right place, then apply our
    // bandwidth/AGC AFTER begin() so they're the last word.
    bruceConfigPins.rfFreq = g_freq;
    RfRxSession rx;
    if (!rx.begin()) {
        displayError("RX session failed", true);
        return;
    }
    ELECHOUSE_cc1101.setRxBW(g_bw);
    apply_agc(g_agc);

    std::vector<WeatherStation> stations;
    std::vector<RepeatEntry> repeats;
    int attempts = 0;
    int rssi = -127;
    uint32_t lastDraw = 0;

    draw_weather_screen(stations, rssi, attempts, true);

    while (!check(EscPress)) {
        if (check(SelPress)) {
            float prevFreq = g_freq, prevBw = g_bw;
            int prevAgc = g_agc;
            settings_menu();
            if (g_freq != prevFreq || g_bw != prevBw || g_agc != prevAgc) {
                rx.end();
                deinitRfModule();
                bruceConfigPins.rfFreq = g_freq;
                if (!rx.begin()) {
                    displayError("CC1101 re-init failed", true);
                    return;
                }
                ELECHOUSE_cc1101.setRxBW(g_bw);
                apply_agc(g_agc);
            }
            draw_weather_screen(stations, rssi, attempts, true);
        }

        if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) rssi = ELECHOUSE_cc1101.getRssi();

        std::vector<int> durations;
        bool updated = false;
        if (rx.poll(durations)) {
            attempts++;
            WeatherStation st;
            bool found = false;

            std::vector<uint64_t> acuFrames;
            ppm_extract_frames(durations, 1600, 2800, 3150, 4700, 7000, 32, acuFrames);
            for (uint64_t f : acuFrames) {
                if (decode_acurite606(f, st)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                uint32_t now = millis();
                for (size_t i = 0; i < repeats.size();) {
                    if (now - repeats[i].lastMs > 15000) repeats.erase(repeats.begin() + i);
                    else i++;
                }
                std::vector<uint64_t> nexFrames, proFrames;
                ppm_extract_frames(durations, 450, 1450, 1450, 2900, 3200, 36, nexFrames);
                ppm_extract_frames(durations, 1300, 2850, 2850, 6200, 7000, 36, proFrames);
                for (uint64_t f : nexFrames) {
                    if (bump_repeat(repeats, 0, f, now) >= 3 && decode_nexus(f, st)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    for (uint64_t f : proFrames) {
                        if (bump_repeat(repeats, 1, f, now) >= 4 && decode_prologue(f, st)) {
                            found = true;
                            break;
                        }
                    }
                }
            }

            if (found) {
                if (g_saveCaptures) save_capture(durations, g_freq, st.model);
                bool merged = false;
                for (auto &existing : stations) {
                    if (existing.model == st.model && existing.id == st.id && existing.channel == st.channel) {
                        st.count = existing.count + 1;
                        existing = st;
                        merged = true;
                        break;
                    }
                }
                if (!merged) {
                    st.count = 1;
                    stations.insert(stations.begin(), st);
                    if (stations.size() > 8) stations.pop_back();
                }
                updated = true;
            }
        }

        uint32_t nowMs = millis();
        if (updated || nowMs - lastDraw > 500) {
            draw_weather_screen(stations, rssi, attempts, true);
            lastDraw = nowMs;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    rx.end();
    deinitRfModule();
}
