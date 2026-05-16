#include "LR11x0.h"

#include <math.h>
#include <string.h>

#if !RADIOLIB_EXCLUDE_LR11X0

LR11x0::LR11x0(Module* mod) : LRxxxx(mod) {
  this->freqStep = RADIOLIB_LR11X0_FREQUENCY_STEP_SIZE;
  this->maxPacketLength = RADIOLIB_LR11X0_MAX_PACKET_LENGTH;
  this->irqMap[RADIOLIB_IRQ_TX_DONE] = RADIOLIB_LR11X0_IRQ_TX_DONE;
  this->irqMap[RADIOLIB_IRQ_RX_DONE] = RADIOLIB_LR11X0_IRQ_RX_DONE;
  this->irqMap[RADIOLIB_IRQ_PREAMBLE_DETECTED] = RADIOLIB_LR11X0_IRQ_PREAMBLE_DETECTED;
  this->irqMap[RADIOLIB_IRQ_SYNC_WORD_VALID] = RADIOLIB_LR11X0_IRQ_SYNC_WORD_HEADER_VALID;
  this->irqMap[RADIOLIB_IRQ_HEADER_VALID] = RADIOLIB_LR11X0_IRQ_SYNC_WORD_HEADER_VALID;
  this->irqMap[RADIOLIB_IRQ_HEADER_ERR] = RADIOLIB_LR11X0_IRQ_HEADER_ERR;
  this->irqMap[RADIOLIB_IRQ_CRC_ERR] = RADIOLIB_LR11X0_IRQ_CRC_ERR;
  this->irqMap[RADIOLIB_IRQ_CAD_DONE] = RADIOLIB_LR11X0_IRQ_CAD_DONE;
  this->irqMap[RADIOLIB_IRQ_CAD_DETECTED] = RADIOLIB_LR11X0_IRQ_CAD_DETECTED;
  this->irqMap[RADIOLIB_IRQ_TIMEOUT] = RADIOLIB_LR11X0_IRQ_TIMEOUT;
}

int16_t LR11x0::begin(float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, uint16_t preambleLength, float tcxoVoltage, bool high) {
  // set module properties and perform initial setup
  int16_t state = this->modSetup(tcxoVoltage, RADIOLIB_LR11X0_PACKET_TYPE_LORA);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setBandwidth(bw, high);
  RADIOLIB_ASSERT(state);

  state = setSpreadingFactor(sf);
  RADIOLIB_ASSERT(state);

  state = setCodingRate(cr);
  RADIOLIB_ASSERT(state);

  state = setSyncWord(syncWord);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  state = setCRC(2);
  RADIOLIB_ASSERT(state);

  state = invertIQ(false);
  RADIOLIB_ASSERT(state);

  state = setRegulatorLDO();
  RADIOLIB_ASSERT(state);

  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::beginGFSK(float br, float freqDev, float rxBw, uint16_t preambleLength, float tcxoVoltage) {
  // set module properties and perform initial setup
  int16_t state = this->modSetup(tcxoVoltage, RADIOLIB_LR11X0_PACKET_TYPE_GFSK);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  state = setRxBandwidth(rxBw);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  uint8_t sync[] = { 0x12, 0xAD };
  state = setSyncWord(sync, 2);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  state = variablePacketLengthMode(RADIOLIB_LR11X0_MAX_PACKET_LENGTH);
  RADIOLIB_ASSERT(state);

  state = setCRC(2);
  RADIOLIB_ASSERT(state);

  state = setRegulatorLDO();
  RADIOLIB_ASSERT(state);

  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::beginLRFHSS(uint8_t bw, uint8_t cr, bool narrowGrid, float tcxoVoltage) {
  // set module properties and perform initial setup
  int16_t state = this->modSetup(tcxoVoltage, RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS);
  RADIOLIB_ASSERT(state);

  // set grid spacing
  this->lrFhssGrid = narrowGrid ? RADIOLIB_LRXXXX_LR_FHSS_GRID_STEP_NON_FCC : RADIOLIB_LRXXXX_LR_FHSS_GRID_STEP_FCC;

  // configure publicly accessible settings
  state = setLrFhssConfig(bw, cr);
  RADIOLIB_ASSERT(state);

  uint8_t syncWord[] = { 0x12, 0xAD, 0x10, 0x1B };
  state = setSyncWord(syncWord, 4);
  RADIOLIB_ASSERT(state);

  state = setRegulatorLDO();
  RADIOLIB_ASSERT(state);

  // set fixed configuration
  return(setModulationParamsLrFhss(RADIOLIB_LRXXXX_LR_FHSS_BIT_RATE_RAW, RADIOLIB_LR11X0_LR_FHSS_SHAPING_GAUSSIAN_BT_1_0));
}

int16_t LR11x0::beginGNSS(uint8_t constellations, float tcxoVoltage) {
  // set module properties and perform initial setup - packet type does not matter
  int16_t state = this->modSetup(tcxoVoltage, RADIOLIB_LR11X0_PACKET_TYPE_LORA);
  RADIOLIB_ASSERT(state);

  state = this->clearErrors();
  RADIOLIB_ASSERT(state);

  // set GNSS flag to reserve DIO11 for LF clock
  this->gnss = true;
  state = this->configLfClock(RADIOLIB_LR11X0_LF_BUSY_RELEASE_DISABLED | RADIOLIB_LR11X0_LF_CLK_XOSC);
  RADIOLIB_ASSERT(state);

  uint16_t errs = 0;
  state = this->getErrors(&errs);
  RADIOLIB_ASSERT(state);
  if(errs & 0x40) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("LF_XOSC_START_ERR");
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  }

  state = this->gnssSetConstellationToUse(constellations);
  RADIOLIB_ASSERT(state);

  state = setRegulatorLDO();
  RADIOLIB_ASSERT(state);

  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::transmit(const uint8_t* data, size_t len, uint8_t addr) {
   // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check packet length
  if (this->codingRate > RADIOLIB_LR11X0_LORA_CR_4_8_SHORT) {
    // Long Interleaver needs at least 8 bytes
    if(len < 8) {
      return(RADIOLIB_ERR_PACKET_TOO_SHORT);
    }

    // Long Interleaver supports up to 253 bytes if CRC is enabled
    if (this->crcTypeLoRa == RADIOLIB_LRXXXX_LORA_CRC_ENABLED && (len > RADIOLIB_LR11X0_MAX_PACKET_LENGTH - 2)) {
      return(RADIOLIB_ERR_PACKET_TOO_LONG);
    }  
  } 
  if(len > RADIOLIB_LR11X0_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // get currently active modem
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  RadioLibTime_t timeout = getTimeOnAir(len);
  if(modem == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    // calculate timeout (150% of expected time-on-air)
    timeout = (timeout * 3) / 2;

  } else if((modem == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) || (modem == RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS)) {
    // calculate timeout (500% of expected time-on-air)
    timeout = timeout * 5;

  } else {
    return(RADIOLIB_ERR_UNKNOWN);
  }

  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu us", timeout);

  // start transmission
  state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  // wait for packet transmission or timeout
  RadioLibTime_t start = this->mod->hal->micros();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    if(this->mod->hal->micros() - start > timeout) {
      finishTransmit();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }

  return(finishTransmit());
}

int16_t LR11x0::receive(uint8_t* data, size_t len, RadioLibTime_t timeout) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // calculate timeout based on the configured modem
  RadioLibTime_t timeoutInternal = timeout;
  if(!timeoutInternal) {
    // get currently active modem
    uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
    state = getPacketType(&modem);
    RADIOLIB_ASSERT(state);
    if((modem == RADIOLIB_LR11X0_PACKET_TYPE_LORA) || (modem == RADIOLIB_LR11X0_PACKET_TYPE_GFSK)) {
      // calculate timeout (500 % of expected time-one-air)
      size_t maxLen = len;
      if(len == 0) { maxLen = RADIOLIB_LR11X0_MAX_PACKET_LENGTH; }
      timeoutInternal = (getTimeOnAir(maxLen) * 5) / 1000;
    
    } else if(modem == RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
      // this modem cannot receive
      return(RADIOLIB_ERR_WRONG_MODEM);

    } else {
      return(RADIOLIB_ERR_UNKNOWN);
    
    }
  }

  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu ms", timeoutInternal);

  // start reception
  uint32_t timeoutValue = (uint32_t)(((float)timeoutInternal * 1000.0f) / 30.52f);
  state = startReceive(timeoutValue);
  RADIOLIB_ASSERT(state);

  // wait for packet reception or timeout
  bool softTimeout = false;
  RadioLibTime_t start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    // safety check, the timeout should be done by the radio
    if(this->mod->hal->millis() - start > timeoutInternal) {
      softTimeout = true;
      break;
    }
  }

  // if it was a timeout, this will return an error code
  // TODO taken from SX126x, does this really work?
  state = standby();
  if((state != RADIOLIB_ERR_NONE) && (state != RADIOLIB_ERR_SPI_CMD_TIMEOUT)) {
    return(state);
  }

  // check whether this was a timeout or not
  if(softTimeout || (getIrqFlags() & this->irqMap[RADIOLIB_IRQ_TIMEOUT])) {
    (void)finishReceive();
    return(RADIOLIB_ERR_RX_TIMEOUT);
  }

  // read the received data
  return(readData(data, len));
}

int16_t LR11x0::transmitDirect(uint32_t frf) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_TX);

  // user requested to start transmitting immediately (required for RTTY)
  int16_t state = RADIOLIB_ERR_NONE;
  if(frf != 0) {
    state = setRfFrequency(frf);
  }
  RADIOLIB_ASSERT(state);

  // start transmitting
  return(setTxCw());
}

