#pragma once

#include <vector>
#include "../rf_protocol.h"

// PowerSmart (Anderson/Novar) decoder callback.
// Manchester-encoded 64-bit protocol, te=225µs.
bool rf_decode_power_smart(const std::vector<int>& durations, RfCodes& out);

bool rf_encode_power_smart(const RfCodes& in, std::vector<int>& out);
