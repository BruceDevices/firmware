#ifndef CORE_SYSTEM_STATUS_H
#define CORE_SYSTEM_STATUS_H

#include <stdint.h>

namespace SystemStatus {
void begin();

void update();

uint32_t cpuFrequencyMhz();

uint8_t cpuLoadPercent();
} // namespace SystemStatus

#endif
