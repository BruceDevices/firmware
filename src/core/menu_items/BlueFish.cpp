#include "BlueFish.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "modules/NRF24/nrf_jammer_detector.h"
#include "modules/ble/ble_spam_detector.h"
#include "modules/ir/ir_attack_detector.h"
#include "modules/rf/rf_jammer_detector.h"

#if !defined(LITE_VERSION)
#include "modules/wifi/deauth_detect.h"
#endif

static void bluefish_info() {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("_Disclaimer_", tftWidth / 2, 10, 1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(15, 33);
    padprintln("This menu has Blue Team functions aimed at detecting some possible attacks.");
    padprintln("");
    padprintln("Results are an estimate from heuristics used to guess whether an attack is happening.");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    padprintln("");
    padprintln("This feature may not be fully accurate — adverse conditions can interfere with the analysis."
    );
    delay(1000);
    while (!check(AnyKeyPress)) { vTaskDelay(pdMS_TO_TICKS(1)); }
}

void BlueFishMenu::optionsMenu() {
    options.clear();
    options.push_back({"Information", bluefish_info});
    options.push_back({"2.4Ghz Jam Detector", jammer_detector});
    options.push_back({"SubGhz Jam Detector", rf_jammer_detector});
    options.push_back({"IR Attack Detector", ir_attack_detector});
    options.push_back({"BLE Spam Detector", ble_spam_detector});
#if !defined(LITE_VERSION)
    options.push_back({"Deauth Detect", deauth_detect_setup});
#endif

    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Blue Fish");
}

void BlueFishMenu::drawIcon(float scale) {
    clearIconArea();
    int iconW = scale * 76;
    int iconH = scale * 42;

    // Oval body, offset toward the head so there's room for the tail fin behind it.
    int bodyRx = iconW * 3 / 10;
    int bodyRy = iconH * 2 / 5;
    int bodyCx = iconCenterX - iconW / 8;
    int bodyCy = iconCenterY;

    int tailBaseX = bodyCx + bodyRx - scale * 3;
    int tailNotchX = iconCenterX + iconW / 2 - scale * 6;
    int tailTipX = iconCenterX + iconW / 2;

    // Tail: two lobes meeting at the body with a notch between their tips (forked fin silhouette)
    tft.fillTriangle(tailBaseX, bodyCy, tailTipX, bodyCy - bodyRy, tailNotchX, bodyCy, bruceConfig.priColor);
    tft.fillTriangle(tailBaseX, bodyCy, tailTipX, bodyCy + bodyRy, tailNotchX, bodyCy, bruceConfig.priColor);

    // Body
    tft.fillEllipse(bodyCx, bodyCy, bodyRx, bodyRy, bruceConfig.priColor);

    // Dorsal fin
    tft.fillTriangle(
        bodyCx - scale * 2,
        bodyCy - bodyRy + scale * 3,
        bodyCx + bodyRx / 2,
        bodyCy - bodyRy + scale * 3,
        bodyCx + scale * 4,
        bodyCy - bodyRy - scale * 9,
        bruceConfig.priColor
    );

    // Pectoral fin
    tft.fillTriangle(
        bodyCx - scale * 4,
        bodyCy + bodyRy * 3 / 10,
        bodyCx + scale * 6,
        bodyCy + bodyRy * 3 / 10,
        bodyCx - scale * 1,
        bodyCy + bodyRy + scale * 7,
        bruceConfig.priColor
    );

    // Eye
    int eyeR = max(2, bodyRy / 5);
    tft.fillCircle(bodyCx - bodyRx * 4 / 10, bodyCy - bodyRy * 3 / 10, eyeR, bruceConfig.bgColor);
}
