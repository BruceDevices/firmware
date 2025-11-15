#include "settings.h"
#include "core/batteryLogger.h"
#include "core/wifi/wifi_common.h"
#include "display.h"
#include "modules/others/qrcode_menu.h"
#include "modules/rf/rf_utils.h" // for initRfModule
#include "mykeyboard.h"
#include "powerSave.h"
#include "sd_functions.h"
#include "settingsColor.h"
#include "utils.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <globals.h>

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
}

/*********************************************************************
**  Function: gsetRotation
**  get/set rotation value
**********************************************************************/
int gsetRotation(bool set) {
    int getRot = bruceConfig.rotation;
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
        bruceConfig.setRotation(result);
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

void setScreenOffTimeoutMenu() {
    struct TimeoutOption {
        const char *label;
        int value;
    };
    static const TimeoutOption entries[] = {
        {"5 s",       5  },
        {"15 s",      15 },
        {"30 s",      30 },
        {"1 min",     60 },
        {"2 min",     120},
        {"5 min",     300},
        {"Disabled",  0  },
    };

    options.clear();
    constexpr size_t entryCount = sizeof(entries) / sizeof(entries[0]);
    int selectedIndex = static_cast<int>(entryCount);

    for (size_t i = 0; i < entryCount; ++i) {
        if (bruceConfig.screenOffTimeout == entries[i].value) selectedIndex = i;
        options.push_back(
            {entries[i].label,
             [=]() { bruceConfig.setScreenOffTimeout(entries[i].value); },
             bruceConfig.screenOffTimeout == entries[i].value}
        );
    }

    options.push_back(
        {"Custom",
         [=]() {
             String initialValue =
                 bruceConfig.screenOffTimeout > 0 ? String(bruceConfig.screenOffTimeout) : "15";
             String result = num_keyboard(initialValue, 4, "Seconds (1-3600)");
             if (result == "\x1B" || result.length() == 0) return;
             int seconds = result.toInt();
             seconds = max(0, min(3600, seconds));
             bruceConfig.setScreenOffTimeout(seconds);
         },
         selectedIndex == static_cast<int>(entryCount)}
    );

    loopOptions(options, selectedIndex);
}

static void autoTimeoutCustomPrompt(
    const char *prompt, int currentSeconds, std::function<void(int)> setter
) {
    String initialValue = currentSeconds > 0 ? String(max(1, currentSeconds / 60)) : "0";
    String result = num_keyboard(initialValue, 4, prompt);
    if (result == "\x1B" || result.length() == 0) return;
    int minutes = result.toInt();
    minutes = max(0, min(1440, minutes));
    setter(minutes * 60);
}

void setAutoSleepTimeoutMenu() {
    struct TimeoutOption {
        const char *label;
        int value;
    };
    static const TimeoutOption entries[] = {
        {"Disabled", 0    },
        {"1 min",    60   },
        {"3 min",    180  },
        {"5 min",    300  },
        {"10 min",   600  },
        {"15 min",   900  },
        {"30 min",   1800 },
        {"60 min",   3600 },
    };

    options.clear();
    constexpr size_t entryCount = sizeof(entries) / sizeof(entries[0]);
    int selectedIndex = static_cast<int>(entryCount);

    for (size_t i = 0; i < entryCount; ++i) {
        if (bruceConfig.autoSleepTimeout == entries[i].value) selectedIndex = i;
        options.push_back(
            {entries[i].label,
             [=]() { bruceConfig.setAutoSleepTimeout(entries[i].value); },
             bruceConfig.autoSleepTimeout == entries[i].value}
        );
    }

    options.push_back(
        {"Custom",
         [=]() {
             autoTimeoutCustomPrompt("Minutes (1-1440)", bruceConfig.autoSleepTimeout, [](int seconds) {
                 bruceConfig.setAutoSleepTimeout(seconds);
             });
         },
         selectedIndex == static_cast<int>(entryCount)}
    );

    loopOptions(options, selectedIndex);
}

void setAutoDeepSleepTimeoutMenu() {
    struct TimeoutOption {
        const char *label;
        int value;
    };
    static const TimeoutOption entries[] = {
        {"Disabled", 0    },
        {"5 min",    300  },
        {"10 min",   600  },
        {"15 min",   900  },
        {"30 min",   1800 },
        {"60 min",   3600 },
        {"120 min",  7200 },
    };

    options.clear();
    constexpr size_t entryCount = sizeof(entries) / sizeof(entries[0]);
    int selectedIndex = static_cast<int>(entryCount);

    for (size_t i = 0; i < entryCount; ++i) {
        if (bruceConfig.autoDeepSleepTimeout == entries[i].value) selectedIndex = i;
        options.push_back(
            {entries[i].label,
             [=]() { bruceConfig.setAutoDeepSleepTimeout(entries[i].value); },
             bruceConfig.autoDeepSleepTimeout == entries[i].value}
        );
    }

    options.push_back(
        {"Custom",
         [=]() {
             autoTimeoutCustomPrompt(
                 "Minutes (1-1440)", bruceConfig.autoDeepSleepTimeout, [](int seconds) {
                     bruceConfig.setAutoDeepSleepTimeout(seconds);
                 }
             );
         },
         selectedIndex == static_cast<int>(entryCount)}
    );

    loopOptions(options, selectedIndex);
}

void setBatteryLogIntervalMenu() {
    struct IntervalOption {
        const char *label;
        int value;
    };
    static const IntervalOption entries[] = {
        {"Disabled",  0  },
        {"Every 1 s", 1  },
        {"Every 5 s", 5  },
        {"Every 15 s", 15 },
        {"Every 1 min", 60 },
        {"Every 5 min", 300},
        {"Every 15 min", 900},
    };

    options.clear();
    constexpr size_t entryCount = sizeof(entries) / sizeof(entries[0]);
    int selectedIndex = 0;
    bool matchedPreset = false;

    for (size_t i = 0; i < entryCount; ++i) {
        if (bruceConfig.batteryLogInterval == entries[i].value) {
            selectedIndex = i;
            matchedPreset = true;
        }

        options.push_back(
            {entries[i].label,
             [=]() {
                 bruceConfig.setBatteryLogInterval(entries[i].value);
                 BatteryLogger::updateIntervalFromConfig(entries[i].value > 0);
             },
             bruceConfig.batteryLogInterval == entries[i].value}
        );
    }

    size_t customIndex = entryCount;
    options.push_back(
        {"Custom",
         [=]() {
             String initialValue =
                 bruceConfig.batteryLogInterval > 0 ? String(bruceConfig.batteryLogInterval) : "60";
             String result = num_keyboard(initialValue, 4, "Seconds (1-3600)");
             if (result.isEmpty()) return;
             int seconds = constrain(result.toInt(), 1, 3600);
             bruceConfig.setBatteryLogInterval(seconds);
             BatteryLogger::updateIntervalFromConfig(true);
         },
         !matchedPreset && bruceConfig.batteryLogInterval > 0}
    );

    if (!matchedPreset && bruceConfig.batteryLogInterval > 0) selectedIndex = static_cast<int>(customIndex);
    loopOptions(options, selectedIndex);
}

void showBatteryLogText() { BatteryLogger::showLogAsText(); }

void showBatteryLogGraph() { BatteryLogger::showLogAsGraph(); }

void deleteBatteryLogFileMenu() {
    if (!BatteryLogger::logFileExists()) {
        displayWarning("Log file not found", true);
        return;
    }

    bool removed = BatteryLogger::deleteLogFile();
    displayWarning(removed ? "Log file deleted" : "Unable to delete log", true);
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
             bruceConfig.colorInverted}
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
**  Function: setSoundConfig
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

/*********************************************************************
**  Function: setLedBlinkConfig
**  Enable or disable led blink
**********************************************************************/
void setLedBlinkConfig() {
    options = {
        {"Led Blink off", [=]() { bruceConfig.setLedBlinkEnabled(0); }, bruceConfig.ledBlinkEnabled == 0},
        {"Led Blink on",  [=]() { bruceConfig.setLedBlinkEnabled(1); }, bruceConfig.ledBlinkEnabled == 1},
    };
    loopOptions(options, bruceConfig.ledBlinkEnabled);
}

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
    bruceConfig.addEvilWifiName(apName);
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
**  Function: setRFModuleMenu
**  Handles Menu to set the RF module in use
**********************************************************************/
void setRFModuleMenu() {
    int result = 0;
    int idx = 0;
    uint8_t pins_setup = 0;
    if (bruceConfig.rfModule == M5_RF_MODULE) idx = 0;
    else if (bruceConfig.rfModule == CC1101_SPI_MODULE) {
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
        } else if (pins_setup == 2) {
            result = CC1101_SPI_MODULE;
            bruceConfigPins.setCC1101Pins(
                {(gpio_num_t)SDCARD_SCK,
                 (gpio_num_t)SDCARD_MISO,
                 (gpio_num_t)SDCARD_MOSI,
                 GPIO_NUM_33,
                 GPIO_NUM_32,
                 GPIO_NUM_NC}
            );
        }
        if (initRfModule()) {
            bruceConfig.setRfModule(CC1101_SPI_MODULE);
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
    bruceConfig.setRfModule(M5_RF_MODULE);
}

/*********************************************************************
**  Function: setRFFreqMenu
**  Handles Menu to set the default frequency for the RF module
**********************************************************************/
void setRFFreqMenu() {
    float result = 433.92;
    String freq_str = keyboard(String(bruceConfig.rfFreq), 10, "Default frequency:");
    if (freq_str.length() > 1) {
        result = freq_str.toFloat();          // returns 0 if not valid
        if (result >= 280 && result <= 928) { // TODO: check valid freq according to current module?
            bruceConfig.setRfFreq(result);
            return;
        }
    }
    // else
    displayError("Invalid frequency");
    bruceConfig.setRfFreq(433.92); // reset to default
    delay(1000);
}

/*********************************************************************
**  Function: setRFIDModuleMenu
**  Handles Menu to set the RFID module in use
**********************************************************************/
void setRFIDModuleMenu() {
    options = {
        {"M5 RFID2",
         [=]() { bruceConfig.setRfidModule(M5_RFID2_MODULE); },
         bruceConfig.rfidModule == M5_RFID2_MODULE     },
#ifdef M5STICK
        {"PN532 I2C G33",
         [=]() { bruceConfig.setRfidModule(PN532_I2C_MODULE); },
         bruceConfig.rfidModule == PN532_I2C_MODULE    },
        {"PN532 I2C G36",
         [=]() { bruceConfig.setRfidModule(PN532_I2C_SPI_MODULE); },
         bruceConfig.rfidModule == PN532_I2C_SPI_MODULE},
#else
        {"PN532 on I2C",
         [=]() { bruceConfig.setRfidModule(PN532_I2C_MODULE); },
         bruceConfig.rfidModule == PN532_I2C_MODULE},
#endif
        {"PN532 on SPI",
         [=]() { bruceConfig.setRfidModule(PN532_SPI_MODULE); },
         bruceConfig.rfidModule == PN532_SPI_MODULE    },
        {"RC522 on SPI",
         [=]() { bruceConfig.setRfidModule(RC522_SPI_MODULE); },
         bruceConfig.rfidModule == RC522_SPI_MODULE    },
    };
    loopOptions(options, bruceConfig.rfidModule);
}

/*********************************************************************
**  Function: addMifareKeyMenu
**  Handles Menu to add MIFARE keys into config list
**********************************************************************/
void addMifareKeyMenu() {
    String key = keyboard("", 12, "MIFARE key");
    bruceConfig.addMifareKey(key);
}

/*********************************************************************
**  Function: setClock
**  Handles Menu to set timezone to NTP
**********************************************************************/
const char *ntpServer = "pool.ntp.org";
long selectedTimezone;
const int daylightOffset_sec = 0;
int timeHour;

TimeChangeRule BRST = {"BRST", Last, Sun, Oct, 0, timeHour};
Timezone myTZ(BRST, BRST);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, selectedTimezone, daylightOffset_sec);

void setClock() {
    bool auto_mode = true;

#if defined(HAS_RTC)
    RTC_TimeTypeDef TimeStruct;
    _rtc.GetBm8563Time();
#endif

    options = {
        {"NTP Timezone", [&]() { auto_mode = true; } },
        {"Manually set", [&]() { auto_mode = false; }},
    };
    addOptionToMainMenu();
    loopOptions(options);

    if (returnToMenu) return;

    if (auto_mode) {
        if (!wifiConnected) wifiConnectMenu();

        auto createTimezoneSetter = [&](int timezone) {
            return [&, timezone]() { bruceConfig.setTmz(timezone); };
        };

        options = {
            {"Los Angeles", createTimezoneSetter(-8), bruceConfig.tmz == -8},
            {"Chicago",     createTimezoneSetter(-6), bruceConfig.tmz == -6},
            {"New York",    createTimezoneSetter(-5), bruceConfig.tmz == -5},
            {"Brasilia",    createTimezoneSetter(-3), bruceConfig.tmz == -3},
            {"Pernambuco",  createTimezoneSetter(-2), bruceConfig.tmz == -2},
            {"Lisbon",      createTimezoneSetter(0),  bruceConfig.tmz == 0 },
            {"Paris",       createTimezoneSetter(1),  bruceConfig.tmz == 1 },
            {"Athens",      createTimezoneSetter(2),  bruceConfig.tmz == 2 },
            {"Moscow",      createTimezoneSetter(3),  bruceConfig.tmz == 3 },
            {"Dubai",       createTimezoneSetter(4),  bruceConfig.tmz == 4 },
            {"Jakarta",     createTimezoneSetter(7),  bruceConfig.tmz == 7 },
            {"Hong Kong",   createTimezoneSetter(8),  bruceConfig.tmz == 8 },
            {"Tokyo",       createTimezoneSetter(9),  bruceConfig.tmz == 9 },
            {"Sydney",      createTimezoneSetter(10), bruceConfig.tmz == 10},
        };
        addOptionToMainMenu();

        loopOptions(options);

        if (returnToMenu) return;

        timeClient.setTimeOffset(bruceConfig.tmz * 3600);
        timeClient.begin();
        timeClient.update();
        localTime = myTZ.toLocal(timeClient.getEpochTime());

#if defined(HAS_RTC)
        struct tm *timeinfo = localtime(&localTime);
        TimeStruct.Hours = timeinfo->tm_hour;
        TimeStruct.Minutes = timeinfo->tm_min;
        TimeStruct.Seconds = timeinfo->tm_sec;
        _rtc.SetTime(&TimeStruct);
#else
        rtc.setTime(timeClient.getEpochTime());
#endif

        clock_set = true;
        runClockLoop();
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
#else
        rtc.setTime(0, mn, hr + am, 20, 06, 2024); // send me a gift, @Pirata!
#endif
        clock_set = true;
        runClockLoop();
    }
}

void runClockLoop() {
    int tmp = 0;

#if defined(HAS_RTC)
    _rtc.GetBm8563Time();
    _rtc.GetTime(&_time);
#endif

    // Delay due to SelPress() detected on run
    tft.fillScreen(bruceConfig.bgColor);
    delay(300);

    for (;;) {
        if (millis() - tmp > 1000) {
#if !defined(HAS_RTC)
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
            tft.setCursor(64, tftHeight / 3 + 5);
            uint8_t f_size = 4;
            for (uint8_t i = 4; i > 0; i--) {
                if (i * LW * 8 < (tftWidth - BORDER_PAD_X * 2)) {
                    f_size = i;
                    break;
                }
            }
            tft.setTextSize(f_size);
#if defined(HAS_RTC)
            _rtc.GetBm8563Time();
            _rtc.GetTime(&_time);
            char timeString[9]; // Buffer para armazenar a string formatada "HH:MM:SS"
            snprintf(
                timeString,
                sizeof(timeString),
                "%02d:%02d:%02d",
                _time.Hours % 100,
                _time.Minutes % 100,
                _time.Seconds % 100
            );
            tft.drawCentreString(timeString, tftWidth / 2, tftHeight / 2 - 13, 1);
#else
            tft.drawCentreString(timeStr, tftWidth / 2, tftHeight / 2 - 13, 1);
#endif
            tmp = millis();
        }

        // Checks para sair do loop
        if (check(SelPress) or check(EscPress)) { // Apertar o botão power dos sticks
            tft.fillScreen(bruceConfig.bgColor);
            returnToMenu = true;
            break;
            // goto Exit;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

/*********************************************************************
**  Function: gsetIrTxPin
**  get or set IR Tx Pin
**********************************************************************/
int gsetIrTxPin(bool set) {
    int result = bruceConfig.irTx;

    if (result > 50) bruceConfig.setIrTxPin(LED);
    if (set) {
        options.clear();
        std::vector<std::pair<const char *, int>> pins;
        pins = IR_TX_PINS;
        int idx = 100;
        int j = 0;
        for (auto pin : pins) {
            if (pin.second == bruceConfig.irTx && idx == 100) idx = j;
            j++;
#ifdef ALLOW_ALL_GPIO_FOR_IR_RF
            int i = pin.second;
            if (i != TFT_CS && i != TFT_RST && i != TFT_SCLK && i != TFT_MOSI && i != TFT_BL &&
                i != TOUCH_CS && i != SDCARD_CS && i != SDCARD_MOSI && i != SDCARD_MISO)
#endif
                options.push_back(
                    {pin.first, [=]() { bruceConfig.setIrTxPin(pin.second); }, pin.second == bruceConfig.irTx}
                );
        }

        loopOptions(options, idx);
        options.clear();

        Serial.println("Saved pin: " + String(bruceConfig.irTx));
    }

    returnToMenu = true;
    return bruceConfig.irTx;
}

void setIrTxRepeats() {
    uint8_t chRpts = 0; // Chosen Repeats

    options = {
        {"None",             [&]() { chRpts = 0; } },
        {"5  (+ 1 initial)", [&]() { chRpts = 5; } },
        {"10 (+ 1 initial)", [&]() { chRpts = 10; }},
        {"Custom",           [&]() {
             // up to 99 repeats
             String rpt = keyboard(String(bruceConfig.irTxRepeats), 2, "Nbr of Repeats (+ 1 initial)");
             chRpts = static_cast<uint8_t>(rpt.toInt());
         }                       },
    };
    addOptionToMainMenu();

    loopOptions(options);

    if (returnToMenu) return;

    bruceConfig.setIrTxRepeats(chRpts);
}
/*********************************************************************
**  Function: gsetIrRxPin
**  get or set IR Rx Pin
**********************************************************************/
int gsetIrRxPin(bool set) {
    int result = bruceConfig.irRx;

    if (result > 45) bruceConfig.setIrRxPin(GROVE_SCL);
    if (set) {
        options.clear();
        std::vector<std::pair<const char *, int>> pins;
        pins = IR_RX_PINS;
        int idx = -1;
        int j = 0;
        for (auto pin : pins) {
            if (pin.second == bruceConfig.irRx && idx < 0) idx = j;
            j++;
#ifdef ALLOW_ALL_GPIO_FOR_IR_RF
            int i = pin.second;
            if (i != TFT_CS && i != TFT_RST && i != TFT_SCLK && i != TFT_MOSI && i != TFT_BL &&
                i != TOUCH_CS && i != SDCARD_CS && i != SDCARD_MOSI && i != SDCARD_MISO)
#endif
                options.push_back(
                    {pin.first, [=]() { bruceConfig.setIrRxPin(pin.second); }, pin.second == bruceConfig.irRx}
                );
        }

        loopOptions(options);
    }

    returnToMenu = true;
    return bruceConfig.irRx;
}

/*********************************************************************
**  Function: gsetRfTxPin
**  get or set RF Tx Pin
**********************************************************************/
int gsetRfTxPin(bool set) {
    int result = bruceConfig.rfTx;

    if (result > 45) bruceConfig.setRfTxPin(GROVE_SDA);
    if (set) {
        options.clear();
        std::vector<std::pair<const char *, int>> pins;
        pins = RF_TX_PINS;
        int idx = -1;
        int j = 0;
        for (auto pin : pins) {
            if (pin.second == bruceConfig.rfTx && idx < 0) idx = j;
            j++;
#ifdef ALLOW_ALL_GPIO_FOR_IR_RF
            int i = pin.second;
            if (i != TFT_CS && i != TFT_RST && i != TFT_SCLK && i != TFT_MOSI && i != TFT_BL &&
                i != TOUCH_CS && i != SDCARD_CS && i != SDCARD_MOSI && i != SDCARD_MISO)
#endif
                options.push_back(
                    {pin.first, [=]() { bruceConfig.setRfTxPin(pin.second); }, pin.second == bruceConfig.rfTx}
                );
        }

        loopOptions(options);
        options.clear();
    }

    returnToMenu = true;
    return bruceConfig.rfTx;
}

/*********************************************************************
**  Function: gsetRfRxPin
**  get or set FR Rx Pin
**********************************************************************/
int gsetRfRxPin(bool set) {
    int result = bruceConfig.rfRx;

    if (result > 36) bruceConfig.setRfRxPin(GROVE_SCL);
    if (set) {
        options.clear();
        std::vector<std::pair<const char *, int>> pins;
        pins = RF_RX_PINS;
        int idx = -1;
        int j = 0;
        for (auto pin : pins) {
            if (pin.second == bruceConfig.rfRx && idx < 0) idx = j;
            j++;
#ifdef ALLOW_ALL_GPIO_FOR_IR_RF
            int i = pin.second;
            if (i != TFT_CS && i != TFT_RST && i != TFT_SCLK && i != TFT_MOSI && i != TFT_BL &&
                i != TOUCH_CS && i != SDCARD_CS && i != SDCARD_MOSI && i != SDCARD_MISO)
#endif
                options.push_back(
                    {pin.first, [=]() { bruceConfig.setRfRxPin(pin.second); }, pin.second == bruceConfig.rfRx}
                );
        }

        loopOptions(options);
        options.clear();
    }

    returnToMenu = true;
    return bruceConfig.rfRx;
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

    int index = 1;
    for (String appName : startupApp.getAppNames()) {
        if (bruceConfig.startupApp == appName) idx = index++;

        options.push_back(
            {appName.c_str(),
             [=]() { bruceConfig.setStartupApp(appName); },
             bruceConfig.startupApp == appName}
        );
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
        {"9600 bps",   [=]() { bruceConfig.setGpsBaudrate(9600); },   bruceConfig.gpsBaudrate == 9600  },
        {"19200 bps",  [=]() { bruceConfig.setGpsBaudrate(19200); },  bruceConfig.gpsBaudrate == 19200 },
        {"38400 bps",  [=]() { bruceConfig.setGpsBaudrate(38400); },  bruceConfig.gpsBaudrate == 38400 },
        {"57600 bps",  [=]() { bruceConfig.setGpsBaudrate(57600); },  bruceConfig.gpsBaudrate == 57600 },
        {"115200 bps", [=]() { bruceConfig.setGpsBaudrate(115200); }, bruceConfig.gpsBaudrate == 115200},
    };

    loopOptions(options, bruceConfig.gpsBaudrate);
}

/*********************************************************************
**  Function: setBleNameMenu
**  Handles Menu to set BLE Gap Name
**********************************************************************/
void setBleNameMenu() {
    const String defaultBleName = "Keyboard_" + String((uint8_t)(ESP.getEfuseMac() >> 32), HEX);

    const bool isDefault = bruceConfig.bleName == defaultBleName;

    options = {
        {"Default", [=]() { bruceConfig.setBleName(defaultBleName); }, isDefault },
        {"Custom",
         [=]() {
             String newBleName = keyboard(bruceConfig.bleName, 30, "BLE Device Name:");
             if (!newBleName.isEmpty()) bruceConfig.setBleName(newBleName);
             else displayError("BLE Name cannot be empty", true);
         },                                                            !isDefault},
    };
    addOptionToMainMenu();

    loopOptions(options, isDefault ? 0 : 1);
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
             if (!newSsid.isEmpty()) bruceConfig.setWifiApCreds(newSsid, bruceConfig.wifiAp.pwd);
             else displayError("SSID cannot be empty", true);
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
             String newPassword = keyboard(bruceConfig.wifiAp.pwd, 32, "WiFi AP Password:");
             if (!newPassword.isEmpty()) bruceConfig.setWifiApCreds(bruceConfig.wifiAp.ssid, newPassword);
             else displayError("Password cannot be empty", true);
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
        {"WiFi AP Creds", setWifiApCredsMenu},
        {"BLE Name",      setBleNameMenu    },
    };
    addOptionToMainMenu();

    loopOptions(options);
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
        loopOptions(options);
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
    if (bruceConfig.openThemeFile(fs, filepath)) {
        bruceConfig.themePath = filepath;
        if (fs == &LittleFS) bruceConfig.theme.fs = 1;
        else if (fs == &SD) bruceConfig.theme.fs = 2;
        else bruceConfig.theme.fs = 0;

        bruceConfig.saveFile();
    }
}
