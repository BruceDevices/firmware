// band_types.h
#ifndef BAND_TYPES_H
#define BAND_TYPES_H

#include <vector>

enum BandType {
    BAND_2_4GHZ = 0,
    BAND_5GHZ = 1,
    BAND_6GHZ = 2
};

struct SupportedBands {
    bool has2_4GHz = false;
    bool has5GHz = false;
    bool has6GHz = false;
    int bandCount = 0;
    std::vector<int> bandList;
};

#endif
