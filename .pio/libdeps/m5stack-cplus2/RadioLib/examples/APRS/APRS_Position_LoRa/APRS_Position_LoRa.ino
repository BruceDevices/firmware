/*
  RadioLib APRS Position over LoRa Example

  This example sends APRS position reports 
  using SX1278's LoRa modem.

  Other modules that can be used for APRS:
  - SX127x/RFM9x
  - SX126x/LLCC68
  - SX128x
  - LR11x0

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1278 has the following connections:
// NSS pin:   10
// DIO0 pin:  2
// RESET pin: 9
// DIO1 pin:  3
SX1278 radio = new Module(10, 2, 9, 3);

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// create APRS client instance using the LoRa radio
APRSClient aprs(&radio);

void setup() {
  Serial.begin(9600);

  // initialize SX1278 with the settings necessary for LoRa iGates
  Serial.print(F("[SX1278] Initializing ... "));
  // frequency:                   433.775 MHz
  // bandwidth:                   125 kHz
  // spreading factor:            12
  // coding rate:                 4/5
  int state = radio.begin(433.775, 125, 12, 5);

  // when using one of the non-LoRa modules for AX.25
  // (RF69, CC1101, Si4432 etc.), use the basic begin() method
  // int state = radio.begin();

  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // initialize APRS client
  Serial.print(F("[APRS] Initializing ... "));
  // symbol:                      '>' (car)
  // callsign                     "N7LEM"
  // SSID                         1
  char source[] = "N7LEM";
  state = aprs.begin('>', source, 1);
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
}

void loop() {
  Serial.print(F("[APRS] Sending position ... "));
  
  // send a location with message and timestamp
  // SSID is set to 1, as APRS over LoRa uses WIDE1-1 path by default
  char destination[] = "GPS";
  char latitude[] = "4911.67N";
  char longitude[] = "01635.96E";
  char message[] = "I'm here!";
  char timestamp[] = "093045z";
  int state = aprs.sendPosition(destination, 1, latitude, longitude, message, timestamp);
  delay(500);

  // you can also send Mic-E encoded messages
  state |= state = aprs.sendMicE(49.1945, 16.6000, 120, 10, RADIOLIB_APRS_MIC_E_TYPE_EN_ROUTE);
  delay(500);

  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  // wait one minute before transmitting again
  delay(60000);
}
