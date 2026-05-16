#include "SX128x.h"
#include <math.h>
#include <string.h>
#if !RADIOLIB_EXCLUDE_SX128X

SX128x::SX128x(Module* mod) : PhysicalLayer() {
  this->freqStep = RADIOLIB_SX128X_FREQUENCY_STEP_SIZE;
  this->maxPacketLength = RADIOLIB_SX128X_MAX_PACKET_LENGTH;
  this->mod = mod;
  this->irqMap[RADIOLIB_IRQ_TX_DONE] = RADIOLIB_SX128X_IRQ_TX_DONE;
  this->irqMap[RADIOLIB_IRQ_RX_DONE] = RADIOLIB_SX128X_IRQ_RX_DONE;
  this->irqMap[RADIOLIB_IRQ_PREAMBLE_DETECTED] = RADIOLIB_SX128X_IRQ_PREAMBLE_DETECTED;
  this->irqMap[RADIOLIB_IRQ_SYNC_WORD_VALID] = RADIOLIB_SX128X_IRQ_SYNC_WORD_VALID;
  this->irqMap[RADIOLIB_IRQ_HEADER_VALID] = RADIOLIB_SX128X_IRQ_HEADER_VALID;
  this->irqMap[RADIOLIB_IRQ_HEADER_ERR] = RADIOLIB_SX128X_IRQ_HEADER_ERROR;
  this->irqMap[RADIOLIB_IRQ_CRC_ERR] = RADIOLIB_SX128X_IRQ_CRC_ERROR;
  this->irqMap[RADIOLIB_IRQ_CAD_DONE] = RADIOLIB_SX128X_IRQ_CAD_DONE;
  this->irqMap[RADIOLIB_IRQ_CAD_DETECTED] = RADIOLIB_SX128X_IRQ_CAD_DETECTED;
  this->irqMap[RADIOLIB_IRQ_TIMEOUT] = RADIOLIB_SX128X_IRQ_RX_TX_TIMEOUT;
}

int16_t SX128x::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t pwr, uint16_t preambleLength) {
  // initialize LoRa modulation variables
  this->bandwidthKhz = bw;
  this->spreadingFactor = RADIOLIB_SX128X_LORA_SF_9;
  this->codingRateLoRa = RADIOLIB_SX128X_LORA_CR_4_7;

  // initialize LoRa packet variables
  this->preambleLengthLoRa = preambleLength;
  this->headerType = RADIOLIB_SX128X_LORA_HEADER_EXPLICIT;
  this->payloadLen = 0xFF;
  this->crcLoRa = RADIOLIB_SX128X_LORA_CRC_ON;

  // set module properties and perform initial setup
  int16_t state = this->modSetup(RADIOLIB_SX128X_PACKET_TYPE_LORA);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBandwidth(bw);
  RADIOLIB_ASSERT(state);

  state = setSpreadingFactor(sf);
  RADIOLIB_ASSERT(state);

  state = setCodingRate(cr);
  RADIOLIB_ASSERT(state);

  state = setSyncWord(syncWord);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(pwr);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX128x::beginGFSK(float freq, uint16_t br, float freqDev, int8_t pwr, uint16_t preambleLength) {
  // initialize GFSK modulation variables
  this->bitRateKbps = br;
  this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_0_800_BW_2_4;
  this->modIndexReal = 1.0;
  this->modIndex = RADIOLIB_SX128X_BLE_GFSK_MOD_IND_1_00;
  this->shaping = RADIOLIB_SX128X_BLE_GFSK_BT_0_5;

  // initialize GFSK packet variables
  this->preambleLengthGFSK = preambleLength;
  this->syncWordLen = 2;
  this->syncWordMatch = RADIOLIB_SX128X_GFSK_FLRC_SYNC_WORD_1;
  this->crcGFSK = RADIOLIB_SX128X_GFSK_FLRC_CRC_2_BYTE;
  this->whitening = RADIOLIB_SX128X_GFSK_BLE_WHITENING_ON;

  // set module properties and perform initial setup
  int16_t state = this->modSetup(RADIOLIB_SX128X_PACKET_TYPE_GFSK);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(pwr);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(RADIOLIB_SHAPING_0_5);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  uint8_t sync[] = { 0x12, 0xAD };
  state = setSyncWord(sync, 2);
  RADIOLIB_ASSERT(state);

  state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX128x::beginBLE(float freq, uint16_t br, float freqDev, int8_t pwr, uint8_t dataShaping) {
  // initialize BLE modulation variables
  this->bitRateKbps = br;
  this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_0_800_BW_2_4;
  this->modIndexReal = 1.0;
  this->modIndex = RADIOLIB_SX128X_BLE_GFSK_MOD_IND_1_00;
  this->shaping = RADIOLIB_SX128X_BLE_GFSK_BT_0_5;

  // initialize BLE packet variables
  this->crcGFSK = RADIOLIB_SX128X_BLE_CRC_3_BYTE;
  this->whitening = RADIOLIB_SX128X_GFSK_BLE_WHITENING_ON;

  // set module properties and perform initial setup
  int16_t state = this->modSetup(RADIOLIB_SX128X_PACKET_TYPE_BLE);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(pwr);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(dataShaping);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX128x::beginFLRC(float freq, uint16_t br, uint8_t cr, int8_t pwr, uint16_t preambleLength, uint8_t dataShaping) {
  // initialize FLRC modulation variables
  this->bitRateKbps = br;
  this->bitRate = RADIOLIB_SX128X_FLRC_BR_0_650_BW_0_6;
  this->codingRateFLRC = RADIOLIB_SX128X_FLRC_CR_3_4;
  this->shaping = RADIOLIB_SX128X_FLRC_BT_0_5;

  // initialize FLRC packet variables
  this->preambleLengthGFSK = preambleLength;
  this->syncWordLen = 2;
  this->syncWordMatch = RADIOLIB_SX128X_GFSK_FLRC_SYNC_WORD_1;
  this->crcGFSK = RADIOLIB_SX128X_GFSK_FLRC_CRC_2_BYTE;
  this->whitening = RADIOLIB_SX128X_GFSK_BLE_WHITENING_OFF;

  // set module properties and perform initial setup
  int16_t state = this->modSetup(RADIOLIB_SX128X_PACKET_TYPE_FLRC);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  state = setCodingRate(cr);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(pwr);
  RADIOLIB_ASSERT(state);

  state = setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  state = setDataShaping(dataShaping);
  RADIOLIB_ASSERT(state);

  // set publicly accessible settings that are not a part of begin method
  uint8_t sync[] = { 0x2D, 0x01, 0x4B, 0x1D};
  state = setSyncWord(sync, 4);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX128x::reset(bool verify) {
  // run the reset sequence - same as SX126x, as SX128x docs don't seem to mention this
  this->mod->hal->pinMode(this->mod->getRst(), this->mod->hal->GpioModeOutput);
  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelLow);
  this->mod->hal->delay(1);
  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelHigh);

  // return immediately when verification is disabled
  if(!verify) {
    return(RADIOLIB_ERR_NONE);
  }

  // set mode to standby
  RadioLibTime_t start = this->mod->hal->millis();
  while(true) {
    // try to set mode to standby
    int16_t state = standby();
    if(state == RADIOLIB_ERR_NONE) {
      // standby command successful
      return(RADIOLIB_ERR_NONE);
    }

    // standby command failed, check timeout and try again
    if(this->mod->hal->millis() - start >= 3000) {
      // timed out, possibly incorrect wiring
      return(state);
    }

    // wait a bit to not spam the module
    this->mod->hal->delay(10);
  }
}

int16_t SX128x::transmit(const uint8_t* data, size_t len, uint8_t addr) {
  // check packet length
  if(this->codingRateLoRa == RADIOLIB_SX128X_LORA_CR_4_8_LI && this->crcLoRa == RADIOLIB_SX128X_LORA_CRC_ON) {
    // Long Interleaver at CR 4/8 supports up to 253 bytes if CRC is enabled
    if(len > RADIOLIB_SX128X_MAX_PACKET_LENGTH - 2) {
      return(RADIOLIB_ERR_PACKET_TOO_LONG);
    }
  } else if(len > RADIOLIB_SX128X_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // check active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // calculate timeout in ms (5ms + 500 % of expected time-on-air)
  RadioLibTime_t timeout = 5 + (getTimeOnAir(len) * 5) / 1000;
  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu ms", timeout);

  // start transmission
  state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  // wait for packet transmission or timeout
  RadioLibTime_t start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    if(this->mod->hal->millis() - start > timeout) {
      finishTransmit();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }

  return(finishTransmit());
}

int16_t SX128x::receive(uint8_t* data, size_t len, RadioLibTime_t timeout) {
  // check active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // calculate timeout (1000% of expected time-on-air)
  // for most other modules, it is 500%, however, the overall datarates of SX128x are higher
  // so we use higher value for the default timeout
  RadioLibTime_t timeoutInternal = timeout;
  if(!timeoutInternal) {
    timeoutInternal = getTimeOnAir(len) * 10;
  }
  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu ms", (uint32_t)((timeout + 999) / 1000));

  // start reception
  uint32_t timeoutValue = (uint32_t)((float)timeoutInternal / 15.625f);
  state = startReceive(timeoutValue);
  RADIOLIB_ASSERT(state);

  // wait for packet reception or timeout
  bool softTimeout = false;
  RadioLibTime_t start = this->mod->hal->micros();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    // safety check, the timeout should be done by the radio
    if(this->mod->hal->micros() - start > timeout) {
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
  if(softTimeout || (getIrqStatus() & this->irqMap[RADIOLIB_IRQ_TIMEOUT])) {
    (void)finishReceive();
    return(RADIOLIB_ERR_RX_TIMEOUT);
  }

  // read the received data
  return(readData(data, len));
}

int16_t SX128x::transmitDirect(uint32_t frf) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_TX);

  // user requested to start transmitting immediately (required for RTTY)
  int16_t state = RADIOLIB_ERR_NONE;
  if(frf != 0) {
    state = setRfFrequency(frf);
  }
  RADIOLIB_ASSERT(state);

  // start transmitting
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_TX_CONTINUOUS_WAVE, NULL, 0));
}

