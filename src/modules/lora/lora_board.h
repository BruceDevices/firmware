#pragma once

#include <cmath>

#ifndef LORA_DEFAULT_FREQ_HZ
#define LORA_DEFAULT_FREQ_HZ "434500000.00"
#endif

// Validate the whole occupied channel, including values loaded from LittleFS.
// Boards without an integrated band-specific frontend retain the generic range.
inline bool loraFrequencySupported(double frequencyMHz, double bandwidthKHz) {
    if (!std::isfinite(frequencyMHz) || !std::isfinite(bandwidthKHz) || bandwidthKHz <= 0) return false;
#if defined(LORA_MIN_FREQ_MHZ) && defined(LORA_MAX_FREQ_MHZ)
    const double halfBandwidthMHz = bandwidthKHz / 2000.0;
    return frequencyMHz - halfBandwidthMHz >= LORA_MIN_FREQ_MHZ &&
           frequencyMHz + halfBandwidthMHz <= LORA_MAX_FREQ_MHZ;
#else
    return frequencyMHz > 0 && frequencyMHz <= 1000;
#endif
}
