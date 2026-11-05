/**
 * @file nrf_sniffer.cpp
 * @brief 2.4GHz ESB packet sniffer / recorder / replay for Bruce, modeled after
 *        the SubGHz "Record RAW -> Save -> Replay" flow in modules/rf/record.cpp,
 *        but for the nRF24L01+.
 *
 * Two capture modes, both dwell-scanning a configurable channel range:
 *
 *  - KNOWN  : listens with CRC enabled against a short list of addresses that
 *             a lot of cheap nRF24-based remotes never change (RF24 library
 *             defaults, common example-code addresses), plus any custom
 *             addresses you type in. Payload can be Dynamic (DPL) or a fixed
 *             length (32/16/8/4 bytes) — some cheap clone chips (Beken
 *             BK2423 and friends) don't implement DPL correctly, so if DPL
 *             finds nothing, try a fixed length. A hit here is a clean,
 *             CRC-validated frame -> reliable to save and replay 1:1.
 *  - PROMISC: the classic "Travis Goodspeed" trick already used by
 *             nrf_spectrum.cpp / nrf_mousejack.cpp (2-byte pseudo address,
 *             CRC disabled) to catch traffic regardless of its real address.
 *             This is diagnostic: the true address boundary is unknown, so a
 *             captured frame may be bit/byte-misaligned. Good for confirming
 *             *that* something is transmitting and eyeballing fixed vs rolling
 *             bytes; replay is best-effort only.
 *
 * Auto-sweep dwell-scans the whole configured channel range on its own, and
 * also cycles through all 3 datarates each time it wraps around, so a full
 * "place it down and wait" scan covers channel x datarate combinations
 * without touching a key. With "Full scan" additionally enabled, a complete
 * datarate lap also advances the payload mode (KNOWN only), for a true
 * walk-away scan across channel x datarate x payload combinations. "Stop on
 * hit" pauses auto-sweep automatically the moment something is captured, so
 * you don't need to babysit the screen.
 *
 * Custom addresses you add are persisted to
 * /BruceNRF24/custom_addrs.txt and reloaded automatically next time you open
 * the sniffer, so you don't have to retype them.
 *
 * Captures are buffered in RAM (for on-screen diffing) AND continuously
 * appended+flushed to a live log file on SD/LittleFS as they come in, so a
 * long unattended session survives a power loss. A manual "Save captures"
 * additionally dumps the current RAM buffer to its own file, and saved files
 * can be replayed back frame by frame.
 */
#include "nrf_sniffer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "nrf_common.h"
#include <array>
#include <globals.h>

namespace {

constexpr size_t MAX_CAPTURES = 200;
constexpr int DWELL_US = 700;              // per channel/address/datarate dwell while scanning
constexpr int CANDIDATE_MS = 900;          // time spent per candidate address in KNOWN mode
constexpr int AUTO_SWEEP_MS_DEFAULT = 400; // default dwell per channel while auto-sweeping

enum SniffMode : uint8_t { MODE_KNOWN = 0, MODE_PROMISC = 1 };

// PAYLOAD_DYNAMIC uses Nordic's Dynamic Payload Length feature; the others
// force a fixed size instead, for cheap clone chips that don't do DPL right.
enum PayloadMode : uint8_t {
    PAYLOAD_DYNAMIC = 0,
    PAYLOAD_FIXED32,
    PAYLOAD_FIXED16,
    PAYLOAD_FIXED8,
    PAYLOAD_FIXED4,
    PAYLOAD_MODE_COUNT
};
constexpr uint8_t payloadFixedLens[PAYLOAD_MODE_COUNT] = {0, 32, 16, 8, 4};
const char *payloadModeNames[PAYLOAD_MODE_COUNT] = {"DPL", "32B", "16B", "8B", "4B"};

// RF24 library / very common cheap-clone default addresses. Worth trying first
// because a lot of "no-name" nRF24 remotes (fan/lamp controllers included) ship
// with whatever address was in the vendor's example code.
const uint8_t knownAddr5[][5] = {
    {0xE7, 0xE7, 0xE7, 0xE7, 0xE7}, // RF24 lib default pipe0
    {0xC2, 0xC2, 0xC2, 0xC2, 0xC2}, // RF24 lib default pipe1
    {0x7E, 0x7E, 0x7E, 0x7E, 0x7E}, // seen on several cheap clones
    {0x01, 0x02, 0x03, 0x04, 0x05}, // classic Nordic example-code address
    {0x34, 0x43, 0x10, 0x10, 0x01}, // seen on some Chinese appliance remotes
};
constexpr size_t KNOWN_ADDR_COUNT = sizeof(knownAddr5) / sizeof(knownAddr5[0]);

// User-entered addresses, appended at runtime (Sel -> Add custom address).
// Kept separate from knownAddr5 since that one's a compile-time const array.
std::vector<std::array<uint8_t, 5>> customAddrs;
constexpr const char *CUSTOM_ADDR_FILE = "/BruceNRF24/custom_addrs.txt";

// Loads customAddrs from CUSTOM_ADDR_FILE (format: one "<width>,<hex>" per
// line). File is the single source of truth: called with an already-cleared
// customAddrs so re-opening the sniffer within the same power cycle doesn't
// duplicate entries.
void loadCustomAddresses() {
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) return;
    if (!fs->exists(CUSTOM_ADDR_FILE)) return;
    File f = fs->open(CUSTOM_ADDR_FILE);
    if (!f) return;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        int comma = line.indexOf(',');
        if (comma < 0) continue;
        int width = line.substring(0, comma).toInt();
        String hex = line.substring(comma + 1);
        hex.trim();
        if (width < 3 || width > 5 || (int)hex.length() != width * 2) continue;
        std::array<uint8_t, 5> addr{};
        bool ok = true;
        for (int i = 0; i < width; i++) {
            char b[3] = {hex[i * 2], hex[i * 2 + 1], 0};
            char *endp = nullptr;
            long v = strtol(b, &endp, 16);
            if (endp == b || *endp != '\0') {
                ok = false;
                break;
            }
            addr[i] = (uint8_t)v;
        }
        if (ok) customAddrs.push_back(addr);
    }
    f.close();
}

void appendCustomAddressToFile(uint8_t width, const String &hex) {
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) return;
    if (!fs->exists("/BruceNRF24")) {
        if (!fs->mkdir("/BruceNRF24")) return;
    }
    File f = fs->open(CUSTOM_ADDR_FILE, FILE_APPEND, true);
    if (!f) return;
    f.print(width);
    f.print(",");
    f.println(hex);
    f.close();
}

size_t totalAddrCount() { return KNOWN_ADDR_COUNT + customAddrs.size(); }

// Parses a "<width>*2 hex chars" string into a zero-padded 5-byte address.
// Shared by addCustomAddress() and parseLine()'s no-match fallback further
// down.
bool hexToAddr(const String &hex, uint8_t width, std::array<uint8_t, 5> &out) {
    if ((int)hex.length() != width * 2) return false;
    out = {}; // zero-padded; only the first 'width' bytes are ever read
    for (int i = 0; i < width; i++) {
        char b[3] = {hex[i * 2], hex[i * 2 + 1], 0};
        char *endp = nullptr;
        long v = strtol(b, &endp, 16);
        if (endp == b || *endp != '\0') return false;
        out[i] = (uint8_t)v;
    }
    return true;
}

