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

#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "modules/bjs_interpreter/interpreter.h"
#endif

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
    // Defensive: block path traversal
    if (item.filepath.indexOf("..") >= 0) {
        displayError("Invalid path", true);
        return;
    }

    FS *fs = resolveFs(item);
    if (!fs) {
        displayError("SD not mounted", true);
        return;
    }

    if (!fs->exists(item.filepath)) {
        displayError("File not found", true);
        // Offer to remove the stale pin
        options = {
            {"Remove Pin",
             [&]() {
                 getQuickAccessManager().unpin(item.filepath);
                 displaySuccess("Pin removed");
                 delay(1200);
             }                                           },
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
        if (readSubFile(fs, item.filepath, data)) { txSubFile(data); }
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
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    else if (item.type == "js") {
        delay(200);
        run_bjs_script_headless(*fs, item.filepath);
    }
#endif
}

// ── Main options menu ────────────────────────────────────────────────────
void QuickAccessMenu::optionsMenu() {
    auto &qa = getQuickAccessManager();
    qa.load(); // re-read from LittleFS in case changed elsewhere

    options.clear();

    for (size_t i = 0; i < qa.count(); i++) {
        const auto &item = qa.items()[i];

        // Build a short label: "label (type)" — trim to fit display
        String suffix;
        if (item.type == "ir") suffix = " (IR)";
        else if (item.type == "sub") suffix = " (RF)";
        else if (item.type == "txt") suffix = " (BadUSB)";
        else if (item.type == "js") suffix = " (JS)";

        String displayName = item.label + suffix;

        options.push_back({displayName, [item]() { executePinnedItem(item); }});
    }

    // Management options (always shown, even when empty)
    if (qa.count() > 0) {
        options.push_back({"Manage Quick Access", [this]() { managePinsMenu(); }});
        options.push_back({"Clear All", [&]() {
                               options = {
                                   {"Cancel", []() { /* do nothing */ }},
                                   {"Clear All", [&]() {
                                        qa.clear();
                                        displaySuccess("All pins cleared");
                                        delay(1200);
                                    }},
                               };
                               loopOptions(options, MENU_TYPE_SUBMENU, "Clear all pins?");
                           }});
    }

    options.push_back({"Add Files", [this]() { addFilesMenu(); }});
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

            options.push_back({item.label, [i, &qa, item]() {
                                   options = {
                                       {"Cancel", []() { /* do nothing */ }},
                                       {"Remove", [i, &qa]() {
                                            qa.unpin(i);
                                            displaySuccess("Pin removed");
                                            delay(1200);
                                        }},
                                   };
                                   String msg = "Remove " + item.label + "?";
                                   loopOptions(options, MENU_TYPE_SUBMENU, msg.c_str());
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

// ── Helper: open picker and pin selected file ────────────────────────────
static void pinFromFilePicker(FS &fs, uint8_t fsType) {
    String filepath = loopSD(fs, true, "ir|sub|txt|js|bjs", "/");
    if (filepath == "") return; // user pressed back

    // Extract filename for the label
    String filename = filepath.substring(filepath.lastIndexOf('/') + 1);
    // Determine type from extension
    String type;
    if (filepath.endsWith(".ir")) type = "ir";
    else if (filepath.endsWith(".sub")) type = "sub";
    else if (filepath.endsWith(".txt")) type = "txt";
    else if (filepath.endsWith(".js") || filepath.endsWith(".bjs")) type = "js";
    else type = "";

    if (type == "") {
        displayError("Unsupported file type", true);
        return;
    }

    // Proactive max-pin check
    if (getQuickAccessManager().count() >= QuickAccessManager::MAX_ITEMS) {
        displayError("Max 15 pins reached", true);
        return;
    }

    PinnedItem item;
    item.filepath = filepath;
    item.label = filename.substring(0, filename.lastIndexOf('.'));
    item.type = type;
    item.fsType = fsType;

    auto &qa = getQuickAccessManager();
    if (qa.pin(item)) {
        displaySuccess("Pinned: " + item.label);
        delay(1200);
    } else {
        if (qa.isPinned(filepath)) displayError("Already pinned", true);
        else displayError("Pin failed (max 15)", true);
    }
}

// ── Add Files menu: pick LittleFS or SD ──────────────────────────────────
void QuickAccessMenu::addFilesMenu() {
    if (getQuickAccessManager().count() >= QuickAccessManager::MAX_ITEMS) {
        displayError("Max 15 pins reached", true);
        return;
    }

    options = {
        {"LittleFS", [&]() { pinFromFilePicker(LittleFS, 0); }},
    };

    if (sdcardMounted) {
        options.push_back({"SD Card", [&]() { pinFromFilePicker(SD, 1); }});
    }

    options.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Select Source");
}

// ── Draw star icon ───────────────────────────────────────────────────────
void QuickAccessMenu::drawIcon(float scale) {
    clearIconArea();

    int r = scale * 28;      // outer radius
    int innerR = scale * 12; // inner radius
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
