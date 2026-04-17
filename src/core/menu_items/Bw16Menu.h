#ifndef __BW16_MENU_H__
#define __BW16_MENU_H__

#include "MenuItemInterface.h"
#include "modules/bw16/bw16.h"

class Bw16Menu : public MenuItemInterface {
public:
    Bw16Menu() : MenuItemInterface("BW16") {}

    void optionsMenu(void);
    void attackMenu();
    void configMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return bruceConfig.theme.bw16; }
    String themePath() { return bruceConfig.theme.paths.bw16; }

private:
    static BW16Tool tool;
};

#endif
