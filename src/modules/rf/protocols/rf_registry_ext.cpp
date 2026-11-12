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
#include "decoders/rf_decoder_marantec.h"
#include "decoders/rf_decoder_smc5326.h"
#include "decoders/rf_decoder_honeywell_wdb.h"
#include "decoders/rf_decoder_linear_delta3.h"
// Phase 3 — Rolling code decoders
#include "decoders/rf_decoder_secplus_v1.h"
#include "decoders/rf_decoder_secplus_v2.h"
#include "decoders/rf_decoder_somfy_telis.h"
#include "decoders/rf_decoder_somfy_keytis.h"
#include "decoders/rf_decoder_hormann.h"
#include "decoders/rf_decoder_came_twee.h"
#include "decoders/rf_decoder_came_atomo.h"
#include "decoders/rf_decoder_nice_flor_s.h"
#include "decoders/rf_decoder_ido.h"
#include "decoders/rf_decoder_nero_radio.h"
#include "decoders/rf_decoder_nero_sketch.h"
// Phase 2 — Additional simple OOK decoders
#include "decoders/rf_decoder_bett.h"
#include "decoders/rf_decoder_feron.h"
#include "decoders/rf_decoder_roger.h"
#include "decoders/rf_decoder_elplast.h"
// Phase 4 — TPMS decoders
#include "decoders/rf_decoder_schrader_tpms.h"
#include "decoders/rf_decoder_ford_tpms.h"
#include "decoders/rf_decoder_renault_tpms.h"
#include "decoders/rf_decoder_citroen_tpms.h"
#include "decoders/rf_decoder_toyota_tpms.h"
// Phase 5 — Flipper ported OOK decoders
#include "decoders/rf_decoder_doitrand.h"
#include "decoders/rf_decoder_magellan.h"
#include "decoders/rf_decoder_dooya.h"
#include "decoders/rf_decoder_legrand.h"
#include "decoders/rf_decoder_chamb_code.h"
#include "decoders/rf_decoder_marantec24.h"
#include "decoders/rf_decoder_hollarm.h"
#include "decoders/rf_decoder_hay21.h"
#include "decoders/rf_decoder_gangqi.h"
#include "decoders/rf_decoder_dickert_mahs.h"
#include "decoders/rf_decoder_revers_rb2.h"
#include "decoders/rf_decoder_megacode.h"

#if !defined(LITE_VERSION)
#include "decoders/rf_vehicle_protocols.h"
#endif

#define DECODE RF_PF_HAS_DECODER

