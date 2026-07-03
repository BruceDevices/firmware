#include "settings.h"
#include "core/led_control.h"
#include "core/wifi/wifi_common.h"
#include "current_year.h"
#include "display.h"
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "modules/bjs_interpreter/interpreter.h"
#endif
#include "modules/ble_api/ble_api.hpp"
#include "modules/others/qrcode_menu.h"
#include "modules/rf/rf_utils.h" // for initRfModule
#include "mykeyboard.h"
#include "powerSave.h"
#include "sd_functions.h"
#include "settingsColor.h"
#include "utils.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <globals.h>

int currentScreenBrightness = -1;

// This function comes from interface.h
void _setBrightness(uint8_t brightval) {}

/*********************************************************************
**  Function: setBrightness
**  set brightness value
**********************************************************************/
void setBrightness(uint8_t brightval, bool save) {
    if (bruceConfig.bright > 100) bruceConfig.setBright(100);
    _setBrightness(brightval);
    delay(10);

    currentScreenBrightness = brightval;
    if (save) { bruceConfig.setBright(brightval); }
}

/*********************************************************************
**  Function: getBrightness
**  get brightness value
**********************************************************************/
void getBrightness() {
    if (bruceConfig.bright > 100) {
        bruceConfig.setBright(100);
        _setBrightness(bruceConfig.bright);
        delay(10);
        setBrightness(100);
    }

    _setBrightness(bruceConfig.bright);
    delay(10);

    currentScreenBrightness = bruceConfig.bright;
}

/*********************************************************************
**  Function: gsetRotation
**  get/set rotation value
**********************************************************************/
int gsetRotation(bool set) {
    int getRot = bruceConfigPins.rotation;
    int result = ROTATION;
    int mask = ROTATION > 1 ? -2 : 2;

    options = {
        {"Default",         [&]() { result = ROTATION; }                        },
        {"Landscape (180)", [&]() { result = ROTATION + mask; }                 },
#if TFT_WIDTH >= 170 && TFT_HEIGHT >= 240
        {"Portrait (+90)",  [&]() { result = ROTATION > 0 ? ROTATION - 1 : 3; } },
        {"Portrait (-90)",  [&]() { result = ROTATION == 3 ? 0 : ROTATION + 1; }},

#endif
    };
    addOptionToMainMenu();
    if (set) loopOptions(options);
    else result = getRot;

    if (result > 3 || result < 0) {
        result = ROTATION;
        set = true;
    }
    if (set) {
        bruceConfigPins.setRotation(result);
        tft.setRotation(result);
        tft.setRotation(result); // must repeat, sometimes ESP32S3 miss one SPI command and it just
                                 // jumps this step and don't rotate
    }
    returnToMenu = true;

    if (result & 0b01) { // if 1 or 3
        tftWidth = TFT_HEIGHT;
#if defined(HAS_TOUCH)
        tftHeight = TFT_WIDTH - 20;
#else
        tftHeight = TFT_WIDTH;
#endif
    } else { // if 2 or 0
        tftWidth = TFT_WIDTH;
#if defined(HAS_TOUCH)
        tftHeight = TFT_HEIGHT - 20;
#else
        tftHeight = TFT_HEIGHT;
#endif
    }
    return result;
}

/*********************************************************************
**  Function: setBrightnessMenu
**  Handles Menu to set brightness
**********************************************************************/
void setBrightnessMenu() {
    int idx = 0;
    if (bruceConfig.bright == 100) idx = 0;
    else if (bruceConfig.bright == 75) idx = 1;
    else if (bruceConfig.bright == 50) idx = 2;
    else if (bruceConfig.bright == 25) idx = 3;
    else if (bruceConfig.bright == 1) idx = 4;

    options = {
        {"100%",
         [=]() { setBrightness((uint8_t)100); },
         bruceConfig.bright == 100,
         [](void *pointer, bool shouldRender) {
             setBrightness((uint8_t)100, false);
             return false;
         }},
        {"75 %",
         [=]() { setBrightness((uint8_t)75); },
         bruceConfig.bright == 75,
         [](void *pointer, bool shouldRender) {
             setBrightness((uint8_t)75, false);
             return false;
         }},
        {"50 %",
         [=]() { setBrightness((uint8_t)50); },
         bruceConfig.bright == 50,
         [](void *pointer, bool shouldRender) {
             setBrightness((uint8_t)50, false);
             return false;
         }},
        {"25 %",
         [=]() { setBrightness((uint8_t)25); },
         bruceConfig.bright == 25,
         [](void *pointer, bool shouldRender) {
             setBrightness((uint8_t)25, false);
             return false;
         }},
        {" 1 %",
         [=]() { setBrightness((uint8_t)1); },
         bruceConfig.bright == 1,
         [](void *pointer, bool shouldRender) {
             setBrightness((uint8_t)1, false);
             return false;
         }}
    };
    addOptionToMainMenu(); // this one bugs the brightness selection
    loopOptions(options, MENU_TYPE_REGULAR, "", idx);
    setBrightness(bruceConfig.bright, false);
}

/*********************************************************************
**  Function: setSleepMode
**  Turn screen off and reduces cpu clock
**********************************************************************/
void setSleepMode() {
    sleepModeOn();
    while (1) {
        if (check(AnyKeyPress)) {
            sleepModeOff();
            returnToMenu = true;
            break;
        }
    }
}

/*********************************************************************
**  Function: setDimmerTimeMenu
**  Handles Menu to set dimmer time
**********************************************************************/
void setDimmerTimeMenu() {
    int idx = 0;
    if (bruceConfig.dimmerSet == 10) idx = 0;
    else if (bruceConfig.dimmerSet == 20) idx = 1;
    else if (bruceConfig.dimmerSet == 30) idx = 2;
    else if (bruceConfig.dimmerSet == 60) idx = 3;
    else if (bruceConfig.dimmerSet == 0) idx = 4;
    options = {
        {"10s",      [=]() { bruceConfig.setDimmer(10); }, bruceConfig.dimmerSet == 10},
        {"20s",      [=]() { bruceConfig.setDimmer(20); }, bruceConfig.dimmerSet == 20},
        {"30s",      [=]() { bruceConfig.setDimmer(30); }, bruceConfig.dimmerSet == 30},
        {"60s",      [=]() { bruceConfig.setDimmer(60); }, bruceConfig.dimmerSet == 60},
        {"Disabled", [=]() { bruceConfig.setDimmer(0); },  bruceConfig.dimmerSet == 0 },
    };
    loopOptions(options, idx);
}

/*********************************************************************
**  Function: setUIColor
**  Set and store main UI color
**********************************************************************/
void setUIColor() {

    while (1) {
        options.clear();
        int idx = UI_COLOR_COUNT;
        int i = 0;
        for (const auto &mapping : UI_COLORS) {
            if (bruceConfig.priColor == mapping.priColor && bruceConfig.secColor == mapping.secColor &&
                bruceConfig.bgColor == mapping.bgColor) {
                idx = i;
            }

            options.emplace_back(
                mapping.name,
                [=, &mapping]() {
                    uint16_t secColor = mapping.secColor;
                    uint16_t bgColor = mapping.bgColor;
                    bruceConfig.setUiColor(mapping.priColor, &secColor, &bgColor);
                },
                idx == i
            );
            ++i;
        }

        options.push_back(
            {"Custom Color",
             [=]() {
                 uint16_t oldPriColor = bruceConfig.priColor;
                 uint16_t oldSecColor = bruceConfig.secColor;
                 uint16_t oldBgColor = bruceConfig.bgColor;

                 if (setCustomUIColorMenu()) {
                     bruceConfig.setUiColor(
                         bruceConfig.priColor, &bruceConfig.secColor, &bruceConfig.bgColor
                     );
                 } else {
                     bruceConfig.priColor = oldPriColor;
                     bruceConfig.secColor = oldSecColor;
                     bruceConfig.bgColor = oldBgColor;
                 }
                 tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
             },
             idx == UI_COLOR_COUNT}
        );

        options.push_back(
            {"Invert Color",
             [=]() {
                 bruceConfig.setColorInverted(!bruceConfig.colorInverted);
                 tft.invertDisplay(bruceConfig.colorInverted);
             },
             bruceConfig.colorInverted > 0}
        );

        addOptionToMainMenu();

        int selectedOption = loopOptions(options, idx);
        if (selectedOption == -1 || selectedOption == options.size() - 1) return;
    }
}

