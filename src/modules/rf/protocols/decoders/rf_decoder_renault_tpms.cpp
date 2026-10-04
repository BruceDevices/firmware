#include "rf_decoder_renault_tpms.h"
#include "rf_decoder_tpms.h"
#include "rf_decoder_tpms_helpers.h"
#include "../rf_config.h"

#define RE_TE 48
#define RE_TE_DELTA 16
#define RE_SYNC "01010101010101010110"

bool rf_decode_renault_tpms(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 32) return false;

    auto bits = tpms_durations_to_bitmap(durations, RE_TE, RE_TE_DELTA);
    if (bits.size() < 80) return false;

    uint32_t off = tpms_bitmap_seek_bits(bits, 0, (uint32_t)bits.size(), RE_SYNC);
    if (off == TPMS_SEEK_NOT_FOUND) return false;

    off += 20;

    uint8_t raw[9];
    uint32_t decoded = tpms_convert_from_line_code(
        raw, sizeof(raw), bits, off, (uint32_t)bits.size(), "01", "10");

    if (decoded < 72) return false;
    if (tpms_crc8(raw, 8, 0, 7) != raw[8]) return false;

    uint8_t flags = raw[0] >> 2;
    float kpa = 0.75f * (float)(((uint32_t)(raw[0] & 3) << 8) | raw[1]);
    int temp = raw[2] - 30;

    uint32_t id_val = ((uint32_t)raw[3] << 16) | ((uint32_t)raw[4] << 8) | raw[5];

    out.key = ((uint64_t)id_val << 32) |
              ((uint32_t)((int)(kpa * 10)) << 16) |
              ((uint32_t)(temp + 100) & 0xFFFF);
    out.serial = id_val;
    out.btn = flags;
    out.Bit = 72;
    out.te = RE_TE;
    out.protocol = "Renault_TPMS";
    out.preset = "2FSKDev238Async";
    return true;
}

bool rf_encode_renault_tpms(const RfCodes& in, std::vector<int>& out) {
    (void)in;
    (void)out;
    return false;
}
