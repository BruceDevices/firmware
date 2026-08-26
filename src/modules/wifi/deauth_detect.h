#pragma once

#include <cstdint>

#if !defined(LITE_VERSION)

// Deauth/disassoc flood detector.
// Watches management-frame deauth rate on the selected channel and raises a
// visual alert when it crosses a user-adjustable threshold.
void deauth_detect_setup();

// Headless variant for the `bluefish deauth` serial command: no display output, runs for
// durationMs and reports status to serialDevice (works over USB or the BLE serial bridge).
void deauth_detect_headless(uint32_t durationMs);

#endif
