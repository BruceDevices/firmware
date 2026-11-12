#pragma once

#include <vector>
#include "../rf_protocol.h"

// FAAC SLH (rolling code) protocol decoder callback.
// Decodes the 64-bit FAAC SLH frame: preamble (2×te_long HIGH + LOW), then
// 64 PWM bits (short+long=0, long+short=1).
bool rf_decode_faac_slh(const std::vector<int>& durations, RfCodes& out);

// FAAC SLH encoder callback.
bool rf_encode_faac_slh(const RfCodes& in, std::vector<int>& out);
