#ifndef __SUBGHZ_ADVANCED_SUBFILE_CODEC_H__
#define __SUBGHZ_ADVANCED_SUBFILE_CODEC_H__

#include "subghz_advanced_types.h"

class SubGhzAdvancedSubFileCodec {
public:
    static SubGhzAdvancedFrame parse(const String& text, const String& source = "");
};

#endif
