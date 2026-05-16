#ifndef _LORAWAN_H
#define _LORAWAN_H

#include <RadioLib.h>
#include "config.h"

#warning "You are required to implement persistence here! (ESP32 example provided in comments)"

// #include <Preferences.h>
// Preferences store;
// uint8_t LWnonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];

bool lwBegin() {
#if (LORAWAN_OTAA == 1)
  #if (LORAWAN_VERSION == 1)
    node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  #else
    node.beginOTAA(joinEUI, devEUI, NULL, appKey);
  #endif
#else
  #if (LORAWAN_VERSION == 1)
    node.beginABP(devAddr, fNwkSIntKey, sNwkSIntKey, sNwkSEncKey, appSKey);
  #else
    node.beginABP(devAddr, NULL, NULL, sNwkSEncKey, appSKey);
  #endif
#endif
  return(true);
}

int16_t lwRestore() {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // store.begin("radiolib");
  // if (store.isKey("nonces")) {
  //   radio.standby();

  //   store.getBytes("nonces", LWnonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
  //   state = node.setBufferNonces(LWnonces);
  // }
  // store.end();

  return(state);
}

void lwActivate() {
  int16_t state = RADIOLIB_ERR_NETWORK_NOT_JOINED;
  Serial.println(F("[LoRaWAN] Attempting network join ... "));

  radio.standby();
  
#if (LORAWAN_OTAA == 1)
    state = node.activateOTAA();
#else
    state = node.activateABP();
#endif

  if(state == RADIOLIB_LORAWAN_SESSION_RESTORED) {
    Serial.println(F("[LoRaWAN] Session restored!"));
    return;
  } 

  // store.begin("radiolib");
  // uint8_t *persist = node.getBufferNonces();
  // store.putBytes("nonces", persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
  // store.end();

  if(state == RADIOLIB_LORAWAN_NEW_SESSION) {
    Serial.println(F("[LoRaWAN] Successfully started new session!"));
    return;
  }

  Serial.print(F("[LoRaWAN] Failed, code "));
  Serial.println(state);
}

#endif