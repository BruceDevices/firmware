#include "LR11x0.h"

#include <string.h>

#if !RADIOLIB_EXCLUDE_LR11X0

int16_t LR11x0::isGnssScanCapable() {
  // get the version
  LR11x0VersionInfo_t version;
  int16_t state = this->getVersionInfo(&version);
  RADIOLIB_ASSERT(state);

  // check the device firmware version is sufficient
  uint16_t versionFull = ((uint16_t)version.fwMajor << 8) | (uint16_t)version.fwMinor;
  state = RADIOLIB_ERR_UNSUPPORTED;
  if((version.device == RADIOLIB_LR11X0_DEVICE_LR1110) && (versionFull >= 0x0401)) {
    state = RADIOLIB_ERR_NONE;
  } else if((version.device == RADIOLIB_LR11X0_DEVICE_LR1120) && (versionFull >= 0x0201)) {
    state = RADIOLIB_ERR_NONE;
  }
  RADIOLIB_ASSERT(state);

  // in debug mode, dump the almanac
  #if RADIOLIB_DEBUG_PROTOCOL
  uint32_t addr = 0;
  uint16_t sz = 0;
  state = this->gnssAlmanacReadAddrSize(&addr, &sz);
  RADIOLIB_ASSERT(state);
  RADIOLIB_DEBUG_BASIC_PRINTLN("Almanac@%08x, %d bytes", addr, sz);
  uint32_t buff[32] = { 0 };
  while(sz > 0) {
    size_t len = sz > 32 ? 32 : sz/sizeof(uint32_t);
    state = this->readRegMem32(addr, buff, len);
    RADIOLIB_ASSERT(state);
    RADIOLIB_DEBUG_HEXDUMP(NULL, reinterpret_cast<uint8_t*>(buff), len*sizeof(uint32_t), addr);
    addr += len*sizeof(uint32_t);
    sz -= len*sizeof(uint32_t);
  }

  uint8_t almanac[22] = { 0 };
  for(uint8_t i = 0; i < 128; i++) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Almanac[%d]:", i);
    state = this->gnssAlmanacReadSV(i, almanac);
    RADIOLIB_ASSERT(state);
    RADIOLIB_DEBUG_HEXDUMP(NULL, almanac, 22);
  }

  #endif

  return(state);
}

int16_t LR11x0::gnssScan(LR11x0GnssResult_t* res) {
  RADIOLIB_ASSERT_PTR(res);

  // go to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set DIO mapping
  state = setDioIrqParams(RADIOLIB_LR11X0_IRQ_GNSS_DONE | RADIOLIB_LR11X0_IRQ_GNSS_ABORT);
  RADIOLIB_ASSERT(state);

  // set scan mode (single vs multiple)
  state = this->gnssSetMode(0x03);
  RADIOLIB_ASSERT(state);
  
  // set RF switch
  this->mod->setRfSwitchState(LR11x0::MODE_GNSS);

  // start scan with high effort
  RADIOLIB_DEBUG_BASIC_PRINTLN("GNSS scan start");
  state = this->gnssPerformScan(RADIOLIB_LR11X0_GNSS_EFFORT_MID, 0x3C, 16);
  RADIOLIB_ASSERT(state);

  // wait for scan finished or timeout
  // this can take very long if both GPS and BeiDou are enabled
  RadioLibTime_t softTimeout = 300UL * 1000UL;
  RadioLibTime_t start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    if(this->mod->hal->millis() - start > softTimeout) {
      this->gnssAbort();
      RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout waiting for IRQ");
    }
  }

  // restore the switch
  this->mod->setRfSwitchState(Module::MODE_IDLE);
  RADIOLIB_DEBUG_BASIC_PRINTLN("GNSS scan done in %lu ms", (long unsigned int)(this->mod->hal->millis() - start));

  // distinguish between GNSS-done and GNSS-abort outcomes and clear the flags
  uint32_t irq = this->getIrqStatus();
  this->clearIrqState(RADIOLIB_LR11X0_IRQ_ALL);
  if(irq & RADIOLIB_LR11X0_IRQ_GNSS_ABORT) {
    return(RADIOLIB_ERR_RX_TIMEOUT);
  }

  // retrieve the demodulator status
  uint8_t info = 0;
  state = this->gnssReadDemodStatus(&res->demodStat, &info);
  RADIOLIB_ASSERT(state);
  RADIOLIB_DEBUG_BASIC_PRINTLN("Demod status %d, info %02x", (int)res->demodStat, (unsigned int)info);

  // retrieve the number of detected satellites
  state = this->gnssGetNbSvDetected(&res->numSatsDet);
  RADIOLIB_ASSERT(state);

  // retrieve the result size
  state = this->gnssGetResultSize(&res->resSize);
  RADIOLIB_ASSERT(state);

  // check and return demodulator status
  if(res->demodStat < RADIOLIB_LR11X0_GNSS_DEMOD_STATUS_TOW_FOUND) {
    return(RADIOLIB_ERR_GNSS_DEMOD(res->demodStat));
  }
  
  return(state);
}

