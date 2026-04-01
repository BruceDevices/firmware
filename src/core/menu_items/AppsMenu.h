#ifndef __APPS_MENU_H__
#define __APPS_MENU_H__

#include <MenuItemInterface.h>

class AppsMenu : public MenuItemInterface {
public:
    AppsMenu() : MenuItemInterface("Apps") {}

    void optionsMenu();
    void drawIcon(float scale);
    bool hasTheme() { return false; }
    String themePath() { return ""; }
};

#endif
