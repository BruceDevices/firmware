#pragma once

#include <vector>
#include "../rf_protocol.h"

bool rf_decode_somfy_keytis(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_somfy_keytis(const RfCodes& in, std::vector<int>& out);
