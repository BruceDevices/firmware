#pragma once
#include "core/menu_items/Menu.h"

class BleMenu : public Menu {
public:
    void optionsMenu() override;
    void drawIconImg() override;
    void drawIcon(float scale = 1.0) override;
    
private:
    std::vector<MenuItem> options;
};
