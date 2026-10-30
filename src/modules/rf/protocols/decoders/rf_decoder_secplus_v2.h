#pragma once

#include <vector>
#include "../rf_protocol.h"

bool rf_decode_secplus_v2(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_secplus_v2(const RfCodes& in, std::vector<int>& out);
