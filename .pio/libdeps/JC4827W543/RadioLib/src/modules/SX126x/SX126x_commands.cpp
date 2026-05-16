#include "SX126x.h"

// this file contains implementation of all commands
// supported by the SX126x SPI interface
// in most cases, the names of methods match those in the datasheet
// however, sometimes slight changes had to be made in order to
// better fit the RadioLib API

#if !RADIOLIB_EXCLUDE_SX126X

int16_t SX126x::sleep() {
  return(SX126x::sleep(true));
}

int16_t SX126x::sleep(bool retainConfig) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  uint8_t sleepMode = RADIOLIB_SX126X_SLEEP_START_WARM | RADIOLIB_SX126X_SLEEP_RTC_OFF;
  if(!retainConfig) {
    sleepMode = RADIOLIB_SX126X_SLEEP_START_COLD | RADIOLIB_SX126X_SLEEP_RTC_OFF;
  }
  int16_t state = this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_SLEEP, &sleepMode, 1, false, false);

  // wait for SX126x to safely enter sleep mode
  this->mod->hal->delay(1);

  return(state);
}

int16_t SX126x::standby() {
  return(SX126x::standby(this->standbyXOSC ? RADIOLIB_SX126X_STANDBY_XOSC : RADIOLIB_SX126X_STANDBY_RC, true));
}

int16_t SX126x::standby(uint8_t mode) {
  return(SX126x::standby(mode, true));
}

int16_t SX126x::standby(uint8_t mode, bool wakeup) {
  // set RF switch (if present)
  this->mod->setRfSwitchState(Module::MODE_IDLE);

  if(wakeup) {
    // send a NOP command - this pulls the NSS low to exit the sleep mode,
    // while preventing interference with possible other SPI transactions
    // see https://github.com/jgromes/RadioLib/discussions/1364
    (void)this->mod->SPIwriteStream((uint16_t)RADIOLIB_SX126X_CMD_NOP, NULL, 0, false, false);
  }

  const uint8_t data[] = { mode };
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_STANDBY, data, 1));
}

int16_t SX126x::setFs() {
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_FS, NULL, 0));
}

int16_t SX126x::setTx(uint32_t timeout) {
  const uint8_t data[] = { (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF)} ;
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_TX, data, 3));
}

int16_t SX126x::setRx(uint32_t timeout) {
  const uint8_t data[] = { (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF) };
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_RX, data, 3, true, false));
}

int16_t SX126x::setCad(uint8_t symbolNum, uint8_t detPeak, uint8_t detMin, uint8_t exitMode, RadioLibTime_t timeout) {
  // default CAD parameters are selected according to recommendations on Semtech DS.SX1261-2.W.APP rev. 1.1, page 92.

  // build the packet with default configuration
  uint8_t data[7];
  data[0] = RADIOLIB_SX126X_CAD_ON_4_SYMB;
  data[1] = this->spreadingFactor + 13;
  data[2] = RADIOLIB_SX126X_CAD_PARAM_DET_MIN;
  data[3] = RADIOLIB_SX126X_CAD_GOTO_STDBY;
  uint32_t timeout_raw = (float)timeout / 15.625f;
  data[4] = (uint8_t)((timeout_raw >> 16) & 0xFF);
  data[5] = (uint8_t)((timeout_raw >> 8) & 0xFF);
  data[6] = (uint8_t)(timeout_raw & 0xFF);

  // set user-provided values
  if(symbolNum != RADIOLIB_SX126X_CAD_PARAM_DEFAULT) {
    data[0] = symbolNum;
  }

  if(detPeak != RADIOLIB_SX126X_CAD_PARAM_DEFAULT) {
    data[1] = detPeak;
  }

  if(detMin != RADIOLIB_SX126X_CAD_PARAM_DEFAULT) {
    data[2] = detMin;
  }

  if(exitMode != RADIOLIB_SX126X_CAD_PARAM_DEFAULT) {
    data[3] = exitMode;
  }

  // configure parameters
  int16_t state = this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_CAD_PARAMS, data, 7);
  RADIOLIB_ASSERT(state);

  // start CAD
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_CAD, NULL, 0));
}

int16_t SX126x::setPaConfig(uint8_t paDutyCycle, uint8_t deviceSel, uint8_t hpMax, uint8_t paLut) {
  const uint8_t data[] = { paDutyCycle, hpMax, deviceSel, paLut };
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_PA_CONFIG, data, 4));
}

