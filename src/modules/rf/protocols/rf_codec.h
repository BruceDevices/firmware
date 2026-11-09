// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "rf_protocol.h"

// Hardware-free beta codecs, used by RF RX/TX and the shared protocol facade.
bool rf_decode_ook(const std::vector<int> &durations, RfCodes &out, bool skipStream = false);
bool rf_decode_keeloq(const std::vector<int> &durations, RfCodes &out);
bool rf_encode_protocol(
    uint64_t data, unsigned int bits, int te, const RfProtocolDef *def, int repeat, std::vector<int> &out
);
bool rf_keeloq_durations(uint64_t key, std::vector<int> &out);