int16_t SX128x::receiveDirect() {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // SX128x is unable to output received data directly
  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX128x::scanChannel() {
  ChannelScanConfig_t cfg = {
    .cad = {
      .symNum = RADIOLIB_SX128X_CAD_PARAM_DEFAULT,
      .detPeak = 0,
      .detMin = 0,
      .exitMode = 0,
      .timeout = 0,
      .irqFlags = RADIOLIB_IRQ_CAD_DEFAULT_FLAGS,
      .irqMask = RADIOLIB_IRQ_CAD_DEFAULT_MASK,
    },
  };
  return(this->scanChannel(cfg));
}

int16_t SX128x::scanChannel(const ChannelScanConfig_t &config) {
  // set mode to CAD
  int16_t state = startChannelScan(config);
  RADIOLIB_ASSERT(state);

  // wait for channel activity detected or timeout
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
  }

  // check CAD result
  return(getChannelScanResult());
}

int16_t SX128x::sleep() {
  return(SX128x::sleep(true));
}

int16_t SX128x::sleep(bool retainConfig) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  uint8_t sleepConfig = RADIOLIB_SX128X_SLEEP_DATA_BUFFER_RETAIN | RADIOLIB_SX128X_SLEEP_DATA_RAM_RETAIN;
  if(!retainConfig) {
    sleepConfig = RADIOLIB_SX128X_SLEEP_DATA_BUFFER_FLUSH | RADIOLIB_SX128X_SLEEP_DATA_RAM_FLUSH;
  }
  int16_t state = this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SAVE_CONTEXT, NULL, 0, false, false);
  RADIOLIB_ASSERT(state);
  state = this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_SLEEP, &sleepConfig, 1, false, false);

  // wait for SX128x to safely enter sleep mode
  this->mod->hal->delay(1);

  return(state);
}

int16_t SX128x::standby() {
  return(SX128x::standby(RADIOLIB_SX128X_STANDBY_RC));
}

int16_t SX128x::standby(uint8_t mode) {
  return(SX128x::standby(mode, true));
}

int16_t SX128x::standby(uint8_t mode, bool wakeup) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  if(wakeup) {
    // send a NOP command - this pulls the NSS low to exit the sleep mode,
    // while preventing interference with possible other SPI transactions
    (void)this->mod->SPIwriteStream((uint16_t)RADIOLIB_SX128X_CMD_NOP, NULL, 0, false, false);
  }

  const uint8_t data[] = { mode };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_STANDBY, data, 1));
}

void SX128x::setDio1Action(void (*func)(void)) {
  this->mod->hal->attachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()), func, this->mod->hal->GpioInterruptRising);
}

void SX128x::clearDio1Action() {
  this->mod->hal->detachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()));
}

void SX128x::setPacketReceivedAction(void (*func)(void)) {
  this->setDio1Action(func);
}

void SX128x::clearPacketReceivedAction() {
  this->clearDio1Action();
}

void SX128x::setPacketSentAction(void (*func)(void)) {
  this->setDio1Action(func);
}

void SX128x::clearPacketSentAction() {
  this->clearDio1Action();
}

int16_t SX128x::finishTransmit() {
  // clear interrupt flags
  clearIrqStatus();

  // set mode to standby to disable transmitter/RF switch
  return(standby());
}

int16_t SX128x::startReceive() {
  return(this->startReceive(RADIOLIB_SX128X_RX_TIMEOUT_INF, RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RADIOLIB_IRQ_RX_DEFAULT_MASK, 0));
}

