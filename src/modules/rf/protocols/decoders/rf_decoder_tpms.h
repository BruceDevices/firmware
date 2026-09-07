#pragma once
#include <stdint.h>
#include <stddef.h>

uint8_t tpms_crc8(const uint8_t *data, size_t len, uint8_t init, uint8_t poly);
