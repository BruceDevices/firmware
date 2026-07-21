// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). Extended protocol registry for
// callback-based decoders that do not fit the classic factor-based OOK model
// (e.g. Manchester, rolling code, pulse-train, FSK). Protocol timing
// definitions are DERIVED FROM the Flipper Zero firmware (GPL-3.0-or-later).
// See THIRD_PARTY.md for full attribution.
#include "rf_registry_ext.h"
#include "decoders/rf_decoder_faac_slh.h"
#include "decoders/rf_decoder_power_smart.h"
#include "decoders/rf_decoder_intertechno_v3.h"

#define DECODE RF_PF_HAS_DECODER

static const RfProtocolDef rf_ext_protocols[] = {
    // name                   te    sync    zero     one     bits  inv  flags                decode                           encode
    {"FAAC_SLH",              255,  {0, 0}, {0, 0},  {0, 0}, 64,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_faac_slh,           rf_encode_faac_slh},
    {"PowerSmart",            225,  {0, 0}, {0, 0},  {0, 0}, 64,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_power_smart,        rf_encode_power_smart},
    {"Intertechno_V3",        275,  {0, 0}, {0, 0},  {0, 0}, 32,   false, DECODE,
     rf_decode_intertechno_v3,     rf_encode_intertechno_v3},
};

#undef DECODE

static const int rf_ext_count = sizeof(rf_ext_protocols) / sizeof(rf_ext_protocols[0]);

int rf_protocol_ext_count() { return rf_ext_count; }

const RfProtocolDef *rf_protocol_ext_at(int index) {
    if (index < 0 || index >= rf_ext_count) return nullptr;
    return &rf_ext_protocols[index];
}

const RfProtocolDef *rf_find_ext_protocol(const String &name) {
    for (int i = 0; i < rf_ext_count; i++) {
        if (name == rf_ext_protocols[i].name) return &rf_ext_protocols[i];
    }
    return nullptr;
}
