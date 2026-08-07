#include "RFIRMenu.h"
#include "core/display.h"
#include "modules/ir/dual_detect.h"

const String RFIRMenu::_noTheme = "";

void RFIRMenu::optionsMenu() {
    dualDetect();
}

void RFIRMenu::drawIcon(float scale) {
    clearIconArea();
    int cx = iconCenterX;
    int cy = iconCenterY;
    int s = scale * 10;

    // Left: RF waves
    tft.drawArc(cx - 2 * s, cy, 2 * s, 1.5f * s, 200, 340, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawArc(cx - 2 * s, cy, 3 * s, 2.5f * s, 200, 340, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawArc(cx - 2 * s, cy, 4 * s, 3.5f * s, 200, 340, bruceConfig.priColor, bruceConfig.bgColor);
    tft.fillCircle(cx - 2 * s, cy, s / 3, bruceConfig.priColor);

    // Right: IR remote with emission beam
    int rx = cx + s;
    int ry = cy - 2 * s;
    tft.fillRoundRect(rx, ry, 3 * s, 4 * s, s / 2, bruceConfig.secColor);
    tft.drawLine(rx + 3 * s / 2, ry, rx + 5 * s / 2, ry - 2 * s, bruceConfig.secColor);
    tft.drawLine(rx + 3 * s / 2, ry, rx + s / 2, ry - 2 * s, bruceConfig.secColor);
    tft.fillCircle(rx + 3 * s / 2, ry - s, s / 4, bruceConfig.secColor);
}
