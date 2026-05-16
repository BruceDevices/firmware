#ifndef PI_HAL_LGPIO_H
#define PI_HAL_LGPIO_H

// include RadioLib
#include <RadioLib.h>

// include the library for Raspberry GPIO pins
#include <lgpio.h>

#define PI_RISING         (LG_RISING_EDGE)
#define PI_FALLING        (LG_FALLING_EDGE)
#define PI_INPUT          (0)
#define PI_OUTPUT         (1)
#define PI_MAX_USER_GPIO  (31)

// forward declaration of alert handler that will be used to emulate interrupts
static void lgpioAlertHandler(int num_alerts, lgGpioAlert_p alerts, void *userdata);

// create a new Raspberry Pi hardware abstraction layer
// using the lgpio library
// the HAL must inherit from the base RadioLibHal class
// and implement all of its virtual methods
class PiHal : public RadioLibHal {
  public:
    // default constructor - initializes the base HAL and any needed private members
    PiHal(uint8_t spiChannel, uint32_t spiSpeed = 2000000, uint8_t spiDevice = 0, uint8_t gpioDevice = 0)
      : RadioLibHal(PI_INPUT, PI_OUTPUT, LG_LOW, LG_HIGH, PI_RISING, PI_FALLING),
      _gpioDevice(gpioDevice),
      _spiDevice(spiDevice),
      _spiSpeed(spiSpeed),
      _spiChannel(spiChannel) {
    }

    void init() override {
      if(_gpioHandle != -1) {
        return;
      }

      // first initialise lgpio library
      if((_gpioHandle = lgGpiochipOpen(_gpioDevice)) < 0) {
        fprintf(stderr, "Could not open GPIO chip: %s\n", lguErrorText(_gpioHandle));
        return;
      }

      // now the SPI
      spiBegin();
    }

    void term() override {
      // stop the SPI
      spiEnd();

      // finally, stop the lgpio library
      lgGpiochipClose(_gpioHandle);
      _gpioHandle = -1;
    }

    // GPIO-related methods (pinMode, digitalWrite etc.) should check
    // RADIOLIB_NC as an alias for non-connected pins
    void pinMode(uint32_t pin, uint32_t mode) override {
      if(pin == RADIOLIB_NC) {
        return;
      }

      int result;
      switch(mode) {
        case PI_INPUT:
          result = lgGpioClaimInput(_gpioHandle, pinFlags[pin], pin);
          break;
        case PI_OUTPUT:
          result = lgGpioClaimOutput(_gpioHandle, pinFlags[pin], pin, LG_HIGH);
          break;
        default:
          fprintf(stderr, "Unknown pinMode mode %" PRIu32 "\n", mode);
          return;
      }

      if(result < 0) {
        fprintf(stderr, "Could not claim pin %" PRIu32 " for mode %" PRIu32 ": %s\n",
            pin, mode, lguErrorText(result));
      }
    }

    void digitalWrite(uint32_t pin, uint32_t value) override {
      if(pin == RADIOLIB_NC) {
        return;
      }

      int result = lgGpioWrite(_gpioHandle, pin, value);
      if(result < 0) {
        fprintf(stderr, "Error writing value to pin %" PRIu32 ": %s\n", pin, lguErrorText(result));
      }
    }

    uint32_t digitalRead(uint32_t pin) override {
      if(pin == RADIOLIB_NC) {
        return(0);
      }

      int result = lgGpioRead(_gpioHandle, pin);
      if(result < 0) {
        fprintf(stderr, "Error writing reading from pin %" PRIu32 ": %s\n", pin, lguErrorText(result));
      }
      return result;
    }

    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
      if((interruptNum == RADIOLIB_NC) || (interruptNum > PI_MAX_USER_GPIO)) {
        return;
      }

      // set lgpio alert callback
      int result = lgGpioClaimAlert(_gpioHandle, 0, mode, interruptNum, -1);
      if(result < 0) {
        fprintf(stderr, "Could not claim pin %" PRIu32 " for alert: %s\n", interruptNum, lguErrorText(result));
        return;
      }

      // enable emulated interrupt
      interruptEnabled[interruptNum] = true;
      interruptCallbacks[interruptNum] = interruptCb;

      // lpgio reports the value of level after an interrupt, not the actual direction
      interruptModes[interruptNum] = (mode == this->GpioInterruptFalling) ? LG_LOW : LG_HIGH;

