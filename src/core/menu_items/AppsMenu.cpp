#include "AppsMenu.h"
#include "core/display.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/bap_loader.h"
#include <LittleFS.h>
#include <SD.h>

void AppsMenu::optionsMenu() {
    options.clear();
    
    // Scan /apps directory on SD card first, then LittleFS
    File root = SD.open("/apps");
    if (!root || !root.isDirectory()) {
        root = LittleFS.open("/apps");
    }
    
    if (root && root.isDirectory()) {
        File file = root.openNextFile();
        while (file) {
            String name = file.name();
            if (name.endsWith(".bruce")) {
                String path = "/apps/" + name;
                
                // Read BapHeader to get the app name
                String appLabel = name.substring(0, name.length() - 6); // fallback (.bruce = 6 chars)
                
                // Optimize: Read directly from the current 'file' object instead of reopening
                BapHeader header;
                // Since this is the root directory file object, we might not be able to read contents directly 
                // if it's just a directory entry. But `openNextFile()` returns a File object that we can read.
                if (!file.isDirectory()) {
                    file.seek(0);
                    if (file.read((uint8_t*)&header, sizeof(header)) == sizeof(header)) {
                        if (memcmp(header.magic, BAP_MAGIC, 4) == 0) {
                            header.name[31] = '\0';
                            if (strlen(header.name) > 0) {
                                appLabel = String(header.name);
                            }
                        }
                    }
                }

                options.push_back({
                    appLabel,
                    [path]() {
                        launch_bap_app(path.c_str());
                    }
                });
            }
            file = root.openNextFile();
        }
        root.close();
    }
    
    if (options.empty()) {
        options.push_back({"No apps found", []() {}});
    }
    
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "Apps");
}

void AppsMenu::drawIcon(float scale) {
    clearIconArea();

    int iconW = scale * 40;
    int iconH = scale * 60;

    if (iconW % 2 != 0) iconW++;
    if (iconH % 2 != 0) iconH++;

    // Draw an app icon representation (a box with a dot)
    tft.drawRect(iconCenterX - iconW / 2, iconCenterY - iconH / 2, iconW, iconH, bruceConfig.priColor);
    tft.drawRect(iconCenterX - iconW / 4, iconCenterY - iconH / 4, iconW / 2, iconW / 2, bruceConfig.priColor);
    tft.fillCircle(iconCenterX, iconCenterY - iconH / 4 + iconW / 4, 3 * scale, bruceConfig.priColor);
}