const uint8_t *getKnownAddr(size_t idx) {
    if (idx < KNOWN_ADDR_COUNT) return knownAddr5[idx];
    size_t ci = idx - KNOWN_ADDR_COUNT;
    if (ci < customAddrs.size()) return customAddrs[ci].data();
    return knownAddr5[0]; // shouldn't happen if callers always mod by totalAddrCount()
}

// ── XN297/XN297L compatibility ────────────────────────────────────────
// XN297 and friends are register-compatible with the nRF24L01+ (same SPI
// command set) but differ on-air: address bytes go out bit-reversed
// compared to what a real nRF24 expects, and their CRC polynomial doesn't
// match the nRF24's, so the nRF24's hardware CRC check would silently drop
// every valid XN297 frame. XN297 mode bit-reverses the candidate address
// before writing it to the pipe and disables the nRF24's own CRC check so
// XN297 frames get through — at the cost of losing hardware CRC validation
// (more false positives are expected in this mode, same trade-off every
// XN297-bridging tool makes). Preamble length still differs too (XN297 uses
// a longer sync word); this covers the address/CRC half of the mismatch,
// which is the part that actually determines whether the nRF24 radio even
// latches onto the packet at all.
uint8_t reverseBits(uint8_t b) {
    b = (uint8_t)(((b & 0xF0) >> 4) | ((b & 0x0F) << 4));
    b = (uint8_t)(((b & 0xCC) >> 2) | ((b & 0x33) << 2));
    b = (uint8_t)(((b & 0xAA) >> 1) | ((b & 0x55) << 1));
    return b;
}

void reverseAddrBytes(const uint8_t *in, uint8_t width, uint8_t *out) {
    for (uint8_t i = 0; i < width && i < 5; i++) out[i] = reverseBits(in[i]);
}

// Promiscuous "noise" addresses: identical set already used in nrf_spectrum.cpp /
// nrf_mousejack.cpp. Pipes 2-5 only get a unique LSB on real nRF24 hardware, so
// they intentionally all share pipe1's upper byte (0xAA) — this is correct, not
// a typo.
const uint8_t noiseAddr2[][2] = {
    {0x55, 0x55},
    {0xAA, 0xAA},
    {0xA0, 0xAA},
    {0xAB, 0xAA},
    {0xAC, 0xAA},
    {0xAD, 0xAA}
};

const rf24_datarate_e dataRates[3] = {RF24_250KBPS, RF24_1MBPS, RF24_2MBPS};
const char *dataRateNames[3] = {"250K", "1M", "2M"};

struct Capture {
    uint8_t channel;
    uint8_t drIdx;
    uint8_t mode;        // SniffMode
    uint8_t addrIdx;     // candidate index (KNOWN, into combined known+custom list) or 0xFF (PROMISC)
    uint8_t payloadMode; // PayloadMode this frame was captured with (KNOWN only; irrelevant for PROMISC)
    uint8_t addrWidth;   // 3, 4, or 5 (KNOWN only; PROMISC is always fixed 2-byte pseudo-address)
    bool xn297;          // captured with XN297 bit-reversal + CRC-off compatibility mode
    uint8_t len;
    uint8_t data[32];
    unsigned long tMs;
};

struct SnifferState {
    uint8_t chMin = 0;
    uint8_t chMax = 84; // covers the practically-relevant ESB range (same span nrf_mousejack.cpp scans)
    uint8_t channel = 0;
    uint8_t drIdx = 1; // start at 1MBPS
    uint8_t mode = MODE_KNOWN;
    uint8_t payloadMode = PAYLOAD_DYNAMIC;
    uint8_t addrWidth = 5; // 3, 4, or 5
    bool xn297Mode = false;
    bool asciiMode = false; // false = hex dump, true = printable ASCII (non-printable shown as '.')
    uint8_t rfActivity = 0; // smoothed RPD-based activity level, 0-125 (same scale as nrf_spectrum.cpp)
    size_t candidateIdx = 0;
    uint32_t hits = 0;
    bool paused = false;
    bool autoSweep =
        true; // true = auto dwell-scan (channel + datarate); Next/Prev still work as a manual nudge
    bool fullScan = false;  // also cycle payload mode on each full datarate lap (KNOWN only)
    bool stopOnHit = false; // pause auto-sweep automatically the moment something is captured
    uint32_t dwellMs = AUTO_SWEEP_MS_DEFAULT; // time spent per channel while auto-sweeping, manually tunable
};

std::vector<Capture> captures;

// ── Live SD logging ──────────────────────────────────────────────────
File liveLogFile;
bool liveLogOpen = false;
String liveLogName;

String hexStr(const uint8_t *data, uint8_t len) {
    String s;
    s.reserve(len * 2);
    char buf[3];
    for (uint8_t i = 0; i < len; i++) {
        snprintf(buf, sizeof(buf), "%02X", data[i]);
        s += buf;
    }
    return s;
}

String addrStr(const uint8_t *addr, uint8_t width) { return hexStr(addr, width); }

String captureToLine(const Capture &c) {
    String addr = (c.mode == MODE_KNOWN) ? addrStr(getKnownAddr(c.addrIdx % totalAddrCount()), c.addrWidth)
                                         : addrStr(noiseAddr2[1], 2); // pipe1 pseudo-addr for reference
    char line[220];
    snprintf(
        line,
        sizeof(line),
        "CH=%02u DR=%s MODE=%s ADDR=%s AW=%u XN=%u LEN=%02u PLM=%s DATA=%s T=%lu",
        c.channel,
        dataRateNames[c.drIdx],
        c.mode == MODE_KNOWN ? "KNOWN" : "PROMISC",
        addr.c_str(),
        c.addrWidth,
        c.xn297 ? 1 : 0,
        c.len,
        payloadModeNames[c.payloadMode % PAYLOAD_MODE_COUNT],
        hexStr(c.data, c.len).c_str(),
        c.tMs
    );
    return String(line);
}

// ── Radio (re)configuration ──────────────────────────────────────────
void applyKnownConfig(SnifferState &st) {
    NRFradio.stopListening();
    NRFradio.setAutoAck(false);
    NRFradio.setRetries(0, 0);
    if (st.xn297Mode) {
        // XN297's CRC polynomial differs from the nRF24's; letting the
        // nRF24 hardware check CRC here would just drop every real XN297
        // frame, so we turn it off and accept the extra noise instead.
        NRFradio.disableCRC();
    } else {
        NRFradio.setCRCLength(RF24_CRC_16);
    }
    NRFradio.setAddressWidth(st.addrWidth);
    NRFradio.setDataRate(dataRates[st.drIdx]);
    if (st.payloadMode == PAYLOAD_DYNAMIC) {
        NRFradio.enableDynamicPayloads();
    } else {
        NRFradio.disableDynamicPayloads();
        NRFradio.setPayloadSize(payloadFixedLens[st.payloadMode]);
    }
    NRFradio.flush_rx();
    NRFradio.flush_tx();
    const uint8_t *rawAddr = getKnownAddr(st.candidateIdx % totalAddrCount());
    if (st.xn297Mode) {
        uint8_t revAddr[5];
        reverseAddrBytes(rawAddr, st.addrWidth, revAddr);
        NRFradio.openReadingPipe(0, revAddr);
    } else {
        NRFradio.openReadingPipe(0, rawAddr);
    }
    NRFradio.setChannel(st.channel);
    NRFradio.startListening();
}

