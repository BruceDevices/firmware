#include "rf_decoder_citroen_tpms.h"
#include "rf_decoder_tpms.h"
#include "rf_decoder_tpms_helpers.h"
#include "../rf_config.h"

#define CI_TE 52
#define CI_TE_DELTA 18
#define CI_SYNC "10101010101010110"

bool rf_decode_citroen_tpms(const std::vector<int>& durations, RfCodes& out) {
    if (durations.size() < 32) return false;

    auto bits = tpms_durations_to_bitmap(durations, CI_TE, CI_TE_DELTA);
    if (bits.size() < 80) return false;

    uint32_t sync_len = 17;
    uint32_t off = tpms_bitmap_seek_bits(bits, 0, (uint32_t)bits.size(), CI_SYNC);
    if (off == TPMS_SEEK_NOT_FOUND) return false;

    off += sync_len;

    uint8_t raw[10];
    uint32_t decoded = tpms_convert_from_line_code(
        raw, sizeof(raw), bits, off, (uint32_t)bits.size(), "01", "10");

    if (decoded < 80) return false;

    uint8_t crc = 0;
    for (int j = 1; j < 10; j++) crc ^= raw[j];
    if (crc != 0) return false;

    int repeat = raw[5] & 0xf;
    float kpa = (float)raw[6] * 1.364f;
    int temp = raw[7] - 50;
    int battery = raw[8];

    uint32_t id_val = ((uint32_t)raw[1] << 24) | ((uint32_t)raw[2] << 16) |
                       ((uint32_t)raw[3] << 8) | raw[4];

    out.key = ((uint64_t)id_val << 32) |
              ((uint32_t)((int)(kpa * 10)) << 16) |
              ((uint32_t)(temp + 100) & 0xFFFF);
    out.serial = id_val;
    out.cnt = (uint16_t)repeat;
    out.fix = (uint32_t)battery;
    out.Bit = 80;
    out.te = CI_TE;
    out.protocol = "Citroen_TPMS";
    out.preset = "2FSKDev238Async";
    return true;
}

bool rf_encode_citroen_tpms(const RfCodes& in, std::vector<int>& out) {
    (void)in;
    (void)out;
    return false;
}
