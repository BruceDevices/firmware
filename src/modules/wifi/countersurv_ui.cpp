#include "core/display.h"
#include "core/scrollableTextArea.h"
#include "countersurv.h"
#include <globals.h>

// Task 3, the panel half: the same two passes the console exposes, shown on the
// encoder display. Nothing is invented here - the page runs the capture and
// prints its report, so a finding on the screen is a finding `bluewatch` or
// `blecadence` would also print.
namespace CounterSurv {

static void showReport(const String &title, const String &text) {
    ScrollableTextArea area = ScrollableTextArea(title);
    area.fromString(text);
    area.show();
}

void dashboard() {
    int last = 0;
    while (true) {
        std::vector<Option> opts = {
            {"WiFi 15s + BLE 15s",
             []() {
                 String report = run(15, true);
                 report += "\n";
                 report += bleCadence(15);
                 showReport("Blue Team", report);
             }                         },
            {"WiFi 15s", []() { showReport("Blue Team", run(15, true)); }},
            {"BLE cadence 20s", []() { showReport("BLE Cadence", bleCadence(20)); }},
            {"Back", []() {}},
        };

        const int selected = loopOptions(opts, MENU_TYPE_SUBMENU, "Counter-Surveillance", last);
        if (selected == -1 || selected == (int)opts.size() - 1) return;
        last = selected;
    }
}

// One press: run the BLE pass and show it. Used by BLE > BLE Attacks.
//
// The old version called bleCadence() first and only drew when the 20 s scan
// finished, so the menu looked frozen and the press appeared to do nothing.
// Paint the scan state immediately; the report replaces it when the pass ends.
void blePage() {
    Serial.println("[BLEUI] cadence start 20s");
    drawMainBorderWithTitle("BLE Cadence");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString("Scanning advertisements...", 10, 44);
    tft.drawString("20 seconds, all BLE channels", 10, 62);
    tft.drawString("Please wait", 10, 80);
    vTaskDelay(pdMS_TO_TICKS(60));
    String report = bleCadence(20);
    // Print before the report takes over: the pass is over, the page stays
    // open until the user leaves it, so a host driving the menu cannot wait
    // for this line after the report is on screen.
    Serial.println("[BLEUI] cadence done");
    showReport("BLE Cadence", report);
}

} // namespace CounterSurv
