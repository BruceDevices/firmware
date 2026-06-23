#ifndef __NRF_SPECTRUM_H
#define __NRF_SPECTRUM_H
#include "modules/NRF24/nrf_common.h"
#include <RF24.h>

void nrf_spectrum(SPIClass *SSPI);

String scanChannels(SPIClass *SSPI, bool web = false);

#endif
