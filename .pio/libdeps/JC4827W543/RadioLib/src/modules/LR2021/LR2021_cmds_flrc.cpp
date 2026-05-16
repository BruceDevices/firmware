#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::setFlrcModulationParams(uint8_t brBw, uint8_t cr, uint8_t pulseShape) {
  uint8_t buff[] = { brBw, (uint8_t)((cr << 4) | (pulseShape & 0x0F)) };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_FLRC_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setFlrcPacketParams(uint8_t agcPreambleLen, uint8_t syncWordLen, uint8_t syncWordTx, uint8_t syncMatch, bool fixedLength, uint8_t crc, uint16_t payloadLen) {
  uint8_t buff[] = { 
    (uint8_t)(((agcPreambleLen & 0x0F) << 2) | (syncWordLen / 2)),
    (uint8_t)(((syncWordTx & 0x03) << 6) | ((syncMatch & 0x07) << 3) | ((uint8_t)fixedLength << 2) | (crc & 0x03)),
    (uint8_t)((payloadLen >> 8) & 0xFF), (uint8_t)(payloadLen & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_FLRC_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::getFlrcRxStats(uint16_t* packetRx, uint16_t* packetCrcError, uint16_t* lenError) {
  uint8_t buff[14] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_FLRC_RX_STATS, false, buff, sizeof(buff));
  if(packetRx) { *packetRx = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  if(packetCrcError) { *packetCrcError = ((uint16_t)(buff[2]) << 8) | (uint16_t)buff[3]; }
  if(lenError) { *lenError = ((uint16_t)(buff[4]) << 8) | (uint16_t)buff[5]; }
  return(state);
}

int16_t LR2021::getFlrcPacketStatus(uint16_t* packetLen, float* rssiAvg, float* rssiSync, uint8_t* syncWordNum) {
  uint8_t buff[5] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_FLRC_PACKET_STATUS, false, buff, sizeof(buff));
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
  if(syncWordNum) { *syncWordNum = (buff[4] & 0xF0) >> 4; }
  return(state);
}

int16_t LR2021::setFlrcSyncWord(uint8_t syncWordNum, uint32_t syncWord) {
  uint8_t buff[] = { 
    syncWordNum,
    (uint8_t)((syncWord >> 24) & 0xFF), (uint8_t)((syncWord >> 16) & 0xFF),
    (uint8_t)((syncWord >> 8) & 0xFF), (uint8_t)(syncWord & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_FLRC_SYNCWORD, true, buff, sizeof(buff)));
}

#endif