int16_t SX128x::readData(uint8_t* data, size_t len) {
  // check active modem
  if(getPacketType() == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check integrity CRC
  uint16_t irq = getIrqStatus();
  int16_t crcState = RADIOLIB_ERR_NONE;
  // Report CRC mismatch when there's a payload CRC error, or a header error and no valid header (to avoid false alarm from previous packet)
  if((irq & RADIOLIB_SX128X_IRQ_CRC_ERROR) || ((irq & RADIOLIB_SX128X_IRQ_HEADER_ERROR) && !(irq & RADIOLIB_SX128X_IRQ_HEADER_VALID))) {
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

int16_t SX128x::finishReceive() {
  // set mode to standby to disable RF switch
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  return(clearIrqStatus());
}

uint32_t SX128x::getIrqFlags() {
  return((uint32_t)this->getIrqStatus());
}

int16_t SX128x::setIrqFlags(uint32_t irq) {
  return(this->setDioIrqParams(irq, irq));
}

int16_t SX128x::clearIrqFlags(uint32_t irq) {
  return(this->clearIrqStatus(irq));
}

int16_t SX128x::startChannelScan() {
  ChannelScanConfig_t cfg = {
    .cad = {
      .symNum = RADIOLIB_SX128X_CAD_PARAM_DEFAULT,
      .detPeak = 0,
      .detMin = 0,
      .exitMode = 0,
      .timeout = 0,
      .irqFlags = RADIOLIB_IRQ_CAD_DEFAULT_FLAGS,
      .irqMask = RADIOLIB_IRQ_CAD_DEFAULT_MASK,
    },
  };
  return(this->startChannelScan(cfg));
}

int16_t SX128x::startChannelScan(const ChannelScanConfig_t &config) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set DIO pin mapping
  state = setDioIrqParams(getIrqMapped(config.cad.irqFlags), getIrqMapped(config.cad.irqMask));
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  state = clearIrqStatus();
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // set mode to CAD
  return(setCad(config.cad.symNum));
}

int16_t SX128x::getChannelScanResult() {
  // check active modem
  if(getPacketType() != RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check CAD result
  uint16_t cadResult = getIrqStatus();
  int16_t state = RADIOLIB_ERR_UNKNOWN;
  if(cadResult & RADIOLIB_SX128X_IRQ_CAD_DETECTED) {
    // detected some LoRa activity
    state = RADIOLIB_LORA_DETECTED;
  } else if(cadResult & RADIOLIB_SX128X_IRQ_CAD_DONE) {
    // channel is free
    state = RADIOLIB_CHANNEL_FREE;
  }

  clearIrqStatus();
  return(state);
}

int16_t SX128x::setFrequency(float freq) {
  RADIOLIB_CHECK_RANGE(freq, 2400.0f, 2500.0f, RADIOLIB_ERR_INVALID_FREQUENCY);

  // calculate raw value
  uint32_t frf = (freq * (uint32_t(1) << RADIOLIB_SX128X_DIV_EXPONENT)) / RADIOLIB_SX128X_CRYSTAL_FREQ;
  return(setRfFrequency(frf));
}

int16_t SX128x::setBandwidth(float bw) {
  // check active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    // check range for LoRa
    RADIOLIB_CHECK_RANGE(bw, 203.125f, 1625.0f, RADIOLIB_ERR_INVALID_BANDWIDTH);
  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    // check range for ranging
    RADIOLIB_CHECK_RANGE(bw, 406.25f, 1625.0f, RADIOLIB_ERR_INVALID_BANDWIDTH);
  } else {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(fabsf(bw - 203.125f) <= 0.001f) {
    this->bandwidth = RADIOLIB_SX128X_LORA_BW_203_125;
  } else if(fabsf(bw - 406.25f) <= 0.001f) {
    this->bandwidth = RADIOLIB_SX128X_LORA_BW_406_25;
  } else if(fabsf(bw - 812.5f) <= 0.001f) {
    this->bandwidth = RADIOLIB_SX128X_LORA_BW_812_50;
  } else if(fabsf(bw - 1625.0f) <= 0.001f) {
    this->bandwidth = RADIOLIB_SX128X_LORA_BW_1625_00;
  } else {
    return(RADIOLIB_ERR_INVALID_BANDWIDTH);
  }

  // update modulation parameters
  this->bandwidthKhz = bw;
  return(setModulationParams(this->spreadingFactor, this->bandwidth, this->codingRateLoRa));
}

int16_t SX128x::setSpreadingFactor(uint8_t sf) {
  // check active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    // check range for LoRa
    RADIOLIB_CHECK_RANGE(sf, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
    // check range for ranging
    RADIOLIB_CHECK_RANGE(sf, 5, 10, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
  } else {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update modulation parameters
  this->spreadingFactor = sf << 4;
  int16_t state = setModulationParams(this->spreadingFactor, this->bandwidth, this->codingRateLoRa);
  RADIOLIB_ASSERT(state);

  // update mystery register in LoRa mode - SX1280 datasheet rev 3.2 section 14.4.1
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    uint8_t data = 0;
    if((this->spreadingFactor == RADIOLIB_SX128X_LORA_SF_5) || (this->spreadingFactor == RADIOLIB_SX128X_LORA_SF_6)) {
      data = 0x1E;
    } else if((this->spreadingFactor == RADIOLIB_SX128X_LORA_SF_7) || (this->spreadingFactor == RADIOLIB_SX128X_LORA_SF_8)) {
      data = 0x37;
    } else {
      data = 0x32;
    }
    state = SX128x::writeRegister(RADIOLIB_SX128X_REG_LORA_SF_CONFIG, &data, 1);
    RADIOLIB_ASSERT(state);

    // this register must also be updated for some reason
    state = SX128x::readRegister(RADIOLIB_SX128X_REG_FREQ_ERROR_CORRECTION, &data, 1);
    RADIOLIB_ASSERT(state);

    data |= 0x01;
    state = SX128x::writeRegister(RADIOLIB_SX128X_REG_FREQ_ERROR_CORRECTION, &data, 1);
    RADIOLIB_ASSERT(state);
  }

  return(state);
}

int16_t SX128x::setCodingRate(uint8_t cr, bool longInterleaving) {
  // check active modem
  uint8_t modem = getPacketType();

  // LoRa/ranging
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING)) {
    RADIOLIB_CHECK_RANGE(cr, 4, 8, RADIOLIB_ERR_INVALID_CODING_RATE);

    // update modulation parameters
    if(longInterleaving && (modem == RADIOLIB_SX128X_PACKET_TYPE_LORA)) {
      switch(cr) {
        case 4:
          this->codingRateLoRa = 0;
          break;
        case 5:
        case 6:
          this->codingRateLoRa = cr;
          break;
        case 8: 
          this->codingRateLoRa = cr - 1;
          break;
        default:
          return(RADIOLIB_ERR_INVALID_CODING_RATE);
      }
    } else {
      this->codingRateLoRa = cr - 4;
    }
    return(setModulationParams(this->spreadingFactor, this->bandwidth, this->codingRateLoRa));

  // FLRC
  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC) {
    RADIOLIB_CHECK_RANGE(cr, 2, 4, RADIOLIB_ERR_INVALID_CODING_RATE);

    // update modulation parameters
    this->codingRateFLRC = (cr - 2) * 2;
    return(setModulationParams(this->bitRate, this->codingRateFLRC, this->shaping));
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX128x::setOutputPower(int8_t pwr) {
  // check if power value is configurable
  int16_t state = checkOutputPower(pwr, NULL);
  RADIOLIB_ASSERT(state);

  this->power = pwr + 18;
  return(setTxParams(this->power));
}

int16_t SX128x::checkOutputPower(int8_t pwr, int8_t* clipped) {
  if(clipped) {
    *clipped = RADIOLIB_MAX(-18, RADIOLIB_MIN(13, pwr));
  }
  RADIOLIB_CHECK_RANGE(pwr, -18, 13, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  return(RADIOLIB_ERR_NONE);
}

int16_t SX128x::setModem(ModemType_t modem) {
  switch(modem) {
    case(ModemType_t::RADIOLIB_MODEM_LORA): {
      return(this->begin());
    } break;
    case(ModemType_t::RADIOLIB_MODEM_FSK): {
      return(this->beginGFSK());
    } break;
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
}

int16_t SX128x::getModem(ModemType_t* modem) {
  RADIOLIB_ASSERT_PTR(modem);

  switch(getPacketType()) {
    case(RADIOLIB_SX128X_PACKET_TYPE_LORA):
      *modem = ModemType_t::RADIOLIB_MODEM_LORA;
      return(RADIOLIB_ERR_NONE);
    case(RADIOLIB_SX128X_PACKET_TYPE_GFSK):
      *modem = ModemType_t::RADIOLIB_MODEM_FSK;
      return(RADIOLIB_ERR_NONE);
  }
  
  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX128x::setPreambleLength(size_t preambleLength) {
  uint8_t modem = getPacketType();
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING)) {
    // LoRa or ranging
    // the actual limit is 491520, however, some platforms (notably AVR) limit size_t to 16 bits
    RADIOLIB_CHECK_RANGE(preambleLength, 2, 65534, RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);

    // check preamble length is even - no point even trying odd numbers
    if(preambleLength % 2 != 0) {
      return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
    }

    // calculate exponent and mantissa values (use the next longer preamble if there's no exact match)
    uint8_t e = 1;
    uint8_t m = 1;
    uint32_t len = 0;
    for(; e <= 15; e++) {
      for(m = 1; m <= 15; m++) {
        len = m * (uint32_t(1) << e);
        if(len >= preambleLength) {
          break;
        }
      }
      if(len >= preambleLength) {
        break;
      }
    }

    // update packet parameters
    this->preambleLengthLoRa = (e << 4) | m;
    return(setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->payloadLen, this->crcLoRa, this->invertIQEnabled));

  } else if((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC)) {
    // GFSK or FLRC
    RADIOLIB_CHECK_RANGE(preambleLength, 4, 32, RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);

    // check preamble length is multiple of 4
    if(preambleLength % 4 != 0) {
      return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
    }

    // update packet parameters
    this->preambleLengthGFSK = ((preambleLength / 4) - 1) << 4;
    return(setPacketParamsGFSK(this->preambleLengthGFSK, this->syncWordLen, this->syncWordMatch, this->crcGFSK, this->whitening, this->packetType));
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX128x::setDataRate(DataRate_t dr, ModemType_t modem) {
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
  if (modem == RADIOLIB_MODEM_LORA) {
      state = this->setBandwidth(dr.lora.bandwidth);
      RADIOLIB_ASSERT(state);
      state = this->setSpreadingFactor(dr.lora.spreadingFactor);
      RADIOLIB_ASSERT(state);
      state = this->setCodingRate(dr.lora.codingRate);
  } else {
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(state);
}

int16_t SX128x::checkDataRate(DataRate_t dr, ModemType_t modem) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // retrieve modem if not supplied
  if(modem == RADIOLIB_MODEM_NONE) {
    state = this->getModem(&modem);
    RADIOLIB_ASSERT(state);
  }

  // select interpretation based on modem
  if(modem == RADIOLIB_MODEM_FSK) {
    RADIOLIB_CHECK_RANGE(dr.fsk.bitRate, 125.0f, 2000.0f, RADIOLIB_ERR_INVALID_BIT_RATE);
    RADIOLIB_CHECK_RANGE(dr.fsk.freqDev, 62.5f, 1000.0f, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
    return(RADIOLIB_ERR_NONE);

  } else if(modem == RADIOLIB_MODEM_LORA) {
    RADIOLIB_CHECK_RANGE(dr.lora.spreadingFactor, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
    RADIOLIB_CHECK_RANGE(dr.lora.bandwidth, 203.0f, 1625.0f, RADIOLIB_ERR_INVALID_BANDWIDTH);
    RADIOLIB_CHECK_RANGE(dr.lora.codingRate, 4, 8, RADIOLIB_ERR_INVALID_CODING_RATE);
    return(RADIOLIB_ERR_NONE);
  
  }

  return(state);
}

int16_t SX128x::setBitRate(float br) {
  // check active modem
  uint8_t modem = getPacketType();

  // GFSK/BLE
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_BLE)) {
    if((uint16_t)br == 125) {
      this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_0_125_BW_0_3;
    } else if((uint16_t)br == 250) {
      this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_0_250_BW_0_6;
    } else if((uint16_t)br == 400) {
      this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_0_400_BW_1_2;
    } else if((uint16_t)br == 500) {
      this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_0_500_BW_1_2;
    } else if((uint16_t)br == 800) {
      this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_0_800_BW_2_4;
    } else if((uint16_t)br == 1000) {
      this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_1_000_BW_2_4;
    } else if((uint16_t)br == 1600) {
      this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_1_600_BW_2_4;
    } else if((uint16_t)br == 2000) {
      this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_2_000_BW_2_4;
    } else {
      return(RADIOLIB_ERR_INVALID_BIT_RATE);
    }

    // update modulation parameters
    this->bitRateKbps = (uint16_t)br;
    return(setModulationParams(this->bitRate, this->modIndex, this->shaping));

  // FLRC
  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC) {
    if((uint16_t)br == 260) {
      this->bitRate = RADIOLIB_SX128X_FLRC_BR_0_260_BW_0_3;
    } else if((uint16_t)br == 325) {
      this->bitRate = RADIOLIB_SX128X_FLRC_BR_0_325_BW_0_3;
    } else if((uint16_t)br == 520) {
      this->bitRate = RADIOLIB_SX128X_FLRC_BR_0_520_BW_0_6;
    } else if((uint16_t)br == 650) {
      this->bitRate = RADIOLIB_SX128X_FLRC_BR_0_650_BW_0_6;
    } else if((uint16_t)br == 1000) {
      this->bitRate = RADIOLIB_SX128X_FLRC_BR_1_000_BW_1_2;
    } else if((uint16_t)br == 1300) {
      this->bitRate = RADIOLIB_SX128X_FLRC_BR_1_300_BW_1_2;
    } else {
      return(RADIOLIB_ERR_INVALID_BIT_RATE);
    }

    // update modulation parameters
    this->bitRateKbps = (uint16_t)br;
    return(setModulationParams(this->bitRate, this->codingRateFLRC, this->shaping));

  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX128x::setFrequencyDeviation(float freqDev) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_BLE))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0f) {
    newFreqDev = 62.5f;
  }

  RADIOLIB_CHECK_RANGE(newFreqDev, 62.5f, 1000.0f, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);

  // override for the lowest possible frequency deviation - required for some PhysicalLayer protocols
  if(newFreqDev == 0.0f) {
    this->modIndex = RADIOLIB_SX128X_BLE_GFSK_MOD_IND_0_35;
    this->bitRate = RADIOLIB_SX128X_BLE_GFSK_BR_0_125_BW_0_3;
    return(setModulationParams(this->bitRate, this->modIndex, this->shaping));
  }

  // update modulation parameters
  uint8_t modInd = (uint8_t)((8.0f * (newFreqDev / (float)this->bitRateKbps)) - 1.0f);
  if(modInd > RADIOLIB_SX128X_BLE_GFSK_MOD_IND_4_00) {
    return(RADIOLIB_ERR_INVALID_MODULATION_PARAMETERS);
  }

  // update modulation parameters
  this->frequencyDev = newFreqDev;
  this->modIndex = modInd;
  return(setModulationParams(this->bitRate, this->modIndex, this->shaping));
}

int16_t SX128x::setDataShaping(uint8_t sh) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_BLE) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set data this->shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      this->shaping = RADIOLIB_SX128X_BLE_GFSK_BT_OFF;
      break;
    case RADIOLIB_SHAPING_0_5:
      this->shaping = RADIOLIB_SX128X_BLE_GFSK_BT_0_5;
      break;
    case RADIOLIB_SHAPING_1_0:
      this->shaping = RADIOLIB_SX128X_BLE_GFSK_BT_1_0;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
  }

  // update modulation parameters
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_BLE)) {
    return(setModulationParams(this->bitRate, this->modIndex, this->shaping));
  } else {
    return(setModulationParams(this->bitRate, this->codingRateFLRC, this->shaping));
  }
}

