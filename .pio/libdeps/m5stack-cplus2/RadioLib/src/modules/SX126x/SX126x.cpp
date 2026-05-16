#include "SX126x.h"
#include <string.h>
#include <math.h>
#if !RADIOLIB_EXCLUDE_SX126X

SX126x::SX126x(Module* mod) : PhysicalLayer() {
  this->freqStep = RADIOLIB_SX126X_FREQUENCY_STEP_SIZE;
  this->maxPacketLength = RADIOLIB_SX126X_MAX_PACKET_LENGTH;
  this->mod = mod;
  this->XTAL = false;
  this->standbyXOSC = false;
  this->irqMap[RADIOLIB_IRQ_TX_DONE] = RADIOLIB_SX126X_IRQ_TX_DONE;
  this->irqMap[RADIOLIB_IRQ_RX_DONE] = RADIOLIB_SX126X_IRQ_RX_DONE;
  this->irqMap[RADIOLIB_IRQ_PREAMBLE_DETECTED] = RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED;
  this->irqMap[RADIOLIB_IRQ_SYNC_WORD_VALID] = RADIOLIB_SX126X_IRQ_SYNC_WORD_VALID;
  this->irqMap[RADIOLIB_IRQ_HEADER_VALID] = RADIOLIB_SX126X_IRQ_HEADER_VALID;
  this->irqMap[RADIOLIB_IRQ_HEADER_ERR] = RADIOLIB_SX126X_IRQ_HEADER_ERR;
  this->irqMap[RADIOLIB_IRQ_CRC_ERR] = RADIOLIB_SX126X_IRQ_CRC_ERR;
  this->irqMap[RADIOLIB_IRQ_CAD_DONE] = RADIOLIB_SX126X_IRQ_CAD_DONE;
  this->irqMap[RADIOLIB_IRQ_CAD_DETECTED] = RADIOLIB_SX126X_IRQ_CAD_DETECTED;
  this->irqMap[RADIOLIB_IRQ_TIMEOUT] = RADIOLIB_SX126X_IRQ_TIMEOUT;
}

int16_t SX126x::begin(uint8_t cr, uint8_t syncWord, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // BW in kHz and SF are required in order to calculate LDRO for setModulationParams
  // set the defaults, this will get overwritten later anyway
  this->bandwidthKhz = 500.0;
  this->spreadingFactor = 9;

  // initialize configuration variables (will be overwritten during public settings configuration)
  this->bandwidth = RADIOLIB_SX126X_LORA_BW_500_0;  // initialized to 500 kHz, since lower values will interfere with LLCC68
  this->codingRate = RADIOLIB_SX126X_LORA_CR_4_7;
  this->ldrOptimize = 0x00;
  this->crcTypeLoRa = RADIOLIB_SX126X_LORA_CRC_ON;
  this->preambleLengthLoRa = preambleLength;
  this->tcxoDelay = 0;
  this->headerType = RADIOLIB_SX126X_LORA_HEADER_EXPLICIT;
  this->implicitLen = 0xFF;

  // set module properties and perform initial setup
  int16_t state = this->modSetup(tcxoVoltage, useRegulatorLDO, RADIOLIB_SX126X_PACKET_TYPE_LORA);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setCodingRate(cr);
  RADIOLIB_ASSERT(state);

  state = setSyncWord(syncWord);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  state = setCurrentLimit(60.0);
  RADIOLIB_ASSERT(state);

  state = setDio2AsRfSwitch(true);
  RADIOLIB_ASSERT(state);

  state = setCRC(2);
  RADIOLIB_ASSERT(state);

  state = invertIQ(false);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX126x::beginFSK(float br, float freqDev, float rxBw, uint16_t preambleLength, float tcxoVoltage, bool useRegulatorLDO) {
  // initialize configuration variables (will be overwritten during public settings configuration)
  this->bitRate = 21333;                                  // 48.0 kbps
  this->frequencyDev = 52428;                             // 50.0 kHz
  this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_156_2;
  this->rxBandwidthKhz = 156.2;
  this->pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_0_5;
  this->crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_2_BYTE_INV;     // CCITT CRC configuration
  this->preambleLengthFSK = preambleLength;

  // set module properties and perform initial setup
  int16_t state = this->modSetup(tcxoVoltage, useRegulatorLDO, RADIOLIB_SX126X_PACKET_TYPE_GFSK);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  state = setRxBandwidth(rxBw);
  RADIOLIB_ASSERT(state);

  state = setCurrentLimit(60.0);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  uint8_t sync[] = {0x12, 0xAD};
  state = setSyncWord(sync, 2);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  state = variablePacketLengthMode(RADIOLIB_SX126X_MAX_PACKET_LENGTH);
  RADIOLIB_ASSERT(state);

  state = setCRC(2);
  RADIOLIB_ASSERT(state);

  state = setDio2AsRfSwitch(true);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX126x::beginBPSK(float br, float tcxoVoltage, bool useRegulatorLDO) {
  // set module properties and perform initial setup
  int16_t state = this->modSetup(tcxoVoltage, useRegulatorLDO, RADIOLIB_SX126X_PACKET_TYPE_BPSK);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setBitRate(br);
  RADIOLIB_ASSERT(state);
  
  // set publicly accessible settings that are not a part of begin method
  state = setDio2AsRfSwitch(true);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX126x::beginLRFHSS(uint8_t bw, uint8_t cr, bool narrowGrid, float tcxoVoltage, bool useRegulatorLDO) {
  this->lrFhssGridNonFcc = narrowGrid;
  
  // set module properties and perform initial setup
  int16_t state = this->modSetup(tcxoVoltage, useRegulatorLDO, RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  state = setCurrentLimit(60.0);
  RADIOLIB_ASSERT(state);

  state = setDio2AsRfSwitch(true);
  RADIOLIB_ASSERT(state);

  // set all packet params to 0 (packet engine is disabled in LR-FHSS mode)
  state = setPacketParamsFSK(0, 0, 0, 0, 0, 0, 0, 0);
  RADIOLIB_ASSERT(state);

  // set bit rate
  this->rxBandwidth = 0;
  this->frequencyDev = 0;
  this->pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_1;
  state = setBitRate(0.48828125f);
  RADIOLIB_ASSERT(state);

  return(setLrFhssConfig(bw, cr));
}

int16_t SX126x::setLrFhssConfig(uint8_t bw, uint8_t cr, uint8_t hdrCount, uint16_t hopSeqId) {
  // check and cache all parameters
  RADIOLIB_CHECK_RANGE((int8_t)cr, (int8_t)RADIOLIB_SX126X_LR_FHSS_CR_5_6, (int8_t)RADIOLIB_SX126X_LR_FHSS_CR_1_3, RADIOLIB_ERR_INVALID_CODING_RATE);
  this->lrFhssCr = cr;
  RADIOLIB_CHECK_RANGE((int8_t)bw, (int8_t)RADIOLIB_SX126X_LR_FHSS_BW_39_06, (int8_t)RADIOLIB_SX126X_LR_FHSS_BW_1574_2, RADIOLIB_ERR_INVALID_BANDWIDTH);
  this->lrFhssBw = bw;
  RADIOLIB_CHECK_RANGE(hdrCount, 1, 4, RADIOLIB_ERR_INVALID_BIT_RANGE);
  this->lrFhssHdrCount = hdrCount;
  RADIOLIB_CHECK_RANGE((int16_t)hopSeqId, (int16_t)0x000, (int16_t)0x1FF, RADIOLIB_ERR_INVALID_DATA_SHAPING);
  this->lrFhssHopSeqId = hopSeqId;
  return(RADIOLIB_ERR_NONE);
}

int16_t SX126x::reset(bool verify) {
  // run the reset sequence
  this->mod->hal->pinMode(this->mod->getRst(), this->mod->hal->GpioModeOutput);
  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelLow);
  this->mod->hal->delay(1);
  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelHigh);

  // return immediately when verification is disabled
  if(!verify) {
    return(RADIOLIB_ERR_NONE);
  }

  // set mode to standby - SX126x often refuses first few commands after reset
  RadioLibTime_t start = this->mod->hal->millis();
  while(true) {
    // try to set mode to standby
    int16_t state = standby();
    if(state == RADIOLIB_ERR_NONE) {
      // standby command successful
      return(RADIOLIB_ERR_NONE);
    }

    // standby command failed, check timeout and try again
    if(this->mod->hal->millis() - start >= 1000) {
      // timed out, possibly incorrect wiring
      return(state);
    }

    // wait a bit to not spam the module
    this->mod->hal->delay(10);
  }
}

int16_t SX126x::transmit(const uint8_t* data, size_t len, uint8_t addr) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check packet length
  if(this->codingRate > RADIOLIB_SX126X_LORA_CR_4_8) {
    // Long Interleaver needs at least 8 bytes
    if(len < 8) {
      return(RADIOLIB_ERR_PACKET_TOO_SHORT);
    }

    // Long Interleaver supports up to 253 bytes if CRC is enabled
    if(this->crcTypeLoRa == RADIOLIB_SX126X_LORA_CRC_ON && (len > RADIOLIB_SX126X_MAX_PACKET_LENGTH - 2)) {
      return(RADIOLIB_ERR_PACKET_TOO_LONG);
    }  
  } 
  if(len > RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // calculate timeout in ms (5ms + 500 % of expected time-on-air)
  RadioLibTime_t timeout = 5 + (getTimeOnAir(len) * 5) / 1000;
  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu ms", timeout);

  // start transmission
  state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  // wait for packet transmission or timeout
  uint8_t modem = getPacketType();
  RadioLibTime_t start = this->mod->hal->millis();
  while(true) {
    // yield for  multi-threaded platforms
    this->mod->hal->yield();

    // check timeout
    if(this->mod->hal->millis() - start > timeout) {
      finishTransmit();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }

    // poll the interrupt pin
    if(this->mod->hal->digitalRead(this->mod->getIrq())) {
      // in LoRa or GFSK, only Tx done interrupt is enabled
      if(modem != RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS) {
        break;
      }

      // in LR-FHSS, IRQ signals both Tx done as frequency hop request
      if(this->getIrqFlags() & RADIOLIB_SX126X_IRQ_TX_DONE) {
        break;
      } else {
        // handle frequency hop
        this->hopLRFHSS();
      }
    }
  }

  return(finishTransmit());
}

int16_t SX126x::receive(uint8_t* data, size_t len, RadioLibTime_t timeout) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  RadioLibTime_t timeoutInternal = timeout;
  if(!timeoutInternal) {
    // calculate timeout (500 % of expected time-one-air)
    size_t maxLen = len;
    if(len == 0) { maxLen = RADIOLIB_SX126X_MAX_PACKET_LENGTH; }
    timeoutInternal = (getTimeOnAir(maxLen) * 5) / 1000;
  }

  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu ms", timeoutInternal);

  // start reception
  uint32_t timeoutValue = (uint32_t)(((float)timeoutInternal * 1000.0f) / 15.625f);
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

int16_t SX126x::transmitDirect(uint32_t frf) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(this->txMode);

  // user requested to start transmitting immediately (required for RTTY)
  int16_t state = RADIOLIB_ERR_NONE;
  if(frf != 0) {
    state = setRfFrequency(frf);
  }
  RADIOLIB_ASSERT(state);

  // direct mode activation intentionally skipped here, as it seems to lead to much worse results
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_TX_CONTINUOUS_WAVE, NULL, 0));
}

