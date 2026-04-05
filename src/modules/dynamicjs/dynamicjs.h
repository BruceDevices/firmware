#include "MenuItemInterface.h"
#include <FS.h>

class DynamicJSApp : public MenuItemInterface {
public:
    String filePath;
    FS *fs; 

    DynamicJSApp(String name, String path, FS *fileSystem);

    void optionsMenu() override;
    void drawIcon(float scale = 1) override;
    bool hasTheme() override;
    String themePath() override;
};