int16_t LR11x0::getGnssAlmanacStatus(LR11x0GnssAlmanacStatus_t *stat) {
  RADIOLIB_ASSERT_PTR(stat);

  // save the time the time until subframe is relative to
  stat->start = this->mod->hal->millis();

  // get the raw data
  uint8_t raw[53] = { 0 };
  int16_t state = this->gnssReadAlmanacStatus(raw);
  RADIOLIB_ASSERT(state);

  // parse the reply
  stat->gps.status = (int8_t)raw[0];
  stat->gps.timeUntilSubframe = ((uint32_t)(raw[1]) << 24) | ((uint32_t)(raw[2]) << 16) | ((uint32_t)(raw[3]) << 8) | (uint32_t)raw[4];
  stat->gps.numSubframes = raw[5];
  stat->gps.nextSubframe4SvId = raw[6];
  stat->gps.nextSubframe5SvId = raw[7];
  stat->gps.nextSubframeStart = raw[8];
  stat->gps.numUpdateNeeded = raw[9];
  stat->gps.flagsUpdateNeeded[0] = ((uint32_t)(raw[10]) << 24) | ((uint32_t)(raw[11]) << 16) | ((uint32_t)(raw[12]) << 8) | (uint32_t)raw[13];
  stat->gps.flagsActive[0] = ((uint32_t)(raw[14]) << 24) | ((uint32_t)(raw[15]) << 16) | ((uint32_t)(raw[16]) << 8) | (uint32_t)raw[17];
  stat->beidou.status = (int8_t)raw[18];
  stat->beidou.timeUntilSubframe = ((uint32_t)(raw[19]) << 24) | ((uint32_t)(raw[20]) << 16) | ((uint32_t)(raw[21]) << 8) | (uint32_t)raw[22];
  stat->beidou.numSubframes = raw[23];
  stat->beidou.nextSubframe4SvId = raw[24];
  stat->beidou.nextSubframe5SvId = raw[25];
  stat->beidou.nextSubframeStart = raw[26];
  stat->beidou.numUpdateNeeded = raw[27];
  stat->beidou.flagsUpdateNeeded[0] = ((uint32_t)(raw[28]) << 24) | ((uint32_t)(raw[29]) << 16) | ((uint32_t)(raw[30]) << 8) | (uint32_t)raw[31];
  stat->beidou.flagsUpdateNeeded[1] = ((uint32_t)(raw[32]) << 24) | ((uint32_t)(raw[33]) << 16) | ((uint32_t)(raw[34]) << 8) | (uint32_t)raw[35];
  stat->beidou.flagsActive[0] = ((uint32_t)(raw[36]) << 24) | ((uint32_t)(raw[37]) << 16) | ((uint32_t)(raw[38]) << 8) | (uint32_t)raw[39];
  stat->beidou.flagsActive[1] = ((uint32_t)(raw[40]) << 24) | ((uint32_t)(raw[41]) << 16) | ((uint32_t)(raw[42]) << 8) | (uint32_t)raw[43];
  stat->beidouSvNoAlmanacFlags[0] = ((uint32_t)(raw[44]) << 24) | ((uint32_t)(raw[45]) << 16) | ((uint32_t)(raw[46]) << 8) | (uint32_t)raw[47];
  stat->beidouSvNoAlmanacFlags[1] = ((uint32_t)(raw[18]) << 24) | ((uint32_t)(raw[49]) << 16) | ((uint32_t)(raw[50]) << 8) | (uint32_t)raw[51];
  stat->nextAlmanacId = raw[52];

  return(state);
}

int16_t LR11x0::gnssDelayUntilSubframe(LR11x0GnssAlmanacStatus_t *stat, uint8_t constellation) {
  RADIOLIB_ASSERT_PTR(stat);

  // almanac update has to be called at least 1.3 seconds before the subframe
  // we use 2.3 seconds to be on the safe side

  // calculate absolute times
  RadioLibTime_t window = stat->start + stat->gps.timeUntilSubframe - 2300;
  if(constellation == RADIOLIB_LR11X0_GNSS_CONSTELLATION_BEIDOU) {
    window = stat->start + stat->beidou.timeUntilSubframe - 2300;
  }
  RadioLibTime_t now = this->mod->hal->millis();
  if(now > window) {
    // we missed the window
    return(RADIOLIB_ERR_GNSS_SUBFRAME_NOT_AVAILABLE);
  }

  RadioLibTime_t delay = window - now;
  RADIOLIB_DEBUG_BASIC_PRINTLN("Time until subframe %lu ms", delay);
  this->mod->hal->delay(delay); 
  return(RADIOLIB_ERR_NONE);
}