int16_t SX126x::receiveDirect() {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // SX126x is unable to output received data directly
  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX126x::directMode() {
  // check modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // disable DIO2 RF switch
  state = setDio2AsRfSwitch(false);
  RADIOLIB_ASSERT(state);

  // set DIO2 to clock output and DIO3 to data input
  // this is done exclusively by writing magic values to even more magic registers
  state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_TX_BITBANG_ENABLE_1, RADIOLIB_SX126X_TX_BITBANG_1_ENABLED, 6, 4);
  RADIOLIB_ASSERT(state);
  state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_TX_BITBANG_ENABLE_0, RADIOLIB_SX126X_TX_BITBANG_0_ENABLED, 3, 0);
  RADIOLIB_ASSERT(state);
  state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_DIOX_OUT_ENABLE, RADIOLIB_SX126X_DIO3_OUT_DISABLED, 3, 3);
  RADIOLIB_ASSERT(state);
  state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_DIOX_IN_ENABLE, RADIOLIB_SX126X_DIO3_IN_ENABLED, 3, 3);
  RADIOLIB_ASSERT(state);

  // enable TxDone interrupt
  state = setDioIrqParams(RADIOLIB_SX126X_IRQ_TX_DONE, RADIOLIB_SX126X_IRQ_TX_DONE);
  RADIOLIB_ASSERT(state);

  // set preamble length to the maximum to prevent SX126x from exiting Tx mode for a while
  state = setPreambleLength(0xFFFF);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX126x::packetMode() {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set preamble length to the default
  state = setPreambleLength(16);
  RADIOLIB_ASSERT(state);

  // disable TxDone interrupt
  state = setDioIrqParams(RADIOLIB_SX126X_IRQ_NONE, RADIOLIB_SX126X_IRQ_NONE);
  RADIOLIB_ASSERT(state);

  // restore the magic registers
  state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_DIOX_IN_ENABLE, RADIOLIB_SX126X_DIO3_IN_DISABLED, 3, 3);
  RADIOLIB_ASSERT(state);
  state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_DIOX_OUT_ENABLE, RADIOLIB_SX126X_DIO3_OUT_ENABLED, 3, 3);
  RADIOLIB_ASSERT(state);
  state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_TX_BITBANG_ENABLE_0, RADIOLIB_SX126X_TX_BITBANG_0_DISABLED, 3, 0);
  RADIOLIB_ASSERT(state);
  state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_TX_BITBANG_ENABLE_1, RADIOLIB_SX126X_TX_BITBANG_1_DISABLED, 6, 4);
  RADIOLIB_ASSERT(state);

  // enable DIO2 RF switch
  state = setDio2AsRfSwitch(true);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX126x::scanChannel() {
  ChannelScanConfig_t cfg = {
    .cad = {
      .symNum = RADIOLIB_SX126X_CAD_PARAM_DEFAULT,
      .detPeak = RADIOLIB_SX126X_CAD_PARAM_DEFAULT,
      .detMin = RADIOLIB_SX126X_CAD_PARAM_DEFAULT,
      .exitMode = RADIOLIB_SX126X_CAD_PARAM_DEFAULT,
      .timeout = 0,
      .irqFlags = RADIOLIB_IRQ_CAD_DEFAULT_FLAGS,
      .irqMask = RADIOLIB_IRQ_CAD_DEFAULT_MASK,
    },
  };
  return(this->scanChannel(cfg));
}

int16_t SX126x::scanChannel(const ChannelScanConfig_t &config) {
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

int16_t SX126x::hopLRFHSS() {
  if(!(this->getIrqFlags() & RADIOLIB_SX126X_IRQ_LR_FHSS_HOP)) {
    return(RADIOLIB_ERR_TX_TIMEOUT);
  }

  int16_t state = this->setLRFHSSHop(this->lrFhssHopNum % 16);
  RADIOLIB_ASSERT(state);
  return(clearIrqStatus());
}

int16_t SX126x::finishTransmit() {
  // clear interrupt flags
  int16_t state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // set mode to standby to disable transmitter/RF switch
  return(standby());
}

int16_t SX126x::finishReceive() {
  // set mode to standby to disable RF switch
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // try to fix timeout error in implicit header mode
  // check for modem type and header mode is done in fixImplicitTimeout()
  state = fixImplicitTimeout();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  return(clearIrqStatus());
}

int16_t SX126x::startReceive() {
  return(this->startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF, RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RADIOLIB_IRQ_RX_DEFAULT_MASK, 0));
}

