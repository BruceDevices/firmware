#if !defined(LITE_VERSION)
#include "wire_tools.h"

#include "core/bus_HAL.h"
#include "core/display.h"
#include <globals.h>
#include "core/mykeyboard.h"
#include "core/scrollableTextArea.h"
#include "core/settings.h"
#include "core/utils.h"
#include <SPI.h>

// Session state for the console UI (the serial commands pass everything explicitly)
static SPIClass *spiBus = nullptr;
static int8_t spiCsPin = -1;
static uint32_t spiFreq = 1000000;

/*********************************************************************
**  Helpers
**********************************************************************/
static String bytesToHex(const uint8_t *data, size_t len) {
    String out;
    for (size_t i = 0; i < len; i++) {
        if (i) out += " ";
        if (data[i] < 0x10) out += "0";
        out += String(data[i], HEX);
    }
    return out;
}

static int parseHexBytePairs(String hex, uint8_t *out, size_t maxLen) {
    hex.trim();
    hex.replace("0x", "");
    hex.replace("0X", "");
    hex.replace(" ", "");
    if (hex.isEmpty() || hex.length() % 2 != 0) return -1;
    size_t len = hex.length() / 2;
    if (len > maxLen) return -1;
    for (size_t i = 0; i < len; i++) out[i] = (uint8_t)strtol(hex.substring(i * 2, i * 2 + 2).c_str(), nullptr, 16);
    return (int)len;
}

static bool spi_resolve(int8_t &sck, int8_t &miso, int8_t &mosi, int8_t &cs) {
    BruceConfigPins::SPIPins pins = bruceConfigPins.outer_bus;
    if (sck < 0) sck = (int8_t)pins.sck;
    if (miso < 0) miso = (int8_t)pins.miso;
    if (mosi < 0) mosi = (int8_t)pins.mosi;
    if (cs < 0) cs = (int8_t)pins.cs;
    return sck >= 0 && miso >= 0 && mosi >= 0 && cs >= 0;
}

/*********************************************************************
**  Serial-command primitives (wire spi ...), see wire_tools.h
**********************************************************************/
bool wire_spi_begin(int8_t sck, int8_t miso, int8_t mosi, int8_t cs, uint32_t freqHz) {
    if (!spi_resolve(sck, miso, mosi, cs)) return false;
    if (freqHz < 1000 || freqHz > 40000000) return false;

    SPIClass *bus = acquireSPIBus((gpio_num_t)sck, (gpio_num_t)miso, (gpio_num_t)mosi);
    if (bus == nullptr) return false;

    spiBus = bus;
    spiCsPin = cs;
    spiFreq = freqHz;
    pinMode(cs, OUTPUT);
    digitalWrite(cs, HIGH);
    return true;
}

void wire_spi_end() {
    // The auxiliary SPI bus is shared (HAL tracks it); never end() it, just drop our handle
    spiBus = nullptr;
    spiCsPin = -1;
}

String wire_spi_transfer(const String &hexBytes) {
    if (spiBus == nullptr) return "ERR: bus not started";
    uint8_t tx[256];
    uint8_t rx[256];
    int len = parseHexBytePairs(hexBytes, tx, sizeof(tx));
    if (len <= 0) return "ERR: need hex byte pairs";

    spiBus->beginTransaction(SPISettings(spiFreq, MSBFIRST, SPI_MODE0));
    digitalWrite(spiCsPin, LOW);
    for (int i = 0; i < len; i++) rx[i] = spiBus->transfer(tx[i]);
    digitalWrite(spiCsPin, HIGH);
    spiBus->endTransaction();
    return bytesToHex(rx, len);
}

