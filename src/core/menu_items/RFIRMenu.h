#ifndef __RFIR_MENU_H__
#define __RFIR_MENU_H__

#include <MenuItemInterface.h>

class RFIRMenu : public MenuItemInterface {
public:
    RFIRMenu() : MenuItemInterface("RF+IR Dual") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    bool hasTheme() { return false; }
    const String& themePath() override { return _noTheme; }

private:
    static const String _noTheme;
};

#endif
