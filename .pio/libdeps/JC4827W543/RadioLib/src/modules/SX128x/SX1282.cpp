#include "SX1282.h"
#if !RADIOLIB_EXCLUDE_SX128X

/// \todo implement advanced ranging
SX1282::SX1282(Module* mod) : SX1280(mod) {
  chipType = RADIOLIB_SX1282_CHIP_TYPE;
}

#endif
