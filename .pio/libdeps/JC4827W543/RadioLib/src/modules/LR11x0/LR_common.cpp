#include "LR_common.h"

#include <string.h>

LRxxxx::LRxxxx(Module* mod) : PhysicalLayer() {
  this->mod = mod;
  this->XTAL = false;
  this->mod->spiConfig.stream = true;
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_16;
  this->mod->spiConfig.statusPos = 0;
  this->mod->spiConfig.parseStatusCb = SPIparseStatus;
  this->mod->spiConfig.checkStatusCb = SPIcheckStatus;
}

void LRxxxx::setIrqAction(void (*func)(void)) {
  this->mod->hal->attachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()), func, this->mod->hal->GpioInterruptRising);
}

void LRxxxx::clearIrqAction() {
  this->mod->hal->detachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()));
}

void LRxxxx::setPacketReceivedAction(void (*func)(void)) {
  this->setIrqAction(func);
}

void LRxxxx::clearPacketReceivedAction() {
  this->clearIrqAction();
}

void LRxxxx::setPacketSentAction(void (*func)(void)) {
  this->setIrqAction(func);
}

void LRxxxx::clearPacketSentAction() {
  this->clearIrqAction();
}

uint32_t LRxxxx::getIrqStatus() {
  // there is no dedicated "get IRQ" command, the IRQ bits are sent after the status bytes
  uint8_t buff[6] = { 0 };
  Module::BitWidth_t statusWidth = mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS];
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_0;
  mod->SPItransferStream(NULL, 0, false, NULL, buff, sizeof(buff), true);
  this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = statusWidth;
  uint32_t irq = ((uint32_t)(buff[2]) << 24) | ((uint32_t)(buff[3]) << 16) | ((uint32_t)(buff[4]) << 8) | (uint32_t)buff[5];
  return(irq);
}

RadioLibTime_t LRxxxx::getTimeOnAir(size_t len, ModemType_t modem) {
  DataRate_t dr = {};
  PacketConfig_t pc = {};
  switch(modem) {
    case ModemType_t::RADIOLIB_MODEM_LORA: {
      uint8_t cr = this->codingRate;
      // We assume same calculation for short and long interleaving, so map CR values 0-4 and 5-7 to the same values
      if (cr < 5) {
        cr = cr + 4;
      } else if (cr == 7) {
        cr = cr + 1;
      }

      dr.lora.spreadingFactor = this->spreadingFactor;
      dr.lora.bandwidth = this->bandwidthKhz;
      dr.lora.codingRate = cr;

      pc.lora.preambleLength = this->preambleLengthLoRa;
      pc.lora.implicitHeader = (this->headerType == RADIOLIB_LRXXXX_LORA_HEADER_IMPLICIT);
      pc.lora.crcEnabled = (this->crcTypeLoRa == RADIOLIB_LRXXXX_LORA_CRC_ENABLED);
      pc.lora.ldrOptimize = (bool)this->ldrOptimize;
      break;
    }

    case ModemType_t::RADIOLIB_MODEM_FSK: {
      dr.fsk.bitRate = (float)this->bitRate / 1000.0f;
      dr.fsk.freqDev = (float)this->frequencyDev;
      pc.fsk.preambleLength = this->preambleLengthGFSK;
      pc.fsk.syncWordLength = this->syncWordLength; 
      pc.fsk.crcLength = this->crcLenGFSK;
      break;
    }

    case ModemType_t::RADIOLIB_MODEM_LRFHSS: {
      dr.lrFhss.bw = this->lrFhssBw;
      dr.lrFhss.cr = this->lrFhssCr;
      dr.lrFhss.narrowGrid = (this->lrFhssGrid == RADIOLIB_LRXXXX_LR_FHSS_GRID_STEP_NON_FCC) ? true : false;

      pc.lrFhss.hdrCount = this->lrFhssHdrCount;
      break;
    }

    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }

  return(this->calculateTimeOnAir(modem, dr, pc, len));
}

