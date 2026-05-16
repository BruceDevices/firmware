#if !defined(_RADIOLIB_UTILS_H)
#define _RADIOLIB_UTILS_H

#include <stdint.h>
#include <stdlib.h>

#include "../BuildOpt.h"

// macros to access bits in byte array, from http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
#define SET_BIT_IN_ARRAY_MSB(A, k)                              ( A[((k)/8)] |= (1 << ((k)%8)) )
#define CLEAR_BIT_IN_ARRAY_MSB(A, k)                            ( A[((k)/8)] &= ~(1 << ((k)%8)) )
#define TEST_BIT_IN_ARRAY_MSB(A, k)                             ( A[((k)/8)] & (1 << ((k)%8)) )
#define GET_BIT_IN_ARRAY_MSB(A, k)                              ( (A[((k)/8)] & (1 << ((k)%8))) ? 1 : 0 )
#define SET_BIT_IN_ARRAY_LSB(A, k)                              ( A[((k)/8)] |= (1 << (7 - ((k)%8))) )
#define CLEAR_BIT_IN_ARRAY_LSB(A, k)                            ( A[((k)/8)] &= ~(1 << (7 - ((k)%8))) )
#define TEST_BIT_IN_ARRAY_LSB(A, k)                             ( A[((k)/8)] & (1 << (7 - ((k)%8))) )
#define GET_BIT_IN_ARRAY_LSB(A, k)                              ( (A[((k)/8)] & (1 << (7 - ((k)%8)))) ? 1 : 0 )

// frequently used scrambling configurations
// the final bit (x^0 term in polynomial notation) is assumed to always be present
#define RADIOLIB_SCRAMBLER_G3RUH_POLY                           (0x00021001UL)    // x^17 + x^12 + 1
#define RADIOLIB_SCRAMBLER_G3RUH_INIT                           (0x00000000UL)

/*!
  \brief Function to reflect bits within a byte.
  \param in The input to reflect.
  \param bits Number of bits to reflect.
  \return The reflected input.
*/
uint32_t rlb_reflect(uint32_t in, uint8_t bits);

/*!
  \brief Function to scramble or descramble input using a linear feedback shift register (LFSR).
  \param data The input data to (de)scramble.
  \param len Number of input bytes.
  \param poly Polynomial to use for scrambling.
  \param init Initial LFSR value, sometimes called seed.
  \param scramble Whether to perform scrambling (true) or de-scrambling (false).
*/
void rlb_scrambler(uint8_t* data, size_t len, const uint32_t poly, const uint32_t init, bool scramble);

/*!
  \brief Function to dump data as hex into the debug port.
  \param level RadioLib debug level, set to NULL to not print.
  \param data Data to dump.
  \param len Number of bytes to dump.
  \param offset Address offset to add when printing the data.
  \param width Word width (1 for uint8_t, 2 for uint16_t, 4 for uint32_t).
  \param be Print multi-byte data as big endian. Defaults to false.
*/
void rlb_hexdump(const char* level, const uint8_t* data, size_t len, uint32_t offset = 0, uint8_t width = 1, bool be = false);

#if RADIOLIB_DEBUG
size_t rlb_printf(bool ts, const char* format, ...);
#endif

#endif
