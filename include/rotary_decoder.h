#pragma once
#include <Arduino.h>

static const int8_t ROTARY_DECODER_ENC_TABLE[16] = {
    0, +1, -1, 0, -1, 0, 0, +1, +1, 0, 0, -1, 0, -1, +1, 0,
};

class RotaryDecoder {
public:
    void begin(uint8_t pinA, uint8_t pinB, uint8_t stepsPerDetent = 2) {
        _pinA = pinA;
        _pinB = pinB;
        _stepsPerDetent = stepsPerDetent;
        bool a = digitalRead(_pinA);
        bool b = digitalRead(_pinB);
        _abState = (a << 1) | b;
        _rawPosition = 0;
        _position = 0;
    }

    void poll() {
        bool a = digitalRead(_pinA);
        bool b = digitalRead(_pinB);
        uint8_t newAb = (a << 1) | b;
        if (newAb == _abState) return;

        int8_t delta = ROTARY_DECODER_ENC_TABLE[(_abState << 2) | newAb];
        _abState = newAb;
        if (delta == 0) return;

        _rawPosition += delta;
        _position = floorDiv(_rawPosition, (int32_t)_stepsPerDetent);
    }

    int32_t getPosition() { return _position; }

private:
    static int32_t floorDiv(int32_t a, int32_t b) {
        int32_t q = a / b;
        if ((a % b != 0) && ((a < 0) != (b < 0))) q--;
        return q;
    }

    uint8_t _pinA = 0;
    uint8_t _pinB = 0;
    uint8_t _stepsPerDetent = 2;
    uint8_t _abState = 0;
    int32_t _rawPosition = 0;
    volatile int32_t _position = 0;
};
