#ifndef __HW_PROFILE_MENU_H__
#define __HW_PROFILE_MENU_H__

#ifdef TCA8418_I2C_ADDR

#include <MenuItemInterface.h>

class HWProfileMenu : public MenuItemInterface {
public:
    HWProfileMenu() : MenuItemInterface("Profile") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return false; }
    const String &themePath() override {
        static String empty;
        return empty;
    }
};

#endif // TCA8418_I2C_ADDR
#endif // __HW_PROFILE_MENU_H__