uint16_t alterOneColorChannel565(uint16_t color, int newR, int newG, int newB) {
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;

    if (newR != 256) r = newR & 0x1F;
    if (newG != 256) g = newG & 0x3F;
    if (newB != 256) b = newB & 0x1F;

    return (r << 11) | (g << 5) | b;
}

bool setCustomUIColorMenu() {
    while (1) {
        options = {
            {"Primary",    [=]() { setCustomUIColorChoiceMenu(1); }},
            {"Secondary",  [=]() { setCustomUIColorChoiceMenu(2); }},
            {"Background", [=]() { setCustomUIColorChoiceMenu(3); }},
            {"Save",       [=]() {}                                },
            {"Cancel",     [=]() {}                                }
        };

        int selectedOption = loopOptions(options);
        if (selectedOption == -1 || selectedOption == options.size() - 1) {
            return false;
        } else if (selectedOption == 3) {
            return true;
        }
    }
}

void setCustomUIColorChoiceMenu(int colorType) {
    while (1) {
        options = {
            {"Red Channel",   [=]() { setCustomUIColorSettingMenuR(colorType); }},
            {"Green Channel", [=]() { setCustomUIColorSettingMenuG(colorType); }},
            {"Blue Channel",  [=]() { setCustomUIColorSettingMenuB(colorType); }},
            {"Back",          [=]() {}                                          }
        };

        int selectedOption = loopOptions(options);
        if (selectedOption == -1 || selectedOption == options.size() - 1) return;
    }
}

void setCustomUIColorSettingMenuR(int colorType) {
    setCustomUIColorSettingMenu(colorType, 1, [](uint16_t baseColor, int i) {
        return alterOneColorChannel565(baseColor, i, 256, 256);
    });
}

void setCustomUIColorSettingMenuG(int colorType) {
    setCustomUIColorSettingMenu(colorType, 2, [](uint16_t baseColor, int i) {
        return alterOneColorChannel565(baseColor, 256, i, 256);
    });
}

void setCustomUIColorSettingMenuB(int colorType) {
    setCustomUIColorSettingMenu(colorType, 3, [](uint16_t baseColor, int i) {
        return alterOneColorChannel565(baseColor, 256, 256, i);
    });
}

constexpr const char *colorTypes[] = {
    "Background", // 0
    "Primary",    // 1
    "Secondary"   // 2
};

constexpr const char *rgbNames[] = {
    "Blue", // 0
    "Red",  // 1
    "Green" // 2
};

void setCustomUIColorSettingMenu(
    int colorType, int rgb, std::function<uint16_t(uint16_t, int)> colorGenerator
) {
    uint16_t color = (colorType == 1)   ? bruceConfig.priColor
                     : (colorType == 2) ? bruceConfig.secColor
                                        : bruceConfig.bgColor;

    options.clear();

    static auto hoverFunctionPriColor = [](void *pointer, bool shouldRender) -> bool {
        uint16_t colorToSet = *static_cast<uint16_t *>(pointer);
        // Serial.printf("Setting primary color to: %04X\n", colorToSet);
        bruceConfig.priColor = colorToSet;
        return false;
    };
    static auto hoverFunctionSecColor = [](void *pointer, bool shouldRender) -> bool {
        uint16_t colorToSet = *static_cast<uint16_t *>(pointer);
        // Serial.printf("Setting secondary color to: %04X\n", colorToSet);
        bruceConfig.secColor = colorToSet;
        return false;
    };

    static auto hoverFunctionBgColor = [](void *pointer, bool shouldRender) -> bool {
        uint16_t colorToSet = *static_cast<uint16_t *>(pointer);
        // Serial.printf("Setting bg color to: %04X\n", colorToSet);
        bruceConfig.bgColor = colorToSet;
        tft.fillScreen(bruceConfig.bgColor);
        return false;
    };

    static uint16_t colorStorage[32];
    int selectedIndex = 0;
    int i = 0;
    int index = 0;

    if (rgb == 1) {
        selectedIndex = (color >> 11) & 0x1F;
    } else if (rgb == 2) {
        selectedIndex = ((color >> 5) & 0x3F);
    } else {
        selectedIndex = color & 0x1F;
    }

    while (i <= (rgb == 2 ? 63 : 31)) {
        if (i == 0 || (rgb == 2 && (i + 1) % 2 == 0) || (rgb != 2)) {
            uint16_t updatedColor = colorGenerator(color, i);
            colorStorage[index] = updatedColor;

            options.emplace_back(
                String(i),
                [colorType, updatedColor]() {
                    if (colorType == 1) bruceConfig.priColor = updatedColor;
                    else if (colorType == 2) bruceConfig.secColor = updatedColor;
                    else bruceConfig.bgColor = updatedColor;
                },
                selectedIndex == i,
                (colorType == 1 ? hoverFunctionPriColor
                                : (colorType == 2 ? hoverFunctionSecColor : hoverFunctionBgColor)),
                &colorStorage[index]
            );
            ++index;
        }
        ++i;
    }

    addOptionToMainMenu();

    int selectedOption = loopOptions(
        options,
        MENU_TYPE_SUBMENU,
        (String(colorType == 1 ? "Primary" : (colorType == 2 ? "Secondary" : "Background")) + " - " +
         (rgb == 1 ? "Red" : (rgb == 2 ? "Green" : "Blue")))
            .c_str(),
        (rgb != 2) ? selectedIndex : (selectedIndex > 0 ? (selectedIndex + 1) / 2 : 0)
    );
    if (selectedOption == -1 || selectedOption == options.size() - 1) {
        if (colorType == 1) {
            bruceConfig.priColor = color;
        } else if (colorType == 2) {
            bruceConfig.secColor = color;
        } else {
            bruceConfig.bgColor = color;
        }
        return;
    }
}

/*********************************************************************
**  Function: setSoundConfig - 01/2026 - Refactored "ConfigMenu" (this function manteined for
* retrocompatibility)
**  Enable or disable sound
**********************************************************************/
void setSoundConfig() {
    options = {
        {"Sound off", [=]() { bruceConfig.setSoundEnabled(0); }, bruceConfig.soundEnabled == 0},
        {"Sound on",  [=]() { bruceConfig.setSoundEnabled(1); }, bruceConfig.soundEnabled == 1},
    };
    loopOptions(options, bruceConfig.soundEnabled);
}

/*********************************************************************
**  Function: setSoundVolume
**  Set sound volume
**********************************************************************/
void setSoundVolume() {
    options = {
        {"10%",  [=]() { bruceConfig.setSoundVolume(10); },  bruceConfig.soundVolume == 10 },
        {"20%",  [=]() { bruceConfig.setSoundVolume(20); },  bruceConfig.soundVolume == 20 },
        {"30%",  [=]() { bruceConfig.setSoundVolume(30); },  bruceConfig.soundVolume == 30 },
        {"40%",  [=]() { bruceConfig.setSoundVolume(40); },  bruceConfig.soundVolume == 40 },
        {"50%",  [=]() { bruceConfig.setSoundVolume(50); },  bruceConfig.soundVolume == 50 },
        {"60%",  [=]() { bruceConfig.setSoundVolume(60); },  bruceConfig.soundVolume == 60 },
        {"70%",  [=]() { bruceConfig.setSoundVolume(70); },  bruceConfig.soundVolume == 70 },
        {"80%",  [=]() { bruceConfig.setSoundVolume(80); },  bruceConfig.soundVolume == 80 },
        {"90%",  [=]() { bruceConfig.setSoundVolume(90); },  bruceConfig.soundVolume == 90 },
        {"100%", [=]() { bruceConfig.setSoundVolume(100); }, bruceConfig.soundVolume == 100},
    };
    loopOptions(options, bruceConfig.soundVolume);
}

