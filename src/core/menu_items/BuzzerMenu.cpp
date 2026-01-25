#include "BuzzerMenu.h"
#include "core/display.h"
#include "core/utils.h"

#include "modules/Frequency_Gen/frequency.h"

void BuzzerMenu::optionsMenu() {
    returnToMenu = false;
    options = {
        //  {"100%",      [=]() { Start11600_100(); }},
        // {"100%",      [=]() { Start100(); }    },
        // {"50%",       [=]() { Start50(); }     },
        // {"20%",       [=]() { Start20(); }     },
        // {"10%",       [=]() { Start10(); }     },
        {"Freq Gen",  [=]() { StartFreq(); }   },
        {"Config",    [=]() { BuzzerConfig(); }},
        {"Volume",    [=]() { BuzzerVolume(); }},
        {"Erika",     [=]() { PlayErika(); }   },
        {"Nokia",     [=]() { PlayNokia(); }   },
        //{"Rickroll",  [=]() { playNeverGonnaGiveYouUp(); }},
        {"Main Menu", [=]() { return; }        },
    };
    // addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Buzzer");
};

void BuzzerMenu::drawIconImg() {
    drawImg(
        *bruceConfig.themeFS(),
        bruceConfig.getThemeItemImg(bruceConfig.theme.paths.buzzer),
        0,
        imgCenterY,
        true
    );
}

// chatgpt part (: xd
void BuzzerMenu::drawIcon(float scale) {
    clearIconArea();

    // Setup sizes
    int buzzerRadius = scale * 20;
    int innerRadius = buzzerRadius * 0.4; // inner circle for central hole
    int x = iconCenterX;
    int y = iconCenterY;

    // --- Outer body (main disc) ---
    tft.fillCircle(x, y, buzzerRadius, bruceConfig.priColor);

    // --- Inner hole (like on real piezo buzzers) ---
    tft.fillCircle(x, y, innerRadius, bruceConfig.bgColor);

    // --- Sound waves (arcs or lines) ---
    int waveLength = scale * 12;
    int waveGap = scale * 6;

    // Left wave lines
    tft.drawLine(
        x - buzzerRadius - waveGap,
        y - waveLength / 2,
        x - buzzerRadius - waveGap,
        y + waveLength / 2,
        bruceConfig.priColor
    );
    tft.drawLine(
        x - buzzerRadius - waveGap * 2,
        y - waveLength / 3,
        x - buzzerRadius - waveGap * 2,
        y + waveLength / 3,
        bruceConfig.priColor
    );

    // Right wave lines
    tft.drawLine(
        x + buzzerRadius + waveGap,
        y - waveLength / 2,
        x + buzzerRadius + waveGap,
        y + waveLength / 2,
        bruceConfig.priColor
    );
    tft.drawLine(
        x + buzzerRadius + waveGap * 2,
        y - waveLength / 3,
        x + buzzerRadius + waveGap * 2,
        y + waveLength / 3,
        bruceConfig.priColor
    );
}
