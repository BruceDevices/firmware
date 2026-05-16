#include "CC1101.h"
#include <math.h>
#if !RADIOLIB_EXCLUDE_CC1101

CC1101::CC1101(Module* module) : PhysicalLayer() {
  this->freqStep = RADIOLIB_CC1101_FREQUENCY_STEP_SIZE;
  this->maxPacketLength = RADIOLIB_CC1101_MAX_PACKET_LENGTH;
  this->mod = module;
}

int16_t CC1101::begin(float freq, float br, float freqDev, float rxBw, int8_t pwr, uint8_t preambleLength) {
  // set the modulation and execute the common part
  this->modulation = RADIOLIB_CC1101_MOD_FORMAT_2_FSK;
  return(this->beginCommon(freq, br, freqDev, rxBw, pwr, preambleLength));
}

int16_t CC1101::beginFSK4(float freq, float br, float freqDev, float rxBw, int8_t pwr, uint8_t preambleLength) {
  // set the modulation and execute the common part
  this->modulation = RADIOLIB_CC1101_MOD_FORMAT_4_FSK;
  return(this->beginCommon(freq, br, freqDev, rxBw, pwr, preambleLength));
}

void CC1101::reset() {
  // just send the command, the reset sequence as described in datasheet seems unnecessary in our usage
  SPIsendCommand(RADIOLIB_CC1101_CMD_RESET);
}

int16_t CC1101::transmit(const uint8_t* data, size_t len, uint8_t addr) {
  // calculate timeout (5ms + 500 % of expected time-on-air)
  RadioLibTime_t timeout = 5 + (RadioLibTime_t)((((float)(len * 8)) / this->bitRate) * 5);

  // start transmission
  int16_t state = startTransmit(data, len, addr);
  RADIOLIB_ASSERT(state);

  // wait for transmission start or timeout
  RadioLibTime_t start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();

    if(this->mod->hal->millis() - start > timeout) {
      (void)finishTransmit();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }

  // wait for transmission end or timeout
  start = this->mod->hal->millis();
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();

    if(this->mod->hal->millis() - start > timeout) {
      (void)finishTransmit();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }

  return(finishTransmit());
}

int16_t CC1101::receive(uint8_t* data, size_t len, RadioLibTime_t timeout) {
  RadioLibTime_t timeoutInternal = timeout;
  if(!timeoutInternal) {
    // calculate timeout (500 ms + 400 full max-length packets at current bit rate)
    timeoutInternal = 500 + (1.0f/(this->bitRate))*(RADIOLIB_CC1101_MAX_PACKET_LENGTH*400.0f);
  } 

  // start reception
  int16_t state = startReceive();
  RADIOLIB_ASSERT(state);

  // wait for packet start or timeout
  RadioLibTime_t start = this->mod->hal->millis();
  while(this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();

    if(this->mod->hal->millis() - start > timeoutInternal) {
      (void)finishReceive();
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }

  // wait for packet end or timeout
  start = this->mod->hal->millis();
  while(!this->mod->hal->digitalRead(this->mod->getIrq())) {
    this->mod->hal->yield();

    if(this->mod->hal->millis() - start > timeoutInternal) {
      (void)finishReceive();
      return(RADIOLIB_ERR_RX_TIMEOUT);
    }
  }

  // read packet data
  return(readData(data, len));
}

int16_t CC1101::standby() {
  // set idle mode
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  // wait until idle is reached
  RadioLibTime_t start = this->mod->hal->millis();
  while(SPIgetRegValue(RADIOLIB_CC1101_REG_MARCSTATE, 4, 0) != RADIOLIB_CC1101_MARC_STATE_IDLE) {
    mod->hal->yield();
    if(this->mod->hal->millis() - start > 100) {
      // timeout, this should really not happen
      return(RADIOLIB_ERR_UNKNOWN);
    }
  };

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);
  return(RADIOLIB_ERR_NONE);
}

int16_t CC1101::standby(uint8_t mode) {
  (void)mode;
  return(standby());
}

int16_t CC1101::sleep() {
  int16_t state =standby();
  SPIsendCommand(RADIOLIB_CC1101_CMD_POWER_DOWN);
  return(state);
}

int16_t CC1101::transmitDirect(uint32_t frf) {
  return transmitDirect(true, frf);
}

int16_t CC1101::transmitDirectAsync(uint32_t frf) {
  return transmitDirect(false, frf);
}

int16_t CC1101::transmitDirect(bool sync, uint32_t frf) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_TX);

  // user requested to start transmitting immediately (required for RTTY)
  if(frf != 0) {
    SPIwriteRegister(RADIOLIB_CC1101_REG_FREQ2, (frf & 0xFF0000) >> 16);
    SPIwriteRegister(RADIOLIB_CC1101_REG_FREQ1, (frf & 0x00FF00) >> 8);
    SPIwriteRegister(RADIOLIB_CC1101_REG_FREQ0, frf & 0x0000FF);

    SPIsendCommand(RADIOLIB_CC1101_CMD_TX);
    return(RADIOLIB_ERR_NONE);
  }

  // activate direct mode
  int16_t state = directMode(sync);
  RADIOLIB_ASSERT(state);

  // start transmitting
  SPIsendCommand(RADIOLIB_CC1101_CMD_TX);
  return(state);
}

int16_t CC1101::receiveDirect() {
  return receiveDirect(true);
}

int16_t CC1101::receiveDirectAsync() {
  return receiveDirect(false);
}

