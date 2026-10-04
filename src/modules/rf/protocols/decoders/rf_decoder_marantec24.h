#pragma once
#include <vector>
#include "../rf_protocol.h"

bool rf_decode_marantec24(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_marantec24(const RfCodes& in, std::vector<int>& out);
