#include "LR11x0.h"

#include <string.h>

#if !RADIOLIB_EXCLUDE_LR11X0

int16_t LR11x0::startWifiScan(char wifiType, uint8_t mode, uint16_t chanMask, uint8_t numScans, uint16_t timeout) {
  // LR1121 cannot do WiFi scanning
  if(this->chipType == RADIOLIB_LR11X0_DEVICE_LR1121) {
    return(RADIOLIB_ERR_UNSUPPORTED);
  }

  uint8_t type;
  switch(wifiType) {
    case('b'):
      type = RADIOLIB_LR11X0_WIFI_SCAN_802_11_B;
      break;
    case('g'):
      type = RADIOLIB_LR11X0_WIFI_SCAN_802_11_G;
      break;
    case('n'):
      type = RADIOLIB_LR11X0_WIFI_SCAN_802_11_N;
      break;
    case('*'):
      type = RADIOLIB_LR11X0_WIFI_SCAN_ALL;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_WIFI_TYPE);
  }

  // go to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // reset cumulative timings
  state = wifiResetCumulTimings();
  RADIOLIB_ASSERT(state);

  // set DIO mapping
  state = setDioIrqParams(RADIOLIB_LR11X0_IRQ_WIFI_DONE);
  RADIOLIB_ASSERT(state);

  // start scan with the maximum number of results and abort on timeout
  this->wifiScanMode = mode;
  state = wifiScan(type, chanMask, this->wifiScanMode, RADIOLIB_LR11X0_WIFI_MAX_NUM_RESULTS, numScans, timeout, RADIOLIB_LR11X0_WIFI_ABORT_ON_TIMEOUT_ENABLED);
  return(state);
}

void LR11x0::setWiFiScanAction(void (*func)(void)) {
  this->setIrqAction(func);
}

void LR11x0::clearWiFiScanAction() {
  this->clearIrqAction();
}

int16_t LR11x0::getWifiScanResultsCount(uint8_t* count) {
  // clear IRQ first, as this is likely to be called right after scan has finished
  int16_t state = clearIrqState(RADIOLIB_LR11X0_IRQ_ALL);
  RADIOLIB_ASSERT(state);

  uint8_t buff[1] = { 0 };
  state = this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_GET_NB_RESULTS, false, buff, sizeof(buff));

  // pass the replies
  if(count) { *count = buff[0]; }

  return(state);
}

