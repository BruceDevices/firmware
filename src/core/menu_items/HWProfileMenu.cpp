#ifdef TCA8418_I2C_ADDR

#include "HWProfileMenu.h"
#include "hw_profiles.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"

static void setProfileAndApply(uint8_t prof) {
    saveHWProfile(prof);
    applyHWProfile(prof);
    bruceConfigPins.saveFile(); // persist new pin assignments (GPS, CC1101, LoRa, etc.)
    displaySuccess(String("Profile: ") + hwProfileName(prof));
}

void HWProfileMenu::optionsMenu() {
    String title = String("Profile: ") + hwProfileName(loadHWProfile());
    options = {
        {"Cap CC1101",    [=]() { setProfileAndApply(HW_CAP_CC1101);  }},
        {"Cap LoRa-1262", [=]() { setProfileAndApply(HW_CAP_LORA);    }},
        {"Stock/Shield",  [=]() { setProfileAndApply(HW_STOCK);       }},
        {"Grove GPS v1.1",[=]() { setProfileAndApply(HW_GROVE_GPS);   }},
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, title.c_str());
}

/*
** Icon: a chip — square body with pins on left/right sides.
** Represents hardware configuration in a recognisable way.
*/
void HWProfileMenu::drawIcon(float scale) {
    clearIconArea();

    int bodyW = (int)(scale * 36);
    int bodyH = (int)(scale * 30);
    if (bodyW % 2 != 0) bodyW++;
    if (bodyH % 2 != 0) bodyH++;

    int bx = iconCenterX - bodyW / 2;
    int by = iconCenterY - bodyH / 2;

    int pinW = (int)(scale * 8);
    int pinH = (int)(scale * 4);
    int pinSpacing = (int)(scale * 8);
    int nPins = 3;

    // Pins on the left
    int pinStartY = by + (bodyH - (nPins * pinH + (nPins - 1) * (pinSpacing - pinH))) / 2;
    for (int i = 0; i < nPins; i++) {
        int py = pinStartY + i * pinSpacing;
        tft.fillRect(bx - pinW, py, pinW, pinH, bruceConfig.priColor);
    }
    // Pins on the right
    for (int i = 0; i < nPins; i++) {
        int py = pinStartY + i * pinSpacing;
        tft.fillRect(bx + bodyW, py, pinW, pinH, bruceConfig.priColor);
    }

    // Chip body (filled, then border)
    tft.fillRect(bx, by, bodyW, bodyH, bruceConfig.priColor);
    tft.fillRect(bx + 2, by + 2, bodyW - 4, bodyH - 4, bruceConfig.bgColor);

    // Circle in the centre (pin-1 indicator style)
    int notchR = (int)(scale * 3);
    tft.fillCircle(iconCenterX, iconCenterY, notchR, bruceConfig.priColor);
}

#endif // TCA8418_I2C_ADDR
