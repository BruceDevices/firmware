#include "BleMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/badusb_ble/ducky_typer.h"
#include "modules/ble/ble_common.h"
#include "modules/ble/ble_ninebot.h"
#include "modules/ble/ble_spam.h"
#if !defined(LITE_VERSION)
#include "modules/ble/BLE_Suite.h"
#endif
#include <globals.h>

void BleMenu::optionsMenu() {
    options.clear();

    if (BLEConnected) {
        options.push_back({"Disconnect", [=]() {
#if defined(CONFIG_IDF_TARGET_ESP32C5)
            esp_bt_controller_deinit();
#else
            BLEDevice::deinit();
#endif
            BLEConnected = false;
            delete hid_ble;
            hid_ble = nullptr;
            if (_Ask_for_restart == 1)
                _Ask_for_restart = 2;
        }});
    }

    options.push_back({"Media Cmds", [=]() { MediaCommands(hid_ble, true); }});
#if !defined(LITE_VERSION)
    options.push_back({"BLE Scan", ble_scan});
    options.push_back({"iBeacon", [=]() {
                           ibeacon("Bruce", "e4c159a0-8c82-11e6-bdf4-0800200c9a66", 0x004C);
                       }});
    options.push_back({"Bad BLE", [=]() { ducky_setup(hid_ble, true); }});
#endif
    options.push_back({"BLE Keyboard", [=]() { ducky_keyboard(hid_ble, true); }});
    options.push_back({"BLE Spam", [=]() { spamMenu(); }});

#if !defined(LITE_VERSION)
    options.push_back({"BLE Suite", [=]() { BleSuiteMenu(); }});
#endif

#if !defined(LITE_VERSION)
    options.push_back({"Ninebot", [=]() { BLENinebot(); }});
#endif
    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Bluetooth", 0, false);
}

void BleMenu::drawIcon(float scale) {
    clearIconArea();

    int lineWidth = scale * 5;
    int iconW = scale * 36;
    int iconH = scale * 60;
    int radius = scale * 5;
    int deltaRadius = scale * 10;

    if (iconW % 2 != 0) iconW++;
    if (iconH % 4 != 0) iconH += 4 - (iconH % 4);

    tft.drawWideLine(
        iconCenterX,
        iconCenterY + iconH / 4,
        iconCenterX - iconW,
        iconCenterY - iconH / 4,
        lineWidth,
        bruceConfig.priColor,
        bruceConfig.priColor
    );
    tft.drawWideLine(
        iconCenterX,
        iconCenterY - iconH / 4,
        iconCenterX - iconW,
        iconCenterY + iconH / 4,
        lineWidth,
        bruceConfig.priColor,
        bruceConfig.priColor
    );
    tft.drawWideLine(
        iconCenterX,
        iconCenterY + iconH / 4,
        iconCenterX - iconW / 2,
        iconCenterY + iconH / 2,
        lineWidth,
        bruceConfig.priColor,
        bruceConfig.priColor
    );
    tft.drawWideLine(
        iconCenterX,
        iconCenterY - iconH / 4,
        iconCenterX - iconW / 2,
        iconCenterY - iconH / 2,
        lineWidth,
        bruceConfig.priColor,
        bruceConfig.priColor
    );

    tft.drawWideLine(
        iconCenterX - iconW / 2,
        iconCenterY - iconH / 2,
        iconCenterX - iconW / 2,
        iconCenterY + iconH / 2,
        lineWidth,
        bruceConfig.priColor,
        bruceConfig.priColor
    );

    tft.drawArc(
        iconCenterX,
        iconCenterY,
        2.5 * radius,
        2 * radius,
        210,
        330,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY,
        2.5 * radius + deltaRadius,
        2 * radius + deltaRadius,
        210,
        330,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY,
        2.5 * radius + 2 * deltaRadius,
        2 * radius + 2 * deltaRadius,
        210,
        330,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
}