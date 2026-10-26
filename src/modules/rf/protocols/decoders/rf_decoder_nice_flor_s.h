#pragma once

#include <vector>
#include "../rf_protocol.h"

bool rf_decode_nice_flor_s(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_nice_flor_s(const RfCodes& in, std::vector<int>& out);
