#include "TempHumidityMenu.h"
#include "core/display.h"
#include "core/utils.h"

#include "modules/Temp_Humidity/temp_n_humidity.h"

void TempHumidityMenu::optionsMenu() {
    returnToMenu = false;
    options = {
        {"Data",      [=]() { DHTData3(); }},
        //{"Config",    [=]() { DHTConfig(); }},
        {"Main Menu", [=]() { return; }    },
    };
    // addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "DHT22");
};

void TempHumidityMenu::drawIconImg() {
    drawImg(
        *bruceConfig.themeFS(), bruceConfig.getThemeItemImg(bruceConfig.theme.paths.temp), 0, imgCenterY, true
    );
}

// chatgpt part (: xd
void TempHumidityMenu::drawIcon(int iconCenterX, int iconCenterY, int scale) {
    uint16_t fg = bruceConfig.priColor;

    // Sizes
    int bulbR = scale * 9;  // radius of the thermometer bulb
    int tubeW = scale * 3;  // width of the thermometer tube
    int tubeH = scale * 36; // height of the tube above bulb
    int tubeX = iconCenterX - tubeW / 2;
    int tubeY = iconCenterY - tubeH / 2;

    // Draw bulb (circle)
    tft.fillCircle(
        iconCenterX,
        tubeY + tubeH + bulbR - scale, // smooth join
        bulbR,
        fg
    );

    // Draw tube (rect)
    tft.fillRect(tubeX, tubeY, tubeW, tubeH, fg);

    // Draw droplet
    int dropW = scale * 12;
    int dropH = scale * 18;
    int dropX = iconCenterX + bulbR + scale * 4;
    int dropTopY = iconCenterY - dropH / 2;
    int dropBaseY = dropTopY + (dropH * 2 / 3);
    int dropR = dropW / 2;

    // Rounded bottom
    tft.fillCircle(dropX, dropBaseY, dropR, fg);
    // Pointy top
    tft.fillTriangle(dropX, dropTopY, dropX - dropR, dropBaseY, dropX + dropR, dropBaseY, fg);
}

void TempHumidityMenu::drawIcon(float scale) {
    clearIconArea();
    drawIcon(iconCenterX, iconCenterY, (int)scale);
}
