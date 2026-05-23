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

    int iconW = scale * 60;
    int iconH = scale * 60;
    if (iconW % 2 != 0) iconW++;
    if (iconH % 2 != 0) iconH++;

    int cx = iconCenterX;
    int cy = iconCenterY;
    int half = iconW / 2;

    tft.drawRoundRect(cx - half, cy - half, iconW, iconH, half / 4, bruceConfig.priColor);
    tft.drawLine(cx - half, cy - half + iconH / 4, cx + half, cy - half + iconH / 4, bruceConfig.priColor);

    int gridW = iconW / 5;
    int gridGap = iconW / 8;

    int gridCenterY = cy + iconH / 8;
    int gx1 = cx - gridW - gridGap / 2;
    int gy1 = gridCenterY - gridW - gridGap / 2;
    int gx2 = cx + gridGap / 2;
    int gy2 = gridCenterY + gridGap / 2;

    tft.fillRect(gx1, gy1, gridW, gridW, bruceConfig.priColor);
    tft.drawRect(gx2, gy1, gridW, gridW, bruceConfig.priColor);
    tft.drawRect(gx1, gy2, gridW, gridW, bruceConfig.priColor);
    tft.fillRect(gx2, gy2, gridW, gridW, bruceConfig.priColor);
}