void applyPromiscConfig(SnifferState &st) {
    NRFradio.stopListening();
    NRFradio.setAutoAck(false);
    NRFradio.setRetries(0, 0);
    NRFradio.disableCRC();
    NRFradio.setAddressWidth(2);
    NRFradio.setPayloadSize(32);
    NRFradio.setDataRate(dataRates[st.drIdx]);
    NRFradio.flush_rx();
    NRFradio.flush_tx();
    for (uint8_t i = 0; i < 6; i++) NRFradio.openReadingPipe(i, noiseAddr2[i]);
    NRFradio.setChannel(st.channel);
    NRFradio.startListening();
}

void applyConfig(SnifferState &st) {
    if (st.mode == MODE_KNOWN) applyKnownConfig(st);
    else applyPromiscConfig(st);
    st.rfActivity = 0; // any reconfiguration (channel, datarate, ...) invalidates the old reading
}

// ── Fixed vs rolling helper ──────────────────────────────────────────
// Compares a new capture against the most recent one with the same
// channel/datarate/mode/address, fills maskOut[0..len-1] with which byte
// positions differ, and returns the number of differing bytes (0 == looks
// like a fixed code so far, -1 == nothing to compare against yet).
int diffFromLast(const Capture &c, bool maskOut[32]) {
    for (int i = (int)captures.size() - 1; i >= 0; i--) {
        const Capture &prev = captures[i];
        if (prev.channel == c.channel && prev.drIdx == c.drIdx && prev.mode == c.mode &&
            prev.addrIdx == c.addrIdx && prev.addrWidth == c.addrWidth && prev.xn297 == c.xn297 &&
            prev.len == c.len) {
            int diff = 0;
            for (uint8_t b = 0; b < c.len; b++) {
                bool d = prev.data[b] != c.data[b];
                if (maskOut) maskOut[b] = d;
                if (d) diff++;
            }
            return diff;
        }
    }
    return -1;
}

// ── Screen ────────────────────────────────────────────────────────────
void drawHexLine(const uint8_t *data, uint8_t count, const bool *diffMask, int x, int y) {
    char buf[3];
    int cx = x;
    for (uint8_t i = 0; i < count; i++) {
        snprintf(buf, sizeof(buf), "%02X", data[i]);
        bool d = diffMask && diffMask[i];
        tft.setTextColor(d ? TFT_RED : bruceConfig.secColor, bruceConfig.bgColor);
        tft.drawString(buf, cx, y, 1);
        cx += LW * 2; // 2 hex chars per byte
    }
}

// Printable-ASCII rendering of a payload, for devices that send text/strings
// instead of pure binary (idea borrowed from Flipper Zero's nrf24tool RX
// mode, which offers the same hex/ASCII toggle). Non-printable bytes show as
// '.', same convention as a standard hex-editor "ASCII gutter".
void drawAsciiLine(const uint8_t *data, uint8_t count, const bool *diffMask, int x, int y) {
    char buf[2];
    int cx = x;
    for (uint8_t i = 0; i < count; i++) {
        buf[0] = (data[i] >= 0x20 && data[i] <= 0x7E) ? (char)data[i] : '.';
        buf[1] = 0;
        bool d = diffMask && diffMask[i];
        tft.setTextColor(d ? TFT_RED : bruceConfig.secColor, bruceConfig.bgColor);
        tft.drawString(buf, cx, y, 1);
        cx += LW; // 1 char per byte, half the width hex mode needs
    }
}

void drawScreen(SnifferState &st, const Capture *last, int lastDiff, const bool *diffMask, bool initial) {
    if (initial) drawMainBorderWithTitle("NRF24 SNIFFER");

    int y = 24;
    tft.setTextSize(FP);
    tft.fillRect(7, y, tftWidth - 14, 110, bruceConfig.bgColor);

    // Every line below is kept well under ~36 chars (the rough budget for a
    // 240px-wide screen at 6px/char, starting at x=10) so nothing runs off
    // the right edge — cramming everything onto one line (as an earlier
    // version did) pushed well past 45+ chars and got clipped invisibly.
    char line[40];

    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
    snprintf(
        line,
        sizeof(line),
        "CH:%d  Range:%d-%d  DW:%ums",
        st.channel,
        st.chMin,
        st.chMax,
        (unsigned)st.dwellMs
    );
    tft.drawString(line, 10, y, 1);
    y += 10;

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    snprintf(
        line,
        sizeof(line),
        "Mode:%s DR:%s",
        st.mode == MODE_KNOWN ? "KNOWN" : "PROMISC",
        dataRateNames[st.drIdx]
    );
    tft.drawString(line, 10, y, 1);
    y += 10;

    if (st.mode == MODE_KNOWN) {
        snprintf(
            line,
            sizeof(line),
            "PL:%s AW:%uB XN:%s",
            payloadModeNames[st.payloadMode],
            st.addrWidth,
            st.xn297Mode ? "ON" : "OFF"
        );
        tft.drawString(line, 10, y, 1);
        y += 10;
        snprintf(
            line,
            sizeof(line),
            "Addr:%s (%d/%d)",
            addrStr(getKnownAddr(st.candidateIdx % totalAddrCount()), st.addrWidth).c_str(),
            (int)(st.candidateIdx % totalAddrCount()) + 1,
            (int)totalAddrCount()
        );
        tft.drawString(line, 10, y, 1);
        y += 10;
    }

    // Only the flags that are actually on get a line at all, so the layout
    // stays short instead of always reserving space for every combination.
    String flags;
    if (st.autoSweep) flags += "AUTO ";
    if (st.fullScan) flags += "FULL ";
    if (st.stopOnHit) flags += "STOP ";
    if (liveLogOpen) flags += "REC ";
    if (flags.length() > 0) {
        tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
        snprintf(line, sizeof(line), "Flags: %s", flags.c_str());
        tft.drawString(line, 10, y, 1);
        y += 10;
    }

    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    snprintf(
        line,
        sizeof(line),
        "Hits:%lu Buf:%d/%d RF:%d%%",
        (unsigned long)st.hits,
        (int)captures.size(),
        (int)MAX_CAPTURES,
        (int)((st.rfActivity * 100) / 125)
    );
    tft.drawString(line, 10, y, 1);
    y += 12;

    if (last != nullptr) {
        tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
        // Show the actual capture's own channel/datarate/address, not the
        // live scan state above — auto-sweep and candidate rotation keep
        // moving after the hit, so by the time this redraws, CH/DR/Addr up
        // top may already point somewhere else entirely.
        if (last->mode == MODE_KNOWN) {
            snprintf(
                line,
                sizeof(line),
                "Last(CH%d %s %s):",
                last->channel,
                dataRateNames[last->drIdx],
                addrStr(getKnownAddr(last->addrIdx % totalAddrCount()), last->addrWidth).c_str()
            );
        } else {
            snprintf(line, sizeof(line), "Last(CH%d %s):", last->channel, dataRateNames[last->drIdx]);
        }
        tft.drawString(line, 10, y, 1);
        y += 10;

        if (st.asciiMode) {
            // All 32 possible payload bytes fit on one line at 1 char/byte
            // (max 32*LW=192px), so ASCII mode never needs a second line.
            drawAsciiLine(last->data, last->len, diffMask, 10, y);
            y += 10;
        } else {
            uint8_t line1Bytes = min((uint8_t)16, last->len);
            drawHexLine(last->data, line1Bytes, diffMask, 10, y);
            y += 10;
            if (last->len > 16) {
                drawHexLine(last->data + 16, last->len - 16, diffMask ? diffMask + 16 : nullptr, 10, y);
                y += 10;
            }
        }

        if (lastDiff == 0) tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        else tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
        if (lastDiff == 0) tft.drawString("== identical to prev", 10, y, 1);
        else if (lastDiff > 0) {
            snprintf(line, sizeof(line), "%d byte(s) differ", lastDiff);
            tft.drawString(line, 10, y, 1);
        }
    }

    int footerY = tftHeight - BORDER_PAD_X - FP * LH - 2;
    tft.fillRect(7, footerY, tftWidth - 14, FP * LH, bruceConfig.bgColor);
    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
    tft.drawCentreString("Nxt/Prv:ch  Sel:menu  Esc:stop", tftWidth / 2, footerY, 1);
}