// TODO fix last satellite always out of date
int16_t LR11x0::updateGnssAlmanac(uint8_t constellation) {
  int16_t state = this->setDioIrqParams(RADIOLIB_LR11X0_IRQ_GNSS_DONE | RADIOLIB_LR11X0_IRQ_GNSS_ABORT);
  RADIOLIB_ASSERT(state);

  state = this->gnssAlmanacUpdateFromSat(RADIOLIB_LR11X0_GNSS_EFFORT_MID, constellation);
  RADIOLIB_ASSERT(state);

  // wait for scan finished or timeout, assumes 2 subframes and up to 2.3s pre-roll
  uint32_t softTimeout = 16UL * 1000UL;
  uint32_t start = this->mod->hal->millis();
  while (!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    if(this->mod->hal->millis() - start > softTimeout) {
      this->gnssAbort();
      RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout waiting for almanac update");
    }
  }

  RADIOLIB_DEBUG_BASIC_PRINTLN("GPS almanac update done in %lu ms", (long unsigned int)(this->mod->hal->millis() - start));

  // distinguish between GNSS-done and GNSS-abort outcomes and clear the flags
  uint32_t irq = this->getIrqStatus();
  this->clearIrqState(RADIOLIB_LR11X0_IRQ_ALL);
  if(irq & RADIOLIB_LR11X0_IRQ_GNSS_ABORT) {
    state = RADIOLIB_ERR_RX_TIMEOUT;
  }
  
  return(state);
}

int16_t LR11x0::getGnssPosition(LR11x0GnssPosition_t* pos, bool filtered) {
  RADIOLIB_ASSERT_PTR(pos);

  uint8_t error = 0;
  int16_t state;
  if(filtered) {
    state = this->gnssReadDopplerSolverRes(&error, &pos->numSatsUsed, NULL, NULL, NULL, NULL, &pos->latitude, &pos->longitude, &pos->accuracy, NULL);
  } else {
    state = this->gnssReadDopplerSolverRes(&error, &pos->numSatsUsed, &pos->latitude, &pos->longitude, &pos->accuracy, NULL, NULL, NULL, NULL, NULL);
  }
  RADIOLIB_ASSERT(state);

  // check the solver error
  if(error != 0) {
    return(RADIOLIB_ERR_GNSS_SOLVER(error));
  }

  return(state);
}

int16_t LR11x0::getGnssSatellites(LR11x0GnssSatellite_t* sats, uint8_t numSats) {
  RADIOLIB_ASSERT_PTR(sats);
  if(numSats >= 32) {
    return(RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
  }

  uint8_t svId[32] = { 0 };
  uint8_t snr[32] = { 0 };
  int16_t doppler[32] = { 0 };
  int16_t state = this->gnssGetSvDetected(svId, snr, doppler, numSats);
  RADIOLIB_ASSERT(state);
  for(size_t i = 0; i < numSats; i++) {
    sats[i].svId = svId[i];
    sats[i].c_n0 = snr[i] + 31;
    sats[i].doppler = doppler[i];
  }

  return(state);
}

int16_t LR11x0::gnssReadRssi(int8_t* rssi) {
  uint8_t reqBuff[1] = { 0x09 };  // some undocumented magic byte, from the official driver
  uint8_t rplBuff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_RSSI, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);
  if(rssi) { *rssi = rplBuff[1]; }
  return(state);
}

int16_t LR11x0::gnssSetConstellationToUse(uint8_t mask) {
  uint8_t buff[1] = { mask };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_CONSTELLATION_TO_USE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadConstellationToUse(uint8_t* mask) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_CONSTELLATION_TO_USE, false, buff, sizeof(buff));

  // pass the replies
  if(mask) { *mask = buff[0]; }

  return(state);
}

int16_t LR11x0::gnssSetAlmanacUpdate(uint8_t mask) {
  uint8_t buff[1] = { mask };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_ALMANAC_UPDATE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadAlmanacUpdate(uint8_t* mask) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_UPDATE, false, buff, sizeof(buff));

  // pass the replies
  if(mask) { *mask = buff[0]; }

  return(state);
}