int16_t SX128x::setSyncWord(uint8_t* sync, size_t len) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) {
    // GFSK can use up to 5 bytes as sync word
    if(len > 5) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }

    // calculate sync word length parameter value
    if(len > 0) {
      this->syncWordLen = (len - 1)*2;
    }

  } else {
    // FLRC requires 32-bit sync word
    if(!((len == 0) || (len == 4))) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }

    // save sync word length parameter value
    this->syncWordLen = len;
  }

  // update sync word
  int16_t state = SX128x::writeRegister(RADIOLIB_SX128X_REG_SYNC_WORD_1_BYTE_4 + (5 - len), sync, len);
  RADIOLIB_ASSERT(state);

  // update packet parameters
  if(this->syncWordLen == 0) {
    this->syncWordMatch = RADIOLIB_SX128X_GFSK_FLRC_SYNC_WORD_OFF;
  } else {
    /// \todo add support for multiple sync words
    this->syncWordMatch = RADIOLIB_SX128X_GFSK_FLRC_SYNC_WORD_1;
  }
  return(setPacketParamsGFSK(this->preambleLengthGFSK, this->syncWordLen, this->syncWordMatch, this->crcGFSK, this->whitening, this->packetType));
}

int16_t SX128x::setSyncWord(uint8_t syncWord, uint8_t controlBits) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update register
  const uint8_t data[2] = {(uint8_t)((syncWord & 0xF0) | ((controlBits & 0xF0) >> 4)), (uint8_t)(((syncWord & 0x0F) << 4) | (controlBits & 0x0F))};
  return(writeRegister(RADIOLIB_SX128X_REG_LORA_SYNC_WORD_MSB, data, 2));
}