// ── Live log (continuous, survives power loss) ───────────────────────
bool openLiveLog() {
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) return false;
    if (!fs->exists("/BruceNRF24")) {
        if (!fs->mkdir("/BruceNRF24")) return false;
    }
    char filename[40];
    int index = 0;
    do {
        snprintf(filename, sizeof(filename), "/BruceNRF24/live_%d.nrf24", index++);
    } while (fs->exists(filename));
    liveLogFile = fs->open(filename, FILE_WRITE, true);
    if (!liveLogFile) return false;
    liveLogFile.println("# Bruce NRF24 Sniffer Capture v2 (live log)");
    liveLogFile.flush();
    liveLogName = String(filename);
    liveLogOpen = true;
    return true;
}

void appendLiveLog(const Capture &c) {
    if (!liveLogOpen) return;
    liveLogFile.println(captureToLine(c));
    liveLogFile.flush();
}

void closeLiveLog() {
    if (liveLogOpen) {
        liveLogFile.close();
        liveLogOpen = false;
    }
}

// ── Save / Load ───────────────────────────────────────────────────────
bool saveCaptures() {
    if (captures.empty()) {
        displayError("Nothing to save", true);
        return false;
    }
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) {
        displayError("No space left on device", true);
        return false;
    }
    if (!fs->exists("/BruceNRF24")) {
        if (!fs->mkdir("/BruceNRF24")) {
            displayError("Error creating directory", true);
            return false;
        }
    }
    char filename[40];
    int index = 0;
    do {
        snprintf(filename, sizeof(filename), "/BruceNRF24/sniff_%d.nrf24", index++);
    } while (fs->exists(filename));

    File file = fs->open(filename, FILE_WRITE, true);
    if (!file) {
        displayError("Error creating file", true);
        return false;
    }
    file.println("# Bruce NRF24 Sniffer Capture v2");
    for (auto &c : captures) file.println(captureToLine(c));
    file.close();
    displaySuccess(filename, true);
    return true;
}

// Parses one saved line back into a Capture. Returns false on malformed lines.
bool parseLine(const String &line, Capture &out) {
    if (line.length() == 0 || line.startsWith("#")) return false;
    int ch = 0, len = 0, aw = 5, xn = 0;
    char drBuf[8] = {0};
    char modeBuf[10] = {0};
    char addrBuf[16] = {0};
    char plmBuf[6] = {0};
    char dataBuf[80] = {0};
    unsigned long t = 0;
    int matched = sscanf(
        line.c_str(),
        "CH=%d DR=%7s MODE=%9s ADDR=%15s AW=%d XN=%d LEN=%d PLM=%5s DATA=%79s T=%lu",
        &ch,
        drBuf,
        modeBuf,
        addrBuf,
        &aw,
        &xn,
        &len,
        plmBuf,
        dataBuf,
        &t
    );
    // Accept older files saved before AW=/XN= existed too: retry without them.
    if (matched < 9) {
        aw = 5;
        xn = 0;
        matched = sscanf(
            line.c_str(),
            "CH=%d DR=%7s MODE=%9s ADDR=%15s LEN=%d PLM=%5s DATA=%79s T=%lu",
            &ch,
            drBuf,
            modeBuf,
            addrBuf,
            &len,
            plmBuf,
            dataBuf,
            &t
        );
    }
    // Accept the very first sniffer format too, saved before PLM= existed
    // (no payload-mode field at all) — otherwise every line in a file that
    // old fails to parse and the whole capture is silently dropped, not just
    // its address.
    if (matched < 7) {
        strcpy(plmBuf, "DPL"); // that version only ever used dynamic payloads
        matched = sscanf(
            line.c_str(),
            "CH=%d DR=%7s MODE=%9s ADDR=%15s LEN=%d DATA=%79s T=%lu",
            &ch,
            drBuf,
            modeBuf,
            addrBuf,
            &len,
            dataBuf,
            &t
        );
        if (matched < 6) return false; // require at least up through DATA
    }

    out.channel = (uint8_t)ch;
    out.len = (uint8_t)min(len, 32);
    out.mode = (strcmp(modeBuf, "KNOWN") == 0) ? MODE_KNOWN : MODE_PROMISC;
    out.tMs = t;
    out.addrIdx = 0;
    out.payloadMode = PAYLOAD_DYNAMIC;
    out.addrWidth = (uint8_t)constrain(aw, 3, 5);
    out.xn297 = xn != 0;
    for (uint8_t i = 0; i < 3; i++) {
        if (strcmp(drBuf, dataRateNames[i]) == 0) {
            out.drIdx = i;
            break;
        }
    }
    for (uint8_t i = 0; i < PAYLOAD_MODE_COUNT; i++) {
        if (strcmp(plmBuf, payloadModeNames[i]) == 0) {
            out.payloadMode = i;
            break;
        }
    }
    // Resolve the saved address string to a candidate index for
    // display/replay. If it doesn't match anything currently known (e.g. a
    // custom address that predates this session, or one never saved to
    // custom_addrs.txt), append it as a new custom candidate instead of
    // silently defaulting to index 0 — replaying a saved KNOWN-mode capture
    // must send on the address it actually used, never a guess.
    if (out.mode == MODE_KNOWN) {
        bool addrMatched = false;
        for (size_t i = 0; i < totalAddrCount(); i++) {
            if (addrStr(getKnownAddr(i), out.addrWidth) == String(addrBuf)) {
                out.addrIdx = i;
                addrMatched = true;
                break;
            }
        }
        if (!addrMatched) {
            std::array<uint8_t, 5> addr;
            if (hexToAddr(String(addrBuf), out.addrWidth, addr)) {
                out.addrIdx = (uint8_t)totalAddrCount();
                customAddrs.push_back(addr);
            }
            // If even hexToAddr() fails, the file is malformed beyond what
            // we can recover — out.addrIdx stays at its 0 default, same as
            // before, rather than failing the whole load.
        }
    }
    for (uint8_t i = 0; i < out.len; i++) {
        char byteStr[3] = {dataBuf[i * 2], dataBuf[i * 2 + 1], 0};
        out.data[i] = (uint8_t)strtol(byteStr, nullptr, 16);
    }
    return true;
}

