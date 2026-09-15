#pragma once

#include <vector>
#include "../rf_protocol.h"

bool rf_decode_came_twee(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_came_twee(const RfCodes& in, std::vector<int>& out);