int16_t LR11x0::gnssSetFreqSearchSpace(uint8_t freq) {
  uint8_t buff[1] = { freq };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_FREQ_SEARCH_SPACE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadFreqSearchSpace(uint8_t* freq) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_FREQ_SEARCH_SPACE, false, buff, sizeof(buff));
  if(freq) { *freq = buff[0]; }
  return(state);
}

int16_t LR11x0::gnssReadVersion(uint8_t* fw, uint8_t* almanac) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_VERSION, false, buff, sizeof(buff));

  // pass the replies
  if(fw) { *fw = buff[0]; }
  if(almanac) { *almanac = buff[1]; }

  return(state);
}

int16_t LR11x0::gnssReadSupportedConstellations(uint8_t* mask) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_SUPPORTED_CONSTELLATIONS, false, buff, sizeof(buff));

  // pass the replies
  if(mask) { *mask = buff[0]; }

  return(state);
}

int16_t LR11x0::gnssSetMode(uint8_t mode) {
  uint8_t buff[1] = { mode };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_MODE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssAutonomous(uint32_t gpsTime, uint8_t resMask, uint8_t nbSvMask) {
  uint8_t buff[7] = {
    (uint8_t)((gpsTime >> 24) & 0xFF), (uint8_t)((gpsTime >> 16) & 0xFF),
    (uint8_t)((gpsTime >> 8) & 0xFF), (uint8_t)(gpsTime & 0xFF),
    RADIOLIB_LR11X0_GNSS_AUTO_EFFORT_MODE, resMask, nbSvMask
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_AUTONOMOUS, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssAssisted(uint32_t gpsTime, uint8_t effort, uint8_t resMask, uint8_t nbSvMask) {
  uint8_t buff[7] = {
    (uint8_t)((gpsTime >> 24) & 0xFF), (uint8_t)((gpsTime >> 16) & 0xFF),
    (uint8_t)((gpsTime >> 8) & 0xFF), (uint8_t)(gpsTime & 0xFF),
    effort, resMask, nbSvMask
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_ASSISTED, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssSetAssistancePosition(float lat, float lon) {
  int16_t latRaw = (lat*2048.0f)/90.0f + 0.5f;
  int16_t lonRaw = (lon*2048.0f)/180.0f + 0.5f;
  uint8_t buff[4] = {
    (uint8_t)((latRaw >> 8) & 0xFF), (uint8_t)(latRaw & 0xFF),
    (uint8_t)((lonRaw >> 8) & 0xFF), (uint8_t)(lonRaw & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_ASSISTANCE_POSITION, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadAssistancePosition(float* lat, float* lon) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_ASSISTANCE_POSITION, false, buff, sizeof(buff));

  // pass the replies
  if(lat) {
    int16_t latRaw = ((int16_t)(buff[0]) << 8) | (int16_t)(buff[1]);
    *lat = ((float)latRaw*90.0f)/2048.0f;
  }
  if(lon) {
    int16_t lonRaw = ((int16_t)(buff[2]) << 8) | (int16_t)(buff[3]);
    *lon = ((float)lonRaw*180.0f)/2048.0f;
  }

  return(state);
}

int16_t LR11x0::gnssPushSolverMsg(uint8_t* payload, size_t len) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_PUSH_SOLVER_MSG, true, payload, len));
}

int16_t LR11x0::gnssPushDmMsg(uint8_t* payload, size_t len) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_PUSH_DM_MSG, true, payload, len));
}

int16_t LR11x0::gnssGetContextStatus(uint8_t* fwVersion, uint32_t* almanacCrc, uint8_t* errCode, uint8_t* almUpdMask, uint8_t* freqSpace) {
  // send the command - datasheet here shows extra bytes being sent in the request
  // but doing that fails so treat it like any other read command
  uint8_t buff[9] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_CONTEXT_STATUS, false, buff, sizeof(buff));

  // pass the replies
  if(fwVersion) { *fwVersion = buff[2]; }
  if(almanacCrc) { *almanacCrc = ((uint32_t)(buff[3]) << 24) | ((uint32_t)(buff[4]) << 16) | ((uint32_t)(buff[5]) << 8) | (uint32_t)buff[6]; }
  if(errCode) { *errCode = (buff[7] & 0xF0) >> 4; }
  if(almUpdMask) { *almUpdMask = (buff[7] & 0x0E) >> 1; }
  if(freqSpace) { *freqSpace = ((buff[7] & 0x01) << 1) | ((buff[8] & 0x80) >> 7); }

  return(state);
}

