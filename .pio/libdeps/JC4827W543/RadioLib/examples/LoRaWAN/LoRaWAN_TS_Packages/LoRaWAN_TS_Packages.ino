/*
  RadioLib LoRaWAN Packages Example

  This example shows how the TS009 package can be used
  and is the sketch used for passing pre-certification testing.
  This scheme can also be used for other future packages
  such as TS003/004/005/006/007 which, combined, build FUOTA.

  PLEASE NOTE that this is a highly customized sketch with
  settings that likely violate laws & regulations, and it is
  intended to be used with RF blocking materials and attenuators.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN

*/

#include <Arduino.h>
#include <RadioLib.h>

#include "LoRaWAN_TS009.h"
#include "config.h"
#include "lorawan.h"

uint32_t periodicity = uplinkIntervalSeconds;
bool isConfirmed = false;
bool reply = false;

uint8_t fPort = 1;
uint8_t dataUp[255];
uint8_t dataDown[255];
size_t lenUp = 0;
size_t lenDown = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.print(F("Initialise the radio ... "));
  int state = radio.begin();
  if(state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while(true);
  }

  // setup, restore and join the network
  lwBegin();
  lwRestore();
  lwActivate();

  // add TS009 package
  node.addAppPackage(RADIOLIB_LORAWAN_PACKAGE_TS009, handleTS009);
  
  // LCTT (TS009 testing) has a huge timing problem on the JoinAccept Rx2 window...
  node.scanGuard = 100;

  // these settings are totally not recommended
  // but unfortunately they are the default settings for TS009 testing
  node.setDutyCycle(false);
  node.setADR(false);
}

void loop() {
  while(!node.isActivated()) {
    lwActivate();
    // this 5s interval is way too short for normal use!
    // but you'd be waiting around for long minutes during TS009 testing otherwise
    if(!node.isActivated()) {
      delay(5000);
    }
    node.setDutyCycle(false);
  }

  int state = RADIOLIB_ERR_NONE;
  LoRaWANEvent_t eventUp;
  LoRaWANEvent_t eventDown;

  uint32_t start = millis();

  Serial.println("--------------------");
  Serial.println("[LoRaWAN] Sending uplink packet ... ");
  if (!reply) {
    memset(dataUp, 0, 255);
    lenUp = 4;
    fPort = 1;
    sprintf((char*)dataUp, "%04lu", node.getFCntUp());
    state = node.sendReceive(dataUp, lenUp, fPort, dataDown, &lenDown, isConfirmed, &eventUp, &eventDown);
  } else {
    reply = false;
    state = node.sendReceive(dataUp, lenUp, fPort, dataDown, &lenDown, isConfirmed, &eventUp, &eventDown);
  }
  
  if(state >= RADIOLIB_ERR_NONE) {
    Serial.println(F("[LoRaWAN] success!"));
  }
  
  if(state > 0) {
    // print data of the packet (if there are any)
    Serial.print(F("[LoRaWAN] Data:\t\t"));
    if(lenDown > 0) {
      arrayDump(dataDown, lenDown);
    } else {
      Serial.println(F("<MAC / package commands only>"));
    }
    
    Serial.print(F("[LoRaWAN] FPort:\t"));
    Serial.print(eventDown.fPort);

    // print RSSI (Received Signal Strength Indicator)
    Serial.print(F("[LoRaWAN] RSSI:\t\t"));
    Serial.print(radio.getRSSI());
    Serial.println(F(" dBm"));

    // print SNR (Signal-to-Noise Ratio)
    Serial.print(F("[LoRaWAN] SNR:\t\t"));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));

    uint8_t margin = 0;
    uint8_t gwCnt = 0;
    if(node.getMacLinkCheckAns(&margin, &gwCnt) == RADIOLIB_ERR_NONE) {
      Serial.print(F("[LoRaWAN] LinkCheck margin:\t"));
      Serial.println(margin);
      Serial.print(F("[LoRaWAN] LinkCheck count:\t"));
      Serial.println(gwCnt);
    }

    uint32_t timestamp = 0;
    uint16_t milliseconds = 0;
    if(node.getMacDeviceTimeAns(&timestamp, &milliseconds, true) == RADIOLIB_ERR_NONE) {
      Serial.print(F("[LoRaWAN] DeviceTime Unix:\t"));
      Serial.println(timestamp);
      Serial.print(F("[LoRaWAN] DeviceTime frac:\t"));
      Serial.print(milliseconds);
      Serial.println(F(" ms"));
    }
  
  } else if(state == 0) {
    Serial.println(F("No downlink!"));
  
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }

  uint32_t end = millis();

  uint32_t delayDc = node.timeUntilUplink();
  uint32_t delayMs = periodicity*1000;
  if(delayMs > end - start) {
    delayMs -= (end - start);
  } else {
    delayMs = 1;
  }
  delayMs += 50;
  Serial.print(F("Delay: "));
  Serial.print(max(delayDc, delayMs));
  Serial.println(F(" ms"));

  // wait before sending another packet
  delay(max(delayDc, delayMs));
}