RadioLibTime_t LRxxxx::calculateTimeOnAir(ModemType_t modem, DataRate_t dr, PacketConfig_t pc, size_t len) {
  // check active modem
  if (modem == ModemType_t::RADIOLIB_MODEM_LORA) {  
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

    // get time-on-air in us
    return((symbolLength_us * nSymbol_x4) / 4);

  } else if(modem == ModemType_t::RADIOLIB_MODEM_FSK) {
    return((((float)(pc.fsk.crcLength * 8) + pc.fsk.syncWordLength + pc.fsk.preambleLength + (uint32_t)len * 8) / (dr.fsk.bitRate / 1000.0f)));

  } else if(modem == ModemType_t::RADIOLIB_MODEM_LRFHSS) {
    // calculate the number of bits based on coding rate
    uint16_t N_bits;
    switch(dr.lrFhss.cr) {
      case RADIOLIB_LRXXXX_LR_FHSS_CR_5_6:
        N_bits = ((len * 6) + 4) / 5; // this is from the official LR11xx driver, but why the extra +4?
        break;
      case RADIOLIB_LRXXXX_LR_FHSS_CR_2_3:
        N_bits = (len * 3) / 2;
        break;
      case RADIOLIB_LRXXXX_LR_FHSS_CR_1_2:
        N_bits = len * 2;
        break;
      case RADIOLIB_LRXXXX_LR_FHSS_CR_1_3:
        N_bits = len * 3;
        break;
      default:
        return(RADIOLIB_ERR_INVALID_CODING_RATE);
    }

    // calculate number of bits when accounting for unaligned last block
    uint16_t N_payBits = (N_bits / RADIOLIB_LRXXXX_LR_FHSS_FRAG_BITS) * RADIOLIB_LRXXXX_LR_FHSS_BLOCK_BITS;
    uint16_t N_lastBlockBits = N_bits % RADIOLIB_LRXXXX_LR_FHSS_FRAG_BITS;
    if(N_lastBlockBits) {
      N_payBits += N_lastBlockBits + 2;
    }

    // add header bits
    uint16_t N_totalBits = (RADIOLIB_LRXXXX_LR_FHSS_HEADER_BITS * pc.lrFhss.hdrCount) + N_payBits;
    return(((uint32_t)N_totalBits * 8 * 1000000UL) / RADIOLIB_LRXXXX_LR_FHSS_BIT_RATE);
  
  } else {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  return(0);
}

RadioLibTime_t LRxxxx::calculateRxTimeout(RadioLibTime_t timeoutUs) {
  // the timeout value is given in units of 30.52 microseconds
  // the calling function should provide some extra width, as this number of units is truncated to integer
  RadioLibTime_t timeout = timeoutUs / 30.52;
  return(timeout);
}

int16_t LRxxxx::reset() {
  // run the reset sequence
  this->mod->hal->pinMode(this->mod->getRst(), this->mod->hal->GpioModeOutput);
  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelLow);
  this->mod->hal->delay(10);
  this->mod->hal->digitalWrite(this->mod->getRst(), this->mod->hal->GpioLevelHigh);

  // the typical transition duration should be 273 ms
  this->mod->hal->delay(300);
  
  // wait for BUSY to go low
  RadioLibTime_t start = this->mod->hal->millis();
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();
    if(this->mod->hal->millis() - start >= 3000) {
      RADIOLIB_DEBUG_BASIC_PRINTLN("BUSY pin timeout after reset!");
      return(RADIOLIB_ERR_SPI_CMD_TIMEOUT);
    }
  }

  return(RADIOLIB_ERR_NONE);
}

int16_t LRxxxx::getStatus(uint8_t* stat1, uint8_t* stat2, uint32_t* irq) {
  uint8_t buff[6] = { 0 };

  // the status check command doesn't return status in the same place as other read commands
  // but only as the first byte (as with any other command), hence LRxxxx::SPIcommand can't be used
  // it also seems to ignore the actual command, and just sending in bunch of NOPs will work 
  int16_t state = this->mod->SPItransferStream(NULL, 0, false, NULL, buff, sizeof(buff), true);

  // pass the replies
  if(stat1) { *stat1 = buff[0]; }
  if(stat2) { *stat2 = buff[1]; }
  if(irq)   { *irq = ((uint32_t)(buff[2]) << 24) | ((uint32_t)(buff[3]) << 16) | ((uint32_t)(buff[4]) << 8) | (uint32_t)buff[5]; }

  return(state);
}