int16_t LR11x0::getWifiScanResult(LR11x0WifiResult_t* result, uint8_t index, bool brief) {
  RADIOLIB_ASSERT_PTR(result);

  // read a single result
  uint8_t format = brief ? RADIOLIB_LR11X0_WIFI_RESULT_TYPE_BASIC : RADIOLIB_LR11X0_WIFI_RESULT_TYPE_COMPLETE;
  uint8_t raw[RADIOLIB_LR11X0_WIFI_RESULT_MAX_LEN] = { 0 };
  int16_t state = wifiReadResults(index, 1, format, raw);
  RADIOLIB_ASSERT(state);

  // parse the information
  switch(raw[0] & 0x03) {
    case(RADIOLIB_LR11X0_WIFI_SCAN_802_11_B):
      result->type = 'b';
      break;
    case(RADIOLIB_LR11X0_WIFI_SCAN_802_11_G):
      result->type = 'g';
      break;
    case(RADIOLIB_LR11X0_WIFI_SCAN_802_11_N):
      result->type = 'n';
      break;
  }
  result->dataRateId = (raw[0] & 0xFC) >> 2;
  result->channelFreq = 2407 + (raw[1] & 0x0F)*5;
  result->origin = (raw[1] & 0x30) >> 4;
  result->ap = (raw[1] & 0x40) != 0;
  result->rssi = (float)raw[2] / -2.0f;;
  memcpy(result->mac, &raw[3], RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN);

  if(!brief) {
    if(this->wifiScanMode == RADIOLIB_LR11X0_WIFI_ACQ_MODE_FULL_BEACON) {
      LR11x0WifiResultExtended_t* resultExtended = reinterpret_cast<LR11x0WifiResultExtended_t*>(result);
      resultExtended->rate = raw[3];
      resultExtended->service = (((uint16_t)raw[4] << 8) | ((uint16_t)raw[5]));
      resultExtended->length = (((uint16_t)raw[6] << 8) | ((uint16_t)raw[7]));
      resultExtended->frameType = raw[9] & 0x03;
      resultExtended->frameSubType = (raw[9] & 0x3C) >> 2;
      resultExtended->toDistributionSystem = (raw[9] & 0x40) != 0;
      resultExtended->fromDistributionSystem = (raw[9] & 0x80) != 0;
      memcpy(resultExtended->mac0, &raw[10], RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN);
      memcpy(resultExtended->mac, &raw[16], RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN);
      memcpy(resultExtended->mac2, &raw[22], RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN);
      resultExtended->timestamp = (((uint64_t)raw[28] << 56) | ((uint64_t)raw[29] << 48)) | 
                                  (((uint64_t)raw[30] << 40) | ((uint64_t)raw[31] << 32)) | 
                                  (((uint64_t)raw[32] << 24) | ((uint64_t)raw[33] << 16)) | 
                                  (((uint64_t)raw[34] << 8) | (uint64_t)raw[35]);
      resultExtended->periodBeacon = (((uint16_t)raw[36] << 8) | ((uint16_t)raw[37])) * 1024UL;
      resultExtended->seqCtrl = (((uint16_t)raw[38] << 8) | ((uint16_t)raw[39]));
      memcpy(resultExtended->ssid, &raw[40], RADIOLIB_LR11X0_WIFI_RESULT_SSID_LEN);
      resultExtended->currentChannel = raw[72];
      memcpy(resultExtended->countryCode, &raw[73], 2);
      resultExtended->countryCode[2] = '\0';
      resultExtended->ioReg = raw[75];
      resultExtended->fcsCheckOk = (raw[76] != 0);
      resultExtended->phiOffset = (((uint16_t)raw[77] << 8) | ((uint16_t)raw[78]));
      return(RADIOLIB_ERR_NONE);
    }

    LR11x0WifiResultFull_t* resultFull = reinterpret_cast<LR11x0WifiResultFull_t*>(result);
    resultFull->frameType = raw[3] & 0x03;
    resultFull->frameSubType = (raw[3] & 0x3C) >> 2;
    resultFull->toDistributionSystem = (raw[3] & 0x40) != 0;
    resultFull->fromDistributionSystem = (raw[3] & 0x80) != 0;
    memcpy(resultFull->mac, &raw[4], RADIOLIB_LR11X0_WIFI_RESULT_MAC_LEN);
    resultFull->phiOffset = (((uint16_t)raw[10] << 8) | ((uint16_t)raw[11]));
    resultFull->timestamp = (((uint64_t)raw[12] << 56) | ((uint64_t)raw[13] << 48)) | 
                            (((uint64_t)raw[14] << 40) | ((uint64_t)raw[15] << 32)) | 
                            (((uint64_t)raw[16] << 24) | ((uint64_t)raw[17] << 16)) | 
                            (((uint64_t)raw[18] << 8) | (uint64_t)raw[19]);
    resultFull->periodBeacon = (((uint16_t)raw[20] << 8) | ((uint16_t)raw[21])) * 1024UL;
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::wifiScan(uint8_t wifiType, uint8_t* count, uint8_t mode, uint16_t chanMask, uint8_t numScans, uint16_t timeout) {
  RADIOLIB_ASSERT_PTR(count);

  // start scan
  RADIOLIB_DEBUG_BASIC_PRINTLN("WiFi scan start");
  int16_t state = startWifiScan(wifiType, mode, chanMask, numScans, timeout);
  RADIOLIB_ASSERT(state);

  // wait for scan finished or timeout
  RadioLibTime_t softTimeout = 30UL * 1000UL;
  RadioLibTime_t start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    if(this->mod->hal->millis() - start > softTimeout) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout waiting for IRQ");
      this->standby();
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }
  RADIOLIB_DEBUG_BASIC_PRINTLN("WiFi scan done in %lu ms", (long unsigned int)(this->mod->hal->millis() - start));

  // read number of results
  return(getWifiScanResultsCount(count));
}

int16_t LR11x0::wifiScan(uint8_t type, uint16_t mask, uint8_t acqMode, uint8_t nbMaxRes, uint8_t nbScanPerChan, uint16_t timeout, uint8_t abortOnTimeout) {
  uint8_t buff[9] = {
    type, (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    acqMode, nbMaxRes, nbScanPerChan,
    (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
    abortOnTimeout
  };

  // call the SPI write stream directly to skip waiting for BUSY - it will be set to high once the scan starts
  return(this->mod->SPIwriteStream(RADIOLIB_LR11X0_CMD_WIFI_SCAN, buff, sizeof(buff), false, false));
}

int16_t LR11x0::wifiScanTimeLimit(uint8_t type, uint16_t mask, uint8_t acqMode, uint8_t nbMaxRes, uint16_t timePerChan, uint16_t timeout) {
  uint8_t buff[9] = {
    type, (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    acqMode, nbMaxRes,
    (uint8_t)((timePerChan >> 8) & 0xFF), (uint8_t)(timePerChan & 0xFF),
    (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_SCAN_TIME_LIMIT, true, buff, sizeof(buff)));
}

int16_t LR11x0::wifiCountryCode(uint16_t mask, uint8_t nbMaxRes, uint8_t nbScanPerChan, uint16_t timeout, uint8_t abortOnTimeout) {
  uint8_t buff[7] = {
    (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    nbMaxRes, nbScanPerChan,
    (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
    abortOnTimeout
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_COUNTRY_CODE, true, buff, sizeof(buff)));
}

int16_t LR11x0::wifiCountryCodeTimeLimit(uint16_t mask, uint8_t nbMaxRes, uint16_t timePerChan, uint16_t timeout) {
  uint8_t buff[7] = {
    (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    nbMaxRes,
    (uint8_t)((timePerChan >> 8) & 0xFF), (uint8_t)(timePerChan & 0xFF),
    (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_COUNTRY_CODE_TIME_LIMIT, true, buff, sizeof(buff)));
}

int16_t LR11x0::wifiReadResults(uint8_t index, uint8_t nbResults, uint8_t format, uint8_t* results) {
  uint8_t buff[3] = { index, nbResults, format };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_READ_RESULTS, false, results, RADIOLIB_LR11X0_WIFI_RESULT_MAX_LEN, buff, sizeof(buff)));
}

int16_t LR11x0::wifiResetCumulTimings(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_RESET_CUMUL_TIMINGS, true, NULL, 0));
}

int16_t LR11x0::wifiReadCumulTimings(uint32_t* detection, uint32_t* capture, uint32_t* demodulation) {
  uint8_t buff[16] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_READ_CUMUL_TIMINGS, false, buff, sizeof(buff));

  // pass the replies
  if(detection) { *detection = ((uint32_t)(buff[4]) << 24) | ((uint32_t)(buff[5]) << 16) | ((uint32_t)(buff[6]) << 8) | (uint32_t)buff[7]; }
  if(capture) { *capture = ((uint32_t)(buff[8]) << 24) | ((uint32_t)(buff[9]) << 16) | ((uint32_t)(buff[10]) << 8) | (uint32_t)buff[11]; }
  if(demodulation) { *demodulation = ((uint32_t)(buff[12]) << 24) | ((uint32_t)(buff[13]) << 16) | ((uint32_t)(buff[14]) << 8) | (uint32_t)buff[15]; }

  return(state);
}

int16_t LR11x0::wifiGetNbCountryCodeResults(uint8_t* nbResults) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_GET_NB_COUNTRY_CODE_RESULTS, false, buff, sizeof(buff));

  // pass the replies
  if(nbResults) { *nbResults = buff[0]; }

  return(state);
}

int16_t LR11x0::wifiReadCountryCodeResults(uint8_t index, uint8_t nbResults, uint8_t* results) {
  uint8_t reqBuff[2] = { index, nbResults };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_READ_COUNTRY_CODE_RESULTS, false, results, nbResults, reqBuff, sizeof(reqBuff)));
}

int16_t LR11x0::wifiCfgTimestampAPphone(uint32_t timestamp) {
  uint8_t buff[4] = {
    (uint8_t)((timestamp >> 24) & 0xFF), (uint8_t)((timestamp >> 16) & 0xFF),
    (uint8_t)((timestamp >> 8) & 0xFF), (uint8_t)(timestamp & 0xFF)
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_COUNTRY_CODE_TIME_LIMIT, true, buff, sizeof(buff)));
}

int16_t LR11x0::wifiReadVersion(uint8_t* major, uint8_t* minor) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_WIFI_READ_VERSION, false, buff, sizeof(buff));

  // pass the replies
  if(major) { *major = buff[0]; }
  if(minor) { *minor = buff[1]; }

  return(state);
}

#endif
