// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). This file contains code DERIVED FROM and
// modified after:
//   - rc-switch (LGPL-2.1-or-later), (C) 2011 Suat Ozgur and contributors —
//     the classic OOK protocol capture/decode state machine;
//   - Flipper Zero firmware (GPL-3.0-or-later), (C) Flipper Devices Inc. and
//     contributors — the KeeLoq frame decoder framing.
// See THIRD_PARTY.md for full attribution.
#include "rf_codec.h"
#include "rf_config.h"
#include "rf_registry.h"
#include "rf_registry_ext.h"
#include <cstdlib>

#define RF_SEPARATION_LIMIT 4300
#define RF_RECEIVE_TOLERANCE 60
#define RF_NOMINAL_TE_TOLERANCE 20
#define RF_MAX_CHANGES 131

static inline unsigned int rf_udiff(int a, int b) { return (unsigned int)abs(a - b); }
static inline unsigned int rf_udiff_u(unsigned int a, unsigned int b) { return (a > b) ? (a - b) : (b - a); }

static bool rf_match_protocol(
    const RfProtocolDef *pro, unsigned int changeCount, const unsigned int *timings, RfCodes &out
) {
    if (pro == nullptr) return false;
    uint64_t code = 0;
    // The longer sync factor maps to the captured inter-frame gap in timings[0].
    unsigned int syncLen = (pro->sync.low > pro->sync.high) ? pro->sync.low : pro->sync.high;
    if (syncLen == 0) return false;
    unsigned int delay = timings[0] / syncLen;
    if (delay == 0) return false;
    unsigned int teTol = pro->te * RF_NOMINAL_TE_TOLERANCE / 100;
    if (teTol < 80) teTol = 80;
    if (rf_udiff_u(delay, pro->te) > teTol) return false;

    unsigned int tol = delay * RF_RECEIVE_TOLERANCE / 100;
    // Protocols that start high have their first data timing filtered out.
    unsigned int first = pro->inverted ? 2 : 1;

    for (unsigned int i = first; i + 1 < changeCount; i += 2) {
        code <<= 1;
        if (rf_udiff(timings[i], delay * pro->zero.high) < tol &&
            rf_udiff(timings[i + 1], delay * pro->zero.low) < tol) {
            // zero bit
        } else if (rf_udiff(timings[i], delay * pro->one.high) < tol &&
                   rf_udiff(timings[i + 1], delay * pro->one.low) < tol) {
            code |= 1; // one bit
        } else {
            return false;
        }
    }

    if (changeCount > 7) { // ignore very short bursts: that would be noise
        int nbits = (changeCount - 1) / 2;
        // Fixed-length protocols only match a frame of exactly their length. This
        // keeps a 12-bit CAME frame from being claimed by a longer/shorter code
        // with otherwise-compatible timings (the M3 cross-match ambiguity).
        if ((pro->flags & RF_PF_FIXED_LEN) && nbits != pro->bits) return false;
        out.key = code;
        out.Bit = nbits;
        out.te = delay;
        out.protocol = pro->name;
        return true;
    }
    return false;
}

static bool rf_decode_chamberlain_9bit(const std::vector<int> &durations, RfCodes &out) {
    const unsigned int te = 430;
    const unsigned int dataTol = 260;
    const unsigned int stopTol = 700;

    for (size_t stop = 18; stop + 1 < durations.size(); stop++) {
        unsigned int stop0 = (unsigned int)abs(durations[stop]);
        unsigned int stop1 = (unsigned int)abs(durations[stop + 1]);
        if (rf_udiff_u(stop0, 3000) > stopTol || rf_udiff_u(stop1, 1000) > stopTol) continue;

        uint64_t code = 0;
        bool ok = true;
        size_t start = stop - 18;
        for (size_t i = start; i < stop; i += 2) {
            unsigned int a = (unsigned int)abs(durations[i]);
            unsigned int b = (unsigned int)abs(durations[i + 1]);
            code <<= 1;

            if (rf_udiff_u(a, te * 2) <= dataTol && rf_udiff_u(b, te) <= dataTol) {
                // zero bit: long then short
            } else if (rf_udiff_u(a, te) <= dataTol && rf_udiff_u(b, te * 2) <= dataTol) {
                code |= 1;
            } else {
                ok = false;
                break;
            }
        }
        if (!ok) continue;

        out.key = code;
        out.Bit = 9;
        out.te = te;
        out.protocol = "Chamberlain_9bit";
        out.preset = "Ook270Async";
        RF_DBG("decode MATCH proto=Chamberlain_9bit key=%llX bits=9 te=%u", (unsigned long long)code, te);
        return true;
    }
    return false;
}