int16_t LRxxxx::lrFhssBuildFrame(uint16_t cmd, uint8_t hdrCount, uint8_t cr, uint8_t grid, uint8_t hop, uint8_t bw, uint16_t hopSeq, int8_t devOffset, const uint8_t* payload, size_t len) {
  // check maximum size
  const uint8_t maxLen[4][4] = {
    { 189, 178, 167, 155, },
    { 151, 142, 133, 123, },
    { 112, 105,  99,  92, },
    {  74,  69,  65,  60, },
  };
  if((cr > RADIOLIB_LRXXXX_LR_FHSS_CR_1_3) || ((hdrCount - 1) > (int)sizeof(maxLen[0])) || (len > maxLen[cr][hdrCount - 1])) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  }

  // build buffers
  size_t buffLen = 9 + len;
  #if RADIOLIB_STATIC_ONLY
    uint8_t dataBuff[9 + 190];
  #else
    uint8_t* dataBuff = new uint8_t[buffLen];
  #endif

  // set properties of the packet
  dataBuff[0] = hdrCount;
  dataBuff[1] = cr;
  dataBuff[2] = RADIOLIB_LRXXXX_LR_FHSS_MOD_TYPE_GMSK;
  dataBuff[3] = grid;
  dataBuff[4] = hop;
  dataBuff[5] = bw;
  dataBuff[6] = (uint8_t)((hopSeq >> 8) & 0x01);
  dataBuff[7] = (uint8_t)(hopSeq & 0xFF);
  dataBuff[8] = devOffset;
  memcpy(&dataBuff[9], payload, len);

  int16_t state = this->SPIcommand(cmd, true, dataBuff, buffLen);
  #if !RADIOLIB_STATIC_ONLY
    delete[] dataBuff;
  #endif
  return(state);
}

uint8_t LRxxxx::roundRampTime(uint32_t rampTimeUs) {
  uint8_t regVal;

  // Round up the ramp time to nearest discrete register value
  if(rampTimeUs <= 2) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_2U;
  } else if(rampTimeUs <= 4) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_4U;
  } else if(rampTimeUs <= 8) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_8U;
  } else if(rampTimeUs <= 16) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_16U;
  } else if(rampTimeUs <= 32) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_32U;
  } else if(rampTimeUs <= 48) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_48U;
  } else if(rampTimeUs <= 64) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_64U;
  } else if(rampTimeUs <= 80) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_80U;
  } else if(rampTimeUs <= 96) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_96U;
  } else if(rampTimeUs <= 112) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_112U;
  } else if(rampTimeUs <= 128) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_128U;
  } else if(rampTimeUs <= 144) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_144U;
  } else if(rampTimeUs <= 160) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_160U;
  } else if(rampTimeUs <= 176) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_176U;
  } else if(rampTimeUs <= 192) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_192U;
  } else if(rampTimeUs <= 208) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_208U;
  } else if(rampTimeUs <= 240) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_240U;
  } else if(rampTimeUs <= 272) {
    regVal = RADIOLIB_LRXXXX_PA_RAMP_272U;
  } else {  // 304
    regVal = RADIOLIB_LRXXXX_PA_RAMP_304U;
  }

  return regVal;
}

int16_t LRxxxx::findRxBw(float rxBw, const uint8_t* lut, size_t lutSize, float rxBwMax, uint8_t* val) {
  // lookup tables to avoid comparing a whole bunch of floats
  const uint16_t rxBwAvg[] = {
    53, 66, 85, 108, 134, 170, 211, 264,
    341, 424, 529, 682, 847, 1058, 1364,
    1695, 2116, 2729, 3390, 4233, 5159,
    6111, 7179, 9401, 16665, 24440, 28710,
  };

  // iterate through the table and find whether the user-provided value
  // is lower than the pre-computed average of the adjacent bandwidth values
  // if it is, we consider that to be a match even though the actual value is not precise
  uint16_t rxBwInt = rxBw*10.0f;
  for(size_t i = 0; i < lutSize; i++) {
    if(rxBwInt < rxBwAvg[i]) {
      *val = lut[i];
      return(RADIOLIB_ERR_NONE);
    }
  }

  // if nothing matched up to here, match with the last value
  if(rxBwInt <= rxBwMax*10) {
    *val = lut[lutSize - 1];
    return(RADIOLIB_ERR_NONE);
  }

  return(RADIOLIB_ERR_INVALID_RX_BANDWIDTH);
}

int16_t LRxxxx::setU32(uint16_t cmd, uint32_t u32) {
  uint8_t buff[] = { 
    (uint8_t)((u32 >> 24) & 0xFF), (uint8_t)((u32 >> 16) & 0xFF),
    (uint8_t)((u32 >> 8) & 0xFF), (uint8_t)(u32 & 0xFF),
  };
  return(this->SPIcommand(cmd, true, buff, sizeof(buff)));
}

