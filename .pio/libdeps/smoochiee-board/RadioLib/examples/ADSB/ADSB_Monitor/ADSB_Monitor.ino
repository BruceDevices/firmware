/*
  RadioLib ADS-B Monitor Example

  This example shows how to receive ADS-B messages
  using LR2021 OOK modem and forward them to be displayed
  in a live dashboard.

  To show the live dashboard in a terminal, run the Python script
  RadioLib/extras/ADSB_Monitor/ADSBMonitorServer.py,
  and then connect to from "modeslive" from the pyModeS package
  (see the script helptext for installation instructions).

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

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

// create ADS-B client instance using the module
ADSBClient adsb(&radio);

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

  // initialize LR2021 OOK modem at 1090 MHz,
  // 2 Mbps bit rate and receiver bandwidth 3076 kHz
  Serial.print(F("[LR2021] Initializing ... "));
  int state = radio.beginOOK(1090, 2000, 3076);

  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // initialize ADS-B client
  Serial.print(F("[ADS-B] Initializing ... "));
  state = adsb.begin();
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // apply LR2021-specific settings
  Serial.print(F("[LR2021] Setting configuration ... "));
  state = radio.setRxBoostedGainMode(7);
  state += radio.setCRC(3, 0, 0x1FFF409UL, false);
  state += radio.ookDetector();
  state += radio.fixedPacketLengthMode(11);
  state += radio.setGain(13);
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // measure current noise floor and add some margin
  // this values is later used to set the signal detection threshold
  float threshold = radio.getRSSI(false) + 10;
  Serial.print(F("[LR2021] Detection threshold: "));
  Serial.print(threshold, 2);
  Serial.println(F(" dBm"));

  Serial.print(F("[LR2021] Setting threshold ... "));
  state = radio.setOokDetectionThreshold(threshold);
  if(state == RADIOLIB_ERR_NONE) {
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
}

void loop() {
  // check if the flag is set
  if(receivedFlag) {
    // reset flag
    receivedFlag = false;

    // read the received binary data
    // ADS-B frames have fixed length of 14 bytes,
    // 3 of which are used as CRC which is handled automatically
    uint8_t buff[11] = { 0 };
    int state = radio.readData(buff, 11);

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      // print it out, further processing is done by pyModeS
      Serial.print(F("[ADS-B] "));
      for(int i = 0; i < 11; i++) {
        if(buff[i] < 0x10) { Serial.print('0'); }
        Serial.print(buff[i], HEX);
      }

      // pyModeS expects the data to include the CRC
      // but does not seem to be actually checking it
      // since we handle CRC automatically, just append 3 null bytes
      Serial.println(F("000000"));
    
    }
  
  }

}
