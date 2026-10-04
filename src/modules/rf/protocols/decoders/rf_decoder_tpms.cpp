#include "rf_decoder_tpms.h"

uint8_t tpms_crc8(const uint8_t *data, size_t len, uint8_t init, uint8_t poly) {
    uint8_t crc = init;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ poly);
            else
                crc <<= 1;
        }
    }
    return crc;
}