bool rf_decode_ook(const std::vector<int> &durations, RfCodes &out, bool skipStream) {
    // Try callback-based extended decoders first. Each walks the full
    // duration vector independently looking for its own preamble pattern.
    const int extCount = rf_protocol_ext_count();
    for (int p = 0; p < extCount; p++) {
        const RfProtocolDef *ext = rf_protocol_ext_at(p);
        if (skipStream && ext->stream) continue;
        if (ext->decode && ext->decode(durations, out)) { return true; }
    }

    if (rf_decode_chamberlain_9bit(durations, out)) return true;

    unsigned int timings[RF_MAX_CHANGES];
    unsigned int changeCount = 0;
    unsigned int repeatCount = 0;
    const int protoCount = rf_protocol_count();

    for (int d : durations) {
        unsigned int dur = (d < 0) ? (unsigned int)(-d) : (unsigned int)d;

        if (dur > RF_SEPARATION_LIMIT) {
            // A long stretch without a level change: likely the gap between two
            // repeated transmissions. Two similar gaps bracket a full frame.
            if (repeatCount == 0 || rf_udiff(dur, timings[0]) < 200) {
                repeatCount++;
                if (repeatCount == 2) {
                    RF_DBG("decode attempt: changeCount=%u gap=%u", changeCount, dur);
                    for (int p = 0; p < protoCount; p++) {
                        if (rf_match_protocol(rf_protocol_at(p), changeCount, timings, out)) {
                            out.preset = "Ook270Async";
                            RF_DBG(
                                "decode MATCH proto=%s key=%llX bits=%d te=%d",
                                out.protocol.c_str(),
                                (unsigned long long)out.key,
                                out.Bit,
                                out.te
                            );
                            return true;
                        }
                    }
                    RF_DBG("decode: no protocol matched (changeCount=%u)", changeCount);
                    repeatCount = 0;
                }
            }
            changeCount = 0;
        }

        if (changeCount >= RF_MAX_CHANGES) {
            changeCount = 0;
            repeatCount = 0;
        }
        timings[changeCount++] = dur;
    }

    return false;
}

// KeeLoq dedicated decoder — faithful port of the reference feed() state machine
// (te_short=400, te_long=800, te_delta=180). Walks the signed durations,
// resyncing on the 11-pulse header + long sync gap, then reads 64 PWM bits
// MSB-first (short HIGH + long LOW = 1; long HIGH + short LOW = 0).
#define RF_KL_TE_SHORT 400
#define RF_KL_TE_LONG 800
#define RF_KL_TE_DELTA 180
#define RF_KL_MIN_BITS 64

static inline uint32_t rf_kl_diff(uint32_t a, uint32_t b) { return (a > b) ? (a - b) : (b - a); }

bool rf_decode_keeloq(const std::vector<int> &durations, RfCodes &out) {
    enum { ST_RESET, ST_PREAMBLE, ST_SAVE, ST_CHECK } step = ST_RESET;
    int header = 0;
    uint64_t data = 0;
    int bits = 0;
    uint32_t te_last = 0;

    for (int raw : durations) {
        bool level = raw > 0;
        uint32_t dur = (uint32_t)(raw > 0 ? raw : -raw);

        switch (step) {
            case ST_RESET:
                if (level && rf_kl_diff(dur, RF_KL_TE_SHORT) < RF_KL_TE_DELTA) {
                    step = ST_PREAMBLE;
                    header++;
                }
                break;
            case ST_PREAMBLE:
                if (!level && rf_kl_diff(dur, RF_KL_TE_SHORT) < RF_KL_TE_DELTA) {
                    step = ST_RESET; // a header LOW: keep counting via next HIGH
                    break;
                }
                if (header > 2 && rf_kl_diff(dur, RF_KL_TE_SHORT * 10) < RF_KL_TE_DELTA * 10) {
                    step = ST_SAVE; // sync gap found
                    data = 0;
                    bits = 0;
                } else {
                    step = ST_RESET;
                    header = 0;
                }
                break;
            case ST_SAVE:
                if (level) {
                    te_last = dur;
                    step = ST_CHECK;
                }
                break;
            case ST_CHECK:
                if (!level) {
                    if (dur >= (uint32_t)(RF_KL_TE_SHORT * 2 + RF_KL_TE_DELTA)) {
                        // End of transmission.
                        if (bits >= RF_KL_MIN_BITS && bits <= RF_KL_MIN_BITS + 2) {
                            out.key = data;
                            out.Bit = RF_KL_MIN_BITS;
                            out.te = RF_KL_TE_SHORT;
                            out.protocol = "KeeLoq";
                            out.preset = "Ook650Async";
                            RF_DBG("decode keeloq: key=%llX bits=%d", (unsigned long long)data, bits);
                            return true;
                        }
                        step = ST_RESET;
                        header = 0;
                    } else if (rf_kl_diff(te_last, RF_KL_TE_SHORT) < RF_KL_TE_DELTA &&
                               rf_kl_diff(dur, RF_KL_TE_LONG) < RF_KL_TE_DELTA * 2) {
                        if (bits < RF_KL_MIN_BITS) data = (data << 1) | 1ULL;
                        bits++;
                        step = ST_SAVE;
                    } else if (rf_kl_diff(te_last, RF_KL_TE_LONG) < RF_KL_TE_DELTA * 2 &&
                               rf_kl_diff(dur, RF_KL_TE_SHORT) < RF_KL_TE_DELTA) {
                        if (bits < RF_KL_MIN_BITS) data = (data << 1);
                        bits++;
                        step = ST_SAVE;
                    } else {
                        step = ST_RESET;
                        header = 0;
                    }
                } else {
                    step = ST_RESET;
                    header = 0;
                }
                break;
        }
    }

    // Capture may end before the trailing gap; accept a full 64-bit payload.
    if (bits >= RF_KL_MIN_BITS && bits <= RF_KL_MIN_BITS + 2) {
        out.key = data;
        out.Bit = RF_KL_MIN_BITS;
        out.te = RF_KL_TE_SHORT;
        out.protocol = "KeeLoq";
        out.preset = "Ook650Async";
        RF_DBG("decode keeloq(eof): key=%llX bits=%d", (unsigned long long)data, bits);
        return true;
    }
    return false;
}

