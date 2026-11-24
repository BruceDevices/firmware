#include "massStorage.h"
#if defined(SOC_USB_OTG_SUPPORTED)
#include "core/display.h"
#include "core/sd_functions.h"
#include <USB.h>
#include <USBMSC.h>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <device/usbd_pvt.h>
#include <memory>
#include <mutex>
#include <new>
#include <tusb.h>

namespace {
enum class Session : uint8_t { Idle, Starting, Active, Stopping, Failed };

// Serialize the session transition with the entire synchronous block transfer.
std::mutex sessionMutex;
std::atomic<Session> session{Session::Idle};
uint32_t sectorCount;
uint16_t sectorSize;
bool exportedBefore;
std::unique_ptr<uint8_t[]> sectorBuffer;

bool validTransfer(uint32_t lba, uint32_t offset, const void *buffer, uint32_t size) {
    if (session != Session::Active || !buffer || sectorSize == 0 || sectorCount == 0 || size == 0 ||
        size > INT32_MAX) {
        return false;
    }
    const uint64_t address = static_cast<uint64_t>(lba) * sectorSize + offset;
    const uint64_t capacity = static_cast<uint64_t>(sectorCount) * sectorSize;
    return address < capacity && size <= capacity - address;
}

bool alignedBuffer(const void *buffer) {
    return reinterpret_cast<uintptr_t>(buffer) % alignof(uint32_t) == 0;
}

bool prepareSectorBuffer(uint32_t offset, const void *buffer, uint32_t size) {
    if (offset == 0 && size % sectorSize == 0 && alignedBuffer(buffer) &&
        (size <= sectorSize || sectorSize % alignof(uint32_t) == 0)) {
        return true;
    }
    // Reserve before any I/O so allocation failure cannot follow an earlier sector write.
    // The session mutex protects this reusable buffer and its lifetime.
    if (!sectorBuffer) sectorBuffer.reset(new (std::nothrow) uint8_t[sectorSize]);
    return static_cast<bool>(sectorBuffer);
}

int32_t writeBlocks(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t size) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    if (!validTransfer(lba, offset, buffer, size)) return -1;
    lba += offset / sectorSize;
    offset %= sectorSize;
    if (!prepareSectorBuffer(offset, buffer, size)) return -1;
    for (uint32_t pos = 0; pos < size; ++lba) {
        const uint32_t count = std::min<uint32_t>(sectorSize - offset, size - pos);
        uint8_t *block = buffer + pos;
        if (offset != 0 || count != sectorSize) {
            if (!SD.readRAW(sectorBuffer.get(), lba)) return -1;
            memcpy(sectorBuffer.get() + offset, block, count);
            block = sectorBuffer.get();
        } else if (!alignedBuffer(block)) {
            // The SPI driver may access a complete block through word-sized pointers.
            memcpy(sectorBuffer.get(), block, count);
            block = sectorBuffer.get();
        }
        if (!SD.writeRAW(block, lba)) return -1;
        pos += count;
        offset = 0;
    }
    return size;
}

int32_t readBlocks(uint32_t lba, uint32_t offset, void *buffer, uint32_t size) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    if (!validTransfer(lba, offset, buffer, size)) return -1;
    lba += offset / sectorSize;
    offset %= sectorSize;
    if (!prepareSectorBuffer(offset, buffer, size)) return -1;
    auto *bytes = static_cast<uint8_t *>(buffer);
    for (uint32_t pos = 0; pos < size; ++lba) {
        const uint32_t count = std::min<uint32_t>(sectorSize - offset, size - pos);
        uint8_t *block = bytes + pos;
        if (offset != 0 || count != sectorSize || !alignedBuffer(block)) {
            if (!SD.readRAW(sectorBuffer.get(), lba)) return -1;
            memcpy(block, sectorBuffer.get() + offset, count);
        } else if (!SD.readRAW(block, lba)) {
            return -1;
        }
        pos += count;
        offset = 0;
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

    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        sectorBuffer.reset();
    }

    if (!endSdCardRawAccess()) displayError("SD remount failed", true);
    else if (!started) displayError("USB storage failed", true);
}

#endif // SOC_USB_OTG_SUPPORTED
