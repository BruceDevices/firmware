#include "rf_decoder_schrader_tpms.h"
#include "rf_decoder_tpms.h"
#include "rf_decoder_tpms_helpers.h"
#include "../rf_config.h"

#define SR_TE 120
#define SR_TE_DELTA 40
#define SR_SYNC "1111010101" "01011010"

bool rf_decode_schrader_tpms(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 32) return false;

    auto bits = tpms_durations_to_bitmap(durations, SR_TE, SR_TE_DELTA);
    if (bits.size() < 64) return false;

    uint32_t off = tpms_bitmap_seek_bits(bits, 0, (uint32_t)bits.size(), SR_SYNC);
    if (off == TPMS_SEEK_NOT_FOUND) return false;

    off += 10;

    uint8_t raw[8];
    uint32_t decoded = tpms_convert_from_line_code(
        raw, sizeof(raw), bits, off, (uint32_t)bits.size(), "01", "10");

    if (decoded < 64) return false;

    raw[0] |= 0xf0;
    uint8_t cksum = tpms_crc8(raw, sizeof(raw) - 1, 0xf0, 0x07);
    if (cksum != raw[7]) return false;

    float kpa = (float)raw[5] * 2.5f;
    int temp = raw[6] - 50;

    uint8_t id[4] = {raw[1] & 7, raw[2], raw[3], raw[4]};
    uint32_t id_val = ((uint32_t)id[0] << 24) | ((uint32_t)id[1] << 16) |
                       ((uint32_t)id[2] << 8) | id[3];

    out.key = ((uint64_t)id_val << 32) |
              ((uint32_t)((int)(kpa * 10)) << 16) |
              ((uint32_t)(temp + 100) & 0xFFFF);
    out.serial = id_val;
    out.Bit = 64;
    out.te = SR_TE;
    out.protocol = "Schrader_TPMS";
    out.preset = "Ook270Async";
    return true;
}

bool rf_encode_schrader_tpms(const RfCodes& in, std::vector<int>& out) {
    (void)in;
    (void)out;
    return false;
}
