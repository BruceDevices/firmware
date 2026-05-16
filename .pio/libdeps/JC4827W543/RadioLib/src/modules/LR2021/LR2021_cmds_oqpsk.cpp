#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::setOqpskParams(uint8_t mode, uint8_t rxBw, uint8_t payloadLen, uint16_t preambleLen, bool addrFilt, bool fcsManual) {
  uint8_t buff[] = { 
    mode, rxBw, payloadLen,
    (uint8_t)((preambleLen >> 8) & 0xFF), (uint8_t)(preambleLen & 0xFF),
    (uint8_t)((uint8_t)addrFilt << 1 | (uint8_t)fcsManual),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_OQPSK_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::getOqpskRxStats(uint16_t* packetRx, uint16_t* crcError, uint16_t* lenError) {
  uint8_t buff[6] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_OQPSK_RX_STATS, false, buff, sizeof(buff));
  if(packetRx) { *packetRx = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  if(crcError) { *crcError = ((uint16_t)(buff[2]) << 8) | (uint16_t)buff[3]; }
  if(lenError) { *lenError = ((uint16_t)(buff[4]) << 8) | (uint16_t)buff[5]; }
  return(state);
}

int16_t LR2021::getOqpskPacketStatus(uint8_t* rxHeader, uint16_t* payloadLen, float* rssiAvg, float* rssiSync, float* lqi) {
  uint8_t buff[7] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_OQPSK_PACKET_STATUS, false, buff, sizeof(buff));
  if(rxHeader) { *rxHeader = buff[0]; }
  if(payloadLen) { *payloadLen = ((uint16_t)(buff[1]) << 8) | (uint16_t)buff[2]; }
  uint16_t raw;
  if(rssiAvg) {
    raw = (uint16_t)buff[3] << 1;
    raw |= (buff[5] & 0x04) >> 2;
    *rssiAvg = (float)raw / -2.0f;
  }
  if(rssiSync) {
    raw = (uint16_t)buff[4] << 1;
    raw |= (buff[5] & 0x01);
    *rssiSync = (float)raw / -2.0f;
  }
  if(lqi) { *lqi = buff[6] * 4.0f; }
  return(state);
}

int16_t LR2021::setOqpskPacketLen(uint8_t len) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_OQPSK_PACKET_LEN, true, &len, sizeof(len)));
}

int16_t LR2021::setOqpskAddress(const uint8_t longDestAddr[8], uint16_t shortDestAddr, uint16_t panId, uint8_t transId) {
  uint8_t buff[] = { 
    longDestAddr[7], longDestAddr[6], longDestAddr[5], longDestAddr[4], 
    longDestAddr[3], longDestAddr[2], longDestAddr[1], longDestAddr[0],
    (uint8_t)((shortDestAddr >> 8) & 0xFF), (uint8_t)(shortDestAddr & 0xFF),
    (uint8_t)((panId >> 8) & 0xFF), (uint8_t)(panId & 0xFF), transId,
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_OQPSK_ADDRESS, true, buff, sizeof(buff)));
}

#endif
