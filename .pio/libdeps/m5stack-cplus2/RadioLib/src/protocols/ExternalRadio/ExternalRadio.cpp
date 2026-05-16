#include "ExternalRadio.h"

#if defined(RADIOLIB_BUILD_ARDUINO)
ExternalRadio::ExternalRadio(uint32_t pin) : PhysicalLayer() {
  this->freqStep = 1;
  mod = new Module(RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC, pin);
  mod->hal->pinMode(pin, mod->hal->GpioModeOutput);
  this->prevFrf = 0;
}
#endif

ExternalRadio::ExternalRadio(RadioLibHal *hal, uint32_t pin) : PhysicalLayer() {
  this->freqStep = 1;
  mod = new Module(hal, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC, pin);
  mod->hal->pinMode(pin, mod->hal->GpioModeOutput);
  this->prevFrf = 0;
}

ExternalRadio::ExternalRadio(const ExternalRadio& ext) : PhysicalLayer() {
  this->freqStep = 1;
  this->prevFrf = ext.prevFrf;
  if(ext.mod) {
    this->mod = new Module(ext.mod->hal, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC, ext.mod->getGpio());
  }
}

ExternalRadio& ExternalRadio::operator=(const ExternalRadio& ext) {
  if(&ext != this) {
    this->prevFrf = ext.prevFrf;
    if(ext.mod) {
      this->mod = new Module(ext.mod->hal, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC, ext.mod->getGpio());
    }
  }
  return(*this);
}

ExternalRadio::~ExternalRadio() {
  delete this->mod;
}

Module* ExternalRadio::getMod() {
  return(mod);
}

int16_t ExternalRadio::setFrequencyDeviation(float freqDev) {
  (void)freqDev;
  return(RADIOLIB_ERR_NONE);
}

int16_t ExternalRadio::setDataShaping(uint8_t sh) {
  (void)sh;
  return(RADIOLIB_ERR_NONE);
}

int16_t ExternalRadio::setEncoding(uint8_t encoding) {
  (void)encoding;
  return(RADIOLIB_ERR_NONE);
}

int16_t ExternalRadio::transmitDirect(uint32_t frf) {
  if(frf != this->prevFrf) {
    uint32_t val = this->mod->hal->GpioLevelLow;
    if(frf > this->prevFrf) {
      val = this->mod->hal->GpioLevelHigh;
    }
    this->prevFrf = frf;
    this->mod->hal->digitalWrite(this->mod->getGpio(), val);
  }
  
  return(RADIOLIB_ERR_NONE);
}