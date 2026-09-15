#pragma once
#include <vector>
#include "../rf_protocol.h"

bool rf_decode_dickert_mahs(const std::vector<int>& durations, RfCodes& out);
bool rf_encode_dickert_mahs(const RfCodes& in, std::vector<int>& out);
