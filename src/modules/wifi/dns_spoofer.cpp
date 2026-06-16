#if !defined(LITE_VERSION)
#include "dns_spoofer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/wifi/wifi_common.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <globals.h>

static DNSServer dnsServer;
static volatile bool dnsRunning = false;
static IPAddress spoofIp(192, 168, 4, 1);
static int queriesHandled = 0;

class SpoofDnsRequestHandler : public DNSServer {
public:
    void processNextRequest() {
        _dnsServer->processNextRequest();
    }
};

static void startDnsSpoofer() {
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", spoofIp);
}

void dnsSpoofer() {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        displayError("Connect to WiFi first");
        return;
    }

    options.clear();
    int target = -1;
    options.push_back({"Gateway (192.168.4.1)", [&target]() { target = 0; }});
    options.push_back({"Custom IP", [&target]() { target = 1; }});
    options.push_back({"Back", [&target]() { target = -1; }});
    loopOptions(options, MENU_TYPE_SUBMENU, "Spoof Target");

    if (target < 0) return;

    if (target == 1) {
        String ip = keyboard("192.168.4.1", 15, "Spoof IP:");
        if (ip == "\x1B" || ip.isEmpty()) return;
        spoofIp.fromString(ip);
    } else {
        spoofIp = WiFi.gatewayIP();
    }

    startDnsSpoofer();
    dnsRunning = true;
    queriesHandled = 0;

    drawMainBorderWithTitle("DNS Spoofer");
    int y = 45;

    tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(12, y);
    tft.printf("Spoofing to: %s", spoofIp.toString().c_str());
    y += 14;
    tft.setCursor(12, y);
    tft.print("All DNS -> target IP");
    y += 20;

    while (dnsRunning) {
        dnsServer.processNextRequest();
        queriesHandled++;

        if (queriesHandled % 100 == 0) {
            tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
            tft.setCursor(12, y);
            tft.printf("Queries: %d", queriesHandled);

            tft.setTextColor(TFT_RED, bruceConfig.bgColor);
            tft.drawCentreString("Esc to stop", tftWidth / 2, tftHeight - 20, 1);
        }

        if (check(EscPress)) dnsRunning = false;
        delay(1);
    }

    dnsServer.stop();
    displayInfo("DNS Spoofer stopped", true);
}
#endif
