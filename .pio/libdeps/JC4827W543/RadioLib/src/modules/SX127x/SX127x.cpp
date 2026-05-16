#include "SX127x.h"
#include <math.h>
#if !RADIOLIB_EXCLUDE_SX127X

SX127x::SX127x(Module* mod) : PhysicalLayer() {
  this->freqStep = RADIOLIB_SX127X_FREQUENCY_STEP_SIZE;
  this->maxPacketLength = RADIOLIB_SX127X_MAX_PACKET_LENGTH;
  this->mod = mod;
}

int16_t SX127x::begin(const uint8_t* chipVersions, uint8_t numVersions, uint8_t syncWord, uint16_t preambleLength) {
  // set module properties
  this->mod->init();
  this->mod->hal->pinMode(this->mod->getIrq(), this->mod->hal->GpioModeInput);
  this->mod->hal->pinMode(this->mod->getGpio(), this->mod->hal->GpioModeInput);

  // set IRQ mapping - it is different for LoRa and FSK mode
  this->irqMap[RADIOLIB_IRQ_TX_DONE] = RADIOLIB_SX127X_CLEAR_IRQ_FLAG_TX_DONE;
  this->irqMap[RADIOLIB_IRQ_RX_DONE] = RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_DONE;
  this->irqMap[RADIOLIB_IRQ_PREAMBLE_DETECTED] = RADIOLIB_IRQ_NOT_SUPPORTED;
  this->irqMap[RADIOLIB_IRQ_SYNC_WORD_VALID] = RADIOLIB_IRQ_NOT_SUPPORTED;
  this->irqMap[RADIOLIB_IRQ_HEADER_VALID] = RADIOLIB_SX127X_CLEAR_IRQ_FLAG_VALID_HEADER;
  this->irqMap[RADIOLIB_IRQ_HEADER_ERR] = RADIOLIB_IRQ_NOT_SUPPORTED;
  this->irqMap[RADIOLIB_IRQ_CRC_ERR] = RADIOLIB_SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR;
  this->irqMap[RADIOLIB_IRQ_CAD_DONE] = RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DONE;
  this->irqMap[RADIOLIB_IRQ_CAD_DETECTED] = RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED;
  this->irqMap[RADIOLIB_IRQ_TIMEOUT] = RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_TIMEOUT;

  // try to find the SX127x chip
  if(!SX127x::findChip(chipVersions, numVersions)) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("No SX127x found!");
    this->mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  RADIOLIB_DEBUG_BASIC_PRINTLN("M\tSX127x");

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // configure settings not accessible by API
  state = config();
  RADIOLIB_ASSERT(state);

  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    // set LoRa mode
    state = setActiveModem(RADIOLIB_SX127X_LORA);
    RADIOLIB_ASSERT(state);
  }

  // set LoRa sync word
  state = SX127x::setSyncWord(syncWord);
  RADIOLIB_ASSERT(state);

  // set over current protection
  state = SX127x::setCurrentLimit(60);
  RADIOLIB_ASSERT(state);

  // set preamble length
  state = SX127x::setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // disable IQ inversion
  state = SX127x::invertIQ(false);
  RADIOLIB_ASSERT(state);

  return(state);
}

int16_t SX127x::beginFSK(const uint8_t* chipVersions, uint8_t numVersions, float freqDev, float rxBw, uint16_t preambleLength, bool enableOOK) {
  // set module properties
  this->mod->init();
  this->mod->hal->pinMode(this->mod->getIrq(), this->mod->hal->GpioModeInput);
  this->mod->hal->pinMode(this->mod->getGpio(), this->mod->hal->GpioModeInput);

  // set IRQ mapping - it is different for LoRa and FSK mode
  this->irqMap[RADIOLIB_IRQ_TX_DONE] = RADIOLIB_SX127X_FLAG_PACKET_SENT << 8;
  this->irqMap[RADIOLIB_IRQ_RX_DONE] = RADIOLIB_SX127X_FLAG_PAYLOAD_READY << 8;
  this->irqMap[RADIOLIB_IRQ_PREAMBLE_DETECTED] = RADIOLIB_SX127X_FLAG_PREAMBLE_DETECT << 0;
  this->irqMap[RADIOLIB_IRQ_SYNC_WORD_VALID] = RADIOLIB_SX127X_FLAG_SYNC_ADDRESS_MATCH << 0;
  this->irqMap[RADIOLIB_IRQ_HEADER_VALID] = RADIOLIB_IRQ_NOT_SUPPORTED;
  this->irqMap[RADIOLIB_IRQ_HEADER_ERR] = RADIOLIB_IRQ_NOT_SUPPORTED;
  this->irqMap[RADIOLIB_IRQ_CRC_ERR] = RADIOLIB_IRQ_NOT_SUPPORTED;
  this->irqMap[RADIOLIB_IRQ_CAD_DONE] = RADIOLIB_IRQ_NOT_SUPPORTED;
  this->irqMap[RADIOLIB_IRQ_CAD_DETECTED] = RADIOLIB_IRQ_NOT_SUPPORTED;
  this->irqMap[RADIOLIB_IRQ_TIMEOUT] = RADIOLIB_SX127X_FLAG_TIMEOUT << 0;

  // try to find the SX127x chip
  if(!SX127x::findChip(chipVersions, numVersions)) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("No SX127x found!");
    this->mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  RADIOLIB_DEBUG_BASIC_PRINTLN("M\tSX127x");

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check currently active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    // set FSK mode
    state = setActiveModem(RADIOLIB_SX127X_FSK_OOK);
    RADIOLIB_ASSERT(state);
  }

  // enable/disable OOK
  state = setOOK(enableOOK);
  RADIOLIB_ASSERT(state);

  // set frequency deviation
  state = SX127x::setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  // set AFC bandwidth
  state = SX127x::setAFCBandwidth(rxBw);
  RADIOLIB_ASSERT(state);

  // set AFC&AGC trigger to RSSI (both in OOK and FSK)
  state = SX127x::setAFCAGCTrigger(RADIOLIB_SX127X_RX_TRIGGER_RSSI_INTERRUPT);
  RADIOLIB_ASSERT(state);

  // enable AFC
  state = SX127x::setAFC(false);
  RADIOLIB_ASSERT(state);

  // set receiver bandwidth
  state = SX127x::setRxBandwidth(rxBw);
  RADIOLIB_ASSERT(state);

  // set over current protection
  state = SX127x::setCurrentLimit(60);
  RADIOLIB_ASSERT(state);

  // set preamble length
  state = SX127x::setPreambleLength(preambleLength);
  RADIOLIB_ASSERT(state);

  // set preamble polarity
  state = invertPreamble(false);
  RADIOLIB_ASSERT(state);

  // set default sync word
  uint8_t syncWord[] = {0x12, 0xAD};
  state = setSyncWord(syncWord, 2);
  RADIOLIB_ASSERT(state);

  // disable address filtering
  state = disableAddressFiltering();
  RADIOLIB_ASSERT(state);

  // set default RSSI measurement config
  state = setRSSIConfig(2);
  RADIOLIB_ASSERT(state);

  // set default encoding
  state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  // set default packet length mode
  state = variablePacketLengthMode();

  return(state);
}

int16_t SX127x::transmit(const uint8_t* data, size_t len, uint8_t addr) {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  int16_t modem = getActiveModem();
  RadioLibTime_t start = 0;
  RadioLibTime_t timeout = 0;
  RadioLibTime_t toa = getTimeOnAir(len);
  if(modem == RADIOLIB_SX127X_LORA) {
    // calculate timeout in ms (150 % of expected time-on-air)
    timeout = (toa * 1.5) / 1000;

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // calculate timeout in ms (5ms + 500 % of expected time-on-air)
    timeout = 5 + (toa * 5) / 1000;

  } else {
    return(RADIOLIB_ERR_UNKNOWN);
  
  }

  // start transmission
  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu ms", timeout);
  state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  // wait for packet transmission or timeout
  start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    if(this->mod->hal->millis() - start > timeout) {
      finishTransmit();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }

  return(finishTransmit());
}

