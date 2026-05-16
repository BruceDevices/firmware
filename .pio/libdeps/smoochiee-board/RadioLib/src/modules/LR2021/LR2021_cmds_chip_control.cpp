#include "LR2021.h"

#include "../LR11x0/LR_common.h"

#include <string.h>
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR2021

int16_t LR2021::readRadioRxFifo(uint8_t* data, size_t len) {
  // FIFO read is just a single transaction sent without the status code
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_0;
  int16_t state = this->mod->SPIreadStream(RADIOLIB_LR2021_CMD_READ_RX_FIFO, data, len, true, false);
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_16;
  return(state);
}

int16_t LR2021::writeRadioTxFifo(const uint8_t* data, size_t len) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_WRITE_TX_FIFO, true, const_cast<uint8_t*>(data), len, NULL, 0));
}

int16_t LR2021::writeRegMem32(uint32_t addr, const uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }
  return(this->writeCommon(RADIOLIB_LR2021_CMD_WRITE_REG_MEM_32, addr, data, len, false));
}

int16_t LR2021::writeRegMemMask32(uint32_t addr, uint32_t mask, uint32_t data) {
  uint8_t buff[11] = {
    (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
    (uint8_t)((mask >> 24) & 0xFF), (uint8_t)((mask >> 16) & 0xFF), (uint8_t)((mask >> 8) & 0xFF), (uint8_t)(mask & 0xFF),
    (uint8_t)((data >> 24) & 0xFF), (uint8_t)((data >> 16) & 0xFF), (uint8_t)((data >> 8) & 0xFF), (uint8_t)(data & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_WRITE_REG_MEM_MASK_32, true, buff, sizeof(buff)));
}

int16_t LR2021::readRegMem32(uint32_t addr, uint32_t* data, size_t len) {
  // check maximum size
  if(len > (RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN/sizeof(uint32_t))) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // the request contains the address and length
  uint8_t reqBuff[4] = {
    (uint8_t)((addr >> 16) & 0xFF), (uint8_t)((addr >> 8) & 0xFF),
    (uint8_t)(addr & 0xFF), (uint8_t)len,
  };

  // build buffers - later we need to ensure endians are correct, 
  // so there is probably no way to do this without copying buffers and iterating
  #if RADIOLIB_STATIC_ONLY
    uint8_t rplBuff[RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* rplBuff = new uint8_t[len*sizeof(uint32_t)];
  #endif

  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_READ_REG_MEM_32, false, rplBuff, len*sizeof(uint32_t), reqBuff, sizeof(reqBuff));

  // convert endians
  if(data && (state == RADIOLIB_ERR_NONE)) {
    for(size_t i = 0; i < len; i++) {
      data[i] = ((uint32_t)rplBuff[2 + i*sizeof(uint32_t)] << 24) | ((uint32_t)rplBuff[3 + i*sizeof(uint32_t)] << 16) | ((uint32_t)rplBuff[4 + i*sizeof(uint32_t)] << 8) | (uint32_t)rplBuff[5 + i*sizeof(uint32_t)];
    }
  }

  #if !RADIOLIB_STATIC_ONLY
    delete[] rplBuff;
  #endif
  
  return(state);
}

int16_t LR2021::setFs(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_FS, true, NULL, 0));
}

int16_t LR2021::setAdditionalRegToRetain(uint8_t slot, uint32_t addr) {
  uint8_t buff[] = { 
    slot, (uint8_t)((addr >> 16) & 0xFF),
    (uint8_t)((addr >> 8) & 0xFF), (uint8_t)(addr & 0xFF),
  };

  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_ADDITIONAL_REG_TO_RETAIN, true, buff, sizeof(buff)));
}

int16_t LR2021::setRx(uint32_t timeout) {
  uint8_t buff[] = {
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RX, true, buff, sizeof(buff)));
}

int16_t LR2021::setTx(uint32_t timeout) {
  uint8_t buff[] = {
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_TX, true, buff, sizeof(buff)));
}

int16_t LR2021::setRxTxFallbackMode(uint8_t mode) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RX_TX_FALLBACK_MODE, true, &mode, sizeof(mode)));
}

int16_t LR2021::setRxDutyCycle(uint32_t rxMaxTime, uint32_t cycleTime, uint8_t cfg) {
  uint8_t buff[] = {
    (uint8_t)((rxMaxTime >> 16) & 0xFF), (uint8_t)((rxMaxTime >> 8) & 0xFF), (uint8_t)(rxMaxTime & 0xFF),
    (uint8_t)((cycleTime >> 16) & 0xFF), (uint8_t)((cycleTime >> 8) & 0xFF), (uint8_t)(cycleTime & 0xFF),
    cfg
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_RX_DUTY_CYCLE, true, buff, sizeof(buff)));
}

