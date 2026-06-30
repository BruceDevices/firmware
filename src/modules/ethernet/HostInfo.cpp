/**
 * @file HostInfo.cpp
 * @brief HostInfo module for every esp-netif
 * @version 0.2
 * @date 2025-11-26
 */

#include "HostInfo.h"
#include "ESPNetifEthernetClient.h"
#include "core/display.h"
#include "core/net_utils.h"
#include "core/scrollableTextArea.h"

HostInfo::HostInfo(const Host &host, bool wifi) {
#if !defined(LITE_VERSION)
    if (!wifi) {
        eth_client = new ESPNetifEthernetClient();
    } else {
        wifi_client = new WiFiClient();
    }
#else
    wifi_client = new WiFiClient();
#endif

    setup(host);
}

HostInfo::~HostInfo() {
#if !defined(LITE_VERSION)
    if (eth_client != nullptr) {
        delete eth_client;
    } else {
        delete wifi_client;
    }
#else
    delete wifi_client;
#endif
}

void HostInfo::client_stop() {
#if !defined(LITE_VERSION)
    if (eth_client != nullptr) {
        eth_client->client_close(sockfd);
    } else {
        wifi_client->stop();
    }
#else
    wifi_client->stop();
#endif
}

void HostInfo::client_connect(IPAddress ip, int port, int timeout_ms) {
#if !defined(LITE_VERSION)
    if (eth_client != nullptr) {
        sockfd = eth_client->connect(ip, port, timeout_ms);
    } else {
        wifi_client->connect(ip, port, timeout_ms);
    }
#else
    wifi_client->connect(ip, port, timeout_ms);
#endif
}

bool HostInfo::client_connected() {
#if !defined(LITE_VERSION)
    if (eth_client != nullptr) {
        return eth_client->connected(sockfd);
    } else {
        return wifi_client->connected();
    }
#else
    return wifi_client->connected();
#endif
}

void HostInfo::setup(const Host &host) {
    const int TIMEOUT_MS = 150; // Timeout for connection attempt

    // Array of TCP ports to scan
    const int portNumbers[] = {
        19, 20, 21, 22, 23, 25, 42, 53, 67, 68, 69, 80, 88,
        110, 111, 113, 119, 123, 135, 137, 139, 143, 161, 162,
        179, 194, 389, 427, 443, 445, 464, 465, 500, 514, 515,
        520, 554, 587, 631, 636, 873, 902, 989, 990, 993, 995,
        1024, 1025, 1080, 1194, 1337, 1352, 1433, 1434, 1512,
        1521, 1604, 1701, 1720, 1723, 1812, 1813, 1883, 1900,
        2049, 2082, 2083, 2086, 2087, 2095, 2181, 2222, 2375,
        2376, 2379, 2380, 3128, 3306, 3389, 3690, 4000, 4500,
        4789, 5000, 5060, 5061, 5222, 5353, 5432, 5671, 5672,
        5900, 5985, 5986, 6379, 6443, 6514, 6667, 7001, 7077,
        8000, 8080, 8081, 8443, 8444, 8888, 9000, 9042, 9090,
        9100, 9200, 9300, 9418, 10000, 11211, 15672, 27017,
        32768, 49152, 49153, 49154, 49155, 49156, 49157
    };
    const int portCount = sizeof(portNumbers) / sizeof(portNumbers[0]);

    // Initialize display
    drawMainBorder();
    tft.setTextSize(FP);

    ScrollableTextArea area = ScrollableTextArea("HOST INFO");

    area.addLine("Host: " + host.ip.toString());
    area.addLine("Mac: " + host.mac);
    area.addLine("Manufacturer: " + getManufacturer(host.mac));
    area.addLine("Scanning Ports... Wait");
    area.addLine("Open TCP Ports: ");

    area.draw();

    //bool scanCanceled = false;

    for (int i = 0; i < portCount; i++) {
        int port = portNumbers[i];

        if (check(EscPress)) {
            returnToMenu = true;
            break;
        }

        client_connect(host.ip, port, TIMEOUT_MS);
        unsigned long startTime = millis();

        bool connected = false;
        while (millis() - startTime < TIMEOUT_MS) {
            if (check(EscPress)) {
                returnToMenu = true;
                break;
            }
            if (client_connected()) {
                connected = true;
                break;
            }
        }

        if (returnToMenu) {
            client_stop();
            break;
        }

        if (connected) {
            area.addLine(String(port));
            Serial.println(port, DEC);
            area.draw();
        }

        client_stop();
    }

    /*
    if (scanCanceled) {
        tft.print("Scan Canceled!");
    } else {
        tft.print("Done!");
    }
    */

    area.show();
}