int16_t SX126x::startReceiveDutyCycle(uint32_t rxPeriod, uint32_t sleepPeriod, RadioLibIrqFlags_t irqFlags, RadioLibIrqFlags_t irqMask) {
  // datasheet claims time to go to sleep is ~500us, same to wake up, compensate for that with 1 ms + TCXO delay
  uint32_t transitionTime = this->tcxoDelay + 1000;
  sleepPeriod -= transitionTime;

  // divide by 15.625
  uint32_t rxPeriodRaw = (rxPeriod * 8) / 125;
  uint32_t sleepPeriodRaw = (sleepPeriod * 8) / 125;

  // check 24 bit limit and zero value (likely not intended)
  if((rxPeriodRaw & 0xFF000000) || (rxPeriodRaw == 0)) {
    return(RADIOLIB_ERR_INVALID_RX_PERIOD);
  }

  // this check of the high byte also catches underflow when we subtracted transitionTime
  if((sleepPeriodRaw & 0xFF000000) || (sleepPeriodRaw == 0)) {
    return(RADIOLIB_ERR_INVALID_SLEEP_PERIOD);
  }

  int16_t state = startReceiveCommon(RADIOLIB_SX126X_RX_TIMEOUT_INF, irqFlags, irqMask);
  RADIOLIB_ASSERT(state);

  const uint8_t data[6] = {(uint8_t)((rxPeriodRaw >> 16) & 0xFF), (uint8_t)((rxPeriodRaw >> 8) & 0xFF), (uint8_t)(rxPeriodRaw & 0xFF),
                     (uint8_t)((sleepPeriodRaw >> 16) & 0xFF), (uint8_t)((sleepPeriodRaw >> 8) & 0xFF), (uint8_t)(sleepPeriodRaw & 0xFF)};
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_RX_DUTY_CYCLE, data, 6));
}

int16_t SX126x::startReceiveDutyCycleAuto(uint16_t senderPreambleLength, uint16_t minSymbols, RadioLibIrqFlags_t irqFlags, RadioLibIrqFlags_t irqMask) {
  if(senderPreambleLength == 0) {
    senderPreambleLength = this->preambleLengthLoRa;
  } else if (senderPreambleLength > this->preambleLengthLoRa) {
    // the unit must be configured to expect a preamble length at least as long as the sender is using
    return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
  }
  if(minSymbols == 0) {
    if (this->spreadingFactor <= 6) {
      minSymbols = 12;
    } else {
      minSymbols = 8;
    }
  }

  // worst case is that the sender starts transmitting when we're just less than minSymbols from going back to sleep.
  // in this case, we don't catch minSymbols before going to sleep,
  // so we must be awake for at least that long before the sender stops transmitting.
  uint16_t sleepSymbols = senderPreambleLength - 2 * minSymbols;

  // if we're not to sleep at all, just use the standard startReceive.
  if(2 * minSymbols > senderPreambleLength) {
    return(startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF, irqFlags, irqMask));
  }

  uint32_t symbolLength = ((uint32_t)(10 * 1000) << this->spreadingFactor) / (10 * this->bandwidthKhz);
  uint32_t sleepPeriod = symbolLength * sleepSymbols;
  RADIOLIB_DEBUG_BASIC_PRINTLN("Auto sleep period: %lu", (long unsigned int)sleepPeriod);

  // when the unit detects a preamble, it starts a timer that will timeout if it doesn't receive a header in time.
  // the duration is sleepPeriod + 2 * wakePeriod.
  // The sleepPeriod doesn't take into account shutdown and startup time for the unit (~1ms)
  // We need to ensure that the timeout is longer than senderPreambleLength.
  // So we must satisfy: wakePeriod > (preamblePeriod - (sleepPeriod - 1000)) / 2. (A)
  // we also need to ensure the unit is awake to see at least minSymbols. (B)
  uint32_t wakePeriod = RADIOLIB_MAX(
    (symbolLength * (senderPreambleLength + 1) - (sleepPeriod - 1000)) / 2, // (A)
    symbolLength * (minSymbols + 1)); //(B)
  RADIOLIB_DEBUG_BASIC_PRINTLN("Auto wake period: %lu", (long unsigned int)wakePeriod);

  // If our sleep period is shorter than our transition time, just use the standard startReceive
  if(sleepPeriod < this->tcxoDelay + 1016) {
    return(startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF, irqFlags, irqMask));
  }

  return(startReceiveDutyCycle(wakePeriod, sleepPeriod, irqFlags, irqMask));
}

int16_t SX126x::startReceiveCommon(uint32_t timeout, RadioLibIrqFlags_t irqFlags, RadioLibIrqFlags_t irqMask) {
  // ensure we are in standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set DIO mapping
  if(timeout != RADIOLIB_SX126X_RX_TIMEOUT_INF) {
    irqMask |= (1UL << RADIOLIB_IRQ_TIMEOUT);
  }
  state = setDioIrqParams(getIrqMapped(irqFlags), getIrqMapped(irqMask));
  RADIOLIB_ASSERT(state);

  // set buffer pointers
  state = setBufferBaseAddress();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // restore original packet length
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    state = setPacketParams(this->preambleLengthLoRa, this->crcTypeLoRa, this->implicitLen, this->headerType, this->invertIQEnabled);
  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    state = setPacketParamsFSK(this->preambleLengthFSK, this->preambleDetLength, this->crcTypeFSK, this->syncWordLength, RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF, this->whitening, this->packetType);
  } else {
    return(RADIOLIB_ERR_UNKNOWN);
  }

  return(state);
}

int16_t SX126x::readData(uint8_t* data, size_t len) {
  // this method may get called from receive() after Rx timeout
  // if that's the case, the first call will return "SPI command timeout error"
  // check the IRQ to be sure this really originated from timeout event
  int16_t state = this->mod->SPIcheckStream();
  uint16_t irq = getIrqFlags();
  if((state == RADIOLIB_ERR_SPI_CMD_TIMEOUT) && (irq & RADIOLIB_SX126X_IRQ_TIMEOUT)) {
    // this is definitely Rx timeout
    return(RADIOLIB_ERR_RX_TIMEOUT);
  }
  RADIOLIB_ASSERT(state);

  // check integrity CRC
  int16_t crcState = RADIOLIB_ERR_NONE;
  // Report CRC mismatch when there's a payload CRC error, or a header error and no valid header (to avoid false alarm from previous packet)
  if((irq & RADIOLIB_SX126X_IRQ_CRC_ERR) || ((irq & RADIOLIB_SX126X_IRQ_HEADER_ERR) && !(irq & RADIOLIB_SX126X_IRQ_HEADER_VALID))) {
    crcState = RADIOLIB_ERR_CRC_MISMATCH;
  }
  
  // get packet length and Rx buffer offset
  uint8_t offset = 0;
  size_t length = getPacketLength(true, &offset);
  if((len != 0) && (len < length)) {
    // user requested less data than we got, only return what was requested
    length = len;
  }

  // read packet data starting at offset
  state = readBuffer(data, length, offset);
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();

  // check if CRC failed - this is done after reading data to give user the option to keep them
  RADIOLIB_ASSERT(crcState);

  return(state);
}

