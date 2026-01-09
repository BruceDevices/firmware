#ifndef __OTHERS_MENU_H__
#define __OTHERS_MENU_H__

#include <MenuItemInterface.h>

class OthersMenu : public MenuItemInterface {
public:
    OthersMenu() : MenuItemInterface("Others") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return bruceConfig.theme.others; }
    String themePath() { return bruceConfig.theme.paths.others; }
};

#endif
