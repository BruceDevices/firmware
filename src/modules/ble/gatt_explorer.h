#ifndef GATT_EXPLORER_H
#define GATT_EXPLORER_H

#include <Arduino.h>

#if !defined(LITE_VERSION)

void gattExplorerMenu();
bool gattConnectCli(const String &macStr, uint8_t addrType = 0);
void gattScanCli(int timeoutSec = 5);

#endif // !LITE_VERSION

#endif // GATT_EXPLORER_H
