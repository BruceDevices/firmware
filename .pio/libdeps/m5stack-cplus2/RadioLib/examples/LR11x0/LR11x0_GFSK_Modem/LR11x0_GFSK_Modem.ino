/*
  RadioLib LR11x0 GFSK Modem Example

  This example shows how to use GFSK modem in LR11x0 chips.

  NOTE: The sketch below is just a guide on how to use
        GFSK modem, so this code should not be run directly!
        Instead, modify the other examples to use GFSK
        modem and use the appropriate configuration
        methods.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#lr11x0---gfsk-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// LR1110 has the following connections:
// NSS pin:   10
// IRQ pin:   2
// NRST pin:  3
// BUSY pin:  9
LR1110 radio = new Module(10, 2, 3, 9);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

void setup() {
  Serial.begin(9600);

  // initialize LR1110 with default settings
  Serial.print(F("[LR1110] Initializing ... "));
  int state = radio.beginGFSK();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // if needed, you can switch between any of the modems
  //
  // radio.begin()       start LoRa modem (and disable GFSK)
  // radio.beginGFSK()   start GFSK modem (and disable LoRa)

  // the following settings can also
  // be modified at run-time
  state = radio.setFrequency(433.5);
  state = radio.setBitRate(100.0);
  state = radio.setFrequencyDeviation(10.0);
  state = radio.setRxBandwidth(250.0);
  state = radio.setOutputPower(10.0);
  state = radio.setDataShaping(RADIOLIB_SHAPING_1_0);
  uint8_t syncWord[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
  state = radio.setSyncWord(syncWord, 8);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Unable to set configuration, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // GFSK modem allows advanced CRC configuration
  // Default is CCIT CRC16 (2 bytes, initial 0x1D0F, polynomial 0x1021, inverted)
  // Set CRC to IBM CRC (2 bytes, initial 0xFFFF, polynomial 0x8005, non-inverted)
  state = radio.setCRC(2, 0xFFFF, 0x8005, false);
  // set CRC length to 0 to disable CRC

  #warning "This sketch is just an API guide! Read the note at line 6."
}

void loop() {
  // GFSK modem can use the same transmit/receive methods
  // as the LoRa modem, even their interrupt-driven versions

  // transmit GFSK packet
  int state = radio.transmit("Hello World!");
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
  */
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[LR1110] Packet transmitted successfully!"));
  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    Serial.println(F("[LR1110] Packet too long!"));
  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    Serial.println(F("[LR1110] Timed out while transmitting!"));
  } else {
    Serial.println(F("[LR1110] Failed to transmit packet, code "));
    Serial.println(state);
  }

  // receive GFSK packet
  String str;
  state = radio.receive(str);
  /*
    byte byteArr[8];
    int state = radio.receive(byteArr, 8);
  */
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[LR1110] Received packet!"));
    Serial.print(F("[LR1110] Data:\t"));
    Serial.println(str);
  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.println(F("[LR1110] Timed out while waiting for packet!"));
  } else {
    Serial.print(F("[LR1110] Failed to receive packet, code "));
    Serial.println(state);
  }

  // GFSK modem has built-in address filtering system
  // it can be enabled by setting node address, broadcast
  // address, or both
  //
  // to transmit packet to a particular address,
  // use the following methods:
  //
  // radio.transmit("Hello World!", address);
  // radio.startTransmit("Hello World!", address);

  // set node address to 0x02
  state = radio.setNodeAddress(0x02);
  // set broadcast address to 0xFF
  state = radio.setBroadcastAddress(0xFF);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.println(F("[LR1110] Unable to set address filter, code "));
    Serial.println(state);
  }

  // address filtering can also be disabled
  // NOTE: calling this method will also erase previously set
  //       node and broadcast address
  /*
    state = radio.disableAddressFiltering();
    if (state != RADIOLIB_ERR_NONE) {
      Serial.println(F("Unable to remove address filter, code "));
    }
  */
}
