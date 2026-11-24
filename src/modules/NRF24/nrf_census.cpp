#include "nrf_census.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "modules/rf/rf_ui_shim.h"
#include "nrf_capture.h"
#include "nrf_common.h"

#include <RF24.h>
#include <globals.h>

// ---------------------------------------------------------------------------
// 2.4 GHz census (task 2.2)
//
// 126 channels x N addresses, one short listen window per combination. The
// counters saturate rather than wrap: a busy address is still "a lot" at 65535,
// and a wrapped count would read as a quiet channel.
//
// The address list is the module defaults every hobby nRF24 sketch uses plus
// the RGB-controller syncwords this firmware already knows (NRF_SIGNAL_PROFILES
// in nrf_capture.cpp). Anything on a different address is invisible - that is
// the technique's limit, printed on the page.
// ---------------------------------------------------------------------------

#define NRF_CENSUS_CHANNELS 126
#define NRF_CENSUS_LISTEN_US 900
#define NRF_CENSUS_PAYLOAD 32

struct CensusAddr {
    const char *name;
    uint8_t bytes[5];
};

static const CensusAddr CENSUS_ADDRS[] = {
    {"1Node", {0x31, 0x4E, 0x6F, 0x64, 0x65}},
    {"2Node", {0x32, 0x4E, 0x6F, 0x64, 0x65}},
    {"3Node", {0x33, 0x4E, 0x6F, 0x64, 0x65}},
    {"NRF24", {0x4E, 0x52, 0x46, 0x32, 0x34}},
    {"RF24 ", {0x52, 0x46, 0x32, 0x34, 0x20}},
    {"12345", {0x31, 0x32, 0x33, 0x34, 0x35}},
    {"Mi.RGBW", {0x4A, 0x1A, 0x8D, 0xE2, 0x55}},
    {"Mi.CCT ", {0xAA, 0x5A, 0x05, 0x0A, 0x55}},
    {"Mi.CCT2", {0x8A, 0x01, 0xE9, 0xC4, 0x56}},
    {"Mi.RGB ", {0xD5, 0x33, 0x9B, 0x55, 0xAD}},
    {"Mi.F20 ", {0x55, 0xA5, 0xAA, 0x50, 0x50}},
};
#define CENSUS_ADDR_COUNT ((int)(sizeof(CENSUS_ADDRS) / sizeof(CENSUS_ADDRS[0])))

// 126 * 2 + 11 * 2 bytes of counters: small enough for .bss on every board, and
// the page is the only thing that touches them.
static uint16_t censusChan[NRF_CENSUS_CHANNELS];
static uint16_t censusAddr[CENSUS_ADDR_COUNT];
static uint8_t censusLast[32]; // last payload heard, shown as hex
static int8_t censusLastAddr = -1;
static int8_t censusLastChan = -1;
static uint32_t censusPasses = 0;
static uint32_t censusTotal = 0;

static void censusReset() {
    memset(censusChan, 0, sizeof(censusChan));
    memset(censusAddr, 0, sizeof(censusAddr));
    memset(censusLast, 0, sizeof(censusLast));
    censusLastAddr = -1;
    censusLastChan = -1;
    censusPasses = 0;
    censusTotal = 0;
}

static void censusBump(uint16_t &v) {
    if (v < 0xFFFF) v++;
}

static bool censusSave() {
    if (!sdcardMounted) return false;
    if (!SD.exists("/BruceRF")) SD.mkdir("/BruceRF");
    File f = SD.open("/BruceRF/nrf_census.csv", FILE_WRITE);
    if (!f) return false;
    f.printf("# Vandeta nRF24 census - %u passes, %u packets\n", (unsigned)censusPasses, (unsigned)censusTotal);
    f.println("channel,hits");
    for (int ch = 0; ch < NRF_CENSUS_CHANNELS; ch++)
        if (censusChan[ch]) f.printf("%d,%u\n", ch, (unsigned)censusChan[ch]);
    f.println("address,hits");
    for (int i = 0; i < CENSUS_ADDR_COUNT; i++)
        if (censusAddr[i]) f.printf("%s,%u\n", CENSUS_ADDRS[i].name, (unsigned)censusAddr[i]);
    f.close();
    return true;
}