int16_t CC1101::receiveDirect(bool sync) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // enable promiscuous mode - needed for protocols that decode in software (e.g. PagerClient)
  int16_t state = setPromiscuousMode(true);
  RADIOLIB_ASSERT(state);

  // activate direct mode
  state = directMode(sync);
  RADIOLIB_ASSERT(state);

  // start receiving
  SPIsendCommand(RADIOLIB_CC1101_CMD_RX);
  return(RADIOLIB_ERR_NONE);
}

int16_t CC1101::packetMode() {
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, RADIOLIB_CC1101_CRC_AUTOFLUSH_OFF | RADIOLIB_CC1101_APPEND_STATUS_ON | RADIOLIB_CC1101_ADR_CHK_NONE, 3, 0);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_WHITE_DATA_OFF | RADIOLIB_CC1101_PKT_FORMAT_NORMAL, 6, 4);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_CRC_ON | this->packetLengthConfig, 2, 0);
  return(state);
}

void CC1101::setGdo0Action(void (*func)(void), uint32_t dir) {
  this->mod->hal->attachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()), func, dir);
}

void CC1101::clearGdo0Action() {
  this->mod->hal->detachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()));
}

void CC1101::setPacketReceivedAction(void (*func)(void)) {
  this->setGdo0Action(func, this->mod->hal->GpioInterruptRising);
}

void CC1101::clearPacketReceivedAction() {
  this->clearGdo0Action();
}

void CC1101::setPacketSentAction(void (*func)(void)) {
  this->setGdo2Action(func, this->mod->hal->GpioInterruptFalling);
}

void CC1101::clearPacketSentAction() {
  this->clearGdo2Action();
}

void CC1101::setGdo2Action(void (*func)(void), uint32_t dir) {
  if(this->mod->getGpio() == RADIOLIB_NC) {
    return;
  }
  this->mod->hal->pinMode(this->mod->getGpio(), this->mod->hal->GpioModeInput);
  this->mod->hal->attachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getGpio()), func, dir);
}

void CC1101::clearGdo2Action() {
  if(this->mod->getGpio() == RADIOLIB_NC) {
    return;
  }
  this->mod->hal->detachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getGpio()));
}

int16_t CC1101::startTransmit(const uint8_t* data, size_t len, uint8_t addr) {
  // check packet length
  if(len > RADIOLIB_CC1101_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // set mode to standby
  standby();

  // flush Tx FIFO
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_TX);

  // Turn on freq oscilator
  SPIsendCommand(RADIOLIB_CC1101_CMD_FSTXON);

  // Check MARCSTATE and wait until ready to tx
  // 724us is the longest time for calibrate per datasheet
  // Needs a bit more time for reliability
  RadioLibTime_t start = this->mod->hal->micros();
  while(SPIgetRegValue(RADIOLIB_CC1101_REG_MARCSTATE, 4, 0) != 0x12) {
    if(this->mod->hal->micros() - start > 1600) {
      standby();
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }

  // set GDO2 mapping only if we aren't refilling the FIFO
  int16_t state = RADIOLIB_ERR_NONE;
  if(len <= RADIOLIB_CC1101_FIFO_SIZE) {
    state = SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG2, RADIOLIB_CC1101_GDOX_SYNC_WORD_SENT_OR_PKT_RECEIVED, 5, 0);
    RADIOLIB_ASSERT(state);
  }

  // data put on FIFO
  uint8_t dataSent = 0;
  uint8_t filter = SPIgetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, 1, 0);

  // optionally write packet length
  if(this->packetLengthConfig == RADIOLIB_CC1101_LENGTH_CONFIG_VARIABLE) {
    if (len > RADIOLIB_CC1101_MAX_PACKET_LENGTH - 1) {
        return(RADIOLIB_ERR_PACKET_TOO_LONG);
    }
    SPIwriteRegister(RADIOLIB_CC1101_REG_FIFO, len + (filter != RADIOLIB_CC1101_ADR_CHK_NONE? 1:0));
    dataSent+= 1;
  }

  // check address filtering
  if(filter != RADIOLIB_CC1101_ADR_CHK_NONE) {
    SPIwriteRegister(RADIOLIB_CC1101_REG_FIFO, addr);
    dataSent += 1;
  }

  // fill the FIFO
  uint8_t initialWrite = RADIOLIB_MIN((uint8_t)len, (uint8_t)(RADIOLIB_CC1101_FIFO_SIZE - dataSent));
  SPIwriteRegisterBurst(RADIOLIB_CC1101_REG_FIFO, const_cast<uint8_t*>(data), initialWrite);
  dataSent += initialWrite;

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_TX);

  // set mode to transmit
  SPIsendCommand(RADIOLIB_CC1101_CMD_TX);

  // Keep feeding the FIFO until the packet is done
  while (dataSent < len) {
    uint8_t fifoBytes = 0;
    uint8_t prevFifobytes = 0;

    // Check number of bytes on FIFO twice due to the CC1101 errata. Block until two reads are equal.
    do{
      fifoBytes = SPIgetRegValue(RADIOLIB_CC1101_REG_TXBYTES, 6, 0);
      prevFifobytes = SPIgetRegValue(RADIOLIB_CC1101_REG_TXBYTES, 6, 0);
    } while (fifoBytes != prevFifobytes);

    //If there is room add more data to the FIFO
    if (fifoBytes < RADIOLIB_CC1101_FIFO_SIZE) {
        uint8_t bytesToWrite = RADIOLIB_MIN((uint8_t)(RADIOLIB_CC1101_FIFO_SIZE - fifoBytes), (uint8_t)(len - dataSent));
        SPIwriteRegisterBurst(RADIOLIB_CC1101_REG_FIFO, const_cast<uint8_t*>(&data[dataSent]), bytesToWrite);
        dataSent += bytesToWrite;
    }
  }
  return(state);
}