int16_t SX128x::setCRC(uint8_t len, uint32_t initial, uint16_t polynomial) {
  // check active modem
  uint8_t modem = getPacketType();

  int16_t state;
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC)) {
    // update packet parameters
    if(modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) {
      if(len > 2) {
        return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
      }
    } else {
      if(len > 3) {
        return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
      }
    }
    this->crcGFSK = len << 4;
    state = setPacketParamsGFSK(this->preambleLengthGFSK, this->syncWordLen, this->syncWordMatch, this->crcGFSK, this->whitening, this->packetType);
    RADIOLIB_ASSERT(state);

    // set initial CRC value
    uint8_t data[] = { (uint8_t)((initial >> 8) & 0xFF), (uint8_t)(initial & 0xFF) };
    state = writeRegister(RADIOLIB_SX128X_REG_CRC_INITIAL_MSB, data, 2);
    RADIOLIB_ASSERT(state);

    // set CRC polynomial
    data[0] = (uint8_t)((polynomial >> 8) & 0xFF);
    data[1] = (uint8_t)(polynomial & 0xFF);
    state = writeRegister(RADIOLIB_SX128X_REG_CRC_POLYNOMIAL_MSB, data, 2);
    return(state);

  } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_BLE) {
    // update packet parameters
    if(len == 0) {
      this->crcBLE = RADIOLIB_SX128X_BLE_CRC_OFF;
    } else if(len == 3) {
      this->crcBLE = RADIOLIB_SX128X_BLE_CRC_3_BYTE;
    } else {
      return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }
    state = setPacketParamsBLE(this->connectionState, this->crcBLE, this->bleTestPayload, this->whitening);
    RADIOLIB_ASSERT(state);

    // set initial CRC value
    const uint8_t data[] = { (uint8_t)((initial >> 16) & 0xFF), (uint8_t)((initial >> 8) & 0xFF), (uint8_t)(initial & 0xFF) };
    state = writeRegister(RADIOLIB_SX128X_REG_BLE_CRC_INITIAL_MSB, data, 3);
    return(state);

  } else if((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING)) {
    // update packet parameters
    if(len == 0) {
      this->crcLoRa = RADIOLIB_SX128X_LORA_CRC_OFF;
    } else if(len == 2) {
      this->crcLoRa = RADIOLIB_SX128X_LORA_CRC_ON;
    } else {
      return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }
    state = setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->payloadLen, this->crcLoRa, this->invertIQEnabled);
    return(state);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX128x::setWhitening(bool enabled) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_BLE))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update packet parameters
  if(enabled) {
    this->whitening = RADIOLIB_SX128X_GFSK_BLE_WHITENING_ON;
  } else {
    this->whitening = RADIOLIB_SX128X_GFSK_BLE_WHITENING_OFF;
  }

  if(modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) {
    return(setPacketParamsGFSK(this->preambleLengthGFSK, this->syncWordLen, this->syncWordMatch, this->crcGFSK, this->whitening, this->packetType));
  }
  return(setPacketParamsBLE(this->connectionState, this->crcBLE, this->bleTestPayload, this->whitening));
}

int16_t SX128x::setAccessAddress(uint32_t addr) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX128X_PACKET_TYPE_BLE) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set the address
  const uint8_t addrBuff[] = { (uint8_t)((addr >> 24) & 0xFF), (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF) };
  return(SX128x::writeRegister(RADIOLIB_SX128X_REG_ACCESS_ADDRESS_BYTE_3, addrBuff, 4));
}

int16_t SX128x::setHighSensitivityMode(bool enable) {
  // read the current registers
  uint8_t RxGain = 0;
  int16_t state = readRegister(RADIOLIB_SX128X_REG_GAIN_MODE, &RxGain, 1);
  RADIOLIB_ASSERT(state);

  if(enable) {
    RxGain |= 0xC0; // Set bits 6 and 7
  } else {
    RxGain &= ~0xC0; // Unset bits 6 and 7
  }

  // update all values
  state = writeRegister(RADIOLIB_SX128X_REG_GAIN_MODE, &RxGain, 1);
  return(state);
}

int16_t SX128x::setGainControl(uint8_t gain) {
  // read the current registers
  uint8_t ManualGainSetting = 0;
  int16_t state = readRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_CONTROL_ENABLE_2, &ManualGainSetting, 1);
  RADIOLIB_ASSERT(state);
  uint8_t LNAGainValue = 0;
  state = readRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_SETTING, &LNAGainValue, 1);
  RADIOLIB_ASSERT(state);
  uint8_t LNAGainControl = 0;
  state = readRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_CONTROL_ENABLE_1, &LNAGainControl, 1);
  RADIOLIB_ASSERT(state);

  // set the gain
  if (gain > 0 && gain < 14) {
    // Set manual gain
    ManualGainSetting &= ~0x01; // Set bit 0 to 0 (Enable Manual Gain Control)
    LNAGainValue &= 0xF0; // Bits 0, 1, 2 and 3 to 0
    LNAGainValue |= gain; // Set bits 0, 1, 2 and 3 to Manual Gain Setting (1-13)
    LNAGainControl |= 0x80; // Set bit 7 to 1 (Enable Manual Gain Control)
  } else {
    // Set automatic gain if 0 or out of range
    ManualGainSetting |= 0x01; // Set bit 0 to 1 (Enable Automatic Gain Control)
    LNAGainValue &= 0xF0; // Bits 0, 1, 2 and 3 to 0
    LNAGainValue |= 0x0A; // Set bits 0, 1, 2 and 3 to Manual Gain Setting (1-13)
    LNAGainControl &= ~0x80; // Set bit 7 to 0 (Enable Automatic Gain Control)
  }

  // update all values
  state = writeRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_CONTROL_ENABLE_2, &ManualGainSetting, 1);
  RADIOLIB_ASSERT(state);
  state = writeRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_SETTING, &LNAGainValue, 1);
  RADIOLIB_ASSERT(state);
  state = writeRegister(RADIOLIB_SX128X_REG_MANUAL_GAIN_CONTROL_ENABLE_1, &LNAGainControl, 1);
  return(state);
}

float SX128x::getRSSI() {
  // get packet status
  uint8_t packetStatus[5];
  this->mod->SPIreadStream(RADIOLIB_SX128X_CMD_GET_PACKET_STATUS, packetStatus, 5);

  // check active modem
  uint8_t modem = getPacketType();
  if((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING)) {
    // LoRa or ranging
    uint8_t rssiSync = packetStatus[0];
    float rssiMeasured = -1.0 * rssiSync/2.0;
    float snr = getSNR();
    if(snr <= 0.0f) {
      return(rssiMeasured - snr);
    } else {
      return(rssiMeasured);
    }
  } else {
    // GFSK, BLE or FLRC
    uint8_t rssiSync = packetStatus[1];
    return(-1.0 * rssiSync/2.0);
  }
}

float SX128x::getRSSI(bool packet) {
    if (!packet) {
        // get instantaneous RSSI value
        uint8_t data[3] = {0, 0, 0}; // RssiInst, Status, RFU
        this->mod->SPIreadStream(RADIOLIB_SX128X_CMD_GET_RSSI_INST, data, 3);
        return ((float)data[0] / (-2.0f));
    } else {
        return this->getRSSI();
    }
}

float SX128x::getSNR() {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING))) {
    return(0.0);
  }

  // get packet status
  uint8_t packetStatus[5];
  this->mod->SPIreadStream(RADIOLIB_SX128X_CMD_GET_PACKET_STATUS, packetStatus, 5);

  // calculate real SNR
  uint8_t snr = packetStatus[1];
  if(snr < 128) {
    return(snr/4.0);
  } else {
    return((snr - 256)/4.0f);
  }
}

float SX128x::getFrequencyError() {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING))) {
    return(0.0);
  }

  // read the raw frequency error register values
  uint8_t efeRaw[3] = {0};
  int16_t state = readRegister(RADIOLIB_SX128X_REG_FEI_MSB, &efeRaw[0], 1);
  RADIOLIB_ASSERT(state);
  state = readRegister(RADIOLIB_SX128X_REG_FEI_MID, &efeRaw[1], 1);
  RADIOLIB_ASSERT(state);
  state = readRegister(RADIOLIB_SX128X_REG_FEI_LSB, &efeRaw[2], 1);
  RADIOLIB_ASSERT(state);
  uint32_t efe = ((uint32_t) efeRaw[0] << 16) | ((uint32_t) efeRaw[1] << 8) | efeRaw[2];
  efe &= 0x0FFFFF;

  float error = 0;

  // check the first bit
  if (efe & 0x80000) {
    // frequency error is negative
    efe |= (uint32_t) 0xFFF00000;
    efe = ~efe + 1;
    error = 1.55f * (float) efe / (1600.0f / this->bandwidthKhz) * -1.0f;
  } else {
    error = 1.55f * (float) efe / (1600.0f / this->bandwidthKhz);
  }

  return(error);
}

