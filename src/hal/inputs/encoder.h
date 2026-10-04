#ifndef BRUCE_HAL_INPUTS_ENCODER_H
#define BRUCE_HAL_INPUTS_ENCODER_H

#include "../device.h"

enum class EncoderLatchMode : uint8_t { FOUR3, FOUR0, TWO03 };

void hal_encoder_init(const DeviceEncoder &cfg, EncoderLatchMode mode = EncoderLatchMode::TWO03);

void hal_encoder_poll(const DeviceEncoder &cfg);

#endif