int16_t LR11x0::gnssGetNbSvDetected(uint8_t* nbSv) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_NB_SV_DETECTED, false, buff, sizeof(buff));

  // pass the replies
  if(nbSv) { *nbSv = buff[0]; }

  return(state);
}

int16_t LR11x0::gnssGetSvDetected(uint8_t* svId, uint8_t* snr, int16_t* doppler, size_t nbSv) {
  // TODO this is arbitrary - is there an actual maximum?
  if(nbSv > RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t)) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers
  size_t buffLen = nbSv*sizeof(uint32_t);
  #if RADIOLIB_STATIC_ONLY
    uint8_t dataBuff[RADIOLIB_LR11X0_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* dataBuff = new uint8_t[buffLen];
  #endif

  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_SV_DETECTED, false, dataBuff, buffLen);
  if(state == RADIOLIB_ERR_NONE) {
    for(size_t i = 0; i < nbSv; i++) {
      if(svId) { svId[i] = dataBuff[4*i]; }
      if(snr) { snr[i] = dataBuff[4*i + 1]; }
      if(doppler) { doppler[i] = ((uint16_t)(dataBuff[4*i + 2]) << 8) | (uint16_t)dataBuff[4*i + 3]; }
    }
  }

  #if !RADIOLIB_STATIC_ONLY
    delete[] dataBuff;
  #endif
  return(state);
}

int16_t LR11x0::gnssGetConsumption(uint32_t* cpu, uint32_t* radio) {
  uint8_t buff[8] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_CONSUMPTION, false, buff, sizeof(buff));

  // pass the replies
  if(cpu) { *cpu = ((uint32_t)(buff[0]) << 24) | ((uint32_t)(buff[1]) << 16) | ((uint32_t)(buff[2]) << 8) | (uint32_t)buff[3]; }
  if(radio) { *radio = ((uint32_t)(buff[4]) << 24) | ((uint32_t)(buff[5]) << 16) | ((uint32_t)(buff[6]) << 8) | (uint32_t)buff[7]; }

  return(state);
}

int16_t LR11x0::gnssGetResultSize(uint16_t* size) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_RESULT_SIZE, false, buff, sizeof(buff));

  // pass the replies
  if(size) { *size = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  
  return(state);
}

int16_t LR11x0::gnssReadResults(uint8_t* result, uint16_t size) {
  RADIOLIB_ASSERT_PTR(result);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_RESULTS, false, result, size));
}

int16_t LR11x0::gnssAlmanacFullUpdateHeader(uint16_t date, uint32_t globalCrc) {
  uint8_t buff[RADIOLIB_LR11X0_GNSS_ALMANAC_BLOCK_SIZE] = {
    RADIOLIB_LR11X0_GNSS_ALMANAC_HEADER_ID,
    (uint8_t)((date >> 8) & 0xFF), (uint8_t)(date & 0xFF),
    (uint8_t)((globalCrc >> 24) & 0xFF), (uint8_t)((globalCrc >> 16) & 0xFF), 
    (uint8_t)((globalCrc >> 8) & 0xFF), (uint8_t)(globalCrc & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_FULL_UPDATE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssAlmanacFullUpdateSV(uint8_t svn, const uint8_t* svnAlmanac) {
  uint8_t buff[RADIOLIB_LR11X0_GNSS_ALMANAC_BLOCK_SIZE] = { svn };
  memcpy(&buff[1], svnAlmanac, RADIOLIB_LR11X0_GNSS_ALMANAC_BLOCK_SIZE - 1);
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_FULL_UPDATE, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssAlmanacReadAddrSize(uint32_t* addr, uint16_t* size) {
  uint8_t buff[6] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_READ_ADDR_SIZE, false, buff, sizeof(buff));

  if(addr) { *addr = ((uint32_t)(buff[0]) << 24) | ((uint32_t)(buff[1]) << 16) | ((uint32_t)(buff[2]) << 8) | (uint32_t)buff[3]; }
  if(size) { *size = ((uint16_t)(buff[4]) << 8) | (uint16_t)buff[5]; }
  
  return(state);
}

int16_t LR11x0::gnssAlmanacReadSV(uint8_t svId, uint8_t* almanac) {
  uint8_t reqBuff[2] = { svId, 0x01 }; // in theory multiple SV entries can be read at the same time, but we don't need that
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_PER_SATELLITE, false, almanac, 22, reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);
  return(state);
}

int16_t LR11x0::gnssGetNbSvVisible(uint32_t time, float lat, float lon, uint8_t constellation, uint8_t* nbSv) {
  int16_t latRaw = (lat*2048.0f)/90.0f + 0.5f;
  int16_t lonRaw = (lon*2048.0f)/180.0f + 0.5f;
  uint8_t reqBuff[9] = { 
    (uint8_t)((time >> 24) & 0xFF), (uint8_t)((time >> 16) & 0xFF),
    (uint8_t)((time >> 8) & 0xFF), (uint8_t)(time & 0xFF),
    (uint8_t)((latRaw >> 8) & 0xFF), (uint8_t)(latRaw & 0xFF),
    (uint8_t)((lonRaw >> 8) & 0xFF), (uint8_t)(lonRaw & 0xFF),
    constellation,
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_SV_VISIBLE, false, nbSv, 1, reqBuff, sizeof(reqBuff)));
}

int16_t LR11x0::gnssGetSvVisible(uint8_t nbSv, uint8_t** svId, int16_t** doppler, int16_t** dopplerErr) {
  // enforce a maximum of 12 SVs
  if(nbSv > 12) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  uint8_t buff[60] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_SV_VISIBLE_DOPPLER, false, buff, sizeof(buff));
  for(uint8_t i = 0; i < nbSv; i++) {
    if(svId && svId[i]) { *svId[i] = buff[i*12]; }
    if(doppler && doppler[i]) { *doppler[i] = ((uint16_t)(buff[i*12 + 1]) << 8) | (uint16_t)buff[i*12 + 2]; }
    if(dopplerErr && dopplerErr[i]) { *dopplerErr[i] = ((uint16_t)(buff[i*12 + 3]) << 8) | (uint16_t)buff[i*12 + 4]; }
  }
  
  return(state);
}

