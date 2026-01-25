#ifndef __EXT_LED_CONTROL_H__
#define __EXT_LED_CONTROL_H__

#include <MenuItemInterface.h>

class EXTLedControlMenu : public MenuItemInterface {
    public:
        EXTLedControlMenu() : MenuItemInterface("Led Control") {}

        void optionsMenu(void);
        void drawIcon(float scale);
        void drawIconImg();
        bool getTheme() { return bruceConfig.theme.others; }
    };

#endif