int16_t LR11x0::receiveDirect() {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // LR11x0 is unable to output received data directly
  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t LR11x0::scanChannel() {
  ChannelScanConfig_t cfg = {
    .cad = {
      .symNum = RADIOLIB_LR11X0_CAD_PARAM_DEFAULT,
      .detPeak = RADIOLIB_LR11X0_CAD_PARAM_DEFAULT,
      .detMin = RADIOLIB_LR11X0_CAD_PARAM_DEFAULT,
      .exitMode = RADIOLIB_LR11X0_CAD_PARAM_DEFAULT,
      .timeout = 0,
      .irqFlags = RADIOLIB_IRQ_CAD_DEFAULT_FLAGS,
      .irqMask = RADIOLIB_IRQ_CAD_DEFAULT_MASK,
    },
  };
  return(this->scanChannel(cfg));
}

int16_t LR11x0::scanChannel(const ChannelScanConfig_t &config) {
  // set mode to CAD
  int state = startChannelScan(config);
  RADIOLIB_ASSERT(state);

  // wait for channel activity detected or timeout
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
  }

  // check CAD result
  return(getChannelScanResult());
}

int16_t LR11x0::standby() {
  return(LR11x0::standby(RADIOLIB_LR11X0_STANDBY_RC));
}

int16_t LR11x0::standby(uint8_t mode) {
  return(LR11x0::standby(mode, true));
}

int16_t LR11x0::standby(uint8_t mode, bool wakeup) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  if(wakeup) {
    // send a NOP command - this pulls the NSS low to exit the sleep mode,
    // while preventing interference with possible other SPI transactions
    (void)this->mod->SPIwriteStream((uint16_t)RADIOLIB_LR11X0_CMD_NOP, NULL, 0, false, false);
  }

  uint8_t buff[] = { mode };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_STANDBY, true, buff, 1));
}

int16_t LR11x0::sleep() {
  return(LR11x0::sleep(true, 0));
}

int16_t LR11x0::sleep(bool retainConfig, uint32_t sleepTime) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  uint8_t buff[] = { 
    (uint8_t)retainConfig,
    (uint8_t)((sleepTime >> 24) & 0xFF), (uint8_t)((sleepTime >> 16) & 0xFF),
    (uint8_t)((sleepTime >> 16) & 0xFF), (uint8_t)(sleepTime & 0xFF),
  };
  if(sleepTime) {
    buff[0] |= RADIOLIB_LR11X0_SLEEP_WAKEUP_ENABLED;
  }

  // in sleep, the busy line will remain high, so we have to use this method to disable waiting for it to go low
  int16_t state = this->mod->SPIwriteStream(RADIOLIB_LR11X0_CMD_SET_SLEEP, buff, sizeof(buff), false, false);

  // wait for the module to safely enter sleep mode
  this->mod->hal->delay(1);

  return(state);
}

int16_t LR11x0::finishTransmit() {
  // clear interrupt flags
  clearIrqState(RADIOLIB_LR11X0_IRQ_ALL);

  // set mode to standby to disable transmitter/RF switch
  return(standby());
}

int16_t LR11x0::startReceive() {
  return(this->startReceive(RADIOLIB_LR11X0_RX_TIMEOUT_INF, RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RADIOLIB_IRQ_RX_DEFAULT_MASK, 0));
}

int16_t LR11x0::readData(uint8_t* data, size_t len) {
  // check active modem
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if((modem != RADIOLIB_LR11X0_PACKET_TYPE_LORA) && 
     (modem != RADIOLIB_LR11X0_PACKET_TYPE_GFSK)) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check integrity CRC
  uint32_t irq = getIrqStatus();
  int16_t crcState = RADIOLIB_ERR_NONE;
  // Report CRC mismatch when there's a payload CRC error, or a header error and no valid header (to avoid false alarm from previous packet)
  if((irq & RADIOLIB_LR11X0_IRQ_CRC_ERR) || ((irq & RADIOLIB_LR11X0_IRQ_HEADER_ERR) && !(irq & RADIOLIB_LR11X0_IRQ_SYNC_WORD_HEADER_VALID))) {
    crcState = RADIOLIB_ERR_CRC_MISMATCH;
  }

  // get packet length
  // the offset is needed since LR11x0 seems to move the buffer base by 4 bytes on every packet
  uint8_t offset = 0;
  size_t length = getPacketLength(true, &offset);
  if((len != 0) && (len < length)) {
    // user requested less data than we got, only return what was requested
    length = len;
  }

  // read packet data
  state = readBuffer8(data, length, offset);
  RADIOLIB_ASSERT(state);

  // clear the Rx buffer
  state = clearRxBuffer();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqState(RADIOLIB_LR11X0_IRQ_ALL);

  // check if CRC failed - this is done after reading data to give user the option to keep them
  RADIOLIB_ASSERT(crcState);

  return(state);
}

