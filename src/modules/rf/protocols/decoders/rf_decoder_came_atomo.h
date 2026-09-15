#pragma once

#include <vector>
#include "../rf_protocol.h"

bool rf_decode_came_atomo(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_came_atomo(const RfCodes& in, std::vector<int>& out);