size_t SX128x::getPacketLength(bool update) {
  return(this->getPacketLength(update, NULL));
}

size_t SX128x::getPacketLength(bool update, uint8_t* offset) {
  (void)update;

  // in implicit mode, return the cached value
  if((getPacketType() == RADIOLIB_SX128X_PACKET_TYPE_LORA) && (this->headerType == RADIOLIB_SX128X_LORA_HEADER_IMPLICIT)) {
    return(this->payloadLen);
  }

  uint8_t rxBufStatus[2] = {0, 0};
  this->mod->SPIreadStream(RADIOLIB_SX128X_CMD_GET_RX_BUFFER_STATUS, rxBufStatus, 2);

  if(offset) { *offset = rxBufStatus[1]; }

  return((size_t)rxBufStatus[0]);
}

int16_t SX128x::getLoRaRxHeaderInfo(uint8_t* cr, bool* hasCRC) {
  int16_t state = RADIOLIB_ERR_NONE;

  // check if in explicit header mode
  if(this->headerType == RADIOLIB_SX128X_LORA_HEADER_IMPLICIT) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(cr) { *cr = this->mod->SPIgetRegValue(RADIOLIB_SX128X_REG_LORA_RX_CODING_RATE, 6, 4) >> 4; }
  if(hasCRC) { *hasCRC = (this->mod->SPIgetRegValue(RADIOLIB_SX128X_REG_FEI_MSB, 4, 4) != 0); }

  return(state);
}

int16_t SX128x::fixedPacketLengthMode(uint8_t len) {
  return(setPacketMode(RADIOLIB_SX128X_GFSK_FLRC_PACKET_FIXED, len));
}

int16_t SX128x::variablePacketLengthMode(uint8_t maxLen) {
  return(setPacketMode(RADIOLIB_SX128X_GFSK_FLRC_PACKET_VARIABLE, maxLen));
}

RadioLibTime_t SX128x::calculateTimeOnAir(ModemType_t modem, DataRate_t dr, PacketConfig_t pc, size_t len) {
  switch(modem) {
    case (ModemType_t::RADIOLIB_MODEM_LORA): {
      // calculate number of symbols
      float N_symbol = 0;
      uint8_t sf = dr.lora.spreadingFactor;
      float cr = (float)dr.lora.codingRate;

      // get SF coefficients
      float coeff1 = 0;
      int16_t coeff2 = 0;
      int16_t coeff3 = 0;
      if(sf < 7) {
        // SF5, SF6
        coeff1 = 6.25;
        coeff2 = 4*sf;
        coeff3 = 4*sf;
      } else if(sf < 11) {
        // SF7. SF8, SF9, SF10
        coeff1 = 4.25;
        coeff2 = 4*sf + 8;
        coeff3 = 4*sf;
      } else {
        // SF11, SF12
        coeff1 = 4.25;
        coeff2 = 4*sf + 8;
        coeff3 = 4*(sf - 2);
      }

      // get CRC length
      int16_t N_bitCRC = 16;
      if(!pc.lora.crcEnabled) {
        N_bitCRC = 0;
      }

      // get header length
      int16_t N_symbolHeader = 20;
      if(pc.lora.implicitHeader) {
        N_symbolHeader = 0;
      }

      // calculate number of LoRa preamble symbols
      uint32_t N_symbolPreamble = pc.lora.preambleLength;

      // calculate the number of symbols
      N_symbol = (float)N_symbolPreamble + coeff1 + 8.0f + ceilf((float)RADIOLIB_MAX((int16_t)(8 * len + N_bitCRC - coeff2 + N_symbolHeader), (int16_t)0) / (float)coeff3) * cr;

      // get time-on-air in us
      return(((uint32_t(1) << sf) / dr.lora.bandwidth) * N_symbol * 1000.0f);
    }
    case (ModemType_t::RADIOLIB_MODEM_FSK):
      return((((float)(pc.fsk.crcLength * 8) + pc.fsk.syncWordLength + pc.fsk.preambleLength + (uint32_t)len * 8) / (dr.fsk.bitRate / 1000.0f)));

    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
  
}

RadioLibTime_t SX128x::getTimeOnAir(size_t len) {
  // check active modem
  uint8_t modem = getPacketType();
  DataRate_t dr = {};
  PacketConfig_t pc = {};
  
  if(modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    uint8_t sf = this->spreadingFactor >> 4;
    uint8_t cr = this->codingRateLoRa;
    // We assume same calculation for short and long interleaving, so map CR values 0-4 and 5-7 to the same values
    if (cr < 5) {
      cr = cr + 4;
    } else if (cr == 7) {
      cr = cr + 1;
    }
    
    dr.lora.spreadingFactor = sf;
    dr.lora.codingRate = cr;
    dr.lora.bandwidth = this->bandwidthKhz;

    uint16_t preambleLength = (this->preambleLengthLoRa & 0x0F) * (uint32_t(1) << ((this->preambleLengthLoRa & 0xF0) >> 4));
    
    pc.lora.preambleLength = preambleLength;
    pc.lora.implicitHeader = this->headerType == RADIOLIB_SX128X_LORA_HEADER_IMPLICIT;
    pc.lora.crcEnabled = this->crcLoRa == RADIOLIB_SX128X_LORA_CRC_ON;
    pc.lora.ldrOptimize = false;

    return(calculateTimeOnAir(ModemType_t::RADIOLIB_MODEM_LORA, dr, pc, len));
  } else if (modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) {
    dr.fsk.bitRate = (float)this->bitRateKbps;
    dr.fsk.freqDev = this->frequencyDev;

    pc.fsk.preambleLength = ((uint16_t)this->preambleLengthGFSK >> 2) + 4;
    pc.fsk.syncWordLength = ((this->syncWordLen >> 1) + 1) * 8;
    pc.fsk.crcLength = this->crcGFSK >> 4;

    return(calculateTimeOnAir(ModemType_t::RADIOLIB_MODEM_FSK, dr, pc, len));
  } else {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

}

RadioLibTime_t SX128x::calculateRxTimeout(RadioLibTime_t timeoutUs) {
  // the timeout value is given in units of 15.625 microseconds
  // the calling function should provide some extra width, as this number of units is truncated to integer
  RadioLibTime_t timeout = timeoutUs / 15.625f;
  return(timeout);
}

int16_t SX128x::implicitHeader(size_t len) {
  return(setHeaderType(RADIOLIB_SX128X_LORA_HEADER_IMPLICIT, len));
}

int16_t SX128x::explicitHeader() {
  return(setHeaderType(RADIOLIB_SX128X_LORA_HEADER_EXPLICIT));
}

int16_t SX128x::setEncoding(uint8_t encoding) {
  return(setWhitening(encoding));
}

void SX128x::setRfSwitchPins(uint32_t rxEn, uint32_t txEn) {
  this->mod->setRfSwitchPins(rxEn, txEn);
}

void SX128x::setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]) {
  this->mod->setRfSwitchTable(pins, table);
}

uint8_t SX128x::randomByte() {
  // it's unclear whether SX128x can measure RSSI while not receiving a packet
  // this method is implemented only for PhysicalLayer compatibility
  return(0);
}

int16_t SX128x::invertIQ(bool enable) {
  if(getPacketType() != RADIOLIB_SX128X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(enable) {
    this->invertIQEnabled = RADIOLIB_SX128X_LORA_IQ_INVERTED;
  } else {
    this->invertIQEnabled = RADIOLIB_SX128X_LORA_IQ_STANDARD;
  }

  return(setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->payloadLen, this->crcLoRa, this->invertIQEnabled));
}

