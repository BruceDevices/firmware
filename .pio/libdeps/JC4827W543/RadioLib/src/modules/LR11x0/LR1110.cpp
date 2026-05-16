#include "LR1110.h"
#include <math.h>

#if !RADIOLIB_EXCLUDE_LR11X0

LR1110::LR1110(Module* mod) : LR11x0(mod) {
  chipType = RADIOLIB_LR11X0_DEVICE_LR1110;
}

int16_t LR1110::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::begin(bw, sf, cr, syncWord, preambleLength, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  return(state);
}

int16_t LR1110::beginGFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::beginGFSK(br, freqDev, rxBw, preambleLength, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  return(state);
}

int16_t LR1110::beginLRFHSS(float freq, uint8_t bw, uint8_t cr, bool narrowGrid, int8_t power, float tcxoVoltage) {
  // execute common part
  int16_t state = LR11x0::beginLRFHSS(bw, cr, narrowGrid, tcxoVoltage);
  RADIOLIB_ASSERT(state);

  // configure publicly accessible settings
  state = setFrequency(freq);
  RADIOLIB_ASSERT(state);

  state = setOutputPower(power);
  return(state);
}

int16_t LR1110::setFrequency(float freq) {
  return(this->setFrequency(freq, false));
}

int16_t LR1110::setFrequency(float freq, bool skipCalibration, float band) {
  RADIOLIB_CHECK_RANGE(freq, 150.0f, 960.0f, RADIOLIB_ERR_INVALID_FREQUENCY);
  
  // check if we need to recalibrate image
  int16_t state;
  if(!skipCalibration && (fabsf(freq - this->freqMHz) >= RADIOLIB_LR11X0_CAL_IMG_FREQ_TRIG_MHZ)) {
    state = LR11x0::calibrateImageRejection(freq - band, freq + band);
    RADIOLIB_ASSERT(state);
  }

  // set frequency
  state = LR11x0::setRfFrequency((uint32_t)(freq*1000000.0f));
  RADIOLIB_ASSERT(state);
  this->freqMHz = freq;
  return(state);
}

int16_t LR1110::setOutputPower(int8_t power) {
  return(this->setOutputPower(power, false));
}

int16_t LR1110::setOutputPower(int8_t power, bool forceHighPower, uint32_t rampTimeUs) {
  // check if power value is configurable
  int16_t state = this->checkOutputPower(power, NULL, forceHighPower);
  RADIOLIB_ASSERT(state);

  // determine whether to use HP or LP PA and check range accordingly
  bool useHp = forceHighPower || (power > 14);
  
  // TODO how and when to configure OCP?

  // update PA config and set output power - always use VBAT for high-power PA
  // the value returned by LRxxxx class is offset by 3 for LR11x0
  state = LR11x0::setOutputPower(power, (uint8_t)useHp, (uint8_t)useHp, 0x04, 0x07, roundRampTime(rampTimeUs) - 0x03);
  return(state);
}

int16_t LR1110::checkOutputPower(int8_t power, int8_t* clipped) {
  return(checkOutputPower(power, clipped, false));
}

int16_t LR1110::checkOutputPower(int8_t power, int8_t* clipped, bool forceHighPower) {
  if(forceHighPower || (power > 14)) {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-9, RADIOLIB_MIN(22, power));
    }
    RADIOLIB_CHECK_RANGE(power, -9, 22, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  
  } else {
    if(clipped) {
      *clipped = RADIOLIB_MAX(-17, RADIOLIB_MIN(14, power));
    }
    RADIOLIB_CHECK_RANGE(power, -17, 14, RADIOLIB_ERR_INVALID_OUTPUT_POWER);
  
  }
  return(RADIOLIB_ERR_NONE);
}

int16_t LR1110::setModem(ModemType_t modem) {
  switch(modem) {
    case(ModemType_t::RADIOLIB_MODEM_LORA): {
      return(this->begin());
    } break;
    case(ModemType_t::RADIOLIB_MODEM_FSK): {
      return(this->beginGFSK());
    } break;
    case(ModemType_t::RADIOLIB_MODEM_LRFHSS): {
      return(this->beginLRFHSS());
    } break;
    default:
      return(RADIOLIB_ERR_WRONG_MODEM);
  }
  return(RADIOLIB_ERR_WRONG_MODEM);
}

#endif