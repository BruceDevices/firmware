#include "EXTLedControlMenu.h"
#include "core/display.h"
#include "core/utils.h"

#include "modules/EXTled_control/ext_led_control.h"

void EXTLedControlMenu::optionsMenu() {
    returnToMenu = false;
    options = {
        {"Power",      [=]() { power(); }    },
        {"Fade",       [=]() { fade(); }     },
        {"Blink",      [=]() { blink(); }    },
        {"Fast Blink", [=]() { FastBlink(); }},
        {"Config",     [=]() { LedConfig(); }},
        {"Main Menu",  [=]() { return; }     },
    };
    // addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Led Control");
};

void EXTLedControlMenu::drawIconImg() {
    drawImg(
        *bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.led), 0, imgCenterY, true
    );
}

// chatgpt part (: xd
void EXTLedControlMenu::drawIcon(float scale) {
    clearIconArea();

    // Setup sizes
    int domeRadius = scale * 20;
    int bodyWidth = domeRadius * 4;
    int bodyHeight = scale * 12;
    int legLength = scale * 16;

    int x = iconCenterX;
    int y = iconCenterY;

    // --- Dome (semicircle top) using full circle + masking ---
    tft.fillCircle(x, y, domeRadius, bruceConfig.priColor);
    // Mask bottom half to simulate semicircle
    tft.fillRect(x - domeRadius, y, domeRadius * 2, domeRadius, bruceConfig.bgColor);

    // --- Rectangle body ---
    int rectTop = y;
    int rectLeft = x - bodyWidth / 2;
    tft.fillRect(rectLeft, rectTop, bodyWidth, bodyHeight, bruceConfig.priColor);

    // --- Legs ---
    int legY = rectTop + bodyHeight;
    int legOffsetX = scale * 4;

    tft.fillRect(x - legOffsetX, legY, scale * 2, legLength, bruceConfig.priColor);
    tft.fillRect(x + legOffsetX - scale * 2, legY, scale * 2, legLength, bruceConfig.priColor);

    // --- Light rays ---
    int rayLen = scale * 6;
    int rayGap = scale * 5;

    // Center ray
    tft.drawLine(x, y - domeRadius - rayLen, x, y - domeRadius - scale, bruceConfig.priColor);

    // Left ray
    tft.drawLine(
        x - rayGap, y - domeRadius - scale, x - rayGap + scale, y - domeRadius - rayLen, bruceConfig.priColor
    );

    // Right ray
    tft.drawLine(
        x + rayGap, y - domeRadius - scale, x + rayGap - scale, y - domeRadius - rayLen, bruceConfig.priColor
    );
}
