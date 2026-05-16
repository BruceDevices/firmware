#if !defined(_RADIOLIB_LR11X0_FIRMWARE_H)
#define _RADIOLIB_LR11X0_FIRMWARE_H

#if defined(RADIOLIB_LR1110_FIRMWARE_IN_RAM)
  #define RADIOLIB_LR1110_FIRMWARE_ATTR
#else
  #define RADIOLIB_LR1110_FIRMWARE_ATTR   RADIOLIB_NONVOLATILE
#endif

#define RADIOLIB_LR11X0_FIRMWARE_IMAGE_SIZE LR11XX_FIRMWARE_IMAGE_SIZE

#if defined(RADIOLIB_LR1110_FIRMWARE_0303)
  #include "firmware/lr1110_transceiver_0303.h"
#elif defined(RADIOLIB_LR1110_FIRMWARE_0304)
  #include "firmware/lr1110_transceiver_0304.h"
#elif defined(RADIOLIB_LR1110_FIRMWARE_0305)
  #include "firmware/lr1110_transceiver_0305.h"
#elif defined(RADIOLIB_LR1110_FIRMWARE_0306)
  #include "firmware/lr1110_transceiver_0306.h"
#elif defined(RADIOLIB_LR1110_FIRMWARE_0307)
  #include "firmware/lr1110_transceiver_0307.h"
#elif defined(RADIOLIB_LR1110_FIRMWARE_0401)
  #include "firmware/lr1110_transceiver_0401.h"
#elif defined(RADIOLIB_LR1120_FIRMWARE_0101)
  #include "firmware/lr1120_transceiver_0101.h"
#elif defined(RADIOLIB_LR1120_FIRMWARE_0102)
  #include "firmware/lr1120_transceiver_0102.h"
#elif defined(RADIOLIB_LR1120_FIRMWARE_0201)
  #include "firmware/lr1120_transceiver_0201.h"
#elif defined(RADIOLIB_LR1121_FIRMWARE_0102)
  #include "firmware/lr1121_transceiver_0102.h"
#elif defined(RADIOLIB_LR1121_FIRMWARE_0103)
  #include "firmware/lr1121_transceiver_0103.h"
#else
  #error "No LR11x0 firmware image selected!"
#endif

#endif
