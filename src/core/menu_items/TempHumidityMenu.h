#pragma once
#ifndef __TEMP_N_HUMIDITY__
#define __TEMP_N_HUMIDITY__

#include <MenuItemInterface.h>

class TempHumidityMenu : public MenuItemInterface {
public:
    TempHumidityMenu() : MenuItemInterface("DHT22") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    void drawIconImg() override;
    bool getTheme() { return bruceConfig.theme.others; }

private:
    void drawIcon(int iconCenterX, int iconCenterY, int scale);
};

#endif
