#ifndef GATT_SERVER_H
#define GATT_SERVER_H

#include <Arduino.h>

#if !defined(LITE_VERSION)

void gattServerMenu();
void runGattServer(int profileMode = 0);
bool startGattServerService(int profileMode = 0);
void stopGattServerService();
bool isGattServerActive();

#endif // !LITE_VERSION

#endif // GATT_SERVER_H
