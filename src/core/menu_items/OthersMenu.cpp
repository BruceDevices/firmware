#include "OthersMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/badusb_ble/ducky_typer.h"
#include "modules/bjs_interpreter/interpreter.h"
#include "modules/others/clicker.h"
#include "modules/others/ibutton.h"
#include "modules/others/mic.h"
#include "modules/others/qrcode_menu.h"
#include "modules/others/timer.h"
#include "modules/others/tururururu.h"

void OthersMenu::optionsMenu() {
    options = {
        {"QRCodes",      qrcode_menu                              },
        {"Megalodon",    shark_setup                              },
#if defined(MIC_SPM1423) || defined(MIC_INMP441)
        {"Mic Spectrum", mic_test                                 },
        {"Mic Record",   mic_record                               }, //@deveclipse
#endif
#ifndef LITE_VERSION
        {"BadUSB",       [=]() { ducky_setup(hid_usb, false); }   },
        {"USB Keyboard", [=]() { ducky_keyboard(hid_usb, false); }},
#endif
#ifdef USB_as_HID
        {"Clicker",      clicker_setup                            },
#endif
#ifndef LITE_VERSION
        {"iButton",      setup_ibutton                            },
#endif
        {"Timer",        [=]() { Timer(); }                       },
    };
    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Others");
}

void OthersMenu::drawIcon(float scale) {
    clearIconArea();
    int radius = scale * 7;

    tft.fillCircle(iconCenterX, iconCenterY, radius, bruceConfig.priColor);

    tft.drawArc(
        iconCenterX, iconCenterY, 2.5 * radius, 2 * radius, 0, 340, bruceConfig.priColor, bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX, iconCenterY, 3.5 * radius, 3 * radius, 20, 360, bruceConfig.priColor, bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX, iconCenterY, 4.5 * radius, 4 * radius, 0, 200, bruceConfig.priColor, bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY,
        4.5 * radius,
        4 * radius,
        240,
        360,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
}
