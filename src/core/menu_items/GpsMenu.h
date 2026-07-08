#ifndef __GPS_MENU_H__
#define __GPS_MENU_H__

#include <MenuItemInterface.h>

class GpsMenu : public MenuItemInterface {
public:
    GpsMenu() : MenuItemInterface("GPS") {}

    void optionsMenu(void);
    void wardrivingMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return bruceConfig.theme.gps; }
    const String& themePath() override { return bruceConfig.theme.paths.gps; }

private:
    void configMenu(void);
};

#endif