int16_t LR11x0::finishReceive() {
  // set mode to standby to disable RF switch
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  return(clearIrqState(RADIOLIB_LR11X0_IRQ_ALL));
}

int16_t LR11x0::startChannelScan() {
  ChannelScanConfig_t cfg = {
    .cad = {
      .symNum = RADIOLIB_LR11X0_CAD_PARAM_DEFAULT,
      .detPeak = RADIOLIB_LR11X0_CAD_PARAM_DEFAULT,
      .detMin = RADIOLIB_LR11X0_CAD_PARAM_DEFAULT,
      .exitMode = RADIOLIB_LR11X0_CAD_PARAM_DEFAULT,
      .timeout = 0,
      .irqFlags = RADIOLIB_IRQ_CAD_DEFAULT_FLAGS,
      .irqMask = RADIOLIB_IRQ_CAD_DEFAULT_MASK,
    },
  };
  return(this->startChannelScan(cfg));
}

int16_t LR11x0::startChannelScan(const ChannelScanConfig_t &config) {
  // check active modem
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if(modem != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  state = standby();
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // set DIO pin mapping
  uint16_t irqFlags = (config.cad.irqFlags == RADIOLIB_IRQ_NOT_SUPPORTED) ? RADIOLIB_LR11X0_IRQ_CAD_DETECTED | RADIOLIB_LR11X0_IRQ_CAD_DONE : config.cad.irqFlags;
  state = setDioIrqParams(getIrqMapped(irqFlags), getIrqMapped(irqFlags));
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqState(RADIOLIB_LR11X0_IRQ_ALL);
  RADIOLIB_ASSERT(state);

  // set mode to CAD
  return(startCad(config.cad.symNum, config.cad.detPeak, config.cad.detMin, config.cad.exitMode, config.cad.timeout));
}

int16_t LR11x0::getChannelScanResult() {
  // check active modem
  int16_t state = RADIOLIB_ERR_NONE;
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if(modem != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check CAD result
  uint32_t cadResult = getIrqStatus();
  if(cadResult & RADIOLIB_LR11X0_IRQ_CAD_DETECTED) {
    // detected some LoRa activity
    return(RADIOLIB_LORA_DETECTED);
  } else if(cadResult & RADIOLIB_LR11X0_IRQ_CAD_DONE) {
    // channel is free
    return(RADIOLIB_CHANNEL_FREE);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t LR11x0::setBandwidth(float bw, bool high) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // ensure byte conversion doesn't overflow
  if (high) {
    RADIOLIB_CHECK_RANGE(bw, 203.125f, 815.0f, RADIOLIB_ERR_INVALID_BANDWIDTH);

    if(fabsf(bw - 203.125f) <= 0.001f) {
      this->bandwidth = RADIOLIB_LR11X0_LORA_BW_203_125;
    } else if(fabsf(bw - 406.25f) <= 0.001f) {
      this->bandwidth = RADIOLIB_LR11X0_LORA_BW_406_25;
    } else if(fabsf(bw - 812.5f) <= 0.001f) {
      this->bandwidth = RADIOLIB_LR11X0_LORA_BW_812_50;
    } else {
      return(RADIOLIB_ERR_INVALID_BANDWIDTH);
    }
  } else {
    RADIOLIB_CHECK_RANGE(bw, 0.0f, 510.0f, RADIOLIB_ERR_INVALID_BANDWIDTH);
    
    // check allowed bandwidth values
    uint8_t bw_div2 = bw / 2 + 0.01f;
    switch (bw_div2)  {
      case 31: // 62.5:
        this->bandwidth = RADIOLIB_LR11X0_LORA_BW_62_5;
        break;
      case 62: // 125.0:
        this->bandwidth = RADIOLIB_LR11X0_LORA_BW_125_0;
        break;
      case 125: // 250.0
        this->bandwidth = RADIOLIB_LR11X0_LORA_BW_250_0;
        break;
      case 250: // 500.0
        this->bandwidth = RADIOLIB_LR11X0_LORA_BW_500_0;
        break;
      default:
        return(RADIOLIB_ERR_INVALID_BANDWIDTH);
    }
  }

  // update modulation parameters
  this->bandwidthKhz = bw;
  return(setModulationParamsLoRa(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR11x0::setSpreadingFactor(uint8_t sf, bool legacy) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(sf, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);

  // TODO enable SF6 legacy mode
  if(legacy && (sf == 6)) {
    //this->mod->SPIsetRegValue(RADIOLIB_LR11X0_REG_SF6_SX127X_COMPAT, RADIOLIB_LR11X0_SF6_SX127X, 18, 18);
  }

  // update modulation parameters
  this->spreadingFactor = sf;
  return(setModulationParamsLoRa(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR11x0::setCodingRate(uint8_t cr, bool longInterleave) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(cr, 4, 8, RADIOLIB_ERR_INVALID_CODING_RATE);

  if(longInterleave) {
    switch(cr) {
      case 4:
        this->codingRate = 0;
        break;
      case 5:
      case 6:
        this->codingRate = cr;
        break;
      case 8: 
        this->codingRate = cr - 1;
        break;
      default:
        return(RADIOLIB_ERR_INVALID_CODING_RATE);
    }
  
  } else {
    this->codingRate = cr - 4;
  
  }

  // update modulation parameters
  return(setModulationParamsLoRa(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR11x0::setSyncWord(uint8_t syncWord) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  
  return(setLoRaSyncWord(syncWord));
}

int16_t LR11x0::setBitRate(float br) {
  RADIOLIB_CHECK_RANGE(br, 0.6f, 300.0f, RADIOLIB_ERR_INVALID_BIT_RATE);

  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set bit rate value
  // TODO implement fractional bit rate configuration
  this->bitRate = br * 1000.0f;
  state = setModulationParamsGFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev);
  RADIOLIB_ASSERT(state);

  // apply workaround
  return(workaroundGFSK());
}

int16_t LR11x0::setFrequencyDeviation(float freqDev) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0f) {
    newFreqDev = 0.6f;
  }

  RADIOLIB_CHECK_RANGE(newFreqDev, 0.6f, 200.0f, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
  this->frequencyDev = newFreqDev * 1000.0f;
  state = setModulationParamsGFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev);
  RADIOLIB_ASSERT(state);

  // apply workaround
  return(workaroundGFSK());
}

int16_t LR11x0::setRxBandwidth(float rxBw) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check modulation parameters
  /*if(2 * this->frequencyDev + this->bitRate > rxBw * 1000.0) {
    return(RADIOLIB_ERR_INVALID_MODULATION_PARAMETERS);
  }*/

  const uint8_t rxBwLut[] = {
    RADIOLIB_LR11X0_GFSK_RX_BW_4_8,
    RADIOLIB_LR11X0_GFSK_RX_BW_5_8,
    RADIOLIB_LR11X0_GFSK_RX_BW_7_3,
    RADIOLIB_LR11X0_GFSK_RX_BW_9_7,
    RADIOLIB_LR11X0_GFSK_RX_BW_11_7,
    RADIOLIB_LR11X0_GFSK_RX_BW_14_6,
    RADIOLIB_LR11X0_GFSK_RX_BW_19_5,
    RADIOLIB_LR11X0_GFSK_RX_BW_23_4,
    RADIOLIB_LR11X0_GFSK_RX_BW_29_3,
    RADIOLIB_LR11X0_GFSK_RX_BW_39_0,
    RADIOLIB_LR11X0_GFSK_RX_BW_46_9,
    RADIOLIB_LR11X0_GFSK_RX_BW_58_6,
    RADIOLIB_LR11X0_GFSK_RX_BW_78_2,
    RADIOLIB_LR11X0_GFSK_RX_BW_93_8,
    RADIOLIB_LR11X0_GFSK_RX_BW_117_3,
    RADIOLIB_LR11X0_GFSK_RX_BW_156_2,
    RADIOLIB_LR11X0_GFSK_RX_BW_187_2,
    RADIOLIB_LR11X0_GFSK_RX_BW_234_3,
    RADIOLIB_LR11X0_GFSK_RX_BW_312_0,
    RADIOLIB_LR11X0_GFSK_RX_BW_373_6,
    RADIOLIB_LR11X0_GFSK_RX_BW_467_0,
  };

  state = findRxBw(rxBw, rxBwLut, sizeof(rxBwLut)/sizeof(rxBwLut[0]), 467.0f, &this->rxBandwidth);
  RADIOLIB_ASSERT(state);

  // update modulation parameters
  state = setModulationParamsGFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev);
  RADIOLIB_ASSERT(state);

  // apply workaround
  return(workaroundGFSK());
}

int16_t LR11x0::setSyncWord(uint8_t* syncWord, size_t len) {
  if((!syncWord) || (!len) || (len > RADIOLIB_LR11X0_GFSK_SYNC_WORD_LEN)) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }

  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    // update sync word length
    this->syncWordLength = len*8;
    state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
    RADIOLIB_ASSERT(state);

    // sync word is passed most-significant byte first
    uint8_t fullSyncWord[RADIOLIB_LR11X0_GFSK_SYNC_WORD_LEN] = { 0 };
    memcpy(fullSyncWord, syncWord, len);
    return(setGfskSyncWord(fullSyncWord));

  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    // with length set to 1 and LoRa modem active, assume it is the LoRa sync word
    if(len > 1) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }
    return(setSyncWord(syncWord[0]));

  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
    // with length set to 4 and LR-FHSS modem active, assume it is the LR-FHSS sync word
    if(len != sizeof(uint32_t)) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }
    uint32_t sync = 0;
    memcpy(&sync, syncWord, sizeof(uint32_t));
    return(lrFhssSetSyncWord(sync));
  
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR11x0::setNodeAddress(uint8_t nodeAddr) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node only)
  this->addrComp = RADIOLIB_LR11X0_GFSK_ADDR_FILTER_NODE;
  state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
  RADIOLIB_ASSERT(state);
  
  // set node address
  this->node = nodeAddr;
  return(setPacketAdrs(this->node, 0));
}

