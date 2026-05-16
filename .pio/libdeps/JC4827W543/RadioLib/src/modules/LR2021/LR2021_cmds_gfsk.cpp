#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::setGfskModulationParams(uint32_t bitRate, uint8_t pulseShape, uint8_t rxBw, uint32_t freqDev) {
  uint8_t buff[] = { 
    (uint8_t)((bitRate >> 24) & 0xFF), (uint8_t)((bitRate >> 16) & 0xFF),
    (uint8_t)((bitRate >> 8) & 0xFF), (uint8_t)(bitRate & 0xFF),
    pulseShape, rxBw, (uint8_t)((freqDev >> 16) & 0xFF),
    (uint8_t)((freqDev >> 8) & 0xFF), (uint8_t)(freqDev & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_GFSK_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setGfskPacketParams(uint16_t preambleLen, uint8_t preambleDetect, bool longPreamble, bool pldLenBits, uint8_t addrComp, uint8_t packetFormat, uint16_t payloadLen, uint8_t crc, uint8_t dcFree) {
  uint8_t buff[] = { 
    (uint8_t)((preambleLen >> 8) & 0xFF), (uint8_t)(preambleLen & 0xFF), preambleDetect,
    (uint8_t)(((uint8_t)longPreamble << 5) | ((uint8_t)pldLenBits << 4) | (addrComp << 2) | ((uint8_t)packetFormat & 0x03)),
    (uint8_t)((payloadLen >> 8) & 0xFF), (uint8_t)(payloadLen & 0xFF),
    (uint8_t)((crc << 4) | (dcFree << 4)),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_GFSK_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setGfskWhiteningParams(uint8_t whitenType, uint16_t init) {
  uint8_t buff[] = { 
    (uint8_t)((whitenType << 4) | (uint8_t)((init >> 8) & 0x0F)),
    (uint8_t)(init & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_GFSK_WHITENING_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setGfskCrcParams(uint32_t poly, uint32_t init) {
  uint8_t buff[] = { 
    (uint8_t)((poly >> 24) & 0xFF), (uint8_t)((poly >> 16) & 0xFF),
    (uint8_t)((poly >> 8) & 0xFF), (uint8_t)(poly & 0xFF),
    (uint8_t)((init >> 24) & 0xFF), (uint8_t)((init >> 16) & 0xFF),
    (uint8_t)((init >> 8) & 0xFF), (uint8_t)(init & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_GFSK_CRC_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setGfskSyncword(const uint8_t* syncWord, size_t syncWordLen, bool msbFirst) {
  uint8_t buff[9] = { 0 };
  for(int8_t i = 7; i >= (int8_t) (RADIOLIB_LR2021_GFSK_SYNC_WORD_LEN - syncWordLen); i--) {
    buff[i] = syncWord[i - (int8_t) (RADIOLIB_LR2021_GFSK_SYNC_WORD_LEN - syncWordLen)];
  }
  buff[8] = (uint8_t)msbFirst << 7 | ((syncWordLen*8) & 0x7F);
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_GFSK_SYNCWORD, true, buff, sizeof(buff)));
}

int16_t LR2021::setGfskAddress(uint8_t addrNode, uint8_t addrBroadcast) {
  uint8_t buff[] = { addrNode, addrBroadcast };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_GFSK_ADDRESS, true, buff, sizeof(buff)));
}

int16_t LR2021::getGfskRxStats(uint16_t* packetRx, uint16_t* packetCrcError, uint16_t* lenError, uint16_t* preambleDet, uint16_t* syncOk, uint16_t* syncFail, uint16_t* timeout) {
  uint8_t buff[14] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_GFSK_RX_STATS, false, buff, sizeof(buff));
  if(packetRx) { *packetRx = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  if(packetCrcError) { *packetCrcError = ((uint16_t)(buff[2]) << 8) | (uint16_t)buff[3]; }
  if(lenError) { *lenError = ((uint16_t)(buff[4]) << 8) | (uint16_t)buff[5]; }
  if(preambleDet) { *preambleDet = ((uint16_t)(buff[6]) << 8) | (uint16_t)buff[7]; }
  if(syncOk) { *syncOk = ((uint16_t)(buff[8]) << 8) | (uint16_t)buff[9]; }
  if(syncFail) { *syncFail = ((uint16_t)(buff[10]) << 8) | (uint16_t)buff[11]; }
  if(timeout) { *timeout = ((uint16_t)(buff[12]) << 8) | (uint16_t)buff[13]; }
  return(state);
}

int16_t LR2021::getGfskPacketStatus(uint16_t* packetLen, float* rssiAvg, float* rssiSync, bool* addrMatchNode, bool* addrMatchBroadcast, float* lqi) {
  uint8_t buff[6] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_GFSK_PACKET_STATUS, false, buff, sizeof(buff));
  uint16_t raw;
  if(packetLen) { *packetLen = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  if(rssiAvg) {
    raw = (uint16_t)buff[2] << 1;
    raw |= (buff[4] & 0x04) >> 2;
    *rssiAvg = (float)raw / -2.0f;
  }
  if(rssiSync) {
    raw = (uint16_t)buff[3] << 1;
    raw |= (buff[4] & 0x01);
    *rssiSync = (float)raw / -2.0f;
  }
  if(addrMatchNode) { *addrMatchNode = (buff[4] & 0x10); }
  if(addrMatchBroadcast) { *addrMatchBroadcast = (buff[4] & 0x20); }
  if(lqi) { *lqi = buff[5] * 4.0f; }
  return(state);
}

#endif
