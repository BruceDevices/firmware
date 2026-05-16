#include "SX126x.h"

#include <math.h>
#include <string.h>

// this file contains all configuration methods
// of the SX126x, which let user control the
// modulation properties, packet configuration etc.

#if !RADIOLIB_EXCLUDE_SX126X

void SX126x::setDio1Action(void (*func)(void)) {
  this->mod->hal->attachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()), func, this->mod->hal->GpioInterruptRising);
}

void SX126x::clearDio1Action() {
  this->mod->hal->detachInterrupt(this->mod->hal->pinToInterrupt(this->mod->getIrq()));
}

void SX126x::setPacketReceivedAction(void (*func)(void)) {
  this->setDio1Action(func);
}

void SX126x::clearPacketReceivedAction() {
  this->clearDio1Action();
}

void SX126x::setPacketSentAction(void (*func)(void)) {
  this->setDio1Action(func);
}

void SX126x::clearPacketSentAction() {
  this->clearDio1Action();
}

void SX126x::setChannelScanAction(void (*func)(void)) {
  this->setDio1Action(func);
}

void SX126x::clearChannelScanAction() {
  this->clearDio1Action();
}

int16_t SX126x::setBandwidth(float bw) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // ensure byte conversion doesn't overflow
  RADIOLIB_CHECK_RANGE(bw, 0.0f, 510.0f, RADIOLIB_ERR_INVALID_BANDWIDTH);

  // check allowed bandwidth values
  uint8_t bw_div2 = bw / 2 + 0.01f;
  switch (bw_div2)  {
    case 3: // 7.8:
      this->bandwidth = RADIOLIB_SX126X_LORA_BW_7_8;
      break;
    case 5: // 10.4:
      this->bandwidth = RADIOLIB_SX126X_LORA_BW_10_4;
      break;
    case 7: // 15.6:
      this->bandwidth = RADIOLIB_SX126X_LORA_BW_15_6;
      break;
    case 10: // 20.8:
      this->bandwidth = RADIOLIB_SX126X_LORA_BW_20_8;
      break;
    case 15: // 31.25:
      this->bandwidth = RADIOLIB_SX126X_LORA_BW_31_25;
      break;
    case 20: // 41.7:
      this->bandwidth = RADIOLIB_SX126X_LORA_BW_41_7;
      break;
    case 31: // 62.5:
      this->bandwidth = RADIOLIB_SX126X_LORA_BW_62_5;
      break;
    case 62: // 125.0:
      this->bandwidth = RADIOLIB_SX126X_LORA_BW_125_0;
      break;
    case 125: // 250.0
      this->bandwidth = RADIOLIB_SX126X_LORA_BW_250_0;
      break;
    case 250: // 500.0
      this->bandwidth = RADIOLIB_SX126X_LORA_BW_500_0;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_BANDWIDTH);
  }

  // update modulation parameters
  this->bandwidthKhz = bw;
  return(setModulationParams(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t SX126x::setSpreadingFactor(uint8_t sf) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(sf, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);

  // update modulation parameters
  this->spreadingFactor = sf;
  return(setModulationParams(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t SX126x::setCodingRate(uint8_t cr, bool longInterleave) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
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
  return(setModulationParams(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t SX126x::setSyncWord(uint8_t syncWord, uint8_t controlBits) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update register
  const uint8_t data[2] = {(uint8_t)((syncWord & 0xF0) | ((controlBits & 0xF0) >> 4)), (uint8_t)(((syncWord & 0x0F) << 4) | (controlBits & 0x0F))};
  return(writeRegister(RADIOLIB_SX126X_REG_LORA_SYNC_WORD_MSB, data, 2));
}

int16_t SX126x::setCurrentLimit(float currentLimit) {
  // check allowed range
  if(!((currentLimit >= 0) && (currentLimit <= 140))) {
    return(RADIOLIB_ERR_INVALID_CURRENT_LIMIT);
  }

  // calculate raw value
  uint8_t rawLimit = (uint8_t)(currentLimit / 2.5f);

  // update register
  return(writeRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &rawLimit, 1));
}

float SX126x::getCurrentLimit() {
  // get the raw value
  uint8_t ocp = 0;
  readRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1);

  // return the actual value
  return((float)ocp * 2.5f);
}

int16_t SX126x::setPreambleLength(size_t preambleLength) {
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    this->preambleLengthLoRa = preambleLength;
    return(setPacketParams(this->preambleLengthLoRa, this->crcTypeLoRa, this->implicitLen, this->headerType, this->invertIQEnabled));
  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    this->preambleLengthFSK = preambleLength;
    // maximum preamble detector length is limited by sync word length
    // for details, see the note in SX1261 datasheet, Rev 2.1, section 6.2.2.1, page 45
    uint8_t maxDetLen = RADIOLIB_MIN(this->syncWordLength, this->preambleLengthFSK);
    this->preambleDetLength = maxDetLen >= 32 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_32 :
                              maxDetLen >= 24 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_24 :
                              maxDetLen >= 16 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_16 :
                              maxDetLen >   0 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_8 :
                              RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_OFF;
    return(setPacketParamsFSK(this->preambleLengthFSK, this->preambleDetLength, this->crcTypeFSK, this->syncWordLength, RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF, this->whitening, this->packetType));
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX126x::setFrequencyDeviation(float freqDev) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.0f) {
    newFreqDev = 0.6f;
  }

  RADIOLIB_CHECK_RANGE(newFreqDev, 0.6f, 200.0f, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);

  // calculate raw frequency deviation value
  uint32_t freqDevRaw = (uint32_t)(((newFreqDev * 1000.0f) * (float)((uint32_t)(1) << 25)) / (RADIOLIB_SX126X_CRYSTAL_FREQ * 1000000.0f));

  // check modulation parameters
  this->frequencyDev = freqDevRaw;

  // update modulation parameters
  return(setModulationParamsFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev));
}

int16_t SX126x::setBitRate(float br) {
  // check active modem
  uint8_t modem = getPacketType();
  if((modem != RADIOLIB_SX126X_PACKET_TYPE_GFSK) &&
     (modem != RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS) &&
     (modem != RADIOLIB_SX126X_PACKET_TYPE_BPSK)) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(modem == RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS) {
    // at the moment only the very specific 488.28125 bps rate is supported
    RADIOLIB_CHECK_RANGE(br, 0.488f, 0.489f, RADIOLIB_ERR_INVALID_BIT_RATE);
  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_BPSK) {
    // this should be just either 100 or 600 bps, not the range
    // but the BPSK support is so experimental it probably does not matter
    RADIOLIB_CHECK_RANGE(br, 0.1f, 0.6f, RADIOLIB_ERR_INVALID_BIT_RATE);
  }

  // calculate raw bit rate value
  uint32_t brRaw = (uint32_t)((RADIOLIB_SX126X_CRYSTAL_FREQ * 1000000.0f * 32.0f) / (br * 1000.0f));

  // check modulation parameters
  this->bitRate = brRaw;

  // update modulation parameters
  int16_t state = RADIOLIB_ERR_UNKNOWN;
  if(modem == RADIOLIB_SX126X_PACKET_TYPE_BPSK) {
    state = setModulationParamsBPSK(this->bitRate);
  } else {
    state = setModulationParamsFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev);
  }
  RADIOLIB_ASSERT(state);

  // apply workaround or reset it, as needed
  return(fixGFSK());
}