int16_t LR11x0::setBroadcastAddress(uint8_t broadAddr) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node and broadcast)
  this->addrComp = RADIOLIB_LR11X0_GFSK_ADDR_FILTER_NODE_BROADCAST;
  state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
  RADIOLIB_ASSERT(state);
  
  // set node and broadcast address
  return(setPacketAdrs(this->node, broadAddr));
}

int16_t LR11x0::disableAddressFiltering() {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // disable address filtering
  this->addrComp = RADIOLIB_LR11X0_GFSK_ADDR_FILTER_DISABLED;
  return(setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
}

int16_t LR11x0::setDataShaping(uint8_t sh) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      this->pulseShape = RADIOLIB_LR11X0_GFSK_SHAPING_NONE;
      break;
    case RADIOLIB_SHAPING_0_3:
      this->pulseShape = RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_3;
      break;
    case RADIOLIB_SHAPING_0_5:
      this->pulseShape = RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_5;
      break;
    case RADIOLIB_SHAPING_0_7:
      this->pulseShape = RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_0_7;
      break;
    case RADIOLIB_SHAPING_1_0:
      this->pulseShape = RADIOLIB_LR11X0_GFSK_SHAPING_GAUSSIAN_BT_1_0;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
  }

  // update modulation parameters
  return(setModulationParamsGFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev));
}

int16_t LR11x0::setEncoding(uint8_t encoding) {
  return(setWhitening(encoding));
}

int16_t LR11x0::fixedPacketLengthMode(uint8_t len) {
  return(setPacketMode(RADIOLIB_LR11X0_GFSK_PACKET_LENGTH_FIXED, len));
}

int16_t LR11x0::variablePacketLengthMode(uint8_t maxLen) {
  return(setPacketMode(RADIOLIB_LR11X0_GFSK_PACKET_LENGTH_VARIABLE, maxLen));
}

int16_t LR11x0::setWhitening(bool enabled, uint16_t initial) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(!enabled) {
    // disable whitening
    this->whitening = RADIOLIB_LR11X0_GFSK_WHITENING_DISABLED;

  } else {
    // enable whitening
    this->whitening = RADIOLIB_LR11X0_GFSK_WHITENING_ENABLED;

    // write initial whitening value
    state = setGfskWhitParams(initial);
    RADIOLIB_ASSERT(state);
  }

  return(setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
}

int16_t LR11x0::setDataRate(DataRate_t dr, ModemType_t modem) {
  // get the current modem
  ModemType_t currentModem;
  int16_t state = this->getModem(&currentModem);
  RADIOLIB_ASSERT(state);

  // switch over if the requested modem is different
  if(modem != RADIOLIB_MODEM_NONE && modem != currentModem) {
    state = this->standby();
    RADIOLIB_ASSERT(state);
    state = this->setModem(modem);
    RADIOLIB_ASSERT(state);
  }
  
  if(modem == RADIOLIB_MODEM_NONE) {
    modem = currentModem;
  }

  // select interpretation based on modem
  if(modem == RADIOLIB_MODEM_FSK) {
    // set the bit rate
    state = this->setBitRate(dr.fsk.bitRate);
    RADIOLIB_ASSERT(state);

    // set the frequency deviation
    state = this->setFrequencyDeviation(dr.fsk.freqDev);

  } else if(modem == RADIOLIB_MODEM_LORA) {
    // set the spreading factor
    state = this->setSpreadingFactor(dr.lora.spreadingFactor);
    RADIOLIB_ASSERT(state);

    // set the bandwidth
    state = this->setBandwidth(dr.lora.bandwidth);
    RADIOLIB_ASSERT(state);

    // set the coding rate
    state = this->setCodingRate(dr.lora.codingRate);
  
  } else if(modem == RADIOLIB_MODEM_LRFHSS) {
    // set the basic config
    state = this->setLrFhssConfig(dr.lrFhss.bw, dr.lrFhss.cr);
    RADIOLIB_ASSERT(state);

    // set hopping grid
    this->lrFhssGrid = dr.lrFhss.narrowGrid ? RADIOLIB_LRXXXX_LR_FHSS_GRID_STEP_NON_FCC : RADIOLIB_LRXXXX_LR_FHSS_GRID_STEP_FCC;
  
  }

  return(state);
}

