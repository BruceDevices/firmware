#include "PicoHal.h"

#if defined(RADIOLIB_BUILD_RPI_PICO)

// pre-calculated pulse-widths for 1200 and 2200Hz
// we do this to save calculation time (see https://github.com/khoih-prog/RP2040_PWM/issues/6)
#define SLEEP_1200 416.666
#define SLEEP_2200 227.272

static uint32_t toneLoopPin;
static unsigned int toneLoopFrequency;
static unsigned long toneLoopDuration;

// === NOTE ===
// The tone(...) implementation uses the second core on the RPi Pico. This is to diminish as much 
// jitter in the output tones as possible.

static void toneLoop() {
  gpio_set_dir(toneLoopPin, GPIO_OUT);

  uint32_t sleep_dur;
  if(toneLoopFrequency == 1200) {
    sleep_dur = SLEEP_1200;
  } else if(toneLoopFrequency == 2200) {
    sleep_dur = SLEEP_2200;
  } else {
    sleep_dur = 500000 / toneLoopFrequency;
  }

  // tone bitbang
  while(1) {
    gpio_put(toneLoopPin, 1);
    sleep_us(sleep_dur);
    gpio_put(toneLoopPin, 0);
    sleep_us(sleep_dur);
    tight_loop_contents();
  }
}

void PicoHal::tone(uint32_t pin, unsigned int frequency, unsigned long duration) {
  // tones on the Pico are generated using bitbanging. This process is offloaded to the Pico's second core
  multicore_reset_core1();
  toneLoopPin = pin;
  toneLoopFrequency = frequency;
  toneLoopDuration = duration;
  multicore_launch_core1(toneLoop);
}

#endif
