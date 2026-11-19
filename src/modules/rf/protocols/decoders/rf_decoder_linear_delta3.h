#pragma once

#include <vector>
#include "../rf_protocol.h"

bool rf_decode_linear_delta3(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_linear_delta3(const RfCodes& in, std::vector<int>& out);