int16_t LR11x0::gnssPerformScan(uint8_t effort, uint8_t resMask, uint8_t nbSvMax) {
  uint8_t buff[3] = { effort, resMask, nbSvMax };
  // call the SPI write stream directly to skip waiting for BUSY - it will be set to high once the scan starts
  return(this->mod->SPIwriteStream(RADIOLIB_LR11X0_CMD_GNSS_SCAN, buff, sizeof(buff), false, false));
}

int16_t LR11x0::gnssReadLastScanModeLaunched(uint8_t* lastScanMode) {
  uint8_t buff[1] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_LAST_SCAN_MODE_LAUNCHED, false, buff, sizeof(buff));

  // pass the replies
  if(lastScanMode) { *lastScanMode = buff[0]; }
  
  return(state);
}

int16_t LR11x0::gnssFetchTime(uint8_t effort, uint8_t opt) {
  uint8_t buff[2] = { effort, opt };
  // call the SPI write stream directly to skip waiting for BUSY - it will be set to high once the scan starts
  return(this->mod->SPIwriteStream(RADIOLIB_LR11X0_CMD_GNSS_FETCH_TIME, buff, sizeof(buff), false, false));
}

int16_t LR11x0::gnssReadTime(uint8_t* err, uint32_t* time, uint32_t* nbUs, uint32_t* timeAccuracy) {
  uint8_t buff[12] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_TIME, false, buff, sizeof(buff));

  // pass the replies
  if(err) { *err = buff[0]; }
  
  if(time) {
    *time = ((uint32_t)(buff[1]) << 24) | ((uint32_t)(buff[2]) << 16) | ((uint32_t)(buff[3]) << 8) | (uint32_t)buff[4];
    *time += 2UL*1024UL*7UL*24UL*3600UL; // assume WN rollover is at 2, this will fail sometime in 2038
    *time += 315964800UL; // convert to UTC
  }

  if(nbUs) {
    *nbUs = ((uint32_t)(buff[5]) << 16) | ((uint32_t)(buff[6]) << 8) | (uint32_t)buff[7];
    *nbUs /= 16;
  }

  if(timeAccuracy) {
    *timeAccuracy = ((uint32_t)(buff[8]) << 24) | ((uint32_t)(buff[9]) << 16) | ((uint32_t)(buff[10]) << 8) | (uint32_t)buff[11];
    *timeAccuracy /= 16;
  }
  
  return(state);
}

int16_t LR11x0::gnssResetTime(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_RESET_TIME, true, NULL, 0));
}

int16_t LR11x0::gnssResetPosition(void) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_RESET_POSITION, true, NULL, 0));
}

int16_t LR11x0::gnssReadWeekNumberRollover(uint8_t* status, uint8_t* rollover) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_WEEK_NUMBER_ROLLOWER, false, buff, sizeof(buff));
  if(status) { *status = buff[0]; }
  if(rollover) { *rollover = buff[1]; }
  return(state);
}

