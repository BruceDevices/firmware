#include "telnet_scan.h"

#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/scrollableTextArea.h"
#include "core/wifi/wifi_common.h"
#include "wifi_raw_portal.h"
#include <WiFi.h>
#include <globals.h>
#include <vector>

// TCP/23 sweep of the local /24.
//
// Bounded on purpose: one short connect attempt per host, no retries, and the
// whole sweep stops early on BACK. A full /24 at 120 ms worst case is ~30 s,
// which is the price of a synchronous scan that cannot leave a task behind
// holding sockets (that is what makes this safe to run from a menu).

#define TELNET_PORT 23
#define TELNET_CONNECT_TIMEOUT_MS 120
#define TELNET_MAX_HITS 32

struct TelnetHit {
    IPAddress ip;
    uint16_t ms;
};

static std::vector<TelnetHit> s_hits;

// Draws "[####      ] 96/254" plus the hits found so far, so a 30 s sweep never
// looks like a hang.
static void drawProgress(int done, int total, const IPAddress &current) {
    const int barY = 30;
    const int barW = tftWidth - 20;
    tft.fillRect(10, barY, barW, 10, bruceConfig.bgColor);
    const int filled = total > 0 ? (barW * done) / total : 0;
    tft.drawRect(10, barY, barW, 10, bruceConfig.priColor);
    if (filled > 0) tft.fillRect(11, barY + 1, filled > barW - 2 ? barW - 2 : filled - 1, 8, bruceConfig.priColor);

    tft.fillRect(10, barY + 14, barW, 12, bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.secColor, bruceConfig.bgColor);
    tft.drawCentreString(String(done) + "/" + String(total) + "  " + current.toString(), tftWidth / 2, barY + 15, 1);
}

static void drawHits(int from) {
    // Five rows fit under the progress area on every supported screen.
    const int top = 62;
    const int rowH = 12;
    for (int i = 0; i < 5; i++) {
        const int idx = from + i;
        const int y = top + i * rowH;
        tft.fillRect(10, y, tftWidth - 20, rowH, bruceConfig.bgColor);
        if (idx >= (int)s_hits.size()) continue;
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.drawString(s_hits[idx].ip.toString() + ":" + String(TELNET_PORT), 12, y, 1);
        tft.drawRightString(String(s_hits[idx].ms) + "ms", tftWidth - 12, y, 1);
    }
}

void telnetScanMenu() {
    if (WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_MODE_NULL) {
        // No subnet to walk: say so instead of sweeping 0.0.0.0/24.
        displayError("Connect to WiFi first", true);
        returnToMenu = true;
        return;
    }

    const IPAddress local = WiFi.localIP();
    if (local == IPAddress(0, 0, 0, 0)) {
        displayError("No local IP: connect first", true);
        returnToMenu = true;
        return;
    }

    if (!wifiEnsureRadioMem()) {
        displayError("Low RAM for WiFi scan", true);
        returnToMenu = true;
        return;
    }

    s_hits.clear();

    drawMainBorderWithTitle("TELNET SCAN");
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawCentreString("port " + String(TELNET_PORT) + "  BACK stops", tftWidth / 2, 18, 1);

    uint8_t base[4] = {local[0], local[1], local[2], 0};
    const int total = 254;
    int done = 0;
    int shown = 0;
    const uint32_t startMs = millis();

    WiFiClient client;
    for (int host = 1; host <= total; host++) {
        if (check(EscPress)) break;

        IPAddress target(base[0], base[1], base[2], (uint8_t)host);
        if (target == local) { // skip our own address
            done++;
            continue;
        }

        const uint32_t t0 = millis();
        // connect() with an explicit timeout: the default is 30 s per host,
        // which would turn one dead IP into a hung page.
        const bool ok = client.connect(target, TELNET_PORT, TELNET_CONNECT_TIMEOUT_MS);
        const uint32_t dt = millis() - t0;
        client.stop();

        if (ok && s_hits.size() < TELNET_MAX_HITS) s_hits.push_back({target, (uint16_t)dt});

        done++;
        if ((done & 3) == 0) { // redraw every fourth host: fast, but not per-host
            drawProgress(done, total, target);
            if ((int)s_hits.size() != shown) {
                shown = s_hits.size();
                drawHits(0);
            }
            vTaskDelay(pdMS_TO_TICKS(1)); // let the idle task and the radio breathe
        }
    }

    drawProgress(done, total, local);

    // What was found, on one page that stays until BACK - a list that vanishes
    // after the sweep would be useless on a screen this small.
    String report = "Telnet scan " + local.toString() + "/24\n";
    report += String(done) + " hosts probed in " + String((millis() - startMs) / 1000) + "s\n";
    report += String(s_hits.size()) + " host(s) on port " + String(TELNET_PORT) + "\n";
    for (const TelnetHit &h : s_hits) { report += "  " + h.ip.toString() + ":" + String(TELNET_PORT) + "\n"; }
    if (s_hits.empty()) report += "  none\n";

    ScrollableTextArea area = ScrollableTextArea("TELNET SCAN");
    area.fromString(report);
    area.show();
    returnToMenu = true;
}