int16_t LR2021::autoTxRx(uint32_t delay, uint8_t mode, uint32_t timeout) {
  uint8_t buff[] = {
    (uint8_t)((delay >> 16) & 0xFF), (uint8_t)((delay >> 8) & 0xFF), (uint8_t)(delay & 0xFF), mode,
    (uint8_t)((timeout >> 16) & 0xFF), (uint8_t)((timeout >> 8) & 0xFF), (uint8_t)(timeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_AUTO_RX_TX, true, buff, sizeof(buff)));
}

int16_t LR2021::getRxPktLength(uint16_t* len) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_RX_PKT_LENGTH, false, buff, sizeof(buff));
  if(len) { *len = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1]; }
  return(state);
}

int16_t LR2021::resetRxStats(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_RESET_RX_STATS, true, NULL, 0));
}

int16_t LR2021::setDefaultRxTxTimeout(uint32_t rxTimeout, uint32_t txTimeout) {
  uint8_t buff[] = {
    (uint8_t)((rxTimeout >> 16) & 0xFF), (uint8_t)((rxTimeout >> 8) & 0xFF), (uint8_t)(rxTimeout & 0xFF),
    (uint8_t)((txTimeout >> 16) & 0xFF), (uint8_t)((txTimeout >> 8) & 0xFF), (uint8_t)(txTimeout & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_DEFAULT_RX_TX_TIMEOUT, true, buff, sizeof(buff)));
}

int16_t LR2021::setRegMode(uint8_t simoUsage, const uint8_t rampTimes[4]) {
  uint8_t buff[] = { simoUsage, 
    rampTimes[RADIOLIB_LR2021_REG_MODE_RAMP_INDEX_RC2RU], rampTimes[RADIOLIB_LR2021_REG_MODE_RAMP_INDEX_TX2RU], 
    rampTimes[RADIOLIB_LR2021_REG_MODE_RAMP_INDEX_RU2RC], rampTimes[RADIOLIB_LR2021_REG_MODE_RAMP_INDEX_RAMP_DOWN],
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_REG_MODE, true, buff, sizeof(buff)));
}

int16_t LR2021::calibrate(uint8_t blocks) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CALIBRATE, true, &blocks, sizeof(blocks)));
}

int16_t LR2021::calibrateFrontEnd(const uint16_t freq[3]) {
  uint8_t buff[] = {
    (uint8_t)((freq[0] >> 8) & 0xFF), (uint8_t)(freq[0] & 0xFF),
    (uint8_t)((freq[1] >> 8) & 0xFF), (uint8_t)(freq[1] & 0xFF),
    (uint8_t)((freq[2] >> 8) & 0xFF), (uint8_t)(freq[2] & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CALIB_FRONT_END, true, buff, sizeof(buff)));
}

int16_t LR2021::getVbat(uint8_t resolution, uint16_t* vbat) {
  uint8_t reqBuff[] = { (uint8_t)(RADIOLIB_LR2021_VBAT_FORMAT_MV | ((RADIOLIB_LR2021_MEAS_RESOLUTION_OFFSET + resolution) & 0x07)) };
  uint8_t rplBuff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_V_BAT, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  if(vbat) { *vbat = ((uint16_t)(rplBuff[0]) << 8) | (uint16_t)rplBuff[1]; }
  return(state);
}

int16_t LR2021::getTemp(uint8_t source, uint8_t resolution, float* temp) {
  uint8_t reqBuff[] = { (uint8_t)((source & 0x30) | RADIOLIB_LR2021_TEMP_FORMAT_DEG_C | ((RADIOLIB_LR2021_MEAS_RESOLUTION_OFFSET + resolution) & 0x07)) };
  uint8_t rplBuff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_TEMP, false, rplBuff, sizeof(rplBuff), reqBuff, sizeof(reqBuff));
  if(temp) { 
    uint16_t raw = ((uint16_t)(rplBuff[0]) << 8) | (uint16_t)rplBuff[1];
    *temp = (float)raw/320.0f;
  }
  return(state);
}

int16_t LR2021::setEolConfig(bool enable, uint8_t trim) {
  uint8_t buff[] = { (uint8_t)((trim & 0x06) | (uint8_t)enable) };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_EOL_CONFIG, true, buff, sizeof(buff)));
}

int16_t LR2021::getRandomNumber(uint32_t* rnd) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_RANDOM_NUMBER, false, buff, sizeof(buff));
  if(rnd) { *rnd = ((uint32_t)(buff[0]) << 24) | ((uint32_t)(buff[1]) << 16) | ((uint32_t)(buff[2]) << 8) | (uint32_t)buff[3];  }
  return(state);
}

int16_t LR2021::getVersion(uint8_t* major, uint8_t* minor) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_VERSION, false, buff, sizeof(buff));
  if(major)   { *major = buff[0]; }
  if(minor)   { *minor = buff[1]; }
  return(state);
}

int16_t LR2021::clearErrors(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CLEAR_ERRORS, true, NULL, 0));
}

int16_t LR2021::getErrors(uint16_t* err) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_ERRORS, false, buff, sizeof(buff));
  if(err) { *err = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];  }
  return(state);
}

