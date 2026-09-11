#ifndef __MAIN_MENU_H__
#define __MAIN_MENU_H__

#include <MenuItemInterface.h>

#include "menu_items/BleMenu.h"
#include "menu_items/ConfigMenu.h"
#include "menu_items/ConnectMenu.h"
#include "menu_items/NRF24.h"
#include "menu_items/RFIDMenu.h"
#include "menu_items/RFMenu.h"
#include "menu_items/WifiMenu.h"
class MainMenu {
public:
    BleMenu bleMenu;
    ConnectMenu connectMenu;
    ConfigMenu configMenu;
    NRF24Menu nrf24Menu;
    RFIDMenu rfidMenu;
    RFMenu rfMenu;
    WifiMenu wifiMenu;
#if !defined(LITE_VERSION)
#endif

    MainMenu();
    ~MainMenu();

    void begin(void);
    std::vector<MenuItemInterface *> getItems(void) { return _menuItems; }
    void hideAppsMenu();

private:
    int _currentIndex = 0;
    int _totalItems = 0;
    std::vector<MenuItemInterface *> _menuItems;
};
extern MainMenu mainMenu;

#endif
