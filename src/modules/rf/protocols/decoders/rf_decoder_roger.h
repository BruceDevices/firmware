#pragma once
#include <vector>
#include "../rf_protocol.h"

bool rf_decode_roger(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_roger(const RfCodes& in, std::vector<int>& out);
