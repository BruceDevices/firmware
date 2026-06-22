#include "rf_registry.h"

// Canonical static OOK protocol table. Timings use the RCSwitch factor model
// ({high,low} multiples of `te` µs). The numbered "RcSwitch_N" entries mirror
// the classic RCSwitch protocol table (so legacy numeric presets still
// resolve); the named entries are the human-facing protocol identities
// written to `Protocol:` in `.sub` files.
//
// Sources: classic RCSwitch protocol table (proto 1..22) and the brute-force
// timing table previously kept in rf_bruteforce.h.
#define SYNC RF_PF_HAS_SYNC
#define FIXED RF_PF_FIXED_LEN

static const RfProtocolDef rf_protocols[] = {
    // ---- Named protocols (canonical identities) --------------------------
    // name           te    sync       zero      one       bits inv  flags
    {"Princeton",     350, {1, 31},   {1, 3},   {3, 1},   24, false, SYNC},           // RCSwitch proto 1
    {"CAME",          250, {1, 3},    {2, 1},   {1, 2},   12, false, SYNC | FIXED},   // RCSwitch proto 20
    {"NICE_FLO",      700, {1, 36},   {2, 1},   {1, 2},   12, false, SYNC | FIXED},   // RCSwitch proto 22
    {"Holtek_HT12",   450, {23, 1},   {1, 2},   {2, 1},   12, true,  SYNC | FIXED},   // RCSwitch proto 6 (HT6P20B)
    {"Ansonic",       555, {1, 35},   {1, 2},   {2, 1},   12, false, SYNC | FIXED},   // brute "Ansonic 12bit"

    // ---- Generic numbered protocols (classic RCSwitch proto 1..12) -------
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

const RfProtocolDef *rf_find_protocol(const String &name) {
    for (const auto &p : rf_protocols) {
        if (name == p.name) return &p;
    }
    return nullptr;
}

const RfProtocolDef *rf_protocol_at(int index) {
    if (index < 0 || index >= rf_protocols_count) return nullptr;
    return &rf_protocols[index];
}

int rf_protocol_count() { return rf_protocols_count; }