int16_t LR11x0::checkDataRate(DataRate_t dr, ModemType_t modem) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // retrieve modem if not supplied
  if(modem == RADIOLIB_MODEM_NONE) {
    state = this->getModem(&modem);
    RADIOLIB_ASSERT(state);
  }

  // select interpretation based on modem
  if(modem == RADIOLIB_MODEM_FSK) {
    RADIOLIB_CHECK_RANGE(dr.fsk.bitRate, 0.6f, 300.0f, RADIOLIB_ERR_INVALID_BIT_RATE);
    RADIOLIB_CHECK_RANGE(dr.fsk.freqDev, 0.6f, 200.0f, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
    return(RADIOLIB_ERR_NONE);

  } else if(modem == RADIOLIB_MODEM_LORA) {
    RADIOLIB_CHECK_RANGE(dr.lora.spreadingFactor, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
    RADIOLIB_CHECK_RANGE(dr.lora.bandwidth, 0.0f, 510.0f, RADIOLIB_ERR_INVALID_BANDWIDTH);
    RADIOLIB_CHECK_RANGE(dr.lora.codingRate, 4, 8, RADIOLIB_ERR_INVALID_CODING_RATE);
    return(RADIOLIB_ERR_NONE);
  
  }

  return(state);
}

int16_t LR11x0::setPreambleLength(size_t preambleLength) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    this->preambleLengthLoRa = preambleLength;
    return(setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType,  this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled));
  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    this->preambleLengthGFSK = preambleLength;
    //! \TODO: [LR11x0] this can probably be simplified with a simple calculation
    this->preambleDetLength = preambleLength >= 32 ? RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_32_BITS :
                              preambleLength >= 24 ? RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_24_BITS :
                              preambleLength >= 16 ? RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_16_BITS :
                              preambleLength >   0 ? RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_8_BITS :
                              RADIOLIB_LR11X0_GFSK_PREAMBLE_DETECT_DISABLED;
    return(setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR11x0::setTCXO(float voltage, uint32_t delay) {
  // check if TCXO is enabled at all
  if(this->XTAL) {
    return(RADIOLIB_ERR_INVALID_TCXO_VOLTAGE);
  }

  // set mode to standby
  standby();

  // check RADIOLIB_LR11X0_ERROR_STAT_HF_XOSC_START_ERR flag and clear it
  uint16_t errors = 0;
  int16_t state = getErrors(&errors);
  RADIOLIB_ASSERT(state);
  if(errors & RADIOLIB_LR11X0_ERROR_STAT_HF_XOSC_START_ERR) {
    clearErrors();
  }

  // check 0 V disable
  if(fabsf(voltage - 0.0f) <= 0.001f) {
    setTcxoMode(0, 0);
    return(reset());
  }

  // check allowed voltage values
  uint8_t tune = 0;
  if(fabsf(voltage - 1.6f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_1_6;
  } else if(fabsf(voltage - 1.7f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_1_7;
  } else if(fabsf(voltage - 1.8f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_1_8;
  } else if(fabsf(voltage - 2.2f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_2_2;
  } else if(fabsf(voltage - 2.4f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_2_4;
  } else if(fabsf(voltage - 2.7f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_2_7;
  } else if(fabsf(voltage - 3.0f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_3_0;
  } else if(fabsf(voltage - 3.3f) <= 0.001f) {
    tune = RADIOLIB_LRXXXX_TCXO_VOLTAGE_3_3;
  } else {
    return(RADIOLIB_ERR_INVALID_TCXO_VOLTAGE);
  }

  // calculate delay value
  uint32_t delayValue = (uint32_t)((float)delay / 30.52f);
  if(delayValue == 0) {
    delayValue = 1;
  }
 
  // enable TCXO control
  return(setTcxoMode(tune, delayValue));
}

int16_t LR11x0::setCRC(uint8_t len, uint32_t initial, uint32_t polynomial, bool inverted) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    // LoRa CRC doesn't allow to set CRC polynomial, initial value, or inversion
    this->crcTypeLoRa = len > 0 ? RADIOLIB_LRXXXX_LORA_CRC_ENABLED : RADIOLIB_LRXXXX_LORA_CRC_DISABLED;
    state = setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled);
  
  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    // update packet parameters
    switch(len) {
      case 0:
        this->crcTypeGFSK = RADIOLIB_LR11X0_GFSK_CRC_DISABLED;
        break;
      case 1:
        if(inverted) {
          this->crcTypeGFSK = RADIOLIB_LR11X0_GFSK_CRC_1_BYTE_INV;
        } else {
          this->crcTypeGFSK = RADIOLIB_LR11X0_GFSK_CRC_1_BYTE;
        }
        break;
      case 2:
        if(inverted) {
          this->crcTypeGFSK = RADIOLIB_LR11X0_GFSK_CRC_2_BYTE_INV;
        } else {
          this->crcTypeGFSK = RADIOLIB_LR11X0_GFSK_CRC_2_BYTE;
        }
        break;
      default:
        return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }

    this->crcLenGFSK = len;
    state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, RADIOLIB_LR11X0_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
    RADIOLIB_ASSERT(state);

    state = setGfskCrcParams(initial, polynomial);
  
  }

  return(state);
}

int16_t LR11x0::invertIQ(bool enable) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  this->invertIQEnabled = enable;
  return(setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled));
}

float LR11x0::getRSSI() {
  float val = 0;

  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  (void)getPacketType(&type);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    (void)getPacketStatusLoRa(&val, NULL, NULL);

  } else if(type == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    (void)getPacketStatusGFSK(NULL, &val, NULL, NULL);
  
  }

  return(val);
}

float LR11x0::getSNR() {
  float val = 0;

  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  (void)getPacketType(&type);
  if(type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    (void)getPacketStatusLoRa(NULL, &val, NULL);
  }

  return(val);
}

float LR11x0::getFrequencyError() {
  // TODO implement this
  return(0);
}

size_t LR11x0::getPacketLength(bool update) {
  return(this->getPacketLength(update, NULL));
}

size_t LR11x0::getPacketLength(bool update, uint8_t* offset) {
  (void)update;

  // in implicit mode, return the cached value
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  (void)getPacketType(&type);
  if((type == RADIOLIB_LR11X0_PACKET_TYPE_LORA) && (this->headerType == RADIOLIB_LRXXXX_LORA_HEADER_IMPLICIT)) {
    return(this->implicitLen);
  }

  uint8_t len = 0;
  int state = getRxBufferStatus(&len, offset);
  RADIOLIB_DEBUG_BASIC_PRINT("getRxBufferStatus state = %d\n", state);
  (void)state;
  return((size_t)len);
}

RadioLibTime_t LR11x0::getTimeOnAir(size_t len) {
  ModemType_t modem;
  getModem(&modem);
  return(LRxxxx::getTimeOnAir(len, modem));
}

uint32_t LR11x0::getIrqFlags() {
  return((uint32_t)this->getIrqStatus());
}

int16_t LR11x0::setIrqFlags(uint32_t irq) {
  return(this->setDioIrqParams(irq, irq));
}

int16_t LR11x0::clearIrqFlags(uint32_t irq) {
  return(this->clearIrqState(irq));
}

uint8_t LR11x0::randomByte() {
  uint32_t num = 0;
  (void)getRandomNumber(&num);
  return((uint8_t)num);
}

int16_t LR11x0::implicitHeader(size_t len) {
  return(this->setHeaderType(RADIOLIB_LRXXXX_LORA_HEADER_IMPLICIT, len));
}

int16_t LR11x0::explicitHeader() {
  return(this->setHeaderType(RADIOLIB_LRXXXX_LORA_HEADER_EXPLICIT));
}

int16_t LR11x0::setRegulatorLDO() {
  return(this->setRegMode(RADIOLIB_LR11X0_REG_MODE_LDO));
}

int16_t LR11x0::setRegulatorDCDC() {
  return(this->setRegMode(RADIOLIB_LR11X0_REG_MODE_DC_DC));
}

int16_t LR11x0::setRxBoostedGainMode(bool en) {
  uint8_t buff[1] = { (uint8_t)en };
  return(this->SPIcommand(RADIOLIB_LR11X0_CMD_SET_RX_BOOSTED, true, buff, sizeof(buff)));
}

int16_t LR11x0::setOutputPower(int8_t power, uint8_t paSel, uint8_t regPaSupply, uint8_t paDutyCycle, uint8_t paHpSel, uint8_t rampTime) {
  // set PA config
  int16_t state = setPaConfig(paSel, regPaSupply, paDutyCycle, paHpSel);
  RADIOLIB_ASSERT(state);

  // set output power
  state = setTxParams(power, rampTime);
  return(state);
}

void LR11x0::setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]) {
  // find which pins are used
  uint8_t enable = 0;
  for(size_t i = 0; i < Module::RFSWITCH_MAX_PINS; i++) {
    // check if this pin is unused
    if(pins[i] == RADIOLIB_NC) {
      continue;
    }

    // only keep DIO pins, there may be some GPIOs in the switch tabke
    if(pins[i] & RFSWITCH_PIN_FLAG) {
      enable |= 1UL << RADIOLIB_LRXXXX_DIOx_VAL(pins[i]);
    }
    
  }

  // now get the configuration
  uint8_t modes[7] = { 0 };
  for(size_t i = 0; i < 7; i++) {
    // check end of table
    if(table[i].mode == LR11x0::MODE_END_OF_TABLE) {
      break;
    }

    // get the mode ID in case the modes are out-of-order
    uint8_t index = table[i].mode - LR11x0::MODE_STBY;

    // iterate over the pins
    for(size_t j = 0; j < Module::RFSWITCH_MAX_PINS; j++) {
      // only process modes for the DIOx pins, skip GPIO pins
      if(!(pins[j] & RFSWITCH_PIN_FLAG)) {
        continue;
      }
      modes[index] |= (table[i].values[j] == this->mod->hal->GpioLevelHigh) ? (1UL << j) : 0;
    }
  }

  // set it
  this->setDioAsRfSwitch(enable, modes[0], modes[1], modes[2], modes[3], modes[4], modes[5], modes[6]);
}

