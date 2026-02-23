#include "ConfigMenu.h"
#include "core/display.h"
#include "core/i2c_finder.h"
#include "core/main_menu.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#include "../mykeyboard.h"
#ifdef HAS_RGB_LED
#include "core/led_control.h"
#endif

/*********************************************************************
**  Function: optionsMenu
**  Main Config menu entry point
**********************************************************************/
void ConfigMenu::optionsMenu() {
    returnToMenu = false;
    while (true) {
        // Check if we need to exit to Main Menu (e.g., DevMode disabled)
        if (returnToMenu) {
            returnToMenu = false; // Reset flag
            return;
        }

        std::vector<Option> localOptions = {
            {"Display & UI",  [this]() { displayUIMenu(); }},
#ifdef HAS_RGB_LED
            {"LED Config",    [this]() { ledMenu(); }      },
#endif
            {"Audio Config",  [this]() { audioMenu(); }    },
            {"System Config", [this]() { systemMenu(); }   },
            {"Power",         [this]() { powerMenu(); }    },
        };
#if !defined(LITE_VERSION)
        if (!appStoreInstalled()) {
            localOptions.push_back({"Install App Store", []() { installAppStoreJS(); }});
        }
#endif

        if (bruceConfig.devMode) {
            localOptions.push_back({"Dev Mode", [this]() { devMenu(); }});
        }

        localOptions.push_back({"About", showDeviceInfo});
        localOptions.push_back({"Main Menu", []() {}});

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "Config");

        // Exit to Main Menu only if user pressed Back
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Otherwise rebuild Config menu after submenu returns
    }
}

/*********************************************************************
**  Function: displayUIMenu
**  Display & UI configuration submenu with auto-rebuild
**********************************************************************/
void ConfigMenu::displayUIMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {"Brightness",  [this]() { setBrightnessMenu(); }               },
            {"Dim Time",    [this]() { setDimmerTimeMenu(); }               },
            {"Orientation", [this]() { lambdaHelper(gsetRotation, true)(); }},
            {"UI Color",    [this]() { setUIColor(); }                      },
            {"UI Theme",    [this]() { setTheme(); }                        },
            {"Back",        []() {}                                         },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "Display & UI");

        // Exit only if user pressed Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Otherwise loop continues and menu rebuilds
    }
}

/*********************************************************************
**  Function: ledMenu
**  LED configuration submenu with auto-rebuild for toggles
**********************************************************************/
#ifdef HAS_RGB_LED
void ConfigMenu::ledMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {"LED Color",
             [this]() {
                 beginLed();
                 setLedColorConfig();
             }                                                                            },
            {"LED Effect",
             [this]() {
                 beginLed();
                 setLedEffectConfig();
             }                                                                            },
            {"LED Brightness",
             [this]() {
                 beginLed();
                 setLedBrightnessConfig();
             }                                                                            },
            {String("LED Blink: ") + (bruceConfig.ledBlinkEnabled ? "ON" : "OFF"),
             [this]() {
                 // Toggle LED blink setting
                 bruceConfig.ledBlinkEnabled = !bruceConfig.ledBlinkEnabled;
                 bruceConfig.saveFile();
             }                                                                            },
            {"Back",                                                               []() {}},
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "LED Config");

        // Exit only if user pressed Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds to update toggle label
    }
}
#endif
/*********************************************************************
**  Function: audioMenu
**  Audio configuration submenu with auto-rebuild for toggles
**********************************************************************/
void ConfigMenu::audioMenu() {
    while (true) {
        std::vector<Option> localOptions = {
#if !defined(LITE_VERSION)
#if defined(BUZZ_PIN) || defined(HAS_NS4168_SPKR)

            {String("Sound: ") + (bruceConfig.soundEnabled ? "ON" : "OFF"),
                                                             [this]() {
                 // Toggle sound setting
                 bruceConfig.soundEnabled = !bruceConfig.soundEnabled;
                 bruceConfig.saveFile();
             }                                                                                                                                            },
#if defined(HAS_NS4168_SPKR)
            {"Sound Volume",                                                [this]() { setSoundVolume(); }},
#endif  // BUZZ_PIN || HAS_NS4168_SPKR
#endif  //  HAS_NS4168_SPKR
#endif  //  LITE_VERSION
            {"Back",                                                        []() {}                       },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "Audio Config");

        // Exit only if user pressed Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds to update toggle label
    }
}