static void censusDraw(bool saved) {
    drawMainBorder(false);
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.setCursor(10, 26);
    tft.print("2.4G CENSUS");
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.drawRightString(String(censusTotal) + " pkt " + String((unsigned)censusPasses) + "x", tftWidth - 10, 26, 1);
    tft.drawFastHLine(10, 38, tftWidth - 20, bruceConfig.secColor);

    // Channel occupancy: 2 px per channel, height grows with the hit count.
    // A logarithmic-ish scale (one step per hit, capped) keeps a single packet
    // visible instead of being lost next to a busy channel.
    const int baseY = tftHeight - 62;
    const int maxH = 46;
    for (int ch = 0; ch < NRF_CENSUS_CHANNELS; ch++) {
        const int x = 8 + ch * 2;
        if (x + 1 > tftWidth - 8) break;
        const uint16_t hits = censusChan[ch];
        if (hits == 0) continue;
        int h = 4 + (int)hits * 3;
        if (h > maxH) h = maxH;
        tft.fillRect(x, baseY - h, 2, h, uiGradientColorAt(x));
    }
    tft.drawFastHLine(8, baseY, tftWidth - 16, bruceConfig.secColor);
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.setCursor(8, baseY + 2);
    tft.print("ch0");
    tft.drawCentreString("ch63", tftWidth / 2, baseY + 2, 1);
    tft.drawRightString("ch125", tftWidth - 8, baseY + 2, 1);

    // Top talkers: the addresses that actually answered, most first.
    int order[CENSUS_ADDR_COUNT];
    for (int i = 0; i < CENSUS_ADDR_COUNT; i++) order[i] = i;
    for (int i = 1; i < CENSUS_ADDR_COUNT; i++) {
        int key = order[i], j = i;
        while (j > 0 && censusAddr[order[j - 1]] < censusAddr[key]) {
            order[j] = order[j - 1];
            j--;
        }
        order[j] = key;
    }
    int y = 44;
    int shown = 0;
    for (int i = 0; i < CENSUS_ADDR_COUNT && shown < 3; i++) {
        const int idx = order[i];
        if (censusAddr[idx] == 0) break;
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setCursor(10, y);
        tft.print(String(CENSUS_ADDRS[idx].name) + "  " + String((unsigned)censusAddr[idx]) + " pkt");
        y += 11;
        shown++;
    }
    if (shown == 0) {
        tft.setCursor(10, 44);
        tft.print("nothing on the known addresses yet");
    }

    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.setCursor(10, tftHeight - 22);
    tft.print("only the listed addresses are heard");
    tft.setCursor(10, tftHeight - 10);
    tft.print(saved ? "saved to /BruceRF/nrf_census.csv" : "SEL save csv   BACK exit");
}

void nrf_census() {
    if (!nrf_start(NRF_MODE_SPI)) {
        displayError("No NRF24 on SPI", true);
        return;
    }

    censusReset();
    NRFradio.setAutoAck(false);
    NRFradio.setDataRate(RF24_1MBPS);
    NRFradio.setAddressWidth(5);
    NRFradio.setPayloadSize(NRF_CENSUS_PAYLOAD);

    while (check(SelPress) || check(EscPress)) vTaskDelay(pdMS_TO_TICKS(10));
    delay(150); // the key that opened the page is still held (see check())

    // Own page, not an overlay: censusDraw() only calls drawMainBorder(false),
    // so without this the NRF24 menu stayed visible behind the graph.
    drawMainBorder(true);

    bool saved = false;
    uint32_t lastDraw = 0;
    for (;;) {
        if (check(EscPress)) break;

        // BACK pressed inside the channel sweep used to break only the inner
        // loop - and it consumed the flag on the way, so the outer check never
        // saw it and the page could not be left at all (the sweep just kept
        // running and the menu never came back). Latch it and leave both.
        bool exitRequested = false;

        for (int ch = 0; ch < NRF_CENSUS_CHANNELS; ch++) {
            NRFradio.setChannel(ch);
            for (int a = 0; a < CENSUS_ADDR_COUNT; a++) {
                NRFradio.openReadingPipe(1, CENSUS_ADDRS[a].bytes);
                NRFradio.startListening();
                delayMicroseconds(NRF_CENSUS_LISTEN_US);
                if (NRFradio.available()) {
                    uint8_t buf[NRF_CENSUS_PAYLOAD] = {0};
                    NRFradio.read(buf, NRF_CENSUS_PAYLOAD);
                    censusBump(censusChan[ch]);
                    censusBump(censusAddr[a]);
                    censusLastAddr = (int8_t)a;
                    censusLastChan = (int8_t)ch;
                    memcpy(censusLast, buf, sizeof(censusLast));
                    if (censusTotal < 0xFFFFFFFFu) censusTotal++;
                }
                NRFradio.stopListening();
            }
            // A 126-channel pass takes ~1.2 s; redraw between channels so the
            // bars move while the sweep runs instead of appearing at the end.
            if (millis() - lastDraw > 250) {
                censusDraw(saved);
                lastDraw = millis();
            }
            if (check(EscPress)) {
                exitRequested = true;
                break;
            }
        }
        if (exitRequested) break;
        censusPasses++;
        censusDraw(saved);

        if (check(SelPress)) {
            saved = censusSave();
            if (!saved) displayError("No SD card", true);
            censusDraw(saved);
            while (check(SelPress)) vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    NRFradio.stopListening();
    NRFradio.powerDown();
    returnToMenu = true;
}
