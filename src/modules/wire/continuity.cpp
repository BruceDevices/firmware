#if !defined(LITE_VERSION)
#include "wire_tools.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/settings.h"
#include "core/utils.h"
#include <globals.h>
#include <Arduino.h>
#include "modules/others/audio.h"

/*********************************************************************
**  Multimeter: continuity tests only.
**
**  Two-pin mode: pin A is driven LOW while pin B is pulled up internally;
**  a LOW on B means A-B are electrically connected (the current loop runs
**  through the probe points). GND mode checks one pin against the board
**  GND through its internal pull-up.
**********************************************************************/

static int8_t mmPinA = -1, mmPinB = -1;

#define CONT_DEBOUNCE_READS 3
#define CONT_READ_INTERVAL_MS 5
#define CONT_BEEP_INTERVAL_MS 300

// Classification of one measurement pass. CONT_HELD means the probe is held by
// something other than our drive/pull (e.g. the Cardputer ADV wires the Grove
// G1/G2 to the onboard CH9329, whose TX idles HIGH; CS pins 4/5/6/13 are driven
// HIGH by board init). Such pins can never show SHORT and would otherwise
// silently report OPEN forever. The check also keeps the tester from fighting
// an external driver: a held pin is detected before we ever drive it LOW.
enum ContState : uint8_t { CONT_OPEN = 0, CONT_SHORT = 1, CONT_HELD = 2 };

static uint8_t cont_classify(bool toGnd, int8_t pinA, int8_t pinB) {
    // 2 ms settle: long probe cables add nF-level capacitance; a 30 us settle
    // would read the previous pull's level and report a false SHORT
    delayMicroseconds(2000);
    if (toGnd) {
        pinMode(pinA, INPUT_PULLUP);
        bool up = digitalRead(pinA) == HIGH;
        pinMode(pinA, INPUT_PULLDOWN);
        bool down = digitalRead(pinA) == HIGH;
        if (!up) return CONT_SHORT; // pulled low against the internal pull-up
        if (down) return CONT_HELD; // HIGH even against the pull-down: driven externally
        return CONT_OPEN;
    }
    // Two-pin: first make sure the drive pin is not held by another device,
    // otherwise our LOW drive would fight it (and never win).
    pinMode(pinA, INPUT_PULLUP);
    bool aUp = digitalRead(pinA) == HIGH;
    pinMode(pinA, INPUT_PULLDOWN);
    bool aDown = digitalRead(pinA) == HIGH;
    if (aUp && aDown) return CONT_HELD;

    pinMode(pinA, OUTPUT);
    digitalWrite(pinA, LOW);
    pinMode(pinB, INPUT_PULLUP);
    bool bUp = digitalRead(pinB) == HIGH;
    pinMode(pinB, INPUT_PULLDOWN);
    bool bDown = digitalRead(pinB) == HIGH;
    pinMode(pinA, INPUT);
    if (!bUp) return CONT_SHORT;
    if (bDown) return CONT_HELD;
    return CONT_OPEN;
}

