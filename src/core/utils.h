#ifndef __UTILS_H__
#define __UTILS_H__
#include <Arduino.h>
#include <Wire.h>
#include <globals.h>
void backToMenu();
void addOptionToMainMenu();
int getBattery() __attribute__((weak));

#define TIME_UPDATE_MODE_AUTO_DETECT 0
#define TIME_UPDATE_MODE_AUTO_UPDATE_MANUAL_TIMEZONE 1
#define TIME_UPDATE_MODE_MANUAL 2
bool updateClockTimezone(bool print = false);
void updateTimeStr(struct tm timeInfo);
void formatTimeStr(int hours, int minutes, int seconds);
void showDeviceInfo();
String getOptionsJSON();
void touchHeatMap(struct TouchPoint t);
void i2c_bulk_write(TwoWire *wire, uint8_t addr, const uint8_t *bulk_data);
#endif