int16_t LR11x0::forceLDRO(bool enable) {
  // check packet type
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update modulation parameters
  this->ldroAuto = false;
  this->ldrOptimize = (uint8_t)enable;
  return(setModulationParamsLoRa(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR11x0::autoLDRO() {
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  this->ldroAuto = true;
  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::setLrFhssConfig(uint8_t bw, uint8_t cr, uint8_t hdrCount, uint16_t hopSeed) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check and cache all parameters
  RADIOLIB_CHECK_RANGE((int8_t)cr, (int8_t)RADIOLIB_LRXXXX_LR_FHSS_CR_5_6, (int8_t)RADIOLIB_LRXXXX_LR_FHSS_CR_1_3, RADIOLIB_ERR_INVALID_CODING_RATE);
  this->lrFhssCr = cr;
  RADIOLIB_CHECK_RANGE((int8_t)bw, (int8_t)RADIOLIB_LRXXXX_LR_FHSS_BW_39_06, (int8_t)RADIOLIB_LRXXXX_LR_FHSS_BW_1574_2, RADIOLIB_ERR_INVALID_BANDWIDTH);
  this->lrFhssBw = bw;
  RADIOLIB_CHECK_RANGE(hdrCount, 1, 4, RADIOLIB_ERR_INVALID_BIT_RANGE);
  this->lrFhssHdrCount = hdrCount;
  RADIOLIB_CHECK_RANGE((int16_t)hopSeed, (int16_t)0x000, (int16_t)0x1FF, RADIOLIB_ERR_INVALID_DATA_SHAPING);
  this->lrFhssHopSeq = hopSeed;
  return(RADIOLIB_ERR_NONE);
}

int16_t LR11x0::getVersionInfo(LR11x0VersionInfo_t* info) {
  RADIOLIB_ASSERT_PTR(info);

  int16_t state = this->getVersion(&info->hardware, &info->device, &info->fwMajor, &info->fwMinor);
  RADIOLIB_ASSERT(state);
  
  // LR1121 does not have GNSS and WiFi scanning
  if(this->chipType == RADIOLIB_LR11X0_DEVICE_LR1121) {
    info->fwMajorWiFi = 0;
    info->fwMinorWiFi = 0;
    info->fwGNSS = 0;
    info->almanacGNSS = 0;
    return(RADIOLIB_ERR_NONE);
  }

  state = this->wifiReadVersion(&info->fwMajorWiFi, &info->fwMinorWiFi);
  RADIOLIB_ASSERT(state);
  return(this->gnssReadVersion(&info->fwGNSS, &info->almanacGNSS));
}

int16_t LR11x0::updateFirmware(const uint32_t* image, size_t size, bool nonvolatile) {
  RADIOLIB_ASSERT_PTR(image);

  // put the device to bootloader mode
  int16_t state = this->reboot(true);
  RADIOLIB_ASSERT(state);
  this->mod->hal->delay(500);

  // check we're in bootloader
  uint8_t device = 0xFF;
  state = this->getVersion(NULL, &device, NULL, NULL);
  RADIOLIB_ASSERT(state);
  if(device != RADIOLIB_LR11X0_DEVICE_BOOT) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Failed to put device to bootloader mode, %02x != %02x", (unsigned int)device, (unsigned int)RADIOLIB_LR11X0_DEVICE_BOOT);
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }

  // erase the image
  state = this->bootEraseFlash();
  RADIOLIB_ASSERT(state);

  // wait for BUSY to go low
  RadioLibTime_t start = this->mod->hal->millis();
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();
    if(this->mod->hal->millis() - start >= 3000) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("BUSY pin timeout after erase!");
      return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    }
  }

  // upload the new image
  const size_t maxLen = 64;
  size_t rem = size % maxLen;
  size_t numWrites = (rem == 0) ? (size / maxLen) : ((size / maxLen) + 1);
  RADIOLIB_DEBUG_BASIC_PRINTLN("Writing image in %lu chunks, last chunk size is %lu words", (unsigned long)numWrites, (unsigned long)rem);
  for(size_t i = 0; i < numWrites; i ++) {
    uint32_t offset = i * maxLen;
    uint32_t len = (i == (numWrites - 1)) ? rem : maxLen;
    RADIOLIB_DEBUG_BASIC_PRINTLN("Writing chunk %d at offset %08lx (%u words)", (int)i, (unsigned long)offset, (unsigned int)len);
    this->bootWriteFlashEncrypted(offset*sizeof(uint32_t), const_cast<uint32_t*>(&image[offset]), len, nonvolatile);
  }

  // kick the device from bootloader
  state = this->reset();
  RADIOLIB_ASSERT(state);

  // verify we are no longer in bootloader
  state = this->getVersion(NULL, &device, NULL, NULL);
  RADIOLIB_ASSERT(state);
  if(device == RADIOLIB_LR11X0_DEVICE_BOOT) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("Failed to kick device from bootloader mode, %02x == %02x", (unsigned int)device, (unsigned int)RADIOLIB_LR11X0_DEVICE_BOOT);
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }

  return(state);
}