int16_t SX127x::receive(uint8_t* data, size_t len, RadioLibTime_t timeout) {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // calculate timeout based on the configured modem
  RadioLibTime_t timeoutInternal = timeout;
  uint32_t timeoutValue = 0;
  if(!timeoutInternal) {
    // calculate timeout (500 % of expected time-one-air)
    size_t maxLen = len;
    if(len == 0) { maxLen = RADIOLIB_SX127X_MAX_PACKET_LENGTH; }
    timeoutInternal = (getTimeOnAir(maxLen) * 5) / 1000;

    // convert to symbols
    float symbolLength = (float)(uint32_t(1) << this->spreadingFactor) / (float) this->bandwidth;
    timeoutValue = (float)timeoutInternal / symbolLength;
  }

  RADIOLIB_DEBUG_BASIC_PRINTLN("Timeout in %lu ms", timeoutInternal);

  // start reception
  state = startReceive(timeoutValue, RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RADIOLIB_IRQ_RX_DEFAULT_MASK, len);
  RADIOLIB_ASSERT(state);

  // wait for packet reception or timeout
  bool softTimeout = false;
  RadioLibTime_t start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    // check the blocking timeout
    if(this->mod->hal->millis() - start > timeoutInternal) {
      softTimeout = true;
      break;
    }
  }

  // check whether this was a timeout or not
  if(softTimeout || (getIRQFlags() & this->irqMap[RADIOLIB_IRQ_TIMEOUT])) {
    (void)finishReceive();
    return(RADIOLIB_ERR_RX_TIMEOUT);
  }

  // read the received data
  return(readData(data, len));
}

void SX127x::reset() {
  // should be implemented in derived class
}

int16_t SX127x::scanChannel() {
  // the configuration is not actually used
  const ChannelScanConfig_t cfg = { .rssi = { .limit = 0 } };
  return(scanChannel(cfg));
}

int16_t SX127x::scanChannel(const ChannelScanConfig_t &config) {
  (void)config;

  // start CAD
  int16_t state = startChannelScan();
  RADIOLIB_ASSERT(state);

  // wait for channel activity detected or timeout
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();
    if(this->mod->hal->digitalRead(this->mod->getGpio())) {
      return(RADIOLIB_PREAMBLE_DETECTED);
    }
  }

  return(RADIOLIB_CHANNEL_FREE);
}

int16_t SX127x::sleep() {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  // set mode to sleep
  int16_t state = setMode(RADIOLIB_SX127X_SLEEP);
  
  // wait for SX127x to safely enter sleep mode
  this->mod->hal->delay(1);

  return(state);
}

int16_t SX127x::standby() {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  // set mode to standby
  return(setMode(RADIOLIB_SX127X_STANDBY));
}

int16_t SX127x::standby(uint8_t mode) {
  (void)mode;
  return(standby());
}

int16_t SX127x::transmitDirect(uint32_t frf) {
  // check modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_TX);

  // user requested to start transmitting immediately (required for RTTY)
  if(frf != 0) {
    this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FRF_MSB, (frf & 0xFF0000) >> 16);
    this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FRF_MID, (frf & 0x00FF00) >> 8);
    this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FRF_LSB, frf & 0x0000FF);

    return(setMode(RADIOLIB_SX127X_TX));
  }

  // activate direct mode
  int16_t state = directMode();
  RADIOLIB_ASSERT(state);

  // apply fixes to errata
  RADIOLIB_ERRATA_SX127X(false);

  // start transmitting
  return(setMode(RADIOLIB_SX127X_TX));
}

int16_t SX127x::receiveDirect() {
  // check modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // activate direct mode
  int16_t state = directMode();
  RADIOLIB_ASSERT(state);

  // apply fixes to errata
  RADIOLIB_ERRATA_SX127X(true);

  // start receiving
  return(setMode(RADIOLIB_SX127X_RX));
}

int16_t SX127x::directMode() {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set DIO mapping
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO1_CONT_DCLK | RADIOLIB_SX127X_DIO2_CONT_DATA, 5, 2);
  RADIOLIB_ASSERT(state);

  // enable receiver startup without preamble or RSSI
  state = SX127x::setAFCAGCTrigger(RADIOLIB_SX127X_RX_TRIGGER_NONE);
  RADIOLIB_ASSERT(state);

  // set continuous mode
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_2, RADIOLIB_SX127X_DATA_MODE_CONTINUOUS, 6, 6));
}

int16_t SX127x::packetMode() {
  // check modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_2, RADIOLIB_SX127X_DATA_MODE_PACKET, 6, 6));
}

int16_t SX127x::startReceive() {
  return(this->startReceive(0, RADIOLIB_IRQ_RX_DEFAULT_FLAGS, RADIOLIB_IRQ_RX_DEFAULT_MASK, 0));
}

void SX127x::setDio0Action(void (*func)(void), uint32_t dir) {
  this->mod->hal->attachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()), func, dir);
}

void SX127x::clearDio0Action() {
  this->mod->hal->detachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()));
}

void SX127x::setDio1Action(void (*func)(void), uint32_t dir) {
  if(this->mod->getGpio() == RADIOLIB_NC) {
    return;
  }
  this->mod->hal->attachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getGpio()), func, dir);
}

void SX127x::clearDio1Action() {
  if(this->mod->getGpio() == RADIOLIB_NC) {
    return;
  }
  this->mod->hal->detachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getGpio()));
}

void SX127x::setPacketReceivedAction(void (*func)(void)) {
  this->setDio0Action(func, this->mod->hal->GpioInterruptRising);
}

void SX127x::clearPacketReceivedAction() {
  this->clearDio0Action();
}

void SX127x::setPacketSentAction(void (*func)(void)) {
  this->setDio0Action(func, this->mod->hal->GpioInterruptRising);
}

void SX127x::clearPacketSentAction() {
  this->clearDio0Action();
}

void SX127x::setChannelScanAction(void (*func)(void)) {
  this->setDio0Action(func, this->mod->hal->GpioInterruptRising);
}

void SX127x::clearChannelScanAction() {
  this->clearDio0Action();
}

void SX127x::setFifoEmptyAction(void (*func)(void)) {
  // set DIO1 to the FIFO empty event (the register setting is done in startTransmit)
  setDio1Action(func, this->mod->hal->GpioInterruptRising);
}

void SX127x::clearFifoEmptyAction() {
  clearDio1Action();
}

void SX127x::setFifoThreshold(uint8_t threshold) {
  this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_THRESH, threshold, 5, 0);
}

void SX127x::setFifoFullAction(void (*func)(void)) {
  // set the interrupt
  this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_THRESH, RADIOLIB_SX127X_FIFO_THRESH, 5, 0);
  this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO1_PACK_FIFO_LEVEL, 5, 4);

  // set DIO1 to the FIFO full event
  setDio1Action(func, this->mod->hal->GpioInterruptRising);
}

void SX127x::clearFifoFullAction() {
  clearDio1Action();
  this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, 0x00, 5, 4);
}

void SX127x::errataFix(bool rx) {
  (void)rx;
  // should be implemented in derived class
}

bool SX127x::fifoAdd(uint8_t* data, int totalLen, int* remLen) {
  // subtract first (this may be the first time we get to modify the remaining length)
  *remLen -= RADIOLIB_SX127X_FIFO_THRESH - 1;

  // check if there is still something left to send
  if(*remLen <= 0) {
    // we're done
    return(true);
  }

  // calculate the number of bytes we can copy
  int len = *remLen;
  if(len > RADIOLIB_SX127X_FIFO_THRESH - 1) {
    len = RADIOLIB_SX127X_FIFO_THRESH - 1;
  }

  // copy the bytes to the FIFO
  this->mod->SPIwriteRegisterBurst(RADIOLIB_SX127X_REG_FIFO, &data[totalLen - *remLen], len);

  // we're not done yet
  return(false);
}

bool SX127x::fifoGet(volatile uint8_t* data, int totalLen, volatile int* rcvLen) {
  // get pointer to the correct position in data buffer
  uint8_t* dataPtr = const_cast<uint8_t*>(&data[*rcvLen]);

  // check how much data are we still expecting
  uint8_t len = RADIOLIB_SX127X_FIFO_THRESH - 1;
  if(totalLen - *rcvLen < len) {
    // we're nearly at the end
    len = totalLen - *rcvLen;
  }

  // get the data
  this->mod->SPIreadRegisterBurst(RADIOLIB_SX127X_REG_FIFO, len, dataPtr);
  *rcvLen = *rcvLen + len;

  // check if we're done
  if(*rcvLen >= totalLen) {
    return(true);
  }
  return(false);
}

int16_t SX127x::finishTransmit() {
  // wait for at least 1 bit at the lowest possible bit rate before clearing IRQ flags
  // not doing this and clearing RADIOLIB_SX127X_FLAG_FIFO_OVERRUN will dump the FIFO,
  // which can lead to mangling of the last bit (#808)
  mod->hal->delayMicroseconds(1000000/1200);

  // clear interrupt flags
  clearIrqFlags(RADIOLIB_SX127X_FLAGS_ALL);

  // set mode to standby to disable transmitter/RF switch
  return(standby());
}

