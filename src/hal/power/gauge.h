#ifndef BRUCE_HAL_POWER_GAUGE_H
#define BRUCE_HAL_POWER_GAUGE_H

#include "../device.h"

bool hal_gauge_init(const DeviceGauge &cfg);

int hal_gauge_get_percent();

bool hal_gauge_is_charging();

// true when a fuel-gauge IC (GAUGE_BQ27220) provides detailed readings
bool hal_gauge_has_info();

// Fills `out` with detailed readings; false when there is no gauge IC.
bool hal_gauge_get_info(DeviceGaugeInfo &out);

#endif
