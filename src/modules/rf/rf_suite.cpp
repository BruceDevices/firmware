#include "rf_suite.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "rf_ui_shim.h"
#include "rf_bits_ui.h"
#include "rf_field_meter.h"
#include "rf_freq_analyzer.h"
#include "rf_wardriver.h"

#include <globals.h>

// ---------------------------------------------------------------------------
// Vandeta RF Suite
//
// The IR Suite (IR > IR Suite) is a script that draws its own screen: a
// gradient title band with "VANDETA IR", a gradient highlight on the selected
// row and a footer hint. This page copies that chrome in C++ so the sub-GHz
// tools can live behind one row of the RF menu and still look like the same
// suite. The layout constants are the ones the script uses (26 px title band,
// 16 px rows) with the row height grown to 21 px because there are five rows
// and no remote-button row above them.
//
// Nothing here owns a radio: every row calls the page that already exists, so
// the memory behaviour of those pages is unchanged.
// ---------------------------------------------------------------------------

#define SUITE_TITLE_H 26
#define SUITE_ROW_TOP 34
#define SUITE_ROW_H 18

struct SuiteEntry {
    const char *label;
    void (*run)();
};

static const SuiteEntry SUITE_ENTRIES[] = {
    {"Frequency Analyzer", rf_freq_analyzer},
    {"Field Meter", rf_field_meter},
    // "Packet Monitor" belongs here too; it arrives with core/packet_view.cpp in
    // the follow-up commit, because that page is built on the BLE capture
    // module as well and both are needed before the row can be shown.
    // Task 2: the encoder front end of `rfbits`. It sits in the suite with the
    // other sub-GHz tools rather than as a top-level RF row.
    {"Bit Editor", rfBitsEditor},
#ifndef LITE_VERSION
    {"Wardriver", rfWardriverMenu},
#endif
    {"Exit", nullptr},
};
#define SUITE_COUNT ((int)(sizeof(SUITE_ENTRIES) / sizeof(SUITE_ENTRIES[0])))

static void suiteDraw(int sel) {
    // Full repaint: a menu of five rows costs less to redraw than it costs to
    // track which pixels belong to the old selection, and it matches what the
    // IR Suite does on every cursor move.
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(1);

    uiGradientFillRect(0, 0, tftWidth, SUITE_TITLE_H);
    tft.setTextColor(TFT_BLACK); // same value for fg and bg = transparent glyphs
    tft.setTextSize(2);
    tft.setCursor(8, 5);
    tft.print("VANDETA RF");
    tft.setTextSize(1);
    tft.drawRightString("SUITE 1.0", tftWidth - 8, 9, 1);

    for (int i = 0; i < SUITE_COUNT; i++) {
        const int y = SUITE_ROW_TOP + i * SUITE_ROW_H;
        const bool on = (i == sel);
        if (on) uiGradientFillRoundRect(4, y - 2, tftWidth - 8, SUITE_ROW_H, 3);
        tft.setTextColor(on ? TFT_BLACK : bruceConfig.secColor);
        tft.setCursor(8, y + 4);
        tft.print(on ? "> " : "  ");
        tft.print(SUITE_ENTRIES[i].label);
    }

    tft.setTextSize(1);
    tft.drawFastHLine(8, tftHeight - 22, tftWidth - 16, bruceConfig.priColor);
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.drawCentreString("SEL open   BACK exit", tftWidth / 2, tftHeight - 13, 1);
}

// One action per press. The flags are re-asserted while a key is held (the
// encoder publishes a step per detent, the console injects a whole press), so
// without this the cursor ran through every row on a single press.
static void suiteEndPress() {
    PrevPress = false;
    NextPress = false;
    UpPress = false;
    DownPress = false;
    SelPress = false;
    vTaskDelay(pdMS_TO_TICKS(150));
}

void rf_suite() {
    int sel = 0;
    suiteDraw(sel);

    // The SEL that opened the suite may still be down when the first frame is
    // up; without a grace period it would select row 0 immediately. Same guard
    // loopOptions() uses, shortened because this page is a launcher.
    const uint32_t opened = millis();
    const uint32_t SELECT_GRACE_MS = 300;

    for (;;) {
        // Up/Down as well as the encoder flags: loopOptions() accepts both,
        // boards with real direction buttons set Up/Down, and the console's
        // `ui up`/`ui down` inject Up/Down - only listening to the encoder
        // flags left this page un-navigable over USB. Both flags are READ
        // (not short-circuited) because one step can set both: leaving one
        // standing moved the cursor twice.
        const bool up = check(PrevPress) | check(UpPress);
        const bool down = check(NextPress) | check(DownPress);
        if (up) {
            sel = (sel + SUITE_COUNT - 1) % SUITE_COUNT;
            suiteDraw(sel);
            suiteEndPress();
        } else if (down) {
            sel = (sel + 1) % SUITE_COUNT;
            suiteDraw(sel);
            suiteEndPress();
        } else if (check(SelPress)) {
            if (millis() - opened < SELECT_GRACE_MS) {
                suiteEndPress();
                continue;
            }
            SelPress = false;
            if (SUITE_ENTRIES[sel].run == nullptr) break; // "Exit"
            SUITE_ENTRIES[sel].run();
            // Pages exit by setting returnToMenu; inside a suite that means
            // "leave the page", so clear it and show the suite again.
            returnToMenu = false;
            suiteDraw(sel);
            suiteEndPress();
        } else if (check(EscPress)) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    returnToMenu = true;
}