int16_t SX127x::readData(uint8_t* data, size_t len) {
  int16_t modem = getActiveModem();

  // get packet length
  size_t length = getPacketLength();
  size_t dumpLen = 0;
  if((len != 0) && (len < length)) {
    // user requested less data than we got, only return what was requested
    dumpLen = length - len;
    length = len;
  }

  // check payload CRC
  int16_t state = RADIOLIB_ERR_NONE;
  if(this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_IRQ_FLAGS, 5, 5) == RADIOLIB_SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
    state = RADIOLIB_ERR_CRC_MISMATCH;
  }

  if(modem == RADIOLIB_SX127X_LORA) {
    // check packet header integrity
    if(this->crcEnabled && (state == RADIOLIB_ERR_NONE)  && (this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_CHANNEL, 6, 6) == 0)) {
      // CRC is disabled according to packet header and enabled according to user
      // most likely damaged packet header
      state = RADIOLIB_ERR_LORA_HEADER_DAMAGED;
    } 
    // set FIFO read pointer to the start of the current packet
    int16_t addr = this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FIFO_RX_CURRENT_ADDR);
    if (addr >= 0) {
      this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_ADDR_PTR, addr);
    }

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // check address filtering
    uint8_t filter = this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, 2, 1);
    if((filter == RADIOLIB_SX127X_ADDRESS_FILTERING_NODE) || (filter == RADIOLIB_SX127X_ADDRESS_FILTERING_NODE_BROADCAST)) {
      this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_FIFO);
    }
  }

  // read packet data
  this->mod->SPIreadRegisterBurst(RADIOLIB_SX127X_REG_FIFO, length, data);

  // dump the bytes that weren't requested
  if(dumpLen != 0) {
    clearFIFO(dumpLen);
  }

  // clear internal flag so getPacketLength can return the new packet length
  this->packetLengthQueried = false;

  // clear interrupt flags
  clearIrqFlags(RADIOLIB_SX127X_FLAGS_ALL);

  return(state);
}

int16_t SX127x::finishReceive() {
  // set mode to standby to disable RF switch
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  return(clearIrqFlags(RADIOLIB_SX127X_FLAGS_ALL));
}

int16_t SX127x::startChannelScan() {
  // the configuration is not actually used
  const ChannelScanConfig_t cfg = { .rssi = { .limit = 0 } };
  return(startChannelScan(cfg));
}

int16_t SX127x::startChannelScan(const ChannelScanConfig_t &config) {
  (void)config;

  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // clear interrupt flags
  clearIrqFlags(RADIOLIB_SX127X_FLAGS_ALL);

  // set DIO pin mapping
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_LORA_CAD_DONE | RADIOLIB_SX127X_DIO1_LORA_CAD_DETECTED, 7, 4);
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // set mode to CAD
  state = setMode(RADIOLIB_SX127X_CAD);
  return(state);
}

int16_t SX127x::getChannelScanResult() {
  if((this->getIRQFlags() & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED) == RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED) {
    return(RADIOLIB_PREAMBLE_DETECTED);
  }
  return(RADIOLIB_CHANNEL_FREE);
}

int16_t SX127x::setSyncWord(uint8_t syncWord) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  setMode(RADIOLIB_SX127X_STANDBY);

  // write register
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_SYNC_WORD, syncWord));
}

int16_t SX127x::setCurrentLimit(uint8_t currentLimit) {
  // check allowed range
  if(!(((currentLimit >= 45) && (currentLimit <= 240)) || (currentLimit == 0))) {
    return(RADIOLIB_ERR_INVALID_CURRENT_LIMIT);
  }

  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);

  // set OCP limit
  uint8_t raw;
  if(currentLimit == 0) {
    // limit set to 0, disable OCP
    state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OCP, RADIOLIB_SX127X_OCP_OFF, 5, 5);
  } else if(currentLimit <= 120) {
    raw = (currentLimit - 45) / 5;
    state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OCP, RADIOLIB_SX127X_OCP_ON | raw, 5, 0);
  } else if(currentLimit <= 240) {
    raw = (currentLimit + 30) / 10;
    state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OCP, RADIOLIB_SX127X_OCP_ON | raw, 5, 0);
  }
  return(state);
}

int16_t SX127x::setPreambleLength(size_t preambleLength) {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // check active modem
  uint8_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    // check allowed range
    if(preambleLength < 6) {
      return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
    }

    // set preamble length
    state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_MSB, (uint8_t)((preambleLength >> 8) & 0xFF));
    state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_LSB, (uint8_t)(preambleLength & 0xFF));
    return(state);

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // set preamble length (in bytes)
    uint16_t numBytes = preambleLength / 8;
    state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_MSB_FSK, (uint8_t)((numBytes >> 8) & 0xFF));
    state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_LSB_FSK, (uint8_t)(numBytes & 0xFF));
    return(state);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX127x::invertPreamble(bool enable) {
  // set mode to standby
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // check active modem
  uint8_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // set preamble polarity
    uint8_t polarity = enable ? RADIOLIB_SX127X_PREAMBLE_POLARITY_AA : RADIOLIB_SX127X_PREAMBLE_POLARITY_55;
    state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_SYNC_CONFIG, polarity, 5, 5);
    return(state);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

float SX127x::getFrequencyError(bool autoCorrect) {
  int16_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    // get raw frequency error
    uint32_t raw = (uint32_t)this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FEI_MSB, 3, 0) << 16;
    raw |= (uint16_t)this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FEI_MID) << 8;
    raw |= this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FEI_LSB);

    uint32_t base = (uint32_t)2 << 23;
    float error;

    // check the first bit
    if(raw & 0x80000) {
      // frequency error is negative
      raw |= (uint32_t)0xFFF00000;
      raw = ~raw + 1;
      error = (((float)raw * (float)base)/32000000.0f) * (this->bandwidth/500.0f) * -1.0f;
    } else {
      error = (((float)raw * (float)base)/32000000.0f) * (this->bandwidth/500.0f);
    }

    if(autoCorrect) {
      // adjust LoRa modem data rate
      float ppmOffset = 0.95f * (error/32.0f);
      this->mod->SPIwriteRegister(0x27, (uint8_t)ppmOffset);
    }

    return(error);

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // get raw frequency error
    uint16_t raw = (uint16_t)this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FEI_MSB_FSK) << 8;
    raw |= this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_FEI_LSB_FSK);

    uint32_t base = 1;
    float error;

    // check the first bit
    if(raw & 0x8000) {
      // frequency error is negative
      raw |= (uint32_t)0xFFF00000;
      raw = ~raw + 1;
      error = (float)raw * (32000000.0f / (float)(base << 19)) * -1.0f;
    } else {
      error = (float)raw * (32000000.0f / (float)(base << 19));
    }

    return(error);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

float SX127x::getAFCError()
{
  // check active modem
  int16_t modem = getActiveModem();
  if(modem != RADIOLIB_SX127X_FSK_OOK) {
    return 0;
  }

  // get raw frequency error
  int16_t raw = (uint16_t)this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_AFC_MSB) << 8;
  raw |= this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_AFC_LSB);

  uint32_t base = 1;
  return raw * (32000000.0f / (float)(base << 19));
}

float SX127x::getSNR() {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(0);
  }

  // get SNR value
  int8_t rawSNR = (int8_t)this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PKT_SNR_VALUE);
  return(rawSNR / 4.0);
}

int16_t SX127x::setBitRateCommon(float br, uint8_t fracRegAddr) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check allowed bit rate
  // datasheet says 1.2 kbps should be the smallest possible, but 0.512 works fine
  if(ookEnabled) {
    RADIOLIB_CHECK_RANGE(br, 0.5f, 32.768002f, RADIOLIB_ERR_INVALID_BIT_RATE);      // Found that 32.768 is 32.768002
  } else {
    RADIOLIB_CHECK_RANGE(br, 0.5f, 300.0f, RADIOLIB_ERR_INVALID_BIT_RATE);
  }

  // set mode to STANDBY
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set bit rate
  uint16_t bitRateRaw = (RADIOLIB_SX127X_CRYSTAL_FREQ * 1000.0f) / br;
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_BITRATE_MSB, (bitRateRaw & 0xFF00) >> 8, 7, 0);
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_BITRATE_LSB, bitRateRaw & 0x00FF, 7, 0);

  // set fractional part of bit rate
  if(!ookEnabled) {
    float bitRateRem = ((RADIOLIB_SX127X_CRYSTAL_FREQ * 1000.0f) / br) - (float)bitRateRaw;
    uint8_t bitRateFrac = bitRateRem * 16;
    state |= this->mod->SPIsetRegValue(fracRegAddr, bitRateFrac, 7, 0);
  }

  if(state == RADIOLIB_ERR_NONE) {
    this->bitRate = br;
  }
  return(state);
}