int16_t SX128x::stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg) {
  int16_t state;

  switch(mode) {
    case(RADIOLIB_RADIO_MODE_RX): {
      // in implicit header mode, use the provided length if it is nonzero
      // otherwise we trust the user has previously set the payload length manually
      if((this->headerType == RADIOLIB_SX128X_LORA_HEADER_IMPLICIT) && (cfg->receive.len != 0)) {
        this->payloadLen = cfg->receive.len;
      }
      
      // check active modem
      if(getPacketType() == RADIOLIB_SX128X_PACKET_TYPE_RANGING) {
        return(RADIOLIB_ERR_WRONG_MODEM);
      }

      // set DIO mapping
      if(cfg->receive.timeout != RADIOLIB_SX128X_RX_TIMEOUT_INF) {
        cfg->receive.irqMask |= (1UL << RADIOLIB_IRQ_TIMEOUT);
      }

      state = setDioIrqParams(getIrqMapped(cfg->receive.irqFlags), getIrqMapped(cfg->receive.irqMask));
      RADIOLIB_ASSERT(state);

      // set buffer pointers
      state = setBufferBaseAddress();
      RADIOLIB_ASSERT(state);

      // clear interrupt flags
      state = clearIrqStatus();
      RADIOLIB_ASSERT(state);

      // set implicit mode and expected len if applicable
      if((this->headerType == RADIOLIB_SX128X_LORA_HEADER_IMPLICIT) && (getPacketType() == RADIOLIB_SX128X_PACKET_TYPE_LORA)) {
        state = setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->payloadLen, this->crcLoRa, this->invertIQEnabled);
        RADIOLIB_ASSERT(state);
      }
      // if max(uint32_t) is used, revert to RxContinuous
      if(cfg->receive.timeout == 0xFFFFFFFF) {
        cfg->receive.timeout = 0xFFFF;
      }
      this->rxTimeout = cfg->receive.timeout;
    } break;
  
    case(RADIOLIB_RADIO_MODE_TX): {
      // check packet length
      if(cfg->transmit.len > RADIOLIB_SX128X_MAX_PACKET_LENGTH) {
        return(RADIOLIB_ERR_PACKET_TOO_LONG);
      }

      // set packet Length
      uint8_t modem = getPacketType();
      if(modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) {
        state = setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, cfg->transmit.len, this->crcLoRa, this->invertIQEnabled);
      } else if((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC)) {
        state = setPacketParamsGFSK(this->preambleLengthGFSK, this->syncWordLen, this->syncWordMatch, this->crcGFSK, this->whitening, this->packetType, cfg->transmit.len);
      } else if(modem == RADIOLIB_SX128X_PACKET_TYPE_BLE) {
        state = setPacketParamsBLE(this->connectionState, this->crcBLE, this->bleTestPayload, this->whitening);
      } else {
        return(RADIOLIB_ERR_WRONG_MODEM);
      }
      RADIOLIB_ASSERT(state);

      // update output power
      state = setTxParams(this->power);
      RADIOLIB_ASSERT(state);

      // set buffer pointers
      state = setBufferBaseAddress();
      RADIOLIB_ASSERT(state);

      // write packet to buffer
      if(modem == RADIOLIB_SX128X_PACKET_TYPE_BLE) {
        // first 2 bytes of BLE payload are PDU header
        state = writeBuffer(cfg->transmit.data, cfg->transmit.len, 2);
        RADIOLIB_ASSERT(state);
      } else {
        state = writeBuffer(cfg->transmit.data, cfg->transmit.len);
        RADIOLIB_ASSERT(state);
      }

      // set DIO mapping
      state = setDioIrqParams(RADIOLIB_SX128X_IRQ_TX_DONE | RADIOLIB_SX128X_IRQ_RX_TX_TIMEOUT, RADIOLIB_SX128X_IRQ_TX_DONE);
      RADIOLIB_ASSERT(state);

      // clear interrupt flags
      state = clearIrqStatus();
      RADIOLIB_ASSERT(state);
    } break;
    
    default:
      return(RADIOLIB_ERR_UNSUPPORTED);
  }

  this->stagedMode = mode;
  return(state);
}

int16_t SX128x::launchMode() {
  int16_t state;
  switch(this->stagedMode) {
    case(RADIOLIB_RADIO_MODE_RX): {
      this->mod->setRfSwitchState(Module::MODE_RX);
      state = setRx(this->rxTimeout);
      RADIOLIB_ASSERT(state);
    } break;
  
    case(RADIOLIB_RADIO_MODE_TX): {
      this->mod->setRfSwitchState(Module::MODE_TX);
      state = setTx(RADIOLIB_SX128X_TX_TIMEOUT_NONE);
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
void SX128x::setDirectAction(void (*func)(void)) {
  // SX128x is unable to perform direct mode reception
  // this method is implemented only for PhysicalLayer compatibility
  (void)func;
}

void SX128x::readBit(uint32_t pin) {
  // SX128x is unable to perform direct mode reception
  // this method is implemented only for PhysicalLayer compatibility
  (void)pin;
}
#endif

Module* SX128x::getMod() {
  return(this->mod);
}

int16_t SX128x::modSetup(uint8_t modem) {
  // set module properties
  this->mod->init();
  this->mod->hal->pinMode(this->mod->getIrq(), this->mod->hal->GpioModeInput);
  this->mod->hal->pinMode(this->mod->getGpio(), this->mod->hal->GpioModeInput);
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR] = Module::BITS_16;
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_8;
  this->mod->spiConfig.statusPos = 1;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_SX128X_CMD_READ_REGISTER;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_SX128X_CMD_WRITE_REGISTER;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_NOP] = RADIOLIB_SX128X_CMD_NOP;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_STATUS] = RADIOLIB_SX128X_CMD_GET_STATUS;
  this->mod->spiConfig.stream = true;
  this->mod->spiConfig.parseStatusCb = SPIparseStatus;

  // find the chip - this will also reset the module and verify the module
  if(!SX128x::findChip(this->chipType)) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("No SX128x found!");
    this->mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  RADIOLIB_DEBUG_BASIC_PRINTLN("M\tSX128x");

  // configure settings not accessible by API
  return(config(modem));
}

uint8_t SX128x::getStatus() {
  uint8_t data = 0;
  this->mod->SPIreadStream(RADIOLIB_SX128X_CMD_GET_STATUS, &data, 0);
  return(data);
}

int16_t SX128x::writeRegister(uint16_t addr, const uint8_t* data, uint8_t numBytes) {
  this->mod->SPIwriteRegisterBurst(addr, data, numBytes);
  return(RADIOLIB_ERR_NONE);
}

int16_t SX128x::readRegister(uint16_t addr, uint8_t* data, uint8_t numBytes) {
  // send the command
  this->mod->SPIreadRegisterBurst(addr, numBytes, data);

  // check the status
  int16_t state = this->mod->SPIcheckStream();
  return(state);
}

int16_t SX128x::writeBuffer(const uint8_t* data, uint8_t numBytes, uint8_t offset) {
  const uint8_t cmd[] = { RADIOLIB_SX128X_CMD_WRITE_BUFFER, offset };
  return(this->mod->SPIwriteStream(cmd, 2, data, numBytes));
}

int16_t SX128x::readBuffer(uint8_t* data, uint8_t numBytes, uint8_t offset) {
  const uint8_t cmd[] = { RADIOLIB_SX128X_CMD_READ_BUFFER, offset };
  return(this->mod->SPIreadStream(cmd, 2, data, numBytes));
}

int16_t SX128x::setTx(uint16_t periodBaseCount, uint8_t periodBase) {
  const uint8_t data[] = { periodBase, (uint8_t)((periodBaseCount >> 8) & 0xFF), (uint8_t)(periodBaseCount & 0xFF) };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_TX, data, 3));
}