int16_t LR11x0::getModem(ModemType_t* modem) {
  RADIOLIB_ASSERT_PTR(modem);

  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);

  switch(type) {
    case(RADIOLIB_LR11X0_PACKET_TYPE_LORA):
      *modem = ModemType_t::RADIOLIB_MODEM_LORA;
      return(RADIOLIB_ERR_NONE);
    case(RADIOLIB_LR11X0_PACKET_TYPE_GFSK):
      *modem = ModemType_t::RADIOLIB_MODEM_FSK;
      return(RADIOLIB_ERR_NONE);
    case(RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS):
      *modem = ModemType_t::RADIOLIB_MODEM_LRFHSS;
      return(RADIOLIB_ERR_NONE);
  }
  
  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR11x0::stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg) {
  int16_t state;

  switch(mode) {
    case(RADIOLIB_RADIO_MODE_RX): {
      // check active modem
      uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
      state = getPacketType(&modem);
      RADIOLIB_ASSERT(state);
      if((modem != RADIOLIB_LR11X0_PACKET_TYPE_LORA) && 
        (modem != RADIOLIB_LR11X0_PACKET_TYPE_GFSK)) {
        return(RADIOLIB_ERR_WRONG_MODEM);
      }

      // set DIO mapping
      if(cfg->receive.timeout != RADIOLIB_LR11X0_RX_TIMEOUT_INF) {
        cfg->receive.irqMask |= (1UL << RADIOLIB_IRQ_TIMEOUT);
      }
      state = setDioIrqParams(getIrqMapped(cfg->receive.irqFlags & cfg->receive.irqMask));
      RADIOLIB_ASSERT(state);

      // clear interrupt flags
      state = clearIrqState(RADIOLIB_LR11X0_IRQ_ALL);
      RADIOLIB_ASSERT(state);

      // set implicit mode and expected len if applicable
      if((this->headerType == RADIOLIB_LRXXXX_LORA_HEADER_IMPLICIT) && (modem == RADIOLIB_LR11X0_PACKET_TYPE_LORA)) {
        state = setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, this->invertIQEnabled);
        RADIOLIB_ASSERT(state);
      }

      // if max(uint32_t) is used, revert to RxContinuous
      if(cfg->receive.timeout == 0xFFFFFFFF) {
        cfg->receive.timeout = 0xFFFFFF;
      }
      this->rxTimeout = cfg->receive.timeout;
    } break;
  
    case(RADIOLIB_RADIO_MODE_TX): {
      // check packet length
      if(cfg->transmit.len > RADIOLIB_LR11X0_MAX_PACKET_LENGTH) {
        return(RADIOLIB_ERR_PACKET_TOO_LONG);
      }

      // maximum packet length is decreased by 1 when address filtering is active
      if((this->addrComp != RADIOLIB_LR11X0_GFSK_ADDR_FILTER_DISABLED) && (cfg->transmit.len > RADIOLIB_LR11X0_MAX_PACKET_LENGTH - 1)) {
        return(RADIOLIB_ERR_PACKET_TOO_LONG);
      }

      // set packet Length
      uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
      state = getPacketType(&modem);
      RADIOLIB_ASSERT(state);
      if(modem == RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
        state = setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, cfg->transmit.len, this->crcTypeLoRa, this->invertIQEnabled);
      
      } else if(modem == RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
        state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, this->packetType, cfg->transmit.len, this->crcTypeGFSK, this->whitening);
      
      } else if(modem != RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
        return(RADIOLIB_ERR_UNKNOWN);
      
      }
      RADIOLIB_ASSERT(state);

      // set DIO mapping
      state = setDioIrqParams(RADIOLIB_LR11X0_IRQ_TX_DONE | RADIOLIB_LR11X0_IRQ_TIMEOUT);
      RADIOLIB_ASSERT(state);

      if(modem == RADIOLIB_LR11X0_PACKET_TYPE_LR_FHSS) {
        // in LR-FHSS mode, the packet is built by the device
        // TODO add configurable device offset
        state = LRxxxx::lrFhssBuildFrame(RADIOLIB_LR11X0_CMD_LR_FHSS_BUILD_FRAME, this->lrFhssHdrCount, this->lrFhssCr, this->lrFhssGrid, true, this->lrFhssBw, this->lrFhssHopSeq, 0, cfg->transmit.data, cfg->transmit.len);
        RADIOLIB_ASSERT(state);

      } else {
        // write packet to buffer
        state = writeBuffer8(cfg->transmit.data, cfg->transmit.len);
        RADIOLIB_ASSERT(state);

      }

      // clear interrupt flags
      state = clearIrqState(RADIOLIB_LR11X0_IRQ_ALL);
      RADIOLIB_ASSERT(state);
    } break;
    
    default:
      return(RADIOLIB_ERR_UNSUPPORTED);
  }

  this->stagedMode = mode;
  return(state);
}

int16_t LR11x0::launchMode() {
  int16_t state;
  switch(this->stagedMode) {
    case(RADIOLIB_RADIO_MODE_RX): {
      this->mod->setRfSwitchState(Module::MODE_RX);
      state = setRx(this->rxTimeout);
    } break;
  
    case(RADIOLIB_RADIO_MODE_TX): {
      this->mod->setRfSwitchState(Module::MODE_TX);
      state = setTx(RADIOLIB_LR11X0_TX_TIMEOUT_NONE);
      RADIOLIB_ASSERT(state);

      // wait for BUSY to go low (= PA ramp up done)
      while(this->mod->hal->digitalRead(this->mod->getGpio())) {
        this->mod->hal->yield();
      }
    } break;
    
    default:
      return(RADIOLIB_ERR_UNSUPPORTED);
  }

  this->stagedMode = RADIOLIB_RADIO_MODE_NONE;
  return(state);
}

int16_t LR11x0::workaroundGFSK() {
  // first, check we are using GFSK modem
  uint8_t modem = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if(modem != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    // not in GFSK, nothing to do here
    return(RADIOLIB_ERR_NONE);
  }

  // this seems to always be the first step (even when resetting)
  state = this->writeRegMemMask32(RADIOLIB_LR11X0_REG_GFSK_FIX1, 0x30, 0x10);
  RADIOLIB_ASSERT(state);

  // these are the default values that will be applied if nothing matches
  uint32_t valFix2 = 0x01;
  uint32_t valFix3 = 0x0A01;

  // next, decide what to change based on modulation properties
  if((this->bitRate == 1200) && (this->frequencyDev == 5000) && (this->rxBandwidth == RADIOLIB_LR11X0_GFSK_RX_BW_19_5)) {
    // workaround for 1.2 kbps
    valFix2 = 0x04;

  } else if((this->bitRate == 600) && (this->frequencyDev == 800) && (this->rxBandwidth == RADIOLIB_LR11X0_GFSK_RX_BW_4_8))  {
    // value to write depends on the frequency
    valFix3 = (this->freqMHz >= 1000.0f) ? 0x1100 : 0x0600;
  
  }

  // update the registers
  state = this->writeRegMemMask32(RADIOLIB_LR11X0_REG_GFSK_FIX2, 0x05, valFix2);
  RADIOLIB_ASSERT(state);
  return(this->writeRegMemMask32(RADIOLIB_LR11X0_REG_GFSK_FIX3, 0x01FF03, valFix3));
}