int16_t CC1101::finishTransmit() {
  // set mode to standby to disable transmitter/RF switch
  
  // Check MARCSTATE for Idle to let anything in the FIFO empty
  // Timeout is 2x FIFO transmit time
  RadioLibTime_t timeout = (1.0f/(this->bitRate))*(RADIOLIB_CC1101_FIFO_SIZE*2.0f);
  RadioLibTime_t start = this->mod->hal->millis();
  while(SPIgetRegValue(RADIOLIB_CC1101_REG_MARCSTATE, 4, 0) != 0x01) {
    if(this->mod->hal->millis() - start > timeout) {
      return(RADIOLIB_ERR_TX_TIMEOUT);
    }
  }
  
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // flush Tx FIFO
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_TX);
  
  // reset GDO2 back to high-Z
  return(SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG2, RADIOLIB_CC1101_GDOX_HIGH_Z, 5, 0));
}

int16_t CC1101::startReceive() {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // flush Rx FIFO
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_RX);

  // set GDO0 mapping
  // this is the only interrupt source that works reliably
  // RADIOLIB_CC1101_GDOX_SYNC_WORD_SENT_OR_PKT_RECEIVED gets triggered by both packet received as well as packet discarded,
  // RADIOLIB_CC1101_GDOX_PKT_RECEIVED_CRC_OK does not get triggered with CRC disabled
  state = SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0, RADIOLIB_CC1101_GDOX_RX_FIFO_FULL_OR_PKT_END, 6, 0);
  RADIOLIB_ASSERT(state);

  // set Rx FIFO threshold to the maximum Rx size
  state = SPIsetRegValue(RADIOLIB_CC1101_REG_FIFOTHR, RADIOLIB_CC1101_FIFO_THR_TX_1_RX_64, 3, 0);
  RADIOLIB_ASSERT(state);

  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_RX);

  // set mode to receive
  SPIsendCommand(RADIOLIB_CC1101_CMD_RX);

  return(state);
}

int16_t CC1101::startReceive(uint32_t timeout, uint32_t irqFlags, uint32_t irqMask, size_t len) {
  (void)timeout;
  (void)irqFlags;
  (void)irqMask;
  (void)len;
  return(startReceive());
}

int16_t CC1101::readData(uint8_t* data, size_t len) {
  // get packet length
  size_t length = getPacketLength();
  if((len != 0) && (len < length)) {
    // user requested less data than we got, only return what was requested
    length = len;
  }

  // check address filtering
  uint8_t filter = SPIgetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, 1, 0);
  if(filter != RADIOLIB_CC1101_ADR_CHK_NONE) {
    SPIreadRegister(RADIOLIB_CC1101_REG_FIFO);
  }

  // read packet data
  SPIreadRegisterBurst(RADIOLIB_CC1101_REG_FIFO, length, data);

  // check if status bytes are enabled (default: RADIOLIB_CC1101_APPEND_STATUS_ON)
  bool isAppendStatus = SPIgetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, 2, 2) == RADIOLIB_CC1101_APPEND_STATUS_ON;

  // if status byte is enabled at least 2 bytes (2 status bytes + any following packet) will remain in FIFO.
  int16_t state = RADIOLIB_ERR_NONE;
  if (isAppendStatus) {
    // read RSSI byte
    this->rawRSSI = SPIgetRegValue(RADIOLIB_CC1101_REG_FIFO);

    // read LQI and CRC byte
    uint8_t val = SPIgetRegValue(RADIOLIB_CC1101_REG_FIFO);
    this->rawLQI = val & 0x7F;

    // check CRC
    if(this->crcOn && (val & RADIOLIB_CC1101_CRC_OK) == RADIOLIB_CC1101_CRC_ERROR) {
      this->packetLengthQueried = false;
      state = RADIOLIB_ERR_CRC_MISMATCH;
    }
  }

  // clear internal flag so getPacketLength can return the new packet length
  this->packetLengthQueried = false;

  // flush then standby according to RXOFF_MODE (default: RADIOLIB_CC1101_RXOFF_IDLE)
  if(SPIgetRegValue(RADIOLIB_CC1101_REG_MCSM1, 3, 2) == RADIOLIB_CC1101_RXOFF_IDLE) {
    (void)finishReceive();
  }

  return(state);
}

int16_t CC1101::finishReceive() {
  // go to standby and flush the FIFO
  int16_t state = standby();
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_RX);
  RADIOLIB_ASSERT(state);

  // reset GDO0 back to high-Z
  return(SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0, RADIOLIB_CC1101_GDOX_HIGH_Z, 5, 0));
}

int16_t CC1101::setFrequency(float freq) {
  // check allowed frequency range
  #if RADIOLIB_CHECK_PARAMS
  if(!(((freq >= 300.0f) && (freq <= 348.0f)) ||
       ((freq >= 387.0f) && (freq <= 464.0f)) ||
       ((freq >= 779.0f) && (freq <= 928.0f)))) {
    return(RADIOLIB_ERR_INVALID_FREQUENCY);
  }
  #endif

  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  //set carrier frequency
  uint32_t base = 1;
  uint32_t FRF = (freq * (base << 16)) / 26.0f;
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_FREQ2, (FRF & 0xFF0000) >> 16, 7, 0);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_FREQ1, (FRF & 0x00FF00) >> 8, 7, 0);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_FREQ0, FRF & 0x0000FF, 7, 0);

  if(state == RADIOLIB_ERR_NONE) {
    this->frequency = freq;
  }

  // Update the TX power accordingly to new freq. (PA values depend on chosen freq)
  return(setOutputPower(this->power));
}

