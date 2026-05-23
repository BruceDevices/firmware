#include "SubGHZMenu.h"

#include "core/display.h"
#include "modules/subghz_advanced/subghz_advanced_menu.h"

void SubGHZMenu::optionsMenu() { subghz_advanced_menu(); }

void SubGHZMenu::drawIcon(float scale) {
    clearIconArea();

    int cx = iconCenterX;
    int cy = iconCenterY;

    int base = scale * 6;
    int step = scale * 8;

    // Antenna mast
    tft.fillRect(cx - base / 6, cy + base / 3, base / 3, base * 2, bruceConfig.priColor);

    // Beacon head
    tft.fillCircle(cx, cy, base / 2, bruceConfig.priColor);

    // Left arcs
    tft.drawArc(cx, cy, base + step, base + step - 2, 40, 140, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawArc(
        cx,
        cy,
        base + step * 2,
        base + step * 2 - 2,
        40,
        140,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );

    // Right arcs
    tft.drawArc(cx, cy, base + step, base + step - 2, 220, 320, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawArc(
        cx,
        cy,
        base + step * 2,
        base + step * 2 - 2,
        220,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
}
