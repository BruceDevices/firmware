#pragma once
#include <vector>
#include "../rf_protocol.h"

bool rf_decode_magellan(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_magellan(const RfCodes& in, std::vector<int>& out);
