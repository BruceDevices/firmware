#ifndef __TOOLS_MENU_H__
#define __TOOLS_MENU_H__

#include <MenuItemInterface.h>

class ToolsMenu : public MenuItemInterface {
public:
    ToolsMenu() : MenuItemInterface("Tools") {}

    void optionsMenu(void);
    void drawIcon(float scale);

    bool hasTheme() override { return false; }
    const String& themePath() override { static const String empty = ""; return empty; }
};

#endif
