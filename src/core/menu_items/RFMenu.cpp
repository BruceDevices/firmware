#include "RFMenu.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"
#include "modules/rf/record.h"
#include "modules/rf/rf_bruteforce.h"
#include "modules/rf/rf_jammer.h"
#include "modules/rf/rf_listen.h"
#include "modules/rf/rf_scan.h"
#include "modules/rf/rf_send.h"
#include "modules/rf/rf_utils.h"
#include "modules/rf/rf_spectrum.h"
#include "modules/rf/rf_waterfall.h"
#include "modules/rf/rf_repeater.h"
#include "modules/rf/rf_subghz.h"
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "modules/bjs_interpreter/interpreter.h"
#include "core/sd_functions.h"
#endif

// RX bandwidth profile setting lives in rf_utils.cpp.
extern uint8_t bruceRfRxProfile;

#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
static void launchBruceRfScript(const char *path) {
    if (!sdcardMounted || !SD.exists(path)) {
        displayError(String("Missing SD file: ") + path, true);
        return;
    }
    run_bjs_script_headless(SD, String(path));
}
#endif

void RFMenu::optionsMenu() {
    options = {
        {"SubGHZ",          rf_subghz                 },
        {"Scan/copy",       [=]() { RFScan(); }       },
        {"Repeater",        rf_repeater               },
#if !defined(LITE_VERSION)
        {"Record RAW",      rf_raw_record             }, // Pablo-Ortiz-Lopez
        {"Custom SubGhz",   sendCustomRF              },
#endif
        {"Spectrum",        rf_spectrum               },
#if !defined(LITE_VERSION)
        {"RSSI Spectrum",   rf_CC1101_rssi            }, // @Pirata
        {"SquareWave Spec", rf_SquareWave             }, // @Pirata
        {"Spectogram",      rf_waterfall              }, // dev_eclipse
#if defined(BUZZ_PIN) or defined(HAS_NS4168_SPKR) and defined(RF_LISTEN_H)
        {"Listen",          rf_listen                 }, // dev_eclipse
#endif
        {"Bruteforce",      rf_bruteforce             }, // dev_eclipse
        {"Jammer",          [=]() { RFJammer(true); } },
#endif
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
        {"RF Tools (Custom)", [this]() { customRfAppsMenu(); }},
#endif
        {"Config",          [this]() { configMenu(); }},
    };
    addOptionToMainMenu();

    delay(200);
    String txt = "Radio Frequency";
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) txt += " (CC1101)"; // Indicates if CC1101 is connected
    else txt += " Tx: " + String(bruceConfigPins.rfTx) + " Rx: " + String(bruceConfigPins.rfRx);

    loopOptions(options, MENU_TYPE_SUBMENU, txt.c_str());
}

void RFMenu::configMenu() {
    options = {
        {"RF TX Pin", lambdaHelper(gsetRfTxPin, true)},
        {"RF RX Pin", lambdaHelper(gsetRfRxPin, true)},
        {"RF Module", setRFModuleMenu},
        {"RF Frequency", setRFFreqMenu},
        {"CC1101 RX Bandwidth", [this]() { cc1101RxProfileMenu(); }},
        {"Back", [this]() { optionsMenu(); }},
    };

    loopOptions(options, MENU_TYPE_SUBMENU, "RF Config");
}

void RFMenu::customRfAppsMenu() {
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    options = {
        {"RF Repeater", []() { launchBruceRfScript("/BruceJS/RF/01_RF_Repeater_Live.js"); }},
        {"Weather RF", []() { launchBruceRfScript("/BruceJS/RF/02_Weather_RF_Live.js"); }},
        {"Pager Monitor", []() { launchBruceRfScript("/BruceJS/RF/03_Pager_RF_Live.js"); }},
        {"TPMS Monitor", []() { launchBruceRfScript("/BruceJS/RF/04_TPMS_RF_Live.js"); }},
        {"Flipper Chat Test", []() { launchBruceRfScript("/BruceJS/RF/05_Flipper_Chat_Probe_Live.js"); }},
        {"Back", [this]() { optionsMenu(); }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "RF Tools");
#endif
}

void RFMenu::cc1101RxProfileMenu() {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE) {
        displayWarning("CC1101 module required", true);
        return;
    }
    options = {
        {"Auto / Default 256 kHz", []() { bruceRfRxProfile = 0; }},
        {"Narrow 58 kHz", []() { bruceRfRxProfile = 1; }},
        {"Balanced 135 kHz", []() { bruceRfRxProfile = 2; }},
        {"Wide 406 kHz", []() { bruceRfRxProfile = 3; }},
        {"Back", [this]() { configMenu(); }},
    };
    loopOptions(options, MENU_TYPE_SUBMENU, "CC1101 RX Bandwidth");
    // Profile applies at the next receiver initialization; no TX changes.
}

void RFMenu::drawIcon(float scale) {
    clearIconArea();
    int radius = scale * 7;
    int deltaRadius = scale * 10;
    int triangleSize = scale * 30;

    if (triangleSize % 2 != 0) triangleSize++;

    // Body
    tft.fillCircle(iconCenterX, iconCenterY - radius, radius, bruceConfig.priColor);
    tft.fillTriangle(
        iconCenterX,
        iconCenterY,
        iconCenterX - triangleSize / 2,
        iconCenterY + triangleSize,
        iconCenterX + triangleSize / 2,
        iconCenterY + triangleSize,
        bruceConfig.priColor
    );

    // Left Arcs
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius,
        2 * radius,
        40,
        140,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius + deltaRadius,
        2 * radius + deltaRadius,
        40,
        140,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius + 2 * deltaRadius,
        2 * radius + 2 * deltaRadius,
        40,
        140,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );

    // Right Arcs
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius,
        2 * radius,
        220,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius + deltaRadius,
        2 * radius + deltaRadius,
        220,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
    tft.drawArc(
        iconCenterX,
        iconCenterY - radius,
        2.5 * radius + 2 * deltaRadius,
        2 * radius + 2 * deltaRadius,
        220,
        320,
        bruceConfig.priColor,
        bruceConfig.bgColor
    );
}
