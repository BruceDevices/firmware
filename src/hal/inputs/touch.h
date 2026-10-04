#ifndef BRUCE_HAL_INPUTS_TOUCH_H
#define BRUCE_HAL_INPUTS_TOUCH_H

#include "../device.h"

struct BruceTouchPoint; // include/globals.h -- only referenced by pointer/ref here

bool hal_touch_init(const DeviceTouch &cfg, uint8_t i2c_addr = 0x5D, bool xpt_shared_spi = true);

bool hal_touch_read(const DeviceTouch &cfg, BruceTouchPoint &out);

void hal_touch_set_home_button(int16_t x, int16_t y, void (*cb)(void *user_data), void *user_data = nullptr);

void hal_touch_disable_auto_sleep();

void hal_touch_set_threshold(uint8_t value);

bool hal_touch_read_raw(BruceTouchPoint &out);

bool hal_touch_get_resolution(uint16_t &width, uint16_t &height);

bool hal_touch_apply(const BruceTouchPoint &t, bool wakeUp = true);

#endif
