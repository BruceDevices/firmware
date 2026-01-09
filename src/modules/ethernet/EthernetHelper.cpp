/**
 * @file EthernetHelper.cpp
 * @author Andrea Canale (https://github.com/andreock)
 * @brief Ethernet initialization file for W5500 Ethernet SPI card
 * @version 0.1
 * @date 2025-05-20
 */
#if !defined(LITE_VERSION)
#include "EthernetHelper.h"
#include "core/display.h"
#include <ETH.h>
#include <Network.h>
#include <SPI.h>
#include <esp_eth.h>
#include <globals.h>

EthernetHelper::EthernetHelper() {}

EthernetHelper::~EthernetHelper() {}

static bool connected = false;

/** Select the proper SPI bus for the W5500, reusing existing shared buses when possible. */
static SPIClass *selectEthernetSPIBus() {
    SPIClass *selectedSPI = &SPI;
    if (bruceConfigPins.W5500_bus.mosi == (gpio_num_t)TFT_MOSI &&
        bruceConfigPins.W5500_bus.mosi != GPIO_NUM_NC) {
#if TFT_MOSI > 0
        selectedSPI = &tft.getSPIinstance();
        Serial.println("Using TFT SPI for Ethernet");
#else
        Serial.println("TFT SPI unavailable, falling back to default SPI for Ethernet");
#endif
    } else if (bruceConfigPins.W5500_bus.mosi == bruceConfigPins.SDCARD_bus.mosi &&
               bruceConfigPins.W5500_bus.mosi != GPIO_NUM_NC) {
        selectedSPI = &sdcardSPI;
        sdcardSPI.begin(
            (int8_t)bruceConfigPins.W5500_bus.sck,
            (int8_t)bruceConfigPins.W5500_bus.miso,
            (int8_t)bruceConfigPins.W5500_bus.mosi,
            (int8_t)bruceConfigPins.W5500_bus.cs
        );
        Serial.println("Using SDCard SPI for Ethernet");
    } else if ((bruceConfigPins.W5500_bus.mosi == bruceConfigPins.NRF24_bus.mosi ||
                bruceConfigPins.W5500_bus.mosi == bruceConfigPins.CC1101_bus.mosi) &&
               bruceConfigPins.W5500_bus.mosi != GPIO_NUM_NC) {
        selectedSPI = &CC_NRF_SPI;
        CC_NRF_SPI.begin(
            (int8_t)bruceConfigPins.W5500_bus.sck,
            (int8_t)bruceConfigPins.W5500_bus.miso,
            (int8_t)bruceConfigPins.W5500_bus.mosi,
            (int8_t)bruceConfigPins.W5500_bus.cs
        );
        Serial.println("Using CC/NRF SPI for Ethernet");
    } else {
        SPI.begin(
            (int8_t)bruceConfigPins.W5500_bus.sck,
            (int8_t)bruceConfigPins.W5500_bus.miso,
            (int8_t)bruceConfigPins.W5500_bus.mosi,
            (int8_t)bruceConfigPins.W5500_bus.cs
        );
        Serial.println("Using dedicated SPI for Ethernet");
    }
    return selectedSPI;
}

/** Event handler for Ethernet events */
static void ethernet_event_handler(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_ETH_CONNECTED: {
            uint8_t mac_addr[6] = {0};
            ETH.macAddress(mac_addr);
            Serial.println("Ethernet Link Up");
            Serial.printf(
                "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                mac_addr[0],
                mac_addr[1],
                mac_addr[2],
                mac_addr[3],
                mac_addr[4],
                mac_addr[5]
            );
            displaySuccess("Ethernet Link Up");
            break;
        }
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("Ethernet Link Down");
            displayError("Ethernet Link Down");
            connected = false;
            break;
        case ARDUINO_EVENT_ETH_START: Serial.println("Ethernet Started"); break;
        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("Ethernet Stopped");
            displayError("Ethernet Stopped");
            connected = false;
            break;
        case ARDUINO_EVENT_ETH_LOST_IP:
            Serial.println("Ethernet Lost IP");
            displayError("Ethernet Lost IP");
            connected = false;
            break;
        case ARDUINO_EVENT_ETH_GOT_IP: {
            const esp_netif_ip_info_t &ip_info = info.got_ip.ip_info;
            connected = true;
            Serial.println("Ethernet Got IP Address");
            Serial.println("~~~~~~~~~~~");
            Serial.print("ETHIP:");
            Serial.println(IPAddress(ip_info.ip.addr));
            Serial.print("ETHMASK:");
            Serial.println(IPAddress(ip_info.netmask.addr));
            Serial.print("ETHGW:");
            Serial.println(IPAddress(ip_info.gw.addr));
            Serial.println("~~~~~~~~~~~");
            displaySuccess("Ethernet Got IP");
            break;
        }
        default: break;
    }
}

void EthernetHelper::generate_mac() {
    mac[0] = random(0, 255);
    mac[1] = random(0, 255);
    mac[2] = random(0, 255);
    mac[3] = random(0, 255);
    mac[4] = random(0, 255);
    mac[5] = random(0, 255);
}

bool EthernetHelper::setup() {
    if (bruceConfigPins.W5500_bus.cs == GPIO_NUM_NC || bruceConfigPins.W5500_bus.sck == GPIO_NUM_NC ||
        bruceConfigPins.W5500_bus.miso == GPIO_NUM_NC || bruceConfigPins.W5500_bus.mosi == GPIO_NUM_NC) {
        displayError("W5500 Pins not set", true);
        Serial.println("W5500 pins not configured, skipping Ethernet setup.");
        return false;
    }

    generate_mac();
    connected = false;

    if (ethEventId) {
        Network.removeEvent(ethEventId);
        ethEventId = 0;
    }
    ethEventId = Network.onEvent(ethernet_event_handler);

    ethSpi = selectEthernetSPIBus();
    const int csPin = static_cast<int>(bruceConfigPins.W5500_bus.cs);
    const int irqPin =
        (bruceConfigPins.W5500_bus.io0 == GPIO_NUM_NC) ? -1 : static_cast<int>(bruceConfigPins.W5500_bus.io0);
    const int rstPin =
        (bruceConfigPins.W5500_bus.io2 == GPIO_NUM_NC) ? -1 : static_cast<int>(bruceConfigPins.W5500_bus.io2);

    if (!ETH.begin(ETH_PHY_W5500, 1, csPin, irqPin, rstPin, *ethSpi)) {
        displayError("Ethernet start failed", true);
        Serial.println("ETH.begin failed");
        return false;
    }

    ethNetif = ETH.netif();

    return true;
}

bool EthernetHelper::is_connected() { return connected; }

void EthernetHelper::stop() {
    connected = false;
    if (ethEventId) {
        Network.removeEvent(ethEventId);
        ethEventId = 0;
    }
    ETH.end();
    ethSpi = nullptr;
}
#endif