int16_t LR11x0::gnssReadDemodStatus(int8_t* status, uint8_t* info) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_DEMOD_STATUS, false, buff, sizeof(buff));

  // pass the replies
  if(status) { *status = (int8_t)buff[0]; }
  if(info) { *info = buff[1]; }
  
  return(state);
}

int16_t LR11x0::gnssReadCumulTiming(uint32_t* timing, uint8_t* constDemod) {
  uint8_t rplBuff[125] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_CUMUL_TIMING, false, rplBuff, 125);
  RADIOLIB_ASSERT(state);

  // convert endians
  if(timing) {
    for(size_t i = 0; i < 31; i++) {
      timing[i] = ((uint32_t)rplBuff[i*sizeof(uint32_t)] << 24) | ((uint32_t)rplBuff[1 + i*sizeof(uint32_t)] << 16) | ((uint32_t)rplBuff[2 + i*sizeof(uint32_t)] << 8) | (uint32_t)rplBuff[3 + i*sizeof(uint32_t)];
    }
  }

  if(constDemod) { *constDemod = rplBuff[124]; }
  
  return(state);
}

int16_t LR11x0::gnssSetTime(uint32_t time, uint16_t accuracy) {
  uint8_t buff[6] = {
    (uint8_t)((time >> 24) & 0xFF), (uint8_t)((time >> 16) & 0xFF),
    (uint8_t)((time >> 8) & 0xFF), (uint8_t)(time & 0xFF),
    (uint8_t)((accuracy >> 8) & 0xFF), (uint8_t)(accuracy & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_SET_TIME, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadDopplerSolverRes(uint8_t* error, uint8_t* nbSvUsed, float* lat, float* lon, uint16_t* accuracy, uint16_t* xtal, float* latFilt, float* lonFilt, uint16_t* accuracyFilt, uint16_t* xtalFilt) {
  uint8_t buff[18] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_DOPPLER_SOLVER_RES, false, buff, sizeof(buff));

  // pass the replies
  if(error) { *error = buff[0]; }
  if(nbSvUsed) { *nbSvUsed = buff[1]; }
  if(lat) {
    int16_t latRaw = ((int16_t)(buff[2]) << 8) | (int16_t)buff[3];
    *lat = ((float)latRaw * 90.0f)/2048.0f;
  }
  if(lon) {
    int16_t lonRaw = ((int16_t)(buff[4]) << 8) | (int16_t)buff[5];
    *lon = ((float)lonRaw * 180.0f)/2048.0f;
  }
  if(accuracy) { *accuracy = ((uint16_t)(buff[6]) << 8) | (uint16_t)buff[7]; }
  if(xtal) { *xtal = ((uint16_t)(buff[8]) << 8) | (uint16_t)buff[9]; }
  if(latFilt) {
    int16_t latRaw = ((int16_t)(buff[10]) << 8) | (int16_t)buff[11];
    *latFilt = ((float)latRaw * 90.0f)/2048.0f;
  }
  if(lonFilt) {
    int16_t lonRaw = ((int16_t)(buff[12]) << 8) | (int16_t)buff[13];
    *lonFilt = ((float)lonRaw * 180.0f)/2048.0f;
  }
  if(accuracyFilt) { *accuracyFilt = ((uint16_t)(buff[14]) << 8) | (uint16_t)buff[15]; }
  if(xtalFilt) { *xtalFilt = ((uint16_t)(buff[16]) << 8) | (uint16_t)buff[17]; }
  
  return(state);
}

int16_t LR11x0::gnssReadDelayResetAP(uint32_t* delay) {
  uint8_t buff[3] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_DELAY_RESET_AP, false, buff, sizeof(buff));

  if(delay) { *delay = ((uint32_t)(buff[0]) << 16) | ((uint32_t)(buff[1]) << 8) | (uint32_t)buff[2]; }
  
  return(state);
}

int16_t LR11x0::gnssAlmanacUpdateFromSat(uint8_t effort, uint8_t bitMask) {
  uint8_t buff[2] = { effort, bitMask };
  // call the SPI write stream directly to skip waiting for BUSY - it will be set to high once the scan starts
  return(this->mod->SPIwriteStream(RADIOLIB_LR11X0_CMD_GNSS_ALMANAC_UPDATE_FROM_SAT, buff, sizeof(buff), false, false));
}

int16_t LR11x0::gnssReadKeepSyncStatus(uint8_t mask, uint8_t* nbSvVisible, uint32_t* elapsed) {
  uint8_t reqBuff[1] = { mask };
  uint8_t rplBuff[5] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_KEEP_SYNC_STATUS, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);
  if(nbSvVisible) { *nbSvVisible = rplBuff[0]; }
  if(elapsed) { *elapsed = ((uint32_t)(rplBuff[1]) << 24) | ((uint32_t)(rplBuff[2]) << 16) | ((uint32_t)(rplBuff[3]) << 8) | (uint32_t)rplBuff[4]; }
  return(state);
}

