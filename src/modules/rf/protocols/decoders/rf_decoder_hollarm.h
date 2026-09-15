#pragma once
#include <vector>
#include "../rf_protocol.h"

bool rf_decode_hollarm(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_hollarm(const RfCodes& in, std::vector<int>& out);
