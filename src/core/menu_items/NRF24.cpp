#include "NRF24.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/NRF24/nrf_common.h"
#include "modules/NRF24/nrf_jammer.h"
#include "modules/NRF24/nrf_mousejack.h"
#include "modules/NRF24/nrf_spectrum.h"

void NRF24Menu::optionsMenu() {
    options.clear();
    options.push_back({"Information", nrf_info});
    options.push_back({"Spectrum", nrf_spectrum});
    #if !defined(LITE_VERSION)
    options.push_back({"MouseJack", nrf_mousejack});
    #endif
    options.push_back({"NRF Jammer", nrf_jammer});

    if (!bruceConfigPins.NRF24_presets.empty())
        options.push_back({"Config pins", [this]() { configMenu(); }});

    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "NRF24");
}

void NRF24Menu::configMenu() {
    int idx = 0;
    for (size_t i = 0; i < bruceConfigPins.NRF24_presets.size(); i++) {
        const BruceConfigPins::SPIPins &p = bruceConfigPins.NRF24_presets[i].pins;
        if (bruceConfigPins.NRF24_bus.sck == p.sck && bruceConfigPins.NRF24_bus.mosi == p.mosi &&
            bruceConfigPins.NRF24_bus.cs == p.cs) {
            idx = (int)i;
            break;
        }
    }

    options.clear();
    for (size_t i = 0; i < bruceConfigPins.NRF24_presets.size(); i++) {
        options.push_back(
            {bruceConfigPins.NRF24_presets[i].label,
             [this, i]() {
                 bruceConfigPins.setNrf24Pins(bruceConfigPins.NRF24_presets[i].pins);
                 bruceConfigPins.setCC1101Pins(bruceConfigPins.NRF24_presets[i].pins);
             }}
        );
    }
    options.push_back({"Back", [this]() { optionsMenu(); }});

    loopOptions(options, MENU_TYPE_SUBMENU, "RF Config", idx);
}

void NRF24Menu::drawIcon(float scale) {
    clearIconArea();
    int iconW = scale * 80;
    int iconH = scale * 60;

    if (iconW % 2 != 0) iconW++;
    if (iconH % 2 != 0) iconH++;

    int caseW = 3 * iconW / 4;
    int caseH = 2 * iconH / 3;
    int caseX = iconCenterX - iconW / 2;
    int caseY = iconCenterY - iconH / 6;

    int antW = iconW / 8;
    int connR = iconH / 20;

    // Case
    tft.drawRect(caseX, caseY, caseW, caseH, bruceConfig.priColor);

    // Antenna
    tft.fillRect(caseX + caseW, caseY + caseH / 2 - antW / 2, antW, antW, bruceConfig.priColor);
    tft.fillRoundRect(
        caseX + caseW + antW,
        caseY + caseH - iconH,
        antW,
        iconH - caseH / 2 + antW / 2,
        antW / 2,
        bruceConfig.priColor
    );

    // Connectors
    tft.fillCircle(caseX + caseW / 6, caseY + 1 * caseH / 5, connR, bruceConfig.priColor);
    tft.fillCircle(caseX + caseW / 6, caseY + 2 * caseH / 5, connR, bruceConfig.priColor);
    tft.fillCircle(caseX + caseW / 6, caseY + 3 * caseH / 5, connR, bruceConfig.priColor);
    tft.fillCircle(caseX + caseW / 6, caseY + 4 * caseH / 5, connR, bruceConfig.priColor);

    tft.fillCircle(caseX + caseW / 3, caseY + 1 * caseH / 5, connR, bruceConfig.priColor);
    tft.fillCircle(caseX + caseW / 3, caseY + 2 * caseH / 5, connR, bruceConfig.priColor);
    tft.fillCircle(caseX + caseW / 3, caseY + 3 * caseH / 5, connR, bruceConfig.priColor);
    tft.fillCircle(caseX + caseW / 3, caseY + 4 * caseH / 5, connR, bruceConfig.priColor);

    // Chip
    tft.fillRect(
        caseX + caseW - 2 * antW - connR, caseY + caseH / 2 - antW / 2, antW, antW, bruceConfig.priColor
    );
}
