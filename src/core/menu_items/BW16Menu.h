#ifndef __BW16_MENU_H__
#define __BW16_MENU_H__

#include <MenuItemInterface.h>
#include <modules/bw16/BW16.h>

class BW16Menu : public MenuItemInterface {
public:
    BW16Menu() : MenuItemInterface("BW16 5G") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return false; }
    String themePath() { return ""; }

    void runLoop() { bw16.loop(); }

private:
    BW16 bw16;
    void showAPList();
    void scanNetworks();
    void showActionMenu(int index);
};

#endif
