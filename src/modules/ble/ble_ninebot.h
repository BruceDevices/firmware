#ifndef __BLE_SCOOTER_H__
#define __BLE_SCOOTER_H__
#if !defined(LITE_VERSION)
#include <Arduino.h>
#include <NimBLEDevice.h>

#include "core/display.h"
#include <globals.h>

class BLENinebot {
public:
    BLENinebot();
    ~BLENinebot();

    void setup();
    void loop();

private:
    void clientDisconnect(void);
    void redrawMainBorder(void);
};
#endif
#endif