int16_t SX127x::setFrequencyDeviation(float freqDev) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0f) {
    newFreqDev = 0.6f;
  }

  // check frequency deviation range
  if(!((newFreqDev + this->bitRate/2.0f <= 250.0f) && (freqDev <= 200.0f))) {
    return(RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
  }

  this->frequencyDev = newFreqDev;

  // set mode to STANDBY
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set allowed frequency deviation
  uint32_t base = 1;
  uint32_t FDEV = (newFreqDev * (base << 19)) / 32000;
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FDEV_MSB, (FDEV & 0xFF00) >> 8, 5, 0);
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FDEV_LSB, FDEV & 0x00FF, 7, 0);
  return(state);
}

uint8_t SX127x::calculateBWManExp(float bandwidth)
{
  for(uint8_t e = 7; e >= 1; e--) {
    for(int8_t m = 2; m >= 0; m--) {
      float point = (RADIOLIB_SX127X_CRYSTAL_FREQ * 1000000.0f)/(((4 * m) + 16) * ((uint32_t)1 << (e + 2)));
      if(fabsf(bandwidth - ((point / 1000.0f) + 0.05f)) <= 0.5f) {
        return((m << 3) | e);
      }
    }
  }
  return 0;
}

int16_t SX127x::setRxBandwidth(float rxBw) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(rxBw, 2.6f, 250.0f, RADIOLIB_ERR_INVALID_RX_BANDWIDTH);

  // set mode to STANDBY
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set Rx bandwidth
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_BW, calculateBWManExp(rxBw), 4, 0));
}

int16_t SX127x::setAFCBandwidth(float rxBw) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK){
      return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(rxBw, 2.6f, 250.0f, RADIOLIB_ERR_INVALID_RX_BANDWIDTH);

  // set mode to STANDBY
  int16_t state = setMode(RADIOLIB_SX127X_STANDBY);
  RADIOLIB_ASSERT(state);

  // set AFC bandwidth
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_AFC_BW, calculateBWManExp(rxBw), 4, 0));
}

int16_t SX127x::setAFC(bool isEnabled) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  //set AFC auto on/off
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_CONFIG, isEnabled ? RADIOLIB_SX127X_AFC_AUTO_ON : RADIOLIB_SX127X_AFC_AUTO_OFF, 4, 4));
}

int16_t SX127x::setAFCAGCTrigger(uint8_t trigger) {
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  //set AFC&AGC trigger
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_CONFIG, trigger, 2, 0));
}

int16_t SX127x::setSyncWord(uint8_t* syncWord, size_t len) {
  // check active modem
  uint8_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_FSK_OOK) {

    // disable sync word in case len is 0
    if(len == 0) {
      int16_t state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_SYNC_CONFIG, RADIOLIB_SX127X_SYNC_OFF, 4, 4);
      return(state);
    }

    RADIOLIB_CHECK_RANGE(len, 1, 8, RADIOLIB_ERR_INVALID_SYNC_WORD);

    // sync word must not contain value 0x00
    for(size_t i = 0; i < len; i++) {
      if(syncWord[i] == 0x00) {
        return(RADIOLIB_ERR_INVALID_SYNC_WORD);
      }
    }

    // enable sync word recognition
    int16_t state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_SYNC_CONFIG, RADIOLIB_SX127X_SYNC_ON, 4, 4);
    state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_SYNC_CONFIG, len - 1, 2, 0);
    RADIOLIB_ASSERT(state);

    // set sync word
    this->mod->SPIwriteRegisterBurst(RADIOLIB_SX127X_REG_SYNC_VALUE_1, syncWord, len);
    return(RADIOLIB_ERR_NONE);
  
  } else if(modem == RADIOLIB_SX127X_LORA) {
    // with length set to 1 and LoRa modem active, assume it is the LoRa sync word
    if(len > 1) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }

    return(this->setSyncWord(syncWord[0]));
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX127x::setNodeAddress(uint8_t nodeAddr) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node only)
  int16_t state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_ADDRESS_FILTERING_NODE, 2, 1);
  RADIOLIB_ASSERT(state);

  // set node address
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_NODE_ADRS, nodeAddr));
}

int16_t SX127x::setBroadcastAddress(uint8_t broadAddr) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node + broadcast)
  int16_t state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_ADDRESS_FILTERING_NODE_BROADCAST, 2, 1);
  RADIOLIB_ASSERT(state);

  // set broadcast address
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_BROADCAST_ADRS, broadAddr));
}

int16_t SX127x::disableAddressFiltering() {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // disable address filtering
  int16_t state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_ADDRESS_FILTERING_OFF, 2, 1);
  RADIOLIB_ASSERT(state);

  // set node address to default (0x00)
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_NODE_ADRS, 0x00);
  RADIOLIB_ASSERT(state);

  // set broadcast address to default (0x00)
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_BROADCAST_ADRS, 0x00));
}

int16_t SX127x::setOokThresholdType(uint8_t type) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_PEAK, type, 4, 3, 5));
}

int16_t SX127x::setOokFixedOrFloorThreshold(uint8_t value) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_FIX, value, 7, 0, 5));
}

int16_t SX127x::setOokPeakThresholdDecrement(uint8_t value) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_AVG, value, 7, 5, 5));
}

int16_t SX127x::setOokPeakThresholdStep(uint8_t value) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_PEAK, value, 2, 0, 5));
}

int16_t SX127x::enableBitSync() {
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_PEAK, RADIOLIB_SX127X_BIT_SYNC_ON, 5, 5, 5));
}

int16_t SX127x::disableBitSync() {
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OOK_PEAK, RADIOLIB_SX127X_BIT_SYNC_OFF, 5, 5, 5));
}

int16_t SX127x::setOOK(bool enableOOK) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set OOK and if successful, save the new setting
  int16_t state = RADIOLIB_ERR_NONE;
  if(enableOOK) {
    state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, RADIOLIB_SX127X_MODULATION_OOK, 6, 5, 5);
    state |= SX127x::setAFCAGCTrigger(RADIOLIB_SX127X_RX_TRIGGER_RSSI_INTERRUPT);
  } else {
    state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, RADIOLIB_SX127X_MODULATION_FSK, 6, 5, 5);
    state |= SX127x::setAFCAGCTrigger(RADIOLIB_SX127X_RX_TRIGGER_BOTH);
  }
  if(state == RADIOLIB_ERR_NONE) {
    ookEnabled = enableOOK;
  }

  return(state);
}

int16_t SX127x::setFrequencyRaw(float newFreq) {
  int16_t state = RADIOLIB_ERR_NONE;

  // set mode to standby if not FHSS
  if(this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD) == RADIOLIB_SX127X_HOP_PERIOD_OFF) {
    state = setMode(RADIOLIB_SX127X_STANDBY);
  }

  // calculate register values
  uint32_t FRF = (newFreq * (uint32_t(1) << RADIOLIB_SX127X_DIV_EXPONENT)) / RADIOLIB_SX127X_CRYSTAL_FREQ;

  // write registers
  // lsb needs to be written no matter what in order for the module to update the frequency
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FRF_MSB, (FRF & 0xFF0000) >> 16);
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FRF_MID, (FRF & 0x00FF00) >> 8);
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FRF_LSB, FRF & 0x0000FF, 7U, 0U, 2U, 0xFF, true);
  return(state);
}

size_t SX127x::getPacketLength(bool update) {
  int16_t modem = getActiveModem();

  if(modem == RADIOLIB_SX127X_LORA) {
    if(!this->implicitHdr) {
      // get packet length for explicit header mode
      return(this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_RX_NB_BYTES));

    } else {
      // return the cached value for implicit header mode
      return(this->packetLength);
    }

  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    // get packet length
    if(!this->packetLengthQueried && update) {
      if (this->packetLengthConfig == RADIOLIB_SX127X_PACKET_VARIABLE) {
        this->packetLength = this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_FIFO);
      } else {
        this->packetLength = this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_PAYLOAD_LENGTH_FSK);
      }
      this->packetLengthQueried = true;
    }
  }

  return(this->packetLength);
}