int16_t CC1101::setBitRate(float br) {
  RADIOLIB_CHECK_RANGE(br, 0.025f, 600.0f, RADIOLIB_ERR_INVALID_BIT_RATE);

  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  // calculate exponent and mantissa values
  uint8_t e = 0;
  uint8_t m = 0;
  getExpMant(br * 1000.0f, 256, 28, 14, e, m);

  // set bit rate value
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG4, e, 3, 0);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG3, m);
  if(state == RADIOLIB_ERR_NONE) {
    this->bitRate = br;
  }
  return(state);
}

int16_t CC1101::setBitRateTolerance(uint8_t brt) {
  if (brt > 0x03)  return (RADIOLIB_ERR_INVALID_BIT_RATE_TOLERANCE_VALUE);

  // Set Bit Rate tolerance
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_BSCFG, brt, 1, 0);

  return(state);
}

int16_t CC1101::setRxBandwidth(float rxBw) {
  RADIOLIB_CHECK_RANGE(rxBw, 58.0f, 812.0f, RADIOLIB_ERR_INVALID_RX_BANDWIDTH);

  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  // calculate exponent and mantissa values
  for(int8_t e = 3; e >= 0; e--) {
    for(int8_t m = 3; m >= 0; m --) {
      float point = (RADIOLIB_CC1101_CRYSTAL_FREQ * 1000000.0f)/(8 * (m + 4) * ((uint32_t)1 << e));
      if(fabs((rxBw * 1000.0f - point) <= 1000.0f)) {
        // set Rx channel filter bandwidth
        return(SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG4, (e << 6) | (m << 4), 7, 4));
      }

    }
  }

  return(RADIOLIB_ERR_INVALID_RX_BANDWIDTH);
}

int16_t CC1101::autoSetRxBandwidth() {
  // Uncertainty ~ +/- 40ppm for a cheap CC1101
  // Uncertainty * 2 for both transmitter and receiver
  float uncertainty = ((this->frequency) * 40 * 2);
  uncertainty = (uncertainty/1000); //Since bitrate is in kBit
  float minbw = ((this->bitRate) + uncertainty);
  
  const int options[16] = { 58, 68, 81, 102, 116, 135, 162, 203, 232, 270, 325, 406, 464, 541, 650, 812 };
  
  for(int i = 0; i < 16; i++) {
    if(options[i] > minbw) {
      return(setRxBandwidth(options[i]));
    }
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t CC1101::setFrequencyDeviation(float freqDev) {
  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0f) {
    newFreqDev = 1.587f;
  }

  // check range unless 0 (special value)
  if (freqDev != 0) {
    RADIOLIB_CHECK_RANGE(newFreqDev, 1.587f, 380.8f, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
  }

  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  // calculate exponent and mantissa values
  uint8_t e = 0;
  uint8_t m = 0;
  getExpMant(newFreqDev * 1000.0f, 8, 17, 7, e, m);

  // set frequency deviation value
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_DEVIATN, (e << 4), 6, 4);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_DEVIATN, m, 2, 0);
  
  return(state);
}

int16_t CC1101::getFrequencyDeviation(float *freqDev) {
  if (freqDev == NULL) {
    return(RADIOLIB_ERR_NULL_POINTER);
  }

  // if ASK/OOK, deviation makes no sense
  if (this->modulation == RADIOLIB_CC1101_MOD_FORMAT_ASK_OOK) {
    *freqDev = 0.0;

    return(RADIOLIB_ERR_NONE);
  }

  // get exponent and mantissa values from registers
  uint8_t e = (uint8_t)(SPIgetRegValue(RADIOLIB_CC1101_REG_DEVIATN, 6, 4) >> 4);
  uint8_t m = (uint8_t)SPIgetRegValue(RADIOLIB_CC1101_REG_DEVIATN, 2, 0);

  // calculate frequency deviation (pag. 79 of the CC1101 datasheet):
  //
  //   freqDev = (fXosc / 2^17) * (8 + m) * 2^e
  //
  *freqDev = (1000.0f / (uint32_t(1) << 17)) - (8 + m) * (uint32_t(1) << e);

  return(RADIOLIB_ERR_NONE);
}

int16_t CC1101::setOutputPower(int8_t pwr) {
  // check if power value is configurable
  uint8_t powerRaw = 0;
  int16_t state = checkOutputPower(pwr, NULL, &powerRaw);
  RADIOLIB_ASSERT(state);

  // store the value
  this->power = pwr;

  if(this->modulation == RADIOLIB_CC1101_MOD_FORMAT_ASK_OOK){
    // Amplitude modulation:
    // PA_TABLE[0] is the power to be used when transmitting a 0  (no power)
    // PA_TABLE[1] is the power to be used when transmitting a 1  (full power)

    const uint8_t paValues[2] = { 0x00, powerRaw };
    SPIwriteRegisterBurst(RADIOLIB_CC1101_REG_PATABLE, paValues, 2);
    return(RADIOLIB_ERR_NONE);

  } else {
    // Freq modulation:
    // PA_TABLE[0] is the power to be used when transmitting.
    return(SPIsetRegValue(RADIOLIB_CC1101_REG_PATABLE, powerRaw));
  }
}

int16_t CC1101::checkOutputPower(int8_t power, int8_t* clipped) {
  return(checkOutputPower(power, clipped, NULL));
}