int16_t SX126x::startChannelScan() {
  ChannelScanConfig_t cfg = {
    .cad = {
      .symNum = RADIOLIB_SX126X_CAD_PARAM_DEFAULT,
      .detPeak = RADIOLIB_SX126X_CAD_PARAM_DEFAULT,
      .detMin = RADIOLIB_SX126X_CAD_PARAM_DEFAULT,
      .exitMode = RADIOLIB_SX126X_CAD_PARAM_DEFAULT,
      .timeout = 0,
      .irqFlags = RADIOLIB_IRQ_CAD_DEFAULT_FLAGS,
      .irqMask = RADIOLIB_IRQ_CAD_DEFAULT_MASK,
    },
  };
  return(this->startChannelScan(cfg));
}

int16_t SX126x::startChannelScan(const ChannelScanConfig_t &config) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // set DIO pin mapping
  state = setDioIrqParams(getIrqMapped(config.cad.irqFlags), getIrqMapped(config.cad.irqMask));
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // set mode to CAD
  state = setCad(config.cad.symNum, config.cad.detPeak, config.cad.detMin, config.cad.exitMode, config.cad.timeout);
  return(state);
}

int16_t SX126x::getChannelScanResult() {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check CAD result
  uint16_t cadResult = getIrqFlags();
  if(cadResult & RADIOLIB_SX126X_IRQ_CAD_DETECTED) {
    // detected some LoRa activity
    return(RADIOLIB_LORA_DETECTED);
  } else if(cadResult & RADIOLIB_SX126X_IRQ_CAD_DONE) {
    // channel is free
    return(RADIOLIB_CHANNEL_FREE);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

float SX126x::getRSSI() {
  return(this->getRSSI(true));
}

float SX126x::getRSSI(bool packet) {
  if(packet) { 
    // get last packet RSSI from packet status
    uint32_t packetStatus = getPacketStatus();
    uint8_t rssiPkt = packetStatus & 0xFF;
    return(-1.0 * rssiPkt/2.0);
  } else {
    // get instantaneous RSSI value
    uint8_t rssiRaw = 0;
    this->mod->SPIreadStream(RADIOLIB_SX126X_CMD_GET_RSSI_INST, &rssiRaw, 1);
    return((float)rssiRaw / (-2.0f));
  }
}

float SX126x::getSNR() {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // get last packet SNR from packet status
  uint32_t packetStatus = getPacketStatus();
  uint8_t snrPkt = (packetStatus >> 8) & 0xFF;
  if(snrPkt < 128) {
    return(snrPkt/4.0);
  } else {
    return((snrPkt - 256)/4.0);
  }
}

float SX126x::getFrequencyError() {
  // check active modem
  uint8_t modem = getPacketType();
  if(modem != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(0.0);
  }

  // read the raw frequency error register values
  uint8_t efeRaw[3] = {0};
  int16_t state = readRegister(RADIOLIB_SX126X_REG_FREQ_ERROR_RX_CRC, &efeRaw[0], 1);
  RADIOLIB_ASSERT(state);
  state = readRegister(RADIOLIB_SX126X_REG_FREQ_ERROR_RX_CRC + 1, &efeRaw[1], 1);
  RADIOLIB_ASSERT(state);
  state = readRegister(RADIOLIB_SX126X_REG_FREQ_ERROR_RX_CRC + 2, &efeRaw[2], 1);
  RADIOLIB_ASSERT(state);
  uint32_t efe = ((uint32_t) efeRaw[0] << 16) | ((uint32_t) efeRaw[1] << 8) | efeRaw[2];
  efe &= 0x0FFFFF;

  float error = 0;

  // check the first bit
  if (efe & 0x80000) {
    // frequency error is negative
    efe |= (uint32_t) 0xFFF00000;
    efe = ~efe + 1;
    error = 1.55f * (float) efe / (1600.0f / (float) this->bandwidthKhz) * -1.0f;
  } else {
    error = 1.55f * (float) efe / (1600.0f / (float) this->bandwidthKhz);
  }

  return(error);
}

size_t SX126x::getPacketLength(bool update) {
  return(this->getPacketLength(update, NULL));
}

size_t SX126x::getPacketLength(bool update, uint8_t* offset) {
  (void)update;

  // in implicit mode, return the cached value if the offset was not requested
  if((getPacketType() == RADIOLIB_SX126X_PACKET_TYPE_LORA) && (this->headerType == RADIOLIB_SX126X_LORA_HEADER_IMPLICIT) && (!offset)) {
    return(this->implicitLen);
  }

  // if offset was requested, or in explicit mode, we always have to perform the SPI transaction
  uint8_t rxBufStatus[2] = {0, 0};
  this->mod->SPIreadStream(RADIOLIB_SX126X_CMD_GET_RX_BUFFER_STATUS, rxBufStatus, 2);

  if(offset) { *offset = rxBufStatus[1]; }

  return((size_t)rxBufStatus[0]);
}

int16_t SX126x::getLoRaRxHeaderInfo(uint8_t* cr, bool* hasCRC) {
  int16_t state = RADIOLIB_ERR_NONE;

  // check if in explicit header mode
  if(this->headerType == RADIOLIB_SX126X_LORA_HEADER_IMPLICIT) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(cr) { *cr = this->mod->SPIgetRegValue(RADIOLIB_SX126X_REG_LORA_RX_CODING_RATE, 6, 4) >> 4; }
  if(hasCRC) { *hasCRC = (this->mod->SPIgetRegValue(RADIOLIB_SX126X_REG_FREQ_ERROR_RX_CRC, 4, 4) != 0); }

  return(state);
}

RadioLibTime_t SX126x::calculateTimeOnAir(ModemType_t modem, DataRate_t dr, PacketConfig_t pc, size_t len) {
  // everything is in microseconds to allow integer arithmetic
  // some constants have .25, these are multiplied by 4, and have _x4 postfix to indicate that fact
  switch (modem) {
    case RADIOLIB_MODEM_LORA: {
      uint32_t symbolLength_us = ((uint32_t)(1000 * 10) << dr.lora.spreadingFactor) / (dr.lora.bandwidth * 10) ;
      uint8_t sfCoeff1_x4 = 17; // (4.25 * 4)
      uint8_t sfCoeff2 = 8;
      if(dr.lora.spreadingFactor == 5 || dr.lora.spreadingFactor == 6) {
        sfCoeff1_x4 = 25; // 6.25 * 4
        sfCoeff2 = 0;
      }
      uint8_t sfDivisor = 4*dr.lora.spreadingFactor;
      if(pc.lora.ldrOptimize) {
        sfDivisor = 4*(dr.lora.spreadingFactor - 2);
      }
      const int8_t bitsPerCrc = 16;
      const int8_t N_symbol_header = pc.lora.implicitHeader ? 0 : 20;

      // numerator of equation in section 6.1.4 of SX1268 datasheet v1.1 (might not actually be bitcount, but it has len * 8)
      int16_t bitCount = (int16_t) 8 * len + pc.lora.crcEnabled * bitsPerCrc - 4 * dr.lora.spreadingFactor  + sfCoeff2 + N_symbol_header;
      if(bitCount < 0) {
        bitCount = 0;
      }
      // add (sfDivisor) - 1 to the numerator to give integer CEIL(...)
      uint16_t nPreCodedSymbols = (bitCount + (sfDivisor - 1)) / (sfDivisor);

      // preamble can be 65k, therefore nSymbol_x4 needs to be 32 bit
      uint32_t nSymbol_x4 = (pc.lora.preambleLength + 8) * 4 + sfCoeff1_x4 + nPreCodedSymbols * dr.lora.codingRate * 4;

      return((symbolLength_us * nSymbol_x4) / 4);
    }
    case RADIOLIB_MODEM_FSK: {
       return((((float)(pc.fsk.crcLength * 8) + pc.fsk.syncWordLength + pc.fsk.preambleLength + (uint32_t)len * 8) / (dr.fsk.bitRate / 1000.0f)));
    }
    case RADIOLIB_MODEM_LRFHSS: {
      // calculate the number of bits based on coding rate
      uint16_t N_bits;
      switch(dr.lrFhss.cr) {
        case RADIOLIB_SX126X_LR_FHSS_CR_5_6:
          N_bits = ((len * 6) + 4) / 5; // this is from the official LR11xx driver, but why the extra +4?
          break;
        case RADIOLIB_SX126X_LR_FHSS_CR_2_3:
          N_bits = (len * 3) / 2;
          break;
        case RADIOLIB_SX126X_LR_FHSS_CR_1_2:
          N_bits = len * 2;
          break;
        case RADIOLIB_SX126X_LR_FHSS_CR_1_3:
          N_bits = len * 3;
          break;
        default:
          return(RADIOLIB_ERR_INVALID_CODING_RATE);
      }

      // calculate number of bits when accounting for unaligned last block
      uint16_t N_payBits = (N_bits / RADIOLIB_SX126X_LR_FHSS_FRAG_BITS) * RADIOLIB_SX126X_LR_FHSS_BLOCK_BITS;
      uint16_t N_lastBlockBits = N_bits % RADIOLIB_SX126X_LR_FHSS_FRAG_BITS;
      if(N_lastBlockBits) {
        N_payBits += N_lastBlockBits + 2;
      }

      // add header bits
      uint16_t N_totalBits = (RADIOLIB_SX126X_LR_FHSS_HEADER_BITS * pc.lrFhss.hdrCount) + N_payBits;
      return(((uint32_t)N_totalBits * 8 * 1000000UL) / 488.28215f);
    }
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

RadioLibTime_t SX126x::getTimeOnAir(size_t len) {
  uint8_t type = getPacketType();
  ModemType_t modem = RADIOLIB_MODEM_LORA;
  DataRate_t dataRate = {};
  PacketConfig_t packetConfig = {};

  if(type == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    uint8_t cr = this->codingRate;
    // We assume same calculation for short and long interleaving, so map CR values 0-4 and 5-7 to the same values
    if (cr < 5) {
      cr = cr + 4;
    } else if (cr == 7) {
      cr = cr + 1;
    }

    dataRate.lora.codingRate = cr;
    dataRate.lora.spreadingFactor = this->spreadingFactor;
    dataRate.lora.bandwidth = this->bandwidthKhz;

    packetConfig.lora.preambleLength = this->preambleLengthLoRa;
    packetConfig.lora.crcEnabled = (bool)this->crcTypeLoRa;
    packetConfig.lora.implicitHeader = this->headerType == RADIOLIB_SX126X_LORA_HEADER_IMPLICIT;
    packetConfig.lora.ldrOptimize = (bool)this->ldrOptimize;
  } else if(type == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    modem = RADIOLIB_MODEM_FSK;

    dataRate.fsk.bitRate = RADIOLIB_SX126X_CRYSTAL_FREQ * 32.0f * 1000.0f / (float)this->bitRate;
    dataRate.fsk.freqDev = (float)this->frequencyDev;

    uint8_t crcLen = 0;
    if(this->crcTypeFSK == RADIOLIB_SX126X_GFSK_CRC_1_BYTE || this->crcTypeFSK == RADIOLIB_SX126X_GFSK_CRC_1_BYTE_INV) {
      crcLen = 1;
    } else if(this->crcTypeFSK == RADIOLIB_SX126X_GFSK_CRC_2_BYTE || this->crcTypeFSK == RADIOLIB_SX126X_GFSK_CRC_2_BYTE_INV) {
      crcLen = 2;
    }

    packetConfig.fsk.preambleLength = this->preambleLengthFSK;
    packetConfig.fsk.syncWordLength = this->syncWordLength;
    packetConfig.fsk.crcLength = crcLen;
  } else if(type == RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS) {
    modem = RADIOLIB_MODEM_LRFHSS;

    dataRate.lrFhss.bw = this->lrFhssBw;
    dataRate.lrFhss.cr = this->lrFhssCr;
    dataRate.lrFhss.narrowGrid = this->lrFhssGridNonFcc;
    
    packetConfig.lrFhss.hdrCount = this->lrFhssHdrCount;
  } else if(type == RADIOLIB_SX126X_PACKET_TYPE_BPSK) {
    // BPSK is so experimental it does not have a specific data rate structure
    // so just reuse FSK
    modem = RADIOLIB_MODEM_FSK;

    dataRate.fsk.bitRate = RADIOLIB_SX126X_CRYSTAL_FREQ * 32.0f * 1000.0f / (float)this->bitRate;
    dataRate.fsk.freqDev = 0;

    packetConfig.fsk.preambleLength = 0;
    packetConfig.fsk.syncWordLength = 0;
    packetConfig.fsk.crcLength = 0;

  } else {
    return(RADIOLIB_ERR_WRONG_MODEM);
  
  }

  return(calculateTimeOnAir(modem, dataRate, packetConfig, len));
}

RadioLibTime_t SX126x::calculateRxTimeout(RadioLibTime_t timeoutUs) {
  // the timeout value is given in units of 15.625 microseconds
  // the calling function should provide some extra width, as this number of units is truncated to integer
  RadioLibTime_t timeout = timeoutUs / 15.625f;
  return(timeout);
}

uint32_t SX126x::getIrqFlags() {
  uint8_t data[] = { 0x00, 0x00 };
  this->mod->SPIreadStream(RADIOLIB_SX126X_CMD_GET_IRQ_STATUS, data, 2);
  return(((uint32_t)(data[0]) << 8) | data[1]);
}

int16_t SX126x::setIrqFlags(uint32_t irq) {
  return(this->setDioIrqParams(irq, irq));
}

int16_t SX126x::clearIrqFlags(uint32_t irq) {
  return(this->clearIrqStatus(irq));
}

uint8_t SX126x::randomByte() {
  // set some magic registers
  this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_ANA_LNA, RADIOLIB_SX126X_LNA_RNG_ENABLED, 0, 0);
  this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_ANA_MIXER, RADIOLIB_SX126X_MIXER_RNG_ENABLED, 0, 0);

  // set mode to Rx
  setRx(RADIOLIB_SX126X_RX_TIMEOUT_INF);

  // wait a bit for the RSSI reading to stabilise
  this->mod->hal->delay(10);

  // read RSSI value 8 times, always keep just the least significant bit
  uint8_t randByte = 0x00;
  for(uint8_t i = 0; i < 8; i++) {
    uint8_t val = 0x00;
    readRegister(RADIOLIB_SX126X_REG_RANDOM_NUMBER_0, &val, sizeof(uint8_t));
    randByte |= ((val & 0x01) << i);
  }

  // set mode to standby
  standby();

  // restore the magic registers
  this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_ANA_LNA, RADIOLIB_SX126X_LNA_RNG_DISABLED, 0, 0);
  this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_ANA_MIXER, RADIOLIB_SX126X_MIXER_RNG_DISABLED, 0, 0);

  return(randByte);
}