int16_t SX126x::writeRegister(uint16_t addr, const uint8_t* data, uint8_t numBytes) {
  this->mod->SPIwriteRegisterBurst(addr, data, numBytes);
  return(RADIOLIB_ERR_NONE);
}

int16_t SX126x::readRegister(uint16_t addr, uint8_t* data, uint8_t numBytes) {
  // send the command
  this->mod->SPIreadRegisterBurst(addr, numBytes, data);

  // check the status
  int16_t state = this->mod->SPIcheckStream();
  return(state);
}

int16_t SX126x::writeBuffer(const uint8_t* data, uint8_t numBytes, uint8_t offset) {
  const uint8_t cmd[] = { RADIOLIB_SX126X_CMD_WRITE_BUFFER, offset };
  return(this->mod->SPIwriteStream(cmd, 2, data, numBytes));
}

int16_t SX126x::readBuffer(uint8_t* data, uint8_t numBytes, uint8_t offset) {
  const uint8_t cmd[] = { RADIOLIB_SX126X_CMD_READ_BUFFER, offset };
  return(this->mod->SPIreadStream(cmd, 2, data, numBytes));
}

int16_t SX126x::setDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask) {
  const uint8_t data[8] = {(uint8_t)((irqMask >> 8) & 0xFF), (uint8_t)(irqMask & 0xFF),
                     (uint8_t)((dio1Mask >> 8) & 0xFF), (uint8_t)(dio1Mask & 0xFF),
                     (uint8_t)((dio2Mask >> 8) & 0xFF), (uint8_t)(dio2Mask & 0xFF),
                     (uint8_t)((dio3Mask >> 8) & 0xFF), (uint8_t)(dio3Mask & 0xFF)};
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_DIO_IRQ_PARAMS, data, 8));
}

int16_t SX126x::clearIrqStatus(uint16_t clearIrqParams) {
  const uint8_t data[] = { (uint8_t)((clearIrqParams >> 8) & 0xFF), (uint8_t)(clearIrqParams & 0xFF) };
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_CLEAR_IRQ_STATUS, data, 2));
}

int16_t SX126x::setRfFrequency(uint32_t frf) {
  const uint8_t data[] = { (uint8_t)((frf >> 24) & 0xFF), (uint8_t)((frf >> 16) & 0xFF), (uint8_t)((frf >> 8) & 0xFF), (uint8_t)(frf & 0xFF) };
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_RF_FREQUENCY, data, 4));
}

int16_t SX126x::calibrateImage(const uint8_t* data) {
  int16_t state = this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_CALIBRATE_IMAGE, data, 2);

  // if something failed, show the device errors
  #if RADIOLIB_DEBUG_BASIC
  if(state != RADIOLIB_ERR_NONE) {
    // unless mode is forced to standby, device errors will be 0
    standby();
    uint16_t errors = getDeviceErrors();
    RADIOLIB_DEBUG_BASIC_PRINTLN("Calibration failed, device errors: 0x%X", errors);
  }
  #endif
  return(state);
}

uint8_t SX126x::getPacketType() {
  uint8_t data = 0xFF;
  this->mod->SPIreadStream(RADIOLIB_SX126X_CMD_GET_PACKET_TYPE, &data, 1);
  return(data);
}

int16_t SX126x::setTxParams(uint8_t pwr, uint8_t rampTime) {
  const uint8_t data[] = { pwr, rampTime };
  int16_t state = this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_TX_PARAMS, data, 2);
  if(state == RADIOLIB_ERR_NONE) {
    this->pwr = pwr;
  }
  return(state);
}

int16_t SX126x::setModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro) {
  // calculate symbol length and enable low data rate optimization, if auto-configuration is enabled
  if(this->ldroAuto) {
    float symbolLength = (float)(uint32_t(1) << this->spreadingFactor) / (float)this->bandwidthKhz;
    if(symbolLength >= 16.0f) {
      this->ldrOptimize = RADIOLIB_SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_ON;
    } else {
      this->ldrOptimize = RADIOLIB_SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_OFF;
    }
  } else {
    this->ldrOptimize = ldro;
  }
  // 500/9/8  - 0x09 0x04 0x03 0x00 - SF9, BW125, 4/8
  // 500/11/8 - 0x0B 0x04 0x03 0x00 - SF11 BW125, 4/7
  const uint8_t data[4] = {sf, bw, cr, this->ldrOptimize};
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_MODULATION_PARAMS, data, 4));
}

