/*
   See documentation at https://nRF24.github.io/RF24
   See License information at root directory of this library
   Authors: Brendan Doherty (2bndy5), Douglas Quigg (dstroy0)
*/

/**
  A simple example of getting debug info from the nRF24L01 transceiver.

  This example was written to demonstrate alternative methods to get debugging data.
  1. radio.encodeRadioDetails() will provide a data dump of all the nRF24L01's registers.
  2. radio.sprintfPrettyDetails() will behave similarly to printPrettyDetails(), but it
     outputs to a char buffer that can be printed to any Serial (or other output) stream.

  Additionally, this example will show all default configuration values.
*/
#include <SPI.h>
#include "RF24.h"

#define CE_PIN 7
#define CSN_PIN 8
// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

/*
  For this example, we'll be using a data buffer containing
  radio details encoded with RF24::encodeRadioDetails().
  It is meant to be decoded by an external program.

  There is a python script located in this example's folder that
  will take a space-delimited string of hexadecimal characters and
  decode then print it out as human readable information.
*/
uint8_t encoded_details[43] = { 0 };

// Use this function to print out the encoded_details as a
// space-delimited string of hexadecimal characters.
void dumpRegData() {
  for (uint8_t i = 0; i < 43; ++i) {
    Serial.print(encoded_details[i], HEX);
    if (i < 42)
      Serial.print(F(" "));
  }
}

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }

  // print example's introductory prompt
  Serial.println(F("RF24/examples/encodedRadioDetails"));

  Serial.println(F("Press any key to show debugging information"));
  while (!Serial.available()) {
    // wait for user input
  }

  // For debugging info
  char *debug_info = new char[870];
  uint16_t str_len = radio.sprintfPrettyDetails(debug_info);
  Serial.println(debug_info);
  Serial.print(F("\nThe above output used "));
  Serial.print(str_len);
  Serial.println(F(" characters."));

  // encoded_details is NOT human readable.
  // encodeRadioDetails() is very small when used on its own because it puts debugging information into a byte array
  // No printf() support needed because it doesn't use an output stream.
  radio.encodeRadioDetails(encoded_details);
  Serial.println(F("\nhexadecimal dump of all registers:"));
  dumpRegData();

  Serial.println(F("\n\nCopy the above string of hexadecimal characters (including spaces)."));
  Serial.print(F("Then paste it into a terminal using the print_details.py located in"));
  Serial.print(F(" this example's folder. Like so:\npython print_details.py \""));
  dumpRegData();
  Serial.println(F("\"\n***You may need to use 'python3' (without quotes) on Linux"));
}  // setup

/* Registers corresponding to index of encoded_details array
  0:     NRF_CONFIG
  1:     EN_AA
  2:     EN_RXADDR
  3:     SETUP_AW
  4:     SETUP_RETR
  5:     RF_CH
  6:     RF_SETUP
  7:     NRF_STATUS
  8:     OBSERVE_TX
  9:     CD (aka RPD)
  10-14: RX_ADDR_P0
  15-19: RX_ADDR_P1
  20:    RX_ADDR_P2
  21:    RX_ADDR_P3
  22:    RX_ADDR_P4
  23:    RX_ADDR_P5
  24-28: TX_ADDR
  29:    RX_PW_P0
  30:    RX_PW_P1
  31:    RX_PW_P2
  32:    RX_PW_P3
  33:    RX_PW_P4
  34:    RX_PW_P5
  35:    FIFO_STATUS
  36:    DYNPD
  37:    FEATURE
  38-39: ce_pin
  40-41: csn_pin
  42:    SPI speed MHz | (isPlusVariant << 4)
*/

void loop() {
  // Nothing to do here. We did it all at the end of setup()
}
