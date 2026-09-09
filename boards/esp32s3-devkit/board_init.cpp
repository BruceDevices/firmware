#include <Arduino.h>
#include "pins_arduino.h"
#include "display_sh1106_adapter.h"
#include "include/globals.h"

// Forward declarations for ISRs
void IRAM_ATTR isr_up();
void IRAM_ATTR isr_down();
void IRAM_ATTR isr_left();
void IRAM_ATTR isr_right();
void IRAM_ATTR isr_sel();
void IRAM_ATTR isr_back();

void setup_buttons_irq() {
    // Configure pins as inputs with pullups and attach ISRs
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(DOWN_BTN, INPUT_PULLUP);
    pinMode(LEFT_BTN, INPUT_PULLUP);
    pinMode(RIGHT_BTN, INPUT_PULLUP);
    pinMode(SEL_BTN, INPUT_PULLUP);
    pinMode(BACK_BTN, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(UP_BTN), isr_up, FALLING);
    attachInterrupt(digitalPinToInterrupt(DOWN_BTN), isr_down, FALLING);
    attachInterrupt(digitalPinToInterrupt(LEFT_BTN), isr_left, FALLING);
    attachInterrupt(digitalPinToInterrupt(RIGHT_BTN), isr_right, FALLING);
    attachInterrupt(digitalPinToInterrupt(SEL_BTN), isr_sel, FALLING);
    attachInterrupt(digitalPinToInterrupt(BACK_BTN), isr_back, FALLING);
}

void IRAM_ATTR isr_up()    { UpPress = true;    AnyKeyPress = true; }
void IRAM_ATTR isr_down()  { DownPress = true;  AnyKeyPress = true; }
void IRAM_ATTR isr_left()  { PrevPress = true;  AnyKeyPress = true; }
void IRAM_ATTR isr_right() { NextPress = true;  AnyKeyPress = true; }
void IRAM_ATTR isr_sel()   { SelPress = true;   AnyKeyPress = true; }
void IRAM_ATTR isr_back()  { EscPress = true;   AnyKeyPress = true; }

// Board-level init that callers can call from main
bool board_init() {
    Serial.println("Board init: esp32s3-devkit");
    if (!initOLED()) {
        Serial.println("Warning: OLED init failed");
    }
    setup_buttons_irq();
    return true;
}
