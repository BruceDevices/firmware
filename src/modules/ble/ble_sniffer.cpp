/*
 * BLE Sniffer - Standalone module for LITE_VERSION
 * Author: Ninja-jr
 * Date: 2026-01-24
 * 
 * Captures BLE advertisements, displays hex dumps,
 * parses manufacturer data, and saves to SD/LittleFS.
 */

#if defined(LITE_VERSION)
#include "ble_sniffer.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include <SD.h>
#include <LittleFS.h>
#include <NimBLEDevice.h>
#include <globals.h>

struct SnifferPacket {
    String address;
    String name;
    int rssi;
    std::vector<uint8_t> payload;
    String payloadHex;
    String timestamp;
    int channel;
};

static std::vector<SnifferPacket> snifferPackets;
static int snifferPacketCount = 0;
static NimBLEScan *pSnifferScan = nullptr;

static String payloadToHex(const std::vector<uint8_t> &payload) {
    String hex = "";
    for (size_t i = 0; i < payload.size(); i++) {
        if (payload[i] < 0x10) hex += "0";
        hex += String(payload[i], HEX);
        if (i < payload.size() - 1) hex += " ";
        if ((i + 1) % 16 == 0 && i < payload.size() - 1) hex += "\n";
    }
    return hex;
}

static String parseManufacturerData(const std::vector<uint8_t> &payload) {
    if (payload.size() < 2) return "Unknown";
    
    uint16_t companyId = (payload[1] << 8) | payload[0];
    String info = "Company: 0x" + String(companyId, HEX) + " ";
    
    switch (companyId) {
        case 0x004C:
            info += "(Apple)";
            if (payload.size() >= 4) {
                uint8_t type = payload[2];
                uint8_t subtype = payload[3];
                if (type == 0x07 && subtype == 0x19) info += " Continuity";
                else if (type == 0x04 && subtype == 0x04) info += " Continuity Action";
                else if (type == 0x0F && subtype == 0x05) info += " Nearby Action";
                else if (type == 0x10 && subtype == 0x14) info += " iBeacon";
            }
            break;
        case 0x0075:
            info += "(Samsung)";
            if (payload.size() >= 4) {
                if (payload[2] == 0x42 && payload[3] == 0x09) info += " Galaxy Buds";
                else if (payload[2] == 0x01 && payload[3] == 0x00) info += " Galaxy Watch";
            }
            break;
        case 0xFE2C:
            info += "(Google FastPair)";
            if (payload.size() >= 6) {
                uint32_t modelId = (payload[4] << 16) | (payload[5] << 8) | payload[6];
                info += " Model: 0x" + String(modelId, HEX);
            }
            break;
        case 0x0600:
            info += "(Microsoft)";
            break;
        default:
            info += "(Unknown)";
    }
    return info;
}

