#pragma once
#include <vector>
#include "../rf_protocol.h"

bool rf_decode_gangqi(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_gangqi(const RfCodes& in, std::vector<int>& out);