bool rf_encode_protocol(
    uint64_t data, unsigned int bits, int te, const RfProtocolDef *def, int repeat, std::vector<int> &out
) {
    out.clear();
    if (def == nullptr || bits == 0) return false;
    int base = (te > 0) ? te : def->te;
    if (base <= 0) return false;
    if (repeat < 1) repeat = 1;

    // Faithful port of the classic OOK send: for each repetition emit the data bits
    // MSB first, then the sync/pilot pulse. Each pulse is firstLevel for
    // high*te µs then secondLevel for low*te µs; inverted protocols swap levels.
    const bool inv = def->inverted;
    const bool hasSync = (def->sync.high != 0 || def->sync.low != 0);

    out.reserve((size_t)repeat * (bits + 1) * 2);

    auto emitPulse = [&](const HighLow &p) {
        long first = (long)p.high * base;
        long second = (long)p.low * base;
        // sign: + HIGH, - LOW. Non-inverted -> first half HIGH, second LOW.
        out.push_back(inv ? -(int)first : (int)first);
        out.push_back(inv ? (int)second : -(int)second);
    };

    for (int r = 0; r < repeat; r++) {
        for (int i = (int)bits - 1; i >= 0; i--) {
            const HighLow &p = ((data >> i) & 1ULL) ? def->one : def->zero;
            emitPulse(p);
        }
        if (hasSync) emitPulse(def->sync);
    }
    return true;
}

// KeeLoq OOK framing (te_short=400, te_long=800), ported from the reference
// encoder: 11x{short,short} header, sync {short HIGH, 10*short LOW}, then 64
// data bits MSB-first (bit 1 = short HIGH + long LOW; bit 0 = long HIGH + short
// LOW), a trailing status bit and a large inter-frame gap (40*short).
#define RF_KL_SHORT 400
#define RF_KL_LONG 800

bool rf_keeloq_durations(uint64_t key, std::vector<int> &out) {
    out.clear();
    out.reserve(11 * 2 + 2 + 64 * 2 + 4);

    // Header: 11 short HIGH/LOW pairs.
    for (int i = 0; i < 11; i++) {
        out.push_back(RF_KL_SHORT);
        out.push_back(-RF_KL_SHORT);
    }
    // Sync: short HIGH then 10*short LOW.
    out.push_back(RF_KL_SHORT);
    out.push_back(-RF_KL_SHORT * 10);

    // 64 data bits, MSB first.
    for (int i = 63; i >= 0; i--) {
        if ((key >> i) & 1ULL) { // bit 1: short HIGH, long LOW
            out.push_back(RF_KL_SHORT);
            out.push_back(-RF_KL_LONG);
        } else { // bit 0: long HIGH, short LOW
            out.push_back(RF_KL_LONG);
            out.push_back(-RF_KL_SHORT);
        }
    }
    // Trailing status bit + end pulse + large inter-frame gap.
    out.push_back(RF_KL_SHORT);
    out.push_back(-RF_KL_LONG);
    out.push_back(RF_KL_SHORT);
    out.push_back(-RF_KL_SHORT * 40);
    return true;
}
