// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Part of Bruce (AGPL-3.0-or-later). The OOK protocol timing table is DERIVED
// FROM and modified after rc-switch (LGPL-2.1-or-later), (C) 2011 Suat Ozgur
// and contributors; the Flipper-name mapping mirrors the Flipper Zero firmware
// (GPL-3.0-or-later). See THIRD_PARTY.md for full attribution.
#include "rf_registry.h"

// Canonical static OOK protocol table. Timings use the classic factor model
// ({high,low} multiples of `te` µs). The numbered "RcSwitch_N" entries mirror
// the classic numeric protocol table (so legacy numeric presets still
// resolve); the named entries are the human-facing protocol identities
// written to `Protocol:` in `.sub` files.
//
// Sources: classic numeric protocol table (proto 1..22) and the brute-force
// timing table previously kept in rf_bruteforce.h.
#define SYNC RF_PF_HAS_SYNC
#define FIXED RF_PF_FIXED_LEN

static const RfProtocolDef rf_protocols[] = {
    // ---- Named protocols (canonical identities) --------------------------
    // Two structural families share this table (same {high,low}-factor model):
    //   * high-first  (inv=false): pulse is HIGH for high*te then LOW for low*te;
    //     the sync is a short HIGH followed by the long inter-frame LOW gap.
    //   * space-coded (inv=true):  pulse is LOW  for high*te then HIGH for low*te;
    //     the sync is the long LOW gap followed by a short HIGH preamble. For
    //     these `sync` is stored as {gap_factor, preamble_factor}.
    // `bits` + FIXED is the per-protocol payload length; the decoder rejects a
    // frame whose decoded length differs, which disambiguates same-timing codes.
    // name           te    sync       zero      one       bits inv  flags
    {"Princeton",     350, {1, 31},   {1, 3},   {3, 1},   24, false, SYNC},           // legacy proto 1
    {"NICE_FLO",      700, {1, 36},   {2, 1},   {1, 2},   12, false, SYNC | FIXED},   // legacy proto 22
    {"Linear",        500, {3, 42},   {1, 3},   {3, 1},   10, false, SYNC | FIXED},   // 10-bit DIP
    {"Clemsa",        385, {7, 50},   {1, 7},   {7, 1},   18, false, SYNC | FIXED},
    {"Mastercode",   1072, {2, 14},   {1, 2},   {2, 1},   36, false, SYNC | FIXED},
    {"CAME",          320, {36, 1},   {2, 1},   {1, 2},   12, true,  SYNC | FIXED},   // legacy proto 20 (space-coded)
    {"Ansonic",       555, {35, 1},   {1, 2},   {2, 1},   12, true,  SYNC | FIXED},
    {"GateTX",        350, {49, 2},   {1, 2},   {2, 1},   24, true,  SYNC | FIXED},
    {"Holtek",        430, {36, 1},   {1, 2},   {2, 1},   40, true,  SYNC | FIXED},   // HT6Pxx 40-bit
    {"Holtek_HT12",   450, {23, 1},   {1, 2},   {2, 1},   12, true,  SYNC | FIXED},   // legacy proto 6 (HT6P20B)
    {"PhoenixV2",     427, {60, 6},   {1, 2},   {2, 1},   52, true,  SYNC | FIXED},

    // ---- Generic numbered protocols (classic legacy proto 1..12) -------
    {"RcSwitch_1",    350, {1, 31},   {1, 3},   {3, 1},   0,  false, SYNC},
    {"RcSwitch_2",    650, {1, 10},   {1, 2},   {2, 1},   0,  false, SYNC},
    {"RcSwitch_3",    100, {30, 71},  {4, 11},  {9, 6},   0,  false, SYNC},
    {"RcSwitch_4",    380, {1, 6},    {1, 3},   {3, 1},   0,  false, SYNC},
    {"RcSwitch_5",    500, {6, 14},   {1, 2},   {2, 1},   0,  false, SYNC},
    {"RcSwitch_6",    450, {23, 1},   {1, 2},   {2, 1},   0,  true,  SYNC},
    {"RcSwitch_7",    150, {2, 62},   {1, 6},   {6, 1},   0,  false, SYNC},
    {"RcSwitch_8",    200, {3, 130},  {7, 16},  {3, 16},  0,  false, SYNC},
    {"RcSwitch_9",    200, {130, 7},  {16, 7},  {16, 3},  0,  true,  SYNC},
    {"RcSwitch_10",   365, {18, 1},   {3, 1},   {1, 3},   0,  true,  SYNC},
    {"RcSwitch_11",   270, {36, 1},   {1, 2},   {2, 1},   0,  true,  SYNC},
    {"RcSwitch_12",   320, {36, 1},   {1, 2},   {2, 1},   0,  true,  SYNC},
};

#undef SYNC
#undef FIXED

static const int rf_protocols_count = sizeof(rf_protocols) / sizeof(rf_protocols[0]);

// Flipper Zero protocol name <-> Bruce canonical registry name. Only the entries
// that differ in spelling are listed; names that already match (Princeton, CAME,
// Linear, Clemsa, Mastercode, Ansonic, GateTX, Holtek, KeeLoq...) need no alias.
struct RfProtoAlias {
    const char *flipper;
    const char *canonical;
};
static const RfProtoAlias rf_proto_aliases[] = {
    {"Nice FLO", "NICE_FLO"},
    {"Holtek_HT12X", "Holtek_HT12"},
    {"Phoenix_V2", "PhoenixV2"},
};

const RfProtocolDef *rf_find_protocol(const String &name) {
    // Resolve a Flipper protocol name to the canonical one first.
    String wanted = name;
    for (const auto &a : rf_proto_aliases) {
        if (name == a.flipper) {
            wanted = a.canonical;
            break;
        }
    }
    for (const auto &p : rf_protocols) {
        if (wanted == p.name) return &p;
    }
    return nullptr;
}

String rf_flipper_protocol_name(const String &canonical) {
    for (const auto &a : rf_proto_aliases) {
        if (canonical == a.canonical) return a.flipper;
    }
    return canonical;
}

const RfProtocolDef *rf_protocol_for_number(int proto_no) {
    // Classic legacy numbers that map to a NAMED registry identity.
    switch (proto_no) {
        case 20: return rf_find_protocol("CAME");
        case 22: return rf_find_protocol("NICE_FLO");
        default: break;
    }
    // Numbers 1..12 mirror the generic RcSwitch_N entries.
    const RfProtocolDef *p = rf_find_protocol(String("RcSwitch_") + String(proto_no));
    if (p) return p;
    return rf_find_protocol("RcSwitch_1"); // safe default (never null)
}

const RfProtocolDef *rf_protocol_at(int index) {
    if (index < 0 || index >= rf_protocols_count) return nullptr;
    return &rf_protocols[index];
}

int rf_protocol_count() { return rf_protocols_count; }
