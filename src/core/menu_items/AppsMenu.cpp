#include "AppsMenu.h"
#include "core/display.h"
#include "core/utils.h"
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "core/app_launcher.h"
#endif

void AppsMenu::optionsMenu() {
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

    std::vector<AppManifest> apps = discoverApps();

    if (apps.empty()) {
        displayTextLine("No apps found.");
        displayTextLine("Put apps in /apps/ on SD.");
        delay(2000);
        return;
    }

    options.clear();
    for (size_t i = 0; i < apps.size(); i++) {
        AppManifest app = apps[i];
        String label = app.name;
        if (!app.version.isEmpty()) label += " v" + app.version;
        options.push_back({label.c_str(), [app]() { launchApp(app); }});
    }
    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Apps");
#endif
}

void AppsMenu::drawIcon(float scale) {
    clearIconArea();

    int size = scale * 14;
    int gap = scale * 4;
    int totalW = size * 2 + gap;

    int startX = iconCenterX - totalW / 2;
    int startY = iconCenterY - totalW / 2;

    // Draw a 2x2 grid of rounded squares (app grid icon)
    tft.drawRoundRect(startX, startY, size, size, 2, bruceConfig.priColor);
    tft.drawRoundRect(startX + size + gap, startY, size, size, 2, bruceConfig.priColor);
    tft.drawRoundRect(startX, startY + size + gap, size, size, 2, bruceConfig.priColor);
    tft.drawRoundRect(startX + size + gap, startY + size + gap, size, size, 2, bruceConfig.priColor);
}
