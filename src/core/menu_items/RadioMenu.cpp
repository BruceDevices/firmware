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

    // Radio body
    tft.drawRoundRect(x, y, bodyW, bodyH, bodyH / 6, bruceConfig.priColor);
    tft.drawLine(x, y + bodyH / 2, x + bodyW, y + bodyH / 2, bruceConfig.priColor);

    // Dial
    tft.drawCircle(x + bodyW / 4, y + bodyH / 2 - bodyH / 6, bodyH / 6, bruceConfig.priColor);
    tft.fillCircle(x + (3 * bodyW) / 4, y + (3 * bodyH) / 4, bodyH / 8, bruceConfig.priColor);

    // Antenna
    int antennaX = x + bodyW / 2;
    tft.drawLine(antennaX, y, antennaX, y - bodyH / 2, bruceConfig.priColor);
    tft.drawLine(antennaX, y - bodyH / 2, antennaX - bodyW / 6, y - bodyH, bruceConfig.priColor);
    tft.drawLine(antennaX, y - bodyH / 2, antennaX + bodyW / 6, y - bodyH, bruceConfig.priColor);

    // Waves
    tft.drawArc(antennaX, y - bodyH, bodyW / 3, bodyW / 3 + scale * 4, 220, 320, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawArc(antennaX, y - bodyH, bodyW / 4, bodyW / 4 + scale * 3, 220, 320, bruceConfig.priColor, bruceConfig.bgColor);
}
