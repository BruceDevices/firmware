#ifndef __RF_MENU_H__
#define __RF_MENU_H__

#include <MenuItemInterface.h>

class RFMenu : public MenuItemInterface {
public:
    RFMenu() : MenuItemInterface("RF") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return bruceConfig.theme.rf; }
    const String& themePath() override { return bruceConfig.theme.paths.rf; }

private:
    void configMenu(void);
    void customRfAppsMenu(void);
    void cc1101RxProfileMenu(void);
};

#endif
