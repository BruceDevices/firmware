/*
 *
 */

#include <pigpio.h>
#include "spi.h"

SPI::SPI()
{
}

void SPI::begin(int busNo, uint32_t spi_speed)
{
    if (this->spiIsInitialized) {
        return;
    }
    spiIsInitialized = true;
    gpioInitialise();
    spiHandle = spiOpen((unsigned int)(busNo & 2), spi_speed, (unsigned int)((busNo / 10) << 7));
}

void SPI::init(uint32_t speed)
{
}

uint8_t SPI::transfer(char tx)
{
    char rbuf = 0;
    spiXfer(spiHandle, &tx, &rbuf, 1);
    return rbuf;
}

void SPI::transfernb(char* txBuf, char* rxBuf, uint32_t len)
{
    spiXfer(spiHandle, txBuf, rxBuf, len);
}

SPI::~SPI()
{
}
