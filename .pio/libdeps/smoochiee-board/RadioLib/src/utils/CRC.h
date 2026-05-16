#if !defined(_RADIOLIB_CRC_H)
#define _RADIOLIB_CRC_H

#include "../TypeDef.h"
#include "../Module.h"

// CCITT CRC properties (used by AX.25)
#define RADIOLIB_CRC_CCITT_POLY                                 (0x1021)
#define RADIOLIB_CRC_CCITT_INIT                                 (0xFFFF)
#define RADIOLIB_CRC_CCITT_OUT                                  (0xFFFF)

/*!
  \class RadioLibCRC
  \brief Class to calculate CRCs of varying formats.
*/
class RadioLibCRC {
  public:
    /*!
      \brief CRC size in bits.
    */
    uint8_t size = 8;

    /*!
      \brief CRC polynomial.
    */
    uint32_t poly = 0;

    /*!
      \brief Initial value.
    */
    uint32_t init = 0;

    /*!
      \brief Final XOR value.
    */
    uint32_t out = 0;

    /*!
      \brief Whether to reflect input bytes.
    */
    bool refIn = false;

    /*!
      \brief Whether to reflect the result.
    */
    bool refOut = false;

    /*!
      \brief Default constructor.
    */
    RadioLibCRC();

    /*!
      \brief Calculate checksum of a buffer.
      \param buff Buffer to calculate the checksum over.
      \param len Size of the buffer in bytes.
      \returns The resulting checksum.
    */
    uint32_t checksum(const uint8_t* buff, size_t len);
};

// the global singleton
extern RadioLibCRC RadioLibCRCInstance;

#endif