std::vector<Capture> loadCapturesFromFile(const String &path) {
    std::vector<Capture> out;
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) return out;
    File file = fs->open(path);
    if (!file) return out;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        Capture c;
        if (parseLine(line, c)) out.push_back(c);
    }
    file.close();
    return out;
}

// ── Replay ────────────────────────────────────────────────────────────
void replayCapture(const Capture &c) {
    NRFradio.stopListening();
    NRFradio.setAutoAck(false);
    NRFradio.setRetries(0, 0);
    NRFradio.setDataRate(dataRates[c.drIdx]);
    NRFradio.setChannel(c.channel);

    if (c.mode == MODE_KNOWN) {
        if (c.xn297) NRFradio.disableCRC();
        else NRFradio.setCRCLength(RF24_CRC_16);
        NRFradio.setAddressWidth(c.addrWidth);
        if (c.payloadMode == PAYLOAD_DYNAMIC) {
            NRFradio.enableDynamicPayloads();
        } else {
            NRFradio.disableDynamicPayloads();
            NRFradio.setPayloadSize(c.len);
        }
        const uint8_t *rawAddr = getKnownAddr(c.addrIdx % totalAddrCount());
        if (c.xn297) {
            uint8_t revAddr[5];
            reverseAddrBytes(rawAddr, c.addrWidth, revAddr);
            NRFradio.openWritingPipe(revAddr);
        } else {
            NRFradio.openWritingPipe(rawAddr);
        }
    } else {
        // Best-effort only: real address/frame alignment is unknown in
        // promiscuous captures, this resends exactly what was received right
        // after the pseudo-address match, which only lines up with the real
        // frame if that pseudo-address happened to sit on the true address
        // boundary.
        NRFradio.disableCRC();
        NRFradio.setAddressWidth(2);
        NRFradio.setPayloadSize(c.len);
        NRFradio.openWritingPipe(noiseAddr2[1]);
    }
    NRFradio.write(c.data, c.len);
}

// ── Byte fuzzing (URH-style "iterative range" fuzzing, replay side) ──
// Picks one loaded frame, iterates a chosen byte position through a value
// range, and retransmits the frame with that byte swapped each time — the
// same idea as Universal Radio Hacker's fuzzing component, and the same
// dwell/progress-display pattern Bruce's own Sub-GHz rf_bruteforce.cpp
// already uses for fixed-code garage door openers. Useful for guessing what
// a given byte in an otherwise-understood frame actually controls (e.g.
// "byte 2 looks like it might be the fan speed — let's find out").
// Core fuzz loop for a single already-chosen capture — shared by
// fuzzByteMenu() (picks a frame from a replay file first) and the History
// Browse screen (already has a frame in hand, no picking needed).
void fuzzSingleCapture(const Capture &tmpl) {
    if (tmpl.len == 0) {
        displayError("Empty frame", true);
        return;
    }

    String posS = keyboard(String(0), 2, "Byte position (0-" + String(tmpl.len - 1) + "):");
    int bytePos = constrain(posS.toInt(), 0, tmpl.len - 1);

    // One prompt instead of two separate start/end screens — "00-FF" style,
    // same shorthand you'd write on paper.
    String rangeS = keyboard("00-FF", 5, "Range (hex-hex):");
    int dash = rangeS.indexOf('-');
    int startVal = 0x00, endVal = 0xFF;
    if (dash > 0) {
        startVal = strtol(rangeS.substring(0, dash).c_str(), nullptr, 16);
        endVal = strtol(rangeS.substring(dash + 1).c_str(), nullptr, 16);
    }
    startVal = constrain(startVal, 0, 255);
    endVal = constrain(endVal, 0, 255);
    if (endVal < startVal) std::swap(startVal, endVal);

    String repS = keyboard(String(1), 1, "Repeats per value (1-5):");
    int repeats = constrain(repS.toInt(), 1, 5);

    String delayS = keyboard(String(50), 4, "Delay between sends (ms):");
    int delayMs = constrain(delayS.toInt(), 10, 5000);

    const int total = endVal - startVal + 1;
    drawMainBorderWithTitle("FUZZ BYTE");

    for (int val = startVal; val <= endVal; val++) {
        Capture c = tmpl;
        c.data[bytePos] = (uint8_t)val;
        for (int r = 0; r < repeats; r++) {
            replayCapture(c);
            delay(delayMs);
        }
        if (check(EscPress)) break;

        if ((val - startVal) % 4 == 0) {
            char buf[48];
            snprintf(
                buf,
                sizeof(buf),
                "Byte %d: %02X/%02X (%d/%d)",
                bytePos,
                val,
                endVal,
                val - startVal + 1,
                total
            );
            displayRedStripe(String(buf), getComplementaryColor2(bruceConfig.priColor), bruceConfig.priColor);
        }
    }
    displaySuccess("Fuzz done", true);
}

// Picks which loaded frame to fuzz (from a replay file, which may hold many
// frames), then hands off to fuzzSingleCapture() for the actual fuzzing.
void fuzzByteMenu(std::vector<Capture> &loaded) {
    if (loaded.empty()) return;

    // A tappable list is nice for a handful of frames, but a live-log
    // session can hold up to MAX_CAPTURES (200) — scrolling through 200
    // menu entries one at a time doesn't scale, so past a threshold we
    // switch to typing the index instead, with a peek at the last frame so
    // you're not picking blind.
    constexpr size_t FUZZ_PICKER_LIST_MAX = 15;
    size_t templateIdx = 0;
    if (loaded.size() > 1) {
        if (loaded.size() <= FUZZ_PICKER_LIST_MAX) {
            options.clear();
            int chosen = -1;
            for (size_t i = 0; i < loaded.size(); i++) {
                String label = String(i) + ": " + hexStr(loaded[i].data, min((uint8_t)6, loaded[i].len));
                options.push_back({label.c_str(), [&chosen, i]() { chosen = (int)i; }});
            }
            options.push_back({"Cancel", [&chosen]() { chosen = -2; }});
            loopOptions(options, MENU_TYPE_SUBMENU, "Fuzz which frame?");
            if (chosen < 0) return;
            templateIdx = (size_t)chosen;
        } else {
            String preview = hexStr(loaded.back().data, min((uint8_t)6, loaded.back().len));
            String idxS = keyboard(
                String((int)loaded.size() - 1),
                4,
                "Frame idx (0-" + String((int)loaded.size() - 1) + ", last=" + preview + "):"
            );
            templateIdx = (size_t)constrain(idxS.toInt(), 0, (int)loaded.size() - 1);
        }
    }

    fuzzSingleCapture(loaded[templateIdx]);
}

