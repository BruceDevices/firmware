#ifndef __POWER_MENU_H__
#define __POWER_MENU_H__

#include <MenuItemInterface.h>

class PowerMenu : public MenuItemInterface {
public:
    PowerMenu() : MenuItemInterface("Power") {}

    void optionsMenu(void) override;
    void drawIcon(float scale = 1) override;
    void drawIconImg() override;
    bool getTheme() override { return bruceConfig.theme.power; }
};

#endif
