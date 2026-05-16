/**
 * @file spi.h
 * Class declaration for SPI helper files
 */

/**
 * Example of spi.h class declaration for SPI portability
 *
 * @defgroup Porting_SPI Porting: SPI
 * @{
 */
#ifndef RF24_UTILITY_TEMPLATE_SPI_H_
#define RF24_UTILITY_TEMPLATE_SPI_H_

#include <stdint.h>

#ifndef DOXYGEN_FORCED
// exclude this line from the docs to prevent displaying in the list of classes
class SPI
#endif
{

public:
    /**
     * SPI constructor
     */
    SPI();

    /**
     * Start SPI
     */
    void begin(int busNo);

    /**
     * Transfer a single byte
     * @param tx_ Byte to send
     * @return Data returned via spi
     */
    uint8_t transfer(uint8_t tx_);

    /**
     * Transfer a buffer of data
     * @param txBuf Transmit buffer
     * @param rxBuf Receive buffer
     * @param len Length of the data
     */
    void transfernb(char* txBuf, char* rxBuf, uint32_t len);

    /**
     * Transfer a buffer of data without an rx buffer
     * @param buf Pointer to a buffer of data
     * @param len Length of the data
     */
    void transfern(char* buf, uint32_t len);

#ifndef DOXYGEN_FORCED
    // exclude this line from the docs to prevent warnings docs generators
    virtual ~SPI();
#endif

private:
    /** SPI bus */
    int bus;
    /** SPI Mode set */
    uint8_t mode;
    /** SPI word size */
    uint8_t bits;
    /** SPI speed */
    uint32_t speed;
};

/**@}*/

#endif // RF24_UTILITY_TEMPLATE_SPI_H_
