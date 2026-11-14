// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later).
//
// Sub-GHz modulation auto-classifier. Operates purely on a captured pulse
// train (signed microsecond durations, HIGH > 0 / LOW < 0 as returned by the
// RMT RX session) and estimates the modulation family and the symbol timing
// without touching the radio or any protocol decoder.
//
// The heuristics mirror the classical structure of demodulated sub-GHz
// bitstreams:
//   - ASK/OOK  : bursts of carrier (short highs) separated by long off gaps,
//                typically with a sync pulse. Large max/median pulse spread
//                and a low duty cycle.
//   - 2-FSK    : a continuous square wave (both mark and space are carrier),
//                so pulse widths cluster tightly around one symbol period and
//                the duty cycle is ~50%.
#pragma once

#include <stdint.h>
#include <vector>

// Classification result families.
enum class RfClassifyMod : uint8_t {
    UNKNOWN = 0, // too few pulses to say anything
    OOK = 1,     // ASK/OOK
    FSK = 2,     // 2-FSK / GFSK / MSK continuous bitstream
};

struct RfClassifyResult {
    RfClassifyMod modulation = RfClassifyMod::UNKNOWN;
    float confidence = 0.0f; // 0..1
    int teUs = 0;            // OOK: base pulse period (µs)
    int baud = 0;            // FSK: estimated symbol rate (symbols/s)
    int bitCount = 0;        // estimated payload transitions
    float dutyCycle = 0.0f;  // high-time fraction 0..1
    int pulseCount = 0;
    int minUs = 0;           // shortest pulse seen
    int medianUs = 0;        // median pulse width
    int maxUs = 0;           // longest pulse seen
};

// Classify a captured pulse train. Pure function: no globals, no radio, no UI.
RfClassifyResult rf_classify(const std::vector<int> &durations);
