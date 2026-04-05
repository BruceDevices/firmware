#include "./dynamicjs.h"
#include "./modules/bjs_interpreter/interpreter.h"
#include "core/utils.h"
#include <LittleFS.h>

DynamicJSApp::DynamicJSApp(String name, String path, FS *fileSystem) : MenuItemInterface(name) {
    filePath = path;
    fs = fileSystem;
}

void DynamicJSApp::optionsMenu() { run_bjs_script_headless(*fs, filePath); }

void DynamicJSApp::drawIcon(float scale) {
    tft.setTextSize(2 * scale);
    tft.setTextColor(TFT_CYAN);
    tft.drawCentreString(getName(), tftWidth / 2, tftHeight / 2 - 10, 1);

    tft.setTextSize(1 * scale);
    tft.setTextColor(TFT_WHITE);
    tft.drawCentreString("(JS App)", tftWidth / 2, tftHeight / 2 + 15, 1);
}

bool DynamicJSApp::hasTheme() { return false; }
String DynamicJSApp::themePath() { return ""; }