#ifdef HAS_RGB_LED
/*********************************************************************
**  Function: setLedBlinkConfig - 01/2026 - Refactored "ConfigMenu" (this function manteined for
* retrocompatibility)
**  Enable or disable led blink
**********************************************************************/
void setLedBlinkConfig() {
    options = {
        {"Led Blink off", [=]() { bruceConfig.setLedBlinkEnabled(0); }, bruceConfig.ledBlinkEnabled == 0},
        {"Led Blink on",  [=]() { bruceConfig.setLedBlinkEnabled(1); }, bruceConfig.ledBlinkEnabled == 1},
    };
    loopOptions(options, bruceConfig.ledBlinkEnabled);
}
#endif

/*********************************************************************
**  Function: setWifiStartupConfig
**  Enable or disable wifi connection at startup
**********************************************************************/
void setWifiStartupConfig() {
    options = {
        {"Disable", [=]() { bruceConfig.setWifiAtStartup(0); }, bruceConfig.wifiAtStartup == 0},
        {"Enable",  [=]() { bruceConfig.setWifiAtStartup(1); }, bruceConfig.wifiAtStartup == 1},
    };
    loopOptions(options, bruceConfig.wifiAtStartup);
}

/*********************************************************************
**  Function: addEvilWifiMenu
**  Handles Menu to add evil wifi names into config list
**********************************************************************/
void addEvilWifiMenu() {
    String apName = keyboard("", 30, "Evil Portal SSID");
    if (apName != "\x1B") bruceConfig.addEvilWifiName(apName);
}

/*********************************************************************
**  Function: removeEvilWifiMenu
**  Handles Menu to remove evil wifi names from config list
**********************************************************************/
void removeEvilWifiMenu() {
    options = {};

    for (const auto &wifi_name : bruceConfig.evilWifiNames) {
        options.push_back({wifi_name.c_str(), [wifi_name]() { bruceConfig.removeEvilWifiName(wifi_name); }});
    }

    options.push_back({"Cancel", [=]() { backToMenu(); }});

    loopOptions(options);
}

/*********************************************************************
**  Function: setEvilEndpointCreds
**  Handles menu for changing the endpoint to access captured creds
**********************************************************************/
void setEvilEndpointCreds() {
    String userInput = keyboard(bruceConfig.evilPortalEndpoints.getCredsEndpoint, 30, "Evil creds endpoint");
    if (userInput != "\x1B") bruceConfig.setEvilEndpointCreds(userInput);
}

/*********************************************************************
**  Function: setEvilEndpointSsid
**  Handles menu for changing the endpoint to change evilSsid
**********************************************************************/
void setEvilEndpointSsid() {
    String userInput = keyboard(bruceConfig.evilPortalEndpoints.setSsidEndpoint, 30, "Evil creds endpoint");
    if (userInput != "\x1B") bruceConfig.setEvilEndpointSsid(userInput);
}

/*********************************************************************
**  Function: setEvilAllowGetCredentials
**  Handles menu for toggling access to the credential list endpoint
**********************************************************************/

void setEvilAllowGetCreds() {
    options = {
        {"Disallow",
         [=]() { bruceConfig.setEvilAllowGetCreds(false); },
         bruceConfig.evilPortalEndpoints.allowGetCreds == false},
        {"Allow",
         [=]() { bruceConfig.setEvilAllowGetCreds(true); },
         bruceConfig.evilPortalEndpoints.allowGetCreds == true },
    };
    loopOptions(options, bruceConfig.evilPortalEndpoints.allowGetCreds);
}

/*********************************************************************
**  Function: setEvilAllowGetCredentials
**  Handles menu for toggling access to the change SSID endpoint
**********************************************************************/

void setEvilAllowSetSsid() {
    options = {
        {"Disallow",
         [=]() { bruceConfig.setEvilAllowSetSsid(false); },
         bruceConfig.evilPortalEndpoints.allowSetSsid == false},
        {"Allow",
         [=]() { bruceConfig.setEvilAllowSetSsid(true); },
         bruceConfig.evilPortalEndpoints.allowSetSsid == true },
    };
    loopOptions(options, bruceConfig.evilPortalEndpoints.allowSetSsid);
}

/*********************************************************************
**  Function: setEvilAllowEndpointDisplay
**  Handles menu for toggling the display of the Evil Portal endpoints
**********************************************************************/

void setEvilAllowEndpointDisplay() {
    options = {
        {"Disallow",
         [=]() { bruceConfig.setEvilAllowEndpointDisplay(false); },
         bruceConfig.evilPortalEndpoints.showEndpoints == false},
        {"Allow",
         [=]() { bruceConfig.setEvilAllowEndpointDisplay(true); },
         bruceConfig.evilPortalEndpoints.showEndpoints == true },
    };
    loopOptions(options, bruceConfig.evilPortalEndpoints.showEndpoints);
}

/*********************************************************************
** Function: setEvilPasswordMode
** Handles menu for setting the evil portal password mode
***********************************************************************/
void setEvilPasswordMode() {
    options = {
        {"Save 'password'",
         [=]() { bruceConfig.setEvilPasswordMode(FULL_PASSWORD); },
         bruceConfig.evilPortalPasswordMode == FULL_PASSWORD  },
        {"Save 'p******d'",
         [=]() { bruceConfig.setEvilPasswordMode(FIRST_LAST_CHAR); },
         bruceConfig.evilPortalPasswordMode == FIRST_LAST_CHAR},
        {"Save '*hidden*'",
         [=]() { bruceConfig.setEvilPasswordMode(HIDE_PASSWORD); },
         bruceConfig.evilPortalPasswordMode == HIDE_PASSWORD  },
        {"Save length",
         [=]() { bruceConfig.setEvilPasswordMode(SAVE_LENGTH); },
         bruceConfig.evilPortalPasswordMode == SAVE_LENGTH    },
    };
    loopOptions(options, bruceConfig.evilPortalPasswordMode);
}

