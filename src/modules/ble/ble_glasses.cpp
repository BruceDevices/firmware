// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ble_glasses.h"

#include "ble_common.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <NimBLEDevice.h>

namespace {

// Confidence levels for a match.
constexpr uint8_t CONF_HIGH = 3;
constexpr uint8_t CONF_MED = 2;
constexpr uint8_t CONF_LOW = 1;

struct GlassesMatch {
    String name;
    String address;
    int rssi = 0;
    String vendor;
    uint8_t confidence = 0;
};

// Bluetooth SIG company (manufacturer) IDs used by smart-glasses makers.
struct CompanyEntry {
    uint16_t id;
    const char *label;
};
const CompanyEntry kCompanies[] = {
    {0x01AB, "Meta Platforms"},
    {0x058E, "Meta Technologies"},
    {0x0D53, "Luxottica (Ray-Ban/Oakley)"},
    {0x03C2, "Snap (Spectacles)"},
    {0x05D6, "Jieli (Rogbird/ViewPro)"},
};

// Plaintext product names broadcast during pairing (lowercased substrings).
const char *kNameHints[] = {
    "ray-ban", "rayban", "oakley", "spectacles", "heycyan", "visionpro",
    "meta view", "meta", "stories", "echo frames", "vuzix", "rokid",
    "xreal", "nreal", "smart glasses",
};

const char *companyLabel(uint16_t id) {
    for (const auto &c : kCompanies)
        if (c.id == id) return c.label;
    return nullptr;
}

// Lowercase copy of a std::string (ASCII only).
String lower(const std::string &s) {
    String out;
    out.reserve(s.size());
    for (unsigned char c : s)
        if (c >= 'A' && c <= 'Z') out += (char)(c + 32);
        else out += (char)c;
    return out;
}

bool contains(const String &hay, const char *needle) { return hay.indexOf(needle) >= 0; }

// Truncate a string to fit `maxChars`, appending "..." when clipped.
String fit(const String &s, int maxChars) {
    if (maxChars < 4) return s.substring(0, maxChars);
    if ((int)s.length() <= maxChars) return s;
    return s.substring(0, maxChars - 3) + "...";
}

// Try to match a single advertised device as smart glasses.
bool matchDevice(const NimBLEAdvertisedDevice *dev, GlassesMatch &out) {
    if (!dev) return false;

    const String name = lower(dev->getName());
    uint16_t company = 0;
    bool hasCompany = false;

    if (dev->haveManufacturerData()) {
        const std::string md = dev->getManufacturerData();
        if (md.size() >= 2) {
            company = (uint8_t)md[0] | ((uint8_t)md[1] << 8);
            hasCompany = true;
            // Meta camera glasses embed an ASCII tag right after the company id.
            const String tag = lower(md.substr(2));
            if (contains(tag, "meta_rb_glass")) {
                out.vendor = "Meta Ray-Ban";
                out.confidence = CONF_HIGH;
            }
        }
    }

    // Highest confidence: a registered smart-glasses company ID.
    if (out.confidence < CONF_HIGH && hasCompany) {
        const char *label = companyLabel(company);
        if (label) {
            out.vendor = label;
            out.confidence = CONF_HIGH;
        }
    }

    // Fixed service UUIDs (Google/Eddystone on Meta frames, HeyCyan SDK).
    if (out.confidence < CONF_HIGH) {
        if (dev->isAdvertisingService(NimBLEUUID("FEAA")) ||
            dev->isAdvertisingService(NimBLEUUID("7905fff0-b5ce-4e99-a40f-4b1e122d00d0"))) {
            out.vendor = "Smart glasses (UUID)";
            out.confidence = CONF_MED;
        }
    }

    // Plaintext name match (easy to spoof, so lower confidence).
    if (out.confidence < CONF_MED) {
        for (const char *hint : kNameHints) {
            if (contains(name, hint)) {
                out.vendor = String("Name: ") + hint;
                out.confidence = CONF_LOW;
                break;
            }
        }
    }

    if (out.confidence == 0) return false;

    out.name = dev->getName().empty() ? String("<no name>") : String(dev->getName().c_str());
    out.address = dev->getAddress().toString().c_str();
    out.rssi = dev->getRSSI();
    return true;
}

const char *confLabel(uint8_t c) {
    if (c >= CONF_HIGH) return "HIGH";
    if (c >= CONF_MED) return "MED";
    return "LOW";
}

int textMaxChars() {
    const int w = tftWidth - 2 * BORDER_PAD_X;
    const int cw = LW * FP;
    return (w / cw) - 1;
}

void drawResult(const std::vector<GlassesMatch> &matches, bool scanning) {
    drawMainBorderWithTitle("Glasses Detector");
    const int x = BORDER_PAD_X;
    const int lh = LH * FP + 6;
    int y = BORDER_PAD_Y + FM * LH + 6;
    const int maxChars = textMaxChars();
    tft.setTextSize(FP);

    if (matches.empty()) {
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawString(scanning ? "Scanning..." : "Scanning... none found", x, y, 1);
    } else {
        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        tft.drawString(fit(String("! ") + matches.size() + " possible glasses", maxChars), x, y, 1);
        y += lh + 2;

        const size_t max = matches.size() > 5 ? 5 : matches.size();
        for (size_t i = 0; i < max; i++) {
            const auto &m = matches[i];
            if (y + lh > tftHeight - 14) break;
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.drawString(fit(m.name, maxChars), x, y, 1);
            y += lh;
            tft.setTextColor(blendColors(bruceConfig.bgColor, bruceConfig.priColor, 170), bruceConfig.bgColor);
            tft.drawString(
                fit(String(m.vendor) + "  " + m.rssi + "dBm [" + confLabel(m.confidence) + "]", maxChars),
                x, y, 1
            );
            y += lh + 2;
        }
    }

    tft.setTextColor(blendColors(bruceConfig.bgColor, bruceConfig.priColor, 150), bruceConfig.bgColor);
    tft.drawString(fit("[SEL] menu  [ESC] back", maxChars), x, tftHeight - LH * FP - 8, 1);
}

} // namespace

