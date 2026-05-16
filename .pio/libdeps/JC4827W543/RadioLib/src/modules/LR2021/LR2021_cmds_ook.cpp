#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::setOokModulationParams(uint32_t bitRate, uint8_t pulseShape, uint8_t rxBw, uint8_t depth) {
  uint8_t buff[] = { 
    (uint8_t)((bitRate >> 24) & 0xFF), (uint8_t)((bitRate >> 16) & 0xFF),
    (uint8_t)((bitRate >> 8) & 0xFF), (uint8_t)(bitRate & 0xFF),
    pulseShape, rxBw, depth,
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_OOK_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setOokPacketParams(uint16_t preambleLen, uint8_t addrComp, uint8_t packetFormat, uint16_t payloadLen, uint8_t crc, uint8_t manchester) {
  uint8_t buff[] = { 
    (uint8_t)((preambleLen >> 8) & 0xFF), (uint8_t)(preambleLen & 0xFF),
    (uint8_t)((addrComp << 2) | ((uint8_t)packetFormat & 0x03)),
    (uint8_t)((payloadLen >> 8) & 0xFF), (uint8_t)(payloadLen & 0xFF),
    (uint8_t)(((crc << 4) & 0xF0) | (manchester & 0x0F)),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_OOK_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setOokCrcParams(uint32_t poly, uint32_t init) {
  uint8_t buff[] = { 
    (uint8_t)((poly >> 24) & 0xFF), (uint8_t)((poly >> 16) & 0xFF),
    (uint8_t)((poly >> 8) & 0xFF), (uint8_t)(poly & 0xFF),
    (uint8_t)((init >> 24) & 0xFF), (uint8_t)((init >> 16) & 0xFF),
    (uint8_t)((init >> 8) & 0xFF), (uint8_t)(init & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_OOK_CRC_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setOokSyncword(const uint8_t* syncWord, size_t syncWordLen, bool msbFirst) {
  uint8_t buff[5] = { 0 };
  for(size_t i = 0; i < syncWordLen; i++) {
    buff[3 - i] = syncWord[i];
  }
  buff[4] = (uint8_t)msbFirst << 7 | ((syncWordLen*8) & 0x7F);
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_OOK_SYNCWORD, true, buff, sizeof(buff)));
}

int16_t LR2021::setOokAddress(uint8_t addrNode, uint8_t addrBroadcast) {
  uint8_t buff[] = { addrNode, addrBroadcast };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_OOK_ADDRESS, true, buff, sizeof(buff)));
}

int16_t LR2021::getOokRxStats(uint16_t* packetRx, uint16_t* crcError, uint16_t* lenError) {
  uint8_t buff[6] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_OOK_RX_STATS, false, buff, sizeof(buff));
  if(packetRx) { *packetRx = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  if(crcError) { *crcError = ((uint16_t)(buff[2]) << 8) | (uint16_t)buff[3]; }
  if(lenError) { *lenError = ((uint16_t)(buff[4]) << 8) | (uint16_t)buff[5]; }
  return(state);
}

int16_t LR2021::getOokPacketStatus(uint16_t* packetLen, float* rssiAvg, float* rssiSync, bool* addrMatchNode, bool* addrMatchBroadcast, float* lqi) {
  uint8_t buff[6] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_OOK_PACKET_STATUS, false, buff, sizeof(buff));
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

int16_t LR2021::setOokDetector(uint16_t preamblePattern, uint8_t patternLen, uint8_t patternNumRepeaters, bool syncWordRaw, bool sofDelimiterRising, uint8_t sofDelimiterLen) {
  uint8_t buff[] = { 
    (uint8_t)((preamblePattern >> 8) & 0xFF), (uint8_t)(preamblePattern & 0xFF),
    (uint8_t)(patternLen & 0x0F), (uint8_t)(patternNumRepeaters & 0x1F),
    (uint8_t)(((uint8_t)syncWordRaw << 5) | ((uint8_t)sofDelimiterRising << 4) | (sofDelimiterLen & 0x0F)),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_OOK_DETECTOR, true, buff, sizeof(buff)));
}

int16_t LR2021::setOokWhiteningParams(uint8_t bitIdx, uint16_t poly, uint16_t init) {
  uint8_t buff[] = {
    (uint8_t)((bitIdx << 4) | ((poly >> 8) & 0x0FF)), (uint8_t)(poly & 0xFF),
     (uint8_t)((init >> 8) & 0xFF), (uint8_t)(init & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_OOK_WHITENING_PARAMS, true, buff, sizeof(buff)));
}

#endif
