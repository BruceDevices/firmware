#include "rolling_code_db.h"
#include <string.h>

// ===========================================================================
// Manufacturer key table.
//
// Names and cipher types are accurate (sourced from Momentum's keeloq
// manufacturer list, which is not secret). The 64-bit `key` values are the
// proprietary secrets and are left 0 here — drop your own material in, or ship
// an SD `/mfcodes` file which KeeloqKeystore merges on top of this table.
// ===========================================================================
const RollingMfKey rolling_mf_keys[] = {
    // -- Simple-learning KeeLoq manufacturers ------------------------------
    {"DoorHan",            0x0, CIPHER_KEELOQ_SIMPLE},
    {"AN-Motors",          0x0, CIPHER_KEELOQ_SIMPLE},
    {"HCS101",             0x0, CIPHER_KEELOQ_SIMPLE},
    {"Came_Atomo",         0x0, CIPHER_KEELOQ_SIMPLE},
    // -- Normal-learning KeeLoq manufacturers ------------------------------
    {"Beninca",            0x0, CIPHER_KEELOQ_NORMAL},
    {"Allmatic",           0x0, CIPHER_KEELOQ_NORMAL},
    {"Centurion",          0x0, CIPHER_KEELOQ_NORMAL},
    {"Hormann",            0x0, CIPHER_KEELOQ_NORMAL},
    {"Sommer",             0x0, CIPHER_KEELOQ_NORMAL},
    {"NICE_Smilo",         0x0, CIPHER_KEELOQ_NORMAL},
    {"NICE_MHOUSE",        0x0, CIPHER_KEELOQ_NORMAL},
    {"Aprimatic",          0x0, CIPHER_KEELOQ_NORMAL},
    {"DTM_Neo",            0x0, CIPHER_KEELOQ_NORMAL},
    {"FAAC_RC,XT",         0x0, CIPHER_KEELOQ_NORMAL},
    {"Mutanco_Mutancode",  0x0, CIPHER_KEELOQ_NORMAL},
    {"GSN",                0x0, CIPHER_KEELOQ_NORMAL},
    {"Dea_Mio",            0x0, CIPHER_KEELOQ_NORMAL},
    {"JCM_Tech",           0x0, CIPHER_KEELOQ_NORMAL},
    {"Genius_Bravo",       0x0, CIPHER_KEELOQ_NORMAL},
    {"Cardin_S449",        0x0, CIPHER_KEELOQ_NORMAL},
    {"Normstahl",          0x0, CIPHER_KEELOQ_NORMAL},
    {"Novoferm",           0x0, CIPHER_KEELOQ_NORMAL},
    {"Elmes",              0x0, CIPHER_KEELOQ_NORMAL},
    {"Gibidi",             0x0, CIPHER_KEELOQ_NORMAL},
    {"Motorline",          0x0, CIPHER_KEELOQ_NORMAL},
    {"Seav",               0x0, CIPHER_KEELOQ_NORMAL},
    {"Stilmatic",          0x0, CIPHER_KEELOQ_NORMAL},
    {"Wisniowski",         0x0, CIPHER_KEELOQ_NORMAL},
    {"ET_Blue",            0x0, CIPHER_KEELOQ_NORMAL},
    {"Genius_TX4RC",       0x0, CIPHER_KEELOQ_NORMAL},
    {"Comunello",          0x0, CIPHER_KEELOQ_NORMAL},
    {"Mc_Garcia",          0x0, CIPHER_KEELOQ_NORMAL},
    {"Verex",              0x0, CIPHER_KEELOQ_NORMAL},
    {"Elvox",              0x0, CIPHER_KEELOQ_NORMAL},
    {"Fadini",             0x0, CIPHER_KEELOQ_NORMAL},
    {"DEA_Mio",            0x0, CIPHER_KEELOQ_NORMAL},
    {"IronLogic",          0x0, CIPHER_KEELOQ_NORMAL},
    // -- Secure / magic learning variants ----------------------------------
    {"Hormann_EcoStar",    0x0, CIPHER_KEELOQ_SECURE},
    {"Mhouse",             0x0, CIPHER_KEELOQ_MAGIC_SERIAL_T1},
    {"Pujol",              0x0, CIPHER_KEELOQ_PUJOL},
    {"AERF",               0x0, CIPHER_KEELOQ_AERF},
    {"Erreka",             0x0, CIPHER_KEELOQ_ERREKA},
    // -- Self-contained / table-keyed protocols ----------------------------
    {"FAAC_SLH",           0x0, CIPHER_KEELOQ_FAAC_SLH},
    {"Jarolift",           0x0, CIPHER_KEELOQ_JAROLIFT},
    {"Beninca_ARC",        0x0, CIPHER_AES128_ECB},
};
const size_t rolling_mf_keys_count = sizeof(rolling_mf_keys) / sizeof(rolling_mf_keys[0]);

