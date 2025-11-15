#ifndef CORE_BATTERY_LOGGER_H
#define CORE_BATTERY_LOGGER_H

#include <Arduino.h>

namespace BatteryLogger {
void begin();

void update();

void updateIntervalFromConfig(bool immediateSample = false);

bool deleteLogFile();

bool logFileExists();

void showLogAsText();

void showLogAsGraph();

const char *logFilePath();
} // namespace BatteryLogger

#endif