// Deletes every .nrf24 file under /BruceNRF24/ — the live-log and manually
// saved capture files pile up one per session, and there's no "delete
// folder" in Bruce's file manager, only one-file-at-a-time. Deliberately
// leaves custom_addrs.txt alone (that's a saved preference, not a log), and
// skips the CURRENTLY open live log — deleting a file out from under a still
// -open handle mid-session is asking for filesystem trouble, and it'd also
// wipe the very session you're in the middle of recording.
void clearAllLogs() {
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) {
        displayError("No storage found", true);
        return;
    }
    File dir = fs->open("/BruceNRF24");
    if (!dir || !dir.isDirectory()) {
        displayError("No logs to clear", true);
        return;
    }
    std::vector<String> toDelete;
    File entry = dir.openNextFile();
    while (entry) {
        String name = String(entry.name());
        bool isDir = entry.isDirectory();
        entry.close();
        String fullPath = name.startsWith("/") ? name : String("/BruceNRF24/") + name;
        bool isCurrentLiveLog = liveLogOpen && fullPath == liveLogName;
        if (!isDir && name.endsWith(".nrf24") && !isCurrentLiveLog) toDelete.push_back(fullPath);
        entry = dir.openNextFile();
    }
    dir.close();

    if (toDelete.empty()) {
        displayError("No logs to clear", true);
        return;
    }

    int deleted = 0;
    for (auto &path : toDelete) {
        if (fs->remove(path)) deleted++;
    }
    displaySuccess(String(deleted) + " log file(s) deleted", true);
}

