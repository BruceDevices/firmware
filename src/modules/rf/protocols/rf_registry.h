#pragma once

#include "rf_protocol.h"

// Static OOK protocol registry. Lookup + iteration used by the decoder (M2)
// and the replay dispatch (M3).

// Find a protocol definition by name. Flipper Zero protocol names (as written in
// standard `.sub` files, e.g. "Nice FLO", "Holtek_HT12X", "Phoenix_V2") are
// accepted and resolved to the canonical registry entry. Returns nullptr if none.
const RfProtocolDef *rf_find_protocol(const String &name);

// Map a canonical registry name back to the Flipper protocol name (for writing
// Flipper-standard `.sub` files). Returns the name unchanged when it is already
// Flipper-compatible.
String rf_flipper_protocol_name(const String &canonical);

// Resolve a classic RCSwitch protocol NUMBER (as still carried by the Serial
// CLI `subghz tx` and the `RfSend` JSON API) to a registry definition. This is
// part of the permanent TX motor (not legacy `.sub` migration): the numeric
// transmit contract is external and must survive removal of the legacy module.
// Falls back to RcSwitch_1 for unknown numbers; never returns nullptr.
const RfProtocolDef *rf_protocol_for_number(int proto_no);

// Iteration helpers (e.g. for the generic decoder to try every protocol).
const RfProtocolDef *rf_protocol_at(int index);
int rf_protocol_count();
