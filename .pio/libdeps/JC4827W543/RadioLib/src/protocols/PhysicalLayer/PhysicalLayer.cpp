#include "PhysicalLayer.h"

#include <string.h>

PhysicalLayer::PhysicalLayer() {
  this->freqStep = 1;
  this->maxPacketLength = 1;
  #if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
  this->bufferBitPos = 0;
  this->bufferWritePos = 0;
  #endif
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t PhysicalLayer::transmit(__FlashStringHelper* fstr, uint8_t addr) {
  // read flash string length
  size_t len = 0;
  PGM_P p = reinterpret_cast<PGM_P>(fstr);
  while(true) {
    char c = RADIOLIB_NONVOLATILE_READ_BYTE(p++);
    len++;
    if(c == '\0') {
      break;
    }
  }

  // dynamically allocate memory
  #if RADIOLIB_STATIC_ONLY
    char str[RADIOLIB_STATIC_ARRAY_SIZE];
  #else
    char* str = new char[len];
  #endif

  // copy string from flash
  p = reinterpret_cast<PGM_P>(fstr);
  for(size_t i = 0; i < len; i++) {
    str[i] = RADIOLIB_NONVOLATILE_READ_BYTE(p + i);
  }

  // transmit string
  int16_t state = transmit(str, addr);
  #if !RADIOLIB_STATIC_ONLY
    delete[] str;
  #endif
  return(state);
}

int16_t PhysicalLayer::transmit(String& str, uint8_t addr) {
  return(transmit(str.c_str(), addr));
}
#endif

int16_t PhysicalLayer::transmit(const char* str, uint8_t addr) {
  return(transmit(reinterpret_cast<uint8_t*>(const_cast<char*>(str)), strlen(str), addr));
}

int16_t PhysicalLayer::transmit(const uint8_t* data, size_t len, uint8_t addr) {
  (void)data;
  (void)len;
  (void)addr;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t PhysicalLayer::receive(String& str, size_t len, RadioLibTime_t timeout) {
  int16_t state = RADIOLIB_ERR_NONE;

  // user can override the length of data to read
  size_t length = len;

  // build a temporary buffer
  #if RADIOLIB_STATIC_ONLY
    uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
  #else
    uint8_t* data = NULL;
    if(length == 0) {
      data = new uint8_t[this->maxPacketLength + 1];
    } else {
      data = new uint8_t[length + 1];
    }
    RADIOLIB_ASSERT_PTR(data);
  #endif

  // attempt packet reception
  state = receive(data, length, timeout);

  // any of the following leads to at least some data being available
  // let's leave the decision of whether to keep it or not up to the user
  if((state == RADIOLIB_ERR_NONE) || (state == RADIOLIB_ERR_CRC_MISMATCH) || (state == RADIOLIB_ERR_LORA_HEADER_DAMAGED)) {
    // read the number of actually received bytes (for unknown packets)
    if(len == 0) {
      length = getPacketLength(false);
    }

    // add null terminator
    data[length] = 0;

    // initialize Arduino String class
    str = String(reinterpret_cast<char*>(data));
  }

  // deallocate temporary buffer
  #if !RADIOLIB_STATIC_ONLY
    delete[] data;
  #endif

  return(state);
}
#endif

int16_t PhysicalLayer::receive(uint8_t* data, size_t len, RadioLibTime_t timeout) {
  (void)data;
  (void)len;
  (void)timeout;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::sleep() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::standby() {
  return(standby(RADIOLIB_STANDBY_DEFAULT));
}

int16_t PhysicalLayer::standby(uint8_t mode) {
  (void)mode;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::startReceive() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::startReceive(uint32_t timeout, RadioLibIrqFlags_t irqFlags, RadioLibIrqFlags_t irqMask, size_t len) {
  RadioModeConfig_t cfg = {
    .receive = {
      .timeout = timeout,
      .irqFlags = irqFlags,
      .irqMask = irqMask,
      .len = len,
    }
  };

  int16_t state = this->stageMode(RADIOLIB_RADIO_MODE_RX, &cfg);
  RADIOLIB_ASSERT(state);
  return(this->launchMode());
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t PhysicalLayer::startTransmit(String& str, uint8_t addr) {
  return(startTransmit(str.c_str(), addr));
}
#endif

int16_t PhysicalLayer::startTransmit(const char* str, uint8_t addr) {
  return(startTransmit(reinterpret_cast<uint8_t*>(const_cast<char*>(str)), strlen(str), addr));
}

int16_t PhysicalLayer::startTransmit(const uint8_t* data, size_t len, uint8_t addr) {
  RadioModeConfig_t cfg = {
    .transmit = {
      .data = data,
      .len = len,
      .addr = addr,
    }
  };

  int16_t state = this->stageMode(RADIOLIB_RADIO_MODE_TX, &cfg);
  RADIOLIB_ASSERT(state);
  return(this->launchMode());
}

int16_t PhysicalLayer::finishTransmit() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::finishReceive() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

#if defined(RADIOLIB_BUILD_ARDUINO)
int16_t PhysicalLayer::readData(String& str, size_t len) {
  int16_t state = RADIOLIB_ERR_NONE;

  // read the number of actually received bytes
  size_t length = getPacketLength();

  if((len < length) && (len != 0)) {
    // user requested less bytes than were received, this is allowed (but frowned upon)
    // requests for more data than were received will only return the number of actually received bytes (unlike PhysicalLayer::receive())
    length = len;
  }

  // build a temporary buffer
  #if RADIOLIB_STATIC_ONLY
    uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
  #else
    uint8_t* data = new uint8_t[length + 1];
    RADIOLIB_ASSERT_PTR(data);
  #endif

  // read the received data
  state = readData(data, length);

  // any of the following leads to at least some data being available
  // let's leave the decision of whether to keep it or not up to the user
  if((state == RADIOLIB_ERR_NONE) || (state == RADIOLIB_ERR_CRC_MISMATCH) || (state == RADIOLIB_ERR_LORA_HEADER_DAMAGED)) {
    // add null terminator
    data[length] = 0;

    // initialize Arduino String class
    str = String(reinterpret_cast<char*>(data));
  }

  // deallocate temporary buffer
  #if !RADIOLIB_STATIC_ONLY
    delete[] data;
  #endif

  return(state);
}
#endif

int16_t PhysicalLayer::readData(uint8_t* data, size_t len) {
  (void)data;
  (void)len;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::transmitDirect(uint32_t frf) {
  (void)frf;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::receiveDirect() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setFrequency(float freq) {
  (void)freq;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setBitRate(float br) {
  (void)br;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setFrequencyDeviation(float freqDev) {
  (void)freqDev;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setDataShaping(uint8_t sh) {
  (void)sh;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setEncoding(uint8_t encoding) {
  (void)encoding;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::invertIQ(bool enable) {
  (void)enable;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setOutputPower(int8_t power) {
  (void)power;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::checkOutputPower(int8_t power, int8_t* clipped) {
  (void)power;
  (void)clipped;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setSyncWord(uint8_t* sync, size_t len) {
  (void)sync;
  (void)len;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setPreambleLength(size_t len) {
  (void)len;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setDataRate(DataRate_t dr, ModemType_t modem) {
  (void)dr;
  (void)modem;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::checkDataRate(DataRate_t dr, ModemType_t modem) {
  (void)dr;
  (void)modem;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

size_t PhysicalLayer::getPacketLength(bool update) {
  (void)update;
  return(0);
}

float PhysicalLayer::getRSSI() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

float PhysicalLayer::getSNR() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

RadioLibTime_t PhysicalLayer::calculateTimeOnAir(ModemType_t modem, DataRate_t dr, PacketConfig_t pc, size_t len) {
  (void)modem;
  (void)dr;
  (void)pc;
  (void)len;
  return(0);
}

RadioLibTime_t PhysicalLayer::getTimeOnAir(size_t len) {
  (void)len;
  return(0);
}

RadioLibTime_t PhysicalLayer::calculateRxTimeout(RadioLibTime_t timeoutUs) {
  (void)timeoutUs;
  return(0); 
}

uint32_t PhysicalLayer::getIrqMapped(RadioLibIrqFlags_t irq) {
  // iterate over all set bits and build the module-specific flags
  uint32_t irqRaw = 0;
  for(uint8_t i = 0; i < 8*(sizeof(RadioLibIrqFlags_t)); i++) {
    if((irq & (uint32_t)(1UL << i)) && (this->irqMap[i] != RADIOLIB_IRQ_NOT_SUPPORTED)) {
      irqRaw |= this->irqMap[i];
    }
  }

  return(irqRaw);
}

int16_t PhysicalLayer::checkIrq(RadioLibIrqType_t irq) {
  if((irq > RADIOLIB_IRQ_TIMEOUT) || (this->irqMap[irq] == RADIOLIB_IRQ_NOT_SUPPORTED)) {
    return(RADIOLIB_ERR_UNSUPPORTED);
  }
  
  return((getIrqFlags() & this->irqMap[irq]) != 0);
}

int16_t PhysicalLayer::setIrq(RadioLibIrqFlags_t irq) {
  return(setIrqFlags(getIrqMapped(irq)));
}

int16_t PhysicalLayer::clearIrq(RadioLibIrqFlags_t irq) {
  return(clearIrqFlags(getIrqMapped(irq)));
}

uint32_t PhysicalLayer::getIrqFlags() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::setIrqFlags(uint32_t irq) {
  (void)irq;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::clearIrqFlags(uint32_t irq) {
  (void)irq;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::startChannelScan() {
  return(RADIOLIB_ERR_UNSUPPORTED); 
}

int16_t PhysicalLayer::startChannelScan(const ChannelScanConfig_t &config) {
  (void)config;
  return(RADIOLIB_ERR_UNSUPPORTED); 
}

int16_t PhysicalLayer::getChannelScanResult() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::scanChannel() {
  return(RADIOLIB_ERR_UNSUPPORTED); 
}

int16_t PhysicalLayer::scanChannel(const ChannelScanConfig_t &config) {
  (void)config;
  return(RADIOLIB_ERR_UNSUPPORTED); 
}

int32_t PhysicalLayer::random(int32_t max) {
  if(max == 0) {
    return(0);
  }

  // get random bytes from the radio
  uint8_t randBuff[4];
  for(uint8_t i = 0; i < 4; i++) {
    randBuff[i] = randomByte();
  }

  // create 32-bit TRNG number
  int32_t randNum = ((int32_t)randBuff[0] << 24) | ((int32_t)randBuff[1] << 16) | ((int32_t)randBuff[2] << 8) | ((int32_t)randBuff[3]);
  if(randNum < 0) {
    randNum *= -1;
  }
  return(randNum % max);
}

int32_t PhysicalLayer::random(int32_t min, int32_t max) {
  if(min >= max) {
    return(min);
  }

  return(PhysicalLayer::random(max - min) + min);
}

uint8_t PhysicalLayer::randomByte() {
  return(0);
}

int16_t PhysicalLayer::startDirect() {
  // disable encodings
  int16_t state = setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  // disable shaping
  state = setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);

  // set frequency deviation to the lowest possible value
  state = setFrequencyDeviation(-1);
  return(state);
}

#if !RADIOLIB_EXCLUDE_DIRECT_RECEIVE
int16_t PhysicalLayer::available() {
  return(this->bufferWritePos);
}

void PhysicalLayer::dropSync() {
  if(this->directSyncWordLen > 0) {
    this->gotSync = false;
    this->syncBuffer = 0;
  }
}

uint8_t PhysicalLayer::read(bool drop) {
  if(drop) {
    dropSync();
  }
  this->bufferWritePos--;
  return(this->buffer[this->bufferReadPos++]);
}

int16_t PhysicalLayer::setDirectSyncWord(uint32_t syncWord, uint8_t len) {
  if(len > 32) {
    return(RADIOLIB_ERR_INVALID_SYNC_WORD);
  }
  this->directSyncWordMask = 0xFFFFFFFF >> (32 - len);
  this->directSyncWordLen = len;
  this->directSyncWord = syncWord;

  // override sync word matching when length is set to 0
  if(this->directSyncWordLen == 0) {
    this->gotSync = true;
  }

  return(RADIOLIB_ERR_NONE);
}

void PhysicalLayer::updateDirectBuffer(uint8_t bit) {
  // check sync word
  if(!this->gotSync) {
    this->syncBuffer <<= 1;
    this->syncBuffer |= bit;

    RADIOLIB_DEBUG_PROTOCOL_PRINTLN("S\t%lu", (long unsigned int)this->syncBuffer);

    if((this->syncBuffer & this->directSyncWordMask) == this->directSyncWord) {
      this->gotSync = true;
      this->bufferWritePos = 0;
      this->bufferReadPos = 0;
      this->bufferBitPos = 0;
    }

  } else {
    // save the bit
    if(bit) {
      this->buffer[this->bufferWritePos] |= 0x01 << this->bufferBitPos;
    } else {
      this->buffer[this->bufferWritePos] &= ~(0x01 << this->bufferBitPos);
    }
    this->bufferBitPos++;

    // check complete byte
    if(this->bufferBitPos == 8) {
      this->buffer[this->bufferWritePos] = rlb_reflect(this->buffer[this->bufferWritePos], 8);
      RADIOLIB_DEBUG_PROTOCOL_PRINTLN("R\t%X", this->buffer[this->bufferWritePos]);

      this->bufferWritePos++;
      this->bufferBitPos = 0;
    }
  }
}

void PhysicalLayer::setDirectAction(void (*func)(void)) {
  (void)func;
}

void PhysicalLayer::readBit(uint32_t pin) {
  (void)pin;
}

#endif

void PhysicalLayer::setPacketReceivedAction(void (*func)(void)) {
  (void)func;
}

void PhysicalLayer::clearPacketReceivedAction() {
  
}

void PhysicalLayer::setPacketSentAction(void (*func)(void)) {
  (void)func;
}

void PhysicalLayer::clearPacketSentAction() {
  
}

void PhysicalLayer::setChannelScanAction(void (*func)(void)) {
  (void)func;
}

void PhysicalLayer::clearChannelScanAction() {
  
}

int16_t PhysicalLayer::setModem(ModemType_t modem) {
  (void)modem;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::getModem(ModemType_t* modem) {
  (void)modem;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg) {
  (void)mode;
  (void)cfg;
  return(RADIOLIB_ERR_UNSUPPORTED);
}

int16_t PhysicalLayer::launchMode() {
  return(RADIOLIB_ERR_UNSUPPORTED);
}

#if RADIOLIB_INTERRUPT_TIMING
void PhysicalLayer::setInterruptSetup(void (*func)(uint32_t)) {
  Module* mod = getMod();
  mod->TimerSetupCb = func;
}

void PhysicalLayer::setTimerFlag() {
  Module* mod = getMod();
  mod->TimerFlag = true;
}
#endif