int16_t SX126x::setDataRate(DataRate_t dr, ModemType_t modem) {
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
    this->lrFhssGridNonFcc = dr.lrFhss.narrowGrid ? RADIOLIB_SX126X_LR_FHSS_GRID_STEP_NON_FCC : RADIOLIB_SX126X_LR_FHSS_GRID_STEP_FCC;

  }

  return(state);
}


int16_t SX126x::checkDataRate(DataRate_t dr, ModemType_t modem) {
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

int16_t SX126x::setRxBandwidth(float rxBw) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // check modulation parameters
  /*if(2 * this->frequencyDev + this->bitRate > rxBw * 1000.0) {
    return(RADIOLIB_ERR_INVALID_MODULATION_PARAMETERS);
  }*/
  this->rxBandwidthKhz = rxBw;

  // check allowed receiver bandwidth values
  if(fabsf(rxBw - 4.8f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_4_8;
  } else if(fabsf(rxBw - 5.8f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_5_8;
  } else if(fabsf(rxBw - 7.3f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_7_3;
  } else if(fabsf(rxBw - 9.7f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_9_7;
  } else if(fabsf(rxBw - 11.7f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_11_7;
  } else if(fabsf(rxBw - 14.6f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_14_6;
  } else if(fabsf(rxBw - 19.5f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_19_5;
  } else if(fabsf(rxBw - 23.4f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_23_4;
  } else if(fabsf(rxBw - 29.3f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_29_3;
  } else if(fabsf(rxBw - 39.0f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_39_0;
  } else if(fabsf(rxBw - 46.9f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_46_9;
  } else if(fabsf(rxBw - 58.6f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_58_6;
  } else if(fabsf(rxBw - 78.2f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_78_2;
  } else if(fabsf(rxBw - 93.8f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_93_8;
  } else if(fabsf(rxBw - 117.3f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_117_3;
  } else if(fabsf(rxBw - 156.2f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_156_2;
  } else if(fabsf(rxBw - 187.2f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_187_2;
  } else if(fabsf(rxBw - 234.3f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_234_3;
  } else if(fabsf(rxBw - 312.0f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_312_0;
  } else if(fabsf(rxBw - 373.6f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_373_6;
  } else if(fabsf(rxBw - 467.0f) <= 0.001f) {
    this->rxBandwidth = RADIOLIB_SX126X_GFSK_RX_BW_467_0;
  } else {
    return(RADIOLIB_ERR_INVALID_RX_BANDWIDTH);
  }

  // update modulation parameters
  return(setModulationParamsFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev));
}

int16_t SX126x::setRxBoostedGainMode(bool rxbgm, bool persist) {
  // update RX gain setting register
  uint8_t rxGain = rxbgm ? RADIOLIB_SX126X_RX_GAIN_BOOSTED : RADIOLIB_SX126X_RX_GAIN_POWER_SAVING;
  int16_t state = writeRegister(RADIOLIB_SX126X_REG_RX_GAIN, &rxGain, 1);
  RADIOLIB_ASSERT(state);

  // add Rx Gain register to retention memory if requested
  if(persist) {
    // values and registers below are specified in SX126x datasheet v2.1 section 9.6, just below table 9-3
    const uint8_t data[] = { 0x01, (uint8_t)((RADIOLIB_SX126X_REG_RX_GAIN >> 8) & 0xFF), (uint8_t)(RADIOLIB_SX126X_REG_RX_GAIN & 0xFF) };
    state = writeRegister(RADIOLIB_SX126X_REG_RX_GAIN_RETENTION_0, data, 3);
  }

  return(state);
}

int16_t SX126x::setDataShaping(uint8_t sh) {
  // check active modem
  uint8_t modem = getPacketType();
  if((modem != RADIOLIB_SX126X_PACKET_TYPE_GFSK) && (modem != RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS)) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      this->pulseShape = RADIOLIB_SX126X_GFSK_FILTER_NONE;
      break;
    case RADIOLIB_SHAPING_0_3:
      this->pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_0_3;
      break;
    case RADIOLIB_SHAPING_0_5:
      this->pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_0_5;
      break;
    case RADIOLIB_SHAPING_0_7:
      this->pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_0_7;
      break;
    case RADIOLIB_SHAPING_1_0:
      this->pulseShape = RADIOLIB_SX126X_GFSK_FILTER_GAUSS_1;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
  }

  // update modulation parameters
  return(setModulationParamsFSK(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev));
}

int16_t SX126x::setSyncWord(uint8_t* syncWord, size_t len) {
  // check active modem
  uint8_t modem = getPacketType();
  if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    // check sync word Length
    if(len > 8) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }

    // write sync word
    int16_t state = writeRegister(RADIOLIB_SX126X_REG_SYNC_WORD_0, syncWord, len);
    RADIOLIB_ASSERT(state);

    // update packet parameters
    this->syncWordLength = len * 8;
    
    // maximum preamble detector length is limited by sync word length
    // for details, see the note in SX1261 datasheet, Rev 2.1, section 6.2.2.1, page 45
    uint8_t maxDetLen = RADIOLIB_MIN(this->syncWordLength, this->preambleLengthFSK);
    this->preambleDetLength = maxDetLen >= 32 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_32 :
                              maxDetLen >= 24 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_24 :
                              maxDetLen >= 16 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_16 :
                              maxDetLen >   0 ? RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_8 :
                              RADIOLIB_SX126X_GFSK_PREAMBLE_DETECT_OFF;
    state = setPacketParamsFSK(this->preambleLengthFSK, this->preambleDetLength, this->crcTypeFSK, this->syncWordLength, RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF, this->whitening, this->packetType);

    return(state);
  
  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    // with length set to 1 and LoRa modem active, assume it is the LoRa sync word
    if(len > 1) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }
    return(setSyncWord(syncWord[0]));

  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_LR_FHSS) {
    // with length set to 4 and LR-FHSS modem active, assume it is the LR-FHSS sync word
    if(len != sizeof(uint32_t)) {
      return(RADIOLIB_ERR_INVALID_SYNC_WORD);
    }
    memcpy(this->lrFhssSyncWord, syncWord, sizeof(uint32_t));
  
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t SX126x::setCRC(uint8_t len, uint16_t initial, uint16_t polynomial, bool inverted) {
  // check active modem
  uint8_t modem = getPacketType();

  if(modem == RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    // update packet parameters
    switch(len) {
      case 0:
        this->crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_OFF;
        break;
      case 1:
        if(inverted) {
          this->crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_1_BYTE_INV;
        } else {
          this->crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_1_BYTE;
        }
        break;
      case 2:
        if(inverted) {
          this->crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_2_BYTE_INV;
        } else {
          this->crcTypeFSK = RADIOLIB_SX126X_GFSK_CRC_2_BYTE;
        }
        break;
      default:
        return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }

    int16_t state = setPacketParamsFSK(this->preambleLengthFSK, this->preambleDetLength, this->crcTypeFSK, this->syncWordLength, RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF, this->whitening, this->packetType);
    RADIOLIB_ASSERT(state);

    // write initial CRC value
    uint8_t data[2] = {(uint8_t)((initial >> 8) & 0xFF), (uint8_t)(initial & 0xFF)};
    state = writeRegister(RADIOLIB_SX126X_REG_CRC_INITIAL_MSB, data, 2);
    RADIOLIB_ASSERT(state);

    // write CRC polynomial value
    data[0] = (uint8_t)((polynomial >> 8) & 0xFF);
    data[1] = (uint8_t)(polynomial & 0xFF);
    state = writeRegister(RADIOLIB_SX126X_REG_CRC_POLYNOMIAL_MSB, data, 2);

    return(state);

  } else if(modem == RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    // LoRa CRC doesn't allow to set CRC polynomial, initial value, or inversion

    // update packet parameters
    if(len) {
      this->crcTypeLoRa = RADIOLIB_SX126X_LORA_CRC_ON;
    } else {
      this->crcTypeLoRa = RADIOLIB_SX126X_LORA_CRC_OFF;
    }

    return(setPacketParams(this->preambleLengthLoRa, this->crcTypeLoRa, this->implicitLen, this->headerType, this->invertIQEnabled));
  }

  return(RADIOLIB_ERR_UNKNOWN);
}

int16_t SX126x::setWhitening(bool enabled, uint16_t initial) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  int16_t state = RADIOLIB_ERR_NONE;
  if(!enabled) {
    // disable whitening
    this->whitening = RADIOLIB_SX126X_GFSK_WHITENING_OFF;

    state = setPacketParamsFSK(this->preambleLengthFSK, this->preambleDetLength, this->crcTypeFSK, this->syncWordLength, RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF, this->whitening, this->packetType);
    RADIOLIB_ASSERT(state);

  } else {
    // enable whitening
    this->whitening = RADIOLIB_SX126X_GFSK_WHITENING_ON;

    // write initial whitening value
    // as per note on pg. 65 of datasheet v1.2: "The user should not change the value of the 7 MSB's of this register"
    uint8_t data[2];
    // first read the actual value and mask 7 MSB which we can not change
    // if different value is written in 7 MSB, the Rx won't even work (tested on HW)
    state = readRegister(RADIOLIB_SX126X_REG_WHITENING_INITIAL_MSB, data, 1);
    RADIOLIB_ASSERT(state);

    data[0] = (data[0] & 0xFE) | (uint8_t)((initial >> 8) & 0x01);
    data[1] = (uint8_t)(initial & 0xFF);
    state = writeRegister(RADIOLIB_SX126X_REG_WHITENING_INITIAL_MSB, data, 2);
    RADIOLIB_ASSERT(state);

    state = setPacketParamsFSK(this->preambleLengthFSK, this->preambleDetLength, this->crcTypeFSK, this->syncWordLength, RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF, this->whitening, this->packetType);
    RADIOLIB_ASSERT(state);
  }
  return(state);
}

int16_t SX126x::fixedPacketLengthMode(uint8_t len) {
  return(setPacketMode(RADIOLIB_SX126X_GFSK_PACKET_FIXED, len));
}

int16_t SX126x::variablePacketLengthMode(uint8_t maxLen) {
  return(setPacketMode(RADIOLIB_SX126X_GFSK_PACKET_VARIABLE, maxLen));
}
int16_t SX126x::implicitHeader(size_t len) {
  return(setHeaderType(RADIOLIB_SX126X_LORA_HEADER_IMPLICIT, len));
}

int16_t SX126x::explicitHeader() {
  return(setHeaderType(RADIOLIB_SX126X_LORA_HEADER_EXPLICIT));
}

int16_t SX126x::setRegulatorLDO() {
  return(setRegulatorMode(RADIOLIB_SX126X_REGULATOR_LDO));
}

int16_t SX126x::setRegulatorDCDC() {
  return(setRegulatorMode(RADIOLIB_SX126X_REGULATOR_DC_DC));
}

int16_t SX126x::setEncoding(uint8_t encoding) {
  return(setWhitening(encoding));
}

void SX126x::setRfSwitchPins(uint32_t rxEn, uint32_t txEn) {
  this->mod->setRfSwitchPins(rxEn, txEn);
}

void SX126x::setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]) {
  this->mod->setRfSwitchTable(pins, table);
}

int16_t SX126x::forceLDRO(bool enable) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // update modulation parameters
  this->ldroAuto = false;
  this->ldrOptimize = (uint8_t)enable;
  return(setModulationParams(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t SX126x::autoLDRO() {
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  this->ldroAuto = true;
  return(RADIOLIB_ERR_NONE);
}

int16_t SX126x::invertIQ(bool enable) {
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  if(enable) {
    this->invertIQEnabled = RADIOLIB_SX126X_LORA_IQ_INVERTED;
  } else {
    this->invertIQEnabled = RADIOLIB_SX126X_LORA_IQ_STANDARD;
  }

  return(setPacketParams(this->preambleLengthLoRa, this->crcTypeLoRa, this->implicitLen, this->headerType, this->invertIQEnabled));
}

int16_t SX126x::setTCXO(float voltage, uint32_t delay) {
  // check if TCXO is enabled at all
  if(this->XTAL) {
    return(RADIOLIB_ERR_INVALID_TCXO_VOLTAGE);
  }

  // set mode to standby
  standby();

  // check RADIOLIB_SX126X_XOSC_START_ERR flag and clear it
  if(getDeviceErrors() & RADIOLIB_SX126X_XOSC_START_ERR) {
    clearDeviceErrors();
  }

  // check 0 V disable
  if(fabsf(voltage - 0.0f) <= 0.001f) {
    return(reset(true));
  }

  // check alowed voltage values
  uint8_t data[4];
  if(fabsf(voltage - 1.6f) <= 0.001f) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_1_6;
  } else if(fabsf(voltage - 1.7f) <= 0.001f) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_1_7;
  } else if(fabsf(voltage - 1.8f) <= 0.001f) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_1_8;
  } else if(fabsf(voltage - 2.2f) <= 0.001f) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_2_2;
  } else if(fabsf(voltage - 2.4f) <= 0.001f) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_2_4;
  } else if(fabsf(voltage - 2.7f) <= 0.001f) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_2_7;
  } else if(fabsf(voltage - 3.0f) <= 0.001f) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_3_0;
  } else if(fabsf(voltage - 3.3f) <= 0.001f) {
    data[0] = RADIOLIB_SX126X_DIO3_OUTPUT_3_3;
  } else {
    return(RADIOLIB_ERR_INVALID_TCXO_VOLTAGE);
  }

  // calculate delay
  uint32_t delayValue = (float)delay / 15.625f;
  data[1] = (uint8_t)((delayValue >> 16) & 0xFF);
  data[2] = (uint8_t)((delayValue >> 8) & 0xFF);
  data[3] = (uint8_t)(delayValue & 0xFF);

  this->tcxoDelay = delay;

  // enable TCXO control on DIO3
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, data, 4));
}

int16_t SX126x::setDio2AsRfSwitch(bool enable) {
  uint8_t data = enable ? RADIOLIB_SX126X_DIO2_AS_RF_SWITCH : RADIOLIB_SX126X_DIO2_AS_IRQ;
  return(this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL, &data, 1));
}

int16_t SX126x::setPaRampTime(uint8_t rampTime) {
  return(this->setTxParams(this->pwr, rampTime));
}

int16_t SX126x::setOutputPower(int8_t power, uint8_t paDutyCycle, uint8_t hpMax, uint8_t deviceSel) {
  // get current OCP configuration
  uint8_t ocp = 0;
  int16_t state = readRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1);
  RADIOLIB_ASSERT(state);

  // set PA config
  state = SX126x::setPaConfig(paDutyCycle, deviceSel, hpMax);
  RADIOLIB_ASSERT(state);

  // set output power with default 200us ramp
  state = SX126x::setTxParams(power, RADIOLIB_SX126X_PA_RAMP_200U);
  RADIOLIB_ASSERT(state);

  // restore OCP configuration
  return(writeRegister(RADIOLIB_SX126X_REG_OCP_CONFIGURATION, &ocp, 1));
}

int16_t SX126x::setPacketMode(uint8_t mode, uint8_t len) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  int16_t state = setPacketParamsFSK(this->preambleLengthFSK, this->preambleDetLength, this->crcTypeFSK, this->syncWordLength, RADIOLIB_SX126X_GFSK_ADDRESS_FILT_OFF, this->whitening, mode, len);
  RADIOLIB_ASSERT(state);

  // update cached value
  this->packetType = mode;
  return(state);
}

int16_t SX126x::setHeaderType(uint8_t hdrType, size_t len) {
  // check active modem
  if(getPacketType() != RADIOLIB_SX126X_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  int16_t state = setPacketParams(this->preambleLengthLoRa, this->crcTypeLoRa, len, hdrType, this->invertIQEnabled);
  RADIOLIB_ASSERT(state);

  // update cached value
  this->headerType = hdrType;
  this->implicitLen = len;

  return(state);
}

int16_t SX126x::setFrequencyRaw(float freq) {
  // calculate raw value
  this->freqMHz = freq;
  uint32_t frf = (this->freqMHz * (uint32_t(1) << RADIOLIB_SX126X_DIV_EXPONENT)) / RADIOLIB_SX126X_CRYSTAL_FREQ;
  return(setRfFrequency(frf));
}

int16_t SX126x::config(uint8_t modem) {
  // reset buffer base address
  int16_t state = setBufferBaseAddress();
  RADIOLIB_ASSERT(state);

  // set modem
  uint8_t data[7];
  data[0] = modem;
  state = this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_PACKET_TYPE, data, 1);
  RADIOLIB_ASSERT(state);

  // set Rx/Tx fallback mode to STDBY_RC
  data[0] = this->standbyXOSC ? RADIOLIB_SX126X_RX_TX_FALLBACK_MODE_STDBY_XOSC : RADIOLIB_SX126X_RX_TX_FALLBACK_MODE_STDBY_RC;
  state = this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_RX_TX_FALLBACK_MODE, data, 1);
  RADIOLIB_ASSERT(state);

  // set some CAD parameters - will be overwritten when calling CAD anyway
  data[0] = RADIOLIB_SX126X_CAD_ON_8_SYMB;
  data[1] = this->spreadingFactor + 13;
  data[2] = RADIOLIB_SX126X_CAD_PARAM_DET_MIN;
  data[3] = RADIOLIB_SX126X_CAD_GOTO_STDBY;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  state = this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_SET_CAD_PARAMS, data, 7);
  RADIOLIB_ASSERT(state);

  // clear IRQ
  state = clearIrqStatus();
  state |= setDioIrqParams(RADIOLIB_SX126X_IRQ_NONE, RADIOLIB_SX126X_IRQ_NONE);
  RADIOLIB_ASSERT(state);

  // calibrate all blocks
  data[0] = RADIOLIB_SX126X_CALIBRATE_ALL;
  state = this->mod->SPIwriteStream(RADIOLIB_SX126X_CMD_CALIBRATE, data, 1, true, false);
  RADIOLIB_ASSERT(state);

  // wait for calibration completion
  this->mod->hal->delay(5);
  while(this->mod->hal->digitalRead(this->mod->getGpio())) {
    this->mod->hal->yield();
  }

  // check calibration result
  return(this->mod->SPIcheckStream());
}

#endif
