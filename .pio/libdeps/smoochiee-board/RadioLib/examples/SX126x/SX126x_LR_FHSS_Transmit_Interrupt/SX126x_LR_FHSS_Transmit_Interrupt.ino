/*
  RadioLib SX126x LR-FHSS Transmit with Interrupts Example

  This example shows how to use LR-FHSS modem in SX126x chips.
  This modem can only transmit data, and is not able to receive.

  This example transmits packets using SX1262 LoRa radio module.
  Each packet contains up to 256 bytes of data, in the form of:
  - Arduino String
  - null-terminated char array (C-string)
  - arbitrary binary data (byte array)

  Other modules from SX126x family can also be used.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lr-fhss-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1262 has the following connections:
// NSS pin:   10
// IRQ pin:   2
// NRST pin:  3
// BUSY pin:  9
SX1262 radio = new Module(10, 2, 3, 9);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
// or a frequency hop is needed
volatile bool flag = false;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we sent a packet or need to hop, set the flag
  flag = true;
}

void setup() {
  Serial.begin(9600);

  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.beginLRFHSS();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketSentAction(setFlag);

  // start transmitting the first packet
  Serial.print(F("[SX1262] Sending first packet ... "));

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  transmissionState = radio.startTransmit("Hello World!");

  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    state = radio.startTransmit(byteArr, 8);
  */
}

// counter to keep track of transmitted packets
int count = 0;

void loop() {
  // LR-FHSS modem can only transmit!

  // check if the previous transmission finished
  if(flag) {
    // reset flag
    flag = false;

    // check if this was caused by hopping or transmission finished
    if(radio.getIrqFlags() & RADIOLIB_SX126X_IRQ_LR_FHSS_HOP) {
      radio.hopLRFHSS();
    
    } else {
      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));
  
      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);
  
      }
  
      // clean up after transmission is finished
      // this will ensure transmitter is disabled,
      // RF switch is powered down etc.
      radio.finishTransmit();
  
      // wait a second before transmitting again
      delay(1000);
  
      // send another one
      Serial.print(F("[SX1262] Sending another packet ... "));
  
      // you can transmit C-string or Arduino string up to
      // 256 characters long
      String str = "Hello World! #" + String(count++);
      transmissionState = radio.startTransmit(str);
  
      // you can also transmit byte array up to 256 bytes long
      /*
        byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                          0x89, 0xAB, 0xCD, 0xEF};
        transmissionState = radio.startTransmit(byteArr, 8);
      */
      
    }

    

  }

}