int16_t SX127x::getLoRaRxHeaderInfo(uint8_t* cr, bool* hasCRC) {
  int16_t state = RADIOLIB_ERR_NONE;

  // check if in explicit header mode
  if(this->implicitHdr) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(cr) { *cr = this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_MODEM_STAT, 7, 5) >> 5; }
  if(hasCRC) { *hasCRC = this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_CHANNEL, 6, 6) != 0; }

  return(state);
}

int16_t SX127x::fixedPacketLengthMode(uint8_t len) {
  return(SX127x::setPacketMode(RADIOLIB_SX127X_PACKET_FIXED, len));
}

int16_t SX127x::variablePacketLengthMode(uint8_t maxLen) {
  return(SX127x::setPacketMode(RADIOLIB_SX127X_PACKET_VARIABLE, maxLen));
}

float SX127x::getNumSymbols(size_t len, DataRate_t dr, PacketConfig_t pc) {
  // get Low Data Rate optimization flag
  float de = pc.lora.ldrOptimize ? 1.0f : 0.0f;

  // get explicit/implicit header enabled flag
  float ih = (float) pc.lora.implicitHeader;

  // get CRC enabled flag
  float crc = (float) pc.lora.crcEnabled;

  // get number of preamble symbols
  float n_pre = (float) pc.lora.preambleLength;

  // get number of payload symbols
  float n_pay = 8.0f + RADIOLIB_MAX(ceilf((8.0f * (float) len - 4.0f * (float) dr.lora.spreadingFactor + 28.0f + 16.0f * crc - 20.0f * ih) / (4.0f * (float) dr.lora.spreadingFactor - 8.0f * de)) * (float) dr.lora.codingRate, 0.0f);

  // add 4.25 symbols for the sync
  return(n_pre + n_pay + 4.25f);
}

RadioLibTime_t SX127x::calculateTimeOnAir(ModemType_t modem, DataRate_t dr, PacketConfig_t pc, size_t len) {
  if (modem == RADIOLIB_MODEM_LORA) {
    // get symbol length in us
    float symbolLength = (float) (uint32_t(1) << dr.lora.spreadingFactor) / (float) dr.lora.bandwidth;

    // get number of symbols
    float n_sym = getNumSymbols(len, dr, pc);

    // get time-on-air in us
    return ceil((double)symbolLength * (double)n_sym) * 1000;

  } else if(modem == RADIOLIB_MODEM_FSK) {
    
    // calculate time-on-air in us {[(length in bytes) * (8 bits / 1 byte)] / [(Bit Rate in kbps) * (1000 bps / 1 kbps)]} * (1000000 us in 1 sec)
    return((uint32_t) ((((float)(pc.fsk.crcLength * 8) + pc.fsk.syncWordLength + pc.fsk.preambleLength + (float) (len * 8)) / (dr.fsk.bitRate * 1000.0f)) * 1000000.0f));
  } else {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  
  return(RADIOLIB_ERR_UNKNOWN);
}

RadioLibTime_t SX127x::getTimeOnAir(size_t len) {
  uint8_t modem = getActiveModem();
  DataRate_t dr = {};
  PacketConfig_t pc = {};

  switch (modem) {
    case(RADIOLIB_SX127X_LORA): {
      dr.lora.spreadingFactor = this->spreadingFactor;
      dr.lora.bandwidth = this->bandwidth;
      dr.lora.codingRate = this->codingRate;

      // Get number of preamble symbols
      uint16_t n_pre = ((this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_MSB) << 8) | this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_LSB));

      pc.lora.preambleLength = n_pre;
      pc.lora.implicitHeader = this->implicitHdr;
      pc.lora.crcEnabled = this->crcEnabled;
      pc.lora.ldrOptimize = this->ldroEnabled;

      return(calculateTimeOnAir((ModemType_t)RADIOLIB_MODEM_LORA, dr, pc, len));
    }
    case(RADIOLIB_SX127X_FSK_OOK): {
      dr.fsk.bitRate = this->bitRate;
      dr.fsk.freqDev = this->frequencyDev;

      // get number of bits preamble
      uint16_t n_pre = (uint16_t) ((this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_MSB_FSK) << 8) | this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_LSB_FSK)) * 8;
      // get the number of bits of the sync word
      uint8_t n_syncWord = (uint8_t) ((this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_SYNC_CONFIG, 2, 0) + 1) * 8);
      // get CRC enabled status
      bool crcEn = (this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, 4, 4) == RADIOLIB_SX127X_CRC_ON);

      pc.fsk.preambleLength = n_pre;
      pc.fsk.syncWordLength = n_syncWord;
      pc.fsk.crcLength = (uint8_t)(crcEn * 2);

      if (this->packetLengthConfig == RADIOLIB_SX127X_PACKET_FIXED) {
        // if packet size fixed -> len = fixed packet length
        len = this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PAYLOAD_LENGTH_FSK);
      } else {
        // if packet variable -> Add 1 extra byte for payload length
        len += 1;
      }

      return(calculateTimeOnAir((ModemType_t)RADIOLIB_MODEM_FSK, dr, pc, len));
    }
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
}

RadioLibTime_t SX127x::calculateRxTimeout(RadioLibTime_t timeoutUs) {
  RadioLibTime_t timeout = 0;
  if(getActiveModem() == RADIOLIB_SX127X_LORA) {
    // for LoRa, the timeout is given as the number of symbols
    // the calling function should provide some extra width, as this number of symbols is truncated to integer
    // the order of operators is swapped here to decrease the effects of this truncation error
    float symbolLength = (float) (uint32_t(1) << this->spreadingFactor) / (float) this->bandwidth;
    timeout = (timeoutUs / symbolLength) / 1000;
  
  } else {
    // for FSK, the timeout is in units of 16x bit time
    timeout = ((float)timeoutUs / ((16.0f * 1000.0f) / this->bitRate));
  
  }

  return(timeout);
}

uint32_t SX127x::getIrqFlags() {
  return((uint32_t)this->getIRQFlags());
}

int16_t SX127x::setIrqFlags(uint32_t irq) {
  // this is a bit convoluted, but unfortunately SX127x IRQ flags are not used to enable/disable that IRQ ...
  // in addition, the configuration is often mutually exclusive, so we iterate over the set bits in a loop
  uint8_t usedPinFlags = 0;
  bool conflict = false;
  int16_t modem = getActiveModem();
  int16_t state = RADIOLIB_ERR_NONE;
  for(uint8_t i = 0; i <= 31; i++) {
    // check if the bit is set
    uint32_t irqBit = irq & (1UL << i);
    if(!irqBit) {
      continue;
    }

    // if not, decode it
    uint8_t dioNum = 0; // DIO pin number and register value to set (address and MSB/LSB can be inferred)
    uint8_t regVal = 0;
    if(modem == RADIOLIB_SX127X_LORA) {
      switch(irqBit) {
        case(RADIOLIB_SX127X_CLEAR_IRQ_FLAG_TX_DONE):
          dioNum = 0;
          regVal = RADIOLIB_SX127X_DIO0_PACK_PACKET_SENT;
          break;
        case(RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_DONE):
          dioNum = 0;
          regVal = RADIOLIB_SX127X_DIO0_LORA_RX_DONE;
          break;
        case(RADIOLIB_SX127X_CLEAR_IRQ_FLAG_VALID_HEADER):
          dioNum = 3;
          regVal = RADIOLIB_SX127X_DIO3_LORA_VALID_HEADER;
          break;
        case(RADIOLIB_SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR):
          dioNum = 3;
          regVal = RADIOLIB_SX127X_DIO3_LORA_PAYLOAD_CRC_ERROR;
          break;
        case(RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DONE):
          dioNum = 0;
          regVal = RADIOLIB_SX127X_DIO0_LORA_CAD_DONE;
          break;
        case(RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED):
          dioNum = 1;
          regVal = RADIOLIB_SX127X_DIO1_LORA_CAD_DETECTED;
          break;
        case(RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_TIMEOUT):
          dioNum = 1;
          regVal = RADIOLIB_SX127X_DIO1_LORA_RX_TIMEOUT;
          break;
        default:
          return(RADIOLIB_ERR_UNSUPPORTED);
      }
    
    } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
      switch(irqBit) {
        case(RADIOLIB_SX127X_FLAG_PACKET_SENT << 8):
          dioNum = 0;
          regVal = RADIOLIB_SX127X_DIO0_PACK_PACKET_SENT;
          break;
        case(RADIOLIB_SX127X_FLAG_PAYLOAD_READY << 8):
          dioNum = 0;
          regVal = RADIOLIB_SX127X_DIO0_PACK_PAYLOAD_READY;
          break;
        case(RADIOLIB_SX127X_FLAG_PREAMBLE_DETECT << 0):
          dioNum = 4;
          regVal = RADIOLIB_SX127X_DIO4_PACK_RSSI_PREAMBLE_DETECT;
          break;
        case(RADIOLIB_SX127X_FLAG_SYNC_ADDRESS_MATCH << 0):
          dioNum = 2;
          regVal = RADIOLIB_SX127X_DIO2_PACK_SYNC_ADDRESS;
          break;
        case(RADIOLIB_SX127X_FLAG_TIMEOUT << 0):
          dioNum = 2;
          regVal = RADIOLIB_SX127X_DIO2_PACK_TIMEOUT;
          break;
        default:
          return(RADIOLIB_ERR_UNSUPPORTED);
      }
    }

    // check if this DIO pin has been set already
    if(usedPinFlags & (1UL << dioNum)) {
      // uh oh, this pin is used!
      RADIOLIB_DEBUG_PRINTLN("Unable to set IRQ %04x on DIO%d due to conflict!", irqBit, (int)dioNum);
      conflict = true;
      continue;
    }

    // DIO pin is unused, set the flag and configure it
    usedPinFlags |= (1UL << dioNum);
    uint8_t addr = (dioNum > 3) ? RADIOLIB_SX127X_REG_DIO_MAPPING_2 : RADIOLIB_SX127X_REG_DIO_MAPPING_1;
    uint8_t msb = 7 - 2*(dioNum % 4);
    state = this->mod->SPIsetRegValue(addr, regVal, msb, msb - 1);
    RADIOLIB_ASSERT(state);
  }

  // if there was at least one conflict, this flag is set
  if(conflict) {
    return(RADIOLIB_ERR_INVALID_IRQ);
  }

  return(state);
}