int16_t CC1101::checkOutputPower(int8_t power, int8_t* clipped, uint8_t* raw) {
  const int8_t allowedPwrs[8] = { -30, -20, -15, -10, 0, 5, 7, 10 };

  if(clipped) {
    if(power <= -30) {
      *clipped = -30;
    } else if(power >= 10) {
      *clipped = 10;
    } else {
      for(int i = 0; i < 8; i++) {
        if(allowedPwrs[i] > power) {
          break;
        }
        *clipped = allowedPwrs[i];
      }
    }
  }

  // if just a check occurs (and not requesting the raw power value), return now
  if(!raw) {
    for(size_t i = 0; i < sizeof(allowedPwrs); i++) {
      if(allowedPwrs[i] == power) {
        return(RADIOLIB_ERR_NONE);
      }
    }
    return(RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  }

  // round to the known frequency settings
  uint8_t f;
  if(this->frequency < 374.0f) {
    // 315 MHz
    f = 0;
  } else if(this->frequency < 650.5f) {
    // 434 MHz
    f = 1;
  } else if(this->frequency < 891.5f) {
    // 868 MHz
    f = 2;
  } else {
    // 915 MHz
    f = 3;
  }

  // get raw power setting
  uint8_t paTable[8][4] = {{0x12, 0x12, 0x03, 0x03},
                           {0x0D, 0x0E, 0x0F, 0x0E},
                           {0x1C, 0x1D, 0x1E, 0x1E},
                           {0x34, 0x34, 0x27, 0x27},
                           {0x51, 0x60, 0x50, 0x8E},
                           {0x85, 0x84, 0x81, 0xCD},
                           {0xCB, 0xC8, 0xCB, 0xC7},
                           {0xC2, 0xC0, 0xC2, 0xC0}};

  for(uint8_t i = 0; i < sizeof(allowedPwrs); i++) {
    if(power == allowedPwrs[i]) {
      *raw = paTable[i][f];
      return(RADIOLIB_ERR_NONE);
    }
  }
  
  return(RADIOLIB_ERR_INVALID_OUTPUT_POWER);
}

int16_t CC1101::setSyncWord(uint8_t* sync, size_t len) {
  return this->setSyncWord(sync, len, 0, false);
}

int16_t CC1101::setSyncWord(const uint8_t* syncWord, uint8_t len, uint8_t maxErrBits, bool requireCarrierSense) {
  if((maxErrBits > 1) || (len != 2)) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }

  // sync word must not contain value 0x00
  for(uint8_t i = 0; i < len; i++) {
    if(syncWord[i] == 0x00) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }
  }

  // enable sync word filtering
  int16_t state = enableSyncWordFiltering(maxErrBits, requireCarrierSense);
  RADIOLIB_ASSERT(state);

  // set sync word register
  state = SPIsetRegValue(RADIOLIB_CC1101_REG_SYNC1, syncWord[0]);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_SYNC0, syncWord[1]);

  return(state);
}

int16_t CC1101::setSyncWord(uint8_t syncH, uint8_t syncL, uint8_t maxErrBits, bool requireCarrierSense) {
  uint8_t syncWord[] = { syncH, syncL };
  return(setSyncWord(syncWord, sizeof(syncWord), maxErrBits, requireCarrierSense));
}

int16_t CC1101::setPreambleLength(size_t len) {
  return this->setPreambleLength(len, len-4);
}
int16_t CC1101::setPreambleLength(uint8_t preambleLength, uint8_t qualityThreshold) {
  // check allowed values
  uint8_t value;
  switch(preambleLength) {
    case 16:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_2;
      break;
    case 24:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_3;
      break;
    case 32:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_4;
      break;
    case 48:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_6;
      break;
    case 64:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_8;
      break;
    case 96:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_12;
      break;
    case 128:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_16;
      break;
    case 192:
      value = RADIOLIB_CC1101_NUM_PREAMBLE_24;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
  }

  // set preabmble quality threshold and the actual length
  uint8_t pqt = qualityThreshold/4;
  if(pqt > 7) {
    pqt = 7;
  }
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, pqt << 5, 7, 5);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG1, value, 6, 4);
  return(state);
}

int16_t CC1101::setNodeAddress(uint8_t nodeAddr, uint8_t numBroadcastAddrs) {
  RADIOLIB_CHECK_RANGE(numBroadcastAddrs, 1, 2, RADIOLIB_ERR_INVALID_NUM_BROAD_ADDRS);

  // enable address filtering
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, numBroadcastAddrs + 0x01, 1, 0);
  RADIOLIB_ASSERT(state);

  // set node address
  return(SPIsetRegValue(RADIOLIB_CC1101_REG_ADDR, nodeAddr));
}

int16_t CC1101::disableAddressFiltering() {
  // disable address filtering
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL1, RADIOLIB_CC1101_ADR_CHK_NONE, 1, 0);
  RADIOLIB_ASSERT(state);

  // set node address to default (0x00)
  return(SPIsetRegValue(RADIOLIB_CC1101_REG_ADDR, 0x00));
}


int16_t CC1101::setOOK(bool enableOOK) {
  // Change modulation
  if(enableOOK) {
    int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MOD_FORMAT_ASK_OOK, 6, 4);
    RADIOLIB_ASSERT(state);

    // PA_TABLE[0] is (by default) the power value used when transmitting a "0".
    // Set PA_TABLE[1] to be used when transmitting a "1".
    state = SPIsetRegValue(RADIOLIB_CC1101_REG_FREND0, 1, 2, 0);
    RADIOLIB_ASSERT(state);

    // update current modulation
    this->modulation = RADIOLIB_CC1101_MOD_FORMAT_ASK_OOK;
  } else {
    int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MOD_FORMAT_2_FSK, 6, 4);
    RADIOLIB_ASSERT(state);

    // Reset FREND0 to default value.
    state = SPIsetRegValue(RADIOLIB_CC1101_REG_FREND0, 0, 2, 0);
    RADIOLIB_ASSERT(state);

    // update current modulation
    this->modulation = RADIOLIB_CC1101_MOD_FORMAT_2_FSK;
  }

  // Update PA_TABLE values according to the new this->modulation.
  return(setOutputPower(this->power));
}

