#pragma once

#include <vector>
#include "../rf_protocol.h"

// Intertechno V3 (home automation) decoder callback.
// Pulse-train OOK protocol, 32 or 36 bits, te=275µs.
bool rf_decode_intertechno_v3(const std::vector<int>& durations, RfCodes& out);

bool rf_encode_intertechno_v3(const RfCodes& in, std::vector<int>& out);