/*********************************************************************
**  Function: systemMenu
**  System configuration submenu with auto-rebuild for toggles
**********************************************************************/
void ConfigMenu::systemMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {String("InstaBoot: ") + (bruceConfig.instantBoot ? "ON" : "OFF"),
             [this]() {
                 // Toggle InstaBoot setting
                 bruceConfig.instantBoot = !bruceConfig.instantBoot;
                 bruceConfig.saveFile();
             }                                                                                                           },
            {String("WiFi Startup: ") + (bruceConfig.wifiAtStartup ? "ON" : "OFF"),
             [this]() {
                 // Toggle WiFi at startup setting
                 bruceConfig.wifiAtStartup = !bruceConfig.wifiAtStartup;
                 bruceConfig.saveFile();
             }                                                                                                           },
            {"Startup App",                                                         [this]() { setStartupApp(); }        },
            {"Hide/Show Apps",                                                      [this]() { mainMenu.hideAppsMenu(); }},
            {"Clock",                                                               [this]() { setClock(); }             },
            {String("Keyboard Language: ") + bruceConfig.keyboardLang,             [this]() { setKeyboardLanguage(); }  },
            {"Advanced",                                                            [this]() { advancedMenu(); }         },
            {"Back",                                                                []() {}                              },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "System Config");

        // Exit only if user pressed Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds to update toggle labels
    }
}

/*********************************************************************
**  Function: advancedMenu
**  Advanced settings submenu (nested under System Config)
**********************************************************************/
void ConfigMenu::advancedMenu() {
    while (true) {
        std::vector<Option> localOptions = {
#if !defined(LITE_VERSION)
            {"Toggle BLE API", [this]() { enableBLEAPI(); }       },
            {"BadUSB/BLE",     [this]() { setBadUSBBLEMenu(); }   },
#endif
            {"Network Creds",  [this]() { setNetworkCredsMenu(); }},
            {"Factory Reset",
                                      []() {
                 // Confirmation dialog for destructive action
                 drawMainBorder(true);
                 int8_t choice = displayMessage(
                     "Are you sure you want\nto Factory Reset?\nAll data will be lost!",
                     "No",
                     nullptr,
                     "Yes",
                     TFT_RED
                 );

                 if (choice == 1) {
                     // User confirmed - perform factory reset
                     bruceConfigPins.factoryReset();
                     bruceConfig.factoryReset(); // Restarts ESP
                 }
                 // If cancelled, loop continues and menu rebuilds
             }                                                                             },
            {"Back",           []() {}                            },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "Advanced");

        // Exit to System Config menu
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds after each action
    }
}
/*********************************************************************
**  Function: powerMenu
**  Power management submenu with auto-rebuild
**********************************************************************/
void ConfigMenu::powerMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {"Deep Sleep", goToDeepSleep          },
            {"Sleep",      setSleepMode           },
            {"Restart",    []() { ESP.restart(); }},
            {"Power Off",
             []() {
                 // Confirmation dialog for power off
                 drawMainBorder(true);
                 int8_t choice = displayMessage("Power Off Device?", "No", nullptr, "Yes", TFT_RED);

                 if (choice == 1) { powerOff(); }
             }                                    },
            {"Back",       []() {}                },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "Power Menu");

        // Exit to Config menu
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds after each action
    }
}

