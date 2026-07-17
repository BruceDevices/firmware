// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). This file contains code DERIVED FROM and
// modified after:
//   - rc-switch (LGPL-2.1-or-later), (C) 2011 Suat Ozgur and contributors —
//     the classic OOK protocol send state machine;
//   - Flipper Zero firmware (GPL-3.0-or-later), (C) Flipper Devices Inc. and
//     contributors — the KeeLoq OOK framing.
// See THIRD_PARTY.md for full attribution.
#include "rf_encoder.h"
#include "../rf_utils.h" // bruceConfigPins, RMT defines
#include "rf_config.h"   // RF_DBG
#include "rf_registry.h" // rf_find_protocol (self-test)
#include <cstdlib>       // abs
#include <driver/rmt_tx.h>

// RMT duration fields are 15-bit: a single half-pulse maxes out at 32767 ticks.
// At 1 MHz resolution (1 tick = 1 µs) that is 32767 µs; longer stretches are
// split into several same-level entries (electrically continuous).
#define RF_RMT_MAX_DUR 32767

// Resolve the GPIO that actually drives the RF output for the current board.
static gpio_num_t rf_tx_gpio() {
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE)
        return gpio_num_t(bruceConfigPins.CC1101_bus.io0);
    return gpio_num_t(bruceConfigPins.rfTx);
}

// Append a half-pulse (one logic level held for `dur` µs), splitting it when it
// exceeds the 15-bit RMT field.
static void rf_push_half(std::vector<rmt_symbol_word_t> &syms, bool &pendingLow, uint8_t level, long dur) {
    while (dur > 0) {
        uint16_t chunk = (dur > RF_RMT_MAX_DUR) ? RF_RMT_MAX_DUR : (uint16_t)dur;
        dur -= chunk;
        if (!pendingLow) {
            rmt_symbol_word_t s = {};
            s.level0 = level;
            s.duration0 = chunk;
            syms.push_back(s);
            pendingLow = true;
        } else {
            rmt_symbol_word_t &s = syms.back();
            s.level1 = level;
            s.duration1 = chunk;
            pendingLow = false;
        }
    }
}

bool rf_tx_durations(const std::vector<int> &durations) {
    if (durations.empty()) return false;

    // Pack signed µs timings into RMT symbols (two half-pulses per symbol).
    std::vector<rmt_symbol_word_t> syms;
    syms.reserve(durations.size() / 2 + 1);
    bool pendingLow = false;
    for (int d : durations) {
        if (d == 0) continue;
        uint8_t level = (d > 0) ? 1 : 0;
        long dur = (d > 0) ? d : -d;
        rf_push_half(syms, pendingLow, level, dur);
    }
    if (syms.empty()) return false;

    RF_DBG("tx: %u durations -> %u symbols", (unsigned)durations.size(), (unsigned)syms.size());

    // Create the RMT TX channel on the RF output pin (1 MHz / 1 tick = 1 µs,
    // matching the RX side). The radio itself is already configured by the
    // caller (CC1101 SetTx / single-pin OUTPUT).
    rmt_tx_channel_config_t tx_cfg = {};
    tx_cfg.gpio_num = rf_tx_gpio();
    tx_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
    tx_cfg.resolution_hz = 1 * 1000 * 1000;
    tx_cfg.mem_block_symbols = 64;
    tx_cfg.trans_queue_depth = 4;
    tx_cfg.flags.invert_out = false;
    tx_cfg.flags.with_dma = false;

    rmt_channel_handle_t ch = nullptr;
    esp_err_t err = rmt_new_tx_channel(&tx_cfg, &ch);
    if (err != ESP_OK) {
        RF_DBG("rmt_new_tx_channel failed: %d", (int)err);
        return false;
    }

    rmt_encoder_handle_t encoder = nullptr;
    rmt_copy_encoder_config_t copy_cfg = {};
    err = rmt_new_copy_encoder(&copy_cfg, &encoder);
    if (err != ESP_OK) {
        RF_DBG("rmt_new_copy_encoder failed: %d", (int)err);
        rmt_del_channel(ch);
        return false;
    }

    bool ok = (rmt_enable(ch) == ESP_OK);
    if (ok) {
        rmt_transmit_config_t txc = {};
        txc.loop_count = 0;        // repetitions are already baked into `syms`
        txc.flags.eot_level = 0;   // leave the line LOW when done
        err = rmt_transmit(ch, encoder, syms.data(), syms.size() * sizeof(rmt_symbol_word_t), &txc);
        if (err != ESP_OK) {
            RF_DBG("rmt_transmit failed: %d", (int)err);
            ok = false;
        } else {
            err = rmt_tx_wait_all_done(ch, 2000); // up to 2s for the frame to flush
            if (err != ESP_OK) {
                RF_DBG("rmt_tx_wait_all_done failed: %d", (int)err);
                ok = false;
            }
        }
        rmt_disable(ch);
    }

    rmt_del_encoder(encoder);
    rmt_del_channel(ch);
    return ok;
}