int16_t LRxxxx::SPIparseStatus(uint8_t in) {
  if((in & 0b00001110) == RADIOLIB_LRXXXX_STAT_1_CMD_PERR) {
    return(RADIOLIB_ERR_SPI_CMD_INVALID);
  } else if((in & 0b00001110) == RADIOLIB_LRXXXX_STAT_1_CMD_FAIL) {
    return(RADIOLIB_ERR_SPI_CMD_FAILED);
  } else if((in == 0x00) || (in == 0xFF)) {
    return(RADIOLIB_ERR_CHIP_NOT_FOUND);
  }
  return(RADIOLIB_ERR_NONE);
}

int16_t LRxxxx::SPIcheckStatus(Module* mod) {
  // the status check command doesn't return status in the same place as other read commands,
  // but only as the first byte (as with any other command), hence LR11x0::SPIcommand can't be used
  // it also seems to ignore the actual command, and just sending in bunch of NOPs will work 
  uint8_t buff[6] = { 0 };
  Module::BitWidth_t statusWidth = mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS];
  mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = Module::BITS_0;
  int16_t state = mod->SPItransferStream(NULL, 0, false, NULL, buff, sizeof(buff), true);
  mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_STATUS] = statusWidth;
  RADIOLIB_ASSERT(state);
  return(LRxxxx::SPIparseStatus(buff[0]));
}

int16_t LRxxxx::writeCommon(uint16_t cmd, uint32_t addrOffset, const uint32_t* data, size_t len, bool nonvolatile) {
  // build buffers - later we need to ensure endians are correct, 
  // so there is probably no way to do this without copying buffers and iterating
  size_t buffLen = sizeof(uint32_t) + len*sizeof(uint32_t);
  #if RADIOLIB_STATIC_ONLY
    uint8_t dataBuff[sizeof(uint32_t) + RADIOLIB_LRXXXX_SPI_MAX_READ_WRITE_LEN];
  #else
    uint8_t* dataBuff = new uint8_t[buffLen];
  #endif

  // set the address or offset
  uint8_t* dataBuffPtr = reinterpret_cast<uint8_t*>(dataBuff);
  if(this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_ADDR] >= Module::BITS_32) {
    // LR2021 has 24-bit address, whereas LR11x0 has 32-bit
    *(dataBuffPtr++) = (uint8_t)((addrOffset >> 24) & 0xFF);
  }
  
  *(dataBuffPtr++) = (uint8_t)((addrOffset >> 16) & 0xFF);
  *(dataBuffPtr++) = (uint8_t)((addrOffset >> 8) & 0xFF);
  *(dataBuffPtr++) = (uint8_t)(addrOffset & 0xFF);

  // convert endians
  for(size_t i = 0; i < len; i++) {
    uint32_t bin = 0;
    if(nonvolatile) {
      uint32_t* ptr = const_cast<uint32_t*>(data) + i;
      bin = RADIOLIB_NONVOLATILE_READ_DWORD(ptr);
    } else {
      bin = data[i];
    }
    *(dataBuffPtr++) = (uint8_t)((bin >> 24) & 0xFF);
    *(dataBuffPtr++) = (uint8_t)((bin >> 16) & 0xFF);
    *(dataBuffPtr++) = (uint8_t)((bin >> 8) & 0xFF);
    *(dataBuffPtr++) = (uint8_t)(bin & 0xFF);
  }

  int16_t state = this->mod->SPIwriteStream(cmd, dataBuff, buffLen, true, false);
  #if !RADIOLIB_STATIC_ONLY
    delete[] dataBuff;
  #endif
  return(state);
}

int16_t LRxxxx::SPIcommand(uint16_t cmd, bool write, uint8_t* data, size_t len, const uint8_t* out, size_t outLen) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;
  if(!write) {
    // the SPI interface of LR11x0 requires two separate transactions for reading
    // send the 16-bit command
    state = this->mod->SPIwriteStream(cmd, out, outLen, true, false);
    RADIOLIB_ASSERT(state);

    // read the result without command
    this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_0;
    state = this->mod->SPIreadStream(RADIOLIB_LRXXXX_CMD_NOP, data, len, true, false);
    this->mod->spiConfig.widths[RADIOLIB_MODULE_SPI_WIDTH_CMD] = Module::BITS_16;

  } else {
    // write is just a single transaction
    state = this->mod->SPIwriteStream(cmd, data, len, true, true);
  
  }
  
  return(state);
}
