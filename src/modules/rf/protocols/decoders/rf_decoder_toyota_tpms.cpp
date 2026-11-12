#include "rf_decoder_toyota_tpms.h"
#include "rf_decoder_tpms.h"
#include "rf_decoder_tpms_helpers.h"
#include "../rf_config.h"

#define TO_TE 48
#define TO_TE_DELTA 16

static const char *toyota_sync_patterns[] = {
    "00111100",
    "001111100",
    "00111101",
    "001111101",
    NULL
};

bool rf_decode_toyota_tpms(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 32) return false;

    auto bits = tpms_durations_to_bitmap(durations, TO_TE, TO_TE_DELTA);
    if (bits.size() < 128) return false;

    uint32_t off = 0;
    int sync_idx = 0;
    bool found = false;

    for (int j = 0; toyota_sync_patterns[j]; j++) {
        off = tpms_bitmap_seek_bits(bits, 0, (uint32_t)bits.size(), toyota_sync_patterns[j]);
        if (off != TPMS_SEEK_NOT_FOUND) {
            sync_idx = j;
            found = true;
            off += (uint32_t)strlen(toyota_sync_patterns[j]) - 2;
            break;
        }
    }
    if (!found) return false;

    uint8_t raw[9];
    uint32_t decoded = tpms_convert_from_diff_manchester(
        raw, sizeof(raw), bits, off, (uint32_t)bits.size(), true);

    if (decoded < 72) return false;
    if (tpms_crc8(raw, 8, 0x80, 7) != raw[8]) return false;

    float psi = (float)(((raw[4] & 0x7f) << 1) | (raw[5] >> 7)) * 0.25f - 7.0f;
    int temp = (((raw[5] & 0x7f) << 1) | (raw[6] >> 7)) - 40;

    uint32_t id_val = ((uint32_t)raw[0] << 24) | ((uint32_t)raw[1] << 16) |
                       ((uint32_t)raw[2] << 8) | raw[3];

    out.key = ((uint64_t)id_val << 32) |
              ((uint32_t)((int)(psi * 10)) << 16) |
              ((uint32_t)(temp + 100) & 0xFFFF);
    out.serial = id_val;
    out.Bit = 72;
    out.te = TO_TE;
    out.protocol = "Toyota_TPMS";
    out.preset = "2FSKDev238Async";
    return true;
}

bool rf_encode_toyota_tpms(const RfCodes& in, std::vector<int>& out) {
    (void)in;
    (void)out;
    return false;
}
