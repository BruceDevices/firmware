#ifndef __BLUE_FISH_MENU_H__
#define __BLUE_FISH_MENU_H__

#include <MenuItemInterface.h>

class BlueFishMenu : public MenuItemInterface {
public:
    BlueFishMenu() : MenuItemInterface("Blue Fish") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return bruceConfig.theme.bluefish; }
    const String &themePath() override { return bruceConfig.theme.paths.bluefish; }
};

#endif