static const RfProtocolDef rf_ext_protocols[] = {
#if !defined(LITE_VERSION)
    // Specific vehicle framing is tested before broad generic matches.
    rf_stream_definition<&chrysler_protocol_v0>("Chrysler V0"),
    rf_stream_definition<&fiat_protocol_v0>("Fiat V0"),
    rf_stream_definition<&fiat_v1_protocol>("Fiat V1"),
    rf_stream_definition<&fiat_v2_protocol>("Fiat V2"),
    rf_stream_definition<&ford_protocol_v0>("Ford V0"),
    rf_stream_definition<&ford_protocol_v1>("Ford V1"),
    rf_stream_definition<&ford_protocol_v2>("Ford V2"),
    rf_stream_definition<&ford_protocol_v3>("Ford V3"),
    rf_stream_definition<&honda_static_protocol>("Honda Static"),
    rf_stream_definition<&honda_v1_protocol>("Honda V1"),
    rf_stream_definition<&honda_v2_protocol>("Honda V2"),
    rf_stream_definition<&kia_protocol_v0>("Kia V0"),
    rf_stream_definition<&kia_protocol_v1>("Kia V1"),
    rf_stream_definition<&kia_protocol_v2>("Kia V2"),
    rf_stream_definition<&kia_protocol_v3_v4>("Kia V3/V4"),
    rf_stream_definition<&kia_protocol_v5>("Kia V5"),
    rf_stream_definition<&kia_protocol_v6>("Kia V6"),
    rf_stream_definition<&kia_protocol_v7>("Kia V7"),
    rf_stream_definition<&mazda_v0_protocol>("Mazda V0"),
    rf_stream_definition<&psa_protocol>("PSA"),
    rf_stream_definition<&renault_v0_protocol>("Renault V0"),
    rf_stream_definition<&renault_v1_protocol>("Renault V1"),
    rf_stream_definition<&rf_stream_protocol_scher_khan>("Scher-Khan"),
    rf_stream_definition<&rf_stream_protocol_star_line>("Star Line"),
    rf_stream_definition<&subaru_protocol>("Subaru"),
    rf_stream_definition<&vag_protocol>("VAG"),
#endif

    // name                   te    sync    zero     one     bits  inv  flags                decode                           encode
    {"FAAC_SLH",              255,  {0, 0}, {0, 0},  {0, 0}, 64,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_faac_slh,           rf_encode_faac_slh},
    {"PowerSmart",            225,  {0, 0}, {0, 0},  {0, 0}, 64,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_power_smart,        rf_encode_power_smart},
    {"Intertechno_V3",        275,  {0, 0}, {0, 0},  {0, 0}, 32,   false, DECODE,
     rf_decode_intertechno_v3,     rf_encode_intertechno_v3},
    {"Marantec",              1000, {0, 0}, {0, 0},  {0, 0}, 49,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_marantec,           rf_encode_marantec},
    {"SMC5326",               300,  {0, 0}, {0, 0},  {0, 0}, 25,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_smc5326,            rf_encode_smc5326},
    {"Honeywell_WDB",         160,  {0, 0}, {0, 0},  {0, 0}, 48,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_honeywell_wdb,      rf_encode_honeywell_wdb},
    {"Linear_Delta3",         500,  {0, 0}, {0, 0},  {0, 0}, 8,    false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_linear_delta3,      rf_encode_linear_delta3},
    // Phase 3 — Rolling code decoders
    {"SecPlus_v1",            500,  {0, 0}, {0, 0},  {0, 0}, 42,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_secplus_v1,         rf_encode_secplus_v1},
    {"SecPlus_v2",            250,  {0, 0}, {0, 0},  {0, 0}, 62,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_secplus_v2,         nullptr},
    {"Somfy_Telis",           640,  {0, 0}, {0, 0},  {0, 0}, 56,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_somfy_telis,        nullptr},
    {"Somfy_Keytis",          640,  {0, 0}, {0, 0},  {0, 0}, 80,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_somfy_keytis,       nullptr},
    {"Hormann_HSM",           500,  {0, 0}, {0, 0},  {0, 0}, 44,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_hormann,            rf_encode_hormann},
    {"CAME_Twee",             500,  {0, 0}, {0, 0},  {0, 0}, 54,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_came_twee,          rf_encode_came_twee},
    {"CAME_Atomo",            600,  {0, 0}, {0, 0},  {0, 0}, 62,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_came_atomo,         nullptr},
    {"Nice_Flor_S",           500,  {0, 0}, {0, 0},  {0, 0}, 52,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_nice_flor_s,        nullptr},
    {"IDO",                   450,  {0, 0}, {0, 0},  {0, 0}, 48,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_ido,                rf_encode_ido},
    {"Nero_Radio",            200,  {0, 0}, {0, 0},  {0, 0}, 56,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_nero_radio,         rf_encode_nero_radio},
    {"Nero_Sketch",           330,  {0, 0}, {0, 0},  {0, 0}, 40,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_nero_sketch,        rf_encode_nero_sketch},
    // Phase 2 — Additional simple OOK decoders
    {"Bett",                  340,  {0, 0}, {0, 0},  {0, 0}, 18,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_bett,               rf_encode_bett},
    {"Feron",                 350,  {0, 0}, {0, 0},  {0, 0}, 32,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_feron,              rf_encode_feron},
    {"Roger",                 500,  {0, 0}, {0, 0},  {0, 0}, 28,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_roger,              rf_encode_roger},
    {"Elplast",               230,  {0, 0}, {0, 0},  {0, 0}, 18,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_elplast,            rf_encode_elplast},
    // Phase 4 — TPMS decoders
    {"Schrader_TPMS",         120,  {0, 0}, {0, 0},  {0, 0}, 64,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_schrader_tpms,      nullptr},
    {"Ford_TPMS",             52,   {0, 0}, {0, 0},  {0, 0}, 64,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_ford_tpms,          nullptr},
    {"Renault_TPMS",          48,   {0, 0}, {0, 0},  {0, 0}, 72,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_renault_tpms,       nullptr},
    {"Citroen_TPMS",          52,   {0, 0}, {0, 0},  {0, 0}, 80,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_citroen_tpms,       nullptr},
    {"Toyota_TPMS",           48,   {0, 0}, {0, 0},  {0, 0}, 72,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_toyota_tpms,        nullptr},
    // Phase 5 — Flipper ported OOK decoders
    {"Doitrand",              400,  {0, 0}, {0, 0},  {0, 0}, 37,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_doitrand,           rf_encode_doitrand},
    {"Magellan",              200,  {0, 0}, {0, 0},  {0, 0}, 32,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_magellan,           rf_encode_magellan},
    {"Dooya",                 366,  {0, 0}, {0, 0},  {0, 0}, 40,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_dooya,              rf_encode_dooya},
    {"Legrand",               375,  {0, 0}, {0, 0},  {0, 0}, 18,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_legrand,            nullptr},
    {"Chamb_Code",            1000, {0, 0}, {0, 0},  {0, 0}, 10,   false, DECODE,
     rf_decode_chamb_code,         nullptr},
    {"Marantec24",            800,  {0, 0}, {0, 0},  {0, 0}, 24,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_marantec24,         rf_encode_marantec24},
    {"Hollarm",               200,  {0, 0}, {0, 0},  {0, 0}, 42,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_hollarm,            rf_encode_hollarm},
    {"Hay21",                 300,  {0, 0}, {0, 0},  {0, 0}, 21,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_hay21,              nullptr},
    {"GangQi",                500,  {0, 0}, {0, 0},  {0, 0}, 34,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_gangqi,             rf_encode_gangqi},
    {"Dickert_MAHS",          400,  {0, 0}, {0, 0},  {0, 0}, 36,   false, DECODE,
     rf_decode_dickert_mahs,       rf_encode_dickert_mahs},
     {"Revers_RB2",            250,  {0, 0}, {0, 0},  {0, 0}, 64,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_revers_rb2,         nullptr},
    {"MegaCode",              1000, {0, 0}, {0, 0},  {0, 0}, 24,   false, DECODE | RF_PF_FIXED_LEN,
     rf_decode_megacode,           rf_encode_megacode},
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
