#include "NRF24.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/NRF24/nrf_common.h"
#include "modules/NRF24/nrf_jammer.h"
#include "modules/NRF24/nrf_spectrum.h"

void NRF24Menu::optionsMenu() {
    options.clear();
    options.push_back({"Information", nrf_info});

    if (bruceConfigPins.NRF24_bus.mosi == bruceConfigPins.SDCARD_bus.mosi &&
        bruceConfigPins.NRF24_bus.mosi != GPIO_NUM_NC)
        options.push_back({"Spectrum", [=]() { nrf_spectrum(&sdcardSPI); }});
#if TFT_MOSI > 0 // Display doesn't use SPI bus
    else if (bruceConfigPins.NRF24_bus.mosi == (gpio_num_t)TFT_MOSI)
        options.push_back({"Spectrum", [=]() { nrf_spectrum(&tft.getSPIinstance()); }});
#endif
    else options.push_back({"Spectrum", [=]() { nrf_spectrum(&SPI); }});

    options.push_back({"NRF Jammer", nrf_jammer});

    options.push_back({"CH Jammer", nrf_channel_jammer});
    options.push_back({"CH hopper", nrf_channel_hopper});

#if defined(ARDUINO_M5STICK_C_PLUS) || defined(ARDUINO_M5STICK_C_PLUS2)
    options.push_back({"Config pins", [=]() { configMenu(); }});
#endif

    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "NRF24");
}

void NRF24Menu::configMenu() {
    uint8_t opt = 0;
    options = {
        {"NRF24 (legacy)",     [&]() { opt = 1; }         },
        {"NRF24 (shared SPI)", [&]() { opt = 2; }         },
        {"Back",               [this]() { optionsMenu(); }},
    };

    loopOptions(options, MENU_TYPE_SUBMENU, "RF Config");
    if (opt == 1) {
        bruceConfigPins.setNrf24Pins(
            {(gpio_num_t)NRF24_SCK_PIN,
             (gpio_num_t)NRF24_MISO_PIN,
             (gpio_num_t)NRF24_MOSI_PIN,
             (gpio_num_t)NRF24_SS_PIN,
             (gpio_num_t)NRF24_CE_PIN,
             GPIO_NUM_NC}
        );
    }
#if CONFIG_SOC_GPIO_OUT_RANGE_MAX > 30
    if (opt == 2) {
        bruceConfigPins.setNrf24Pins(
            {(gpio_num_t)SDCARD_SCK,
             (gpio_num_t)SDCARD_MISO,
             (gpio_num_t)SDCARD_MOSI,
             GPIO_NUM_33,
             GPIO_NUM_32,
             GPIO_NUM_NC}
        );
    }
#endif
}
void NRF24Menu::drawIconImg() {
    drawImg(
        *bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.nrf), 0, imgCenterY, true
    );
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
