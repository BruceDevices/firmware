/*
 *
 */

#ifndef RF24_UTILITY_PIGPIO_SPI_H_
#define RF24_UTILITY_PIGPIO_SPI_H_

#include <cstdint>
#include <stdexcept>

/** Specific exception for SPI errors */
class SPIException : public std::runtime_error
{
public:
    explicit SPIException(const std::string& msg)
        : std::runtime_error(msg)
    {
    }
};

class SPI
{

public:
    /**
     * SPI constructor
     */
    SPI();

    /**
     * Start SPI
     */
    void begin(int busNo, uint32_t spi_speed);

    /**
     * Transfer a single byte
     * @param tx Byte to send
     * @return Data returned via spi
     */
    uint8_t transfer(char tx);

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
    void transfern(char* buf, uint32_t len)
    {
        transfernb(buf, buf, len);
    }

    ~SPI();

private:
    unsigned spiHandle;
    bool spiIsInitialized = false;
    void init(uint32_t spi_speed);
};

#endif /* RF24_UTILITY_PIGPIO_SPI_H_ */
