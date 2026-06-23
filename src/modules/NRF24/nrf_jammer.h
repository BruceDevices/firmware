#ifndef __NRF_JAMMER_H
#define __NRF_JAMMER_H
#include "modules/NRF24/nrf_common.h"
#include <RF24.h>

/*
Credits: @smoochiee https://github.com/smoochiee/Ble-jammer
         thank you for providing this simplified code for us!

Information:
    - This Jammer is supposed to be used for Educational purpouse only.
    - The signal is strong enaugh to mess with 2.4Ghz spectrum, so probaby 2.4Gz Wifi networks will be
affected causing a DOS attack
    - If the target is near the bluetooth source (smartphone right beside the bluetooth speaker), the
communication between them will be weakly affected (we don't do magic)

FLAFS:
    NRF24_CE_PIN=-1   ; Set this pin accordingly
    NRF24_SS_PIN=-1   ; Set this pin accordingly
    NRF24_MOSI_PIN=-1 ; Set this pin accordingly
    NRF24_SCK_PIN=-1  ; Set this pin accordingly
    NRF24_MISO_PIN=-1 ; Set this pin accordingly
*/

void nrf_jammer();
void nrf_channel_jammer();
void nrf_channel_hopper();
#endif