int16_t SX126x::getModem(ModemType_t* modem) {
  RADIOLIB_ASSERT_PTR(modem);

  uint8_t type = getPacketType();
  switch(type) {
    case(RADIOLIB_SX126X_PACKET_TYPE_LORA):
      *modem = ModemType_t::RADIOLIB_MODEM_LORA;
      return(RADIOLIB_ERR_NONE);
    case(RADIOLIB_SX126X_PACKET_TYPE_GFSK):
      *modem = ModemType_t::RADIOLIB_MODEM_FSK;
      return(RADIOLIB_ERR_NONE);
    case(RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS):
      *modem = ModemType_t::RADIOLIB_MODEM_LRFHSS;
      return(RADIOLIB_ERR_NONE);
  }
  
  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX126x::stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg) {
  int16_t state;

  switch(mode) {
    case(RADIOLIB_RADIO_MODE_RX): {
      // in implicit header mode, use the provided length if it is nonzero
      // otherwise we trust the user has previously set the payload length manually
      if((this->headerType == RADIOLIB_SX126X_LORA_HEADER_IMPLICIT) && (cfg->receive.len != 0)) {
        this->implicitLen = cfg->receive.len;
      }

      state = startReceiveCommon(cfg->receive.timeout, cfg->receive.irqFlags, cfg->receive.irqMask);
      RADIOLIB_ASSERT(state);

      // if max(uint32_t) is used, revert to RxContinuous
      if(cfg->receive.timeout == 0xFFFFFFFF) {
        cfg->receive.timeout = 0xFFFFFF;
      }
      this->rxTimeout = cfg->receive.timeout;
    } break;
  
    case(RADIOLIB_RADIO_MODE_TX): {
      // check packet length
      if(cfg->transmit.len > RADIOLIB_SX126X_MAX_PACKET_LENGTH) {
        return(RADIOLIB_ERR_PACKET_TOO_LONG);
      }

      // maximum packet length is decreased by 1 when address filtering is active
      if((RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF != RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF) && 
        (cfg->transmit.len > RADIOLIB_SX126X_MAX_PACKET_LENGTH - 1)) {
        return(RADIOLIB_ERR_PACKET_TOO_LONG);
      }

      // set packet Length
      state = RADIOLIB_ERR_NONE;
      uint8_t modem = getPacketType();
      if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
        state = setPacketParams(this->preambleLengthLoRa, this->crcTypeLoRa, cfg->transmit.len, this->headerType, this->invertIQEnabled);
      
      } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
        state = setPacketParamsFSK(this->preambleLengthFSK, this->preambleDetLength, this->crcTypeFSK, this->syncWordLength, RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF, this->whitening, this->packetType, cfg->transmit.len);
      
      } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_BPSK) {
        uint16_t rampUp = RADIOLIB_SX126X_BPSK_RAMP_UP_TIME_600_BPS;
        uint16_t rampDown = RADIOLIB_SX126X_BPSK_RAMP_DOWN_TIME_600_BPS;
        if(this->bitRate == 100) {
          rampUp = RADIOLIB_SX126X_BPSK_RAMP_UP_TIME_100_BPS;
          rampDown = RADIOLIB_SX126X_BPSK_RAMP_DOWN_TIME_100_BPS;
        }
        state = setPacketParamsBPSK(cfg->transmit.len, rampUp, rampDown, 8*cfg->transmit.len);
      
      } else if(modem != RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS) {
        return(RADIOLIB_ERR_UNKNOWN);
      
      }
      RADIOLIB_ASSERT(state);

      // set DIO mapping
      if(modem != RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS) {
        state = setDioIrqParams(RADIOLIB_SX126X_IRQ_TX_DONE | RADIOLIB_SX126X_IRQ_TIMEOUT, RADIOLIB_SX126X_IRQ_TX_DONE);
      } else {
        state = setDioIrqParams(RADIOLIB_SX126X_IRQ_TX_DONE | RADIOLIB_SX126X_IRQ_LR_FHSS_HOP, RADIOLIB_SX126X_IRQ_TX_DONE | RADIOLIB_SX126X_IRQ_LR_FHSS_HOP);
      }
      RADIOLIB_ASSERT(state);

      // set buffer pointers
      state = setBufferBaseAddress();
      RADIOLIB_ASSERT(state);

      // write packet to buffer
      if(modem != RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS) {
        state = writeBuffer(cfg->transmit.data, cfg->transmit.len);
      
      } else {
        // first, reset the LR-FHSS state machine
        state = resetLRFHSS();
        RADIOLIB_ASSERT(state);

        // skip hopping for the first 4 - lrFhssHdrCount blocks
        for(int i = 0; i < 4 - this->lrFhssHdrCount; ++i ) {
          stepLRFHSS();
        }

        // in LR-FHSS mode, we need to build the entire packet manually
        uint8_t frame[RADIOLIB_SX126X_MAX_PACKET_LENGTH] = { 0 };
        size_t frameLen = 0;
        this->lrFhssFrameBitsRem = 0;
        this->lrFhssFrameHopsRem = 0;
        this->lrFhssHopNum = 0;
        state = buildLRFHSSPacket(cfg->transmit.data, cfg->transmit.len, frame, &frameLen, &this->lrFhssFrameBitsRem, &this->lrFhssFrameHopsRem);
        RADIOLIB_ASSERT(state);

        // FIXME check max len for FHSS
        state = writeBuffer(frame, frameLen);
        RADIOLIB_ASSERT(state);

        // activate hopping
        const uint8_t hopCfg[] = { RADIOLIB_SX126X_HOPPING_ENABLED, (uint8_t)frameLen, (uint8_t)this->lrFhssFrameHopsRem };
        state = writeRegister(RADIOLIB_SX126X_REG_HOPPING_ENABLE, hopCfg, 3);
        RADIOLIB_ASSERT(state);

        // write the initial hopping table
        uint8_t initHops = this->lrFhssFrameHopsRem;
        if(initHops > 16) {
          initHops = 16;
        };
        for(size_t i = 0; i < initHops; i++) {
          // set the hop frequency and symbols
          state = this->setLRFHSSHop(i);
          RADIOLIB_ASSERT(state);
        }
      
      }
      RADIOLIB_ASSERT(state);

      // clear interrupt flags
      state = clearIrqStatus();
      RADIOLIB_ASSERT(state);

      // fix sensitivity
      state = fixSensitivity();
      RADIOLIB_ASSERT(state);
    } break;
    
    default:
      return(RADIOLIB_ERR_UNSUPPORTED);
  }

  this->stagedMode = mode;
  return(state);
}

