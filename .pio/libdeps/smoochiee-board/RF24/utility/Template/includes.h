/**
 * @file includes.h
 * Configuration defines for RF24/Linux
 */

/**
 * Example of includes.h for RF24 Linux portability
 *
 * @defgroup Porting_Includes Porting: Includes
 * @{
 */

#ifndef RF24_UTILITY_INCLUDES_H_
#define RF24_UTILITY_INCLUDES_H_

/**
 * Define a specific platform for this configuration
 */
#define RF24_TEMPLATE

/**
 * Load the correct configuration for this platform
 */
#include "Template/RF24_arch_config.h"

/**@}*/

#endif // RF24_UTILITY_INCLUDES_H_