int16_t SX127x::clearIrqFlags(uint32_t irq) {
  int16_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS, (uint8_t)irq);
    return(RADIOLIB_ERR_NONE);
  
  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS_1, (uint8_t)irq);
    this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS_2, (uint8_t)(irq >> 8));
    return(RADIOLIB_ERR_NONE);
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX127x::setCrcFiltering(bool enable) {
  this->crcOn = enable;

  if (enable == true) {
    return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_CRC_ON, 4, 4));
  } else {
    return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_CRC_OFF, 4, 4));
  }
}

int16_t SX127x::setRSSIThreshold(float dbm) {
  RADIOLIB_CHECK_RANGE(dbm, -127.5f, 0.0f, RADIOLIB_ERR_INVALID_RSSI_THRESHOLD);

  return this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RSSI_THRESH, (uint8_t)(-2.0f * dbm), 7, 0);
}

int16_t SX127x::setRSSIConfig(uint8_t smoothingSamples, int8_t offset) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // check provided values
  if(!(smoothingSamples <= 7)) {
    return(RADIOLIB_ERR_INVALID_NUM_SAMPLES);
  }

  RADIOLIB_CHECK_RANGE(offset, -16, 15, RADIOLIB_ERR_INVALID_RSSI_OFFSET);

  // calculate the two's complement
  uint8_t offsetRaw = RADIOLIB_ABS(offset);
  offsetRaw ^= 0x1F;
  offsetRaw += 1;
  offsetRaw &= 0x1F;

  // set new register values
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RSSI_CONFIG, offsetRaw << 3, 7, 3);
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RSSI_CONFIG, smoothingSamples, 2, 0);
  return(state);
}

int16_t SX127x::setEncoding(uint8_t encoding) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set encoding
  switch(encoding) {
    case RADIOLIB_ENCODING_NRZ:
      return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_DC_FREE_NONE, 6, 5));
    case RADIOLIB_ENCODING_MANCHESTER:
      return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_DC_FREE_MANCHESTER, 6, 5));
    case RADIOLIB_ENCODING_WHITENING:
      return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_DC_FREE_WHITENING, 6, 5));
    default:
      return(RADIOLIB_ERR_INVALID_ENCODING);
  }
}

uint16_t SX127x::getIRQFlags() {
  // check active modem
  if(getActiveModem() == RADIOLIB_SX127X_LORA) {
    // LoRa, just 8-bit value
    return((uint16_t)this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS));

  } else {
    // FSK, the IRQ flags are 16 bits in total
    uint16_t flags = ((uint16_t)this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS_2)) << 8;
    flags |= (uint16_t)this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS_1);
    return(flags);
  }

}

uint8_t SX127x::getModemStatus() {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(0x00);
  }

  // read the register
  return(this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_MODEM_STAT));
}

void SX127x::setRfSwitchPins(uint32_t rxEn, uint32_t txEn) {
  this->mod->setRfSwitchPins(rxEn, txEn);
}

void SX127x::setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]) {
  this->mod->setRfSwitchTable(pins, table);
}

uint8_t SX127x::randomByte() {
  // check active modem
  uint8_t rssiValueReg = RADIOLIB_SX127X_REG_RSSI_WIDEBAND;
  if(getActiveModem() == RADIOLIB_SX127X_FSK_OOK) {
    rssiValueReg = RADIOLIB_SX127X_REG_RSSI_VALUE_FSK;
  }

  // set mode to Rx
  setMode(RADIOLIB_SX127X_RX);

  // wait a bit for the RSSI reading to stabilise
  this->mod->hal->delay(10);

  // read RSSI value 8 times, always keep just the least significant bit
  uint8_t randByte = 0x00;
  for(uint8_t i = 0; i < 8; i++) {
    randByte |= ((this->mod->SPIreadRegister(rssiValueReg) & 0x01) << i);
  }

  // set mode to standby
  setMode(RADIOLIB_SX127X_STANDBY);

  return(randByte);
}

int16_t SX127x::getChipVersion() {
  return(this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_VERSION));
}

int8_t SX127x::getTempRaw() {
  int8_t temp = 0;
  uint8_t previousOpMode;
  uint8_t ival;

  // save current Op Mode
  previousOpMode = this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_OP_MODE);

  // check if we need to step out of LoRa mode first
  if((previousOpMode & RADIOLIB_SX127X_LORA) == RADIOLIB_SX127X_LORA) {
    this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, (RADIOLIB_SX127X_LORA | RADIOLIB_SX127X_SLEEP));
  }

  // put device in FSK sleep
  this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, (RADIOLIB_SX127X_FSK_OOK | RADIOLIB_SX127X_SLEEP));

  // put device in FSK RxSynth
  this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, (RADIOLIB_SX127X_FSK_OOK | RADIOLIB_SX127X_FSRX));

  // enable temperature reading
  this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_IMAGE_CAL, RADIOLIB_SX127X_TEMP_MONITOR_ON, 0, 0);

  // wait
  this->mod->hal->delayMicroseconds(200);

  // disable temperature reading
  this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_IMAGE_CAL, RADIOLIB_SX127X_TEMP_MONITOR_OFF, 0, 0);

  // put device in FSK sleep
  this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, (RADIOLIB_SX127X_FSK_OOK | RADIOLIB_SX127X_SLEEP));

  // read temperature
  ival = this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_TEMP);

  // convert very raw value
  if((ival & 0x80) == 0x80) {
    temp = 255 - ival;
  } else {
    temp = -1 * ival;
  }

  // check if we need to step back into LoRa mode
  if((previousOpMode & RADIOLIB_SX127X_LORA) == RADIOLIB_SX127X_LORA) {
    this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, (RADIOLIB_SX127X_LORA | RADIOLIB_SX127X_SLEEP));
  }

  // reload previous Op Mode
  this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, previousOpMode);

  return(temp);
}

Module* SX127x::getMod() {
  return(this->mod);
}

int16_t SX127x::config() {
  // turn off frequency hopping
  int16_t state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD, RADIOLIB_SX127X_HOP_PERIOD_OFF);
  return(state);
}

