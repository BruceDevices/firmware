#include "ScriptsMenu.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"
#include "modules/bjs_interpreter/interpreter.h" // for JavaScript interpreter
#include <algorithm>                             // for std::sort

String getScriptsFolder(FS *&fs) {
    String folder;
    String possibleFolders[] = {"/scripts", "/BruceScripts", "/BruceJS"};
    int listSize = sizeof(possibleFolders) / sizeof(possibleFolders[0]);

    for (int i = 0; i < listSize; i++) {
        if (SD.exists(possibleFolders[i])) {
            fs = &SD;
            return possibleFolders[i];
        }
        if (LittleFS.exists(possibleFolders[i])) {
            fs = &LittleFS;
            return possibleFolders[i];
        }
    }
    return "";
}

std::vector<Option> getScriptsOptionsList() {
    std::vector<Option> opt = {};
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    FS *fs;
    String folder = getScriptsFolder(fs);
    if (folder == "") return opt; // did not find

    File root = fs->open(folder);
    if (!root || !root.isDirectory()) return opt; // not a dir

    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        String nameOnly = fullPath.substring(fullPath.lastIndexOf("/") + 1);
        if (fullPath == "") { break; }
        // Serial.printf("Path: %s (isDir: %d)\n", fullPath.c_str(), isDir);

        if (isDir) continue;

        int dotIndex = nameOnly.lastIndexOf(".");
        String ext = dotIndex >= 0 ? nameOnly.substring(dotIndex + 1) : "";
        ext.toUpperCase();
        if (ext != "JS" && ext != "BJS") continue;

        String entry_title = nameOnly.substring(0, nameOnly.lastIndexOf(".")); // remove the extension
        opt.push_back({entry_title.c_str(), [=]() { run_bjs_script_headless(*fs, fullPath); }});
    }

    root.close();

    std::sort(opt.begin(), opt.end(), [](const Option &a, const Option &b) {
        String fa = String(a.label);
        fa.toUpperCase();
        String fb = String(b.label);
        fb.toUpperCase();
        return fa < fb;
    });

#endif
    return opt;
}

void ScriptsMenu::optionsMenu() {
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    options = getScriptsOptionsList();

    options.push_back({"Load...", run_bjs_script});
    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Scripts");
#endif
}

void ScriptsMenu::drawIcon(float scale) {
    clearIconArea();

    int iconW = scale * 40;
    int iconH = scale * 60;

    if (iconW % 2 != 0) iconW++;
    if (iconH % 2 != 0) iconH++;

    int foldSize = iconH / 4;
    int arrowSize = iconW / 10;
    int arrowPadX = 2 * arrowSize;
    int arrowPadBottom = 3 * arrowPadX;
    int slashSize = 2 * arrowSize;

    // File
    tft.drawRect(iconCenterX - iconW / 2, iconCenterY - iconH / 2, iconW, iconH, bruceConfig.priColor);
    tft.fillRect(
        iconCenterX + iconW / 2 - foldSize, iconCenterY - iconH / 2, foldSize, foldSize, bruceConfig.bgColor
    );
    tft.drawTriangle(
        (iconCenterX + iconW / 2 - foldSize),
        (iconCenterY - iconH / 2),
        (iconCenterX + iconW / 2 - foldSize),
        (iconCenterY - iconH / 2 + foldSize - 1),
        (iconCenterX + iconW / 2 - 1),
        (iconCenterY - iconH / 2 + foldSize - 1),
        bruceConfig.priColor
    );

    // Left Arrow
    tft.drawLine(
        iconCenterX - iconW / 2 + arrowPadX,
        iconCenterY + iconH / 2 - arrowPadBottom,
        iconCenterX - iconW / 2 + arrowPadX + arrowSize,
        iconCenterY + iconH / 2 - arrowPadBottom + arrowSize,
        bruceConfig.priColor
    );
    tft.drawLine(
        iconCenterX - iconW / 2 + arrowPadX,
        iconCenterY + iconH / 2 - arrowPadBottom,
        iconCenterX - iconW / 2 + arrowPadX + arrowSize,
        iconCenterY + iconH / 2 - arrowPadBottom - arrowSize,
        bruceConfig.priColor
    );

    // Slash
    tft.drawLine(
        iconCenterX - slashSize / 2,
        iconCenterY + iconH / 2 - arrowPadBottom + arrowSize,
        iconCenterX + slashSize / 2,
        iconCenterY + iconH / 2 - arrowPadBottom - arrowSize,
        bruceConfig.priColor
    );

    // Right Arrow
    tft.drawLine(
        iconCenterX + iconW / 2 - arrowPadX,
        iconCenterY + iconH / 2 - arrowPadBottom,
        iconCenterX + iconW / 2 - arrowPadX - arrowSize,
        iconCenterY + iconH / 2 - arrowPadBottom + arrowSize,
        bruceConfig.priColor
    );
    tft.drawLine(
        iconCenterX + iconW / 2 - arrowPadX,
        iconCenterY + iconH / 2 - arrowPadBottom,
        iconCenterX + iconW / 2 - arrowPadX - arrowSize,
        iconCenterY + iconH / 2 - arrowPadBottom - arrowSize,
        bruceConfig.priColor
    );
}
