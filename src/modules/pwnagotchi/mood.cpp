/*
Thanks to thoses developers for their projects:
* @7h30th3r0n3 : https://github.com/7h30th3r0n3/Evil-M5Core2 and https://github.com/7h30th3r0n3/PwnGridSpam
* @viniciusbo : https://github.com/viniciusbo/m5-palnagotchi
* @sduenasg : https://github.com/sduenasg/pio_palnagotchi

Thanks to @bmorcelli for his help doing a better code.
*/
#if !defined(LITE_VERSION)
#include "mood.h"

// ASCII equivalent
const String palnagotchi_moods[] = {
    "(v__v)", // 0 - sleeping
    "(=__=)", // 1 - awakening
    "(O__O)", // 2 - awake
    "( O_O)", // 3 - observing (neutral) right
    "(O_O )", // 4 - observig (neutral) left
    "( 0_0)", // 5 - observing (happy) right
    "(0_0 )", // 6 - observing (happy) left
    "(+__+)", // 7 - intense
    "(-@_@)", // 8 - cool
    "(0__0)", // 9 - happy
    "(^__^)", // 10 - grateful
    "(UwU )", // 11 - excited
    "(+__+)", // 12 - smart
    "(*__*)", // 13 - friendly
    "(@__@)", // 14 - motivated
    "(>__<)", // 15 - demotivated
    "(-__-)", // 16 - bored
    "(T_T )", // 17 - sad
    "(;__;)", // 18 - lonely
    "(X__X)", // 19 - broken
    "(#__#)", // 20 - debugging
    "(-_-')", // 21 - angry
    "(^__^)", // 22- helping
};

const String palnagotchi_moods_desc[] = {
    "Zzzz...",                              // 0 - sleeping
    "...",                                  // 1 - awakening
    "Let's MAKE FRIEDNS!",                  // 2 - awake
    "WANTED: FRIENDS",                      // 3 - observing (neutral) right
    "WANTED: FRIENDS",                      // 4 - observig (neutral) left
    "Looking for friends that pwn.",        // 5 - observing (happy) right
    "Looking for friends that pwn.",        // 6 - observing (happy) left
    "YEAH! So many pwnagotchis!",           // 7 - intense
    "The coolest pal in the neighbourhood", // 8 - cool
    "Can we have even more friends?",       // 9 - happy
    "I LOVE PWNAGOTCHIS!",                  // 10 - grateful
    "When I grow up, I'll PWN myself!!",    // 11 - excited
    "3.1415926535897932384626433832795",    // 12 - smart
    "HEY YOU! LETS BE FRIENDS!",            // 13 - friendly
    "IT RUNS! PWND!",                       // 14 - motivated
    "I don't PWN, therefore i don't am.",   // 15 - demotivated
    "Seriously, let's go for a walk...",    // 16 - bored
    "Really? This hardware? can't PWN...",  // 17 - sad
    "Where are all the Pwnagotchis?",       // 18 - lonely
    "It works on my end.",                  // 19 - broken
    "My friends pwn.",                      // 20 - debugging
    "SO ANGERY, BUT CANT PWN",              // 21 - angry
    "Helping Pwnagotchis be sane since 2024",
};

uint8_t current_mood = 0;
String current_phrase = "";
String current_face = "";
bool current_broken = false;
const int number_of_moods = 23; // sizeof(palnagotchi_moods);

uint8_t getCurrentMoodId() { return current_mood; }
int getNumberOfMoods() { return number_of_moods; }
String getCurrentMoodFace() { return current_face; }
String getCurrentMoodPhrase() { return current_phrase; }
bool isCurrentMoodBroken() { return current_broken; }

void setMood(uint8_t mood, String face, String phrase, bool broken) {
    current_mood = mood;
    current_broken = broken;

    if (face != "") {
        current_face = face;
    } else {
        current_face = palnagotchi_moods[current_mood];
    }

    if (phrase != "") {
        current_phrase = phrase;
    } else {
        current_phrase = palnagotchi_moods_desc[current_mood];
    }
}
#endif
