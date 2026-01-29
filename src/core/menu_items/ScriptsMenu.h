#ifndef __SCRIPTS_MENU_H__
#define __SCRIPTS_MENU_H__

#include <MenuItemInterface.h>

class ScriptsMenu : public MenuItemInterface {
public:
    ScriptsMenu() : MenuItemInterface("JS Interpreter") {}

    void optionsMenu();
    void drawIcon(float scale);
    bool hasTheme() { return bruceConfig.theme.interpreter; }
    String themePath() { return bruceConfig.theme.paths.interpreter; }
};

#endif
