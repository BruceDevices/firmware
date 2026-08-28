#ifndef __NRF_COMMON_H
#define __NRF_COMMON_H
#include <RF24.h>
#include <globals.h>

// Define the Macros case it hasn't been declared
#ifndef NRF24_CE_PIN
#define NRF24_CE_PIN -1
#endif
#ifndef NRF24_SS_PIN
#define NRF24_SS_PIN -1
#endif

enum NRF24_MODE {
    NRF_MODE_DISABLED, // 0b00
    NRF_MODE_SPI,      // 0b01
    NRF_MODE_UART,     // 0b10
    NRF_MODE_BOTH      // 0b11
};

enum NRF_BACK_ACTION : uint8_t {
    NRF_BACK_NONE,
    NRF_BACK_SHORT,
    NRF_BACK_EXIT,
};
#define CHECK_NRF_SPI(mode) (mode & NRF_MODE_SPI)
#define CHECK_NRF_UART(mode) (mode & NRF_MODE_UART)
#define CHECK_NRF_BOTH(mode) (mode == NRF_MODE_BOTH)

extern RF24 NRFradio;
extern HardwareSerial NRFSerial; // Uses UART2 for External NRF's

NRF24_MODE nrf_setMode();

bool nrf_start(NRF24_MODE mode);

// Handles the standard Bruce Back gesture used inside active NRF24 screens.
// On three-button devices, a short Back press is reported separately and a
// 700 ms hold draws the normal progress ring before requesting an exit.
NRF_BACK_ACTION nrf_checkBackButton();

void nrf_info();
#endif
