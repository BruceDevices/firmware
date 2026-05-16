#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::setLoRaModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro) {
  // calculate symbol length and enable low data rate optimization, if auto-configuration is enabled
  if(this->ldroAuto) {
    float symbolLength = (float)(uint32_t(1) << this->spreadingFactor) / (float)this->bandwidthKhz;
    if(symbolLength >= 16.0f) {
      this->ldrOptimize = RADIOLIB_LR2021_LORA_LDRO_ENABLED;
    } else {
      this->ldrOptimize = RADIOLIB_LR2021_LORA_LDRO_DISABLED;
    }
  } else {
    this->ldrOptimize = ldro;
  }

  uint8_t buff[] = { (uint8_t)(((sf & 0x0F) << 4) | (bw & 0x0F)), (uint8_t)(((cr & 0x0F) << 4) | this->ldrOptimize) };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_MODULATION_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setLoRaPacketParams(uint16_t preambleLen, uint8_t hdrType, uint8_t payloadLen, uint8_t crcType, uint8_t invertIQ) {
  uint8_t buff[] = { 
    (uint8_t)((preambleLen >> 8) & 0xFF), (uint8_t)(preambleLen & 0xFF), payloadLen,
    (uint8_t)(((hdrType & 0x01) << 2) | ((crcType & 0x01) << 1) | (invertIQ & 0x01)),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_PACKET_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setLoRaSynchTimeout(uint8_t numSymbols, bool format) {
  uint8_t buff[] = { numSymbols, (uint8_t)format };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_SYNCH_TIMEOUT, true, buff, sizeof(buff)));
}

int16_t LR2021::setLoRaSyncword(uint8_t syncword) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_SYNCWORD, true, &syncword, sizeof(syncword)));
}

int16_t LR2021::setLoRaSideDetConfig(uint8_t* configs, size_t numSideDets) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_SIDE_DET_CONFIG, true, configs, numSideDets));
}

int16_t LR2021::setLoRaSideDetSyncword(uint8_t* syncwords, size_t numSideDets) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_SIDE_DET_SYNCWORD, true, syncwords, numSideDets));
}

int16_t LR2021::setLoRaCadParams(uint8_t numSymbols, bool preambleOnly, uint8_t pnrDelta, uint8_t cadExitMode, uint32_t timeout, uint8_t detPeak) {
  uint8_t buff[] = {
    numSymbols, (uint8_t)(((uint8_t)preambleOnly << 4) | (pnrDelta & 0x0F)), cadExitMode, 
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
    (uint8_t)(detPeak & 0x7F)
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_CAD_PARAMS, true, buff, sizeof(buff)));
}

int16_t LR2021::setLoRaCad(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_CAD, true, NULL, 0));
}

int16_t LR2021::getLoRaRxStats(uint16_t* pktRxTotal, uint16_t* pktCrcError, uint16_t* headerCrcError, uint16_t* falseSynch) {
  uint8_t buff[8] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_LORA_RX_STATS, false, buff, sizeof(buff));
  if(pktRxTotal) { *pktRxTotal = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  if(pktCrcError) { *pktCrcError = ((uint16_t)(buff[2]) << 8) | (uint16_t)buff[3]; }
  if(headerCrcError) { *headerCrcError = ((uint16_t)(buff[4]) << 8) | (uint16_t)buff[5]; }
  if(falseSynch) { *falseSynch = ((uint16_t)(buff[7]) << 8) | (uint16_t)buff[6]; }
  return(state);
}

int16_t LR2021::getLoRaPacketStatus(uint8_t* crc, uint8_t* cr, uint8_t* packetLen, float* snrPacket, float* rssiPacket, float* rssiSignalPacket) {
  uint8_t buff[6] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_LORA_PACKET_STATUS, false, buff, sizeof(buff));
  uint16_t raw;
  if(crc) { *crc = (buff[0] & 0x10) >> 4; }
  if(cr) { *cr = buff[0] & 0x0F; }
  if(packetLen) { *packetLen = buff[1]; }
  if(snrPacket) { *snrPacket = (float)((int8_t)buff[2]) / 4.0f; }
  if(rssiPacket) {
    raw = (uint16_t)buff[3] << 1;
    raw |= (buff[5] & 0x02) >> 1;
    *rssiPacket = (float)raw / -2.0f;
  }
  if(rssiSignalPacket) {
    raw = (uint16_t)buff[4] << 1;
    raw |= buff[5] & 0x01;
    *rssiSignalPacket = (float)raw / -2.0f;
  }
  return(state);
}

int16_t LR2021::setLoRaAddress(uint8_t addrLen, uint8_t addrPos, const uint8_t* addr) {
  if(addrLen > 8) { return(RADIOLIB_ERR_UNKNOWN); }
  uint8_t buff[9] = { (uint8_t)(((addrLen & 0x0F) << 4) | (addrPos & 0x0F)) };
  memcpy(&buff[1], addr, addrLen);
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_ADDRESS, true, buff, sizeof(buff)));
}

int16_t LR2021::setLoRaHopping(uint8_t hopCtrl, uint16_t hopPeriod, const uint32_t* freqHops, size_t numFreqHops) {
  if(numFreqHops > 40) { return(RADIOLIB_ERR_UNKNOWN); }
  uint8_t buff[2 + 160] = { (uint8_t)(hopCtrl | ((hopPeriod & 0xF00) >> 8)), (uint8_t)(hopPeriod & 0xFF) };
  for(uint8_t i = 0; i < numFreqHops; i++) {
    buff[i + 2] = (freqHops[i] >> 24) & 0xFF;
    buff[i + 3] = (freqHops[i] >> 16) & 0xFF;
    buff[i + 4] = (freqHops[i] >> 8) & 0xFF;
    buff[i + 5] = freqHops[i] & 0xFF;
  }
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_HOPPING, true, buff, sizeof(buff)));
}

int16_t LR2021::setLoRaTxSync(uint8_t function, uint8_t dioNum) {
  uint8_t buff[] = { (uint8_t)(function | (dioNum & 0x3F)) };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_TX_SYNC, true, buff, sizeof(buff)));
}

int16_t LR2021::setLoRaSideDetCad(const uint8_t* pnrDelta, const uint8_t* detPeak, size_t numSideDets) {
  uint8_t buff[6] = { 0 };
  for(uint8_t i = 0; i < numSideDets; i++) {
    if(i >= 3) { return(RADIOLIB_ERR_UNKNOWN); }
    buff[2*i] = pnrDelta[i] & 0x0F;
    buff[2*i + 1] = detPeak[i] & 0x7F;
  }
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_LORA_SIDE_DET_CAD, true, buff, 2*numSideDets));
}

#endif
