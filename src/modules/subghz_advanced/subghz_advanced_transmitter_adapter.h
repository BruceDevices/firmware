#ifndef __SUBGHZ_ADVANCED_TRANSMITTER_ADAPTER_H__
#define __SUBGHZ_ADVANCED_TRANSMITTER_ADAPTER_H__

#include <Arduino.h>

class SubGhzAdvancedTransmitterAdapter {
public:
    static bool extractProtocolName(const String& capture, String& protocol_name);
    static bool canHandleCapture(const String& capture, bool full_profile);
    static bool transmitCapture(const String& capture, bool full_profile, bool hideDefaultUI = false);
};

#endif
