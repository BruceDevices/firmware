#include <Wire.h>
#include <Adafruit_SH110X.h>
#include "pins_arduino.h"

Adafruit_SH1106G display(128, 64, &Wire, -1);

#define UP_BTN  UP_BTN
#define DOWN_BTN DOWN_BTN
#define LEFT_BTN LEFT_BTN
#define RIGHT_BTN RIGHT_BTN
#define SEL_BTN SEL_BTN
#define BACK_BTN BACK_BTN

struct ButtonState {
    uint32_t lastPress;
    bool lastState;
};

ButtonState btnStates[6];
const uint8_t BTN_PINS[6] = {UP_BTN, DOWN_BTN, LEFT_BTN, RIGHT_BTN, SEL_BTN, BACK_BTN};
const char* btnLabels[6] = {"UP", "DOWN", "LEFT", "RIGHT", "SEL", "BACK"};
bool btnPressed[6] = {false};

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\nESP32-S3 OLED Button Test (devkit)");

    Wire.begin(SDA, SCL);
    Wire.setClock(400000);

    if (!display.begin(0x3C, true)) {
        Serial.println("OLED init failed!");
        while (1);
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("Button Test");
    display.display();

    for (uint8_t i = 0; i < 6; i++) {
        pinMode(BTN_PINS[i], INPUT_PULLUP);
        btnStates[i].lastState = HIGH;
        btnStates[i].lastPress = 0;
    }
    Serial.println("Setup complete");
}

bool readButton(uint8_t idx, uint32_t now) {
    bool state = digitalRead(BTN_PINS[idx]);
    if (state != btnStates[idx].lastState) {
        btnStates[idx].lastState = state;
        btnStates[idx].lastPress = now;
        return false;
    }
    if (state == LOW && (now - btnStates[idx].lastPress) >= 50) {
        btnStates[idx].lastPress = now;
        return true;
    }
    return false;
}

void loop() {
    static uint32_t lastDisplay = 0;
    uint32_t now = millis();

    for (uint8_t i = 0; i < 6; i++) {
        if (readButton(i, now)) {
            btnPressed[i] = true;
            Serial.printf("%s pressed\n", btnLabels[i]);
        }
    }

    if (now - lastDisplay >= 200) {
        lastDisplay = now;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Buttons:");
        for (uint8_t i = 0; i < 6; i++) {
            display.print(btnLabels[i]);
            display.println(btnPressed[i] ? " [ON]" : "");
            btnPressed[i] = false;
        }
        display.display();
    }
    delay(5);
}
