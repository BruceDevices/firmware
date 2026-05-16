#include "LR2021.h"

#include "../LR11x0/LR_common.h"
#include "LR2021_registers.h"

#include <math.h>
#include <string.h>

#if !RADIOLIB_EXCLUDE_LR2021

// maximum number of allowed frontend calibration attempts
#define RADIOLIB_LR2021_MAX_CAL_ATTEMPTS    (10)

int16_t LR2021::setFrequency(float freq) {
  return(this->setFrequency(freq, false));
}

int16_t LR2021::setFrequency(float freq, bool skipCalibration) {
  #if RADIOLIB_CHECK_PARAMS
  if(!(((freq >= 150.0f) && (freq <= 1090.0f)) ||
    ((freq >= 1900.0f) && (freq <= 2200.0f)) ||
    ((freq >= 2400.0f) && (freq <= 2500.0f)))) {
      return(RADIOLIB_ERR_INVALID_FREQUENCY);
  }
  #endif

  // check if we need to recalibrate image
  int16_t state;
  if(!skipCalibration && (fabsf(freq - this->freqMHz) >= RADIOLIB_LR2021_CAL_IMG_FREQ_TRIG_MHZ)) {
    // calibration can fail if there is a strong interfering source
    // run it several times until it passes
    int i = 0;
    for(; i < RADIOLIB_LR2021_MAX_CAL_ATTEMPTS; i++) {
      // get the nearest multiple of 4 MHz
      uint16_t frequencies[3] = { (uint16_t)((freq / 4.0f) + 0.5f), 0, 0 };
      frequencies[0] |= (freq > RADIOLIB_LR2021_LF_CUTOFF_FREQ) ? RADIOLIB_LR2021_CALIBRATE_FE_HF_PATH : RADIOLIB_LR2021_CALIBRATE_FE_LF_PATH;
      state = calibrateFrontEnd(const_cast<const uint16_t*>(frequencies));

      // if something failed, check the device errors
      if(state != RADIOLIB_ERR_NONE) {
        uint16_t errors = 0;
        getErrors(&errors);
        RADIOLIB_DEBUG_BASIC_PRINTLN("Frontend calibration #%d failed, device errors: 0x%X", i, errors);

        // if this is caused by something else than RSSI saturation, repeating will not help
        if((errors & RADIOLIB_LR2021_SRC_SATURATION_CALIB_ERR) == 0) {
          return(state);
        }

        // wait a little while before the next attempt
        this->mod->hal->delay(5);
      
      } else {
        // calibration passed
        break;
      }

    }

    if(i == RADIOLIB_LR2021_MAX_CAL_ATTEMPTS) {
      return(RADIOLIB_ERR_FRONTEND_CALIBRATION_FAILED);
    }

  }

  // set frequency
  state = setRfFrequency((uint32_t)(freq*1000000.0f));
  RADIOLIB_ASSERT(state);
  this->freqMHz = freq;
  this->highFreq = (freq > RADIOLIB_LR2021_LF_CUTOFF_FREQ);
  return(state);
}

int16_t LR2021::setOutputPower(int8_t power) {
  return(this->setOutputPower(power, 48));
}

int16_t LR2021::setOutputPower(int8_t power, uint32_t rampTimeUs) {
  // check if power value is configurable
  int16_t state = this->checkOutputPower(power, NULL);
  RADIOLIB_ASSERT(state);
  
  //! \TODO: [LR2021] how and when to configure OCP?
  //! \TODO: [LR2021] Determine the optimal PA configuration
  // update PA config
  state = setPaConfig(this->highFreq, 
    RADIOLIB_LR2021_PA_LF_MODE_FSM, 
    RADIOLIB_LR2021_PA_LF_DUTY_CYCLE_UNUSED, 
    RADIOLIB_LR2021_PA_LF_SLICES_UNUSED, 
    RADIOLIB_LR2021_PA_HF_DUTY_CYCLE_UNUSED);
  RADIOLIB_ASSERT(state);

  // set output power
  state = setTxParams(power, roundRampTime(rampTimeUs));
  return(state);
}

