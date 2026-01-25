#ifndef __FM_MENU_H__
#define __FM_MENU_H__

#include <MenuItemInterface.h>

class FMMenu : public MenuItemInterface {
public:
    FMMenu() : MenuItemInterface("FM") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    void drawIconImg();
    bool getTheme() { return bruceConfig.theme.fm; }
};

#endif
