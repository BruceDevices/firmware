#ifndef LITE_VERSION
#include "ibutton.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static OneWire *oneWire = nullptr;
static byte keyBuffer[8];
static bool keyLoaded = false;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static String bufferToHexStr(const byte *buf, int len, const char *sep = ":") {
    String s;
    for (int i = 0; i < len; i++) {
        if (buf[i] < 0x10) s += '0';
        s += String(buf[i], HEX);
        if (i < len - 1) s += sep;
    }
    s.toUpperCase();
    return s;
}

static bool bufferCrcValid() { return OneWire::crc8(keyBuffer, 7) == keyBuffer[7]; }

static void displayStatus() {
    drawMainBorderWithTitle("iButton");
    if (keyLoaded) {
        padprintln("UID: " + bufferToHexStr(keyBuffer, 8));
        if (!bufferCrcValid()) {
            tft.setTextColor(TFT_RED);
            padprintln("CRC ERROR!");
            tft.setTextColor(bruceConfig.priColor);
        } else {
            tft.setTextColor(TFT_GREEN);
            padprintln("CRC OK");
            tft.setTextColor(bruceConfig.priColor);
        }
    } else {
        padprintln("No key in buffer");
    }
    padprintln("");
    padprintln("Waiting for iButton...");
    padprintln("[NEXT] for options");
}

// ---------------------------------------------------------------------------
// OneWire RW1990 low-level
// ---------------------------------------------------------------------------

static void writeByte_RW1990(byte data) {
    uint8_t pin = bruceConfigPins.iButton;
    for (int bit = 0; bit < 8; bit++) {
        if (data & 1) {
            digitalWrite(pin, LOW);
            pinMode(pin, OUTPUT);
            delayMicroseconds(60);
            pinMode(pin, INPUT);
            digitalWrite(pin, HIGH);
        } else {
            digitalWrite(pin, LOW);
            pinMode(pin, OUTPUT);
            pinMode(pin, INPUT);
            digitalWrite(pin, HIGH);
        }
        delay(10);
        data >>= 1;
    }
}

// ---------------------------------------------------------------------------
// Core operations
// ---------------------------------------------------------------------------

static bool readKey(int maxRetries = 20) {
    for (int attempt = 0; attempt < maxRetries; attempt++) {
        if (check(EscPress)) return false;
        if (oneWire->reset() == 0) return false;

        oneWire->write(0x33);
        oneWire->read_bytes(keyBuffer, 8);

        if (bufferCrcValid()) {
            keyLoaded = true;
            return true;
        }

        tft.fillScreen(bruceConfig.bgColor);
        drawMainBorderWithTitle("iButton");
        padprintln("Reading... attempt " + String(attempt + 2));
        padprintln("CRC mismatch, retrying...");
        padprintln("");
        padprintln("Keep key on the contact!");
        delay(100);
    }

    // Retries exhausted — still store what we got
    keyLoaded = true;
    return false;
}

static void writeKey() {
    uint8_t pin = bruceConfigPins.iButton;

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0x33);

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0x3C);
    delay(50);

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0xD1);
    delay(50);

    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    delayMicroseconds(60);
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);
    delay(10);

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0xD5);
    delay(50);

    for (byte i = 0; i < 8; i++) {
        writeByte_RW1990(keyBuffer[i]);
        delayMicroseconds(25);
    }

    oneWire->reset();
    oneWire->skip();
    oneWire->write(0xD1);
    delayMicroseconds(16);
    oneWire->reset();
}

// ---------------------------------------------------------------------------
// File I/O
// ---------------------------------------------------------------------------

static const char *IBUTTON_DIR = "/BruceIButton";

static IButtonResult saveKey() {
    FS *fs;
    if (!getFsStorage(fs)) return IBUTTON_FAILURE;
    if (!fs->exists(IBUTTON_DIR)) fs->mkdir(IBUTTON_DIR);

    char fname[48];
    snprintf(
        fname,
        sizeof(fname),
        "%s/%02X%02X%02X%02X%02X%02X.ibtn",
        IBUTTON_DIR,
        keyBuffer[0],
        keyBuffer[1],
        keyBuffer[2],
        keyBuffer[3],
        keyBuffer[4],
        keyBuffer[5]
    );

    String path = fname;
    if (fs->exists(path)) {
        int n = 1;
        String base = path.substring(0, path.lastIndexOf('.'));
        do { path = base + "_" + String(n++) + ".ibtn"; } while (fs->exists(path));
    }

    File file = fs->open(path, FILE_WRITE);
    if (!file) return IBUTTON_FAILURE;

    file.println("Filetype: Bruce iButton File");
    file.println("Version 1");
    file.println("Device type: DS1990A");
    file.print("UID: ");
    file.println(bufferToHexStr(keyBuffer, 8, " "));
    file.print("CRC: ");
    if (keyBuffer[7] < 0x10) file.print('0');
    file.println(keyBuffer[7], HEX);

    file.close();
    return IBUTTON_SUCCESS;
}

static IButtonResult loadKey() {
    FS *fs;
    if (!getFsStorage(fs)) return IBUTTON_FAILURE;

    String filepath = loopSD(*fs, true, "ibtn", IBUTTON_DIR);
    if (filepath.length() == 0) return IBUTTON_FAILURE;

    File file = fs->open(filepath, FILE_READ);
    if (!file) return IBUTTON_FAILURE;

    byte tmp[8];
    bool found = false;

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (!line.startsWith("UID:")) continue;

        String hex = line.substring(line.indexOf(':') + 1);
        hex.trim();
        hex.replace(" ", "");
        if (hex.length() != 16) continue;

        for (int i = 0; i < 8; i++) tmp[i] = strtoul(hex.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
        found = true;
        break;
    }
    file.close();

    if (!found) return IBUTTON_FAILURE;

    memcpy(keyBuffer, tmp, 8);
    keyLoaded = true;
    return bufferCrcValid() ? IBUTTON_SUCCESS : IBUTTON_CRC_ERROR;
}