bool rolling_mf_key_lookup(const char *name, uint64_t *out_key, uint8_t *out_type) {
    if (!name) return false;
    for (size_t i = 0; i < rolling_mf_keys_count; i++) {
        if (strcmp(rolling_mf_keys[i].name, name) == 0) {
            if (out_key) *out_key = rolling_mf_keys[i].key;
            if (out_type) *out_type = rolling_mf_keys[i].type;
            return true;
        }
    }
    return false;
}

// ===========================================================================
// Protocol registry — a single flat, alphabetical list of equal entries, the
// way Momentum's subghz_protocol_registry_items[] treats them. KeeLoq is one
// row among its peers (Nice FloR-S, Somfy, FAAC SLH, Security+, ...), not a
// category. For KeeLoq the specific manufacturer is chosen at runtime (its
// mf_key_name is null here) — exactly as the Flipper does, where KeeLoq is one
// protocol and the manufacturer comes from the keystore.
//
// Columns: display_name, family, freq, mf_key_name, has_seed, has_crc,
//          serial_bytes, button_bytes, counter_bytes, seed_bytes,
//          data_bit_count, te_short, te_long, bw_preset
// ===========================================================================
const RollingProtocol rolling_protocols[] = {
    {"Alutech AT-4N 433MHz",  RF_FAMILY_ALUTECH_AT4N,    433920000, nullptr,       false, true,  4, 1, 2, 0, 72, 400, 800,  BW_OOK_270},
    {"Beninca ARC 433MHz",    RF_FAMILY_BENINCA_ARC,     433920000, "Beninca_ARC", true,  false, 4, 1, 4, 2, 128, 300, 600, BW_OOK_270},
    {"FAAC SLH 433MHz",       RF_FAMILY_FAAC_SLH,        433920000, "FAAC_SLH",    true,  false, 4, 1, 4, 4, 64, 255, 595,  BW_OOK_270},
    {"FAAC SLH 868MHz",       RF_FAMILY_FAAC_SLH,        868350000, "FAAC_SLH",    true,  false, 4, 1, 4, 4, 64, 255, 595,  BW_OOK_270},
    {"Jarolift 433MHz",       RF_FAMILY_JAROLIFT,        433920000, "Jarolift",    true,  false, 4, 1, 2, 1, 72, 400, 800,  BW_OOK_270},
    {"KeeLoq 433MHz",         RF_FAMILY_KEELOQ,          433920000, nullptr,       false, false, 4, 1, 2, 0, 64, 400, 800,  BW_OOK_270},
    {"KingGates Stylo4k 433M",RF_FAMILY_NICE_FLOR_S,     433920000, nullptr,       false, false, 4, 1, 2, 0, 52, 500, 1000, BW_OOK_270},
    {"Nice FloR-S 433MHz",    RF_FAMILY_NICE_FLOR_S,     433920000, nullptr,       false, false, 4, 1, 2, 0, 52, 500, 1000, BW_OOK_270},
    {"Nice One 433MHz",       RF_FAMILY_NICE_ONE,        433920000, nullptr,       false, false, 4, 1, 2, 0, 72, 500, 1000, BW_OOK_270},
    {"Security+ 1.0 315MHz",  RF_FAMILY_SECURITY_PLUS_1, 315000000, nullptr,       false, false, 4, 0, 4, 0, 84, 500, 1500, BW_OOK_270},
    {"Security+ 1.0 390MHz",  RF_FAMILY_SECURITY_PLUS_1, 390000000, nullptr,       false, false, 4, 0, 4, 0, 84, 500, 1500, BW_OOK_270},
    {"Security+ 1.0 433MHz",  RF_FAMILY_SECURITY_PLUS_1, 433920000, nullptr,       false, false, 4, 0, 4, 0, 84, 500, 1500, BW_OOK_270},
    {"Security+ 2.0 310MHz",  RF_FAMILY_SECURITY_PLUS_2, 310000000, nullptr,       false, false, 4, 1, 2, 0, 64, 250, 500,  BW_OOK_270},
    {"Security+ 2.0 315MHz",  RF_FAMILY_SECURITY_PLUS_2, 315000000, nullptr,       false, false, 4, 1, 2, 0, 64, 250, 500,  BW_OOK_270},
    {"Security+ 2.0 390MHz",  RF_FAMILY_SECURITY_PLUS_2, 390000000, nullptr,       false, false, 4, 1, 2, 0, 64, 250, 500,  BW_OOK_270},
    {"Security+ 2.0 433MHz",  RF_FAMILY_SECURITY_PLUS_2, 433920000, nullptr,       false, false, 4, 1, 2, 0, 64, 250, 500,  BW_OOK_270},
    {"Somfy Keytis 433MHz",   RF_FAMILY_SOMFY_KEYTIS,    433420000, nullptr,       false, false, 4, 1, 2, 0, 80, 640, 1280, BW_OOK_650},
    {"Somfy Telis 433MHz",    RF_FAMILY_SOMFY_TELIS,     433420000, nullptr,       false, false, 4, 1, 2, 0, 56, 640, 1280, BW_OOK_650},
};
const size_t rolling_protocols_count = sizeof(rolling_protocols) / sizeof(rolling_protocols[0]);

