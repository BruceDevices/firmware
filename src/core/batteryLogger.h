#ifndef CORE_BATTERY_LOGGER_H
#define CORE_BATTERY_LOGGER_H

#include <Arduino.h>

namespace BatteryLogger {
struct BatteryStatus {
    int percent = 0;
    float voltage = 0.0f;
    bool voltageValid = false;
    bool charging = false;
};

void begin();

void update();

void updateIntervalFromConfig(bool immediateSample = false);

BatteryStatus currentStatus();

bool deleteLogFile();

bool logFileExists();

void showLogAsText();

void showLogAsGraph();

const char *logFilePath();
} // namespace BatteryLogger

#endif