/*********************************************************************
**  Function: setRFModuleMenu
**  Handles Menu to set the RF module in use
**********************************************************************/
void setRFModuleMenu() {
    int result = 0;
    int idx = 0;
    uint8_t pins_setup = 0;
    if (bruceConfigPins.rfModule == M5_RF_MODULE) idx = 0;
    else if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        idx = 1;
#if defined(ARDUINO_M5STICK_C_PLUS) || defined(ARDUINO_M5STICK_C_PLUS2)
        if (bruceConfigPins.CC1101_bus.mosi == GPIO_NUM_26) idx = 2;
#endif
    }

    options = {
        {"M5 RF433T/R",         [&]() { result = M5_RF_MODULE; }   },
#if defined(ARDUINO_M5STICK_C_PLUS) || defined(ARDUINO_M5STICK_C_PLUS2)
        {"CC1101 (legacy)",     [&pins_setup]() { pins_setup = 1; }},
        {"CC1101 (Shared SPI)", [&pins_setup]() { pins_setup = 2; }},
#else
        {"CC1101", [&]() { result = CC1101_SPI_MODULE; }},
#endif
        /* WIP:
         * #ifdef USE_CC1101_VIA_PCA9554
         * {"CC1101+PCA9554",  [&]() { result = 2; }},
         * #endif
         */
    };
    loopOptions(options, idx);
    if (result == CC1101_SPI_MODULE || pins_setup > 0) {
        // This setting is meant to StickCPlus and StickCPlus2 to setup the ports from RF Menu
        if (pins_setup == 1) {
            result = CC1101_SPI_MODULE;
            bruceConfigPins.setCC1101Pins(
                {(gpio_num_t)CC1101_SCK_PIN,
                 (gpio_num_t)CC1101_MISO_PIN,
                 (gpio_num_t)CC1101_MOSI_PIN,
                 (gpio_num_t)CC1101_SS_PIN,
                 (gpio_num_t)CC1101_GDO0_PIN,
                 GPIO_NUM_NC}
            );
            bruceConfigPins.setNrf24Pins(
                {(gpio_num_t)CC1101_SCK_PIN,
                 (gpio_num_t)CC1101_MISO_PIN,
                 (gpio_num_t)CC1101_MOSI_PIN,
                 (gpio_num_t)CC1101_SS_PIN,
                 (gpio_num_t)CC1101_GDO0_PIN,
                 GPIO_NUM_NC}
            );
        } else if (pins_setup == 2) {
#if CONFIG_SOC_GPIO_OUT_RANGE_MAX > 30
            result = CC1101_SPI_MODULE;
            bruceConfigPins.setCC1101Pins(
                {(gpio_num_t)SDCARD_SCK,
                 (gpio_num_t)SDCARD_MISO,
                 (gpio_num_t)SDCARD_MOSI,
                 GPIO_NUM_33,
                 GPIO_NUM_32,
                 GPIO_NUM_NC}
            );
            bruceConfigPins.setNrf24Pins(
                {(gpio_num_t)SDCARD_SCK,
                 (gpio_num_t)SDCARD_MISO,
                 (gpio_num_t)SDCARD_MOSI,
                 GPIO_NUM_33,
                 GPIO_NUM_32,
                 GPIO_NUM_NC}
            );
#endif
        }
        if (initRfModule()) {
            bruceConfigPins.setRfModule(CC1101_SPI_MODULE);
            deinitRfModule();
            if (pins_setup == 1) CC_NRF_SPI.end();
            return;
        }
        // else display an error
        displayError("CC1101 not found", true);
        if (pins_setup == 1)
            qrcode_display("https://github.com/pr3y/Bruce/blob/main/media/connections/cc1101_stick.jpg");
        if (pins_setup == 2)
            qrcode_display(
                "https://github.com/pr3y/Bruce/blob/main/media/connections/cc1101_stick_SDCard.jpg"
            );
        while (!check(AnyKeyPress)) vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    // fallback to "M5 RF433T/R" on errors
    bruceConfigPins.setRfModule(M5_RF_MODULE);
}

/*********************************************************************
**  Function: setRFFreqMenu
**  Handles Menu to set the default frequency for the RF module
**********************************************************************/
void setRFFreqMenu() {
    float result = 433.92;
    String freq_str = num_keyboard(String(bruceConfigPins.rfFreq), 10, "Default frequency:");
    if (freq_str == "\x1B") return;
    if (freq_str.length() > 1) {
        result = freq_str.toFloat();          // returns 0 if not valid
        if (result >= 280 && result <= 928) { // TODO: check valid freq according to current module?
            bruceConfigPins.setRfFreq(result);
            return;
        }
    }
    // else
    displayError("Invalid frequency");
    bruceConfigPins.setRfFreq(433.92); // reset to default
    delay(1000);
}

/*********************************************************************
**  Function: setRFIDModuleMenu
**  Handles Menu to set the RFID module in use
**********************************************************************/
void setRFIDModuleMenu() {
    options = {
        {"M5 RFID2",
         [=]() { bruceConfigPins.setRfidModule(M5_RFID2_MODULE); },
         bruceConfigPins.rfidModule == M5_RFID2_MODULE     },
#ifdef M5STICK
        {"PN532 I2C G33",
         [=]() { bruceConfigPins.setRfidModule(PN532_I2C_MODULE); },
         bruceConfigPins.rfidModule == PN532_I2C_MODULE    },
        {"PN532 I2C G36",
         [=]() { bruceConfigPins.setRfidModule(PN532_I2C_SPI_MODULE); },
         bruceConfigPins.rfidModule == PN532_I2C_SPI_MODULE},
#else
        {"PN532 on I2C",
         [=]() { bruceConfigPins.setRfidModule(PN532_I2C_MODULE); },
         bruceConfigPins.rfidModule == PN532_I2C_MODULE},
#endif
        {"PN532 on SPI",
         [=]() { bruceConfigPins.setRfidModule(PN532_SPI_MODULE); },
         bruceConfigPins.rfidModule == PN532_SPI_MODULE    },
        {"RC522 on SPI",
         [=]() { bruceConfigPins.setRfidModule(RC522_SPI_MODULE); },
         bruceConfigPins.rfidModule == RC522_SPI_MODULE    },
    };
    loopOptions(options, bruceConfigPins.rfidModule);
}

/*********************************************************************
**  Function: addMifareKeyMenu
**  Handles Menu to add MIFARE keys into config list
**********************************************************************/
void addMifareKeyMenu() {
    String key = keyboard("", 12, "MIFARE key");
    if (key != "\x1B") bruceConfig.addMifareKey(key);
}

/*********************************************************************
**  Function: setClock
**  Handles Menu to set timezone to NTP
**********************************************************************/
const char *ntpServer = "pool.ntp.org";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, 0, 0);

