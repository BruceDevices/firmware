// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <sstream>
#include <string>
#include <vector>

namespace rf_capture {
// Serialize the capture itself, without repeat detection or timing quantization.
// Flipper RAW files allow at most 512 signed durations on each RAW_Data line.
inline std::string rawLines(const std::vector<int> &durations) {
    std::ostringstream out;
    size_t count = 0;
    for (int duration : durations) {
        if (!duration) continue;
        if (count % 512 == 0) {
            if (count) out << '\n';
            out << "RAW_Data:";
        }
        out << ' ' << duration;
        ++count;
    }
    if (count) out << '\n';
    return out.str();
}
} // namespace rf_capture
