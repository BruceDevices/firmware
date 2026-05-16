#if !defined(RADIOLIB_LR11X0_REGISTERS_H)
#define RADIOLIB_LR11X0_REGISTERS_H

#include "../../TypeDef.h"

#if !RADIOLIB_EXCLUDE_LR11X0

// LR11X0 register map
#define RADIOLIB_LR11X0_REG_SF6_SX127X_COMPAT                   (0x00F20414)
#define RADIOLIB_LR11X0_REG_LORA_HIGH_POWER_FIX                 (0x00F30054)
#define RADIOLIB_LR11X0_REG_LNA_MODE                            (0x00F3008C)
#define RADIOLIB_LR11X0_REG_GFSK_FIX1                           (0x00F20344)
#define RADIOLIB_LR11X0_REG_GFSK_FIX2                           (0x00F20348)
#define RADIOLIB_LR11X0_REG_GFSK_FIX3                           (0x00F20244)

// LR11X0 SPI register variables

// RADIOLIB_LR11X0_REG_SF6_SX127X_COMPAT
#define RADIOLIB_LR11X0_SF6_SX126X                              (0x00UL << 18)  //  18    18    SF6 mode: SX126x series
#define RADIOLIB_LR11X0_SF6_SX127X                              (0x01UL << 18)  //  18    18              SX127x series

// RADIOLIB_LR11X0_REG_LORA_HIGH_POWER_FIX
#define RADIOLIB_LR11X0_LORA_HIGH_POWER_FIX                     (0x00UL << 30)  //  30    30    fix for errata

// RADIOLIB_LR11X0_REG_LNA_MODE
#define RADIOLIB_LR11X0_LNA_MODE_SINGLE_RFI_N                   (0x01UL << 4)   //  7     4     LNA mode: single-ended RFI_N
#define RADIOLIB_LR11X0_LNA_MODE_SINGLE_RFI_P                   (0x02UL << 4)   //  7     4               single-ended RFI_P
#define RADIOLIB_LR11X0_LNA_MODE_DIFFERENTIAL                   (0x03UL << 4)   //  7     4               differential (default)

#endif

#endif
