#ifndef _IO_EXPANDER_H
#define _IO_EXPANDER_H
#include "pins_arduino.h"
#include <Arduino.h>
#include <Wire.h>

#ifdef IO_EXPANDER_AW9523
#include "Adafruit_AW9523.h"
#define IO_EXP_CLASS Adafruit_AW9523
#define IO_EXPANDER_ADDRESS AW9523_DEFAULT_ADDR // 0x58
#elif defined(IO_EXPANDER_PCA9555)
#include "PCA9555.h"0
#define IO_EXP_CLASS PCA9555
#define IO_EXPANDER_ADDRESS PCA9555_DEFAULT_ADDR // 0x58
#endif

#ifndef IO_EXP_GPS // Used in Smoochiee and T-Lora
#define IO_EXP_GPS -1
#endif
#ifndef IO_EXP_MIC // Used in Smoochiee
#define IO_EXP_MIC -1
#endif
#ifndef IO_EXP_VIBRO // Used in Smoochiee
#define IO_EXP_VIBRO -1
#endif
#ifndef IO_EXP_CC_RX // Used in Smoochiee
#define IO_EXP_CC_RX -1
#endif
#ifndef IO_EXP_CC_TX // Used in Smoochiee
#define IO_EXP_CC_TX -1
#endif
#ifndef IO_EXP_LOGO // Used in REAPER
#define IO_EXP_LOGO -1
#endif
#ifndef IO_EXP_NRF // Used in C5
#define IO_EXP_NRF -1
#endif

// Button pins (likely inputs on the expander)
#ifndef IO_EXP_UP
#define IO_EXP_UP -1
#endif
#ifndef IO_EXP_DOWN
#define IO_EXP_DOWN -1
#endif
#ifndef IO_EXP_ESC
#define IO_EXP_ESC -1
#endif
#ifndef IO_EXP_LEFT
#define IO_EXP_LEFT -1
#endif
#ifndef IO_EXP_RIGHT
#define IO_EXP_RIGHT -1
#endif
#ifndef IO_EXP_SEL
#define IO_EXP_SEL -1
#endif

#if defined(IO_EXPANDER_AW9523) || defined(IO_EXPANDER_PCA9555)

class io_expander : public IO_EXP_CLASS {
private:
    bool _started = false;

public:
    io_expander() : IO_EXP_CLASS() {};

    bool init(uint8_t a = IO_EXPANDER_ADDRESS, TwoWire *_w = &Wire) {
        _started = begin(a, _w);
        if (!_started) return false;

        configureDirection(0xFFFF); // All outputs initially

        turnPinOnOff(IO_EXP_GPS, LOW);   // SMOOOCHIE||REAPER
        turnPinOnOff(IO_EXP_MIC, LOW);   // SMOOOCHIE
        turnPinOnOff(IO_EXP_VIBRO, LOW); // SMOOOCHIE||REAPER
        turnPinOnOff(IO_EXP_CC_RX, LOW); // SMOOOCHIE||REAPER
        turnPinOnOff(IO_EXP_CC_TX, LOW); // SMOOOCHIE||REAPER
        turnPinOnOff(IO_EXP_LOGO, HIGH); // BRUCE LOGO LEAD ON REAPER
        turnPinOnOff(IO_EXP_NRF, HIGH);  // NRF ON BY DEFAULT FOR C5

        // Set button pins as inputs
        button(IO_EXP_UP);
        button(IO_EXP_DOWN);
        button(IO_EXP_ESC);
        button(IO_EXP_LEFT);
        button(IO_EXP_RIGHT);
        button(IO_EXP_SEL);

        // IMPORTANT: Disable all interrupts at startup
        interruptEnableGPIO(0x0000);

        return _started;
    }

    void turnPinOnOff(int8_t pin, bool val) {
        if (!_started) return;
        return pin >= 0 ? IO_EXP_CLASS::digitalWrite(pin, val) : delay(0);
    }

    void setPinDirection(uint8_t pin, uint8_t mode) {
        if (!_started || pin > 15) return;
        pinMode(pin, mode);
    }

    bool readPin(int8_t pin) {
        if (!_started || pin < 0 || pin > 15) return false;
        return digitalRead(pin);
    }

    void button(int8_t pin) {
        if (pin >= 0 && pin <= 15) { setPinDirection(pin, INPUT); }
    }

  bool enableGPIOInterrupts() {
    if (!_started) return false;

    // Start with ALL interrupts DISABLED (bits = 1 = disable)
    uint16_t mask = 0xFFFF;

    // Clear bits (set to 0 = enable) for the input/button pins
    if (IO_EXP_UP    >= 0 && IO_EXP_UP    <= 15) mask &= ~(1 << IO_EXP_UP);
    if (IO_EXP_DOWN  >= 0 && IO_EXP_DOWN  <= 15) mask &= ~(1 << IO_EXP_DOWN);
    if (IO_EXP_ESC   >= 0 && IO_EXP_ESC   <= 15) mask &= ~(1 << IO_EXP_ESC);
    if (IO_EXP_LEFT  >= 0 && IO_EXP_LEFT  <= 15) mask &= ~(1 << IO_EXP_LEFT);
    if (IO_EXP_RIGHT >= 0 && IO_EXP_RIGHT <= 15) mask &= ~(1 << IO_EXP_RIGHT);
    if (IO_EXP_SEL   >= 0 && IO_EXP_SEL   <= 15) mask &= ~(1 << IO_EXP_SEL);

    return interruptEnableGPIO(mask);
}
    // Optional: turn off all interrupts
    void disableGPIOInterrupts() {
        if (_started) interruptEnableGPIO(0x0000);
    }
};

#else
#define IO_EXPANDER_ADDRESS 0
// dummy class
class io_expander {
private:
    /* data */
public:
    io_expander() {};
    ~io_expander() {};
    void turnPinOnOff(int8_t pin, bool val) {};
    bool init(uint8_t a, TwoWire *_w) { return false; };
    void setPinDirection(uint8_t pin, uint8_t mode) {}
    bool readPin(int8_t pin) { return false; }
    void button(int8_t pin) {}
    bool enableGPIOInterrupts() { return false; }
    void disableGPIOInterrupts() {}
};

#endif // #ifdef IO_EXPANDER_AW9523
#endif // _IO_EXPANDER_H
