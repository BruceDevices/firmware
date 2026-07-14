#ifndef __APPS_MENU_H__
#define __APPS_MENU_H__

#include <MenuItemInterface.h>

class AppsMenu : public MenuItemInterface {
public:
    AppsMenu() : MenuItemInterface("Apps") {}

    void optionsMenu() override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    String themePath() override { return ""; }
};

#endif