float CC1101::getRSSI() {
  float rssi;

  if(!this->directModeEnabled) {
    if(this->rawRSSI >= 128) {
      rssi = (((float)this->rawRSSI - 256.0f)/2.0f) - 74.0f;
    } else {
      rssi = (((float)this->rawRSSI)/2.0f) - 74.0f;
    }
  } else {
    uint8_t rawRssi = SPIreadRegister(RADIOLIB_CC1101_REG_RSSI);
    if(rawRssi >= 128) {
      rssi = ((rawRssi - 256) / 2) - 74;
    } else {
      rssi = (rawRssi / 2) - 74;
    }
  }
  return(rssi);
}

uint8_t CC1101::getLQI() const {
  return(this->rawLQI);
}

size_t CC1101::getPacketLength(bool update) {
  if(!this->packetLengthQueried && update) {
    if(this->packetLengthConfig == RADIOLIB_CC1101_LENGTH_CONFIG_VARIABLE) {
      this->packetLength = SPIreadRegister(RADIOLIB_CC1101_REG_FIFO);
    } else {
      this->packetLength = SPIreadRegister(RADIOLIB_CC1101_REG_PKTLEN);
    }

    this->packetLengthQueried = true;
  }

  return(this->packetLength);
}

int16_t CC1101::fixedPacketLengthMode(uint8_t len) {
  if(len == 0) {
    // infinite packet mode
    int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_LENGTH_CONFIG_INFINITE, 1, 0);
    RADIOLIB_ASSERT(state);
  }

  return(setPacketMode(RADIOLIB_CC1101_LENGTH_CONFIG_FIXED, len));
}

int16_t CC1101::variablePacketLengthMode(uint8_t maxLen) {
  return(setPacketMode(RADIOLIB_CC1101_LENGTH_CONFIG_VARIABLE, maxLen));
}

int16_t CC1101::enableSyncWordFiltering(uint8_t maxErrBits, bool requireCarrierSense) {
  int16_t state = RADIOLIB_ERR_NONE;

  switch(maxErrBits) {
    case 0:
      // in 16 bit sync word, expect all 16 bits
      state |= SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, (requireCarrierSense ? RADIOLIB_CC1101_SYNC_MODE_16_16_THR : RADIOLIB_CC1101_SYNC_MODE_16_16), 2, 0);
      break;
    case 1:
      // in 16 bit sync word, expect at least 15 bits
      state |= SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, (requireCarrierSense ? RADIOLIB_CC1101_SYNC_MODE_15_16_THR : RADIOLIB_CC1101_SYNC_MODE_15_16), 2, 0);
      break;
    default:
      state = RADIOLIB_ERR_INVALID_SYNC_WORD;
      break;
  }
  return(state);
}

int16_t CC1101::disableSyncWordFiltering(bool requireCarrierSense) {
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, (requireCarrierSense ? RADIOLIB_CC1101_SYNC_MODE_NONE_THR : RADIOLIB_CC1101_SYNC_MODE_NONE), 2, 0);
  return(state);
}

int16_t CC1101::setCrcFiltering(bool enable) {
  this->crcOn = enable;

  if (this->crcOn == true) {
    return(SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_CRC_ON, 2, 2));
  } else {
    return(SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_CRC_OFF, 2, 2));
  }
}

int16_t CC1101::setPromiscuousMode(bool enable, bool requireCarrierSense) {
  int16_t state = RADIOLIB_ERR_NONE;

  if(this->promiscuous == enable) {
    return(state);
  }

  if(enable) {
    // Lets set PQT to 0 with Promiscuous too
    // We have to set the length to set PQT, but it should get disabled with disableSyncWordFiltering()
    state = setPreambleLength(16, 0);
    RADIOLIB_ASSERT(state);
    // disable sync word filtering and insertion
    // this also disables preamble
    // Can enable Sync Mode with carriersense when promiscuous is enabled. Default is false: Sync Mode None	
    state = disableSyncWordFiltering(requireCarrierSense);
    RADIOLIB_ASSERT(state);

    // disable CRC filtering
    state = setCrcFiltering(false);
  } else {
    state = setPreambleLength(RADIOLIB_CC1101_DEFAULT_PREAMBLELEN, RADIOLIB_CC1101_DEFAULT_PREAMBLELEN/4);
    RADIOLIB_ASSERT(state);

    // enable sync word filtering and insertion
    state = enableSyncWordFiltering();
    RADIOLIB_ASSERT(state);

    // enable CRC filtering
    state = setCrcFiltering(true);
  }

  this->promiscuous = enable;

  return(state);
}

bool CC1101::getPromiscuousMode() {
  return (this->promiscuous);
}

int16_t CC1101::setDataShaping(uint8_t sh) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, this->modulation, 6, 4);
      break;
    case RADIOLIB_SHAPING_0_5:
      state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MOD_FORMAT_GFSK, 6, 4);
      break;
    default:
      return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
  }
  return(state);
}

