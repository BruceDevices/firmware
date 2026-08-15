#ifndef __WIFI_MAC_H__
#define __WIFI_MAC_H__
// @IncursioHack

#include "core/settings.h"
#include <Arduino.h>
#include <WiFi.h>

void applyConfiguredMAC();

void wifiMACMenu();

bool validateMACFormat(const String &mac);

bool setCustomMAC(const String &mac);

String generateRandomMAC();

String generateGlobalRandomMAC();

#endif