int16_t SX126x::setModulationParamsFSK(uint32_t br, uint8_t sh, uint8_t rxBw, uint32_t freqDev) {
  const uint8_t data[8] = {(uint8_t)((br >> 16) & 0xFF), (uint8_t)((br >> 8) & 0xFF), (uint8_t)(br & 0xFF),
                     sh, rxBw,
                     (uint8_t)((freqDev >> 16) & 0xFF), (uint8_t)((freqDev >> 8) & 0xFF), (uint8_t)(freqDev & 0xFF)};
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_MODULATION_PARAMS, data, 8));
}

int16_t SX126x::setModulationParamsBPSK(uint32_t br, uint8_t sh) {
  const uint8_t data[] = {(uint8_t)((br >> 16) & 0xFF), (uint8_t)((br >> 8) & 0xFF), (uint8_t)(br & 0xFF), sh};
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_MODULATION_PARAMS, data, sizeof(data)));
}

int16_t SX126x::setPacketParams(uint16_t preambleLen, uint8_t crcType, uint8_t payloadLen, uint8_t hdrType, uint8_t invertIQ) {
  int16_t state = fixInvertedIQ(invertIQ);
  RADIOLIB_ASSERT(state);
  const uint8_t data[6] = {(uint8_t)((preambleLen >> 8) & 0xFF), (uint8_t)(preambleLen & 0xFF), hdrType, payloadLen, crcType, invertIQ};
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_PACKET_PARAMS, data, 6));
}

int16_t SX126x::setPacketParamsFSK(uint16_t preambleLen, uint8_t preambleDetectorLen, uint8_t crcType, uint8_t syncWordLen, uint8_t addrCmp, uint8_t whiten, uint8_t packType, uint8_t payloadLen) {
  const uint8_t data[9] = {(uint8_t)((preambleLen >> 8) & 0xFF), (uint8_t)(preambleLen & 0xFF),
                     preambleDetectorLen, syncWordLen, addrCmp,
                     packType, payloadLen, crcType, whiten};
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_PACKET_PARAMS, data, 9));
}

int16_t SX126x::setPacketParamsBPSK(uint8_t payloadLen, uint16_t rampUpDelay, uint16_t rampDownDelay, uint16_t payloadLenBits) {
  const uint8_t data[] = { payloadLen,
    (uint8_t)((rampUpDelay >> 8) & 0xFF), (uint8_t)(rampUpDelay & 0xFF),
    (uint8_t)((rampDownDelay >> 8) & 0xFF), (uint8_t)(rampDownDelay & 0xFF),
    (uint8_t)((payloadLenBits >> 8) & 0xFF), (uint8_t)(payloadLenBits & 0xFF)
  };
  
  // this one is a bit different, it seems to be split into command transaction and then a register write
  int16_t state = this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_PACKET_PARAMS, data, sizeof(uint8_t));
  RADIOLIB_ASSERT(state);
  return(this->writeRegister(RADIOLIB_SX126X_REG_BPSK_PACKET_PARAMS, &data[1], sizeof(data) - sizeof(uint8_t)));
}

int16_t SX126x::setBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress) {
  const uint8_t data[2] = {txBaseAddress, rxBaseAddress};
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_BUFFER_BASE_ADDRESS, data, 2));
}

int16_t SX126x::setRegulatorMode(uint8_t mode) {
  const uint8_t data[1] = {mode};
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_REGULATOR_MODE, data, 1));
}

uint8_t SX126x::getStatus() {
  uint8_t data = 0;
  this->mod->SPIreadStream(RADIOLIB_SX126X_CMD_GET_STATUS, &data, 0);
  return(data);
}

uint32_t SX126x::getPacketStatus() {
  uint8_t data[3] = {0, 0, 0};
  this->mod->SPIreadStream(RADIOLIB_SX126X_CMD_GET_PACKET_STATUS, data, 3);
  return((((uint32_t)data[0]) << 16) | (((uint32_t)data[1]) << 8) | (uint32_t)data[2]);
}

uint16_t SX126x::getDeviceErrors() {
  uint8_t data[2] = {0, 0};
  this->mod->SPIreadStream(RADIOLIB_SX126X_CMD_GET_DEVICE_ERRORS, data, 2);
  uint16_t opError = (((uint16_t)data[0] & 0xFF) << 8) | ((uint16_t)data[1]);
  return(opError);
}

int16_t SX126x::clearDeviceErrors() {
  const uint8_t data[2] = {RADIOLIB_SX126X_CMD_NOP, RADIOLIB_SX126X_CMD_NOP};
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_CLEAR_DEVICE_ERRORS, data, 2));
}

#endif