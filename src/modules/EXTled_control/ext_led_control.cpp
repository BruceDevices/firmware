/*
#include "ext_led_control.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
*/

#include "core/display.h"
#include "core/utils.h"
#include <stdlib.h>

#include "core/menu_items/EXTLedControlMenu.h"

#include "driver/ledc.h"
#include <Arduino.h>

const int PWM_CHANNEL = 0;    // ESP32 has 16 channels which can generate 16 independent waveforms
const int PWM_FREQ = 500;     // Recall that Arduino Uno is ~490 Hz. Official ESP32 example uses 5,000Hz
const int PWM_RESOLUTION = 8; // esp up to 16

// The max duty cycle value based on PWM resolution (will be 255 if resolution is 8 bits)
const int MAX_DUTY_CYCLE = (int)(pow(2, PWM_RESOLUTION) - 1);

// See pin diagram here: https://makeabilitylab.github.io/physcomp/esp32/
const int LED_OUTPUT_PIN = 26;

const int DELAY_MS = 4; // delay between fade increments
int _ledFadeStep = 5;   // amount to fade per loop
int ledState = 0;
void setupPINS() {

    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

    // ledcAttachPin(uint8_t pin, uint8_t channel);
    ledcAttachPin(LED_OUTPUT_PIN, PWM_CHANNEL);
}

void GoToLedMenu() {
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Led Control");
}

void ledOff() {
    ledcWrite(PWM_CHANNEL, 0);
    ledState = 0;
}

void ledOn() {
    ledcWrite(PWM_CHANNEL, MAX_DUTY_CYCLE);
    ledState = 1;
}

void fade() {
    setupPINS();

    while (true) {
        if (check(PrevPress)) {
            ledOff();
            returnToMenu = false;
            return;
            // GoToLedMenu();
        }
        bool shouldBreak = false;

        // Fade up
        for (int dutyCycle = 0; dutyCycle <= MAX_DUTY_CYCLE; dutyCycle++) {
            if (check(PrevPress)) {
                shouldBreak = true;
                ledOff();
                returnToMenu = false;
                return;
                // GoToLedMenu();
            }
            ledcWrite(PWM_CHANNEL, dutyCycle);
            delay(DELAY_MS);
        }
        returnToMenu = false;
        if (shouldBreak) return;

        // Fade down
        for (int dutyCycle = MAX_DUTY_CYCLE; dutyCycle >= 0; dutyCycle--) {
            if (check(PrevPress)) {
                shouldBreak = true;
                ledOff();
                returnToMenu = false;
                return;
                GoToLedMenu();
            }
            ledcWrite(PWM_CHANNEL, dutyCycle);
            delay(DELAY_MS);
        }

        if (shouldBreak) {
            ledOff();
            returnToMenu = false;
            return;
            GoToLedMenu();
        }
    }
}

bool isLedOn() { return ledState > 0; }

void power() {
    setupPINS();
    if (isLedOn()) {
        ledOff();
    } else {
        ledOn();
    }
}

void blink() {
    setupPINS();
    while (true) {
        if (check(PrevPress)) {
            ledOff();
            return;
        }
        ledOn();
        delay(100);
        ledOff();
        delay(100);
    }
}

void FastBlink() {
    setupPINS();
    while (true) {
        if (check(PrevPress)) {
            ledOff();
            return;
        }
        ledOn();
        delay(44);
        ledOff();
        delay(44);
    }
}

void LedConfig() { setupPINS(); }
