#include "main_menu.h"
#include "display.h"
#include "main_menu.h"
#include "modules/bjs_interpreter/interpreter.h"
#include "modules/dynamicjs/dynamicjs.h"
#include "utils.h"
#include <LittleFS.h>
#include <globals.h>
#include <vector>

extern String getScriptsFolder(FS *&fs);

MainMenu::MainMenu() {
    _menuItems = {
        &wifiMenu,
        &bleMenu,
#if !defined(LITE_VERSION)
        &ethernetMenu,
#endif
        &rfMenu,
        &rfidMenu,
        &irMenu,
#if defined(FM_SI4713) && !defined(LITE_VERSION)
        &fmMenu,
#endif
        &fileMenu,
        &gpsMenu,
        &nrf24Menu,
#if !defined(LITE_VERSION)
#if !defined(DISABLE_INTERPRETER)
        &scriptsMenu,
        &appsMenu,
#endif
        &loraMenu,
#endif
        &othersMenu,
        &clockMenu,
#if !defined(LITE_VERSION)
        &connectMenu,
#endif
        &configMenu,
    };

    _totalItems = _menuItems.size();
}
static std::vector<DynamicJSApp *> dynamicHomeApps;
void loadDynamicApps() {
    for (auto app : dynamicHomeApps) { delete app; }
    dynamicHomeApps.clear();

    for (String path : bruceConfig.pinnedScripts) {
        FS *appFs = nullptr;

        if (SD.exists(path)) {
            appFs = &SD;
        } else if (LittleFS.exists(path)) {
            appFs = &LittleFS;
        }

        if (appFs != nullptr) {
            String nameOnly = path.substring(path.lastIndexOf("/") + 1);
            int dotIndex = nameOnly.lastIndexOf(".");
            if (dotIndex > 0) { nameOnly = nameOnly.substring(0, dotIndex); }

            DynamicJSApp *newApp = new DynamicJSApp(nameOnly, path, appFs);
            dynamicHomeApps.push_back(newApp);
        }
    }
}
MainMenu::~MainMenu() {}

void MainMenu::begin(void) {
    returnToMenu = false;
    options = {};

    std::vector<String> l = bruceConfig.disabledMenus;

    // 1. Load all standard firmware apps
    for (int i = 0; i < _totalItems; i++) {
        String itemName = _menuItems[i]->getName();
        if (find(l.begin(), l.end(), itemName) == l.end()) { // If menu item is not disabled
            options.push_back(
                {// selected lambda
                 _menuItems[i]->getName(),
                 [this, i]() { _menuItems[i]->optionsMenu(); },
                 false,                                  // selected = false
                 [](void *menuItem, bool shouldRender) { // render lambda
                     if (!shouldRender) return false;
                     drawMainBorder(false);

                     MenuItemInterface *obj = static_cast<MenuItemInterface *>(menuItem);
                     float scale = float((float)tftWidth / (float)240);
                     if (bruceConfigPins.rotation & 0b01) scale = float((float)tftHeight / (float)135);
                     obj->draw(scale);
#if defined(HAS_TOUCH)
                     TouchFooter();
#endif
                     return true;
                 },
                 _menuItems[i]
                }
            );
        }
    }

    loadDynamicApps();

    for (auto app : dynamicHomeApps) {
        String itemName = app->getName();
        if (find(l.begin(), l.end(), itemName) == l.end()) {
            options.push_back(
                {app->getName(),
                 [app]() { app->optionsMenu(); },
                 false,
                 [](void *menuItem, bool shouldRender) {
                     if (!shouldRender) return false;
                     drawMainBorder(false);

                     MenuItemInterface *obj = static_cast<MenuItemInterface *>(menuItem);
                     float scale = float((float)tftWidth / (float)240);
                     if (bruceConfigPins.rotation & 0b01) scale = float((float)tftHeight / (float)135);
                     obj->draw(scale);
#if defined(HAS_TOUCH)
                     TouchFooter();
#endif
                     return true;
                 },
                 app}
            );
        }
    }
    // ----------------------------------------

    _currentIndex = loopOptions(options, MENU_TYPE_MAIN, "Main Menu", _currentIndex);
};

/*********************************************************************
**  Function: hideAppsMenu
**  Menu to Hide or show menus
**********************************************************************/

void MainMenu::hideAppsMenu() {
    auto items = this->getItems();
RESTART: // using gotos to avoid stackoverflow after many choices
    options.clear();
    for (auto item : items) {
        String label = item->getName();
        std::vector<String> l = bruceConfig.disabledMenus;
        bool enabled = find(l.begin(), l.end(), label) == l.end();
        options.push_back({label, [this, label]() { bruceConfig.addDisabledMenu(label); }, enabled});
    }
    options.push_back({"Show All", [=]() { bruceConfig.disabledMenus.clear(); }, true});
    addOptionToMainMenu();
    loopOptions(options);
    bruceConfig.saveFile();
    if (!returnToMenu) goto RESTART;
}
