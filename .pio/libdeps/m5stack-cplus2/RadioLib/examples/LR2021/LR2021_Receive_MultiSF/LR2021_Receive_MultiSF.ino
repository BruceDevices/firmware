/*
  RadioLib LR2021 Receive Multi-SF Example

  This example listens for LoRa transmissions with different 
  spreading factors and tries to receive them. 
  Once a packet is received, an interrupt is triggered. 

  There are the following limits on configuration of multi-SF
  (or side-detect) reception:
  * In Rx mode, all side-detector spreading factors must be higher
    than the primary one (configured via begin or setSpreadingFactor).
  * For CAD mode, the above condition is inverted, 
    all side-detector spreading factors must be smaller.
  * All packets to be detected/received must have the same header type
    (implicit or explicit).
  * If bandwidth is higher than 500 kHz,
    at most 2 side detectors are allowed.
  * If the primary spreading factor is 10, 11 or 12,
    at most 2 side detectors are allowed.
  * All spreading factors must be different.
  * The difference between maximum and minimum spreading factor used
    must be less than or equal to 4.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#lr2021---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// LR2021 has the following connections:
// NSS pin:   10
// IRQ pin:   2
// NRST pin:  3
// BUSY pin:  9
LR2021 radio = new Module(10, 2, 3, 9);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

void setup() {
  Serial.begin(9600);

  // LR2021 allows to use any DIO pin as the interrupt
  // as an example, we set DIO10 to be the IRQ
  // this has to be done prior to calling begin()!
  radio.irqDioNum = 10;

  // initialize LR2021 with default settings
  Serial.print(F("[LR2021] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // prepare array of side detector configuration structures
  // for spreading factors 10 - 12
  // as the default spreading factor is 9, this satisfies
  // the conditions listed at the top of this example
  // this is just an example; you can also set different
  // sync words, IQ inversions and low data-rate optimization
  // depending on the types of transmitters you expect
  const struct LR2021LoRaSideDetector_t sideDet[3] = {
    { .sf = 10, .ldro = false, .invertIQ = false, 
      .syncWord = RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE },
    { .sf = 11, .ldro = true,  .invertIQ = false,
      .syncWord = RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE },
    { .sf = 12, .ldro = true,  .invertIQ = false,
      .syncWord = RADIOLIB_LR2021_LORA_SYNC_WORD_PRIVATE },
  };
  Serial.print(F("[LR2021] Setting side detector configuration ... "));
  state = radio.setSideDetector(sideDet, 3);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setFlag);

  // start listening for LoRa packets
  Serial.print(F("[LR2021] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // radio.standby()
  // radio.sleep()
  // radio.transmit();
  // radio.receive();
  // radio.scanChannel();
}

void loop() {
  // check if the flag is set
  if(receivedFlag) {
    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    String str;
    int state = radio.readData(str);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int numBytes = radio.getPacketLength();
      int state = radio.readData(byteArr, numBytes);
    */

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[LR2021] Received packet!"));

      // print data of the packet
      Serial.print(F("[LR2021] Data:\t\t"));
      Serial.println(str);

      // print RSSI (Received Signal Strength Indicator)
      Serial.print(F("[LR2021] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      Serial.print(F("[LR2021] SNR:\t\t"));
      Serial.print(radio.getSNR());
      Serial.println(F(" dB"));

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);

    }
  }
}