int16_t SX127x::configFSK() {
  // set RSSI threshold
  int16_t state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RSSI_THRESH, RADIOLIB_SX127X_RSSI_THRESHOLD);
  RADIOLIB_ASSERT(state);

  // reset FIFO flag
  this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS_2, RADIOLIB_SX127X_FLAG_FIFO_OVERRUN);

  // set packet configuration
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, RADIOLIB_SX127X_PACKET_VARIABLE | RADIOLIB_SX127X_DC_FREE_NONE | RADIOLIB_SX127X_CRC_ON | RADIOLIB_SX127X_CRC_AUTOCLEAR_ON | RADIOLIB_SX127X_ADDRESS_FILTERING_OFF | RADIOLIB_SX127X_CRC_WHITENING_TYPE_CCITT, 7, 0);
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_2, RADIOLIB_SX127X_DATA_MODE_PACKET | RADIOLIB_SX127X_IO_HOME_OFF, 6, 5);
  RADIOLIB_ASSERT(state);

  // set FIFO threshold
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_THRESH, RADIOLIB_SX127X_TX_START_FIFO_NOT_EMPTY, 7, 7);
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_THRESH, RADIOLIB_SX127X_FIFO_THRESH, 5, 0);
  RADIOLIB_ASSERT(state);

  // disable Rx timeouts
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_TIMEOUT_1, RADIOLIB_SX127X_TIMEOUT_RX_RSSI_OFF);
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_TIMEOUT_2, RADIOLIB_SX127X_TIMEOUT_RX_PREAMBLE_OFF);
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_TIMEOUT_3, RADIOLIB_SX127X_TIMEOUT_SIGNAL_SYNC_OFF);
  RADIOLIB_ASSERT(state);

  // enable preamble detector
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PREAMBLE_DETECT, RADIOLIB_SX127X_PREAMBLE_DETECTOR_ON | RADIOLIB_SX127X_PREAMBLE_DETECTOR_2_BYTE | RADIOLIB_SX127X_PREAMBLE_DETECTOR_TOL);

  return(state);
}

int16_t SX127x::setPacketMode(uint8_t mode, uint8_t len) {
  // check packet length
  if(len > RADIOLIB_SX127X_MAX_PACKET_LENGTH_FSK) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_FSK_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set to fixed packet length
  int16_t state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, mode, 7, 7);
  RADIOLIB_ASSERT(state);

  // set length to register
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PAYLOAD_LENGTH_FSK, len);
  RADIOLIB_ASSERT(state);

  // update cached value
  this->packetLengthConfig = mode;
  return(state);
}

bool SX127x::findChip(const uint8_t* vers, uint8_t num) {
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    // reset the module
    reset();

    // check version register
    int16_t version = getChipVersion();
    for(uint8_t j = 0; j < num; j++) {
      if(version == vers[j]) {
        flagFound = true;
        break;
      }
    }

    if(!flagFound) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("SX127x not found! (%d of 10 tries) RADIOLIB_SX127X_REG_VERSION == 0x%04X", i + 1, version);
      this->mod->hal->delay(10);
      i++;
    }
  
  }

  return(flagFound);
}

int16_t SX127x::setMode(uint8_t mode) {
  uint8_t checkMask = 0xFF;
  if((getActiveModem() == RADIOLIB_SX127X_FSK_OOK) && (mode == RADIOLIB_SX127X_RX)) {
    // disable checking of RX bit in FSK RX mode, as it sometimes seem to fail (#276)
    checkMask = 0xFE;
  }
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, mode, 2, 0, 5, checkMask));
}

int16_t SX127x::getActiveModem() {
  return(this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_OP_MODE, 7, 7));
}

int16_t SX127x::setActiveModem(uint8_t modem) {
  // set mode to SLEEP
  int16_t state = setMode(RADIOLIB_SX127X_SLEEP);

  // set modem
  // low frequency access (bit 3) automatically resets when switching modem
  // so we exclude it from the check 
  state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_OP_MODE, modem, 7, 7, 5, 0xF7);

  // set mode to STANDBY
  state |= setMode(RADIOLIB_SX127X_STANDBY);
  return(state);
}

void SX127x::clearFIFO(size_t count) {
  while(count) {
    this->mod->SPIreadRegister(RADIOLIB_SX127X_REG_FIFO);
    count--;
  }
}

int16_t SX127x::invertIQ(bool enable) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // Tx path inversion is swapped, because it seems that setting it according to the datsheet
  // will actually lead to the wrong inversion. See https://github.com/jgromes/RadioLib/issues/778
  int16_t state;
  if(enable) {
    state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ, RADIOLIB_SX127X_INVERT_IQ_RXPATH_ON, 6, 6);
    state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ, RADIOLIB_SX127X_INVERT_IQ_TXPATH_OFF, 0, 0);
    state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ2, RADIOLIB_SX127X_IQ2_ENABLE);
  } else {
    state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ, RADIOLIB_SX127X_INVERT_IQ_RXPATH_OFF, 6, 6);
    state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ, RADIOLIB_SX127X_INVERT_IQ_TXPATH_ON, 0, 0);
    state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_INVERT_IQ2, RADIOLIB_SX127X_IQ2_DISABLE);
  }

  return(state);
}

int16_t SX127x::getModem(ModemType_t* modem) {
  RADIOLIB_ASSERT_PTR(modem);

  int16_t packetType = getActiveModem();
  switch(packetType) {
    case(RADIOLIB_SX127X_LORA):
      *modem = ModemType_t::RADIOLIB_MODEM_LORA;
      return(RADIOLIB_ERR_NONE);
    case(RADIOLIB_SX127X_FSK_OOK):
      *modem = ModemType_t::RADIOLIB_MODEM_FSK;
      return(RADIOLIB_ERR_NONE);
  }
  
  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX127x::stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg) {
  int16_t state;

  switch(mode) {
    case(RADIOLIB_RADIO_MODE_RX): {
      this->rxMode = RADIOLIB_SX127X_RXCONTINUOUS;

      // set mode to standby
      state = setMode(RADIOLIB_SX127X_STANDBY);
      RADIOLIB_ASSERT(state);

      // set DIO pin mapping
      state = this->setIrqFlags(getIrqMapped(cfg->receive.irqFlags & cfg->receive.irqMask));
      RADIOLIB_ASSERT(state);

      int16_t modem = getActiveModem();
      if(modem == RADIOLIB_SX127X_LORA) {
        // if max(uint32_t) is used, revert to RxContinuous
        if(cfg->receive.timeout == 0xFFFFFFFF) {
          cfg->receive.timeout = 0;
        }
        if(cfg->receive.timeout != 0) {
          // for non-zero timeout value, change mode to Rx single and set the timeout
          this->rxMode = RADIOLIB_SX127X_RXSINGLE;
          uint8_t msb_sym = (cfg->receive.timeout > 0x3FF) ? 0x3 : (uint8_t)(cfg->receive.timeout >> 8);
          uint8_t lsb_sym = (cfg->receive.timeout > 0x3FF) ? 0xFF : (uint8_t)(cfg->receive.timeout & 0xFF);
          state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_MODEM_CONFIG_2, msb_sym, 1, 0);
          state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_SYMB_TIMEOUT_LSB, lsb_sym);
          RADIOLIB_ASSERT(state);
        }

        // in FHSS mode, enable channel change interrupt
        if(this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD) > RADIOLIB_SX127X_HOP_PERIOD_OFF) {
          state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO1_LORA_FHSS_CHANGE_CHANNEL, 5, 4);
        }

        // in implicit header mode, use the provided length if it is nonzero
        // otherwise we trust the user has previously set the payload length manually
        if((this->implicitHdr) && (cfg->receive.len != 0)) {
          state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PAYLOAD_LENGTH, cfg->receive.len);
          this->packetLength = cfg->receive.len;
        }

        // apply fixes to errata
        RADIOLIB_ERRATA_SX127X(true);

        // clear interrupt flags
        clearIrqFlags(RADIOLIB_SX127X_FLAGS_ALL);

        // set FIFO pointers
        state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_RX_BASE_ADDR, RADIOLIB_SX127X_FIFO_RX_BASE_ADDR_MAX);
        state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_ADDR_PTR, RADIOLIB_SX127X_FIFO_RX_BASE_ADDR_MAX);
        RADIOLIB_ASSERT(state);

      } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
        // for non-zero timeout value, emulate timeout
        state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_RX_TIMEOUT_3, cfg->receive.timeout & 0xFF);
        RADIOLIB_ASSERT(state);

        // clear interrupt flags
        clearIrqFlags(RADIOLIB_SX127X_FLAGS_ALL);

        // FSK modem does not actually distinguish between Rx single and continuous mode,
        // Rx single is emulated using timeout
        this->rxMode = RADIOLIB_SX127X_RX;
      }
    } break;
  
    case(RADIOLIB_RADIO_MODE_TX): {
      // set mode to standby
      state = setMode(RADIOLIB_SX127X_STANDBY);

      int16_t modem = getActiveModem();
      if(modem == RADIOLIB_SX127X_LORA) {
        // check packet length
        if(cfg->transmit.len > RADIOLIB_SX127X_MAX_PACKET_LENGTH) {
          return(RADIOLIB_ERR_PACKET_TOO_LONG);
        }

        // set DIO mapping
        if(this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD) > RADIOLIB_SX127X_HOP_PERIOD_OFF) {
          this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_LORA_TX_DONE | RADIOLIB_SX127X_DIO1_LORA_FHSS_CHANGE_CHANNEL, 7, 4);
        } else {
          this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_LORA_TX_DONE, 7, 6);
        }

        // apply fixes to errata
        RADIOLIB_ERRATA_SX127X(false);

        // clear interrupt flags
        clearIrqFlags(RADIOLIB_SX127X_FLAGS_ALL);

        // set packet length
        state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PAYLOAD_LENGTH, cfg->transmit.len);

        // set FIFO pointers
        state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_TX_BASE_ADDR, RADIOLIB_SX127X_FIFO_TX_BASE_ADDR_MAX);
        state |= this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_ADDR_PTR, RADIOLIB_SX127X_FIFO_TX_BASE_ADDR_MAX);

      } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
        // clear interrupt flags
        clearIrqFlags(RADIOLIB_SX127X_FLAGS_ALL);

        // set DIO mapping
        if(cfg->transmit.len > RADIOLIB_SX127X_MAX_PACKET_LENGTH_FSK) {
          this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO1_PACK_FIFO_EMPTY, 5, 4);
        } else {
          this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_PACK_PACKET_SENT, 7, 6);
        }

        // set packet length - increased by 1 when address filter is enabled
        uint8_t filter = this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PACKET_CONFIG_1, 2, 1);
        if(this->packetLengthConfig == RADIOLIB_SX127X_PACKET_VARIABLE) {
          if((filter == RADIOLIB_SX127X_ADDRESS_FILTERING_NODE) || (filter == RADIOLIB_SX127X_ADDRESS_FILTERING_NODE_BROADCAST)) {
            this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FIFO, cfg->transmit.len + 1);
            this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FIFO, cfg->transmit.addr);
          } else {
            this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_FIFO, cfg->transmit.len);
          }
        
        }
      
      }

      // write packet to FIFO
      size_t packetLen = cfg->transmit.len;
      if((modem == RADIOLIB_SX127X_FSK_OOK) && (cfg->transmit.len > RADIOLIB_SX127X_MAX_PACKET_LENGTH_FSK)) {
        packetLen = RADIOLIB_SX127X_FIFO_THRESH - 1;
        this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_FIFO_THRESH, RADIOLIB_SX127X_TX_START_FIFO_NOT_EMPTY, 7, 7);
      }
      this->mod->SPIwriteRegisterBurst(RADIOLIB_SX127X_REG_FIFO, cfg->transmit.data, packetLen);
    } break;
    
    default:
      return(RADIOLIB_ERR_UNSUPPORTED);
  }

  this->stagedMode = mode;
  return(state);
}

