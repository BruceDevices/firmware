#pragma once

#include <vector>
#include "../rf_protocol.h"

bool rf_decode_nero_sketch(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_nero_sketch(const RfCodes& in, std::vector<int>& out);