int16_t SX126x::launchMode() {
  int16_t state;
  switch(this->stagedMode) {
    case(RADIOLIB_RADIO_MODE_RX): {
      this->mod->setRfSwitchState(Module::MODE_RX);
      state = setRx(this->rxTimeout);
    } break;
  
    case(RADIOLIB_RADIO_MODE_TX): {
      this->mod->setRfSwitchState(this->txMode);
      state = setTx(RADIOLIB_SX126X_TX_TIMEOUT_NONE);
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

#if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
void SX126x::setDirectAction(void (*func)(void)) {
  setDio1Action(func);
}

void SX126x::readBit(uint32_t pin) {
  updateDirectBuffer((uint8_t)this->mod->hal->digitalRead(pin));
}
#endif

int16_t SX126x::uploadPatch(const uint32_t* patch, size_t len, bool nonvolatile) {
  // set to standby RC mode
  int16_t state = standby(RADIOLIB_SX126X_STANDBY_RC);
  RADIOLIB_ASSERT(state);

  // check the version
  #if RADIOLIB_DEBUG_BASIC
  char ver_pre[16];
  this->mod->SPIreadRegisterBurst(RADIOLIB_SX126X_REG_VERSION_STRING, 16, reinterpret_cast<uint8_t*>(ver_pre));
  RADIOLIB_DEBUG_BASIC_PRINTLN("Pre-update version string: %s", ver_pre);
  #endif

  // enable patch update
  this->mod->SPIwriteRegister(RADIOLIB_SX126X_REG_PATCH_UPDATE_ENABLE, RADIOLIB_SX126X_PATCH_UPDATE_ENABLED);
  
  // upload the patch
  uint8_t data[4];
  for(uint32_t i = 0; i < len / sizeof(uint32_t); i++) {
    uint32_t bin = 0;
    if(nonvolatile) {
      uint32_t* ptr = const_cast<uint32_t*>(patch) + i;
      bin = RADIOLIB_NONVOLATILE_READ_DWORD(ptr);
    } else {
      bin = patch[i];
    }
    data[0] = (bin >> 24) & 0xFF;
    data[1] = (bin >> 16) & 0xFF;
    data[2] = (bin >> 8) & 0xFF;
    data[3] = bin & 0xFF;
    this->mod->SPIwriteRegisterBurst(RADIOLIB_SX126X_REG_PATCH_MEMORY_BASE + i*sizeof(uint32_t), data, sizeof(uint32_t));
  }

  // disable patch update
  this->mod->SPIwriteRegister(RADIOLIB_SX126X_REG_PATCH_UPDATE_ENABLE, RADIOLIB_SX126X_PATCH_UPDATE_DISABLED);

  // update
  this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_PRAM_UPDATE, NULL, 0);

  // check the version again
  #if RADIOLIB_DEBUG_BASIC
  char ver_post[16];
  this->mod->SPIreadRegisterBurst(RADIOLIB_SX126X_REG_VERSION_STRING, 16, reinterpret_cast<uint8_t*>(ver_post));
  RADIOLIB_DEBUG_BASIC_PRINTLN("Post-update version string: %s", ver_post);
  #endif

  return(state);
}

int16_t SX126x::spectralScanStart(uint16_t numSamples, uint8_t window, uint8_t interval) {
  // abort first - not sure if this is strictly needed, but the example code does this
  spectralScanAbort();

  // set the RSSI window size
  this->mod->SPIwriteRegister(RADIOLIB_SX126X_REG_RSSI_AVG_WINDOW, window);

  // start Rx with infinite timeout
  int16_t state = setRx(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  RADIOLIB_ASSERT(state);

  // now set the actual spectral scan parameters
  const uint8_t data[3] = { (uint8_t)((numSamples >> 8) & 0xFF), (uint8_t)(numSamples & 0xFF), interval };
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_SPECTR_SCAN_PARAMS, data, 3));
}

void SX126x::spectralScanAbort() {
  this->mod->SPIwriteRegister(RADIOLIB_SX126X_REG_RSSI_AVG_WINDOW, 0x00);
}

int16_t SX126x::spectralScanGetStatus() {
  uint8_t status = this->mod->SPIreadRegister(RADIOLIB_SX126X_REG_SPECTRAL_SCAN_STATUS);
  if(status == RADIOLIB_SX126X_SPECTRAL_SCAN_COMPLETED) {
    return(RADIOLIB_ERR_NONE);
  }
  return(RADIOLIB_ERR_RANGING_TIMEOUT);
}

int16_t SX126x::spectralScanGetResult(uint16_t* results) {
  // read the raw results
  uint8_t data[2*RADIOLIB_SX126X_SPECTRAL_SCAN_RES_SIZE];
  this->mod->SPIreadRegisterBurst(RADIOLIB_SX126X_REG_SPECTRAL_SCAN_RESULT, 2*RADIOLIB_SX126X_SPECTRAL_SCAN_RES_SIZE, data);

  // convert it
  for(uint8_t i = 0; i < RADIOLIB_SX126X_SPECTRAL_SCAN_RES_SIZE; i++) {
    results[i] = ((uint16_t)data[i*2] << 8) | ((uint16_t)data[i*2 + 1]);
  }
  return(RADIOLIB_ERR_NONE);
}

int16_t SX126x::calibrateImage(float freq) {
  uint8_t data[2] = { 0, 0 };

  // try to match the frequency ranges
  int freqBand = (int)freq;
  if((freqBand >= 902) && (freqBand <= 928)) {
    data[0] = RADIOLIB_SX126X_CAL_IMG_902_MHZ_1;
    data[1] = RADIOLIB_SX126X_CAL_IMG_902_MHZ_2;
  } else if((freqBand >= 863) && (freqBand <= 870)) {
    data[0] = RADIOLIB_SX126X_CAL_IMG_863_MHZ_1;
    data[1] = RADIOLIB_SX126X_CAL_IMG_863_MHZ_2;
  } else if((freqBand >= 779) && (freqBand <= 787)) {
    data[0] = RADIOLIB_SX126X_CAL_IMG_779_MHZ_1;
    data[1] = RADIOLIB_SX126X_CAL_IMG_779_MHZ_2;
  } else if((freqBand >= 470) && (freqBand <= 510)) {
    data[0] = RADIOLIB_SX126X_CAL_IMG_470_MHZ_1;
    data[1] = RADIOLIB_SX126X_CAL_IMG_470_MHZ_2;
  } else if((freqBand >= 430) && (freqBand <= 440)) {
    data[0] = RADIOLIB_SX126X_CAL_IMG_430_MHZ_1;
    data[1] = RADIOLIB_SX126X_CAL_IMG_430_MHZ_2;
  }

  int16_t state;
  if(data[0]) {
    // matched with predefined ranges, do the calibration
    state = SX126x::calibrateImage(data);
  
  } else {
    // if nothing matched, try custom calibration - the may or may not work
    RADIOLIB_DEBUG_BASIC_PRINTLN("Failed to match predefined frequency range, trying custom");
    state = SX126x::calibrateImageRejection(freq - 4.0f, freq + 4.0f);
  
  }
  
  return(state);
}

int16_t SX126x::calibrateImageRejection(float freqMin, float freqMax) {
  // calculate the calibration coefficients and calibrate image
  uint8_t data[] = { (uint8_t)floor((freqMin - 1.0f) / 4.0f), (uint8_t)ceil((freqMax + 1.0f) / 4.0f) };
  data[0] = (data[0] % 2) ? data[0] : data[0] - 1;
  data[1] = (data[1] % 2) ? data[1] : data[1] + 1;
  return(this->calibrateImage(data));
}

int16_t SX126x::fixSensitivity() {
  // fix receiver sensitivity for 500 kHz LoRa
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.1 for details

  // read current sensitivity configuration
  uint8_t sensitivityConfig = 0;
  int16_t state = readRegister(RADIOLIB_SX126X_REG_SENSITIVITY_CONFIG, &sensitivityConfig, 1);
  RADIOLIB_ASSERT(state);

  // fix the value for LoRa with 500 kHz bandwidth
  if((getPacketType() == RADIOLIB_SX126X_PACKET_TYPE_LORA) && (fabsf(this->bandwidthKhz - 500.0f) <= 0.001f)) {
    sensitivityConfig &= 0xFB;
  } else {
    sensitivityConfig |= 0x04;
  }
  return(writeRegister(RADIOLIB_SX126X_REG_SENSITIVITY_CONFIG, &sensitivityConfig, 1));
}

int16_t SX126x::fixPaClamping(bool enable) {
  // fixes overly eager PA clamping
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.2 for details

  // read current clamping configuration
  uint8_t clampConfig = 0;
  int16_t state = readRegister(RADIOLIB_SX126X_REG_TX_CLAMP_CONFIG, &clampConfig, 1);
  RADIOLIB_ASSERT(state);

  // apply or undo workaround
  if (enable)
    clampConfig |= 0x1E;
  else
    clampConfig = (clampConfig & ~0x1E) | 0x08;

  return(writeRegister(RADIOLIB_SX126X_REG_TX_CLAMP_CONFIG, &clampConfig, 1));
}

int16_t SX126x::fixImplicitTimeout() {
  // fixes timeout in implicit header mode
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.3 for details

  //check if we're in implicit LoRa mode
  if(!((this->headerType == RADIOLIB_SX126X_LORA_HEADER_IMPLICIT) && (getPacketType() == RADIOLIB_SX126X_PACKET_TYPE_LORA))) {
    // not in the correct mode, nothing to do here
    return(RADIOLIB_ERR_NONE);
  }

  // stop RTC counter
  uint8_t rtcStop = 0x00;
  int16_t state = writeRegister(RADIOLIB_SX126X_REG_RTC_CTRL, &rtcStop, 1);
  RADIOLIB_ASSERT(state);

  // read currently active event
  uint8_t rtcEvent = 0;
  state = readRegister(RADIOLIB_SX126X_REG_EVENT_MASK, &rtcEvent, 1);
  RADIOLIB_ASSERT(state);

  // clear events
  rtcEvent |= 0x02;
  return(writeRegister(RADIOLIB_SX126X_REG_EVENT_MASK, &rtcEvent, 1));
}

int16_t SX126x::fixInvertedIQ(uint8_t iqConfig) {
  // fixes IQ configuration for inverted IQ
  // see SX1262/SX1268 datasheet, chapter 15 Known Limitations, section 15.4 for details

  // read current IQ configuration
  uint8_t iqConfigCurrent = 0;
  int16_t state = readRegister(RADIOLIB_SX126X_REG_IQ_CONFIG, &iqConfigCurrent, 1);
  RADIOLIB_ASSERT(state);

  // set correct IQ configuration
  if(iqConfig == RADIOLIB_SX126X_LORA_IQ_INVERTED) {
    iqConfigCurrent &= 0xFB;
  } else {
    iqConfigCurrent |= 0x04;
  }

  // update with the new value
  return(writeRegister(RADIOLIB_SX126X_REG_IQ_CONFIG, &iqConfigCurrent, 1));
}

int16_t SX126x::fixGFSK() {
  // method that applies some magic workaround for specific bitrate, frequency deviation,
  // receiver bandwidth and carrier frequencies for GFSK (and resets it in all other cases)
  // this is not documented in the datasheet, only in Semtech repositories for SX126x and LR11xx

  // first, check we are using GFSK modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    // not in GFSK, nothing to do here
    return(RADIOLIB_ERR_NONE);
  }

  // next, decide what to change based on modulation properties
  int16_t state = RADIOLIB_ERR_UNKNOWN;
  if(this->bitRate == 1200) {
    // workaround for 1.2 kbps
    state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_GFSK_FIX_3, 0x00, 4, 4);

  } else if(this->bitRate == 600)  {
    // workaround for 0.6 kbps
    state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_GFSK_FIX_1, 0x18, 4, 3);
    RADIOLIB_ASSERT(state);
    state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_RSSI_AVG_WINDOW, 0x04, 4, 2);
    RADIOLIB_ASSERT(state);
    state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_GFSK_FIX_3, 0x00, 4, 4);
    RADIOLIB_ASSERT(state);
    state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_GFSK_FIX_4, 0x50, 6, 4);
  
  } else {
    // reset
    state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_GFSK_FIX_1, 0x08, 4, 3);
    RADIOLIB_ASSERT(state);
    state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_RSSI_AVG_WINDOW, 0x00, 4, 2);
    RADIOLIB_ASSERT(state);
    state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_GFSK_FIX_3, 0x10, 4, 4);
    RADIOLIB_ASSERT(state);
    state = this->mod->SPIsetRegValue(RADIOLIB_SX126X_REG_GFSK_FIX_4, 0x00, 6, 4);
  
  }

  return(state);
}

