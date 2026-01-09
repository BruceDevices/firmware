

#include "massStorage.h"
#include "core/display.h"
#include <USB.h>
#if defined(SOC_USB_OTG_SUPPORTED)
bool MassStorage::shouldStop = false;
int32_t MassStorage::status = -1;

MassStorage::MassStorage() { setup(); }

MassStorage::~MassStorage() {
    msc.end();
    USB.~ESPUSB();

    // Hack to make USB back to flash mode
    USB.enableDFU();
}

void MassStorage::setup() {
    displayMessage("Mounting...");

    setShouldStop(false);

    if (!setupSdCard()) {
        displayError("SD card not found.");
        delay(1000);
        return;
    }

    beginUsb();

    delay(500);
    return loop();
}

void MassStorage::loop() {
    int32_t prev_status = -1;
    while (!check(EscPress) && !shouldStop) {
        if (prev_status != status) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            switch (status) {
                case ARDUINO_USB_STARTED_EVENT: drawUSBStickIcon(true); break;
                case ARDUINO_USB_STOPPED_EVENT: drawUSBStickIcon(false); break;
                case ARDUINO_USB_SUSPEND_EVENT: MassStorage::displayMessage("USB suspend"); break;
                case ARDUINO_USB_RESUME_EVENT: MassStorage::displayMessage("USB resume"); break;
                default: break;
            }
            prev_status = status;
        } else vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void MassStorage::beginUsb() {
    setupUsbCallback();
    setupUsbEvent();
    drawUSBStickIcon(false);
    USB.begin();
}

void MassStorage::setupUsbCallback() {
    uint32_t secSize = SD.sectorSize();
    uint32_t numSectors = SD.numSectors();

    msc.vendorID("ESP32");
    msc.productID("BRUCE");
    msc.productRevision("1.0");

    msc.onRead(usbReadCallback);
    msc.onWrite(usbWriteCallback);
    msc.onStartStop(usbStartStopCallback);

    msc.mediaPresent(true);
    msc.begin(numSectors, secSize);
}

void MassStorage::setupUsbEvent() {
    USB.onEvent([](void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        if (event_base == ARDUINO_USB_EVENTS) { status = event_id; }
    });
}

void MassStorage::displayMessage(String message) {
    drawMainBorderWithTitle("Mass Storage");
    padprintln("");
    padprintln(message);
}

int32_t usbWriteCallback(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    // Verify freespace
    uint64_t freeSpace = SD.totalBytes() - SD.usedBytes();
    if (bufsize > freeSpace) {
        return -1; // no space available
    }

    // Verify sector size
    const uint32_t secSize = SD.sectorSize();
    if (secSize == 0) return -1; // disk error

    // Write blocs
    for (uint32_t x = 0; x < bufsize / secSize; ++x) {
        uint8_t blkBuffer[secSize];
        memcpy(blkBuffer, buffer + secSize * x, secSize);
        if (!SD.writeRAW(blkBuffer, lba + x)) {
            return -1; // write error
        }
    }
    return bufsize;
}

int32_t usbReadCallback(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    // Verify sector size
    const uint32_t secSize = SD.sectorSize();
    if (secSize == 0) return -1; // disk error

    // Read blocs
    for (uint32_t x = 0; x < bufsize / secSize; ++x) {
        if (!SD.readRAW(reinterpret_cast<uint8_t *>(buffer) + (x * secSize), lba + x)) {
            return -1; // read error
        }
    }
    return bufsize;
}

bool usbStartStopCallback(uint8_t power_condition, bool start, bool load_eject) {
    if (!start && load_eject) {
        MassStorage::setShouldStop(true);
        return false;
    }

    return true;
}

void drawUSBStickIcon(bool plugged) {
    static bool first = true;

    float scale;
    if (bruceConfigPins.rotation & 0b01) scale = float((float)tftHeight / (float)135);
    else scale = float((float)tftWidth / (float)240);

    int iconW = scale * 120;
    int iconH = scale * 40;

    if (iconW % 2 != 0) iconW++;
    if (iconH % 2 != 0) iconH++;

    int radius = 5;

    int bodyW = 2 * iconW / 3;
    int bodyH = iconH;
    int bodyX = tftWidth / 2 - iconW / 2;
    int bodyY = tftHeight / 2;

    int portW = iconW - bodyW;
    int portH = 0.8 * bodyH;
    int portX = bodyX + bodyW;
    int portY = bodyY + (bodyH - portH) / 2;

    int portDetailW = portW / 2;
    int portDetailH = portH / 4;
    int portDetailX = portX + (portW - portDetailW) / 2;
    int portDetailY1 = portY + 0.8 * portDetailH;
    int portDetailY2 = portY + portH - 1.8 * portDetailH;

    int ledW = 0.1 * bodyH;
    int ledH = 0.6 * bodyH;
    int ledX = bodyX + 2 * ledW;
    int ledY = bodyY + (iconH - ledH) / 2;

    if (first) {
        MassStorage::displayMessage("");
        // Body
        tft.fillRoundRect(bodyX, bodyY, bodyW, bodyH, radius, TFT_DARKCYAN);
        // Port USB
        tft.fillRoundRect(portX, portY, portW, portH, radius, TFT_LIGHTGREY);
        // Small square on port
        tft.fillRoundRect(portDetailX, portDetailY1, portDetailW, portDetailH, radius, TFT_DARKGREY);
        tft.fillRoundRect(portDetailX, portDetailY2, portDetailW, portDetailH, radius, TFT_DARKGREY);
        first = false;
    }
    // Led
    tft.fillRoundRect(ledX, ledY, ledW, ledH, radius, plugged ? TFT_GREEN : TFT_RED);
}

#endif // SOC_USB_OTG_SUPPORTED
