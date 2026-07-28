/**
 * @file QuickAccessMenu.h
 * @brief Quick Access menu — one-tap launch pinned IR, SubGHz, and BadUSB files
 * @version 0.1
 */

#ifndef __QUICK_ACCESS_MENU_H__
#define __QUICK_ACCESS_MENU_H__

#include <MenuItemInterface.h>

class QuickAccessMenu : public MenuItemInterface {
public:
    QuickAccessMenu() : MenuItemInterface("Quick Access") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override {
        static const String empty = "";
        return empty;
    }

private:
    // Render a sub-menu to manage (unpin) pinned items
    void managePinsMenu();
};

#endif // __QUICK_ACCESS_MENU_H__