int16_t CC1101::setEncoding(uint8_t encoding) {
  // set mode to standby
  int16_t state = standby();
  RADIOLIB_ASSERT(state);

  // set encoding
  switch(encoding) {
    case RADIOLIB_ENCODING_NRZ:
      state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MANCHESTER_EN_OFF, 3, 3);
      RADIOLIB_ASSERT(state);
      return(SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_WHITE_DATA_OFF, 6, 6));
    case RADIOLIB_ENCODING_MANCHESTER:
      state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MANCHESTER_EN_ON, 3, 3);
      RADIOLIB_ASSERT(state);
      return(SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_WHITE_DATA_OFF, 6, 6));
    case RADIOLIB_ENCODING_WHITENING:
      state = SPIsetRegValue(RADIOLIB_CC1101_REG_MDMCFG2, RADIOLIB_CC1101_MANCHESTER_EN_OFF, 3, 3);
      RADIOLIB_ASSERT(state);
      return(SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_WHITE_DATA_ON, 6, 6));
    default:
      return(RADIOLIB_ERR_INVALID_ENCODING);
  }
}

void CC1101::setRfSwitchPins(uint32_t rxEn, uint32_t txEn) {
  this->mod->setRfSwitchPins(rxEn, txEn);
}

void CC1101::setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]) {
  this->mod->setRfSwitchTable(pins, table);
}

uint8_t CC1101::randomByte() {
  // set mode to Rx
  SPIsendCommand(RADIOLIB_CC1101_CMD_RX);

  // wait a bit for the RSSI reading to stabilise
  this->mod->hal->delay(10);

  // read RSSI value 8 times, always keep just the least significant bit
  uint8_t randByte = 0x00;
  for(uint8_t i = 0; i < 8; i++) {
    randByte |= ((SPIreadRegister(RADIOLIB_CC1101_REG_RSSI) & 0x01) << i);
  }

  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  return(randByte);
}

int16_t CC1101::getChipVersion() {
  return(SPIgetRegValue(RADIOLIB_CC1101_REG_VERSION));
}

#if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
void CC1101::setDirectAction(void (*func)(void)) {
  setGdo0Action(func, this->mod->hal->GpioInterruptRising);
}

void CC1101::readBit(uint32_t pin) {
  updateDirectBuffer((uint8_t)this->mod->hal->digitalRead(pin));
}
#endif

int16_t CC1101::setDIOMapping(uint32_t pin, uint32_t value) {
  if(pin > 2) {
    return(RADIOLIB_ERR_INVALID_DIO_PIN);
  }

  return(SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0 - pin, value));
}

int16_t CC1101::beginCommon(float freq, float br, float freqDev, float rxBw, int8_t pwr, uint8_t preambleLength) {
  // set module properties
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_READ] = RADIOLIB_CC1101_CMD_READ;
  this->mod->spiConfig.cmds[RADIOLIB_MODULE_SPI_COMMAND_WRITE] = RADIOLIB_CC1101_CMD_WRITE;
  this->mod->init();
  this->mod->hal->pinMode(this->mod->getIrq(), this->mod->hal->GpioModeInput);

  // try to find the CC1101 chip
  uint8_t i = 0;
  bool flagFound = false;
  while((i < 10) && !flagFound) {
    int16_t version = getChipVersion();
    if((version == RADIOLIB_CC1101_VERSION_CURRENT) || (version == RADIOLIB_CC1101_VERSION_LEGACY) || (version == RADIOLIB_CC1101_VERSION_CLONE)) {
      flagFound = true;
    } else {
      RADIOLIB_DEBUG_BASIC_PRINTLN("CC1101 not found! (%d of 10 tries) RADIOLIB_CC1101_REG_VERSION == 0x%04X, expected 0x0004/0x0014", i + 1, version);
      this->mod->hal->delay(10);
      i++;
    }
  }

  if(!flagFound) {
    RADIOLIB_DEBUG_BASIC_PRINTLN("No CC1101 found!");
    this->mod->term();
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  } else {
    RADIOLIB_DEBUG_BASIC_PRINTLN("M\tCC1101");
  }

  // configure settings not accessible by API
  int16_t state = config();
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  // configure bitrate
  state = setBitRate(br);
  RADIOLIB_ASSERT(state);

  // configure default RX bandwidth
  state = setRxBandwidth(rxBw);
  RADIOLIB_ASSERT(state);

  // configure default frequency deviation
  state = setFrequencyDeviation(freqDev);
  RADIOLIB_ASSERT(state);

  // configure default TX output power
  state = setOutputPower(pwr);
  RADIOLIB_ASSERT(state);

  // set default packet length mode
  state = variablePacketLengthMode();
  RADIOLIB_ASSERT(state);

  // configure default preamble length
  state = setPreambleLength(preambleLength, preambleLength - 4);
  RADIOLIB_ASSERT(state);

  // set default data shaping
  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  // set default encoding
  state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  // set default sync word
  const uint8_t sw[RADIOLIB_CC1101_DEFAULT_SW_LEN] = RADIOLIB_CC1101_DEFAULT_SW;
  state = setSyncWord(sw[0], sw[1], 0, false);
  RADIOLIB_ASSERT(state);

  // flush FIFOs
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_RX);
  SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_TX);

  return(state);
}

int16_t CC1101::config() {
  // Reset the radio. Registers may be dirty from previous usage.
  reset();

  // Wait a ridiculous amount of time to be sure radio is ready.
  this->mod->hal->delay(150);

  standby();

  // enable automatic frequency synthesizer calibration and disable pin control
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_MCSM0, RADIOLIB_CC1101_FS_AUTOCAL_IDLE_TO_RXTX, 5, 4);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_MCSM0, RADIOLIB_CC1101_PIN_CTRL_OFF, 1, 1);
  RADIOLIB_ASSERT(state);

  // set GDOs to Hi-Z so that it doesn't output clock on startup (might confuse GDO0 action)
  state = SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0, RADIOLIB_CC1101_GDOX_HIGH_Z, 5, 0);
  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG2, RADIOLIB_CC1101_GDOX_HIGH_Z, 5, 0);
  RADIOLIB_ASSERT(state);

  // set packet mode
  state = packetMode();

  return(state);
}