/*********************************************************************
**  Function: devMenu
**  Developer mode menu for advanced hardware configuration
**********************************************************************/
void ConfigMenu::devMenu() {
    while (true) {
        std::vector<Option> localOptions = {
            {"I2C Finder",      [this]() { find_i2c_addresses(); }                      },
            {"CC1101 Pins",     [this]() { setSPIPinsMenu(bruceConfigPins.CC1101_bus); }},
            {"NRF24  Pins",     [this]() { setSPIPinsMenu(bruceConfigPins.NRF24_bus); } },
#if !defined(LITE_VERSION)
            {"LoRa Pins",       [this]() { setSPIPinsMenu(bruceConfigPins.LoRa_bus); }  },
            {"W5500 Pins",      [this]() { setSPIPinsMenu(bruceConfigPins.W5500_bus); } },
#endif
            {"SDCard Pins",     [this]() { setSPIPinsMenu(bruceConfigPins.SDCARD_bus); }},
            {"I2C Pins",        [this]() { setI2CPinsMenu(bruceConfigPins.i2c_bus); }   },
            {"UART Pins",       [this]() { setUARTPinsMenu(bruceConfigPins.uart_bus); } },
            {"GPS Pins",        [this]() { setUARTPinsMenu(bruceConfigPins.gps_bus); }  },
            {"Serial USB",      [this]() { switchToUSBSerial(); }                       },
            {"Serial UART",     [this]() { switchToUARTSerial(); }                      },
            {"Disable DevMode", [this]() { bruceConfig.setDevMode(false); }             },
            {"Back",            []() {}                                                 },
        };

        int selected = loopOptions(localOptions, MENU_TYPE_SUBMENU, "Dev Mode");

        // Check if "Disable DevMode" was pressed (second-to-last option)
        if (selected == localOptions.size() - 2) {
            returnToMenu = true; // Signal to exit all Config menus
            return;
        }

        // Exit to Config menu on Back or ESC
        if (selected == -1 || selected == localOptions.size() - 1) { return; }
        // Menu rebuilds after each action
    }
}

/*********************************************************************
**  Function: switchToUSBSerial
**  Switch serial output to USB Serial
**********************************************************************/
void ConfigMenu::switchToUSBSerial() {
    USBserial.setSerialOutput(&Serial);
    Serial1.end();
}

/*********************************************************************
**  Function: switchToUARTSerial
**  Switch serial output to UART (handles pin conflicts)
**********************************************************************/
void ConfigMenu::switchToUARTSerial() {
    // Check and resolve SD card pin conflicts
    if (bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
        bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.uart_bus.tx)) {
        sdcardSPI.end();
    }

    // Check and resolve CC1101/NRF24 pin conflicts
    if (bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
        bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.uart_bus.tx) ||
        bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
        bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.uart_bus.tx)) {
        CC_NRF_SPI.end();
    }

    // Configure UART pins and switch serial output
    pinMode(bruceConfigPins.uart_bus.rx, INPUT);
    pinMode(bruceConfigPins.uart_bus.tx, OUTPUT);
    Serial1.begin(115200, SERIAL_8N1, bruceConfigPins.uart_bus.rx, bruceConfigPins.uart_bus.tx);
    USBserial.setSerialOutput(&Serial1);
}
/*********************************************************************
**  Function: drawIcon
**  Draw config gear icon
**********************************************************************/
void ConfigMenu::drawIcon(float scale) {
    clearIconArea();
    int radius = scale * 9;

    // Draw 6 gear teeth segments
    for (int i = 0; i < 6; i++) {
        tft.drawArc(
            iconCenterX,
            iconCenterY,
            3.5 * radius,
            2 * radius,
            15 + 60 * i,
            45 + 60 * i,
            bruceConfig.priColor,
            bruceConfig.bgColor,
            true
        );
    }

    // Draw inner circle
    tft.drawArc(
        iconCenterX,
        iconCenterY,
        2.5 * radius,
        radius,
        0,
        360,
        bruceConfig.priColor,
        bruceConfig.bgColor,
        false
    );
}