bool rf_encode_protocol(
    uint64_t data, unsigned int bits, int te, const RfProtocolDef *def, int repeat,
    std::vector<int> &out
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

bool rf_tx_protocol(uint64_t data, unsigned int bits, int te, const RfProtocolDef *def, int repeat) {
    std::vector<int> durs;
    if (!rf_encode_protocol(data, bits, te, def, repeat, durs)) return false;

    RF_DBG(
        "tx_protocol: proto=%s data=%llX bits=%u te=%d repeat=%d",
        def->name,
        (unsigned long long)data,
        bits,
        (te > 0) ? te : def->te,
        (repeat < 1) ? 1 : repeat
    );
    return rf_tx_durations(durs);
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

bool rf_tx_keeloq(uint64_t key, int repeat) {
    if (repeat < 1) repeat = 1;
    std::vector<int> frame;
    if (!rf_keeloq_durations(key, frame)) return false;

    std::vector<int> durs;
    durs.reserve(frame.size() * repeat);
    for (int r = 0; r < repeat; r++) durs.insert(durs.end(), frame.begin(), frame.end());

    RF_DBG("tx_keeloq: key=%llX repeat=%d", (unsigned long long)key, repeat);
    return rf_tx_durations(durs);
}

bool rf_tx_raw_timings(const int *timings) {
    if (!timings) return false;
    std::vector<int> durs;
    for (size_t i = 0; timings[i] != 0; i++) durs.push_back(timings[i]);
    return rf_tx_durations(durs);
}

bool rf_tx_raw_bits(const String &bits, int te) {
    if (bits.length() == 0 || te <= 0) return false;
    // Each bit is one half-pulse of `te` µs at the matching level. Matches the
    // legacy bit sender, which walked the string from the end toward the start.
    std::vector<int> durs;
    durs.reserve(bits.length());
    for (int i = bits.length() - 1; i >= 0; i--) {
        char c = bits[i];
        if (c == '1') durs.push_back(te);
        else if (c == '0') durs.push_back(-te);
        // any other char is skipped (as before)
    }
    return rf_tx_durations(durs);
}

#if RF_DEBUG
// ---------------------------------------------------------------------------
// Golden encoder self-test (`subghz selftest`).
//
// For each reference-derived protocol, encode key=0xA / 4 bits / 1 repetition
// and compare the produced durations against the EXPECTED absolute timings,
// written here straight from the protocol spec (te_short/te_long, sync gap and
// preamble) rather than re-derived from the registry factors. A mismatch beyond
// RF_GOLDEN_TOL therefore catches a wrong factor, te or pulse order in a def —
// the spec-fidelity gap that loopback (self-consistent by construction) hides.
// ---------------------------------------------------------------------------
#define RF_GOLDEN_TOL 16 // µs (covers the few protocols whose te_long != k*te_short)

// key=0xA -> bits 1,0,1,0 (MSB first) -> one,zero,one,zero,sync
static const int g_linear[]     = {1500,-500, 500,-1500, 1500,-500, 500,-1500, 1500,-21000};
static const int g_clemsa[]     = {2695,-385, 385,-2695, 2695,-385, 385,-2695, 2695,-19250};
static const int g_mastercode[] = {2145,-1072, 1072,-2145, 2145,-1072, 1072,-2145, 2145,-15008};
static const int g_came[]       = {-320,640, -640,320, -320,640, -640,320, -11520,320};
static const int g_nice12[]     = {-1400,700, -700,1400, -1400,700, -700,1400, -25200,700};
static const int g_ansonic[]    = {-1111,555, -555,1111, -1111,555, -555,1111, -19425,555};
static const int g_gatetx[]     = {-700,350, -350,700, -700,350, -350,700, -17150,700};
static const int g_holtek[]     = {-870,430, -430,870, -870,430, -430,870, -15480,430};
static const int g_phoenixv2[]  = {-853,427, -427,853, -853,427, -427,853, -25620,2562};

bool rf_encoder_selftest() {
    struct Case {
        const char *name;
        uint64_t key;
        unsigned bits;
        const int *exp;
        size_t n;
    };
    static const Case cases[] = {
        {"Linear", 0xA, 4, g_linear, sizeof(g_linear) / sizeof(int)},
        {"Clemsa", 0xA, 4, g_clemsa, sizeof(g_clemsa) / sizeof(int)},
        {"Mastercode", 0xA, 4, g_mastercode, sizeof(g_mastercode) / sizeof(int)},
        {"CAME", 0xA, 4, g_came, sizeof(g_came) / sizeof(int)},
        {"Nice_12bit", 0xA, 4, g_nice12, sizeof(g_nice12) / sizeof(int)},
        {"Ansonic", 0xA, 4, g_ansonic, sizeof(g_ansonic) / sizeof(int)},
        {"GateTX", 0xA, 4, g_gatetx, sizeof(g_gatetx) / sizeof(int)},
        {"Holtek", 0xA, 4, g_holtek, sizeof(g_holtek) / sizeof(int)},
        {"Holtek_12bit", 0xA, 4, g_holtek, sizeof(g_holtek) / sizeof(int)},
        {"PhoenixV2", 0xA, 4, g_phoenixv2, sizeof(g_phoenixv2) / sizeof(int)},
    };

    bool allok = true;
    for (const auto &c : cases) {
        const RfProtocolDef *def = rf_find_protocol(c.name);
        std::vector<int> durs;
        bool ok = (def != nullptr) && rf_encode_protocol(c.key, c.bits, 0, def, 1, durs);
        int bad = -1;
        if (ok) {
            if (durs.size() != c.n) {
                ok = false;
                bad = -2; // length mismatch
            } else {
                for (size_t i = 0; i < c.n; i++) {
                    if (abs(durs[i] - c.exp[i]) > RF_GOLDEN_TOL) {
                        ok = false;
                        bad = (int)i;
                        break;
                    }
                }
            }
        }
        if (ok) {
            RF_DBG("selftest %-11s PASS", c.name);
        } else if (bad == -2) {
            RF_DBG("selftest %-11s FAIL (len got=%u exp=%u)", c.name, (unsigned)durs.size(), (unsigned)c.n);
        } else if (bad >= 0) {
            RF_DBG(
                "selftest %-11s FAIL @%d got=%d exp=%d", c.name, bad, durs[bad], c.exp[bad]
            );
        } else {
            RF_DBG("selftest %-11s FAIL (no def/encode)", c.name);
        }
        allok = allok && ok;
    }
    RF_DBG("selftest result: %s", allok ? "ALL PASS" : "FAILURES");
    return allok;
}
#endif // RF_DEBUG
