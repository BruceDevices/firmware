#ifndef __HC11_MODEM_H__
#define __HC11_MODEM_H__

// HC-11 USB modem app entry point (registered in RFMenu).
// Bridges USB CDC host data to the CC1101 radio using the HC-11 protocol.
void hc11_modem_setup();

#endif // __HC11_MODEM_H__
