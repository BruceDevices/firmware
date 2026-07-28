/**
 * @file QuickAccessMenu.cpp
 * @brief Quick Access menu implementation
 */

#include "QuickAccessMenu.h"
#include "core/display.h"
#include "core/quick_access.h"
#include "core/sd_functions.h"
#include "core/settings.h"
#include "core/utils.h"
#include "modules/ir/custom_ir.h"
#include "modules/rf/rf_send.h"
#include <globals.h>

#if defined(USB_as_HID)
#include "modules/badusb_ble/ducky_typer.h"
#endif

// ── Helper: resolve FS pointer from PinnedItem ───────────────────────────
static FS *resolveFs(const PinnedItem &item) {
    if (item.fsType == 1) {
        if (sdcardMounted) return &SD;
        return nullptr;
    }
    return &LittleFS;
}

// ── Helper: execute a pinned item ────────────────────────────────────────
static void executePinnedItem(const PinnedItem &item) {
    FS *fs = resolveFs(item);
    if (!fs) {
        displayError("SD not mounted", true);
        return;
    }

    if (!fs->exists(item.filepath)) {
        displayError("File not found", true);
        // Offer to remove the stale pin
        options = {
            {"Remove Pin", [&]() {
                 getQuickAccessManager().unpin(item.filepath);
                 displaySuccess("Pin removed", true);
             }},
            {"Main Menu",  [&]() { returnToMenu = true; }},
        };
        loopOptions(options);
        return;
    }

    if (item.type == "ir") {
        // Open the "Choose cmd" dialog for interactive use
        delay(200);
        chooseCmdIrFile(fs, item.filepath);
    } else if (item.type == "sub") {
        delay(200);
        RfCodes data{};
        if (readSubFile(fs, item.filepath, data)) {
            txSubFile(data);
        }
    }
#if defined(USB_as_HID)
    else if (item.type == "txt") {
        delay(200);
        ducky_startKb(hid_usb, false);
        key_input(*fs, item.filepath, hid_usb);
        delete hid_usb;
        hid_usb = nullptr;
    }
#endif
}

// ── Main options menu ────────────────────────────────────────────────────
void QuickAccessMenu::optionsMenu() {
    auto &qa = getQuickAccessManager();
    qa.load(); // re-read from LittleFS in case changed elsewhere

    options.clear();

    if (qa.count() == 0) {
        // Friendly empty-state hint
        options.push_back(
            {"(empty) Browse files to pin", []() { delay(500); }, false}
        );
    } else {
        for (size_t i = 0; i < qa.count(); i++) {
            const auto &item = qa.items()[i];

            // Build a short label: "label (type)" — trim to fit display
            String suffix;
            if (item.type == "ir")  suffix = " (IR)";
            else if (item.type == "sub") suffix = " (RF)";
            else if (item.type == "txt") suffix = " (BadUSB)";

            String displayName = item.label + suffix;

            options.push_back({displayName, [item]() { executePinnedItem(item); }});
        }
    }

    // Separator & management options
    if (qa.count() > 0) {
        options.push_back({"Manage Pins", [this]() { managePinsMenu(); }});
        options.push_back({"Clear All",   [&]() {
                               qa.clear();
                               displaySuccess("All pins cleared", true);
                           }});
    }

    options.push_back({"Browse & Pin", [&]() {
                           loopSD(LittleFS, false, ".ir,.sub,.txt");
                       }});
    addOptionToMainMenu();

    loopOptions(options, MENU_TYPE_SUBMENU, "Quick Access");
}

// ── Manage Pins sub-menu ─────────────────────────────────────────────────
void QuickAccessMenu::managePinsMenu() {
    auto &qa = getQuickAccessManager();

    while (true) {
        options.clear();

        for (size_t i = 0; i < qa.count(); i++) {
            const auto &item = qa.items()[i];

            options.push_back({item.label, [i, &qa]() {
                                   qa.unpin(i);
                                   displaySuccess("Pin removed", true);
                               }});
        }

        if (qa.count() == 0) {
            options.push_back({"(no pins)", []() { delay(500); }, false});
            break;
        }

        options.push_back({"Back", []() { returnToMenu = true; }});
        loopOptions(options);

        if (returnToMenu) {
            returnToMenu = false;
            break;
        }
    }
}

// ── Draw star icon ───────────────────────────────────────────────────────
void QuickAccessMenu::drawIcon(float scale) {
    clearIconArea();

    int r = scale * 28;        // outer radius
    int innerR = scale * 12;   // inner radius
    int cx = iconCenterX;
    int cy = iconCenterY;

    // 5-pointed star using 10 points (alternating outer/inner)
    constexpr int points = 10;
    float px[points], py[points];

    for (int i = 0; i < points; i++) {
        float angle = (i * 36.0 - 90.0) * DEG_TO_RAD; // start from top
        float radius = (i % 2 == 0) ? r : innerR;
        px[i] = cx + radius * cos(angle);
        py[i] = cy + radius * sin(angle);
    }

    // Draw filled star as a polygon via filled triangles from center
    for (int i = 0; i < points; i++) {
        int next = (i + 1) % points;
        tft.fillTriangle(cx, cy, px[i], py[i], px[next], py[next], bruceConfig.priColor);
    }

    // Draw the outline
    for (int i = 0; i < points; i++) {
        int next = (i + 1) % points;
        tft.drawLine(px[i], py[i], px[next], py[next], bruceConfig.priColor);
    }
}
