#pragma once
#include <vector>
#include "../rf_protocol.h"

bool rf_decode_schrader_tpms(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_schrader_tpms(const RfCodes& in, std::vector<int>& out);
