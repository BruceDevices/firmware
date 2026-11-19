#pragma once
#include <vector>
#include "../rf_protocol.h"

bool rf_decode_hay21(const std::vector<int>& durations, RfCodes& out);
