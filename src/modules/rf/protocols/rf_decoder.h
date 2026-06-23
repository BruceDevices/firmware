// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). This file contains code DERIVED FROM and
// modified after:
//   - rc-switch (LGPL-2.1-or-later), (C) 2011 Suat Ozgur and contributors —
//     the classic OOK protocol capture/decode state machine;
//   - Flipper Zero firmware (GPL-3.0-or-later), (C) Flipper Devices Inc. and
//     contributors — the KeeLoq frame decoder framing.
// See THIRD_PARTY.md for full attribution.
#pragma once

#include "../structs.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <vector>

// ===========================================================================
// Native RMT RX engine + generic OOK decoder.
//
// Replaces the former interrupt-driven receiver on the decode side. The decode
// logic is a faithful port of the classic OOK capture/decode state
// machine (separation-gap detection + per-protocol pulse matching), driven
// from durations captured with the framework's native RMT RX peripheral.
// Protocol definitions come from the central registry (rf_registry).
//
// References / credits — the SubGhz protocol set and decode behavior mirror
// the Flipper Zero ecosystem; timings and protocol identities were validated
// against these sources:
//   - Flipper Zero firmware (SubGhz):
//       https://github.com/flipperdevices/flipperzero-firmware
//       (lib/subghz/protocols/*)
//   - Momentum firmware (Flipper fork with an extended protocol set):
//       https://github.com/Next-Flip/Momentum-Firmware
// ===========================================================================

// One-shot RMT RX capture session. Wraps channel + queue lifecycle so callers
// can poll for captured signals without touching the RMT driver directly.
class RfRxSession {
public:
    // Create + enable the RMT RX channel (via setup_rf_rx) and arm a receive.
    // Returns false if the RF module / channel could not be initialised.
    bool begin();
    // Non-blocking: when a signal has been captured, fills `durations` with the
    // signed pulse lengths (HIGH > 0, LOW < 0, µs), re-arms the receiver and
    // returns true. Returns false when nothing is ready yet.
    bool poll(std::vector<int> &durations);
    // Disable + delete the channel and free the queue/buffer.
    void end();
    bool active() const { return _ch != nullptr; }
    ~RfRxSession() { end(); }

private:
    rmt_channel_handle_t _ch = nullptr;
    QueueHandle_t _queue = nullptr;
    // Heap-allocated capture buffer: keeping ~1KB off the (8KB) serialcmds task
    // stack, where rfReceiveSignal runs, avoids stack overflow / corruption.
    rmt_symbol_word_t *_buf = nullptr;
    static const size_t _bufSymbols = 256;
    void arm();
};

// Convert a buffer of RMT symbols into signed durations (HIGH > 0, LOW < 0).
void rf_symbols_to_durations(const rmt_symbol_word_t *symbols, size_t count, std::vector<int> &out);

// Try to decode an OOK frame from `durations` using the protocol registry.
// On success fills out.key, out.Bit, out.te, out.protocol (registry name) and
// out.preset (radio preset name) and returns true. Other fields are untouched.
bool rf_decode_ook(const std::vector<int> &durations, RfCodes &out);

// Try to decode a KeeLoq frame (dedicated PWM state machine: header + sync gap +
// 64 PWM bits). On success sets out.key (raw 64-bit), out.Bit=64, out.te,
// out.protocol="KeeLoq", out.preset and returns true. The caller still splits
// out.key into fix/encrypted and runs keeloq_identify.
bool rf_decode_keeloq(const std::vector<int> &durations, RfCodes &out);

// Build the RAW representation from `durations`:
//  - `dataOut`  : "+a -b +c ..." string of signed durations.
//  - `teOut`    : first positive duration (base pulse estimate).
//  - when a repeated pattern is detected, sets `hasCrc=true` and fills
//    `crcOut` (CRC-64 of the pulse-index sequence), `indexedOut` (distinct
//    pulse lengths) and `bitsOut` (number of indexed transitions).
// Returns the number of transitions written to `dataOut`.
int rf_build_raw(
    const std::vector<int> &durations, String &dataOut, bool &hasCrc, uint64_t &crcOut,
    std::vector<int> &indexedOut, int &bitsOut, int &teOut
);
