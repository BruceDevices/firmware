#include "rf_decoder_ford_tpms.h"
#include "rf_decoder_tpms.h"
#include "rf_decoder_tpms_helpers.h"
#include "../rf_config.h"

#define FO_TE 52
#define FO_TE_DELTA 18
#define FO_SYNC "010101010101" "0110"

bool rf_decode_ford_tpms(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 32) return false;

    auto bits = tpms_durations_to_bitmap(durations, FO_TE, FO_TE_DELTA);
    if (bits.size() < 64) return false;

    uint32_t sync_len = 16;
    uint32_t off = tpms_bitmap_seek_bits(bits, 0, (uint32_t)bits.size(), FO_SYNC);
    if (off == TPMS_SEEK_NOT_FOUND) return false;

    off += sync_len;

    uint8_t raw[8];
    uint32_t decoded = tpms_convert_from_line_code(
        raw, sizeof(raw), bits, off, (uint32_t)bits.size(), "01", "10");

    if (decoded < 64) return false;

    uint8_t crc = 0;
    for (int j = 0; j < 7; j++) crc += raw[j];
    if (crc != raw[7]) return false;

    float psi = 0.25f * (float)(((raw[6] & 0x20) << 3) | raw[4]);
    int temp = (raw[5] & 0x80) ? 0 : raw[5] - 56;
    int flags = raw[5] & 0x7f;
    int car_moving = (raw[6] & 0x44) == 0x44;

    uint32_t id_val = ((uint32_t)raw[0] << 24) | ((uint32_t)raw[1] << 16) |
                       ((uint32_t)raw[2] << 8) | raw[3];

    out.key = ((uint64_t)id_val << 32) |
              ((uint32_t)((int)(psi * 10)) << 16) |
              ((uint32_t)(flags & 0xFF) << 8) |
              (uint32_t)(car_moving ? 1 : 0);
    out.serial = id_val;
    out.btn = (uint8_t)(flags & 0x7F);
    out.Bit = 64;
    out.te = FO_TE;
    out.protocol = "Ford_TPMS";
    out.preset = "2FSKDev238Async";
    return true;
}

bool rf_encode_ford_tpms(const RfCodes& in, std::vector<int>& out) {
    (void)in;
    (void)out;
    return false;
}
