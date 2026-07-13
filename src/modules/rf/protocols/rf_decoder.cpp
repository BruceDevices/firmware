// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). This file contains code DERIVED FROM and
// modified after:
//   - rc-switch (LGPL-2.1-or-later), (C) 2011 Suat Ozgur and contributors —
//     the classic OOK protocol capture/decode state machine;
//   - Flipper Zero firmware (GPL-3.0-or-later), (C) Flipper Devices Inc. and
//     contributors — the KeeLoq frame decoder framing.
// See THIRD_PARTY.md for full attribution.
#include "rf_decoder.h"
#include "../rf_utils.h" // setup_rf_rx, find_pulse_index, crc64_ecma, RMT defines
#include "rf_config.h"   // RF_DBG
#include "rf_registry.h"
#include <globals.h>

// --- Decode tuning (mirrors the classic OOK receiver) ----------------------
#define RF_SEPARATION_LIMIT 4300 // µs: a longer low is treated as an inter-frame gap
#define RF_RECEIVE_TOLERANCE 60  // % tolerance on pulse-length matching
#define RF_NOMINAL_TE_TOLERANCE 20
#define RF_MAX_CHANGES 131       // max transitions kept per frame

// --- RX noise rejection ----------------------------------------------------
// In RX the CC1101 OOK slicer outputs random hash when there is no real signal.
// Without these filters every noise burst becomes a "phantom" capture, flooding
// Scan/Copy. The classic OOK receiver rejects this via its own ISR noise
// pre-filter; we reproduce it here.
#define RF_RX_MIN_TRANSITIONS 16 // discard captures with fewer edges (noise bursts)
#define RF_M5_RX_GLITCH_US 220   // M5 RF433R AGC noise is mostly shorter than real OOK pulses
#define RF_M5_RX_MIN_TRANSITIONS 110
// NOTE: signal_range_min_ns is kept at the framework-proven 3µs; larger values
// (e.g. 100µs) were observed to stop the RMT receive from completing at all.

static inline unsigned int rf_udiff(int a, int b) { return (unsigned int)abs(a - b); }
static inline unsigned int rf_udiff_u(unsigned int a, unsigned int b) {
    return (a > b) ? (a - b) : (b - a);
}

static int rf_push_duration_merge(std::vector<int> &out, int d) {
    if (!out.empty() && ((out.back() > 0) == (d > 0))) {
        out.back() += d;
        return 1;
    }
    out.push_back(d);
    return 0;
}

static void rf_filter_m5_rx_glitches(std::vector<int> &durations) {
    if (bruceConfigPins.rfModule != M5_RF_MODULE || durations.empty()) return;

    std::vector<int> filtered;
    filtered.reserve(durations.size());

    int removed = 0;
    int merged = 0;
    for (int d : durations) {
        if (abs(d) < RF_M5_RX_GLITCH_US) {
            removed++;
            continue;
        }
        merged += rf_push_duration_merge(filtered, d);
    }

    if (removed > 0) {
        RF_DBG(
            "m5 glitch filter: %u -> %u durations, removed=%d merged=%d min=%dus",
            (unsigned)durations.size(),
            (unsigned)filtered.size(),
            removed,
            merged,
            RF_M5_RX_GLITCH_US
        );
        durations.swap(filtered);
    }
}