int16_t LR11x0::modSetup(float tcxoVoltage, uint8_t modem) {
  this->mod->init();
  this->mod->hal->pinMode(this->mod->getIrq(), this->mod->hal->GpioModeInput);
  this->mod->hal->pinMode(this->mod->getGpio(), this->mod->hal->GpioModeInput);
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_LR11X0_CMD_READ_REG_MEM;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_LR11X0_CMD_WRITE_REG_MEM;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP] = RADIOLIB_LR11X0_CMD_NOP;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_STATUS] = RADIOLIB_LR11X0_CMD_GET_STATUS;
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR] = Module::BITS_32;
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_8;
  this->gnss = false;

  // try to find the LR11x0 chip - this will also reset the module at least once
  if(!LR11x0::findChip(this->chipType)) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("No LR11x0 found!");
    this->mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  RADIOLIB_DEBUG_BASIC_PRINTLN("M\tLR11x0");

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set TCXO control, if requested
  if(!this->XTAL && tcxoVoltage > 0.0f) {
    state = setTCXO(tcxoVoltage);
    RADIOLIB_ASSERT(state);
  }

  // configure settings not accessible by API
  return(config(modem));
}

bool LR11x0::findChip(uint8_t ver) {
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    // reset the module
    reset();

    // read the version
    LR11x0VersionInfo_t info;
    int16_t state = getVersionInfo(&info);
    RADIOLIB_ASSERT(state);

    if((info.device == ver) || (info.device == RADIOLIB_LR11X0_DEVICE_BOOT)) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("Found LR11x0: RADIOLIB_LR11X0_CMD_GET_VERSION = 0x%02x", info.device);
      RADIOLIB_DEBUG_BASIC_PRINTLN("Base FW version: %d.%d", (int)info.fwMajor, (int)info.fwMinor);
      if(this->chipType != RADIOLIB_LR11X0_DEVICE_LR1121) {
        RADIOLIB_DEBUG_BASIC_PRINTLN("WiFi FW version: %d.%d", (int)info.fwMajorWiFi, (int)info.fwMinorWiFi);
        RADIOLIB_DEBUG_BASIC_PRINTLN("GNSS FW version: %d.%d", (int)info.fwGNSS, (int)info.almanacGNSS);
      }
      if(info.device == RADIOLIB_LR11X0_DEVICE_BOOT) {
        RADIOLIB_DEBUG_BASIC_PRINTLN("Warning: device is in bootloader mode! Only FW update is possible now.");
      }
      flagFound = true;
    } else {
      RADIOLIB_DEBUG_BASIC_PRINTLN("LR11x0 not found! (%d of 10 tries) RADIOLIB_LR11X0_CMD_GET_VERSION = 0x%02x", i + 1, info.device);
      RADIOLIB_DEBUG_BASIC_PRINTLN("Expected: 0x%02x", ver);
      this->mod->hal->delay(10);
      i++;
    }
  }
  

  return(flagFound);
}

int16_t LR11x0::config(uint8_t modem) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // set Rx/Tx fallback mode to STDBY_RC
  state = this->setRxTxFallbackMode(RADIOLIB_LR11X0_FALLBACK_MODE_STBY_RC);
  RADIOLIB_ASSERT(state);

  // clear IRQ
  state = this->clearIrqState(RADIOLIB_LR11X0_IRQ_ALL);
  state |= this->setDioIrqParams(RADIOLIB_LR11X0_IRQ_NONE);
  RADIOLIB_ASSERT(state);

  // calibrate all blocks
  state = this->calibrate(RADIOLIB_LR11X0_CALIBRATE_ALL);

  // wait for calibration completion
  this->mod->hal->delay(5);
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();
  }
  
  // if something failed, show the device errors
  #if RADIOLIB_DEBUG_BASIC
  if(state != RADIOLIB_ERR_NONE) {
    // unless mode is forced to standby, device errors will be 0
    standby();
    uint16_t errors = 0;
    getErrors(&errors);
    RADIOLIB_DEBUG_BASIC_PRINTLN("Calibration failed, device errors: 0x%X", errors);
  }
  #else
  RADIOLIB_ASSERT(state);
  #endif

  // set modem
  state = this->setPacketType(modem);
  return(state);
}

int16_t LR11x0::setPacketMode(uint8_t mode, uint8_t len) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  state = setPacketParamsGFSK(this->preambleLengthGFSK, this->preambleDetLength, this->syncWordLength, this->addrComp, mode, len, this->crcTypeGFSK, this->whitening);
  RADIOLIB_ASSERT(state);

  // update cached value
  this->packetType = mode;
  return(state);
}

int16_t LR11x0::startCad(uint8_t symbolNum, uint8_t detPeak, uint8_t detMin, uint8_t exitMode, RadioLibTime_t timeout) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // select CAD parameters
  // TODO the magic numbers are based on Semtech examples, this is probably suboptimal
  uint8_t num = symbolNum;
  if(num == RADIOLIB_LR11X0_CAD_PARAM_DEFAULT) {
    num = 2;
  }
  
  const uint8_t detPeakValues[8] = { 48, 48, 50, 55, 55, 59, 61, 65 };
  uint8_t peak = detPeak;
  if(peak == RADIOLIB_LR11X0_CAD_PARAM_DEFAULT) {
    peak = detPeakValues[this->spreadingFactor - 5];
  }

  uint8_t min = detMin;
  if(min == RADIOLIB_LR11X0_CAD_PARAM_DEFAULT) {
    min = 10;
  }

  uint8_t mode = exitMode; 
  if(mode == RADIOLIB_LR11X0_CAD_PARAM_DEFAULT) {
    mode = RADIOLIB_LR11X0_CAD_EXIT_MODE_STBY_RC;
  }

  uint32_t timeout_raw = (float)timeout / 30.52f;

  // set CAD parameters
  // TODO add configurable exit mode and timeout
  state = setCadParams(num, peak, min, mode, timeout_raw);
  RADIOLIB_ASSERT(state);

  // start CAD
  return(setCad());
}

int16_t LR11x0::setHeaderType(uint8_t hdrType, size_t len) {
  // check active modem
  uint8_t type = RADIOLIB_LR11X0_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR11X0_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  state = setPacketParamsLoRa(this->preambleLengthLoRa, hdrType, len, this->crcTypeLoRa, this->invertIQEnabled);
  RADIOLIB_ASSERT(state);

  // update cached value
  this->headerType = hdrType;
  this->implicitLen = len;

  return(state);
}

Module* LR11x0::getMod() {
  return(this->mod);
}

#endif
