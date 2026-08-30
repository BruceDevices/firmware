/**
 * @file QuickAccessMenu.h
 * @brief Quick Access menu — one-tap launch pinned IR, SubGHz, and BadUSB files
 * @version 0.1
 */

#ifndef QUICK_ACCESS_MENU_H
#define QUICK_ACCESS_MENU_H

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
    // Open a file-picker to add files from LittleFS or SD
    void addFilesMenu();
};

#endif // QUICK_ACCESS_MENU_H
