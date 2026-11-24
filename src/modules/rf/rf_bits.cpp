#include "rf_bits.h"

#include "rf_compat.h"
#include "protocols/rf_encoder.h"
#include "rf_send.h"
#include "rf_utils.h"
#include <globals.h>
#include <vector>

namespace RfBits {

static bool s_loaded = false;
static String s_bits;
static int s_te = 350;
static int s_repeat = 4;
static float s_freq = 433.92f;

static String sanitize(const String &in) {
    String out;
    out.reserve(in.length());
    for (size_t i = 0; i < in.length(); i++) {
        const char c = in[i];
        if (c == '0' || c == '1') out += c;
    }
    return out;
}

bool load(const String &path, String &err) {
    RfCodes code;
    if (!readSubFile(&SD, path, code)) {
        err = "cannot read " + path;
        return false;
    }
    // Prefer an explicit bit string; otherwise expand the key to its bit count.
    String bits = sanitize(code.data);
    if (bits.length() == 0 && code.Bit > 0 && code.Bit <= 64) {
        for (int i = code.Bit - 1; i >= 0; i--) bits += (char)('0' + ((code.key >> i) & 1));
    }
    if (bits.length() == 0) {
        err = "no bits in " + path + " (RAW captures need 'rfbits set')";
        return false;
    }
    s_bits = bits;
    s_te = code.te > 0 ? code.te : 350;
    s_freq = code.frequency > 0 ? code.frequency / 1000000.0f : 433.92f;
    s_loaded = true;
    return true;
}

void set(const String &bits, int te, float freqMHz) {
    s_bits = sanitize(bits);
    if (te > 0) s_te = te;
    if (freqMHz > 0) s_freq = freqMHz;
    s_loaded = s_bits.length() > 0;
}

String show() {
    if (!s_loaded) return "rfbits: no capture loaded (rfbits load <file> | rfbits set <bits> <te> <MHz>)";
    return "rfbits " + String(s_bits.length()) + " bits te " + String(s_te) + " us repeat " + String(s_repeat) +
           " freq " + String(s_freq, 3) + " MHz\n" + s_bits;
}

bool flip(int index) {
    if (!s_loaded || index < 0 || index >= (int)s_bits.length()) return false;
    s_bits.setCharAt(index, s_bits[index] == '0' ? '1' : '0');
    return true;
}

bool trimLeading(int count) {
    if (!s_loaded || count <= 0 || count >= (int)s_bits.length()) return false;
    s_bits.remove(0, count);
    return true;
}

bool scale(int percent, bool grow) {
    if (!s_loaded || percent <= 0) return false;
    const int next = grow ? (s_te * (100 + percent) / 100) : (s_te * (100 - percent) / 100);
    s_te = constrain(next, 50, 5000);
    return true;
}

void setRepeat(int repeat) { s_repeat = constrain(repeat, 1, 40); }
void setTe(int te) { s_te = constrain(te, 50, 5000); }

static bool buildDurations(const String &bits, int te, std::vector<int> &d) {
    d.clear();
    d.reserve(bits.length() * 2);
    for (size_t i = 0; i < bits.length(); i++) {
        if (bits[i] == '1') {
            d.push_back(3 * te);
            d.push_back(-te);
        } else if (bits[i] == '0') {
            d.push_back(te);
            d.push_back(-3 * te);
        }
    }
    return !d.empty();
}

bool tx(String &err) {
    if (!s_loaded || s_bits.length() == 0) {
        err = "no bits loaded";
        return false;
    }
    if (rfRadioReceiving()) {
        err = "RF busy: a receive page owns the radio";
        return false;
    }
    std::vector<int> durations;
    if (!buildDurations(s_bits, s_te, durations)) {
        err = "nothing to send";
        return false;
    }
    if (!initRfModule("tx", s_freq)) {
        err = "radio init failed";
        return false;
    }
    for (int r = 0; r < s_repeat; r++) rf_tx_durations(durations);
    deinitRfModule();
    err = "sent " + String(s_bits.length()) + " bits x" + String(s_repeat) + " at " + String(s_freq, 3) + " MHz";
    return true;
}

// FKM / "granddaddy" recursion: every Lyndon word whose length divides n is
// emitted, which concatenates into B(2,n). Depth is n+1 (<= 17), so the caller's
// stack is safe.
static void dbRec(int t, int p, int n, std::vector<int> &a, String &seq) {
    if (t > n) {
        if (n % p == 0)
            for (int j = 1; j <= p; j++) seq += (char)('0' + a[j]);
        return;
    }
    a[t] = a[t - p];
    dbRec(t + 1, p, n, a, seq);
    if (a[t - p] == 0) {
        a[t] = 1;
        dbRec(t + 1, t, n, a, seq);
    }
}

String deBruijn(int n) {
    if (n < 1 || n > 16) return "";
    std::vector<int> a(n + 1, 0);
    String seq;
    seq.reserve((1 << n) + n);
    dbRec(1, 1, n, a, seq);
    return seq;
}

String force(const String &bits, int te, float freqMHz, int watchWindowMs) {
    if (bits.length() == 0) return "empty sequence";
    if (rfRadioReceiving()) return "RF busy: a receive page owns the radio";
    if (!initRfModule("tx", freqMHz)) return "radio init failed";

    const int noise = (int)tuneGetByName("rf.noise_floor", nullptr);
    const int baseline = rfGetRssi();
    const int chunk = 512;
    const int repeat = 3;
    int sent = 0;
    bool hit = false;
    uint32_t windowStart = millis();

    for (int start = 0; start < (int)bits.length() && !hit; start += chunk) {
        String part = bits.substring(start, min((int)bits.length(), start + chunk));
        std::vector<int> durations;
        if (!buildDurations(part, te, durations)) break;
        for (int r = 0; r < repeat; r++) rf_tx_durations(durations);
        sent += part.length();

        if ((int)(millis() - windowStart) >= watchWindowMs) {
            windowStart = millis();
            const int rssi = rfGetRssi();
            if (rssi > baseline + 6 && rssi > noise) hit = true;
        }
    }
    deinitRfModule();

    String out = "forced " + String(sent) + "/" + String(bits.length()) + " bits at " + String(freqMHz, 3) +
                 " MHz, rssi " + String(baseline) + " -> " + String(rfGetRssi());
    if (hit) out += " - RESPONSE DETECTED (possible receiver answered)";
    return out;
}

bool busy() { return false; }
bool loaded() { return s_loaded; }
const String &bits() { return s_bits; }
int te() { return s_te; }
int repeat() { return s_repeat; }

} // namespace RfBits
