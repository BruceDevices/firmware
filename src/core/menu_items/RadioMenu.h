#ifndef __RADIO_MENU_H__
#define __RADIO_MENU_H__

#include <MenuItemInterface.h>

class RadioMenu : public MenuItemInterface {
public:
    RadioMenu() : MenuItemInterface("Radio") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    void drawIconImg();
    bool getTheme() { return bruceConfig.theme.radio; }
};

#endif
