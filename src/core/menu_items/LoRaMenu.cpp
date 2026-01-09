#if !defined(LITE_VERSION)
#include "LoRaMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/lora/LoRaRF.h"

void LoRaMenu::optionsMenu() {
    options = {
        {"Chat",             []() { lorachat(); }      },
        {"Change username",  []() { changeusername(); }},
        {"Change Frequency", []() { chfreq(); }        },
    };
    addOptionToMainMenu();
    String txt = "LoRa";
    loopOptions(options, MENU_TYPE_SUBMENU, txt.c_str());
}

void LoRaMenu::drawIcon(float scale) {
    clearIconArea();
    scale *= 0.75;
    int cx = iconCenterX;
    int cy = iconCenterY + (scale * 8);

#define CALC_X(val) (cx + ((val - 50) * scale))
#define CALC_Y(val) (cy + ((val - 50) * scale))

    int lineWidth = scale * 4.5;
    if (lineWidth < 2) lineWidth = 2;
    int ballRad = scale * 6;

    // --- LEGS UPDATED (Y changed from 95 to 85) ---

    // Left Leg
    tft.drawWideLine(
        CALC_X(44), CALC_Y(35), CALC_X(26), CALC_Y(85), lineWidth, bruceConfig.priColor, bruceConfig.priColor
    );
    // Right Leg
    tft.drawWideLine(
        CALC_X(56), CALC_Y(35), CALC_X(74), CALC_Y(85), lineWidth, bruceConfig.priColor, bruceConfig.priColor
    );

    // --- CROSS BARS (Unchanged) ---

    // Top Cross Bar
    tft.drawWideLine(
        CALC_X(44), CALC_Y(35), CALC_X(56), CALC_Y(35), lineWidth, bruceConfig.priColor, bruceConfig.priColor
    );

    // Middle Cross Bar
    tft.drawWideLine(
        CALC_X(35), CALC_Y(65), CALC_X(65), CALC_Y(65), lineWidth, bruceConfig.priColor, bruceConfig.priColor
    );

    // --- X-BRACING TOP (Unchanged) ---

    // X-Bracing (Top Section)
    tft.drawWideLine(
        CALC_X(44), CALC_Y(35), CALC_X(65), CALC_Y(65), lineWidth, bruceConfig.priColor, bruceConfig.priColor
    );
    tft.drawWideLine(
        CALC_X(56), CALC_Y(35), CALC_X(35), CALC_Y(65), lineWidth, bruceConfig.priColor, bruceConfig.priColor
    );

    // --- X-BRACING BOTTOM UPDATED (Y changed from 95 to 85) ---

    // X-Bracing (Bottom Section)
    tft.drawWideLine(
        CALC_X(35), CALC_Y(65), CALC_X(74), CALC_Y(85), lineWidth, bruceConfig.priColor, bruceConfig.priColor
    );
    tft.drawWideLine(
        CALC_X(65), CALC_Y(65), CALC_X(26), CALC_Y(85), lineWidth, bruceConfig.priColor, bruceConfig.priColor
    );

    // --- REST IS UNCHANGED ---

    // ball
    int ballY = CALC_Y(25);
    tft.fillCircle(cx, ballY, ballRad, bruceConfig.priColor);

    // waves
    int r1 = scale * 20;
    int r2 = scale * 32;

    // Right Side (90 degrees +/-)
    tft.drawArc(cx, ballY, r1 + lineWidth, r1, 60, 120, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawArc(cx, ballY, r2 + lineWidth, r2, 60, 120, bruceConfig.priColor, bruceConfig.bgColor);

    // Left Side (270 degrees +/-)
    tft.drawArc(cx, ballY, r1 + lineWidth, r1, 240, 300, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawArc(cx, ballY, r2 + lineWidth, r2, 240, 300, bruceConfig.priColor, bruceConfig.bgColor);
}
#endif
