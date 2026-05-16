/**
 * @file compatibility.h
 * Class declaration for SPI helper files
 */

/**
 * Example of compatibility.h class declaration for timing functions portability
 *
 * @defgroup Porting_Timing Porting: Timing
 * @{
 */

#ifndef RF24_UTILITY_TEMPLATE_COMPATIBLITY_H_
#define RF24_UTILITY_TEMPLATE_COMPATIBLITY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void __msleep(int millisec);

void __usleep(int microsec);

void __start_timer();

uint32_t __millis();

#ifdef __cplusplus
}
#endif

/**@}*/

#endif // RF24_UTILITY_TEMPLATE_COMPATIBLITY_H_