int16_t SX128x::setRx(uint16_t periodBaseCount, uint8_t periodBase) {
  const uint8_t data[] = { periodBase, (uint8_t)((periodBaseCount >> 8) & 0xFF), (uint8_t)(periodBaseCount & 0xFF) };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_RX, data, 3));
}

int16_t SX128x::setCad(uint8_t symbolNum) {
  // configure parameters
  int16_t state = this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_CAD_PARAMS, &symbolNum, 1);
  RADIOLIB_ASSERT(state);

  // start CAD
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_CAD, NULL, 0));
}

uint8_t SX128x::getPacketType() {
  uint8_t data = 0xFF;
  this->mod->SPIreadStream(RADIOLIB_SX128X_CMD_GET_PACKET_TYPE, &data, 1);
  return(data);
}

int16_t SX128x::setRfFrequency(uint32_t frf) {
  const uint8_t data[] = { (uint8_t)((frf >> 16) & 0xFF), (uint8_t)((frf >> 8) & 0xFF), (uint8_t)(frf & 0xFF) };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_RF_FREQUENCY, data, 3));
}

int16_t SX128x::setTxParams(uint8_t pwr, uint8_t rampTime) {
  const uint8_t data[] = { pwr, rampTime };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_TX_PARAMS, data, 2));
}

int16_t SX128x::setBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress) {
  const uint8_t data[] = { txBaseAddress, rxBaseAddress };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_BUFFER_BASE_ADDRESS, data, 2));
}

int16_t SX128x::setModulationParams(uint8_t modParam1, uint8_t modParam2, uint8_t modParam3) {
  const uint8_t data[] = { modParam1, modParam2, modParam3 };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_MODULATION_PARAMS, data, 3));
}

int16_t SX128x::setPacketParamsGFSK(uint8_t preambleLen, uint8_t syncLen, uint8_t syncMatch, uint8_t crcLen, uint8_t whiten, uint8_t hdrType, uint8_t payLen) {
  const uint8_t data[] = { preambleLen, syncLen, syncMatch, hdrType, payLen, crcLen, whiten };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_PACKET_PARAMS, data, 7));
}

int16_t SX128x::setPacketParamsBLE(uint8_t connState, uint8_t crcLen, uint8_t bleTest, uint8_t whiten) {
  const uint8_t data[] = { connState, crcLen, bleTest, whiten, 0x00, 0x00, 0x00 };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_PACKET_PARAMS, data, 7));
}

int16_t SX128x::setPacketParamsLoRa(uint8_t preambleLen, uint8_t hdrType, uint8_t payLen, uint8_t crc, uint8_t invIQ) {
  const uint8_t data[] = { preambleLen, hdrType, payLen, crc, invIQ, 0x00, 0x00 };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_PACKET_PARAMS, data, 7));
}

int16_t SX128x::setDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask) {
  const uint8_t data[] = { (uint8_t)((irqMask >> 8) & 0xFF), (uint8_t)(irqMask & 0xFF),
                     (uint8_t)((dio1Mask >> 8) & 0xFF), (uint8_t)(dio1Mask & 0xFF),
                     (uint8_t)((dio2Mask >> 8) & 0xFF), (uint8_t)(dio2Mask & 0xFF),
                     (uint8_t)((dio3Mask >> 8) & 0xFF), (uint8_t)(dio3Mask & 0xFF) };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_DIO_IRQ_PARAMS, data, 8));
}

uint16_t SX128x::getIrqStatus() {
  uint8_t data[] = { 0x00, 0x00 };
  this->mod->SPIreadStream(RADIOLIB_SX128X_CMD_GET_IRQ_STATUS, data, 2);
  return(((uint16_t)(data[0]) << 8) | data[1]);
}

int16_t SX128x::clearIrqStatus(uint16_t clearIrqParams) {
  const uint8_t data[] = { (uint8_t)((clearIrqParams >> 8) & 0xFF), (uint8_t)(clearIrqParams & 0xFF) };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_CLEAR_IRQ_STATUS, data, 2));
}

int16_t SX128x::setRangingRole(uint8_t role) {
  const uint8_t data[] = { role };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_RANGING_ROLE, data, 1));
}

int16_t SX128x::setPacketType(uint8_t type) {
  const uint8_t data[] = { type };
  return(this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_PACKET_TYPE, data, 1));
}

int16_t SX128x::setPacketMode(uint8_t mode, uint8_t len) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_GFSK) || (modem == RADIOLIB_SX128X_PACKET_TYPE_FLRC))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  int16_t state = setPacketParamsGFSK(this->preambleLengthGFSK, this->syncWordLen, this->syncWordMatch, this->crcGFSK, this->whitening, mode, len);
  RADIOLIB_ASSERT(state);

  // update cached value
  this->packetType = mode;
  return(state);
}

int16_t SX128x::setHeaderType(uint8_t hdrType, size_t len) {
  // check active modem
  uint8_t modem = getPacketType();
  if(!((modem == RADIOLIB_SX128X_PACKET_TYPE_LORA) || (modem == RADIOLIB_SX128X_PACKET_TYPE_RANGING))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update packet parameters
  this->headerType = hdrType;
  this->payloadLen = len;
  return(setPacketParamsLoRa(this->preambleLengthLoRa, this->headerType, this->payloadLen, this->crcLoRa, this->invertIQEnabled));
}

int16_t SX128x::config(uint8_t modem) {
  // reset buffer base address
  int16_t state = setBufferBaseAddress();
  RADIOLIB_ASSERT(state);

  // set modem
  uint8_t data[1];
  data[0] = modem;
  state = this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_PACKET_TYPE, data, 1);
  RADIOLIB_ASSERT(state);

  // set CAD parameters
  data[0] = RADIOLIB_SX128X_CAD_ON_8_SYMB;
  state = this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_CAD_PARAMS, data, 1);
  RADIOLIB_ASSERT(state);

  // set regulator mode to DC-DC
  data[0] = RADIOLIB_SX128X_REGULATOR_DC_DC;
  state = this->mod->SPIwriteStream(RADIOLIB_SX128X_CMD_SET_REGULATOR_MODE, data, 1);
  RADIOLIB_ASSERT(state);

  return(RADIOLIB_ERR_NONE);
}

int16_t SX128x::SPIparseStatus(uint8_t in) {
  if((in & 0b00011100) == RADIOLIB_SX128X_STATUS_CMD_TIMEOUT) {
    return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
  } else if((in & 0b00011100) == RADIOLIB_SX128X_STATUS_CMD_ERROR) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  } else if((in & 0b00011100) == RADIOLIB_SX128X_STATUS_CMD_FAILED) {
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  } else if((in == 0x00) || (in == 0xFF)) {
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  return(RADIOLIB_ERR_NONE);
}

bool SX128x::findChip(const char* verStr) {
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    // reset the module
    reset(true);

    // read the version string
    char version[16] = { 0 };
    this->mod->SPIreadRegisterBurst(RADIOLIB_SX128X_REG_VERSION_STRING, 16, reinterpret_cast<uint8_t*>(version));

    // check version register
    if(strncmp(verStr, version, 6) == 0) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("Found SX128x: RADIOLIB_SX128X_REG_VERSION_STRING:");
      RADIOLIB_DEBUG_BASIC_HEXDUMP(reinterpret_cast<uint8_t*>(version), 16, RADIOLIB_SX128X_REG_VERSION_STRING);
      RADIOLIB_DEBUG_BASIC_PRINTLN();
      flagFound = true;
    } else {
      #if RADIOLIB_DEBUG_BASIC
        RADIOLIB_DEBUG_BASIC_PRINTLN("SX128x not found! (%d of 10 tries) RADIOLIB_SX128X_REG_VERSION_STRING:", i + 1);
        RADIOLIB_DEBUG_BASIC_HEXDUMP(reinterpret_cast<uint8_t*>(version), 16, RADIOLIB_SX128X_REG_VERSION_STRING);
        RADIOLIB_DEBUG_BASIC_PRINTLN("Expected string: %s", verStr);
      #endif
      this->mod->hal->delay(10);
      i++;
    }
  }

  return(flagFound);
}

#endif
