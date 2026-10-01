#include "theme.h"
#include "core/led_control.h"
#include "display.h"

// Colors in a theme file are hex strings ("ffff"), but hand-written themes often use plain numbers.
// strtoul(nullptr) on a non-string value used to crash (reboot) the board.
static uint32_t themeColorValue(JsonVariant v, uint32_t fallback) {
    if (v.is<const char *>()) {
        const char *str = v.as<const char *>();
        if (str != nullptr) return strtoul(str, nullptr, 16);
    } else if (v.is<long>()) {
        return (uint32_t)v.as<long>();
    }
    return fallback;
}

struct ThemeEntry {
    const char *key;
    bool *flag;
    String &path;
};

void BruceTheme::removeTheme(void) {
    themeInfo t;
    theme = t;
}
FS *BruceTheme::themeFS(void) {
    if (theme.fs == 1) return &LittleFS;
    else if (theme.fs == 2) return &SD;
    return &LittleFS; // always get back to safety
}
bool BruceTheme::openThemeFile(FS *fs, String filepath, bool overwriteConfigSettings) {

    if (fs == nullptr) return true;
    // Nothing selected (boot without theme / file picker cancelled): keep the current theme untouched
    if (filepath.isEmpty()) return false;
    if (!fs->exists(filepath)) return false;
    File file;
    file = fs->open(filepath, FILE_READ);
    if (!file) {
        log_e("THEME: %s. Using default theme", "Theme file not found");
        removeTheme();
        return false;
    }

    // Deserialize the JSON document, then release the file handle right away: the PNG pre-cache below
    // opens more files on the SD card (only a few handles are available on SD_MMC)
    JsonDocument jsonDoc;
    bool parseFailed = (bool)deserializeJson(jsonDoc, file);
    file.close();
    if (parseFailed) {
        displayError("5", true);
        log_e("THEME: %s. Using default theme", "Failed reading theme file");
        removeTheme();
        return false;
    }
    themePath = filepath;
    String baseThemePath = filepath.substring(0, filepath.lastIndexOf('/')) + "/";

    ThemeEntry entries[] = {
        {"wifi",        &theme.wifi,        theme.paths.wifi       },
        {"ble",         &theme.ble,         theme.paths.ble        },
        {"ethernet",    &theme.ethernet,    theme.paths.ethernet   },
        {"rf",          &theme.rf,          theme.paths.rf         },
        {"rfid",        &theme.rfid,        theme.paths.rfid       },
        {"fm",          &theme.fm,          theme.paths.fm         },
        {"ir",          &theme.ir,          theme.paths.ir         },
        {"files",       &theme.files,       theme.paths.files      },
        {"gps",         &theme.gps,         theme.paths.gps        },
        {"nrf",         &theme.nrf,         theme.paths.nrf        },
        {"interpreter", &theme.interpreter, theme.paths.interpreter},
        {"clock",       &theme.clock,       theme.paths.clock      },
        {"others",      &theme.others,      theme.paths.others     },
        {"connect",     &theme.connect,     theme.paths.connect    },
        {"config",      &theme.config,      theme.paths.config     },
        {"boot_img",    &theme.boot_img,    theme.paths.boot_img   },
        {"boot_sound",  &theme.boot_sound,  theme.paths.boot_sound },
        {"lora",        &theme.lora,        theme.paths.lora       }
    };

    JsonObject _th = jsonDoc.as<JsonObject>();
    for (auto &entry : entries) {
        if (!_th[entry.key].isNull()) {
            String path = baseThemePath + _th[entry.key].as<String>();
            if (fs->exists(path)) {
                *entry.flag = true;
                entry.path = path;
                // Pre-cache PNGs into BIN files to avoid runtime decoding and allocations
                if (path.endsWith(".png") || path.endsWith(".PNG")) { preparePngBin(*fs, path); }
            } else {
                log_w("THEME: file not found: %s", entry.key);
            }
        }
    }

    if (!_th["border"].isNull()) { theme.border = _th["border"].as<int>(); }
    if (!_th["label"].isNull()) { theme.label = _th["label"].as<int>(); }
    if (!_th["gifDuration"].isNull()) { theme.gifDuration = _th["gifDuration"].as<int>(); }

    if (overwriteConfigSettings) {
        uint16_t _priColor = bruceConfig.priColor;
        uint16_t _secColor = bruceConfig.secColor;
        uint16_t _bgColor = bruceConfig.bgColor;

        _priColor = (uint16_t)themeColorValue(_th["priColor"].as<JsonVariant>(), _priColor);
        _secColor = (uint16_t)themeColorValue(_th["secColor"].as<JsonVariant>(), _secColor);
        _bgColor = (uint16_t)themeColorValue(_th["bgColor"].as<JsonVariant>(), _bgColor);
        _setUiColor(_priColor, &_secColor, &_bgColor);

#ifdef HAS_RGB_LED
        if (!_th["ledBright"].isNull()) { bruceConfig.ledBright = _th["ledBright"].as<int>(); }
        if (!_th["ledColor"].isNull()) {
            bruceConfig.ledColor = themeColorValue(_th["ledColor"].as<JsonVariant>(), bruceConfig.ledColor);
        }
        if (!_th["ledEffect"].isNull()) { bruceConfig.ledEffect = _th["ledEffect"].as<int>(); }
        if (!_th["ledEffectSpeed"].isNull()) { bruceConfig.ledEffectSpeed = _th["ledEffectSpeed"].as<int>(); }
        if (!_th["ledEffectDirection"].isNull()) {
            bruceConfig.ledEffectDirection = _th["ledEffectDirection"].as<int>();
        }
        ledSetup();
#endif
    }

    if (fs == &LittleFS) theme.fs = 1;
    else if (fs == &SD) theme.fs = 2;
    else theme.fs = 0;

    return true;
}

bool BruceTheme::validateImgFile(FS *fs, String filepath) {
    // Validate theme image files
    // Check if file exists
    if (fs == nullptr || filepath.isEmpty()) return false;
    if (!fs->exists(filepath)) return false;
    
    // Check file is not empty
    File file = fs->open(filepath, FILE_READ);
    if (!file) return false;
    
    size_t fileSize = file.size();
    file.close();
    
    // Reject obviously invalid files (too small to be valid images)
    if (fileSize < 64) return false;
    
    // Reject unreasonably large files (likely not intended as menu icons)
    // Menu icons should be small - 500KB is plenty for even high-res icons
    const size_t MAX_ICON_SIZE = 500 * 1024;
    if (fileSize > MAX_ICON_SIZE) return false;
    
    // For more advanced validation (checking actual image dimensions), 
    // we would need to decode the image. For now, basic checks above.
    return true;
}

void BruceTheme::_setUiColor(uint16_t primary, uint16_t *secondary, uint16_t *background) {
    priColor = primary;
    secColor = secondary == nullptr ? primary - 0x2000 : *secondary;
    bgColor = background == nullptr ? 0x0 : *background;
}