/*********************************************************************
**  Function: cont_live_screen
**  Shared live view for both modes; `toGnd` selects the wiring scheme.
**********************************************************************/
static void cont_live_screen(bool toGnd) {
    int8_t pinA = mmPinA, pinB = mmPinB;
    if (pinA < 0) {
        displayError("Assign Pin A first", true);
        return;
    }
    if (!toGnd) {
        if (pinB < 0) {
            displayError("Assign Pin B first", true);
            return;
        }
        if (pinA == pinB) {
            displayError("Pins must be distinct", true);
            return;
        }
    }

    drawMainBorderWithTitle(toGnd ? "Continuity: GND" : "Continuity: Pin-Pin");
    padprintln("");
    if (toGnd) padprintln("Probe: GPIO " + String(pinA) + " vs GND");
    else padprintln("Probes: GPIO " + String(pinA) + " <-> GPIO " + String(pinB));
    padprintln("");
    tft.setTextSize(4);

    uint8_t state = CONT_OPEN;
    uint8_t lastDrawn = 0xFF; // force first draw
    uint8_t debounce = 0;
    uint8_t candidate = CONT_OPEN;
    uint32_t lastRead = 0;
    uint32_t shortStart = 0;
    uint32_t lastBeep = 0;
    uint32_t lastTick = 0;
    uint32_t contacts = 0;

    while (!check(EscPress)) {
        uint32_t now = millis();
        if (now - lastRead >= CONT_READ_INTERVAL_MS) {
            lastRead = now;
            uint8_t measured = cont_classify(toGnd, pinA, pinB);
            if (measured != candidate) {
                candidate = measured;
                debounce = 0;
            } else if (debounce < CONT_DEBOUNCE_READS) {
                debounce++;
            }
            if (debounce >= CONT_DEBOUNCE_READS && state != candidate) {
                state = candidate;
                if (state == CONT_SHORT) {
                    contacts++;
                    shortStart = now;
                    lastBeep = 0; // beep immediately
                }
            }
        }

        uint32_t shortMs = state == CONT_SHORT ? now - shortStart : 0;
        if (state != lastDrawn || (state == CONT_SHORT && now - lastTick >= 500)) {
            lastDrawn = state;
            lastTick = now;
            tft.setTextSize(4);
            tft.setTextDatum(MC_DATUM);
            uint16_t color = state == CONT_SHORT ? TFT_GREEN : (state == CONT_HELD ? TFT_YELLOW : TFT_RED);
            tft.setTextColor(color, bruceConfig.bgColor);
            tft.drawString(state == CONT_SHORT ? "SHORT" : (state == CONT_HELD ? "HELD" : "OPEN"),
                           tftWidth / 2, tftHeight / 2 + 12);
            tft.setTextSize(FP);
            tft.setTextDatum(BC_DATUM);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            String status;
            if (state == CONT_SHORT) {
                status = "contact #" + String(contacts) + "  " + String(shortMs / 1000) + "." +
                         String((shortMs % 1000) / 100) + "s";
            } else if (state == CONT_HELD) {
                status = "pin driven by another device";
            } else {
                status = "waiting...";
            }
            tft.drawString(status, tftWidth / 2, tftHeight - 30);
            tft.setTextDatum(TL_DATUM);
        }

        if (state == CONT_SHORT && now - lastBeep >= CONT_BEEP_INTERVAL_MS) {
            lastBeep = now;
            _tone(4000, 100, PLAYBACK_ASYNC);
        }

        wakeUpScreen();
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    pinMode(pinA, INPUT);
    if (!toGnd) pinMode(pinB, INPUT);
    check(EscPress);
}

/*********************************************************************
**  GPIO picker (same interaction as JTAG pin assignment)
**********************************************************************/
static int8_t cont_pick_pin(const char *title, int8_t current) {
    options.clear();
    int8_t picked = current;
    for (int i = -1; i <= GPIO_NUM_MAX; i++) {
        options.push_back({String(i), [i, &picked]() { picked = (int8_t)i; }, i == current});
    }
    int index = (current < 0) ? 0 : current + 1;
    loopOptions(options, MENU_TYPE_SUBMENU, title, index);
    return picked;
}

/*********************************************************************
**  Function: continuity_tester_setup
**********************************************************************/
void continuity_tester_setup() {
    returnToMenu = false;
    while (!returnToMenu) {
        options = {
            {"Pin A: " + String(mmPinA), []() { mmPinA = cont_pick_pin("Pin A (Drive LOW)", mmPinA); }          },
            {"Pin B: " + String(mmPinB), []() { mmPinB = cont_pick_pin("Pin B (Sense)", mmPinB); }              },
            {"Continuity: Pin<->Pin",    []() { cont_live_screen(false); }                                      },
            {"Continuity: Pin->GND",     []() { cont_live_screen(true); }                                       },
        };
        addOptionToMainMenu();
        loopOptions(options, MENU_TYPE_SUBMENU, "Multimeter");
    }
}

/*********************************************************************
**  Serial-command primitive (wire cont test), see wire_tools.h
**********************************************************************/
String wire_cont_test(int8_t pinA, int8_t pinB) {
    if (pinA < 0) return "ERR: assign pin_a";
    if (pinB >= 0 && pinA == pinB) return "ERR: pins must be distinct";
    uint8_t state = cont_classify(pinB < 0, pinA, pinB);
    pinMode(pinA, INPUT);
    if (pinB >= 0) pinMode(pinB, INPUT);
    if (state == CONT_SHORT) return pinB >= 0 ? "OK SHORT (A-B)" : "OK SHORT (to GND)";
    if (state == CONT_HELD) return "OK HELD (probe driven by another device; results invalid)";
    return "OK OPEN";
}
#endif
