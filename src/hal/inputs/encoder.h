#ifndef BRUCE_HAL_INPUTS_ENCODER_H
#define BRUCE_HAL_INPUTS_ENCODER_H

#include "../device.h"
#include "encoder_state.h"

enum class EncoderLatchMode : uint8_t { FOUR3, FOUR0, TWO03 };

void hal_encoder_init(const DeviceEncoder &cfg, EncoderLatchMode mode = EncoderLatchMode::TWO03);

void hal_encoder_poll(const DeviceEncoder &cfg);

// Called by the input task every 10 ms under inputLock(), independently of the
// legacy event throttle. The configuration is copied once by init().
void hal_encoder_sample_buttons();
EncoderButtonState hal_encoder_select_state();
// Reset alongside SelPress in the legacy event producer. Read/consume under the
// same input mutex so synthetic touch/serial events keep their original meaning.
void hal_encoder_reset_select_event();
bool hal_encoder_select_event_is_physical();

#endif
