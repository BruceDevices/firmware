#ifndef __MAIN_MENU_H__
#define __MAIN_MENU_H__

#include <MenuItemInterface.h>

#include "menu_items/BleMenu.h"
#include "menu_items/ClockMenu.h"
#include "menu_items/ConfigMenu.h"
#include "menu_items/ConnectMenu.h"
#include "menu_items/EthernetMenu.h"
#include "menu_items/FMMenu.h"
#include "menu_items/FileMenu.h"
#include "menu_items/GpsMenu.h"
#include "menu_items/IRMenu.h"
#include "menu_items/LoRaMenu.h"
#include "menu_items/NRF24.h"
#include "menu_items/OthersMenu.h"
#include "menu_items/QuickAccessMenu.h"
#include "menu_items/RFIDMenu.h"
#include "menu_items/RFMenu.h"
#include "menu_items/ScriptsMenu.h"
#include "menu_items/WifiMenu.h"
class MainMenu {
public:
    FileMenu fileMenu;
    BleMenu bleMenu;
    ClockMenu clockMenu;
    ConnectMenu connectMenu;
    ConfigMenu configMenu;
    FMMenu fmMenu;
    GpsMenu gpsMenu;
    IRMenu irMenu;
    NRF24Menu nrf24Menu;
    OthersMenu othersMenu;
    RFIDMenu rfidMenu;
    RFMenu rfMenu;
    ScriptsMenu scriptsMenu;
    QuickAccessMenu quickAccessMenu;
    WifiMenu wifiMenu;
#if !defined(LITE_VERSION)
    LoRaMenu loraMenu;
    EthernetMenu ethernetMenu;
#endif

    MainMenu();
    ~MainMenu();

    void begin(void);
    std::vector<MenuItemInterface *> getItems(void) { return _menuItems; }
    void hideAppsMenu();

    // Grid layout rendering (bruceConfig.mainMenuStyle == MAIN_MENU_GRID)
    void drawGrid(int index);
    int gridIndexOf(MenuItemInterface *item);

    // Registered as gridPageTapHandler (see display.h) so loopOptions() can offer a page-up/down
    // tap zone without depending on this header. Returns true and writes newIndex when (x, y)
    // landed in the zone; currentIndex is used to keep roughly the same column on the new page.
    bool handleGridPageTap(int x, int y, int currentIndex, int &newIndex);

private:
    struct GridLayout {
        int x = 0;
        int y = 0;
        int cellW = 0;
        int cellH = 0;
        int cols = 1;
        int rows = 1;
        int visibleRows = 1;
        int iconBox = 0;
        int labelSize = 1;
    };

    void buildGridLayout(int itemCount);
    void drawGridCell(int index, bool selected);
    void drawGridScrollBar();

    int _currentIndex = 0;
    int _totalItems = 0;
    std::vector<MenuItemInterface *> _menuItems;

    GridLayout _grid;
    int _gridItemCount = 0;
    int _gridScroll = 0;
    int _gridLastIndex = -1;
    uint8_t _gridRotation = ROTATION;
    bool _gridRedrawAll = true;
};
extern MainMenu mainMenu;

#endif