int16_t SX127x::launchMode() {
  int16_t state;
  switch(this->stagedMode) {
    case(RADIOLIB_RADIO_MODE_RX): {
      this->mod->setRfSwitchState(Module::MODE_RX);
      state = setMode(this->rxMode);
      RADIOLIB_ASSERT(state);
    } break;
  
    case(RADIOLIB_RADIO_MODE_TX): {
      this->mod->setRfSwitchState(Module::MODE_TX);
      state = setMode(RADIOLIB_SX127X_TX);
      RADIOLIB_ASSERT(state);
    } break;
    
    default:
      return(RADIOLIB_ERR_UNSUPPORTED);
  }

  this->stagedMode = RADIOLIB_RADIO_MODE_NONE;
  return(state);
}

#if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
void SX127x::setDirectAction(void (*func)(void)) {
  setDio1Action(func, this->mod->hal->GpioInterruptRising);
}

void SX127x::readBit(uint32_t pin) {
  updateDirectBuffer((uint8_t)this->mod->hal->digitalRead(pin));
}
#endif

int16_t SX127x::setFHSSHoppingPeriod(uint8_t freqHoppingPeriod) {
  return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD, freqHoppingPeriod));
}

uint8_t SX127x::getFHSSHoppingPeriod(void) {
  return(this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_PERIOD));
}

uint8_t SX127x::getFHSSChannel(void) {
  return(this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_HOP_CHANNEL, 5, 0));
}

void SX127x::clearFHSSInt(void) {
  int16_t modem = getActiveModem();
  if(modem == RADIOLIB_SX127X_LORA) {
    this->mod->SPIwriteRegister(RADIOLIB_SX127X_REG_IRQ_FLAGS, RADIOLIB_SX127X_CLEAR_IRQ_FLAG_FHSS_CHANGE_CHANNEL);
  } else if(modem == RADIOLIB_SX127X_FSK_OOK) {
    return; //These are not the interrupts you are looking for
  }
}

int16_t SX127x::setDIOMapping(uint32_t pin, uint32_t value) {
  if (pin > 5)
    return RADIOLIB_ERR_INVALID_DIO_PIN;

  if (pin < 4)
    return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, value, 7 - 2 * pin, 6 - 2 * pin));
  else
    return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_2, value, 15 - 2 * pin, 14 - 2 * pin));
}

int16_t SX127x::setDIOPreambleDetect(bool usePreambleDetect) {
  return this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_2, (usePreambleDetect) ? RADIOLIB_SX127X_DIO_MAP_PREAMBLE_DETECT : RADIOLIB_SX127X_DIO_MAP_RSSI, 0, 0);
}

float SX127x::getRSSI(bool packet, bool skipReceive, int16_t offset) {
  if(getActiveModem() == RADIOLIB_SX127X_LORA) {
    if(packet) {
      // LoRa packet mode, get RSSI of the last packet
      float lastPacketRSSI = offset + this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_PKT_RSSI_VALUE);

      // spread-spectrum modulation signal can be received below noise floor
      // check last packet SNR and if it's less than 0, add it to reported RSSI to get the correct value
      float lastPacketSNR = SX127x::getSNR();
      if(lastPacketSNR < 0.0f) {
        lastPacketRSSI += lastPacketSNR;
      }
      return(lastPacketRSSI);

    } else {
      // LoRa instant, get current RSSI
      float currentRSSI = offset + this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_RSSI_VALUE);
      return(currentRSSI);
    }
  
  } else {
    // for FSK, there is no packet RSSI

    // enable listen mode
    if(!skipReceive) {
      startReceive();
    }

    // read the value for FSK
    float rssi = (float)this->mod->SPIgetRegValue(RADIOLIB_SX127X_REG_RSSI_VALUE_FSK) / -2.0f;

    // set mode back to standby
    if(!skipReceive) {
      standby();
    }

    // return the value
    return(rssi);
  }
}

int16_t SX127x::setHeaderType(uint8_t headerType, uint8_t bitIndex, size_t len) {
  // check active modem
  if(getActiveModem() != RADIOLIB_SX127X_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  int16_t state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_MODEM_CONFIG_1, headerType, bitIndex, bitIndex);
  RADIOLIB_ASSERT(state);

  // set length to register
  state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_PAYLOAD_LENGTH, len);
  RADIOLIB_ASSERT(state);

  // update cached value
  SX127x::packetLength = len;

  return(state);
}

int16_t SX127x::setLowBatteryThreshold(int8_t level, uint32_t pin) {
  // check disable
  if(level < 0) {
    return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_LOW_BAT, RADIOLIB_SX127X_LOW_BAT_OFF, 3, 3));
  }

  // enable detector and set the threshold
  int16_t state = this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_LOW_BAT, RADIOLIB_SX127X_LOW_BAT_ON | level, 3, 0);
  RADIOLIB_ASSERT(state);

  // set DIO mapping
  switch(pin) {
    case(0):
      return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO0_PACK_TEMP_CHANGE_LOW_BAT, 7, 6));
    case(3):
      return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_1, RADIOLIB_SX127X_DIO3_CONT_TEMP_CHANGE_LOW_BAT, 1, 0));
    case(4):
      return(this->mod->SPIsetRegValue(RADIOLIB_SX127X_REG_DIO_MAPPING_2, RADIOLIB_SX127X_DIO4_PACK_TEMP_CHANGE_LOW_BAT, 7, 6));
  }
  return(RADIOLIB_ERR_INVALID_DIO_PIN);
}

#endif