Module* SX126x::getMod() {
  return(this->mod);
}

int16_t SX126x::modSetup(float tcxoVoltage, bool useRegulatorLDO, uint8_t modem) {
  // set module properties
  this->mod->init();
  this->mod->hal->pinMode(this->mod->getIrq(), this->mod->hal->GpioModeInput);
  this->mod->hal->pinMode(this->mod->getGpio(), this->mod->hal->GpioModeInput);
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR] = Module::BITS_16;
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_8;
  this->mod->spiConfig.statusPos = 1;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_SX126X_CMD_READ_REGISTER;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_SX126X_CMD_WRITE_REGISTER;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP] = RADIOLIB_SX126X_CMD_NOP;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_STATUS] = RADIOLIB_SX126X_CMD_GET_STATUS;
  this->mod->spiConfig.stream = true;
  this->mod->spiConfig.parseStatusCb = SPIparseStatus;

  // find the SX126x chip - this will also reset the module and verify the module
  if(!SX126x::findChip(this->chipType)) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("No SX126x found!");
    this->mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  RADIOLIB_DEBUG_BASIC_PRINTLN("M\tSX126x");

  int16_t state = RADIOLIB_ERR_NONE;

  // set TCXO control, if requested
  if(!this->XTAL && tcxoVoltage > 0.0f) {
    state = setTCXO(tcxoVoltage);
    RADIOLIB_ASSERT(state);
  }

  // configure settings not accessible by API
  state = config(modem);

  // if something failed, check the device errors
  if(state != RADIOLIB_ERR_NONE) {
    // unless mode is forced to standby, device errors will be 0
    (void)standby();
    uint16_t errors = getDeviceErrors();
    RADIOLIB_DEBUG_BASIC_PRINTLN("Config failed, device errors: 0x%X", errors);

    // SPI command fail and oscillator start error flag indicate incorrectly set oscillator
    if((state == RADIOLIB_ERR_SPI_CMD_FAILED) && (errors & RADIOLIB_SX126X_XOSC_START_ERR)) {
      // typically users with XTAL devices will try to call the default begin method
      // disable TCXO and try to run config again
      this->XTAL = false;
      RADIOLIB_DEBUG_BASIC_PRINTLN("Bad oscillator selected, trying XTAL");

      state = setTCXO(0);
      RADIOLIB_ASSERT(state);

      state = config(modem);
    }
  }
  RADIOLIB_ASSERT(state);

  if (useRegulatorLDO) {
    state = setRegulatorLDO();
  } else {
    state = setRegulatorDCDC();
  }
  return(state);
}

