#include "FMMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/fm/fm.h"

void FMMenu::optionsMenu() {
    // Define the menu options for the FM module
    options = {
        {"Tune Radio",      []() { ::fm_live_run(false); }},
        {"Mute/Unmute",
         []() {
             if (::is_running) {
                 ::fm_stop();
                 displayTextLine("Muted", true);
             } else {
                 ::fm_begin();
                 ::fm_tune(true);
                 displayTextLine("Unmuted");
             }
             delay(1000);
         }                                                },
        {"Scan Stations",
         []() {
             tft.fillScreen(bruceConfig.bgColor);
             displayTextLine("Scanning...");
             uint16_t found = ::fm_scan();
             char buf[30];
             sprintf(buf, "Best: %d.%02d MHz", found / 100, found % 100);
             displayTextLine(buf);
             delay(2000);
             ::fm_tune(false);
         }                                                },
        {"Signal Spectrum", []() { ::fm_spectrum(); }     },
        {"Traffic Alert",   []() { ::fm_ta_run(); }       },
    };
    addOptionToMainMenu();
    loopOptions(options, 0, "FM");
}

// Draw a simple icon for the FM module
void FMMenu::drawIcon(float scale) {
    clearIconArea();
    int iconW = scale * 80;
    int iconH = scale * 60;

    if (iconW % 2 != 0) iconW++;
    if (iconH % 2 != 0) iconH++;

    int caseH = 5 * iconH / 6;
    int caseX = iconCenterX - iconW / 2;
    int caseY = iconCenterY - iconH / 3;

    int btnY = (2 * caseY + caseH + iconH / 10 + iconH / 3) / 2;
    int potX = (2 * caseX + iconW + iconW / 10 + iconW / 2) / 2;
    int potY = caseY + caseH / 3 + iconH / 10;

    tft.drawRoundRect(caseX, caseY, iconW, caseH, iconW / 10, bruceConfig.priColor);
    tft.fillCircle(potX, potY, iconH / 7, bruceConfig.priColor);
    tft.drawRect(caseX + iconW / 10, caseY + iconH / 10, iconW / 2, iconH / 3, bruceConfig.priColor);
    tft.drawLine(
        caseX + iconW / 10, caseY, caseX + iconW / 10 + iconH / 3, caseY - iconH / 6, bruceConfig.priColor
    );
    tft.fillCircle(caseX + iconW / 10 + iconH / 3, caseY - iconH / 6, iconH / 30, bruceConfig.priColor);
    tft.fillCircle(caseX + iconW / 10 + iconH / 8, btnY, iconH / 12, bruceConfig.priColor);
    tft.fillCircle(caseX + iconW / 10 + iconW / 2 - iconH / 8, btnY, iconH / 12, bruceConfig.priColor);
}

// Draw an image-based icon (uses theme image data)
void FMMenu::drawIconImg() {
    drawImg(
        *bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.fm), 0, imgCenterY, true
    );
}
