#include "Bw16Menu.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"

BW16Tool Bw16Menu::tool;

void Bw16Menu::optionsMenu() {
    tool.setup();

    options = {
        {"Bad Ble",     [&]() { tool.attckbleMenu(); }},
        {"Scan WiFi",   [&]() { tool.scanWifi(); }    },
        {"Select WiFi", [&]() { tool.selectWifi(); }  },
        {"Attacks",     [&]() { attackMenu(); }       },
        {"Config",      [this]() { configMenu(); }    }
    };

    addOptionToMainMenu();
    String txt = "BW16 |TX:" + String(bruceConfigPins.gps_bus.tx) +
                 " |RX:" + String(bruceConfigPins.gps_bus.rx) + " | (" + String(bruceConfigPins.gpsBaudrate) +
                 " bps)";
    loopOptions(options, MENU_TYPE_SUBMENU, txt.c_str());
}

void Bw16Menu::attackMenu() {
    options = {
        {"Deauth Selected", [&]() { tool.attackSelected(); }},
        {"Deauth ALL",      [&]() { tool.attackAll(); }     },
        {"Beacon Spam",     [&]() { tool.beaconSpam(); }    },
        {"Beacon List",     [&]() { tool.beaconList(); }    },
        {"Beacon & Deauth", [&]() { tool.beaconDeauth(); }  },
        {"Evil Portal",     [&]() { tool.evilPortal(); }    },
        {"Back",            [this]() { optionsMenu(); }     }
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "BW16 Attacks");
}

void Bw16Menu::configMenu() {
    options = {
        {"Baudrate",  setGpsBaudrateMenu                                 },
        {"UART Pins", [=]() { setUARTPinsMenu(bruceConfigPins.gps_bus); }},
        {"Back",      [this]() { optionsMenu(); }                        }
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "BW16 Config");
}

void Bw16Menu::drawIcon(float scale) {
    clearIconArea();
    uint16_t color = bruceConfig.priColor;
    uint16_t bg = bruceConfig.bgColor;

    int minDim = min(tftWidth, tftHeight);

    int basePx;
    if (minDim <= 135) { 
        basePx = 4;
    } else if (minDim <= 240) { 
        basePx = 5;
    } else if (minDim <= 320) { 
        basePx = 6;
    } else {
        basePx = 7;
    }

    float adjustedScale = scale * (minDim / 240.0f);
    adjustedScale = constrain(adjustedScale, 0.8f, 1.2f);

    int px = max(2, (int)(basePx * adjustedScale));
    int totalW = 24 * px;
    int totalH = 20 * px;
    int x = iconCenterX - totalW / 2;
    int y = iconCenterY - totalH / 2;

    tft.fillRect(x + 2 * px, y, totalW - 4 * px, px, color);
    tft.fillRect(x + 2 * px, y + totalH - px, totalW - 4 * px, px, color);
    tft.fillRect(x, y + 2 * px, px, totalH - 4 * px, color);
    tft.fillRect(x + totalW - px, y + 2 * px, px, totalH - 4 * px, color);

    tft.fillRect(x + px, y + px, px, px, color);
    tft.fillRect(x + totalW - 2 * px, y + px, px, px, color);
    tft.fillRect(x + px, y + totalH - 2 * px, px, px, color);
    tft.fillRect(x + totalW - 2 * px, y + totalH - 2 * px, px, px, color);

    int wy = y + 3 * px;
    int wx = x + 2 * px;

    tft.fillRect(wx, wy, px, 5 * px, color);
    tft.fillRect(wx + 2 * px, wy + 2 * px, px, 3 * px, color);
    tft.fillRect(wx + 4 * px, wy + 2 * px, px, 3 * px, color);
    tft.fillRect(wx + 6 * px, wy, px, 5 * px, color);
    tft.fillRect(wx + 1 * px, wy + 5 * px, px, px, color);
    tft.fillRect(wx + 3 * px, wy + 4 * px, px, px, color);
    tft.fillRect(wx + 5 * px, wy + 5 * px, px, px, color);

    int ix = wx + 8 * px;
    tft.fillRect(ix, wy, 3 * px, px, color);
    tft.fillRect(ix + px, wy + px, px, 3 * px, color);
    tft.fillRect(ix, wy + 4 * px, 3 * px, px, color);

    int fx = ix + 4 * px;
    tft.fillRect(fx, wy, 4 * px, px, color);
    tft.fillRect(fx + px, wy + px, px, 4 * px, color);
    tft.fillRect(fx, wy + 4 * px, px, px, color);
    tft.fillRect(fx + 2 * px, wy + 2 * px, 2 * px, px, color);
    tft.drawPixel(fx + px, wy + 2 * px, bg);

    int ix2 = fx + 5 * px;
    tft.fillRect(ix2, wy, 3 * px, px, color);
    tft.fillRect(ix2 + px, wy + px, px, 3 * px, color);
    tft.fillRect(ix2, wy + 4 * px, 3 * px, px, color);

    int by = y + 11 * px;
    int x5 = x + 5 * px;

    tft.fillRect(x5, by, 7 * px, px, color);
    tft.fillRect(x5, by + px, 2 * px, 2 * px, color);
    tft.fillRect(x5, by + 3 * px, 6 * px, px, color);
    tft.fillRect(x5 + 5 * px, by + 4 * px, 2 * px, 2 * px, color);
    tft.fillRect(x5 + px, by + 6 * px, 5 * px, px, color);
    tft.fillRect(x5, by + 5 * px, px, px, color);
    tft.drawPixel(x5 + 6 * px, by + 3 * px, bg);
    tft.drawPixel(x5 + 6 * px, by + 6 * px, bg);

    int xG = x5 + 8 * px;
    tft.fillRect(xG + px, by, 5 * px, px, color);
    tft.fillRect(xG, by + px, 2 * px, 5 * px, color);
    tft.fillRect(xG + px, by + 6 * px, 5 * px, px, color);
    tft.fillRect(xG + 5 * px, by + 3 * px, 2 * px, 3 * px, color);
    tft.fillRect(xG + 3 * px, by + 3 * px, 2 * px, px, color);
    tft.fillRect(xG + 6 * px, by + px, px, px, color);
    tft.drawPixel(xG, by, bg);
    tft.drawPixel(xG, by + 6 * px, bg);
    tft.drawPixel(xG + 6 * px, by + 6 * px, bg);
}
