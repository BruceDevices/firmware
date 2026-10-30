#if !defined(LITE_VERSION)
#include "WireToolsMenu.h"

#include "core/display.h"
#include "core/utils.h"
#include "modules/wire/wire_tools.h"

void WireToolsMenu::optionsMenu() {
    options = {
        {"UART Terminal", []() { uart_terminal_setup(); }},
        {"I2C Bus",       []() { i2c_bus_setup(); }      },
        {"SPI Bus",       []() { spi_bus_setup(); }      },
        {"JTAG Scanner",  []() { jtag_scan_setup(); }    },
    };
    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Wire Tools");
}

void WireToolsMenu::drawIcon(float scale) {
    clearIconArea();
    int cx = iconCenterX;
    int cy = iconCenterY;

    int w = scale * 44;
    if (w < 14) w = 14;
    int h = scale * 26;
    if (h < 10) h = 10;
    int pinLen = scale * 6;
    if (pinLen < 2) pinLen = 2;
    int lineWidth = scale * 2;
    if (lineWidth < 2) lineWidth = 2;

    // Legs on both sides of the chip
    for (int i = -1; i <= 1; i++) {
        int y = cy + i * h / 4;
        tft.drawWideLine(cx - w / 2 - pinLen, y, cx - w / 2, y, lineWidth, bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawWideLine(cx + w / 2, y, cx + w / 2 + pinLen, y, lineWidth, bruceConfig.priColor, bruceConfig.bgColor);
    }

    // Chip body with a dark screen cutout
    tft.fillRoundRect(cx - w / 2, cy - h / 2, w, h, 2 + scale * 2, bruceConfig.priColor);
    tft.fillRoundRect(
        cx - w / 2 + scale * 6, cy - h / 2 + scale * 5, w - scale * 12, h - scale * 10, 1 + scale, bruceConfig.bgColor
    );

    // ">_" prompt drawn geometrically so it scales to any icon box
    int promptX = cx - scale * 5;
    int s = scale * 3;
    if (s < 2) s = 2;
    tft.drawWideLine(promptX - s, cy - s, promptX, cy, lineWidth, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawWideLine(promptX, cy, promptX - s, cy + s, lineWidth, bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawWideLine(
        promptX + s, cy + s + 1, promptX + s * 3, cy + s + 1, lineWidth, bruceConfig.priColor, bruceConfig.bgColor
    );
}
#endif