int16_t CC1101::directMode(bool sync) {
  // set mode to standby
  SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);

  int16_t state = 0;
  this->directModeEnabled = true;
  if(sync) {
    // set GDO0 and GDO2 mapping
    state |= SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0, RADIOLIB_CC1101_GDOX_SERIAL_CLOCK , 5, 0);
    state |= SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG2, RADIOLIB_CC1101_GDOX_SERIAL_DATA_SYNC , 5, 0);

    // set continuous mode
    state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_PKT_FORMAT_SYNCHRONOUS, 5, 4);
  } else {
    // set GDO0 mapping
    state |= SPIsetRegValue(RADIOLIB_CC1101_REG_IOCFG0, RADIOLIB_CC1101_GDOX_SERIAL_DATA_ASYNC , 5, 0);

    // set asynchronous continuous mode
    state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_PKT_FORMAT_ASYNCHRONOUS, 5, 4);
  }

  state |= SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, RADIOLIB_CC1101_LENGTH_CONFIG_INFINITE, 1, 0);
  return(state);
}

void CC1101::getExpMant(float target, uint16_t mantOffset, uint8_t divExp, uint8_t expMax, uint8_t& exp, uint8_t& mant) {
  // get table origin point (exp = 0, mant = 0)
  float origin = (mantOffset * RADIOLIB_CC1101_CRYSTAL_FREQ * 1000000.0f)/((uint32_t)1 << divExp);

  // iterate over possible exponent values
  for(int8_t e = expMax; e >= 0; e--) {
    // get table column start value (exp = e, mant = 0);
    float intervalStart = ((uint32_t)1 << e) * origin;

    // check if target value is in this column
    if(target >= intervalStart) {
      // save exponent value
      exp = e;

      // calculate size of step between table rows
      float stepSize = intervalStart/(float)mantOffset;

      // get target point position (exp = e, mant = m)
      mant = ((target - intervalStart) / stepSize);

      // we only need the first match, terminate
      return;
    }
  }
}

int16_t CC1101::setPacketMode(uint8_t mode, uint16_t len) {
  // check length
  if (len > RADIOLIB_CC1101_MAX_PACKET_LENGTH) {
    return(RADIOLIB_ERR_PACKET_TOO_LONG);
  }

  // set PKTCTRL0.LENGTH_CONFIG
  int16_t state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTCTRL0, mode, 1, 0);
  RADIOLIB_ASSERT(state);

  // set length to register
  state = SPIsetRegValue(RADIOLIB_CC1101_REG_PKTLEN, len);
  RADIOLIB_ASSERT(state);

  // no longer in a direct mode
  this->directModeEnabled = false;

  // update the cached values
  this->packetLength = len;
  this->packetLengthConfig = mode;
  return(state);
}

Module* CC1101::getMod() {
  return(this->mod);
}

int16_t CC1101::SPIgetRegValue(uint8_t reg, uint8_t msb, uint8_t lsb) {
  // status registers require special command
  if((reg > RADIOLIB_CC1101_REG_TEST0) && (reg < RADIOLIB_CC1101_REG_PATABLE)) {
    reg |= RADIOLIB_CC1101_CMD_ACCESS_STATUS_REG;
  }

  return(this->mod->SPIgetRegValue(reg, msb, lsb));
}

int16_t CC1101::SPIsetRegValue(uint8_t reg, uint8_t value, uint8_t msb, uint8_t lsb, uint8_t checkInterval) {
  // status registers require special command
  if((reg > RADIOLIB_CC1101_REG_TEST0) && (reg < RADIOLIB_CC1101_REG_PATABLE)) {
    reg |= RADIOLIB_CC1101_CMD_ACCESS_STATUS_REG;
  }

  return(this->mod->SPIsetRegValue(reg, value, msb, lsb, checkInterval));
}

void CC1101::SPIreadRegisterBurst(uint8_t reg, uint8_t numBytes, uint8_t* inBytes) {
  this->mod->SPIreadRegisterBurst(reg | RADIOLIB_CC1101_CMD_BURST, numBytes, inBytes);
}

uint8_t CC1101::SPIreadRegister(uint8_t reg) {
  // status registers require special command
  if((reg > RADIOLIB_CC1101_REG_TEST0) && (reg < RADIOLIB_CC1101_REG_PATABLE)) {
    reg |= RADIOLIB_CC1101_CMD_ACCESS_STATUS_REG;
  }

  return(this->mod->SPIreadRegister(reg));
}

void CC1101::SPIwriteRegister(uint8_t reg, uint8_t data) {
  // status registers require special command
  if((reg > RADIOLIB_CC1101_REG_TEST0) && (reg < RADIOLIB_CC1101_REG_PATABLE)) {
    reg |= RADIOLIB_CC1101_CMD_ACCESS_STATUS_REG;
  }

  return(this->mod->SPIwriteRegister(reg, data));
}

void CC1101::SPIwriteRegisterBurst(uint8_t reg, const uint8_t* data, size_t len) {
  this->mod->SPIwriteRegisterBurst(reg | RADIOLIB_CC1101_CMD_BURST, data, len);
}

void CC1101::SPIsendCommand(uint8_t cmd) {
  this->mod->SPItransferStream(&cmd, 1, true, NULL, NULL, 0, false);
}

#endif