void BLE_Sniffer() {
    drawMainBorderWithTitle("BLE SNIFFER");
    padprintln("");
    padprintln("Press [SEL] to start/stop capture");
    padprintln("Press [ESC] to exit");
    padprintln("");
    padprintln("Status: READY");
    
    bool isCapturing = false;
    bool firstRun = true;
    bool bleInitialized = false;
    
    while (true) {
        if (check(EscPress)) {
            if (isCapturing && pSnifferScan) {
                pSnifferScan->stop();
                isCapturing = false;
            }
            if (pSnifferScan) {
                pSnifferScan->clearResults();
                pSnifferScan = nullptr;
            }
            if (bleInitialized) {
                NimBLEDevice::deinit(true);
                bleInitialized = false;
            }
            break;
        }
        
        if (check(SelPress)) {
            isCapturing = !isCapturing;
            if (isCapturing) {
                if (firstRun) {
                    NimBLEDevice::init("BruceSniffer");
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                    pSnifferScan = NimBLEDevice::getScan();
                    if (!pSnifferScan) {
                        displayError("Failed to init scanner");
                        NimBLEDevice::deinit(true);
                        break;
                    }
                    pSnifferScan->setActiveScan(true);
                    pSnifferScan->setInterval(97);
                    pSnifferScan->setWindow(67);
                    pSnifferScan->setDuplicateFilter(false);
                    bleInitialized = true;
                    firstRun = false;
                }
                snifferPacketCount = 0;
                snifferPackets.clear();
                padprintln("");
                padprintln("Status: CAPTURING...");
                padprintln("Press [SEL] to stop");
                
                NimBLEScanResults results = pSnifferScan->getResults(10 * 1000, true);
                
                for (int i = 0; i < results.getCount(); i++) {
                    const NimBLEAdvertisedDevice *device = results.getDevice(i);
                    
                    SnifferPacket packet;
                    packet.address = String(device->getAddress().toString().c_str());
                    packet.name = String(device->getName().c_str());
                    if (packet.name.isEmpty()) packet.name = "Unknown";
                    packet.rssi = device->getRSSI();
                    packet.timestamp = String(millis() / 1000);
                    
                    std::string manufData = device->getManufacturerData();
                    packet.payload.assign(manufData.begin(), manufData.end());
                    packet.payloadHex = payloadToHex(packet.payload);
                    packet.channel = 37 + (i % 3);
                    
                    snifferPackets.push_back(packet);
                    snifferPacketCount++;
                }
                
                pSnifferScan->stop();
                isCapturing = false;
                padprintln("");
                padprintln("Status: DONE");
                padprintln("Captured: " + String(snifferPacketCount) + " packets");
                padprintln("");
                padprintln("Press [SEL] to view packets");
                padprintln("Press [NEXT] to save to SD/LittleFS");
                padprintln("Press [ESC] to exit");
            }
        }
        
        if (check(SelPress) && !isCapturing && snifferPacketCount > 0) {
            int selected = 0;
            int scrollOffset = 0;
            bool viewing = true;
            
            while (viewing) {
                if (check(EscPress)) {
                    viewing = false;
                    break;
                }
                
                tft.fillScreen(bruceConfig.bgColor);
                drawMainBorderWithTitle("CAPTURED PACKETS");
                
                int y = BORDER_PAD_Y + FM * LH + 4;
                int lineH = max(14, tftHeight / 12);
                int visibleItems = (tftHeight - y - 50) / lineH;
                
                tft.setTextSize(FP);
                tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
                tft.setCursor(10, y);
                tft.println("Packets: " + String(snifferPacketCount));
                y += lineH;
                
                for (int i = 0; i < visibleItems && (scrollOffset + i) < snifferPacketCount && i < 5; i++) {
                    int idx = scrollOffset + i;
                    SnifferPacket &pkt = snifferPackets[idx];
                    bool selectedItem = (idx == selected);
                    uint16_t fg = selectedItem ? bruceConfig.bgColor : TFT_WHITE;
                    uint16_t bg = selectedItem ? bruceConfig.priColor : bruceConfig.bgColor;
                    
                    tft.fillRect(10, y, tftWidth - 20, lineH - 2, bg);
                    tft.setTextColor(fg, bg);
                    String display = String(idx + 1) + ". " + pkt.name + " | " + pkt.address + " | " + String(pkt.rssi) + "dB";
                    if (display.length() > 35) display = display.substring(0, 32) + "...";
                    tft.drawString(display, 15, y + 2, 1);
                    y += lineH;
                }
                
                if (snifferPacketCount > visibleItems) {
                    tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
                    tft.setCursor(tftWidth - 30, BORDER_PAD_Y + FM * LH + 4 + lineH);
                    if (scrollOffset > 0) tft.drawString("^", tftWidth - 25, BORDER_PAD_Y + FM * LH + 4 + lineH, 1);
                    if (scrollOffset + visibleItems < snifferPacketCount) {
                        tft.drawString("v", tftWidth - 25, BORDER_PAD_Y + FM * LH + 4 + lineH * (visibleItems - 1), 1);
                    }
                }
                
                tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
                tft.setCursor(10, tftHeight - 20);
                tft.drawString("PREV/NEXT: Navigate  SEL: View Details  ESC: Back", 10, tftHeight - 20, 1);
                
                if (check(NextPress)) {
                    if (selected < snifferPacketCount - 1) {
                        selected++;
                        if (selected >= scrollOffset + visibleItems) {
                            scrollOffset = selected - visibleItems + 1;
                        }
                    }
                }
                if (check(PrevPress)) {
                    if (selected > 0) {
                        selected--;
                        if (selected < scrollOffset) {
                            scrollOffset = selected;
                        }
                    }
                }
                if (check(SelPress)) {
                    SnifferPacket &pkt = snifferPackets[selected];
                    
                    drawMainBorderWithTitle("PACKET DETAILS");
                    int dy = BORDER_PAD_Y + FM * LH + 4;
                    int dlh = max(12, tftHeight / 14);
                    tft.setTextSize(FP);
                    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
                    
                    tft.setCursor(10, dy);
                    tft.println("Device: " + pkt.name);
                    dy += dlh;
                    tft.println("Address: " + pkt.address);
                    dy += dlh;
                    tft.println("RSSI: " + String(pkt.rssi) + " dBm");
                    dy += dlh;
                    tft.println("Channel: " + String(pkt.channel));
                    dy += dlh;
                    tft.println("Timestamp: " + pkt.timestamp + "s");
                    dy += dlh;
                    tft.println("Payload (" + String(pkt.payload.size()) + " bytes):");
                    dy += dlh;
                    
                    String parsed = parseManufacturerData(pkt.payload);
                    tft.setTextColor(TFT_CYAN, bruceConfig.bgColor);
                    tft.println(parsed);
                    dy += dlh;
                    
                    tft.setTextColor(TFT_GREEN, bruceConfig.bgColor);
                    String hexDump = pkt.payloadHex;
                    if (hexDump.length() > 400) hexDump = hexDump.substring(0, 400) + "...\n(truncated)";
                    tft.println(hexDump);
                    
                    tft.setTextColor(TFT_DARKGREY, bruceConfig.bgColor);
                    tft.setCursor(10, tftHeight - 20);
                    tft.drawString("Press any key to continue", 10, tftHeight - 20, 1);
                    
                    while (!check(EscPress) && !check(SelPress) && !check(PrevPress) && !check(NextPress)) {
                        delay(50);
                    }
                }
                delay(100);
            }
        }
        
        if (check(NextPress) && !isCapturing && snifferPacketCount > 0) {
            FS *fs = nullptr;
            String storageType = "";
            
            if (getFsStorage(fs) && fs == &SD) {
                storageType = "SD";
            }
            else if (LittleFS.begin()) {
                fs = &LittleFS;
                storageType = "LittleFS";
            }
            
            if (fs && !storageType.isEmpty()) {
                if (!fs->exists("/BruceSniffer")) fs->mkdir("/BruceSniffer");
                
                String filename = "/BruceSniffer/sniffer_" + String(millis()) + ".txt";
                File file = fs->open(filename, FILE_WRITE);
                if (file) {
                    file.println("=== BLE SNIFFER CAPTURE ===");
                    file.println("Timestamp: " + String(millis()));
                    file.println("Total packets: " + String(snifferPacketCount));
                    file.println("");
                    
                    for (size_t i = 0; i < snifferPackets.size(); i++) {
                        SnifferPacket &pkt = snifferPackets[i];
                        file.printf("[%d] %s | %s | %d dBm | Ch:%d\n", 
                                   i + 1, pkt.name.c_str(), pkt.address.c_str(), pkt.rssi, pkt.channel);
                        file.print("  Payload: ");
                        for (size_t j = 0; j < pkt.payload.size(); j++) {
                            file.printf("%02X ", pkt.payload[j]);
                            if ((j + 1) % 16 == 0) file.print("\n  ");
                        }
                        file.println("\n");
                    }
                    file.close();
                    displaySuccess("Saved to " + storageType);
                } else {
                    displayError("Failed to save");
                }
            } else {
                displayError("No storage available");
            }
            delay(1000);
        }
        
        delay(100);
    }
}

void BLE_SnifferMenu() {
    std::vector<Option> snifferOptions;
    snifferOptions.push_back({"Start Sniffer", BLE_Sniffer});
    snifferOptions.push_back({"Back", []() { returnToMenu = true; }});
    loopOptions(snifferOptions, MENU_TYPE_SUBMENU, "BLE Sniffer");
}

#endif
