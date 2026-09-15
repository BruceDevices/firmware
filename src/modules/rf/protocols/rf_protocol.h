#pragma once

#include "../structs.h" // HighLow, RfCodes
#include <stdint.h>
#include <vector>

// Forward declarations for callback signatures.
struct RfCodes;

// ---------------------------------------------------------------------------
// Callback signatures for protocol-specific decode / encode.
// A decode callback receives raw pulse durations (µs, +HIGH/-LOW) and, on
// success, fills `out` and returns true.
// An encode callback receives the `RfCodes` payload and appends signed-µs
// durations to the `out` vector; returns true on success.
// ---------------------------------------------------------------------------
typedef bool (*RfDecodeCallback)(const std::vector<int>& durations, RfCodes& out);
typedef bool (*RfEncodeCallback)(const RfCodes& in, std::vector<int>& out);

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
//
// Extended protocols that do NOT fit the factor model provide decode/encode
// callbacks instead. When a protocol has a decode callback the factor model
// is bypassed entirely.
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

    // Optional callback for protocol-specific decode. When non-NULL the
    // factor model is skipped and this callback is invoked instead.
    RfDecodeCallback decode;
    // Optional callback for protocol-specific encode. When non-NULL the
    // generic factor-based encoder is replaced by this callback.
    RfEncodeCallback encode;
};

// Protocol flags bitmask.
enum RfProtocolFlags : uint8_t {
    RF_PF_HAS_SYNC = 0x01,  // protocol uses a sync/pilot pulse
    RF_PF_FIXED_LEN = 0x02, // payload length is fixed (== bits)
    RF_PF_HAS_DECODER = 0x04, // protocol has a custom decode callback
};
