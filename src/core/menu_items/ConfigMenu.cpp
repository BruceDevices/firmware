#include "ConfigMenu.h"
#include "core/display.h"
#include "core/i2c_finder.h"
#include "core/main_menu.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/wifi/wifi_common.h"
#ifdef HAS_RGB_LED
#include "core/led_control.h"
#endif

void ConfigMenu::optionsMenu() {
    options = {
        {"Brightness", setBrightnessMenu},
        {"Dim Time", setDimmerTimeMenu},
        {"Orientation", lambdaHelper(gsetRotation, true)},
        {"UI Color", setUIColor},
        {"UI Theme", setTheme},
        {String("InstaBoot: " + String(bruceConfig.instantBoot ? "ON" : "OFF")),
         [=]() {
             bruceConfig.instantBoot = !bruceConfig.instantBoot;
             bruceConfig.saveFile();
         }},
#ifdef HAS_RGB_LED
        {"LED Color",
         [=]() {
             beginLed();
             setLedColorConfig();
         }},
        {"LED Effect",
         [=]() {
             beginLed();
             setLedEffectConfig();
         }},
        {"LED Brightness",
         [=]() {
             beginLed();
             setLedBrightnessConfig();
         }},
        {"Led Blink On/Off", setLedBlinkConfig},
#endif
        {"Sound On/Off", setSoundConfig},
#if defined(HAS_NS4168_SPKR)
        {"Sound Volume", setSoundVolume},
#endif
        {"Startup WiFi", setWifiStartupConfig},
        {"Startup App", setStartupApp},
        {"Hide/Show Apps", []() { mainMenu.hideAppsMenu(); }},
#if !defined(LITE_VERSION)
        {"Toggle BLE API", []() { enableBLEAPI(); }},
#endif
        {"Network Creds", setNetworkCredsMenu},
        {"BadUSB/BLE", setBadUSBBLEMenu},
        {"Clock", setClock},
        {"Sleep", setSleepMode},
        {"Factory Reset",
         [=]() {
             bruceConfigPins.factoryReset(); // this one doesnt restart ESP
             bruceConfig.factoryReset();     // after this one, it restarts
         }},
        {"Restart", [=]() { ESP.restart(); }},
    };

    options.push_back({"Turn-off", powerOff});
    options.push_back({"Deep Sleep", goToDeepSleep});

    if (bruceConfig.devMode) options.push_back({"Dev Mode", [this]() { devMenu(); }});

    options.push_back({"About", showDeviceInfo});
    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Config");
}

void ConfigMenu::devMenu() {
    options = {
        {"I2C Finder",      find_i2c_addresses                                   },
        {"CC1101 Pins",     [=]() { setSPIPinsMenu(bruceConfigPins.CC1101_bus); }},
        {"NRF24  Pins",     [=]() { setSPIPinsMenu(bruceConfigPins.NRF24_bus); } },
#if !defined(LITE_VERSION)
        {"LoRa Pins",       [=]() { setSPIPinsMenu(bruceConfigPins.LoRa_bus); }  },
        {"W5500 Pins",      [=]() { setSPIPinsMenu(bruceConfigPins.W5500_bus); } },
#endif
        {"SDCard Pins",     [=]() { setSPIPinsMenu(bruceConfigPins.SDCARD_bus); }},
        //{"SYSI2C Pins", [=]() { setI2CPinsMenu(bruceConfigPins.sys_i2c); }   },
        {"I2C Pins",        [=]() { setI2CPinsMenu(bruceConfigPins.i2c_bus); }   },
        {"UART Pins",       [=]() { setUARTPinsMenu(bruceConfigPins.uart_bus); } },
        {"GPS Pins",        [=]() { setUARTPinsMenu(bruceConfigPins.gps_bus); }  },
        {"Serial USB",
         [=]() {
             USBserial.setSerialOutput(&Serial);
             Serial1.end();
         }                                                                       },
        {"Serial UART",
         [=]() {
             if (bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
                 bruceConfigPins.SDCARD_bus.checkConflict(bruceConfigPins.uart_bus.tx)) {
                 sdcardSPI.end();
             }
             if (bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
                 bruceConfigPins.CC1101_bus.checkConflict(bruceConfigPins.uart_bus.tx) ||
                 bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.uart_bus.rx) ||
                 bruceConfigPins.NRF24_bus.checkConflict(bruceConfigPins.uart_bus.tx)) {
                 CC_NRF_SPI.end();
             }
             pinMode(bruceConfigPins.uart_bus.rx, INPUT);
             pinMode(bruceConfigPins.uart_bus.tx, OUTPUT);
             Serial1.begin(115200, SERIAL_8N1, bruceConfigPins.uart_bus.rx, bruceConfigPins.uart_bus.tx);
             USBserial.setSerialOutput(&Serial1);
         }                                                                       },
        {"Disable DevMode", [this]() { bruceConfig.setDevMode(false); }          },
        {"Back",            [this]() { optionsMenu(); }                          },
    };

    loopOptions(options, MENU_TYPE_SUBMENU, "Dev Mode");
}

void ConfigMenu::drawIcon(float scale) {
    clearIconArea();
    int radius = scale * 9;

    int i = 0;
    for (i = 0; i < 6; i++) {
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
