// Shared capture-identification and clean-transmit helpers, factored out of
// rf_repeater.cpp so rf_subghz.cpp (Frequency Analyzer) can reuse the exact
// same, already on-device-verified logic instead of duplicating it.
#pragma once

#include "structs.h" // RfCodes
#include <vector>

enum class RfMatchKind { NONE, OOK, KEELOQ };

// Tries rf_decode_ook then rf_decode_keeloq. `kind` tells the caller not just
// WHETHER this is a real signal but HOW to transmit it cleanly: a match means
// `outCode` holds the decoded key/bits/te needed to re-synthesize a textbook
// waveform instead of replaying the noisy as-captured edges.
String rf_identify_capture(const std::vector<int> &durations, RfMatchKind &kind, RfCodes &outCode);

// Re-synthesizes a textbook-correct waveform from the decoded protocol timing
// table (NOT a raw replay of the as-captured edges -- raw replay carries
// forward the RX chain's own jitter/distortion). `repeat` is passed straight
// to the encoder so it emits one continuous burst with the protocol's own
// correct inter-repeat gap, confirmed on-device (2026-11-11) to decode
// reliably on a real Flipper Zero receiver.
bool rf_transmit_clean_ook(const RfCodes &code, float freq, int repeat);
bool rf_transmit_clean_keeloq(uint64_t key, float freq, int repeat);

// Last-resort fallback for a signal that didn't match anything in the
// registry/cipher check: replay it exactly as captured.
bool rf_transmit_raw(const std::vector<int> &durations, float freq);
