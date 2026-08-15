/**
 * IR Jammer Header File - Aggressive Continuous Jammer
 *
 * Primary: Continuous 38kHz carrier with adjustable duty cycle (saturates AGC)
 * Secondary: Broadband noise on 30/33/36/40/42/56 kHz
 * Configurable strength via PWM-style duty cycle
 */

#ifndef __IR_JAMMER_H__
#define __IR_JAMMER_H__

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <globals.h>

// Jammer state with strength control
struct JammerState {
    uint32_t jamCount = 0;
    uint32_t startTime = 0;
    uint32_t lastUIUpdate = 0;
    bool redraw = true;
    uint8_t strength = 50;   // Duty cycle 10-100%
    bool strengthMode = false;  // UI mode: adjusting strength
};

void startIrJammer();
void initJammerState(JammerState &state);
void setupJammer(IRsend &irsend);
void renderJammerUI(JammerState &state);
void handleJammerInput(JammerState &state);
void performJamming(JammerState &state, IRsend &irsend);
void cleanupJammer(IRsend &irsend);

#endif // __IR_JAMMER_H__