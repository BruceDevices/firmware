#pragma once
#include <vector>
#include "../rf_protocol.h"

bool rf_decode_dooya(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_dooya(const RfCodes& in, std::vector<int>& out);
