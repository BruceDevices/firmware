// random.h

#ifndef RANDOM_H
#define RANDOM_H

#include "core/display.h"
#include "core/utils.h"

class Randomizer {
public:
    Randomizer();
    ~Randomizer();
    void setup();
    void loop();

private:
    int randomNumber;
    int generateRandomNumber();
};

#endif // RANDOM_H
