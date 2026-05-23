#ifndef __SUBGHZ_ADVANCED_DECODER_ADAPTER_H__
#define __SUBGHZ_ADVANCED_DECODER_ADAPTER_H__

#include "subghz_advanced_types.h"

#include <vector>

class SubGhzAdvancedDecoderAdapter {
public:
    static SubGhzAdvancedFrame decodeBruceCapture(
        const String& capture,
        const String& source = "live-rx",
        bool full_profile = false);

    static void getEnabledProtocolNames(bool full_profile, std::vector<String>& out);
    static size_t getEnabledProtocolCount(bool full_profile);

    static bool decodeRawSubText(
        const String& text,
        uint32_t frequency_hz,
        bool full_profile,
        SubGhzAdvancedFrame& frame);
};

#endif
