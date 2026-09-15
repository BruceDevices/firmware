#ifndef LITE_VERSION
#pragma once

#include <OneWire.h>

enum IButtonResult {
    IBUTTON_SUCCESS = 0,
    IBUTTON_FAILURE,
    IBUTTON_NO_KEY,
    IBUTTON_CRC_ERROR,
};

void setup_ibutton();
void setiButtonPinMenu();

#endif