int16_t LR2021::setDioFunction(uint8_t dio, uint8_t func, uint8_t pullDrive) {
  uint8_t buff[] = { dio, (uint8_t)((func & 0xF0) | (pullDrive & 0x0F)) };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_DIO_FUNCTION, true, buff, sizeof(buff)));
}

int16_t LR2021::setDioRfSwitchConfig(uint8_t dio, uint8_t func) {
  uint8_t buff[] = { dio, func };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_DIO_RF_SWITCH_CONFIG, true, buff, sizeof(buff)));
}

int16_t LR2021::setDioIrqConfig(uint8_t dio, uint32_t irq) {
  uint8_t buff[] = { dio, 
    (uint8_t)((irq >> 24) & 0xFF), (uint8_t)((irq >> 16) & 0xFF),
    (uint8_t)((irq >> 8) & 0xFF), (uint8_t)(irq & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_DIO_IRQ_CONFIG, true, buff, sizeof(buff)));
}

int16_t LR2021::clearIrqState(uint32_t irq) {
  return(this->setU32(RADIOLIB_LR2021_CMD_CLEAR_IRQ, irq));
}

int16_t LR2021::getAndClearIrqStatus(uint32_t* irq) {
  uint8_t buff[4] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_AND_CLEAR_IRQ_STATUS, false, buff, sizeof(buff));
  if(irq) { *irq = ((uint32_t)(buff[0]) << 24) | ((uint32_t)(buff[1]) << 16) | ((uint32_t)(buff[2]) << 8) |(uint32_t)buff[3]; }
  return(state);
}

int16_t LR2021::configFifoIrq(uint8_t rxFifoIrq, uint8_t txFifoIrq, uint8_t rxHighThreshold, uint8_t txHighThreshold) {
  uint8_t buff[] = { rxFifoIrq, txFifoIrq, rxHighThreshold, txHighThreshold };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CONFIG_FIFO_IRQ, true, buff, sizeof(buff)));
}

int16_t LR2021::getFifoIrqFlags(uint8_t* rxFifoFlags, uint8_t* txFifoFlags) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_FIFO_IRQ_FLAGS, false, buff, sizeof(buff));
  if(rxFifoFlags) { *rxFifoFlags = buff[0]; }
  if(txFifoFlags) { *txFifoFlags = buff[1]; }
  return(state);
}

int16_t LR2021::clearFifoIrqFlags(uint8_t rxFifoFlags, uint8_t txFifoFlags) {
  uint8_t buff[] = { rxFifoFlags, txFifoFlags };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CLEAR_FIFO_IRQ_FLAGS, true, buff, sizeof(buff)));
}

int16_t LR2021::getAndClearFifoIrqFlags(uint8_t* rxFifoFlags, uint8_t* txFifoFlags) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_AND_CLEAR_FIFO_IRQ_FLAGS, false, buff, sizeof(buff));
  if(rxFifoFlags) { *rxFifoFlags = buff[0]; }
  if(txFifoFlags) { *txFifoFlags = buff[1]; }
  return(state);
}

int16_t LR2021::getRxFifoLevel(uint16_t* level) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_RX_FIFO_LEVEL, false, buff, sizeof(buff));
  if(level) { *level = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];  }
  return(state);
}

int16_t LR2021::getTxFifoLevel(uint16_t* level) {
  uint8_t buff[2] = { 0 };
  int16_t state = this->SPIcommand(RADIOLIB_LR2021_CMD_GET_TX_FIFO_LEVEL, false, buff, sizeof(buff));
  if(level) { *level = ((uint16_t)(buff[0]) << 8) | (uint16_t)buff[1];  }
  return(state);
}

int16_t LR2021::clearRxFifo(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CLEAR_RX_FIFO, true, NULL, 0));
}

int16_t LR2021::clearTxFifo(void) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CLEAR_TX_FIFO, true, NULL, 0));
}

int16_t LR2021::configLfClock(uint8_t cfg) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CONFIG_LF_CLOCK, true, &cfg, sizeof(cfg)));
}

int16_t LR2021::configClkOutputs(uint8_t scaling) {
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_CONFIG_CLK_OUTPUTS, true, &scaling, sizeof(scaling)));
}

int16_t LR2021::setTcxoMode(uint8_t tune, uint32_t startTime) {
  uint8_t buff[] = { (uint8_t)(tune & 0x07),
    (uint8_t)((startTime >> 24) & 0xFF), (uint8_t)((startTime >> 16) & 0xFF),
    (uint8_t)((startTime >> 8) & 0xFF), (uint8_t)(startTime & 0xFF),
  };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_TCXO_MODE, true, buff, sizeof(buff)));
}

int16_t LR2021::setXoscCpTrim(uint8_t xta, uint8_t xtb, uint8_t startTime) {
  uint8_t buff[] = { (uint8_t)(xta & 0x3F), (uint8_t)(xtb & 0x3F), startTime };
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_XOSC_CP_TRIM, true, buff, sizeof(buff)));
}

#endif
