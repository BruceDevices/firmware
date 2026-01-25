// random.cpp

#include "random.h"
#include "core/display.h"
#include "core/utils.h"
#include <stdlib.h>
#include <string>
#include <time.h>

Randomizer::Randomizer() { setup(); }

Randomizer::~Randomizer() {
    tft.fillScreen(bruceConfig.bgColor);
    backToMenu();
}

void Randomizer::setup() {
    srand((unsigned)time(NULL));
    randomNumber = generateRandomNumber();
    loop();
}

void Randomizer::loop() {
    bool newRandomGenerated = false;

    while (true) {
        if (check(SelPress)) {
            randomNumber = generateRandomNumber();
            newRandomGenerated = true;
        }

        if (check(PrevPress)) { break; }

        if (newRandomGenerated) {
            tft.fillScreen(bruceConfig.bgColor);
            drawMainBorderWithTitle("Randomizer", false);

            // char randomString[10];
            // snprintf(randomString, sizeof(randomString), "%d", randomNumber);  // Convert number to string

            String randomString;
            if (randomNumber == 0) {
                randomString = "H";
            } else {
                randomString = "I";
            }

            tft.setTextSize(4);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawCentreString(randomString, tft.width() / 2, tft.height() / 2, 1);

            newRandomGenerated = false;
        }

        delay(80);
    }
}

int Randomizer::generateRandomNumber() { return rand() % 2; }