void replayMenu() {
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) {
        displayError("No storage found", true);
        return;
    }
    File dir = fs->open("/BruceNRF24");
    if (!dir || !dir.isDirectory()) {
        displayError("No captures saved yet", true);
        return;
    }
    std::vector<String> files;
    File entry = dir.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) files.push_back(String(entry.name()));
        entry = dir.openNextFile();
    }
    dir.close();
    if (files.empty()) {
        displayError("No captures saved yet", true);
        return;
    }

    options.clear();
    int chosen = -1;
    for (size_t i = 0; i < files.size(); i++) {
        String f = files[i];
        options.push_back({f.c_str(), [&chosen, i]() { chosen = (int)i; }});
    }
    options.push_back({"Back", [&chosen]() { chosen = -2; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Replay file");
    if (chosen < 0) return;

    String path = files[chosen];
    if (!path.startsWith("/")) path = "/BruceNRF24/" + path;
    std::vector<Capture> loaded = loadCapturesFromFile(path);
    if (loaded.empty()) {
        displayError("Could not parse file", true);
        return;
    }

    drawMainBorderWithTitle("REPLAY");
    tft.setTextSize(FP);
    tft.setCursor(10, 30);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.println(String(loaded.size()) + " frame(s) loaded.");
    tft.println("[SEL] send once");
    tft.println("[NEXT] send x5 (100ms)");
    tft.println("[PREV] fuzz a byte");
    tft.println("[ESC] back");

    if (!nrf_start(NRF_MODE_SPI)) {
        displayError("NRF24 not found", true);
        return;
    }

    while (!check(EscPress)) {
        if (check(SelPress)) {
            for (auto &c : loaded) replayCapture(c);
            displaySuccess("Sent", false);
        }
        if (check(NextPress)) {
            for (int i = 0; i < 5; i++) {
                for (auto &c : loaded) replayCapture(c);
                delay(100);
            }
            displaySuccess("Sent x5", false);
        }
        if (check(PrevPress)) {
            fuzzByteMenu(loaded);
            drawMainBorderWithTitle("REPLAY");
            tft.setTextSize(FP);
            tft.setCursor(10, 30);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.println(String(loaded.size()) + " frame(s) loaded.");
            tft.println("[SEL] send once");
            tft.println("[NEXT] send x5 (100ms)");
            tft.println("[PREV] fuzz a byte");
            tft.println("[ESC] back");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    NRFradio.powerDown();
}

// ── Custom address entry ──────────────────────────────────────────────
void addCustomAddress(uint8_t width) {
    int hexLen = width * 2;
    String prompt = String(width) + "-byte addr (" + String(hexLen) + " hex chars):";
    String hex = hex_keyboard("", hexLen, prompt);
    hex.trim();
    hex.toUpperCase();
    std::array<uint8_t, 5> addr;
    if (!hexToAddr(hex, width, addr)) {
        displayError("Invalid hex", true);
        return;
    }
    customAddrs.push_back(addr);
    appendCustomAddressToFile(width, hex);
    displaySuccess("Address added", true);
}

// ── Action menu (Sel while scanning) ─────────────────────────────────
// Returns true if the caller should stop scanning and return to the NRF24 menu.
// ── "Scan Settings" submenu ───────────────────────────────────────────
// ── "Scan Settings" → "Radio" submenu ─────────────────────────────────
void radioSettingsMenu(SnifferState &st) {
    while (true) {
        int action = 0;
        String modeLabel = st.mode == MODE_KNOWN ? "Mode: KNOWN" : "Mode: PROMISC";
        String rangeLabel = String("Range: ") + st.chMin + "-" + st.chMax;
        String drLabel = String("Datarate: ") + dataRateNames[st.drIdx];
        String plLabel = String("Payload: ") + payloadModeNames[st.payloadMode];
        String awLabel = String("Addr width: ") + String((int)st.addrWidth) + "B";
        String xnLabel = st.xn297Mode ? "XN297 mode: ON" : "XN297 mode: OFF";

        options = {
            {modeLabel.c_str(),  [&]() { action = 1; }},
            {rangeLabel.c_str(), [&]() { action = 2; }},
            {drLabel.c_str(),    [&]() { action = 3; }},
            {plLabel.c_str(),    [&]() { action = 4; }},
            {awLabel.c_str(),    [&]() { action = 5; }},
            {xnLabel.c_str(),    [&]() { action = 6; }},
            {"Back",             [&]() { action = 7; }},
        };
        loopOptions(options, MENU_TYPE_SUBMENU, "Radio");

        switch (action) {
            case 1: st.mode = (st.mode == MODE_KNOWN) ? MODE_PROMISC : MODE_KNOWN; break;
            case 2: {
                String minS = keyboard(String(st.chMin), 3, "Min channel (0-125):");
                String maxS = keyboard(String(st.chMax), 3, "Max channel (0-125):");
                int mn = minS.toInt(), mx = maxS.toInt();
                if (mx < mn) std::swap(mn, mx);
                st.chMin = (uint8_t)constrain(mn, 0, 125);
                st.chMax = (uint8_t)constrain(mx, 0, 125);
                st.channel = st.chMin;
                break;
            }
            case 3: st.drIdx = (st.drIdx + 1) % 3; break;
            case 4: st.payloadMode = (st.payloadMode + 1) % PAYLOAD_MODE_COUNT; break;
            case 5: st.addrWidth = (st.addrWidth >= 5) ? 3 : st.addrWidth + 1; break;
            case 6: st.xn297Mode = !st.xn297Mode; break;
            case 7: return;
        }
    }
}

// ── "Scan Settings" → "Sweep" submenu ─────────────────────────────────
void sweepSettingsMenu(SnifferState &st) {
    while (true) {
        int action = 0;
        String sweepLabel = st.autoSweep ? "Auto-sweep: ON" : "Auto-sweep: OFF";
        String dwellLabel = String("Dwell time: ") + (int)st.dwellMs + "ms";
        String fullLabel = st.fullScan ? "Full scan: ON" : "Full scan: OFF";
        String stopLabel = st.stopOnHit ? "Stop on hit: ON" : "Stop on hit: OFF";

        options = {
            {sweepLabel.c_str(), [&]() { action = 1; }},
            {dwellLabel.c_str(), [&]() { action = 2; }},
            {fullLabel.c_str(),  [&]() { action = 3; }},
            {stopLabel.c_str(),  [&]() { action = 4; }},
            {"Back",             [&]() { action = 5; }},
        };
        loopOptions(options, MENU_TYPE_SUBMENU, "Sweep");

        switch (action) {
            case 1: st.autoSweep = !st.autoSweep; break;
            case 2: {
                String msS = keyboard(String((int)st.dwellMs), 5, "Dwell per channel (ms):");
                int ms = msS.toInt();
                st.dwellMs = (uint32_t)constrain(ms, 20, 10000); // below ~20ms there isn't enough time to
                                                                 // actually receive a packet at each stop
                break;
            }
            case 3: st.fullScan = !st.fullScan; break;
            case 4: st.stopOnHit = !st.stopOnHit; break;
            case 5: return;
        }
    }
}

// ── "Scan Settings" → "View" submenu ──────────────────────────────────
void viewSettingsMenu(SnifferState &st) {
    while (true) {
        int action = 0;
        String displayLabel = st.asciiMode ? "Display: ASCII" : "Display: HEX";
        String addrLabel = String("Add addr (") + String((int)customAddrs.size()) + ")";

        options = {
            {displayLabel.c_str(), [&]() { action = 1; }},
            {addrLabel.c_str(),    [&]() { action = 2; }},
            {"Back",               [&]() { action = 3; }},
        };
        loopOptions(options, MENU_TYPE_SUBMENU, "View");

        switch (action) {
            case 1: st.asciiMode = !st.asciiMode; break;
            case 2: addCustomAddress(st.addrWidth); break;
            case 3: return;
        }
    }
}

// ── Top-level "Scan Settings" dispatcher ──────────────────────────────
// 13 flat items got unwieldy to scroll through on a screen this small, so
// they're grouped by how often/together you'd actually touch them: Radio
// (protocol-level, set-and-mostly-forget), Sweep (scanning behavior),
// View (display preference + address list).
void scanSettingsMenu(SnifferState &st) {
    while (true) {
        int action = 0;
        options = {
            {"Radio", [&]() { action = 1; }},
            {"Sweep", [&]() { action = 2; }},
            {"View",  [&]() { action = 3; }},
            {"Back",  [&]() { action = 4; }},
        };
        loopOptions(options, MENU_TYPE_SUBMENU, "Scan Settings");

        switch (action) {
            case 1: radioSettingsMenu(st); break;
            case 2: sweepSettingsMenu(st); break;
            case 3: viewSettingsMenu(st); break;
            case 4: return;
        }
    }
}

// ── History Browse ─────────────────────────────────────────────────────
// A dedicated, explicitly-entered screen for paging through the RAM buffer
// (up to MAX_CAPTURES entries) — deliberately NOT reusing Next/Prev's
// "change channel" meaning from the live scan screen, since overloading the
// same two keys with two different meanings depending on some invisible
// mode would be confusing on a device with no second display to show which
// mode you're in. In here, Next/Prev step through history, Sel jumps
// straight into fuzzing on whichever capture you're looking at (skipping
// the save → replay → pick-file dance entirely), Esc returns to the live
// scan exactly where it left off.
void browseHistoryMenu(SnifferState &st) {
    if (captures.empty()) {
        displayError("No captures yet", true);
        return;
    }

    size_t idx = captures.size() - 1; // start at the most recent
    bool redraw = true;

    while (true) {
        if (redraw) {
            const Capture &c = captures[idx];
            drawMainBorderWithTitle("HISTORY");
            tft.setTextSize(FP);
            int y = 24;
            char line[48];

            tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
            snprintf(line, sizeof(line), "Capture %d/%d", (int)idx + 1, (int)captures.size());
            tft.drawString(line, 10, y, 1);
            y += 12;

            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            if (c.mode == MODE_KNOWN) {
                snprintf(
                    line,
                    sizeof(line),
                    "CH%d %s %s",
                    c.channel,
                    dataRateNames[c.drIdx],
                    addrStr(getKnownAddr(c.addrIdx % totalAddrCount()), c.addrWidth).c_str()
                );
            } else {
                snprintf(line, sizeof(line), "CH%d %s PROMISC", c.channel, dataRateNames[c.drIdx]);
            }
            tft.drawString(line, 10, y, 1);
            y += 12;

            // Diff against the previous entry in the buffer (by position, not
            // by matching config like the live view's diff does) — good
            // enough for "did this change since the one right before it"
            // while casually paging through.
            bool diffMask[32] = {false};
            int diffCount = -1;
            if (idx > 0 && captures[idx - 1].len == c.len) {
                diffCount = 0;
                for (uint8_t b = 0; b < c.len; b++) {
                    bool d = captures[idx - 1].data[b] != c.data[b];
                    diffMask[b] = d;
                    if (d) diffCount++;
                }
            }

            if (st.asciiMode) {
                drawAsciiLine(c.data, c.len, diffMask, 10, y);
                y += 10;
            } else {
                uint8_t l1 = min((uint8_t)16, c.len);
                drawHexLine(c.data, l1, diffMask, 10, y);
                y += 10;
                if (c.len > 16) {
                    drawHexLine(c.data + 16, c.len - 16, diffMask + 16, 10, y);
                    y += 10;
                }
            }

            if (diffCount == 0) tft.setTextColor(TFT_RED, bruceConfig.bgColor);
            else tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            if (diffCount == 0) tft.drawString("== identical to prev entry", 10, y, 1);
            else if (diffCount > 0) {
                snprintf(line, sizeof(line), "%d byte(s) differ vs prev", diffCount);
                tft.drawString(line, 10, y, 1);
            }
            y += 12;

            tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
            tft.drawString("Nxt/Prv:step Sel:fuzz Esc:back", 10, y, 1);
            redraw = false;
        }

        if (check(EscPress)) return;
        if (check(NextPress) && idx + 1 < captures.size()) {
            idx++;
            redraw = true;
        }
        if (check(PrevPress) && idx > 0) {
            idx--;
            redraw = true;
        }
        if (check(SelPress)) {
            fuzzSingleCapture(captures[idx]);
            redraw = true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ── "Session" submenu ──────────────────────────────────────────────────
// lastCap/lastDiff/lastDiffMask are owned by nrf_sniffer()'s main loop and
// threaded down here by reference so "Clear buffer" can reset them alongside
// the vector it points into — otherwise lastCap would dangle at the next
// screen redraw (use-after-free: the Capture it pointed to no longer exists
// once captures.clear() runs its destructors).
void sessionMenu(SnifferState &st, const Capture *&lastCap, int &lastDiff, bool *lastDiffMask) {
    while (true) {
        int action = 0;
        options = {
            {"Save captures",  [&]() { action = 1; }},
            {"Browse History", [&]() { action = 2; }},
            {"Clear buffer",   [&]() { action = 3; }},
            {"Clear all logs", [&]() { action = 4; }},
            {"Back",           [&]() { action = 5; }},
        };
        loopOptions(options, MENU_TYPE_SUBMENU, "Session");

        if (action == 1) saveCaptures();
        else if (action == 2) browseHistoryMenu(st);
        else if (action == 3) {
            captures.clear();
            st.hits = 0;
            lastCap = nullptr;
            lastDiff = -1;
            memset(lastDiffMask, 0, 32 * sizeof(bool));
        } else if (action == 4) clearAllLogs();
        else break;
    }
}

// ── Top-level action menu (Sel while scanning) ────────────────────────
// Returns true if the caller should stop scanning and return to the NRF24 menu.
bool actionMenu(SnifferState &st, const Capture *&lastCap, int &lastDiff, bool *lastDiffMask) {
    int action = 0;
    options = {
        {"Resume",        [&]() { action = 0; }},
        {"Scan Settings", [&]() { action = 1; }},
        {"Session",       [&]() { action = 2; }},
        {"Exit",          [&]() { action = 3; }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "NRF Sniffer");

    switch (action) {
        case 1: scanSettingsMenu(st); break;
        case 2: sessionMenu(st, lastCap, lastDiff, lastDiffMask); break;
        case 3: return true;
    }
    return false;
}

} // namespace

void nrf_sniffer() {
    if (!nrf_start(NRF_MODE_SPI)) {
        displayError("NRF24 not found", true);
        return;
    }

    captures.clear();
    customAddrs.clear();
    loadCustomAddresses(); // file is the source of truth; safe to call every time we open the sniffer
    SnifferState st;
    applyConfig(st);
    openLiveLog(); // best-effort; if it fails (no SD), we just keep the RAM buffer

    bool lastDiffMask[32] = {false};
    const Capture *lastCap = nullptr;
    int lastDiff = -1;
    drawScreen(st, lastCap, lastDiff, lastDiffMask, true);

    unsigned long lastCandidateSwitch = millis();
    unsigned long lastAutoAdvance = millis();
    unsigned long lastDraw = 0;

    while (true) {
        if (check(EscPress)) break;
        // Next/Prev always work as a manual override/nudge, even while
        // auto-sweeping — handy to jump straight to a channel you already
        // know is interesting without waiting for the sweep to get there.
        if (check(NextPress)) {
            st.channel = (st.channel >= st.chMax) ? st.chMin : st.channel + 1;
            applyConfig(st);
            lastAutoAdvance = millis();
        }
        if (check(PrevPress)) {
            st.channel = (st.channel <= st.chMin) ? st.chMax : st.channel - 1;
            applyConfig(st);
            lastAutoAdvance = millis();
        }
        if (check(SelPress)) {
            NRFradio.stopListening();
            if (actionMenu(st, lastCap, lastDiff, lastDiffMask)) break;
            applyConfig(st);
            drawScreen(st, lastCap, lastDiff, lastDiffMask, true);
            lastAutoAdvance = millis();
        }

        // Auto-sweep: dwell-scan the whole [chMin, chMax] range on its own,
        // same idea as nrf_mousejack.cpp's channel sweep — and each time it
        // wraps back to chMin, also cycle to the next datarate, so a full
        // "leave it running" session eventually covers every channel x
        // datarate combination. With "Full scan" also on, a complete datarate
        // lap (KNOWN only) additionally advances the payload mode, covering
        // channel x datarate x payload for a true walk-away scan. Hits are
        // still captured and shown as it goes; whether it stops on a hit is
        // controlled separately by "Stop on hit" below.
        if (st.autoSweep && millis() - lastAutoAdvance > st.dwellMs) {
            bool chWrapped = st.channel >= st.chMax;
            st.channel = chWrapped ? st.chMin : st.channel + 1;
            if (chWrapped) {
                bool drWrapped = st.drIdx >= 2;
                st.drIdx = (st.drIdx + 1) % 3;
                if (drWrapped && st.fullScan && st.mode == MODE_KNOWN) {
                    st.payloadMode = (st.payloadMode + 1) % PAYLOAD_MODE_COUNT;
                }
            }
            applyConfig(st);
            lastAutoAdvance = millis();
        }

        // In KNOWN mode, cycle through candidate addresses (built-in +
        // custom) periodically so we don't sit forever on one guess.
        if (st.mode == MODE_KNOWN && millis() - lastCandidateSwitch > CANDIDATE_MS) {
            st.candidateIdx = (st.candidateIdx + 1) % totalAddrCount();
            applyConfig(st);
            lastCandidateSwitch = millis();
        }

        delayMicroseconds(DWELL_US);

        // Same smoothing formula nrf_spectrum.cpp uses for its bar chart —
        // gives a rough "is there RF energy here at all" reading even when
        // no valid packet decodes, which matters a lot while you're still
        // hunting for the right channel rather than reading known traffic.
        st.rfActivity = (uint8_t)((st.rfActivity * 3 + (NRFradio.testRPD() ? 1 : 0) * 125) / 4);

        if (NRFradio.available()) {
            Capture c{};
            c.channel = st.channel;
            c.drIdx = st.drIdx;
            c.mode = st.mode;
            c.addrIdx = (uint8_t)st.candidateIdx;
            c.payloadMode = st.payloadMode;
            c.addrWidth = st.addrWidth;
            c.xn297 = st.xn297Mode;
            c.tMs = millis();
            if (st.mode == MODE_KNOWN) {
                if (st.payloadMode == PAYLOAD_DYNAMIC) {
                    c.len = NRFradio.getDynamicPayloadSize();
                    if (c.len == 0 || c.len > 32) c.len = 32;
                } else {
                    c.len = payloadFixedLens[st.payloadMode];
                }
            } else {
                c.len = 32;
            }
            NRFradio.read(c.data, c.len);

            if (captures.size() >= MAX_CAPTURES) captures.erase(captures.begin());
            memset(lastDiffMask, 0, sizeof(lastDiffMask));
            lastDiff = diffFromLast(c, lastDiffMask);
            captures.push_back(c);
            lastCap = &captures.back();
            appendLiveLog(c);
            st.hits++;
            if (st.stopOnHit) st.autoSweep = false; // freeze the sweep right where the hit happened
        }

        if (millis() - lastDraw > 150) {
            drawScreen(st, lastCap, lastDiff, lastDiffMask, false);
            lastDraw = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    NRFradio.stopListening();
    NRFradio.powerDown();
    closeLiveLog();

    if (!captures.empty()) {
        options.clear();
        int opt = 0;
        options = {
            {"Save",           [&]() { opt = 1; }},
            {"Replay saved..", [&]() { opt = 2; }},
            {"Discard",        [&]() { opt = 3; }},
        };
        loopOptions(options, MENU_TYPE_SUBMENU, "NRF Sniffer");
        if (opt == 1) saveCaptures();
        else if (opt == 2) replayMenu();
    }
    captures.clear();
}
