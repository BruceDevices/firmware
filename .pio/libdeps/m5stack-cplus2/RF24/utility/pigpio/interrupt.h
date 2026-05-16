/**
 * Interrupt functions wrapped from pigpio library
 */
#ifndef RF24_UTILITY_PIGPIO_INTERRUPT_H_
#define RF24_UTILITY_PIGPIO_INTERRUPT_H_

#include <pigpio.h>

#define INT_EDGE_FALLING FALLING_EDGE
#define INT_EDGE_RISING  RISING_EDGE
#define INT_EDGE_BOTH    EITHER_EDGE

#ifdef __cplusplus
extern "C" {
#endif

int attachInterrupt(int pin, int mode, void (*function)(void));

int detachInterrupt(int pin);

/* Deprecated, no longer functional */
void rfNoInterrupts();

/* Deprecated, no longer functional */
void rfInterrupts();

#ifdef __cplusplus
}
#endif
#endif // RF24_UTILITY_PIGPIO_INTERRUPT_H_