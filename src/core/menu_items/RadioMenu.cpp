#include "RadioMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/radio/radio.h"

void RadioMenu::optionsMenu() {
    options = {
        {"Online Radio", radioMainMenu},
        {"Air Radio (WIP)", radioAirMock},
    };
    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Radio");
}

void RadioMenu::drawIconImg() {
    drawImg(
        *bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.radio), 0, imgCenterY, true
    );
}

void RadioMenu::drawIcon(float scale) {
    clearIconArea();
    int bodyW = scale * 70;
    int bodyH = scale * 40;
    int x = iconCenterX - bodyW / 2;
    int y = iconCenterY - bodyH / 2;

    // Calculate antenna height to avoid status bar
    // Ensure antenna doesn't go above iconAreaY (top of icon area)
    int antennaMaxY = iconAreaY + 5; // Small margin from top
    int antennaHeight = y - antennaMaxY;
    if (antennaHeight < 0) antennaHeight = 0;
    if (antennaHeight > bodyH / 2) antennaHeight = bodyH / 2; // Limit to original design

    // Radio body
    tft.drawRoundRect(x, y, bodyW, bodyH, bodyH / 6, bruceConfig.priColor);
    tft.drawLine(x, y + bodyH / 2, x + bodyW, y + bodyH / 2, bruceConfig.priColor);

    // Dial
    tft.drawCircle(x + bodyW / 4, y + bodyH / 2 - bodyH / 6, bodyH / 6, bruceConfig.priColor);
    tft.fillCircle(x + (3 * bodyW) / 4, y + (3 * bodyH) / 4, bodyH / 8, bruceConfig.priColor);

    // Antenna - adjusted to not exceed iconAreaY
    int antennaX = x + bodyW / 2;
    int antennaTopY = y - antennaHeight;
    if (antennaHeight > 0) {
        tft.drawLine(antennaX, y, antennaX, antennaTopY, bruceConfig.priColor);
        if (antennaHeight > bodyH / 4) {
            // Draw antenna tip only if there's enough space
            tft.drawLine(antennaX, antennaTopY, antennaX - bodyW / 6, antennaTopY - bodyH / 4, bruceConfig.priColor);
            tft.drawLine(antennaX, antennaTopY, antennaX + bodyW / 6, antennaTopY - bodyH / 4, bruceConfig.priColor);
        }
    }

    // Waves - only draw if there's space above antenna
    if (antennaTopY - bodyH / 4 > iconAreaY) {
        int waveY = antennaTopY - bodyH / 4;
        tft.drawArc(antennaX, waveY, bodyW / 3, bodyW / 3 + scale * 4, 220, 320, bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawArc(antennaX, waveY, bodyW / 4, bodyW / 4 + scale * 3, 220, 320, bruceConfig.priColor, bruceConfig.bgColor);
    }
}