int16_t LR2021::checkOutputPower(int8_t power, int8_t* clipped) {
  if(this->highFreq) {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-19, RADIOLIB_MIN(12, power));
    }
    RADIOLIB_CHECK_RANGE(power, -19, 12, RADIOLIB_ERR_INVALID_OUTPUT_POWER);

  } else {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-9, RADIOLIB_MIN(22, power));
    }
    RADIOLIB_CHECK_RANGE(power, -9, 22, RADIOLIB_ERR_INVALID_OUTPUT_POWER);

  }
  
  return(RADIOLIB_ERR_NONE);
}

void LR2021::setRfSwitchTable(const uint32_t (&pins)[Module::RFSWITCH_MAX_PINS], const Module::RfSwitchMode_t table[]) {
  // find which pins are used
  // on LR2021, modes are configured per DIO (exact opposite of LR11x0)
  uint8_t dioConfigs[7] = { 0 };
  for(size_t i = 0; i < Module::RFSWITCH_MAX_PINS; i++) {
    // check if this pin is unused
    if(pins[i] == RADIOLIB_NC) { continue; }

    // only keep DIO pins, there may be some GPIOs in the switch tabke
    if((pins[i] & RFSWITCH_PIN_FLAG) == 0) { continue; }
    
    // find modes in which this pin is used
    uint32_t dioNum = RADIOLIB_LRXXXX_DIOx_VAL(pins[i]);
    size_t j = 0;
    while(table[j].mode != LR2021::MODE_END_OF_TABLE) {
      dioConfigs[dioNum] |= (table[j].values[i] == this->mod->hal->GpioLevelHigh) ? (1UL << (table[j].mode - 1)) : 0;
      j++;
    }

    // For DIO = 5, only DIO_SLEEP_PULL_UP is accepted. Otherwise the SetDioFunction returns FAIL.
    uint8_t pull = dioNum == 0 ? RADIOLIB_LR2021_DIO_SLEEP_PULL_UP : RADIOLIB_LR2021_DIO_SLEEP_PULL_AUTO;
    // enable RF control for this pin and set the modes in which it is active
    (void)this->setDioFunction(dioNum + 5, RADIOLIB_LR2021_DIO_FUNCTION_RF_SWITCH, pull);
    (void)this->setDioRfSwitchConfig(dioNum + 5, dioConfigs[i]);
  }
}

int16_t LR2021::setBandwidth(float bw) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
    
  // convert to int to avoid bunch of math
  int bw_div2 = bw / 2 + 0.01f;

  // check allowed bandwidth values
  switch (bw_div2)  {
    case 15:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_31;
      break;
    case 20:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_41;
      break;
    case 31:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_62;
      break;
    case 41:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_83;
      break;
    case 50:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_101;
      break;
    case 101:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_203;
      break;
    case 62:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_125;
      break;
    case 125:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_250;
      break;
    case 203:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_406;
      break;
    case 250:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_500;
      break;
    case 406:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_812;
      break;
    case 500:
      this->bandwidth = RADIOLIB_LR2021_LORA_BW_1000;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_BANDWIDTH);
  }

  // update modulation parameters
  this->bandwidthKhz = bw;
  return(setLoRaModulationParams(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR2021::setSpreadingFactor(uint8_t sf, bool legacy) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  RADIOLIB_CHECK_RANGE(sf, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);

  //! \TODO: [LR2021] enable SF6 legacy mode
  if(legacy && (sf == 6)) {
    //this->mod->SPIsetRegValue(RADIOLIB_LR11X0_REG_SF6_SX127X_COMPAT, RADIOLIB_LR11X0_SF6_SX127X, 18, 18);
  }

  // update modulation parameters
  this->spreadingFactor = sf;
  return(setLoRaModulationParams(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));
}

int16_t LR2021::setCodingRate(uint8_t cr, bool longInterleave) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR2021_PACKET_TYPE_LORA) {
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
    return(setLoRaModulationParams(this->spreadingFactor, this->bandwidth, this->codingRate, this->ldrOptimize));

  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_FLRC) {
    if(cr > RADIOLIB_LR2021_FLRC_CR_2_3) {
      return(RADIOLIB_ERR_INVALID_CODING_RATE);
    }

    // update modulation parameters
    this->codingRateFlrc = cr;
    return(setFlrcModulationParams(this->bitRateFlrc, this->codingRateFlrc, this->pulseShape));
  
  }
  
  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR2021::setSyncWord(uint8_t syncWord) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }
  
  return(setLoRaSyncword(syncWord));
}

