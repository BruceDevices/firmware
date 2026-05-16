/**
 * @file gpio.h
 * Class declaration for SPI helper files
 */

/**
 * Example of gpio.h class declaration for GPIO portability
 *
 * @defgroup Porting_GPIO Porting: GPIO
 * @{
 */
#ifndef RF24_UTILITY_TEMPLATE_GPIO_H_
#define RF24_UTILITY_TEMPLATE_GPIO_H_

#include <cstdio>

#ifndef DOXYGEN_FORCED
// exclude this line from the docs to prevent displaying in the list of classes
class GPIO
#endif
{

public:
    /* Constants */
    static const int DIRECTION_OUT = 1;
    static const int DIRECTION_IN = 0;

    static const int OUTPUT_HIGH = 1;
    static const int OUTPUT_LOW = 0;

    GPIO();

    /**
     * Similar to Arduino pinMode(pin,mode);
     * @param port
     * @param DDR
     */
    static void open(int port, int DDR);

    /**
     *
     * @param port
     */
    static void close(int port);

    /**
     * Similar to Arduino digitalRead(pin);
     * @param port
     */
    static int read(int port);

    /**
     * Similar to Arduino digitalWrite(pin,state);
     * @param port
     * @param value
     */
    static void write(int port, int value);

#ifndef DOXYGEN_FORCED
    // exclude this line from the docs to prevent warnings docs generators
    virtual ~GPIO();
#endif
};

/**@}*/

#endif // RF24_UTILITY_TEMPLATE_GPIO_H_
