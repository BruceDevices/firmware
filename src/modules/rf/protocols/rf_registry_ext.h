#pragma once

#include "rf_protocol.h"

// Extended protocol registry for protocols that do NOT fit the classic
// factor-based OOK model (e.g. rolling code, Manchester, FSK). These
// entries carry decode/encode callbacks instead of factor timings.
//
// Accessors mirror the factor-based registry so the decoder can iterate
// all known protocols transparently.

// Total number of extended protocol entries.
int rf_protocol_ext_count();

// Look up an extended protocol by index (0 .. count-1).
const RfProtocolDef *rf_protocol_ext_at(int index);

// Find an extended protocol by name (canonical). Returns nullptr if none.
const RfProtocolDef *rf_find_ext_protocol(const String &name);