// ---------------------------------------------------------------------------
// RMT capture session
// ---------------------------------------------------------------------------
static bool rf_rx_done_cb(
    rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data
) {
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    xQueueSendFromISR(queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

static portMUX_TYPE rf_m5_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool rf_m5_active = false;
static volatile bool rf_m5_last_level = false;
static volatile uint32_t rf_m5_last_edge_us = 0;
static volatile int rf_m5_count = 0;
static volatile int rf_m5_ready_count = 0;
static int rf_m5_pin = -1;
static int rf_m5_buf[RF_MAX_CHANGES * 4];

static void IRAM_ATTR rf_m5_edge_isr() {
    if (!rf_m5_active || rf_m5_ready_count > 0) return;

    uint32_t now = micros();
    bool level = digitalRead(rf_m5_pin);
    uint32_t dur = now - rf_m5_last_edge_us;
    if (dur < RF_M5_RX_GLITCH_US) return;

    portENTER_CRITICAL_ISR(&rf_m5_mux);
    if (rf_m5_count < (int)(sizeof(rf_m5_buf) / sizeof(rf_m5_buf[0]))) {
        int idx = rf_m5_count;
        rf_m5_buf[idx] = rf_m5_last_level ? (int)dur : -(int)dur;
        rf_m5_count = idx + 1;
    } else {
        rf_m5_ready_count = rf_m5_count;
        rf_m5_count = 0;
    }
    rf_m5_last_level = level;
    rf_m5_last_edge_us = now;
    portEXIT_CRITICAL_ISR(&rf_m5_mux);
}

void RfRxSession::arm() {
    rmt_receive_config_t cfg = {};
    cfg.signal_range_min_ns = 3000; // 3µs minimum (framework-proven); noise is
                                    // rejected by the transition-count floor below
    // 30ms idle ends the capture. Must exceed the largest inter-frame gap so that
    // several repeats stay in one capture (the decoder needs two gaps to lock on,
    // exactly like the continuous OOK receiver). NICE's gap is ~25ms; the RMT
    // hardware idle threshold maxes out near 32ms.
    cfg.signal_range_max_ns = 30000000;
    esp_err_t err = rmt_receive(_ch, _buf, _bufSymbols * sizeof(rmt_symbol_word_t), &cfg);
    if (err != ESP_OK) RF_DBG("rmt_receive failed: %d", (int)err);
}

bool RfRxSession::begin() {
    if (bruceConfigPins.rfModule == M5_RF_MODULE) {
        if (!initRfModule("rx", bruceConfigPins.rfFreq)) return false;

        portENTER_CRITICAL(&rf_m5_mux);
        rf_m5_pin = bruceConfigPins.rfRx;
        rf_m5_active = true;
        rf_m5_last_level = digitalRead(rf_m5_pin);
        rf_m5_last_edge_us = micros();
        rf_m5_count = 0;
        rf_m5_ready_count = 0;
        portEXIT_CRITICAL(&rf_m5_mux);

        attachInterrupt(digitalPinToInterrupt(rf_m5_pin), rf_m5_edge_isr, CHANGE);
        _m5Isr = true;
        RF_DBG("M5 GPIO RX started on gpio=%d filter=%dus", bruceConfigPins.rfRx, RF_M5_RX_GLITCH_US);
        return true;
    }

    if (_buf == nullptr) {
        _buf = (rmt_symbol_word_t *)malloc(_bufSymbols * sizeof(rmt_symbol_word_t));
        if (_buf == nullptr) return false;
    }
    _ch = setup_rf_rx();
    if (_ch == nullptr) return false;
    _queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    if (_queue == nullptr) {
        rmt_del_channel(_ch);
        _ch = nullptr;
        return false;
    }
    rmt_rx_event_callbacks_t cbs = {};
    cbs.on_recv_done = rf_rx_done_cb;
    if (rmt_rx_register_event_callbacks(_ch, &cbs, _queue) != ESP_OK) {
        end();
        return false;
    }
    rmt_enable(_ch);
    arm();
    return true;
}

bool RfRxSession::poll(std::vector<int> &durations) {
    if (_m5Isr) {
        int ready = 0;
        uint32_t now = micros();

        portENTER_CRITICAL(&rf_m5_mux);
        if (rf_m5_ready_count == 0 && rf_m5_count > 0 && (uint32_t)(now - rf_m5_last_edge_us) > 30000) {
            if (rf_m5_count >= RF_M5_RX_MIN_TRANSITIONS) {
                rf_m5_ready_count = rf_m5_count;
            } else {
                rf_m5_count = 0;
                rf_m5_last_edge_us = now;
                rf_m5_last_level = digitalRead(rf_m5_pin);
            }
        }
        ready = rf_m5_ready_count;
        if (ready > 0) {
            durations.clear();
            durations.reserve(ready);
            for (int i = 0; i < ready; i++) durations.push_back(rf_m5_buf[i]);
            rf_m5_ready_count = 0;
            rf_m5_count = 0;
            rf_m5_last_edge_us = now;
            rf_m5_last_level = digitalRead(rf_m5_pin);
        }
        portEXIT_CRITICAL(&rf_m5_mux);

        if (ready == 0) return false;
        rf_filter_m5_rx_glitches(durations);
#if RF_DEBUG
        RF_DBG("M5 GPIO capture: %u durations", (unsigned)durations.size());
        String head;
        for (size_t i = 0; i < durations.size() && i < 24; i++) head += String(durations[i]) + " ";
        RF_DBG("durations[0..23]: %s", head.c_str());
#endif
        return !durations.empty();
    }

    if (_ch == nullptr) return false;
    rmt_rx_done_event_data_t rx;
    if (xQueueReceive(_queue, &rx, 0) == pdPASS) {
        rf_symbols_to_durations(rx.received_symbols, rx.num_symbols, durations);
        rf_filter_m5_rx_glitches(durations);
        arm(); // re-arm for the next signal

        // Reject noise bursts: too few edges to be a real frame.
        if ((int)durations.size() < RF_RX_MIN_TRANSITIONS) {
            RF_DBG("capture ignored (noise): %u durations", (unsigned)durations.size());
            durations.clear();
            return false;
        }
#if RF_DEBUG
        RF_DBG("capture: %u symbols -> %u durations", (unsigned)rx.num_symbols, (unsigned)durations.size());
        String head;
        for (size_t i = 0; i < durations.size() && i < 24; i++) head += String(durations[i]) + " ";
        RF_DBG("durations[0..23]: %s", head.c_str());
#endif
        return !durations.empty();
    }
    return false;
}

void RfRxSession::end() {
    if (_m5Isr) {
        detachInterrupt(digitalPinToInterrupt(rf_m5_pin));
        portENTER_CRITICAL(&rf_m5_mux);
        rf_m5_active = false;
        rf_m5_count = 0;
        rf_m5_ready_count = 0;
        rf_m5_pin = -1;
        portEXIT_CRITICAL(&rf_m5_mux);
        _m5Isr = false;
    }
    if (_ch != nullptr) {
        rmt_disable(_ch);
        rmt_del_channel(_ch);
        _ch = nullptr;
    }
    if (_queue != nullptr) {
        vQueueDelete(_queue);
        _queue = nullptr;
    }
    if (_buf != nullptr) {
        free(_buf);
        _buf = nullptr;
    }
}

void rf_symbols_to_durations(const rmt_symbol_word_t *symbols, size_t count, std::vector<int> &out) {
    out.clear();
    // RMT RX is configured at 1 MHz (1 tick = 1 µs), so durations are already µs.
    for (size_t i = 0; i < count; i++) {
        int d0 = symbols[i].duration0;
        if (d0 == 0) break;
        out.push_back(symbols[i].level0 ? d0 : -d0);
        int d1 = symbols[i].duration1;
        if (d1 == 0) break;
        out.push_back(symbols[i].level1 ? d1 : -d1);
    }
}

// ---------------------------------------------------------------------------
// OOK decode (faithful port of the classic receiveProtocol state machine)
// ---------------------------------------------------------------------------
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

bool rf_decode_ook(const std::vector<int> &durations, RfCodes &out) {
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

// ---------------------------------------------------------------------------
// RAW builder (port of RFScan::read_raw inner loop)
// ---------------------------------------------------------------------------
int rf_build_raw(
    const std::vector<int> &durations, String &dataOut, bool &hasCrc, uint64_t &crcOut,
    std::vector<int> &indexedOut, int &bitsOut, int &teOut
) {
    dataOut = "";
    hasCrc = false;
    crcOut = 0;
    indexedOut.clear();
    bitsOut = 0;
    teOut = 0;

    std::vector<int> pulseIndexes; // sequence of distinct-pulse indexes, for CRC
    uint8_t repetition = 0;
    int transitions = 0;

    for (int duration : durations) {
        if (duration == 0) break;
        if (transitions > 0) dataOut += " ";

        if (duration < -5000 && repetition < 2) repetition += 1;
        dataOut += String(duration);
        if (teOut == 0 && duration > 0) teOut = duration;

        if (repetition == 1 && duration >= -5000) {
            int index = find_pulse_index(indexedOut, duration);
            if (index == -1) {
                indexedOut.push_back(abs(duration));
                index = indexedOut.size() - 1;
            }
            pulseIndexes.push_back(index);
        }
        transitions++;
    }

    if (repetition >= 2 && !pulseIndexes.empty()) {
        crcOut = crc64_ecma(pulseIndexes);
        bitsOut = pulseIndexes.size();
        hasCrc = true;
    } else {
        indexedOut.clear(); // only meaningful alongside a CRC
    }

    RF_DBG(
        "raw: transitions=%d repetition=%u te=%d hasCrc=%d crc=%llX",
        transitions,
        repetition,
        teOut,
        (int)hasCrc,
        (unsigned long long)crcOut
    );
    return transitions;
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
                    } else if (rf_kl_diff(te_last, RF_KL_TE_SHORT) < RF_KL_TE_DELTA && rf_kl_diff(dur, RF_KL_TE_LONG) < RF_KL_TE_DELTA * 2) {
                        if (bits < RF_KL_MIN_BITS) data = (data << 1) | 1ULL;
                        bits++;
                        step = ST_SAVE;
                    } else if (rf_kl_diff(te_last, RF_KL_TE_LONG) < RF_KL_TE_DELTA * 2 && rf_kl_diff(dur, RF_KL_TE_SHORT) < RF_KL_TE_DELTA) {
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
