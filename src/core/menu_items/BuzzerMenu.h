#ifndef __BUZZER_H__
#define __BUZZER_H__

#include <MenuItemInterface.h>

class BuzzerMenu : public MenuItemInterface {
public:
    BuzzerMenu() : MenuItemInterface("Buzzer") {}

    void optionsMenu(void);
    void drawIcon(float scale);
    void drawIconImg();
    bool getTheme() { return bruceConfig.theme.others; }
};

#endif
