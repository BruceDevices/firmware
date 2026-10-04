#pragma once

#include <vector>
#include "../rf_protocol.h"

bool rf_decode_honeywell_wdb(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_honeywell_wdb(const RfCodes& in, std::vector<int>& out);