String wire_spi_flash_detect() {
    if (spiBus == nullptr) return "ERR: bus not started";
    uint8_t tx[4] = {0x9F, 0x00, 0x00, 0x00}; // JEDEC READ ID
    uint8_t rx[4];

    spiBus->beginTransaction(SPISettings(spiFreq, MSBFIRST, SPI_MODE0));
    digitalWrite(spiCsPin, LOW);
    for (int i = 0; i < 4; i++) rx[i] = spiBus->transfer(tx[i]);
    digitalWrite(spiCsPin, HIGH);
    spiBus->endTransaction();

    uint32_t jedec = ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
    if (jedec == 0 || jedec == 0xFFFFFF || (jedec & 0xFF) == 0) return "ERR: no flash detected";

    String manufacturer;
    switch (rx[1]) {
        case 0xEF: manufacturer = "Winbond";       break;
        case 0xC8: manufacturer = "GigaDevice";    break;
        case 0xC2: manufacturer = "Macronix";      break;
        case 0x20: manufacturer = "Micron";        break;
        case 0x01: manufacturer = "Spansion/Cyp."; break;
        case 0xBF: manufacturer = "SST";           break;
        case 0x1C: manufacturer = "EON";           break;
        case 0x85: manufacturer = "Puya";          break;
        case 0x9D: manufacturer = "ISSI";          break;
        case 0x5E: manufacturer = "Zbit";          break;
        case 0xA1: manufacturer = "Fudan";         break;
        default:   manufacturer = "Unknown";       break;
    }

    uint32_t capacityKb = (1UL << rx[3]) / 1024; // third JEDEC byte is log2(size in bytes)
    return "OK " + manufacturer + " ID:0x" + String(jedec, HEX) + " size:" + String(capacityKb) + "KB";
}

/*********************************************************************
**  Function: spi_pick_clock
**********************************************************************/
static void spi_pick_clock() {
    static const uint32_t clocks[] = {400000, 1000000, 4000000, 8000000, 16000000, 26000000};
    constexpr size_t N = sizeof(clocks) / sizeof(clocks[0]);
    int selected = 0;
    options.clear();
    for (size_t i = 0; i < N; i++) {
        if (clocks[i] == spiFreq) selected = i;
        options.push_back({String(clocks[i] / 1000000.0, 1) + " MHz",
                           [i]() { spiFreq = clocks[i]; }, clocks[i] == spiFreq});
    }
    loopOptions(options, MENU_TYPE_SUBMENU, "SPI Clock", selected);
}

/*********************************************************************
**  Function: spi_console
**  CS-framed HEX exchange loop: OK opens the HEX keyboard, response is
**  appended to a scrolling log. ESC leaves.
**********************************************************************/
static void spi_console() {
    drawMainBorderWithTitle("SPI Console");
    BruceConfigPins::SPIPins pins = bruceConfigPins.outer_bus;
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString(
        String(spiFreq / 1000000.0, 1) + "MHz  CS:" + int(spiCsPin) + "  SCK:" + int(pins.sck),
        BORDER_PAD_X, tft.getCursorY()
    );

    const int16_t areaY = tft.getCursorY() + LH + 2;
    ScrollableTextArea area(FP, BORDER_PAD_X, areaY, tftWidth - 2 * BORDER_PAD_X,
                            tftHeight - BORDER_PAD_X - areaY, false, false);
    area.addLine("OK = send HEX bytes");
    area.draw(true);

    while (!check(EscPress) && !returnToMenu) {
        wakeUpScreen();
        if (check(SelPress)) {
            String input = hex_keyboard("", 64, "Send HEX (cs framed):");
            if (input != "\x1B" && !input.isEmpty()) {
                area.addLine("TX " + input);
                String response = wire_spi_transfer(input);
                area.addLine(response.startsWith("ERR") ? response : "RX " + response);
                area.scrollToLine(area.getMaxLines());
                area.draw();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    check(SelPress);
}

/*********************************************************************
**  Function: spi_bus_setup
**********************************************************************/
void spi_bus_setup() {
    returnToMenu = false;
    while (!returnToMenu) {
        BruceConfigPins::SPIPins pins = bruceConfigPins.outer_bus;
        options = {
            {String("Pins: ") + int(pins.sck) + "/" + int(pins.miso) + "/" + int(pins.mosi) + "/CS" +
                 int(pins.cs),
             []() { setSPIPinsMenu(bruceConfigPins.outer_bus); }              },
            {String("Clock: ") + spiFreq / 1000000.0 + " MHz", spi_pick_clock   },
            {String("Detect Flash (JEDEC)"), []() {
                 if (!wire_spi_begin(-1, -1, -1, -1, spiFreq)) {
                     displayError("Set SPI pins first", true);
                     return;
                 }
                 String result = wire_spi_flash_detect();
                 if (result.startsWith("OK")) displayInfo(result, true);
                 else displayError(result.substring(5), true);
             }},
            {String("Console"),            []() {
                 if (!wire_spi_begin(-1, -1, -1, -1, spiFreq)) {
                     displayError("Set SPI pins first", true);
                     return;
                 }
                 spi_console();
             }},
        };
        addOptionToMainMenu();
        loopOptions(options, MENU_TYPE_SUBMENU, "SPI Bus");
    }
}
#endif