bool rolling_mf_is_keeloq_manufacturer(uint8_t type) {
    switch (type) {
        case CIPHER_KEELOQ_SIMPLE:
        case CIPHER_KEELOQ_NORMAL:
        case CIPHER_KEELOQ_SECURE:
        case CIPHER_KEELOQ_MAGIC_XOR_TYPE1:
        case CIPHER_KEELOQ_MAGIC_SERIAL_T1:
        case CIPHER_KEELOQ_MAGIC_SERIAL_T2:
        case CIPHER_KEELOQ_MAGIC_SERIAL_T3:
        case CIPHER_KEELOQ_ERREKA:
        case CIPHER_KEELOQ_PUJOL:
        case CIPHER_KEELOQ_AERF: return true;
        default: return false; // FAAC SLH, Jarolift, Beninca ARC are own entries
    }
}

const RollingProtocol *rolling_protocol_by_name(const char *display_name) {
    if (!display_name) return nullptr;
    for (size_t i = 0; i < rolling_protocols_count; i++) {
        if (strcmp(rolling_protocols[i].display_name, display_name) == 0) return &rolling_protocols[i];
    }
    return nullptr;
}

const RollingProtocol *rolling_protocol_by_family(RollingFamily family) {
    for (size_t i = 0; i < rolling_protocols_count; i++) {
        if (rolling_protocols[i].family == family) return &rolling_protocols[i];
    }
    return nullptr;
}

// ===========================================================================
// Rainbow tables / AES key — loaded from SD at boot via rolling_code_db_load_sd().
// ===========================================================================
uint8_t nice_flor_s_rainbow[32] = {0};
uint8_t alutech_at4n_rainbow[32] = {0};
uint8_t beninca_arc_aes_key[16] = {0};

static void load_bin_file(FS *fs, const char *path, uint8_t *buf, size_t len) {
    File f = fs->open(path, "r");
    if (!f) return;
    f.read(buf, len);
    f.close();
}

void rolling_code_db_load_sd(FS *fs) {
    if (!fs) return;
    load_bin_file(fs, "/nice_flors.bin", nice_flor_s_rainbow,  sizeof(nice_flor_s_rainbow));
    load_bin_file(fs, "/alutech.bin",    alutech_at4n_rainbow,  sizeof(alutech_at4n_rainbow));
    load_bin_file(fs, "/beninca.bin",    beninca_arc_aes_key,   sizeof(beninca_arc_aes_key));
}

bool rolling_family_needs_key(RollingFamily family) {
    switch (family) {
        case RF_FAMILY_KEELOQ:
        case RF_FAMILY_FAAC_SLH:
        case RF_FAMILY_JAROLIFT:
        case RF_FAMILY_BENINCA_ARC:
        case RF_FAMILY_NICE_FLOR_S:
        case RF_FAMILY_NICE_ONE:
        case RF_FAMILY_ALUTECH_AT4N: return true;
        // Somfy + Security+ are self-contained (public algorithm, no secret).
        default: return false;
    }
}

static bool buffer_nonzero(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++)
        if (buf[i]) return true;
    return false;
}

bool rolling_key_present(const RollingProtocol *proto, const char *keeloq_mf_name) {
    if (!proto) return false;
    if (!rolling_family_needs_key(proto->family)) return true;

    switch (proto->family) {
        case RF_FAMILY_KEELOQ: {
            uint64_t k = 0;
            if (keeloq_mf_name && rolling_mf_key_lookup(keeloq_mf_name, &k, nullptr)) return k != 0;
            return false;
        }
        case RF_FAMILY_FAAC_SLH:
        case RF_FAMILY_JAROLIFT: {
            uint64_t k = 0;
            if (proto->mf_key_name && rolling_mf_key_lookup(proto->mf_key_name, &k, nullptr))
                return k != 0;
            return false;
        }
        case RF_FAMILY_BENINCA_ARC: return buffer_nonzero(beninca_arc_aes_key, 16);
        case RF_FAMILY_NICE_FLOR_S:
        case RF_FAMILY_NICE_ONE: return buffer_nonzero(nice_flor_s_rainbow, 32);
        case RF_FAMILY_ALUTECH_AT4N: return buffer_nonzero(alutech_at4n_rainbow, 32);
        default: return true;
    }
}
