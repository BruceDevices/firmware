/*
  RadioLib ADS-B Reception Example

  This example shows how to receive ADS-B messages
  using LR2021 OOK modem.

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

  // set the reference position (position of the receiver)
  // this will be used later to calculate the aircraft position
  adsb.setReferencePosition(46.24722039359617f, 12.097993784412322f);

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

// these variables will be used later to save decoded information
ADSBFrame frame;
ADSBAircraftCategory category;
char callsign[RADIOLIB_ADSB_CALLSIGN_LEN];
char hexId[RADIOLIB_ADSB_HEX_ID_LEN];
ADSBAircraftCategory cat;
int alt;
float lat, lon;
bool altGnss;

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
      Serial.println(F("[LR2021] Received packet!"));

      state = adsb.decode(buff, &frame);
      if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("[ADS-B] Decoded frame!"));
        // print the decoded information
        Serial.print(F("[ADS-B] DF = "));
        Serial.println(frame.downlinkFormat);
        Serial.print(F("[ADS-B] CA = "));
        Serial.println(frame.capability);
        Serial.print(F("[ADS-B] Message type = "));
        Serial.println((int)frame.messageType);

        // get the hexadecimal ID (ICAO address) of the transceiver
        state = adsb.parseHexId(&frame, hexId);
        if (state == RADIOLIB_ERR_NONE) {
          Serial.print(F("[ADS-B] Hex ID = "));
          Serial.println(hexId);
        }

        // further information depends on the message type
        switch(frame.messageType) {
          case(ADSBMessageType::AIRCRAFT_ID):
            adsb.parseCallsign(&frame, callsign, &cat);
            Serial.print(F("[ADS-B] Callsign = "));
            Serial.println(callsign);
            Serial.print(F("[ADS-B] Category = "));
            Serial.println((int)category);
            break;
          
          case(ADSBMessageType::AIRBORNE_POS_ALT_BARO):
          case(ADSBMessageType::AIRBORNE_POS_ALT_GNSS):
            adsb.parseAirbornePosition(&frame, &alt, &lat, &lon, &altGnss);
            Serial.print(F("[ADS-B] Altitude = "));
            Serial.print(alt);
            if(altGnss) {  Serial.println(" meters\n"); } else {  Serial.println(" feet\n"); }
            Serial.print(F("[ADS-B] Latitude = "));
            Serial.println(lat, 6);
            Serial.print(F("[ADS-B] Longitude = "));
            Serial.println(lon, 6);
            break;

          default:
            break;
          
        }

      } else {
        Serial.print(F("[ADS-B] Failed to decode, code "));
        Serial.println(state);

      }


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
