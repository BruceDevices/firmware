#include "massStorage.h"
#if defined(SOC_USB_OTG_SUPPORTED)
#include "core/display.h"
#include "core/sd_functions.h"
#include <USB.h>
#include <USBMSC.h>
#include <atomic>
#include <device/usbd_pvt.h>
#include <mutex>
#include <tusb.h>

namespace {
enum class Session : uint8_t { Idle, Starting, Active, Stopping, Failed };

// Serialize the session transition with the entire synchronous block transfer.
std::mutex sessionMutex;
std::atomic<Session> session{Session::Idle};
uint32_t sectorCount;
uint16_t sectorSize;
bool exportedBefore;

bool validTransfer(uint32_t lba, uint32_t offset, const void *buffer, uint32_t size) {
    if (session != Session::Active || !buffer || offset != 0 || size > INT32_MAX ||
        size % sectorSize != 0 || lba >= sectorCount) {
        return false;
    }
    return size / sectorSize <= sectorCount - lba;
}

int32_t writeBlocks(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t size) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    if (!validTransfer(lba, offset, buffer, size)) return -1;
    for (uint32_t pos = 0; pos < size; pos += sectorSize, ++lba) {
        if (!SD.writeRAW(buffer + pos, lba)) return -1;
    }
    return size;
}

int32_t readBlocks(uint32_t lba, uint32_t offset, void *buffer, uint32_t size) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    if (!validTransfer(lba, offset, buffer, size)) return -1;
    auto *bytes = static_cast<uint8_t *>(buffer);
    for (uint32_t pos = 0; pos < size; pos += sectorSize, ++lba) {
        if (!SD.readRAW(bytes + pos, lba)) return -1;
    }
    return size;
}

bool startStop(uint8_t, bool start, bool loadEject);

// Register the descriptor before the first USB.begin(), including CDC-on-boot builds.
// Arduino's USBMSC::end() clears callbacks but does not release its LUN.
struct StorageLun : USBMSC {
    StorageLun() {
        vendorID("ESP32");
        productID("BRUCE");
        productRevision("1.0");
        onRead(readBlocks);
        onWrite(writeBlocks);
        onStartStop(startStop);
    }
} msc;

bool startStop(uint8_t, bool start, bool loadEject) {
    if (!start && loadEject) {
        std::lock_guard<std::mutex> lock(sessionMutex);
        if (session == Session::Active) {
            msc.mediaPresent(false);
            session = Session::Idle;
        }
    }
    return true;
}

// TinyUSB reads USBMSC's geometry and media fields outside our callbacks.
// Update those fields only in the USB device task after the stack has started.
void updateMedia(void *) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    if (session == Session::Starting) {
        const bool ready = msc.begin(sectorCount, sectorSize);
        msc.mediaPresent(ready);
        session = ready ? Session::Active : Session::Failed;
    } else if (session == Session::Stopping) {
        msc.mediaPresent(false);
        session = Session::Idle;
    }
}

void waitForMediaUpdate(Session pending) {
    const uint32_t start = millis();
    while (session == pending && millis() - start < 1000) { vTaskDelay(pdMS_TO_TICKS(10)); }
}

void stopMedia() {
    {
        // Wait for any transfer that already owns the mutex before blocking further I/O.
        std::lock_guard<std::mutex> lock(sessionMutex);
        if (session == Session::Idle) return;
        session = Session::Stopping;
    }
    usbd_defer_func(updateMedia, nullptr, false);
    waitForMediaUpdate(Session::Stopping);
}

void displayMessage(const char *message) {
    drawMainBorderWithTitle("Mass Storage");
    padprintln("");
    padprintln(message);
}

void drawUSBStickIcon(bool plugged);

void showSession() {
    bool mounted = tud_mounted();
    displayMessage(mounted ? "Eject on PC before Back" : "Connect USB");
    drawUSBStickIcon(mounted);

    while (session == Session::Active && !check(EscPress)) {
        const bool nowMounted = tud_mounted();
        if (nowMounted != mounted) {
            mounted = nowMounted;
            displayMessage(mounted ? "Eject on PC before Back" : "USB disconnected");
            drawUSBStickIcon(mounted);
        }
        // A host reset can also clear the mounted state. Keep the session available for reconnect.
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void drawUSBStickIcon(bool plugged) {
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

    tft.fillRoundRect(bodyX, bodyY, bodyW, bodyH, radius, TFT_DARKCYAN);
    tft.fillRoundRect(portX, portY, portW, portH, radius, TFT_LIGHTGREY);
    tft.fillRoundRect(portDetailX, portDetailY1, portDetailW, portDetailH, radius, TFT_DARKGREY);
    tft.fillRoundRect(portDetailX, portDetailY2, portDetailW, portDetailH, radius, TFT_DARKGREY);
    tft.fillRoundRect(ledX, ledY, ledW, ledH, radius, plugged ? TFT_GREEN : TFT_RED);
}

} // namespace

MassStorage::MassStorage() {
    displayMessage("Mounting...");
    switch (beginSdCardRawAccess()) {
        case SdRawAccess::Unavailable: displayError("SD card not found", true); return;
        case SdRawAccess::Busy: displayError("Close SD files and retry", true); return;
        case SdRawAccess::Ready: break;
    }

    const auto sectors = SD.numSectors();
    const auto blockSize = SD.sectorSize();
    const bool canStart = sectors > 0 && sectors <= UINT32_MAX && blockSize > 0 &&
                          blockSize <= UINT16_MAX && USB.begin();
    if (canStart) {
        // A host that safely removed the USB device may not poll the LUN again until enumeration.
        const bool reconnect = exportedBefore && tud_disconnect();
        if (reconnect) vTaskDelay(pdMS_TO_TICKS(250));
        {
            std::lock_guard<std::mutex> lock(sessionMutex);
            sectorCount = sectors;
            sectorSize = blockSize;
            session = Session::Starting;
        }
        usbd_defer_func(updateMedia, nullptr, false);
        waitForMediaUpdate(Session::Starting);
        if (reconnect) tud_connect();
    }

    const auto startResult = session.load();
    // A fast host eject can already have moved a successfully started session to Idle.
    const bool started = canStart && (startResult == Session::Active || startResult == Session::Idle);
    if (started) {
        exportedBefore = true;
        showSession();
    }
    if (canStart) stopMedia();

    if (!endSdCardRawAccess()) displayError("SD remount failed", true);
    else if (!started) displayError("USB storage failed", true);
}

#endif // SOC_USB_OTG_SUPPORTED
