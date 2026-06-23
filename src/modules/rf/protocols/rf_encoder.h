// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). This file contains code DERIVED FROM and
// modified after:
//   - rc-switch (LGPL-2.1-or-later), (C) 2011 Suat Ozgur and contributors —
//     the classic OOK protocol send state machine;
//   - Flipper Zero firmware (GPL-3.0-or-later), (C) Flipper Devices Inc. and
//     contributors — the KeeLoq OOK framing.
// See THIRD_PARTY.md for full attribution.
#pragma once

#include "../structs.h"
#include "rf_protocol.h"

// ---------------------------------------------------------------------------
// Native RMT TX motor.
//
// Replaces the former library / bit-bang transmit paths. Every send goes
// through rf_tx_durations(): a list of signed microsecond timings (sign = logic
// level, + HIGH / - LOW) is packed into rmt_symbol_word_t[] and streamed out on
// the configured RF TX pin (CC1101 GDO0/io0, or rfTx on single-pinned modules).
// The radio (CC1101 modulation / PA / SetTx) is configured by the caller via
// initRfModule()/sendRfCommand(); this module only generates the pulse train.
// ---------------------------------------------------------------------------

// Transmit a sequence of signed µs durations once. Blocks until the RMT
// hardware reports the whole frame done; leaves the line idle LOW.
bool rf_tx_durations(const std::vector<int> &durations);

// Build (without transmitting) the signed-µs duration list for `data`
// (`bits` long, MSB first) under protocol `def`, repeated `repeat` times.
// `te` overrides def->te when > 0. Pure / hardware-free: used both by
// rf_tx_protocol and by the encoder self-test. Returns false on bad input.
bool rf_encode_protocol(
    uint64_t data, unsigned int bits, int te, const RfProtocolDef *def, int repeat,
    std::vector<int> &out
);

// Encode `data` (`bits` long, MSB first) using protocol `def`, repeated
// `repeat` times, and transmit via RMT. `te` overrides def->te when > 0.
bool rf_tx_protocol(uint64_t data, unsigned int bits, int te, const RfProtocolDef *def, int repeat);

// Golden self-test: encode known inputs for the reference-derived protocols and
// compare the produced durations against expected absolute timings. Prints a
// PASS/FAIL line per protocol. Returns true iff all pass. Compiled only under
// RF_DEBUG (diagnostic tool, see `subghz selftest`).
bool rf_encoder_selftest();

// KeeLoq has its own framing (12-pulse header, sync gap, 64 PWM bits, trailing
// gap) that does not fit the registry's factor-based OOK model, so it gets a dedicated
// encoder. `rf_keeloq_durations` builds the signed-µs train for the 64-bit
// `key` (MSB first); `rf_tx_keeloq` repeats and transmits it. Pure builder is
// exposed for testing.
bool rf_keeloq_durations(uint64_t key, std::vector<int> &out);
bool rf_tx_keeloq(uint64_t key, int repeat);

// Transmit a 0-terminated RAW timings array (signed µs) via RMT (1 repetition,
// matching the legacy RAW behaviour).
bool rf_tx_raw_timings(const int *timings);

// Transmit a bit string: each '1'/'0' char is held at the matching logic level
// for `te` µs (other chars are skipped, as in the legacy bit sender).
bool rf_tx_raw_bits(const String &bits, int te);
