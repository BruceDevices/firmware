#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::setRangingAddr(uint32_t addr, uint8_t checkLen) {
  uint8_t buff[] = { 
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF),
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    checkLen,
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RANGING_ADDR, true, buff, sizeof(buff)));
}

int16_t LR2021::setRangingReqAddr(uint32_t addr) {
  uint8_t buff[] = { 
    (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF),
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RANGING_REQ_ADDR, true, buff, sizeof(buff)));
}

int16_t LR2021::getRangingResult(uint8_t type, uint32_t* rng1, uint8_t* rssi1, uint32_t* rng2) {
  const uint8_t reqBuff[] = { type };
  uint8_t rplBuff[7] = { 0 };

  //! \TODO: [LR2021] implement AGC gains readout
  size_t rplLen = (type == RADIOLIB_LR2021_RANGING_RESULT_TYPE_RAW) ? 7 : 4;
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_RANGING_RESULT, false, rplBuff, sizeof(rplBuff), reqBuff, rplLen);
  
  if(rng1) { *rng1 = ((uint32_t)(rplBuff[0]) << 16) | ((uint32_t)(rplBuff[1]) << 8) | (uint32_t)rplBuff[2]; }
  if(rssi1) { *rssi1 = rplBuff[3]; }
  if(rng2 && (type == RADIOLIB_LR2021_RANGING_RESULT_TYPE_RAW_EXT)) {
    *rng2 = ((uint32_t)(rplBuff[4]) << 16) | ((uint32_t)(rplBuff[5]) << 8) | (uint32_t)rplBuff[6];
  }

  return(state);
}

int16_t LR2021::getRangingStats(uint16_t* exchangeValid, uint16_t* requestValid, uint16_t* responseDone, uint16_t* timeout, uint16_t* requestDiscarded) {
  uint8_t buff[10] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_RANGING_STATS, false, buff, sizeof(buff));
  if(exchangeValid) { *exchangeValid = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  if(requestValid) { *requestValid = ((uint16_t)(buff[2]) << 8) | (uint16_t)buff[3]; }
  if(responseDone) { *responseDone = ((uint16_t)(buff[4]) << 8) | (uint16_t)buff[5]; }
  if(timeout) { *timeout = ((uint16_t)(buff[6]) << 8) | (uint16_t)buff[7]; }
  if(requestDiscarded) { *requestDiscarded = ((uint16_t)(buff[8]) << 8) | (uint16_t)buff[9]; }
  return(state);
}

int16_t LR2021::setRangingTxRxDelay(uint32_t delay) {
  return(this->setU32(RADIOLIB_LR2021_CMD_SET_RANGING_TX_RX_DELAY, delay));
}

int16_t LR2021::setRangingParams(bool spyMode, uint8_t nbSymbols) {
  uint8_t buff[] = { (uint8_t)(((uint8_t)spyMode << 6) | (nbSymbols & 0x3F)) };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RANGING_PARAMS, true, buff, sizeof(buff)));
}

#endif