void setClock() {
#if defined(HAS_RTC)
    RTC_TimeTypeDef TimeStruct;
#if defined(HAS_RTC_BM8563)
    _rtc.GetBm8563Time();
#endif
#if defined(HAS_RTC_PCF85063A)
    _rtc.GetPcf85063Time();
#endif
#endif

    options = {
        {"Via NTP Set Timezone",                                                 [&]() { bruceConfig.setAutomaticTimeUpdateViaNTP(true); } },
        {"Set Time Manually",                                                    [&]() { bruceConfig.setAutomaticTimeUpdateViaNTP(false); }},
        {("Daylight Savings " + String(bruceConfig.dst ? "On" : "Off")).c_str(),
         [&]() {
             bruceConfig.setDST(!bruceConfig.dst);
             updateClockTimezone();
             returnToMenu = true;
         }                                                                                                                                 },
        {(bruceConfig.clock24hr ? "24-Hour Format" : "12-Hour Format"),          [&]() {
             bruceConfig.setClock24Hr(!bruceConfig.clock24hr);
             returnToMenu = true;
         }                                                          }
    };

    addOptionToMainMenu();
    loopOptions(options);

    if (returnToMenu) return;

    if (bruceConfig.automaticTimeUpdateViaNTP) {
        if (!wifiConnected) wifiConnectMenu();

        options.clear();

#ifndef LITE_VERSION

        struct TimezoneMapping {
            const char *name;
            float offset;
        };

        constexpr TimezoneMapping timezoneMappings[] = {
            {"UTC-12 (Baker Island, Howland Island)",     -12  },
            {"UTC-11 (Niue, Pago Pago)",                  -11  },
            {"UTC-10 (Honolulu, Papeete)",                -10  },
            {"UTC-9 (Anchorage, Gambell)",                -9   },
            {"UTC-9.5 (Marquesas Islands)",               -9.5 },
            {"UTC-8 (Los Angeles, Vancouver, Tijuana)",   -8   },
            {"UTC-7 (Denver, Phoenix, Edmonton)",         -7   },
            {"UTC-6 (Mexico City, Chicago, Tegucigalpa)", -6   },
            {"UTC-5 (New York, Toronto, Lima)",           -5   },
            {"UTC-4 (Caracas, Santiago, La Paz)",         -4   },
            {"UTC-3 (Brasilia, Sao Paulo, Montevideo)",   -3   },
            {"UTC-2 (South Georgia, Mid-Atlantic)",       -2   },
            {"UTC-1 (Azores, Cape Verde)",                -1   },
            {"UTC+0 (London, Lisbon, Casablanca)",        0    },
            {"UTC+0.5 (Tehran)",                          0.5  },
            {"UTC+1 (Berlin, Paris, Rome)",               1    },
            {"UTC+2 (Cairo, Athens, Johannesburg)",       2    },
            {"UTC+3 (Moscow, Riyadh, Nairobi)",           3    },
            {"UTC+3.5 (Tehran)",                          3.5  },
            {"UTC+4 (Dubai, Baku, Muscat)",               4    },
            {"UTC+4.5 (Kabul)",                           4.5  },
            {"UTC+5 (Islamabad, Karachi, Tashkent)",      5    },
            {"UTC+5.5 (New Delhi, Mumbai, Colombo)",      5.5  },
            {"UTC+5.75 (Kathmandu)",                      5.75 },
            {"UTC+6 (Dhaka, Almaty, Omsk)",               6    },
            {"UTC+6.5 (Yangon, Cocos Islands)",           6.5  },
            {"UTC+7 (Bangkok, Jakarta, Hanoi)",           7    },
            {"UTC+8 (Beijing, Singapore, Perth)",         8    },
            {"UTC+8.75 (Eucla)",                          8.75 },
            {"UTC+9 (Tokyo, Seoul, Pyongyang)",           9    },
            {"UTC+9.5 (Adelaide, Darwin)",                9.5  },
            {"UTC+10 (Sydney, Melbourne, Vladivostok)",   10   },
            {"UTC+10.5 (Lord Howe Island)",               10.5 },
            {"UTC+11 (Solomon Islands, Nouméa)",          11   },
            {"UTC+12 (Auckland, Fiji, Kamchatka)",        12   },
            {"UTC+12.75 (Chatham Islands)",               12.75},
            {"UTC+13 (Tonga, Phoenix Islands)",           13   },
            {"UTC+14 (Kiritimati)",                       14   }
        };

        int idx = 0;
        int i = 0;
        for (const auto &mapping : timezoneMappings) {
            if (bruceConfig.tmz == mapping.offset) { idx = i; }

            options.emplace_back(
                mapping.name, [=, &mapping]() { bruceConfig.setTmz(mapping.offset); }, idx == i
            );
            ++i;
        }

#else
        constexpr float timezoneOffsets[] = {-12, -11, -10,  -9.5, -9,  -8,    -7, -6, -5,   -4,
                                             -3,  -2,  -1,   0,    0.5, 1,     2,  3,  3.5,  4,
                                             4.5, 5,   5.5,  5.75, 6,   6.5,   7,  8,  8.75, 9,
                                             9.5, 10,  10.5, 11,   12,  12.75, 13, 14};

        int idx = 0;
        int i = 0;
        for (const auto &offset : timezoneOffsets) {
            if (bruceConfig.tmz == offset) idx = i;

            options.emplace_back(
                ("UTC" + String(offset >= 0 ? "+" : "") + String(offset)).c_str(),
                [=]() { bruceConfig.setTmz(offset); },
                bruceConfig.tmz == offset
            );
            ++i;
        }

#endif

        addOptionToMainMenu();

        loopOptions(options, idx);

        updateClockTimezone();

    } else {
        int hr, mn, am;
        options = {};
        for (int i = 0; i < 12; i++) {
            String tmp = String(i < 10 ? "0" : "") + String(i);
            options.push_back({tmp.c_str(), [&]() { delay(1); }});
        }

        hr = loopOptions(options, MENU_TYPE_SUBMENU, "Set Hour");
        options.clear();

        for (int i = 0; i < 60; i++) {
            String tmp = String(i < 10 ? "0" : "") + String(i);
            options.push_back({tmp.c_str(), [&]() { delay(1); }});
        }

        mn = loopOptions(options, MENU_TYPE_SUBMENU, "Set Minute");
        options.clear();

        options = {
            {"AM", [&]() { am = 0; } },
            {"PM", [&]() { am = 12; }},
        };

        loopOptions(options);

#if defined(HAS_RTC)
        TimeStruct.Hours = hr + am;
        TimeStruct.Minutes = mn;
        TimeStruct.Seconds = 0;
        _rtc.SetTime(&TimeStruct);
        _rtc.GetTime(&_time);
        _rtc.GetDate(&_date);

        struct tm timeinfo = {};
        timeinfo.tm_sec = _time.Seconds;
        timeinfo.tm_min = _time.Minutes;
        timeinfo.tm_hour = _time.Hours;
        timeinfo.tm_mday = _date.Date;
        timeinfo.tm_mon = _date.Month > 0 ? _date.Month - 1 : 0;
        timeinfo.tm_year = _date.Year >= 1900 ? _date.Year - 1900 : 0;
        time_t epoch = mktime(&timeinfo);
        struct timeval tv = {.tv_sec = epoch};
        settimeofday(&tv, nullptr);
#else
        rtc.setTime(0, mn, hr + am, 20, 06, CURRENT_YEAR); // send me a gift, @Pirata!
        struct tm t = rtc.getTimeStruct();
        time_t epoch = mktime(&t);
        struct timeval tv = {.tv_sec = epoch};
        settimeofday(&tv, nullptr);
#endif
        clock_set = true;
    }
}

