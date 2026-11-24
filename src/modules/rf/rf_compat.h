#ifndef __RF_COMPAT_H__
#define __RF_COMPAT_H__

// Compatibility shims for the SubGHz / nRF24 pages ported from Vandeta.
//
// Those pages read a handful of numbers from Vandeta's developer console
// (`core/dev/tunables.h`) and could be patched at runtime through its IRAM
// overlay (`core/dev/iram_patch.h`). Neither exists in Bruce, and porting them
// would mean porting a debugger. These are the two things the RF code actually
// needed:
//
//  * tuneGetByName() answering with the value each tunable defaulted to, so the
//    pages behave exactly as they do on a stock Vandeta build.
//  * a no-op patch gate. The overlay thunks were replaced by direct calls at
//    the two call sites, so nothing here has to jump anywhere.

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

// Value every tunable has when nobody changed it. These are Vandeta's defaults,
// not guesses: they are the constants those entries replaced.
struct RfCompatTune {
    const char *name;
    int32_t value;
};

static const RfCompatTune rf_compat_tunes[] = {
    {"rf.noise_floor", -93},   {"rf.park_ms", 15000},  {"rf.scan_dwell_ms", 70},
    {"rf.spi_hold_us", 0},     {"rf.scan_variant", 2}, {"nrf.cap_ms", 3000},
    {"nrf.repeat", 3},         {"nrf.capture_variant", 0},
    {"ward.dup_window_ms", 30000},
};

inline int32_t tuneGetByName(const char *name, bool *found) {
    if (found != nullptr) *found = false;
    if (name == nullptr) return 0;
    for (size_t i = 0; i < sizeof(rf_compat_tunes) / sizeof(rf_compat_tunes[0]); i++) {
        if (strcmp(rf_compat_tunes[i].name, name) == 0) {
            if (found != nullptr) *found = true;
            return rf_compat_tunes[i].value;
        }
    }
    return 0;
}

// The wardriver mode tunables name constants that live in rf_wardriver.h; the
// reader is a plain fallback so the header does not have to be included here.
inline int32_t tuneGetByNameOr(const char *name, int32_t fallback) {
    bool found = false;
    const int32_t v = tuneGetByName(name, &found);
    return found ? v : fallback;
}

// Gate that used to block a code flip while a radio loop was running. With no
// overlay to flip it has nothing to protect, but the call sites keep their
// scope guard so the intent stays visible.
struct VndPatchGate {
    VndPatchGate() {}
};

#endif
