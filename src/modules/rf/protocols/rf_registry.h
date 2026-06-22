#pragma once

#include "rf_protocol.h"

// Static OOK protocol registry. Lookup + iteration used by the decoder (M2)
// and the replay dispatch (M3).

// Find a protocol definition by its canonical name. Returns nullptr if none.
const RfProtocolDef *rf_find_protocol(const String &name);

// Iteration helpers (e.g. for the generic decoder to try every protocol).
const RfProtocolDef *rf_protocol_at(int index);
int rf_protocol_count();