int16_t SX126x::SPIparseStatus(uint8_t in) {
  if((in & 0b00001110) == RADIOLIB_SX126X_STATUS_CMD_TIMEOUT) {
    return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
  } else if((in & 0b00001110) == RADIOLIB_SX126X_STATUS_CMD_INVALID) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  } else if((in & 0b00001110) == RADIOLIB_SX126X_STATUS_CMD_FAILED) {
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  } else if((in == 0x00) || (in == 0xFF)) {
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  return(RADIOLIB_ERR_NONE);
}

bool SX126x::findChip(const char* verStr) {
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    // reset the module
    reset(true);

    // read the version string
    char version[16] = { 0 };
    this->mod->SPIreadRegisterBurst(RADIOLIB_SX126X_REG_VERSION_STRING, 16, reinterpret_cast<uint8_t*>(version));

    // check version register
    if(strncmp(verStr, version, 6) == 0) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("Found SX126x: RADIOLIB_SX126X_REG_VERSION_STRING:");
      RADIOLIB_DEBUG_BASIC_HEXDUMP(reinterpret_cast<uint8_t*>(version), 16, RADIOLIB_SX126X_REG_VERSION_STRING);
      RADIOLIB_DEBUG_BASIC_PRINTLN();
      flagFound = true;
    } else {
      #if RADIOLIB_DEBUG_BASIC
        RADIOLIB_DEBUG_BASIC_PRINTLN("SX126x not found! (%d of 10 tries) RADIOLIB_SX126X_REG_VERSION_STRING:", i + 1);
        RADIOLIB_DEBUG_BASIC_HEXDUMP(reinterpret_cast<uint8_t*>(version), 16, RADIOLIB_SX126X_REG_VERSION_STRING);
        RADIOLIB_DEBUG_BASIC_PRINTLN("Expected string: %s", verStr);
      #endif
      this->mod->hal->delay(10);
      i++;
    }
  }

  return(flagFound);
}

#endif