void runClockLoop(bool showMenuHint) {
    int tmp = 0;
    unsigned long hintStartTime = millis();
    bool hintVisible = showMenuHint;

#if defined(HAS_RTC)
#if defined(HAS_RTC_BM8563)
    _rtc.GetBm8563Time();
#endif
#if defined(HAS_RTC_PCF85063A)
    _rtc.GetPcf85063Time();
#endif
    _rtc.GetTime(&_time);
#endif

    // Delay due to SelPress() detected on run
    tft.fillScreen(bruceConfig.bgColor);
    delay(300);

    for (;;) {
        if (millis() - tmp > 1000) {
#if defined(HAS_RTC)
            updateTimeStr(_rtc.getTimeStruct());
#else
            updateTimeStr(rtc.getTimeStruct());
#endif
            Serial.print("Current time: ");
            Serial.println(timeStr);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawRect(
                BORDER_PAD_X,
                BORDER_PAD_X,
                tftWidth - 2 * BORDER_PAD_X,
                tftHeight - 2 * BORDER_PAD_X,
                bruceConfig.priColor
            );
            uint8_t f_size = 4;
            for (uint8_t i = 4; i > 0; i--) {
                if (i * LW * strlen(timeStr) < (tftWidth - BORDER_PAD_X * 2)) {
                    f_size = i;
                    break;
                }
            }
            tft.setTextSize(f_size);
            tft.drawCentreString(timeStr, tftWidth / 2, tftHeight / 2 - 13, 1);

            // "OK to show menu" hint management
            if (hintVisible && (millis() - hintStartTime < 5000)) {
                tft.setTextSize(1);
                tft.drawCentreString("OK to show menu", tftWidth / 2, tftHeight / 2 + 25, 1);
            } else if (hintVisible && (millis() - hintStartTime >= 5000)) {
                // Clear hint after 5 seconds
                tft.fillRect(
                    BORDER_PAD_X + 1,
                    tftHeight / 2 + 20,
                    tftWidth - 2 * BORDER_PAD_X - 2,
                    20,
                    bruceConfig.bgColor
                );
                hintVisible = false;
            }
            tmp = millis();
        }

        // Checks to exit the loop
        if (check(SelPress)) {
            tft.fillScreen(bruceConfig.bgColor);
            if (showMenuHint) {
                // Exits the loop to return to the caller (ClockMenu)
                break;
            } else {
                // Original behavior
                returnToMenu = true;
                break;
            }
        }

        if (check(EscPress)) {
            tft.fillScreen(bruceConfig.bgColor);
            returnToMenu = true;
            break;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/*********************************************************************
**  Function: gsetIrTxPin
**  get or set IR Tx Pin
**********************************************************************/
int gsetIrTxPin(bool set) {
    int result = bruceConfigPins.irTx;

    if (result > 50) bruceConfigPins.setIrTxPin(TXLED);
    if (set) {
        options.clear();
        std::vector<std::pair<const char *, int>> pins;
        pins = IR_TX_PINS;
        int idx = 100;
        int j = 0;
        for (auto pin : pins) {
            if (pin.second == bruceConfigPins.irTx && idx == 100) idx = j;
            j++;
#ifdef ALLOW_ALL_GPIO_FOR_IR_RF
            int i = pin.second;
            if (i != TFT_CS && i != TFT_RST && i != TFT_SCLK && i != TFT_MOSI && i != TFT_BL &&
                i != TOUCH_CS && i != SDCARD_CS && i != SDCARD_MOSI && i != SDCARD_MISO)
#endif
                options.push_back(
                    {pin.first,
                     [=]() { bruceConfigPins.setIrTxPin(pin.second); },
                     pin.second == bruceConfigPins.irTx}
                );
        }

        loopOptions(options, idx);
        options.clear();

        Serial.println("Saved pin: " + String(bruceConfigPins.irTx));
    }

    returnToMenu = true;
    return bruceConfigPins.irTx;
}

void setIrTxRepeats() {
    uint8_t chRpts = 0; // Chosen Repeats

    options = {
        {"None",             [&]() { chRpts = 0; } },
        {"5  (+ 1 initial)", [&]() { chRpts = 5; } },
        {"10 (+ 1 initial)", [&]() { chRpts = 10; }},
        {"Custom",           [&]() {
             // up to 99 repeats
             String rpt =
                 num_keyboard(String(bruceConfigPins.irTxRepeats), 2, "Nbr of Repeats (+ 1 initial)");
             chRpts = static_cast<uint8_t>(rpt.toInt());
         }                       },
    };
    addOptionToMainMenu();

    loopOptions(options);

    if (returnToMenu) return;

    bruceConfigPins.setIrTxRepeats(chRpts);
}
/*********************************************************************
**  Function: gsetIrRxPin
**  get or set IR Rx Pin
**********************************************************************/
int gsetIrRxPin(bool set) {
    int result = bruceConfigPins.irRx;

    if (result > 45) bruceConfigPins.setIrRxPin(GROVE_SCL);
    if (set) {
        options.clear();
        std::vector<std::pair<const char *, int>> pins;
        pins = IR_RX_PINS;
        int idx = -1;
        int j = 0;
        for (auto pin : pins) {
            if (pin.second == bruceConfigPins.irRx && idx < 0) idx = j;
            j++;
#ifdef ALLOW_ALL_GPIO_FOR_IR_RF
            int i = pin.second;
            if (i != TFT_CS && i != TFT_RST && i != TFT_SCLK && i != TFT_MOSI && i != TFT_BL &&
                i != TOUCH_CS && i != SDCARD_CS && i != SDCARD_MOSI && i != SDCARD_MISO)
#endif
                options.push_back(
                    {pin.first,
                     [=]() { bruceConfigPins.setIrRxPin(pin.second); },
                     pin.second == bruceConfigPins.irRx}
                );
        }

        loopOptions(options);
    }

    returnToMenu = true;
    return bruceConfigPins.irRx;
}

/*********************************************************************
**  Function: gsetRfTxPin
**  get or set RF Tx Pin
**********************************************************************/
int gsetRfTxPin(bool set) {
    int result = bruceConfigPins.rfTx;

    if (result > 45) bruceConfigPins.setRfTxPin(GROVE_SDA);
    if (set) {
        options.clear();
        std::vector<std::pair<const char *, int>> pins;
        pins = RF_TX_PINS;
        int idx = -1;
        int j = 0;
        for (auto pin : pins) {
            if (pin.second == bruceConfigPins.rfTx && idx < 0) idx = j;
            j++;
#ifdef ALLOW_ALL_GPIO_FOR_IR_RF
            int i = pin.second;
            if (i != TFT_CS && i != TFT_RST && i != TFT_SCLK && i != TFT_MOSI && i != TFT_BL &&
                i != TOUCH_CS && i != SDCARD_CS && i != SDCARD_MOSI && i != SDCARD_MISO)
#endif
                options.push_back(
                    {pin.first,
                     [=]() { bruceConfigPins.setRfTxPin(pin.second); },
                     pin.second == bruceConfigPins.rfTx}
                );
        }

        loopOptions(options);
        options.clear();
    }

    returnToMenu = true;
    return bruceConfigPins.rfTx;
}

/*********************************************************************
**  Function: gsetRfRxPin
**  get or set FR Rx Pin
**********************************************************************/
int gsetRfRxPin(bool set) {
    int result = bruceConfigPins.rfRx;

    if (result > 36) bruceConfigPins.setRfRxPin(GROVE_SCL);
    if (set) {
        options.clear();
        std::vector<std::pair<const char *, int>> pins;
        pins = RF_RX_PINS;
        int idx = -1;
        int j = 0;
        for (auto pin : pins) {
            if (pin.second == bruceConfigPins.rfRx && idx < 0) idx = j;
            j++;
#ifdef ALLOW_ALL_GPIO_FOR_IR_RF
            int i = pin.second;
            if (i != TFT_CS && i != TFT_RST && i != TFT_SCLK && i != TFT_MOSI && i != TFT_BL &&
                i != TOUCH_CS && i != SDCARD_CS && i != SDCARD_MOSI && i != SDCARD_MISO)
#endif
                options.push_back(
                    {pin.first,
                     [=]() { bruceConfigPins.setRfRxPin(pin.second); },
                     pin.second == bruceConfigPins.rfRx}
                );
        }

        loopOptions(options);
        options.clear();
    }

    returnToMenu = true;
    return bruceConfigPins.rfRx;
}

/*********************************************************************
**  Function: setStartupApp
**  Handles Menu to set startup app
**********************************************************************/
void setStartupApp() {
    int idx = 0;
    if (bruceConfig.startupApp == "") idx = 0;

    options = {
        {"None", [=]() { bruceConfig.setStartupApp(""); }, bruceConfig.startupApp == ""}
    };

    int index = 0;
    for (String appName : startupApp.getAppNames()) {
        index++;
        if (bruceConfig.startupApp == appName) idx = index;

        options.push_back({appName.c_str(), [=]() {
                               bruceConfig.setStartupApp(appName);
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
                               if (appName == "JS Interpreter") {
                                   options = getScriptsOptionsList("", true);
                                   loopOptions(options, MENU_TYPE_SUBMENU, "Startup Script");
                               }
#endif
                           }});
    }

    loopOptions(options, idx);
    options.clear();
}

/*********************************************************************
**  Function: setGpsBaudrateMenu
**  Handles Menu to set the baudrate for the GPS module
**********************************************************************/
void setGpsBaudrateMenu() {
    options = {
        {"9600 bps",   [=]() { bruceConfigPins.setGpsBaudrate(9600); },  bruceConfigPins.gpsBaudrate == 9600 },
        {"19200 bps",  [=]() { bruceConfigPins.setGpsBaudrate(19200); }, bruceConfigPins.gpsBaudrate == 19200},
        {"38400 bps",  [=]() { bruceConfigPins.setGpsBaudrate(38400); }, bruceConfigPins.gpsBaudrate == 38400},
        {"57600 bps",  [=]() { bruceConfigPins.setGpsBaudrate(57600); }, bruceConfigPins.gpsBaudrate == 57600},
        {"115200 bps",
         [=]() { bruceConfigPins.setGpsBaudrate(115200); },
         bruceConfigPins.gpsBaudrate == 115200                                                               },
    };

    loopOptions(options, bruceConfigPins.gpsBaudrate);
}

/*********************************************************************
**  Function: setWifiApSsidMenu
**  Handles Menu to set the WiFi AP SSID
**********************************************************************/
void setWifiApSsidMenu() {
    const bool isDefault = bruceConfig.wifiAp.ssid == "BruceNet";

    options = {
        {"Default (BruceNet)",
         [=]() { bruceConfig.setWifiApCreds("BruceNet", bruceConfig.wifiAp.pwd); },
         isDefault                                                                            },
        {"Custom",
         [=]() {
             String newSsid = keyboard(bruceConfig.wifiAp.ssid, 32, "WiFi AP SSID:");
             if (newSsid != "\x1B") {
                 if (!newSsid.isEmpty()) bruceConfig.setWifiApCreds(newSsid, bruceConfig.wifiAp.pwd);
                 else displayError("SSID cannot be empty", true);
             }
         },                                                                         !isDefault},
    };
    addOptionToMainMenu();

    loopOptions(options, isDefault ? 0 : 1);
}

/*********************************************************************
**  Function: setWifiApPasswordMenu
**  Handles Menu to set the WiFi AP Password
**********************************************************************/
void setWifiApPasswordMenu() {
    const bool isDefault = bruceConfig.wifiAp.pwd == "brucenet";

    options = {
        {"Default (brucenet)",
         [=]() { bruceConfig.setWifiApCreds(bruceConfig.wifiAp.ssid, "brucenet"); },
         isDefault                                                                             },
        {"Custom",
         [=]() {
             String newPassword = keyboard(bruceConfig.wifiAp.pwd, 32, "WiFi AP Password:", true);
             if (newPassword != "\x1B") {
                 if (!newPassword.isEmpty()) bruceConfig.setWifiApCreds(bruceConfig.wifiAp.ssid, newPassword);
                 else displayError("Password cannot be empty", true);
             }
         },                                                                          !isDefault},
    };
    addOptionToMainMenu();

    loopOptions(options, isDefault ? 0 : 1);
}

/*********************************************************************
**  Function: setWifiApCredsMenu
**  Handles Menu to configure WiFi AP Credentials
**********************************************************************/
void setWifiApCredsMenu() {
    options = {
        {"SSID",     setWifiApSsidMenu    },
        {"Password", setWifiApPasswordMenu},
    };
    addOptionToMainMenu();

    loopOptions(options);
}

/*********************************************************************
**  Function: setNetworkCredsMenu
**  Main Menu for setting Network credentials (BLE & WiFi)
**********************************************************************/
void setNetworkCredsMenu() {
    options = {
        {"WiFi AP Creds", setWifiApCredsMenu}
    };
    addOptionToMainMenu();

    loopOptions(options);
}

/*********************************************************************
**  Function: setBadUSBBLEMenu
**  Main Menu for setting Bad USB/BLE options
**********************************************************************/
void setBadUSBBLEMenu() {
    options = {
        {"Keyboard Layout", setBadUSBBLEKeyboardLayoutMenu},
        {"Key Delay",       setBadUSBBLEKeyDelayMenu      },
        {"Show Output",     setBadUSBBLEShowOutputMenu    },
    };
    addOptionToMainMenu();

    loopOptions(options);
}

/*********************************************************************
**  Function: setBadUSBBLEKeyboardLayoutMenu
**  Main Menu for setting Bad USB/BLE Keyboard Layout
**********************************************************************/
void setBadUSBBLEKeyboardLayoutMenu() {
    uint8_t opt = bruceConfig.badUSBBLEKeyboardLayout;

    options.clear();
    options = {
        {"US International",      [&]() { opt = 0; } },
        {"Danish",                [&]() { opt = 1; } },
        {"English (UK)",          [&]() { opt = 2; } },
        {"French (AZERTY)",       [&]() { opt = 3; } },
        {"German",                [&]() { opt = 4; } },
        {"Hungarian",             [&]() { opt = 5; } },
        {"Italian",               [&]() { opt = 6; } },
        {"Polish",                [&]() { opt = 7; } },
        {"Portuguese (Brazil)",   [&]() { opt = 8; } },
        {"Portuguese (Portugal)", [&]() { opt = 9; } },
        {"Slovenian",             [&]() { opt = 10; }},
        {"Spanish",               [&]() { opt = 11; }},
        {"Swedish",               [&]() { opt = 12; }},
        {"Turkish",               [&]() { opt = 13; }},
    };
    addOptionToMainMenu();

    loopOptions(options, opt);

    if (opt != bruceConfig.badUSBBLEKeyboardLayout) { bruceConfig.setBadUSBBLEKeyboardLayout(opt); }
}

/*********************************************************************
**  Function: setBadUSBBLEKeyDelayMenu
**  Main Menu for setting Bad USB/BLE Keyboard Key Delay
**********************************************************************/
void setBadUSBBLEKeyDelayMenu() {
    String delayStr = num_keyboard(String(bruceConfig.badUSBBLEKeyDelay), 3, "Key Delay (ms):");
    if (delayStr != "\x1B") {
        uint16_t delayVal = static_cast<uint16_t>(delayStr.toInt());
        if (delayVal <= 500) {
            bruceConfig.setBadUSBBLEKeyDelay(delayVal);
        } else if (delayVal != 0) {
            displayError("Invalid key delay value (0 to 500)", true);
        }
    }
}

/*********************************************************************
**  Function: setBadUSBBLEShowOutputMenu
**  Main Menu for setting Bad USB/BLE Show Output
**********************************************************************/
void setBadUSBBLEShowOutputMenu() {
    options.clear();
    options = {
        {"Enable",  [&]() { bruceConfig.setBadUSBBLEShowOutput(true); } },
        {"Disable", [&]() { bruceConfig.setBadUSBBLEShowOutput(false); }},
    };
    addOptionToMainMenu();

    loopOptions(options, bruceConfig.badUSBBLEShowOutput ? 0 : 1);
}

/*********************************************************************
**  Function: setMacAddressMenu - @IncursioHack
**  Handles Menu to configure WiFi MAC Address
**********************************************************************/
void setMacAddressMenu() {
    String currentMAC = bruceConfig.wifiMAC;
    if (currentMAC == "") currentMAC = WiFi.macAddress();

    options.clear();
    options = {
        {"Default MAC (" + WiFi.macAddress() + ")",
         [&]() { bruceConfig.setWifiMAC(""); },
         bruceConfig.wifiMAC == ""},
        {"Set Custom MAC",
         [&]() {
             String newMAC = keyboard(bruceConfig.wifiMAC, 17, "XX:YY:ZZ:AA:BB:CC");
             if (newMAC == "\x1B") return;
             if (newMAC.length() == 17) {
                 bruceConfig.setWifiMAC(newMAC);
             } else {
                 displayError("Invalid MAC format");
             }
         }, bruceConfig.wifiMAC != ""},
        {"Random MAC", [&]() {
             uint8_t randomMac[6];
             for (int i = 0; i < 6; i++) randomMac[i] = random(0x00, 0xFF);
             char buf[18];
             sprintf(
                 buf,
                 "%02X:%02X:%02X:%02X:%02X:%02X",
                 randomMac[0],
                 randomMac[1],
                 randomMac[2],
                 randomMac[3],
                 randomMac[4],
                 randomMac[5]
             );
             bruceConfig.setWifiMAC(String(buf));
         }}
    };

    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_REGULAR, ("Current: " + currentMAC).c_str());
}

/*********************************************************************
**  Function: setSPIPins
**  Main Menu to manually set SPI Pins
**********************************************************************/
void setSPIPinsMenu(BruceConfigPins::SPIPins &value) {
    uint8_t opt = 0;
    bool changed = false;
    BruceConfigPins::SPIPins points = value;

RELOAD:
    options = {
        {String("SCK =" + String(points.sck)).c_str(), [&]() { opt = 1; }},
        {String("MISO=" + String(points.miso)).c_str(), [&]() { opt = 2; }},
        {String("MOSI=" + String(points.mosi)).c_str(), [&]() { opt = 3; }},
        {String("CS  =" + String(points.cs)).c_str(), [&]() { opt = 4; }},
        {String("CE/GDO0=" + String(points.io0)).c_str(), [&]() { opt = 5; }},
        {String("NC/GDO2=" + String(points.io2)).c_str(), [&]() { opt = 6; }},
        {"Save Config", [&]() { opt = 7; }, changed},
        {"Main Menu", [&]() { opt = 0; }},
    };

    loopOptions(options);
    if (opt == 0) return;
    else if (opt == 7) {
        if (changed) {
            value = points;
            bruceConfigPins.setSpiPins(value);
        }
    } else {
        options = {};
        gpio_num_t sel = GPIO_NUM_NC;
        int index = 0;
        if (opt == 1) index = points.sck + 1;
        else if (opt == 2) index = points.miso + 1;
        else if (opt == 3) index = points.mosi + 1;
        else if (opt == 4) index = points.cs + 1;
        else if (opt == 5) index = points.io0 + 1;
        else if (opt == 6) index = points.io2 + 1;
        for (int8_t i = -1; i <= GPIO_NUM_MAX; i++) {
            String tmp = String(i);
            options.push_back({tmp.c_str(), [i, &sel]() { sel = (gpio_num_t)i; }});
        }
        loopOptions(options, index);
        options.clear();
        if (opt == 1) points.sck = sel;
        else if (opt == 2) points.miso = sel;
        else if (opt == 3) points.mosi = sel;
        else if (opt == 4) points.cs = sel;
        else if (opt == 5) points.io0 = sel;
        else if (opt == 6) points.io2 = sel;
        changed = true;
        goto RELOAD;
    }
}

/*********************************************************************
**  Function: setUARTPins
**  Main Menu to manually set SPI Pins
**********************************************************************/
void setUARTPinsMenu(BruceConfigPins::UARTPins &value) {
    uint8_t opt = 0;
    bool changed = false;
    BruceConfigPins::UARTPins points = value;

RELOAD:
    options = {
        {String("RX = " + String(points.rx)).c_str(), [&]() { opt = 1; }},
        {String("TX = " + String(points.tx)).c_str(), [&]() { opt = 2; }},
        {"Save Config", [&]() { opt = 7; }, changed},
        {"Main Menu", [&]() { opt = 0; }},
    };

    loopOptions(options);
    if (opt == 0) return;
    else if (opt == 7) {
        if (changed) {
            value = points;
            bruceConfigPins.setUARTPins(value);
        }
    } else {
        options = {};
        gpio_num_t sel = GPIO_NUM_NC;
        int index = 0;
        if (opt == 1) index = points.rx + 1;
        else if (opt == 2) index = points.tx + 1;
        for (int8_t i = -1; i <= GPIO_NUM_MAX; i++) {
            String tmp = String(i);
            options.push_back({tmp.c_str(), [i, &sel]() { sel = (gpio_num_t)i; }});
        }
        loopOptions(options, index);
        options.clear();
        if (opt == 1) points.rx = sel;
        else if (opt == 2) points.tx = sel;
        changed = true;
        goto RELOAD;
    }
}

/*********************************************************************
**  Function: setI2CPins
**  Main Menu to manually set SPI Pins
**********************************************************************/
void setI2CPinsMenu(BruceConfigPins::I2CPins &value) {
    uint8_t opt = 0;
    bool changed = false;
    BruceConfigPins::I2CPins points = value;

RELOAD:
    options = {
        {String("SDA = " + String(points.sda)).c_str(), [&]() { opt = 1; }},
        {String("SCL = " + String(points.scl)).c_str(), [&]() { opt = 2; }},
        {"Save Config", [&]() { opt = 7; }, changed},
        {"Main Menu", [&]() { opt = 0; }},
    };

    loopOptions(options);
    if (opt == 0) return;
    else if (opt == 7) {
        if (changed) {
            value = points;
            bruceConfigPins.setI2CPins(value);
        }
    } else {
        options = {};
        gpio_num_t sel = GPIO_NUM_NC;
        int index = 0;
        if (opt == 1) index = points.sda + 1;
        else if (opt == 2) index = points.scl + 1;
        for (int8_t i = -1; i <= GPIO_NUM_MAX; i++) {
            String tmp = String(i);
            options.push_back({tmp.c_str(), [i, &sel]() { sel = (gpio_num_t)i; }});
        }
        loopOptions(options, index);
        options.clear();
        if (opt == 1) points.sda = sel;
        else if (opt == 2) points.scl = sel;
        changed = true;
        goto RELOAD;
    }
}

/*********************************************************************
**  Function: setTheme
**  Menu to change Theme
**********************************************************************/
void setTheme() {
    FS *fs = &LittleFS;
    options = {
        {"Little FS", [&]() { fs = &LittleFS; }},
        {"Default",
         [&]() {
             bruceConfig.removeTheme();
             bruceConfig.themePath = "";
             bruceConfig.theme.fs = 0;
             bruceConfig.secColor = DEFAULT_SECCOLOR;
             bruceConfig.bgColor = TFT_BLACK;
             bruceConfig.setUiColor(DEFAULT_PRICOLOR);
#ifdef HAS_RGB_LED
             bruceConfig.ledBright = 50;
             bruceConfig.ledColor = 0x960064;
             bruceConfig.ledEffect = 0;
             bruceConfig.ledEffectSpeed = 5;
             bruceConfig.ledEffectDirection = 1;
             ledSetup();
#endif
             bruceConfig.saveFile();
             fs = nullptr;
         }                                     },
        {"Main Menu", [&]() { fs = nullptr; }  }
    };
    if (setupSdCard()) {
        options.insert(options.begin(), {"SD Card", [&]() { fs = &SD; }});
    }
    loopOptions(options);
    if (fs == nullptr) return;

    String filepath = loopSD(*fs, true, "JSON");
    if (bruceConfig.openThemeFile(fs, filepath, true)) {
        bruceConfig.themePath = filepath;
        if (fs == &LittleFS) bruceConfig.theme.fs = 1;
        else if (fs == &SD) bruceConfig.theme.fs = 2;
        else bruceConfig.theme.fs = 0;

        bruceConfig.saveFile();
    }
}
#if !defined(LITE_VERSION)
BLE_API bleApi;
static bool ble_api_enabled = false;

void enableBLEAPI() {
    if (!ble_api_enabled) {
        // displayWarning("BLE API require huge amount of RAM.");
        // displayWarning("Some features may stop working.");
        Serial.println(ESP.getFreeHeap());
        bleApi.setup();
        Serial.println(ESP.getFreeHeap());
    } else {
        bleApi.end();
    }

    ble_api_enabled = !ble_api_enabled;
}

bool appStoreInstalled() {
    FS *fs;
    if (!getFsStorage(fs)) {
        log_i("Fail getting filesystem");
        return false;
    }

    return fs->exists("/BruceJS/Tools/App Store.js");
}

#include <HTTPClient.h>
void installAppStoreJS() {

    if (WiFi.status() != WL_CONNECTED) { wifiConnectMenu(WIFI_STA); }
    if (WiFi.status() != WL_CONNECTED) {
        displayWarning("WiFi not connected", true);
        return;
    }

    FS *fs;
    if (!getFsStorage(fs)) {
        log_i("Fail getting filesystem");
        return;
    }

    if (!fs->exists("/BruceJS")) {
        if (!fs->mkdir("/BruceJS")) {
            displayWarning("Failed to create /BruceJS directory", true);
            return;
        }
    }

    if (!fs->exists("/BruceJS/Tools")) {
        if (!fs->mkdir("/BruceJS/Tools")) {
            displayWarning("Failed to create /BruceJS/Tools directory", true);
            return;
        }
    }

    HTTPClient http;
    http.begin("http://ghp.iceis.co.uk/service/appstore/");
    int httpCode = http.GET();
    if (httpCode != 200) {
        http.end();
        displayWarning("Failed to download App Store", true);
        return;
    }

    File file = fs->open("/BruceJS/Tools/App Store.js", FILE_WRITE);
    if (!file) {
        displayWarning("Failed to save App Store", true);
        return;
    }
    file.print(http.getString());
    http.end();
    file.close();

    displaySuccess("App Store installed", true);
    displaySuccess("Goto JS Interpreter -> Tools -> App Store", true);
}
#endif