int16_t LR2021::setPreambleLength(size_t preambleLength) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    this->preambleLengthLoRa = preambleLength;
    return(setLoRaPacketParams(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled));
  
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
    this->preambleLengthGFSK = preambleLength;
    this->preambleDetLength = (preambleLength / 8) << 3;
    return(setGfskPacketParams(this->preambleLengthGFSK, this->preambleDetLength, false, false, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
  
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_OOK) {
    this->preambleLengthGFSK = preambleLength;
    this->preambleDetLength = (preambleLength / 8) << 3;
    return(setOokPacketParams(this->preambleLengthGFSK, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
    
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_FLRC) {
    if((preambleLength % 4) != 0) {
      return(RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH);
    }
    this->preambleLengthGFSK = (preambleLength / 4) - 1;
    return(setFlrcPacketParams(this->preambleLengthGFSK, this->syncWordLength, 1, 0x01, this->packetType == RADIOLIB_LR2021_GFSK_OOK_PACKET_FORMAT_FIXED, this->crcLenGFSK, RADIOLIB_LR2021_MAX_PACKET_LENGTH));

  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR2021::setTCXO(float voltage, uint32_t delay) {
  // check if TCXO is enabled at all
  if(this->XTAL) {
    return(RADIOLIB_ERR_INVALID_TCXO_VOLTAGE);
  }

  // set mode to standby
  standby();

  // check oscillator startup error flag and clear it
  uint16_t errors = 0;
  int16_t state = getErrors(&errors);
  RADIOLIB_ASSERT(state);
  if(errors & RADIOLIB_LR2021_HF_XOSC_START_ERR) {
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

int16_t LR2021::setCRC(uint8_t len, uint32_t initial, uint32_t polynomial, bool inverted) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    // LoRa CRC doesn't allow to set CRC polynomial, initial value, or inversion
    this->crcTypeLoRa = len > 0;
    return(setLoRaPacketParams(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled));
  
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
    if(len > 4) {
      return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }

    this->crcTypeGFSK = len;
    if((this->crcTypeGFSK != RADIOLIB_LR2021_GFSK_OOK_CRC_OFF) && inverted) {
      this->crcTypeGFSK += 0x08;
    }

    state = setGfskPacketParams(this->preambleLengthGFSK, this->preambleDetLength, false, false, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
    RADIOLIB_ASSERT(state);

    return(setGfskCrcParams(polynomial, initial));

  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_OOK) {
    if(len > 4) {
      return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }

    this->crcTypeGFSK = len;
    if((this->crcTypeGFSK != RADIOLIB_LR2021_GFSK_OOK_CRC_OFF) && inverted) {
      this->crcTypeGFSK += 0x08;
    }

    state = setOokPacketParams(this->preambleLengthGFSK, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
    RADIOLIB_ASSERT(state);

    return(setOokCrcParams(polynomial, initial));
  
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_FLRC) {
    if((len == 1) || (len > 4)) {
      return(RADIOLIB_ERR_INVALID_CRC_CONFIGURATION);
    }
    
    this->crcLenGFSK = len ? len - 1 : 0;
    return(setFlrcPacketParams(this->preambleLengthGFSK, this->syncWordLength, 1, 0x01, this->packetType == RADIOLIB_LR2021_GFSK_OOK_PACKET_FORMAT_FIXED, this->crcLenGFSK, RADIOLIB_LR2021_MAX_PACKET_LENGTH));
      
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR2021::invertIQ(bool enable) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  this->invertIQEnabled = enable;
  return(setLoRaPacketParams(this->preambleLengthLoRa, this->headerType, this->implicitLen, this->crcTypeLoRa, (uint8_t)this->invertIQEnabled));
}

int16_t LR2021::setBitRate(float br) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
    RADIOLIB_CHECK_RANGE(br, 0.5f, 2000.0f, RADIOLIB_ERR_INVALID_BIT_RATE);
    //! \TODO: [LR2021] implement fractional bit rate configuration
    this->bitRate = br * 1000.0f;
    state = setGfskModulationParams(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev);
    return(state);
  
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_OOK) {
    RADIOLIB_CHECK_RANGE(br, 0.5f, 2000.0f, RADIOLIB_ERR_INVALID_BIT_RATE);
    //! \TODO: [LR2021] implement fractional bit rate configuration
    this->bitRate = br * 1000.0f;
    //! \TODO: [LR2021] implement OOK magnitude depth configuration
    state = setOokModulationParams(this->bitRate, this->pulseShape, this->rxBandwidth, RADIOLIB_LR2021_OOK_DEPTH_FULL);
    return(state);
    
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_FLRC) {
     if((uint16_t)br == 260) {
      this->bitRateFlrc = RADIOLIB_LR2021_FLRC_BR_260;
    } else if((uint16_t)br == 325) {
      this->bitRateFlrc = RADIOLIB_LR2021_FLRC_BR_325;
    } else if((uint16_t)br == 520) {
      this->bitRateFlrc = RADIOLIB_LR2021_FLRC_BR_520;
    } else if((uint16_t)br == 650) {
      this->bitRateFlrc = RADIOLIB_LR2021_FLRC_BR_650;
    } else if((uint16_t)br == 1040) {
      this->bitRateFlrc = RADIOLIB_LR2021_FLRC_BR_1040;
    } else if((uint16_t)br == 1300) {
      this->bitRateFlrc = RADIOLIB_LR2021_FLRC_BR_1300;
    } else if((uint16_t)br == 2080) {
      this->bitRateFlrc = RADIOLIB_LR2021_FLRC_BR_2080;
    } else if((uint16_t)br == 2600) {
      this->bitRateFlrc = RADIOLIB_LR2021_FLRC_BR_2600;
    } else {
      return(RADIOLIB_ERR_INVALID_BIT_RATE);
    }

    // it is slightly weird to reuse the GFSK bitrate variable in this way
    // but if GFSK gets enabled it should get reset anyway ... I think
    this->bitRate = br;

    // update modulation parameters
    return(setFlrcModulationParams(this->bitRateFlrc, this->codingRateFlrc, this->pulseShape));
  }

  return(RADIOLIB_ERR_WRONG_MODEM);  
}

int16_t LR2021::setFrequencyDeviation(float freqDev) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set frequency deviation to lowest available setting (required for digimodes)
  float newFreqDev = freqDev;
  if(freqDev < 0.6f) {
    newFreqDev = 0.6f;
  }

  RADIOLIB_CHECK_RANGE(newFreqDev, 0.6f, 500.0f, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
  this->frequencyDev = newFreqDev * 1000.0f;
  state = setGfskModulationParams(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev);
  return(state);
}

int16_t LR2021::setRxBandwidth(float rxBw) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(!((type == RADIOLIB_LR2021_PACKET_TYPE_GFSK) || 
       (type == RADIOLIB_LR2021_PACKET_TYPE_OOK))) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  const uint8_t rxBwLut[] = {
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_4_8,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_5_8,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_7_4,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_9_7,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_12_0,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_14_9,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_19_2,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_23_1,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_29_8,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_38_5,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_46_3,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_59_5,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_76_9,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_92_6,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_119_0,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_153_8,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_185_2,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_238_1,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_307_7,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_370_4,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_476_2,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_555_6,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_666_7,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_769_2,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_1111,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_2222,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_2666,
    RADIOLIB_LR2021_GFSK_OOK_RX_BW_3076,
  };

  state = findRxBw(rxBw, rxBwLut, sizeof(rxBwLut)/sizeof(rxBwLut[0]), 3076.0f, &this->rxBandwidth);
  RADIOLIB_ASSERT(state);

  // update modulation parameters
  if(type == RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
    state = setGfskModulationParams(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev);
  } else {
    state = setOokModulationParams(this->bitRate, this->pulseShape, this->rxBandwidth, RADIOLIB_LR2021_OOK_DEPTH_FULL);
  }
  return(state);
}

int16_t LR2021::setSyncWord(uint8_t* syncWord, size_t len) {
  if(len > RADIOLIB_LR2021_GFSK_SYNC_WORD_LEN) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }

  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);

  uint32_t sync = 0;
  switch(type) {
    case(RADIOLIB_LR2021_PACKET_TYPE_GFSK):
      // default to MSB-first
      return(setGfskSyncword(const_cast<const uint8_t*>(syncWord), len, true));
    
      case(RADIOLIB_LR2021_PACKET_TYPE_OOK):
      // default to MSB-first
      return(setOokSyncword(const_cast<const uint8_t*>(syncWord), len, true));
    
    case(RADIOLIB_LR2021_PACKET_TYPE_LORA):
      // with length set to 1 and LoRa modem active, assume it is the LoRa sync word
      if(len > 1) {
        return(RADIOLIB_ERR_INVALID_SYNC_WORD);
      }
      return(setSyncWord(syncWord[0]));
    
    case(RADIOLIB_LR2021_PACKET_TYPE_LR_FHSS):
      // with length set to 4 and LR-FHSS modem active, assume it is the LR-FHSS sync word
      if(len != sizeof(uint32_t)) {
        return(RADIOLIB_ERR_INVALID_SYNC_WORD);
      }
      memcpy(&sync, syncWord, sizeof(uint32_t));
      return(lrFhssSetSyncword(sync));
    
    case(RADIOLIB_LR2021_PACKET_TYPE_FLRC):
      // FLRC requires 16 or 32-bit sync word
      if(!((len == 0) || (len == 2) || (len == 4))) {
        return(RADIOLIB_ERR_INVALID_SYNC_WORD);
      }

      // update sync word length
      this->syncWordLength = len;
      state = setFlrcPacketParams(this->preambleLengthGFSK, this->syncWordLength, 1, 0x01, this->packetType == RADIOLIB_LR2021_GFSK_OOK_PACKET_FORMAT_FIXED, this->crcLenGFSK, RADIOLIB_LR2021_MAX_PACKET_LENGTH);
      RADIOLIB_ASSERT(state);

      sync |= (uint32_t)syncWord[0] << 24;
      sync |= (uint32_t)syncWord[1] << 16;
      sync |= (uint32_t)syncWord[2] << 8;
      sync |= (uint32_t)syncWord[3];
      return(setFlrcSyncWord(1, sync));
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR2021::setDataShaping(uint8_t sh) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);

  // set data shaping
  switch(sh) {
    case RADIOLIB_SHAPING_NONE:
      this->pulseShape = RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_NONE;
      break;
    case RADIOLIB_SHAPING_0_3:
      this->pulseShape = RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_GAUSS_BT_0_3;
      break;
    case RADIOLIB_SHAPING_0_5:
      this->pulseShape = RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_GAUSS_BT_0_5;
      break;
    case RADIOLIB_SHAPING_0_7:
      this->pulseShape = RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_GAUSS_BT_0_7;
      break;
    case RADIOLIB_SHAPING_1_0:
      this->pulseShape = RADIOLIB_LR2021_GFSK_BPSK_FLRC_OOK_SHAPING_GAUSS_BT_1_0;
      break;
    default:
      return(RADIOLIB_ERR_INVALID_DATA_SHAPING);
  }

  // update modulation parameters
  if(type == RADIOLIB_LR2021_PACKET_TYPE_FLRC) {
    return(setFlrcModulationParams(this->bitRateFlrc, this->codingRateFlrc, this->pulseShape));
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
    return(setGfskModulationParams(this->bitRate, this->pulseShape, this->rxBandwidth, this->frequencyDev));
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_OOK) {
    return(setOokModulationParams(this->bitRate, this->pulseShape, this->rxBandwidth, RADIOLIB_LR2021_OOK_DEPTH_FULL));
  }
  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR2021::setEncoding(uint8_t encoding) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);

  switch(type) {
    case(RADIOLIB_LR2021_PACKET_TYPE_GFSK):
      return(setWhitening(encoding == RADIOLIB_ENCODING_WHITENING));
    
    case(RADIOLIB_LR2021_PACKET_TYPE_OOK):
      switch(encoding) {
        case(RADIOLIB_ENCODING_NRZ):
        case(RADIOLIB_ENCODING_WHITENING):
          return(setWhitening(encoding == RADIOLIB_ENCODING_WHITENING));
        
        case(RADIOLIB_ENCODING_MANCHESTER):
          state = setWhitening(false);
          RADIOLIB_ASSERT(state);
          this->whitening = RADIOLIB_LR2021_OOK_MANCHESTER_ON;
          return(setOokPacketParams(this->preambleLengthGFSK, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
        
        case(RADIOLIB_ENCODING_MANCHESTER_INV):
          state = setWhitening(false);
          RADIOLIB_ASSERT(state);
          this->whitening = RADIOLIB_LR2021_OOK_MANCHESTER_ON_INV;
          return(setOokPacketParams(this->preambleLengthGFSK, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
        
        default:
          return(RADIOLIB_ERR_INVALID_ENCODING);
      }
    
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }

  return(state);
}

int16_t LR2021::fixedPacketLengthMode(uint8_t len) {
  return(setPacketMode(RADIOLIB_LR2021_GFSK_OOK_PACKET_FORMAT_FIXED, len));
}

int16_t LR2021::variablePacketLengthMode(uint8_t maxLen) {
  return(setPacketMode(RADIOLIB_LR2021_GFSK_OOK_PACKET_FORMAT_VARIABLE_8BIT, maxLen));
}

int16_t LR2021::setWhitening(bool enabled, uint16_t initial) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  
  switch(type) {
    case(RADIOLIB_LR2021_PACKET_TYPE_GFSK):
      //! \TODO: [LR2021] Implement SX128x-compatible whitening
      if(enabled) {
        state = setGfskWhiteningParams(RADIOLIB_LR2021_GFSK_WHITENING_TYPE_SX126X_LR11XX, initial);
        RADIOLIB_ASSERT(state);
      }
      this->whitening = enabled;
      return(setGfskPacketParams(this->preambleLengthGFSK, this->preambleDetLength, false, false, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
    
    case(RADIOLIB_LR2021_PACKET_TYPE_OOK):
      this->whitening = enabled;
      if(enabled) {
        //! \TODO: [LR2021] Implement configurable index and polynomial
        state = setOokWhiteningParams(12, 0x01FF, initial);
      } else {
        state = setOokWhiteningParams(0, 0, 0);
      }
      RADIOLIB_ASSERT(state);
      return(setOokPacketParams(this->preambleLengthGFSK, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR2021::setDataRate(DataRate_t dr, ModemType_t modem ) {
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

int16_t LR2021::checkDataRate(DataRate_t dr, ModemType_t modem) {
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // retrieve modem if not supplied
  if(modem == RADIOLIB_MODEM_NONE) {
    state = this->getModem(&modem);
    RADIOLIB_ASSERT(state);
  }

  // select interpretation based on modem
  if(modem == RADIOLIB_MODEM_FSK) {
    RADIOLIB_CHECK_RANGE(dr.fsk.bitRate, 0.5f, 2000.0f, RADIOLIB_ERR_INVALID_BIT_RATE);
    RADIOLIB_CHECK_RANGE(dr.fsk.freqDev, 0.6f,  500.0f, RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION);
    return(RADIOLIB_ERR_NONE);

  } else if(modem == RADIOLIB_MODEM_LORA) {
    RADIOLIB_CHECK_RANGE(dr.lora.spreadingFactor, 5, 12, RADIOLIB_ERR_INVALID_SPREADING_FACTOR);
    RADIOLIB_CHECK_RANGE(dr.lora.bandwidth, 0.0f, 510.0f, RADIOLIB_ERR_INVALID_BANDWIDTH);
    RADIOLIB_CHECK_RANGE(dr.lora.codingRate, 4, 8, RADIOLIB_ERR_INVALID_CODING_RATE);
    return(RADIOLIB_ERR_NONE);
  
  }

  return(state);
}

int16_t LR2021::setLrFhssConfig(uint8_t bw, uint8_t cr, uint8_t hdrCount, uint16_t hopSeed) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LR_FHSS) {
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

int16_t LR2021::setRxBoostedGainMode(uint8_t level) {
  int16_t state = this->setRxPath(this->highFreq ? RADIOLIB_LR2021_RX_PATH_HF : RADIOLIB_LR2021_RX_PATH_LF, this->highFreq ? this->gainModeHf : this->gainModeLf);
  RADIOLIB_ASSERT(state);
  if(this->highFreq) {
    this->gainModeHf = level;
  } else {
    this->gainModeLf = level;
  }
  return(state);
}

int16_t LR2021::setPacketMode(uint8_t mode, uint8_t len) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type == RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
    // set requested packet mode
    state = setGfskPacketParams(this->preambleLengthGFSK, this->preambleDetLength, false, false, this->addrComp, mode, len, this->crcTypeGFSK, this->whitening);
    RADIOLIB_ASSERT(state);

    // update cached value
    this->packetType = mode;
    return(state);
  
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_OOK) {
    // set requested packet mode
    state = setOokPacketParams(this->preambleLengthGFSK, this->addrComp, mode, len, this->crcTypeGFSK, this->whitening);
    RADIOLIB_ASSERT(state);

    // update cached value
    this->packetType = mode;
    return(state);
  
  } else if(type == RADIOLIB_LR2021_PACKET_TYPE_FLRC) {
    state = setFlrcPacketParams(this->preambleLengthGFSK, this->syncWordLength, 1, 0x01, mode == RADIOLIB_LR2021_GFSK_OOK_PACKET_FORMAT_FIXED, this->crcLenGFSK, len);
    RADIOLIB_ASSERT(state);

    this->packetType = mode;
    return(state);
  
  }

  return(RADIOLIB_ERR_WRONG_MODEM);
}

int16_t LR2021::setLoRaHeaderType(uint8_t hdrType, size_t len) {
  uint8_t modem = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&modem);
  RADIOLIB_ASSERT(state);
  if(modem != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // set requested packet mode
  state = setLoRaPacketParams(this->preambleLengthLoRa, hdrType, len, this->crcTypeLoRa, this->invertIQEnabled);
  RADIOLIB_ASSERT(state);

  // update cached value
  this->headerType = hdrType;
  this->implicitLen = len;

  return(state);
}

int16_t LR2021::implicitHeader(size_t len) {
  return(this->setLoRaHeaderType(RADIOLIB_LR2021_LORA_HEADER_IMPLICIT, len));
}

int16_t LR2021::explicitHeader() {
  return(this->setLoRaHeaderType(RADIOLIB_LR2021_LORA_HEADER_EXPLICIT));
}

int16_t LR2021::setNodeAddress(uint8_t nodeAddr) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node only)
  this->addrComp = RADIOLIB_LR2021_GFSK_OOK_ADDR_FILT_NODE;
  state = setGfskPacketParams(this->preambleLengthGFSK, this->preambleDetLength, false, false, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
  RADIOLIB_ASSERT(state);

  // set node address
  this->node = nodeAddr;
  return(setGfskAddress(this->node, 0));
}

int16_t LR2021::setBroadcastAddress(uint8_t broadAddr) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // enable address filtering (node and broadcast)
  this->addrComp = RADIOLIB_LR2021_GFSK_OOK_ADDR_FILT_NODE_BROADCAST;
  state = setGfskPacketParams(this->preambleLengthGFSK, this->preambleDetLength, false, false, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening);
  RADIOLIB_ASSERT(state);

  // set node and broadcast address
  return(setGfskAddress(this->node, broadAddr));
}

int16_t LR2021::disableAddressFiltering() {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_GFSK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  // disable address filtering
  this->addrComp = RADIOLIB_LR2021_GFSK_OOK_ADDR_FILT_DISABLED;
  return(setGfskPacketParams(this->preambleLengthGFSK, this->preambleDetLength, false, false, this->addrComp, this->packetType, RADIOLIB_LR2021_MAX_PACKET_LENGTH, this->crcTypeGFSK, this->whitening));
}

int16_t LR2021::ookDetector(uint16_t pattern, uint8_t len, uint8_t repeats, bool syncRaw, bool rising, uint8_t sofLen) {
  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_OOK) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  return(setOokDetector(pattern, len - 1, repeats, syncRaw, rising, sofLen));
}

int16_t LR2021::setOokDetectionThreshold(int16_t level) {
  int16_t levelRaw = 64 + level;
  return(this->writeRegMemMask32(RADIOLIB_LR2021_REG_OOK_DETECTION_THRESHOLD, (0x7FUL << 20), (uint32_t)levelRaw << 20));
}

int16_t LR2021::setSideDetector(const LR2021LoRaSideDetector_t* cfg, size_t numDetectors) {
  // some basic sanity checks
  if((cfg == nullptr) || (numDetectors == 0)) {
    return(RADIOLIB_ERR_NONE);
  }

  if(numDetectors > 3) {
    return(RADIOLIB_ERR_INVALID_SIDE_DETECT);
  }

  // if bandwidth is higher than 500 kHz, at most 2 side detectors are allowed
  if((this->bandwidthKhz > 500.0f) && (numDetectors > 2)) {
    return(RADIOLIB_ERR_INVALID_SIDE_DETECT);
  }

  // if the primary spreading factor is 10, 11 or 12, at most 2 side detectors are allowed
  if((this->spreadingFactor >= 10) && (numDetectors > 2)) {
    return(RADIOLIB_ERR_INVALID_SIDE_DETECT);
  }

  // condition of the primary spreading factor being the smallest/largest is not checked
  // this is intentional, because it depends on whether the user wants to start Rx or CAD

  uint8_t detectors[3] = { 0 };
  uint8_t syncWords[3] = { 0 };
  uint8_t minSf = this->spreadingFactor;
  for(size_t i = 0; i < numDetectors; i++) {
    // all side-detector spreading factors must be higher than the primary one
    //! \todo [LR2021] implement multi-SF for CAD (main SF must be smallest!)
    if(this->spreadingFactor >= cfg[i].sf) {
      return(RADIOLIB_ERR_INVALID_SIDE_DETECT);
    }

    // the difference between maximum and minimum spreading factor used must be less than or equal to 4
    if(cfg[i].sf - minSf > 4) {
      return(RADIOLIB_ERR_INVALID_SIDE_DETECT);
    }

    if(cfg[i].sf < minSf) { minSf = cfg[i].sf; }

    detectors[i] = cfg[i].sf << 4 | cfg[i].ldro << 2 | cfg[i].invertIQ;
    syncWords[i] = cfg[i].syncWord;
  }

  // all spreading factors must be different
  if(numDetectors >= 2) {
    if(cfg[0].sf == cfg[1].sf) { 
      return(RADIOLIB_ERR_INVALID_SIDE_DETECT);
    }
  }

  if(numDetectors == 3) {
    if((cfg[1].sf == cfg[2].sf) || (cfg[0].sf == cfg[2].sf)) { 
      return(RADIOLIB_ERR_INVALID_SIDE_DETECT);
    }
  }

  // check active modem
  uint8_t type = RADIOLIB_LR2021_PACKET_TYPE_NONE;
  int16_t state = getPacketType(&type);
  RADIOLIB_ASSERT(state);
  if(type != RADIOLIB_LR2021_PACKET_TYPE_LORA) {
    return(RADIOLIB_ERR_WRONG_MODEM);
  }

  state = setLoRaSideDetConfig(detectors, numDetectors);
  RADIOLIB_ASSERT(state);

  return(setLoRaSideDetSyncword(syncWords, numDetectors));
}

int16_t LR2021::setGain(uint8_t gain) {
  if(gain > 13) {
    return(RADIOLIB_ERR_INVALID_GAIN);
  }
  return(this->SPIcommand(RADIOLIB_LR2021_CMD_SET_AGC_GAIN_MANUAL, true, &gain, sizeof(gain)));
}

#endif
