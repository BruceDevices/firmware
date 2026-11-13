// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rf_classify.h"

#include <algorithm>

namespace {

// Median of an int vector (sorts a copy). Returns 0 when empty.
int median_of(std::vector<int> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    if (n & 1u) return v[n / 2];
    return (int)(((long)v[n / 2 - 1] + (long)v[n / 2]) / 2);
}

float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

} // namespace

RfClassifyResult rf_classify(const std::vector<int> &durations) {
    RfClassifyResult r;

    // Collect absolute widths, splitting into HIGH (carrier) and LOW (gap)
    // halves. The RMT RX stream alternates HIGH / LOW, so sign is meaningful.
    std::vector<int> absv, highs, lows;
    long long sumHigh = 0, sumTotal = 0;
    absv.reserve(durations.size());
    for (int d : durations) {
        if (d == 0) continue;
        const int a = d < 0 ? -d : d;
        if (a < 4) continue; // drop sub-4us glitches / ringing
        absv.push_back(a);
        sumTotal += a;
        if (d > 0) {
            highs.push_back(a);
            sumHigh += a;
        } else {
            lows.push_back(a);
        }
    }

    r.pulseCount = (int)absv.size();
    if (absv.size() < 6) return r; // too few pulses: UNKNOWN

    r.dutyCycle = sumTotal ? (float)((double)sumHigh / (double)sumTotal) : 0.0f;
    r.minUs = *std::min_element(absv.begin(), absv.end());
    r.maxUs = *std::max_element(absv.begin(), absv.end());
    r.medianUs = median_of(absv);

    const int medHigh = median_of(highs);
    const int medLow = median_of(lows);

    // Spread: how far the longest pulse sits from the typical pulse. OOK has a
    // sync pulse / long off-gaps (spread >> 1); FSK is tightly clustered.
    const float spread = r.medianUs > 0 ? (float)r.maxUs / (float)r.medianUs : 1.0f;
    const float hlRatio =
        (medHigh > 0 && medLow > 0)
            ? (float)std::max(medHigh, medLow) / (float)std::min(medHigh, medLow)
            : 1.0f;

    if (spread >= 4.0f || (medHigh > 0 && medLow >= 3 * medHigh)) {
        // Clear OOK signature: long sync/gap pulses dominate the spread.
        r.modulation = RfClassifyMod::OOK;
        r.confidence = clamp01(0.55f + (spread - 4.0f) * 0.05f);
    } else if (spread < 2.5f && hlRatio < 2.5f && r.dutyCycle >= 0.30f &&
               r.dutyCycle <= 0.70f) {
        // Continuous, balanced square wave => 2-FSK/GFSK/MSK.
        r.modulation = RfClassifyMod::FSK;
        r.confidence = spread < 1.8f ? 0.9f : 0.7f;
    } else {
        // Ambiguous: default to OOK (the scan/RX default) with low confidence.
        r.modulation = RfClassifyMod::OOK;
        r.confidence = 0.4f;
    }

    if (r.modulation == RfClassifyMod::OOK) {
        // Base pulse period from the HIGH bursts, dropping sync outliers.
        int te = medHigh > 0 ? medHigh : r.minUs;
        if (medHigh > 0) {
            std::vector<int> trimmed;
            for (int a : highs)
                if (a <= medHigh * 3) trimmed.push_back(a);
            if (!trimmed.empty()) te = median_of(trimmed);
        }
        r.teUs = std::max(20, te);
        // Payload transitions: pulses within 0.5x..2.2x of the base period.
        int bits = 0;
        for (int a : absv)
            if (a >= r.teUs / 2 && a <= r.teUs * 2.2f) bits++;
        r.bitCount = bits;
    } else if (r.modulation == RfClassifyMod::FSK) {
        // Each level of the demodulated square wave is ~one symbol period.
        const int period = std::max(4, r.medianUs);
        r.baud = (int)(1000000.0f / (float)period);
        r.bitCount = r.pulseCount; // continuous stream: transitions ~= symbols
    }

    return r;
}
