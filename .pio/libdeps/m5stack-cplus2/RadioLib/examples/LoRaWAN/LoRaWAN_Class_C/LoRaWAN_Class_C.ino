/*
  RadioLib LoRaWAN Class C Example

  This example joins a LoRaWAN network and switches to Class C.
  Note that a confirmed uplink with a confirming downlink is
  required for the switch to Class C to complete. This example
  assumes that coverage is good enough to receive the downlink
  at once. It is up to you to handle the situation if coverage
  is worse.

  Running this examples REQUIRES you to check "Resets DevNonces"
  on your LoRaWAN dashboard. Refer to the network's 
  documentation on how to do this.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/

  For LoRaWAN details, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/LoRaWAN

*/

#include "config.h"

void setup() {
  Serial.begin(115200);
  while(!Serial);
  delay(5000);  // Give time to switch to the serial monitor
  Serial.println(F("\nSetup ... "));

  Serial.println(F("Initialise the radio"));
  int16_t state = radio.begin();
  debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  // Setup the OTAA session information
  state = node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  debug(state != RADIOLIB_ERR_NONE, F("Initialise node failed"), state, true);

  Serial.println(F("Join ('login') the LoRaWAN Network"));
  state = node.activateOTAA();
  debug(state != RADIOLIB_LORAWAN_NEW_SESSION, F("Join failed"), state, true);

  // switch class
  node.setClass(RADIOLIB_LORAWAN_CLASS_C);

  // read the note at the top about this first confirmed uplink
  const char* payload = "C";
  Serial.println(F("Sending a confirmed uplink"));
  state = node.sendReceive(payload, 1, true);
  debug(state <= 0, F("No downlink received"), state, true);

  Serial.println(F("Ready!\n"));
}

uint32_t lastUplink = 0;

void loop() {
  uint8_t downlinkPayload[255];
  size_t downlinkLen = 0;
  LoRaWANEvent_t downlinkEvent;

  // check if a Class C downlink is ready for processing
  // tip: internally, this just checks a boolean; 
  //      it does not poll the radio over SPI.
  // tip: you are not required to continuously call
  //      this function; you can do other stuff in between.
  //      however, a downlink may be overwritten if you 
  //      don't call this function in time for the previous one.
  int16_t state = node.getDownlinkClassC(downlinkPayload, &downlinkLen, &downlinkEvent);
  if(state > 0) {
    Serial.println(F("Received a Class C downlink!"));
    // Did we get a downlink with data for us
    if(downlinkLen > 0) {
      Serial.println(F("Downlink data: "));
      arrayDump(downlinkPayload, downlinkLen);
    }

    // print extra information about the event
    Serial.println(F("[LoRaWAN] Event information:"));
    Serial.print(F("[LoRaWAN] Datarate:\t"));
    Serial.println(downlinkEvent.datarate);
    Serial.print(F("[LoRaWAN] Frequency:\t"));
    Serial.print(downlinkEvent.freq, 3);
    Serial.println(F(" MHz"));
    Serial.print(F("[LoRaWAN] Frame count:\t"));
    Serial.println(downlinkEvent.fCnt);
    Serial.print(F("[LoRaWAN] Port:\t\t"));
    Serial.println(downlinkEvent.fPort);
    Serial.println(F(" ms"));
    Serial.print(F("[LoRaWAN] Rx window: \t"));
    Serial.println(state);
    Serial.print(F("[LoRaWAN] Cast:\t\t"));
    Serial.println(downlinkEvent.multicast ? "Multi" : "Uni");
  }

  // if less than uplinkIntervalSeconds have elapsed since previous uplink,
  // stop and go back to the top of the loop()
  if(millis() - lastUplink < uplinkIntervalSeconds * 1000) {
    return;
  }

  Serial.println(F("Sending uplink"));

  // This is the place to gather the sensor inputs
  // Instead of reading any real sensor, we just generate some random numbers as example
  uint8_t value1 = radio.random(100);
  uint16_t value2 = radio.random(2000);

  // Build payload byte array
  uint8_t uplinkPayload[3];
  uplinkPayload[0] = value1;
  uplinkPayload[1] = highByte(value2);   // See notes for high/lowByte functions
  uplinkPayload[2] = lowByte(value2);
  
  // Perform an uplink
  state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload));    
  debug(state < RADIOLIB_ERR_NONE, F("Error in sendReceive"), state, false);

  // Check if a downlink was received 
  // (state 0 = no downlink, state 1/2/3 = downlink in window Rx1/Rx2/RxC)
  if(state > 0) {
    Serial.println(F("Received a downlink"));
  } else {
    Serial.println(F("No downlink received"));
  }

  Serial.print(F("Next uplink in "));
  Serial.print(uplinkIntervalSeconds);
  Serial.println(F(" seconds\n"));

  // set timestamp of last uplink
  lastUplink = millis();
}
