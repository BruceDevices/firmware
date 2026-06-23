#pragma once

#include "../structs.h" // HighLow
#include <stdint.h>

// Single source of truth for static OOK protocol definitions and radio
// presets used by the RF module. Consumers (send / scan / replay) read
// from here; no protocol parameters should be redefined elsewhere.

// ---------------------------------------------------------------------------
// Radio preset: configures the transceiver (modulation, bandwidth, deviation,
// data rate). Replaces the string if/else previously inlined in sendRfCommand.
// A field left at 0 means "keep the module default" (do not override).
// ---------------------------------------------------------------------------
struct RfPreset {
    const char *name;     // canonical preset name written/read in `.sub`
    uint8_t modulation;   // CC1101: 0=2-FSK, 1=GFSK, 2=ASK/OOK, 4=MSK
    float deviation;      // kHz (0 = keep default)
    float rxBW;           // kHz (0 = keep default)
    float dataRate;       // kbps (0 = keep default)
    uint8_t legacyProto;  // default legacy protocol no. for OOK presets
                          // (kept so the legacy TX path stays bit-identical;
                          //  irrelevant for FSK/MSK/GFSK presets)
};

// ---------------------------------------------------------------------------
// Static OOK protocol definition. Timings follow the classic factor model:
// every pulse is a multiple of `te` µs, expressed as {high, low} counts.
// `name` is the protocol identity written to `Protocol:` in the `.sub` file
// and used for replay dispatch — choose neutral, stable names.
// ---------------------------------------------------------------------------
struct RfProtocolDef {
    const char *name;
    uint16_t te;     // base pulse length in µs
    HighLow sync;    // sync / pilot factor ({0,0} = none)
    HighLow zero;    // bit 0 encoding
    HighLow one;     // bit 1 encoding
    uint8_t bits;    // typical payload length in bits (0 = variable)
    bool inverted;   // inverted signal level
    uint8_t flags;   // bitmask, see RF_PF_* below
};

// Protocol flags bitmask.
enum RfProtocolFlags : uint8_t {
    RF_PF_HAS_SYNC = 0x01,  // protocol uses a sync/pilot pulse
    RF_PF_FIXED_LEN = 0x02, // payload length is fixed (== bits)
};