void ble_glasses_detector() {
#if !defined(LITE_VERSION)
    if (!ble_scan_setup() || pBLEScan == nullptr) {
        displayError("Failed to init BLE scan");
        return;
    }

    // No-PSRAM board: keep the retained device set small. Duplicate reports are
    // irrelevant for detection and are the main source of RAM churn.
    pBLEScan->setDuplicateFilter(true);

    bool exit = false;
    std::vector<GlassesMatch> matches;

    while (!exit) {
        // One full scan (same flow as ble_scan), then wait for input instead of
        // tight-looping the radio — that's what exhausted the heap before.
        matches.clear();
        drawResult(matches, true); // "Scanning..."

        pBLEScan->clearResults();
        BLEScanResults found = pBLEScan->getResults(scanTime * 1000, false);

        const int count = found.getCount();
        for (int i = 0; i < count; i++) {
            GlassesMatch m;
            if (matchDevice(found.getDevice(i), m)) matches.push_back(m);
        }

        drawResult(matches, false);
        if (!matches.empty() && bruceConfigPins.buzzer > 0)
            tone((uint8_t)bruceConfigPins.buzzer, 2000, 80);

        // Block on user input so we don't keep allocating scan results.
        bool rescan = false;
        while (!rescan && !exit) {
            if (check(SelPress)) {
                int action = 0;
                options = {
                    {"Rescan",       [&]() { action = 0; }},
                    {"Back to menu", [&]() { action = 1; }},
                };
                loopOptions(options, MENU_TYPE_SUBMENU, "Glasses Detector");
                options.clear();
                if (action == 1) exit = true;
                else rescan = true;
            }
            if (check(EscPress)) exit = true;
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    if (pBLEScan) pBLEScan->stop();
    stopBLEStack();
    returnToMenu = true;
#else
    displayError("Not available on Launcher version");
#endif
}
