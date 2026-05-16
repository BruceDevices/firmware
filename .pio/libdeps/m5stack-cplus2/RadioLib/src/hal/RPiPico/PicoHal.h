#ifndef PICO_HAL_H
#define PICO_HAL_H

#if defined(RADIOLIB_BUILD_RPI_PICO)

// include RadioLib
#include <RadioLib.h>

// include the necessary Pico libraries
#include <pico/stdlib.h>
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"

#define PI_PICO_MAX_USER_GPIO  (48)

// because the Pico SDK does not allow to pass user data into interrupt handlers,
// we keep it as a global here. This is hacky and means that multiple PicoHal
// instances share the same interrupts, which is weird and will probably break.
// However, there seems to be no real use case for creating multiple intances of the HAL
static irq_handler_t picoHalUserCallbacks[PI_PICO_MAX_USER_GPIO] = { 0 };
static uint32_t picoHalIrqEventMasks[PI_PICO_MAX_USER_GPIO] = { 0 };
static uint64_t picoHalIrqMask = 0;

static void picoInterruptHandler(void) {
  for(int gpio = 0; gpio < PI_PICO_MAX_USER_GPIO; gpio++) {
    if(gpio_get_irq_event_mask(gpio) == picoHalIrqEventMasks[gpio]) {
      gpio_acknowledge_irq(gpio, picoHalIrqEventMasks[gpio]);
      if(picoHalUserCallbacks[gpio]) {
        picoHalUserCallbacks[gpio]();
      }
    }
  }
}

// create a new Raspberry Pi Pico hardware abstraction 
// layer using the Pico SDK
// the HAL must inherit from the base RadioLibHal class
// and implement all of its virtual methods
class PicoHal : public RadioLibHal {
public:
  PicoHal(spi_inst_t *spiChannel, uint32_t misoPin, uint32_t mosiPin, uint32_t sckPin, uint32_t spiSpeed = 500 * 1000)
    : RadioLibHal(GPIO_IN, GPIO_OUT, 0, 1, GPIO_IRQ_EDGE_RISE, GPIO_IRQ_EDGE_FALL),
    _spiChannel(spiChannel),
    _spiSpeed(spiSpeed),
    _misoPin(misoPin),
    _mosiPin(mosiPin),
    _sckPin(sckPin){}

  void init() override {
    stdio_init_all();
    spiBegin();
  }

  void term() override {
    spiEnd();
  }

  // GPIO-related methods (pinMode, digitalWrite etc.) should check
  // RADIOLIB_NC as an alias for non-connected pins
  void pinMode(uint32_t pin, uint32_t mode) override {
    if (pin == RADIOLIB_NC) {
      return;
    }

    gpio_init(pin);
    gpio_set_dir(pin, mode);
  }

  void digitalWrite(uint32_t pin, uint32_t value) override {
    if (pin == RADIOLIB_NC) {
      return;
    }

    gpio_put(pin, (bool)value);
  }

  uint32_t digitalRead(uint32_t pin) override {
    if (pin == RADIOLIB_NC) {
      return 0;
    }

    return gpio_get(pin);
  }

  void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
    if (interruptNum == RADIOLIB_NC) {
      return;
    }

    // set callbacks
    picoHalUserCallbacks[interruptNum] = (irq_handler_t)interruptCb;
    picoHalIrqEventMasks[interruptNum] = mode;

    // is it a new interrupt for us to grab ?
    if (!(picoHalIrqMask & (1ULL << interruptNum))) {
      // if we have a handler in place, we must remove it to avoid 'assert' in the PDK
      // and we must disable interrupt to make sure we don't miss anything during that
      // shuffling
      if (picoHalIrqMask) {
        irq_set_enabled(IO_IRQ_BANK0, false);
        gpio_remove_raw_irq_handler_masked64(picoHalIrqMask, picoInterruptHandler);
      }

      // (re-)add shared handler with the new mask
      picoHalIrqMask |= (1ULL << interruptNum);
      gpio_add_raw_irq_handler_masked64(picoHalIrqMask, picoInterruptHandler);
      irq_set_enabled(IO_IRQ_BANK0, true);
    }

    // enable GPIO to generate interrupt
    gpio_set_irq_enabled(interruptNum, mode, true);
  }

  void detachInterrupt(uint32_t interruptNum) override {
    if (interruptNum == RADIOLIB_NC) {
      return;
    }

    // disable the IRQ
    gpio_set_irq_enabled(interruptNum, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

    // clear callbacks
    picoHalUserCallbacks[interruptNum] = NULL;
    picoHalIrqEventMasks[interruptNum] = 0;
  }

  void delay(unsigned long ms) override {
    sleep_ms(ms);
  }

  void delayMicroseconds(unsigned long us) override {
    sleep_us(us);
  }

  unsigned long millis() override {
    return to_ms_since_boot(get_absolute_time());
  }

  unsigned long micros() override {
    return to_us_since_boot(get_absolute_time());
  }

  long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
    if (pin == RADIOLIB_NC) {
      return 0;
    }

    this->pinMode(pin, GPIO_IN);
    uint32_t start = this->micros();
    uint32_t curtick = this->micros();

    while (this->digitalRead(pin) == state) {
      if ((this->micros() - curtick) > timeout) {
        return 0;
      }
    }

    return (this->micros() - start);
  }

  void tone(uint32_t pin, unsigned int frequency, unsigned long duration = 0) override;

  void noTone(uint32_t pin) override {
    multicore_reset_core1();
  }

  void spiBegin() {
    spi_init(_spiChannel, _spiSpeed);
    spi_set_format(_spiChannel, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(_sckPin, GPIO_FUNC_SPI);
    gpio_set_function(_mosiPin, GPIO_FUNC_SPI);
    gpio_set_function(_misoPin, GPIO_FUNC_SPI);
  }

  void spiBeginTransaction() {}

  void spiTransfer(uint8_t *out, size_t len, uint8_t *in) {
    spi_write_read_blocking(_spiChannel, out, in, len);
  }

  void yield() override {
  	tight_loop_contents();
  }

  void spiEndTransaction() {}

  void spiEnd() {
    spi_deinit(_spiChannel);
  }

private:
  // the HAL can contain any additional private members
  spi_inst_t *_spiChannel;
  uint32_t _spiSpeed;
  uint32_t _misoPin;
  uint32_t _mosiPin;
  uint32_t _sckPin;
};

#endif

#endif
