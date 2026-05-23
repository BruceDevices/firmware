#ifndef __SUBGHZ_MENU_H__
#define __SUBGHZ_MENU_H__

#include <MenuItemInterface.h>

class SubGHZMenu : public MenuItemInterface {
public:
    SubGHZMenu() : MenuItemInterface("SubGHz") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return bruceConfig.theme.subghz; }
    String themePath() { return bruceConfig.theme.paths.subghz; }
};

#endif
