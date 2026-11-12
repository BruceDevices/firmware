// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "rf_protocol.h"

// Shared native/JS protocol facade. JSON keeps the existing JS API v1 contract;
// serialized .sub fields remain authoritative for extended vehicle variants.
String rf_protocol_catalog_json();
String rf_protocol_decode_json(const char *subfile);
String rf_protocol_encode_json(const char *subfile);
bool rf_protocol_decode_capture(
    const std::vector<int> &durations, uint32_t frequency, const String &preset, RfCodes &out
);
bool rf_protocol_load(const char *subfile, RfCodes &out);
String rf_protocol_serialize(const RfCodes &code);
bool rf_protocol_encode(const RfCodes &code, int repeat, std::vector<int> &durations);
bool rf_protocol_has_encoder(const String &name);

// Hardware boundary, shared with subghz.protoTransmit. Only explicit callers
// transmit; parsing, catalogue lookup and encoding never touch the radio.
bool rf_protocol_transmit_raw(const char *subfile);