// ---------------------------------------------------------------------------
// Menu actions
// ---------------------------------------------------------------------------

enum MenuAction {
    ACTION_WRITE,
    ACTION_SAVE,
    ACTION_LOAD,
    ACTION_RESET,
    ACTION_SETUP_PIN,
    ACTION_CLOSE,
    ACTION_MAIN_MENU,
};

static bool returnFromMenu = false;
static bool restartNeeded = false;
static MenuAction selectedAction;

static void setAction(MenuAction a) { selectedAction = a; }

static void doWrite() {
    if (!keyLoaded) {
        displayError("No key in buffer", true);
        delay(1500);
        return;
    }

    tft.fillScreen(bruceConfig.bgColor);
    drawMainBorderWithTitle("Write iButton");
    padprintln("UID: " + bufferToHexStr(keyBuffer, 8));
    padprintln("");
    padprintln("Touch blank key to writer...");
    padprintln("[Esc] Cancel");

    while (oneWire->reset() == 0) {
        if (check(EscPress)) return;
        delay(50);
    }

    writeKey();
    displaySuccess("Key written!");
    delay(1500);
}

static void doSave() {
    if (!keyLoaded) {
        displayError("No key in buffer", true);
        delay(1500);
        return;
    }
    IButtonResult r = saveKey();
    if (r == IBUTTON_SUCCESS) displaySuccess("Saved");
    else displayError("Save failed", true);
    delay(1500);
}

static void doLoad() {
    IButtonResult r = loadKey();
    if (r == IBUTTON_SUCCESS) {
        displaySuccess("Key loaded");
        delay(1000);
    } else if (r == IBUTTON_CRC_ERROR) {
        displayWarning("Loaded (CRC mismatch)", true);
        delay(1500);
    } else {
        displayError("Load failed", true);
        delay(1500);
    }
}

static void doReset() {
    memset(keyBuffer, 0, sizeof(keyBuffer));
    keyLoaded = false;
    displaySuccess("Buffer cleared");
    delay(1000);
}

static void selectMenuOption() {
    options = {};

    if (keyLoaded) {
        options.emplace_back("Write to key", []() { setAction(ACTION_WRITE); });
        options.emplace_back("Save to file", []() { setAction(ACTION_SAVE); });
    }

    options.emplace_back("Load from file", []() { setAction(ACTION_LOAD); });

    if (keyLoaded) {
        options.emplace_back("Reset buffer", []() { setAction(ACTION_RESET); });
    }

    options.emplace_back("Setup pin", []() { setAction(ACTION_SETUP_PIN); });
    options.emplace_back("Close menu", []() { setAction(ACTION_CLOSE); });
    options.emplace_back("Main menu", []() { setAction(ACTION_MAIN_MENU); });

    selectedAction = ACTION_CLOSE;
    loopOptions(options);
    options.clear();

    switch (selectedAction) {
        case ACTION_WRITE: doWrite(); break;
        case ACTION_SAVE: doSave(); break;
        case ACTION_LOAD: doLoad(); break;
        case ACTION_RESET: doReset(); break;
        case ACTION_SETUP_PIN: restartNeeded = true; return;
        case ACTION_MAIN_MENU: returnFromMenu = true; return;
        case ACTION_CLOSE: break;
    }
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void setup_ibutton() {
    returnFromMenu = false;
    restartNeeded = false;
    keyLoaded = false;

Restart:
    if (oneWire) delete oneWire;
    oneWire = new OneWire(bruceConfigPins.iButton);

    tft.fillScreen(bruceConfig.bgColor);
    displayStatus();

    for (;;) {
        if (check(EscPress) || returnFromMenu) {
            returnToMenu = true;
            break;
        }

        if (check(NextPress)) {
            selectMenuOption();
            if (returnFromMenu) {
                returnToMenu = true;
                break;
            }
            if (restartNeeded) {
                restartNeeded = false;
                setiButtonPinMenu();
                goto Restart;
            }
            tft.fillScreen(bruceConfig.bgColor);
            displayStatus();
        }

        // Auto-read when iButton touches the probe
        if (oneWire->reset() != 0) {
            bool ok = readKey();
            tft.fillScreen(bruceConfig.bgColor);

            if (ok) {
                displayStatus();
                padprintln("");
                tft.setTextColor(TFT_GREEN);
                padprintln("Key read successfully!");
                tft.setTextColor(bruceConfig.priColor);
            } else if (keyLoaded) {
                // Read failed CRC but we have data
                displayStatus();
                padprintln("");
                tft.setTextColor(TFT_YELLOW);
                padprintln("Read with CRC errors");
                tft.setTextColor(bruceConfig.priColor);
            } else {
                displayStatus();
            }

            // Wait for key removal to avoid re-reading in a loop
            while (oneWire->reset() != 0) {
                if (check(EscPress)) break;
                delay(100);
            }
        }

        delay(50);
    }

    delete oneWire;
    oneWire = nullptr;
}

// ---------------------------------------------------------------------------
// Pin selection menu
// ---------------------------------------------------------------------------

void setiButtonPinMenu() {
    options = {};
    gpio_num_t sel = GPIO_NUM_NC;
    for (int8_t i = -1; i <= GPIO_NUM_MAX; i++) {
        String label = "GPIO " + String(i);
        options.push_back({label.c_str(), [i, &sel]() { sel = (gpio_num_t)i; }});
    }
    loopOptions(options, bruceConfigPins.iButton + 1);
    options.clear();
    bruceConfigPins.setiButtonPin(sel);
}

#endif
