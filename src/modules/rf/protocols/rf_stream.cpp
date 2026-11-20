// SPDX-License-Identifier: AGPL-3.0-or-later
#if !defined(LITE_VERSION)
#include "rf_stream.h"
#include "rf_decode_limits.h"
#include <atomic>

namespace {
std::atomic_flag busy = ATOMIC_FLAG_INIT;
void frame(void *context, const char *data, const char *text, bool encoder, bool invalid) {
    auto &frames = *static_cast<std::vector<RfStreamResult> *>(context);
    if (frames.size() < 16) frames.push_back({data, text, encoder, invalid});
}
struct Encoded {
    std::vector<int> &pulses;
    String &updated;
};
void encoded(void *context, const int32_t *pulses, size_t count, const char *updated) {
    auto &result = *static_cast<Encoded *>(context);
    result.pulses.assign(pulses, pulses + count);
    result.updated = updated;
}
} // namespace

const char *rf_stream_read_frames(
    const RfStreamCodec *codec, const char *record, const std::vector<int> &pulses,
    std::vector<RfStreamResult> &frames
) {
    // Merge consecutive equal levels exactly as the original pulse parser did.
    std::vector<int32_t> normalized;
    uint64_t duration = 0;
    if (pulses.size() > RF_DECODE_MAX_PULSES) return "RAW capture exceeds pulse limit";
    for (int pulse : pulses) {
        if (!pulse || pulse < -1000000 || pulse > 1000000) return "Invalid pulse";
        duration += pulse < 0 ? -pulse : pulse;
        if (duration > RF_MAX_CAPTURE_DURATION_US) return "Capture exceeds 60 seconds";
        if (!normalized.empty() && (normalized.back() > 0) == (pulse > 0)) {
            int64_t sum = (int64_t)normalized.back() + pulse;
            if (sum < -1000000 || sum > 1000000) return "Invalid combined pulse";
            normalized.back() = (int32_t)sum;
        } else normalized.push_back(pulse);
    }
    if (busy.test_and_set()) return "Protocol engine is busy";
    const char *error = rf_stream_read(
        codec, record, normalized.empty() ? nullptr : normalized.data(), normalized.size(), frame, &frames
    );
    busy.clear();
    return error;
}
const char *rf_stream_write_pulses(
    const RfStreamCodec *codec, const char *record, std::vector<int> &pulses, String &updated
) {
    pulses.clear();
    if (busy.test_and_set()) return "Protocol engine is busy";
    Encoded result{pulses, updated};
    const char *error = rf_stream_write(codec, record, encoded, &result);
    busy.clear();
    return error;
}
#endif