int16_t LR11x0::gnssReadAlmanacStatus(uint8_t* status) {
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_STATUS, false, status, 53));
}

int16_t LR11x0::gnssConfigAlmanacUpdatePeriod(uint8_t bitMask, uint8_t svType, uint16_t period) {
  uint8_t buff[4] = { bitMask, svType, (uint8_t)((period >> 8) & 0xFF), (uint8_t)(period & 0xFF) };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_CONFIG_ALMANAC_UPDATE_PERIOD, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssReadAlmanacUpdatePeriod(uint8_t bitMask, uint8_t svType, uint16_t* period) {
  uint8_t reqBuff[2] = { bitMask, svType };
  uint8_t rplBuff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_ALMANAC_UPDATE_PERIOD, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);

  if(period) { *period = ((uint16_t)(rplBuff[0]) << 8) | (uint16_t)rplBuff[1]; }

  return(state);
}

int16_t LR11x0::gnssConfigDelayResetAP(uint32_t delay) {
  uint8_t buff[3] = { (uint8_t)((delay >> 16) & 0xFF), (uint8_t)((delay >> 8) & 0xFF), (uint8_t)(delay & 0xFF) };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_CONFIG_DELAY_RESET_AP, true, buff, sizeof(buff)));
}

int16_t LR11x0::gnssGetSvWarmStart(uint8_t bitMask, uint8_t* sv, uint8_t nbVisSat) {
  uint8_t reqBuff[1] = { bitMask };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_SV_WARM_START, false, sv, nbVisSat, reqBuff, sizeof(reqBuff)));
}

int16_t LR11x0::gnssGetSvSync(uint8_t mask, uint8_t nbSv, uint8_t* syncList) {
  uint8_t reqBuff[2] = { mask, nbSv };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_GET_SV_SYNC, false, syncList, nbSv, reqBuff, sizeof(reqBuff)));
}

int16_t LR11x0::gnssReadWarmStartStatus(uint8_t bitMask, uint8_t* nbVisSat, uint32_t* timeElapsed) {
  uint8_t reqBuff[1] = { bitMask };
  uint8_t rplBuff[5] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_READ_WARM_START_STATUS, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);

  if(nbVisSat) { *nbVisSat = rplBuff[0]; }
  if(timeElapsed) { *timeElapsed = ((uint32_t)(rplBuff[1]) << 24) | ((uint32_t)(rplBuff[2]) << 16) | ((uint32_t)(rplBuff[3]) << 8) | (uint32_t)rplBuff[4]; }

  return(state);
}

int16_t LR11x0::gnssWriteBitMaskSatActivated(uint8_t bitMask, uint32_t* bitMaskActivated0, uint32_t* bitMaskActivated1) {
  uint8_t reqBuff[1] = { bitMask };
  uint8_t rplBuff[8] = { 0 };
  size_t rplLen = (bitMask & 0x01) ? 8 : 4; // GPS only has the first bit mask
  int16_t state = this->SPIcommand(RADIOLIB_LR11X0_CMD_GNSS_WRITE_BIT_MASK_SAT_ACTIVATED, false, rplBuff, rplLen, reqBuff, sizeof(reqBuff));
  RADIOLIB_ASSERT(state);

  if(bitMaskActivated0) { *bitMaskActivated0 = ((uint32_t)(rplBuff[0]) << 24) | ((uint32_t)(rplBuff[1]) << 16) | ((uint32_t)(rplBuff[2]) << 8) | (uint32_t)rplBuff[3]; }
  if(bitMaskActivated1) { *bitMaskActivated1 = ((uint32_t)(rplBuff[4]) << 24) | ((uint32_t)(rplBuff[5]) << 16) | ((uint32_t)(rplBuff[6]) << 8) | (uint32_t)rplBuff[7]; }

  return(state);
}

void LR11x0::gnssAbort() {
  // send the abort signal (single NOP)
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_8;
  // we need to call the most basic overload of the SPI write method otherwise the call will be ambiguous
  const uint8_t cmd[2] = { 0, 0 };
  this->mod->SPIwriteStream(cmd, 2, NULL, 0, false, false);
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_16;

  // wait for at least 2.9 seconds as specified by the user manual
  this->mod->hal->delay(3000);
}

#endif
