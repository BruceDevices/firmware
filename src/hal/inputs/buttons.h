#ifndef BRUCE_HAL_INPUTS_BUTTONS_H
#define BRUCE_HAL_INPUTS_BUTTONS_H

#include "../device.h"

void hal_buttons_init(const DeviceButtons &cfg, uint8_t count);

void hal_buttons_poll_1(const DeviceButtons &cfg);

void hal_buttons_init_2(const DeviceButtons &cfg, uint16_t long_press_ms = 600);

void hal_buttons_poll_2();

void hal_buttons_poll_3(const DeviceButtons &cfg);

void hal_buttons_poll_5(const DeviceButtons &cfg);

void hal_buttons_poll_6(const DeviceButtons &cfg, bool esc_on_combo_too = false);

#endif