      lgGpioSetAlertsFunc(_gpioHandle, interruptNum, lgpioAlertHandler, (void *)this);
    }

    void detachInterrupt(uint32_t interruptNum) override {
      if((interruptNum == RADIOLIB_NC) || (interruptNum > PI_MAX_USER_GPIO)) {
        return;
      }

      // clear emulated interrupt
      interruptEnabled[interruptNum] = false;
      interruptModes[interruptNum] = 0;
      interruptCallbacks[interruptNum] = NULL;

      // disable lgpio alert callback
      lgGpioFree(_gpioHandle, interruptNum);
      lgGpioSetAlertsFunc(_gpioHandle, interruptNum, NULL, NULL);
    }

    void delay(unsigned long ms) override {
      if(ms == 0) {
        sched_yield();
        return;
      }

      lguSleep(ms / 1000.0);
    }

    void delayMicroseconds(unsigned long us) override {
      if(us == 0) {
        sched_yield();
        return;
      }

      lguSleep(us / 1000000.0);
    }

    void yield() override {
      sched_yield();
    }

    unsigned long millis() override {
      uint32_t time = lguTimestamp() / 1000000UL;
      return time;
    }

    unsigned long micros() override {
      uint32_t time = lguTimestamp() / 1000UL;
      return time;
    }

    long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
      if(pin == RADIOLIB_NC) {
        return(0);
      }

      this->pinMode(pin, PI_INPUT);
      uint32_t start = this->micros();
      uint32_t curtick = this->micros();

      while(this->digitalRead(pin) == state) {
        if((this->micros() - curtick) > timeout) {
          return(0);
        }
      }

      return(this->micros() - start);
    }

    void spiBegin() {
      if(_spiHandle < 0) {
        if((_spiHandle = lgSpiOpen(_spiDevice, _spiChannel, _spiSpeed, 0)) < 0) {
          fprintf(stderr, "Could not open SPI handle on 0: %s\n", lguErrorText(_spiHandle));
        }
      }
    }

    void spiBeginTransaction() {}

    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) {
      int result = lgSpiXfer(_spiHandle, (char *)out, (char*)in, len);
      if(result < 0) {
        fprintf(stderr, "Could not perform SPI transfer: %s\n", lguErrorText(result));
      }
    }

    void spiEndTransaction() {}

    void spiEnd() {
      if(_spiHandle >= 0) {
        lgSpiClose(_spiHandle);
        _spiHandle = -1;
      }
    }

    void tone(uint32_t pin, unsigned int frequency, unsigned long duration = 0) {
      lgTxPwm(_gpioHandle, pin, frequency, 50, 0, duration);
    }

    void noTone(uint32_t pin) {
      lgTxPwm(_gpioHandle, pin, 0, 0, 0, 0);
    }

    void pullUpDown(uint32_t pin, bool enable, bool up) {
      if((pin == RADIOLIB_NC) || (pin > PI_MAX_USER_GPIO)) {
        return;
      }

      pinFlags[pin] = enable ? (up ? LG_SET_PULL_UP : LG_SET_PULL_DOWN) : LG_SET_PULL_NONE;
    }

    // interrupt emulation
    bool interruptEnabled[PI_MAX_USER_GPIO + 1];
    uint32_t interruptModes[PI_MAX_USER_GPIO + 1];
    typedef void (*RadioLibISR)(void);
    RadioLibISR interruptCallbacks[PI_MAX_USER_GPIO + 1];

  private:
    // the HAL can contain any additional private members
    const uint8_t _gpioDevice;
    const uint8_t _spiDevice;
    const unsigned int _spiSpeed;
    const uint8_t _spiChannel;
    int _gpioHandle = -1;
    int _spiHandle = -1;

    int pinFlags[PI_MAX_USER_GPIO + 1] = { 0 };
};

// this handler emulates interrupts
static void lgpioAlertHandler(int num_alerts, lgGpioAlert_p alerts, void *userdata) {
  if(!userdata)
    return;

  // PiHal instance is passed via the user data
  PiHal* hal = (PiHal*)userdata;

  // check the interrupt is enabled, the level matches and a callback exists
  for(lgGpioAlert_t *alert = alerts; alert < (alerts + num_alerts); alert++) {
    if((hal->interruptEnabled[alert->report.gpio]) &&
       (hal->interruptModes[alert->report.gpio] == alert->report.level) &&
       (hal->interruptCallbacks[alert->report.gpio])) {
      hal->interruptCallbacks[alert->report.gpio]();
    }
  }
}

#endif
