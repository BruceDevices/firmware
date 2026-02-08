#ifndef __UTILS_H__
#define __UTILS_H__
#include <Arduino.h>
#include <Wire.h>
void backToMenu();
void addOptionToMainMenu();
int getBattery() __attribute__((weak));
void updateClockTimezone();
void updateTimeStr(struct tm timeInfo);
void showDeviceInfo();
String formatTimeDecimal(uint32_t totalMillis);
String getOptionsJSON();
void touchHeatMap(struct TouchPoint t);
void i2c_bulk_write(TwoWire *wire, uint8_t addr, const uint8_t *bulk_data);
void printMemoryUsage(const char *msg = "");
String repeatString(int length, String character);
String formatBytes(uint64_t bytes);
#endif
