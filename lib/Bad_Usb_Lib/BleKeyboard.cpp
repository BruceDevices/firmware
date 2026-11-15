#include "BleKeyboard.h"
#include "KeyboardLayout.h"

#include "HIDTypes.h"
#include "sdkconfig.h"
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <string.h>

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG ""
#else
#include "esp_log.h"
static const char *LOG_TAG = "BLEDevice";
#endif

// Report IDs:
#define KEYBOARD_ID 0x01
#define MEDIA_KEYS_ID 0x02
#define MOUSE_ID 0x03

static const uint8_t _hidReportDescriptor[] = {
    USAGE_PAGE(1),
    0x01, // USAGE_PAGE (Generic Desktop Ctrls)
    USAGE(1),
    0x06, // USAGE (Keyboard)
    COLLECTION(1),
    0x01, // COLLECTION (Application)
    // ------------------------------------------------- Keyboard
    REPORT_ID(1),
    KEYBOARD_ID, //   REPORT_ID (1)
    USAGE_PAGE(1),
    0x07, //   USAGE_PAGE (Kbrd/Keypad)
    USAGE_MINIMUM(1),
    0xE0, //   USAGE_MINIMUM (0xE0)
    USAGE_MAXIMUM(1),
    0xE7, //   USAGE_MAXIMUM (0xE7)
    LOGICAL_MINIMUM(1),
    0x00, //   LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1),
    0x01, //   Logical Maximum (1)
    REPORT_SIZE(1),
    0x01, //   REPORT_SIZE (1)
    REPORT_COUNT(1),
    0x08, //   REPORT_COUNT (8)
    HIDINPUT(1),
    0x02, //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    REPORT_COUNT(1),
    0x01, //   REPORT_COUNT (1) ; 1 byte (Reserved)
    REPORT_SIZE(1),
    0x08, //   REPORT_SIZE (8)
    HIDINPUT(1),
    0x01, //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    REPORT_COUNT(1),
    0x05, //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
    REPORT_SIZE(1),
    0x01, //   REPORT_SIZE (1)
    USAGE_PAGE(1),
    0x08, //   USAGE_PAGE (LEDs)
    USAGE_MINIMUM(1),
    0x01, //   USAGE_MINIMUM (0x01) ; Num Lock
    USAGE_MAXIMUM(1),
    0x05, //   USAGE_MAXIMUM (0x05) ; Kana
    HIDOUTPUT(1),
    0x02, //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    REPORT_COUNT(1),
    0x01, //   REPORT_COUNT (1) ; 3 bits (Padding)
    REPORT_SIZE(1),
    0x03, //   REPORT_SIZE (3)
    HIDOUTPUT(1),
    0x01, //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    REPORT_COUNT(1),
    0x06, //   REPORT_COUNT (6) ; 6 bytes (Keys)
    REPORT_SIZE(1),
    0x08, //   REPORT_SIZE(8)
    LOGICAL_MINIMUM(1),
    0x00, //   LOGICAL_MINIMUM(0)
    LOGICAL_MAXIMUM(1),
    0x65, //   LOGICAL_MAXIMUM(0x65) ; 101 keys
    USAGE_PAGE(1),
    0x07, //   USAGE_PAGE (Kbrd/Keypad)
    USAGE_MINIMUM(1),
    0x00, //   USAGE_MINIMUM (0)
    USAGE_MAXIMUM(1),
    0x65, //   USAGE_MAXIMUM (0x65)
    HIDINPUT(1),
    0x00,              //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    END_COLLECTION(0), // END_COLLECTION
    // ------------------------------------------------- Media Keys
    USAGE_PAGE(1),
    0x0C, // USAGE_PAGE (Consumer)
    USAGE(1),
    0x01, // USAGE (Consumer Control)
    COLLECTION(1),
    0x01, // COLLECTION (Application)
    REPORT_ID(1),
    MEDIA_KEYS_ID, //   REPORT_ID (3)
    USAGE_PAGE(1),
    0x0C, //   USAGE_PAGE (Consumer)
    LOGICAL_MINIMUM(1),
    0x00, //   LOGICAL_MINIMUM (0)
    LOGICAL_MAXIMUM(1),
    0x01, //   LOGICAL_MAXIMUM (1)
    REPORT_SIZE(1),
    0x01, //   REPORT_SIZE (1)
    REPORT_COUNT(1),
    0x10, //   REPORT_COUNT (16)
    USAGE(1),
    0xB5, //   USAGE (Scan Next Track)     ; bit 0: 1
    USAGE(1),
    0xB6, //   USAGE (Scan Previous Track) ; bit 1: 2
    USAGE(1),
    0xB7, //   USAGE (Stop)                ; bit 2: 4
    USAGE(1),
    0xCD, //   USAGE (Play/Pause)          ; bit 3: 8
    USAGE(1),
    0xE2, //   USAGE (Mute)                ; bit 4: 16
    USAGE(1),
    0xE9, //   USAGE (Volume Increment)    ; bit 5: 32
    USAGE(1),
    0xEA, //   USAGE (Volume Decrement)    ; bit 6: 64
    USAGE(2),
    0x23,
    0x02, //   Usage (WWW Home)            ; bit 7: 128
    USAGE(2),
    0x94,
    0x01, //   Usage (My Computer) ; bit 0: 1
    USAGE(2),
    0x92,
    0x01, //   Usage (Calculator)  ; bit 1: 2
    USAGE(2),
    0x2A,
    0x02, //   Usage (WWW fav)     ; bit 2: 4
    USAGE(2),
    0x21,
    0x02, //   Usage (WWW search)  ; bit 3: 8
    USAGE(2),
    0x26,
    0x02, //   Usage (WWW stop)    ; bit 4: 16
    USAGE(2),
    0x24,
    0x02, //   Usage (WWW back)    ; bit 5: 32
    USAGE(2),
    0x83,
    0x01, //   Usage (Media sel)   ; bit 6: 64
    USAGE(2),
    0x8A,
    0x01, //   Usage (Mail)        ; bit 7: 128
    HIDINPUT(1),
    0x02,             //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    END_COLLECTION(0) // END_COLLECTION
};

// Only advertised by Manga Remote, so other Bruce HID modes keep the same report map.
static const uint8_t _mouseHidReportDescriptor[] = {
    0x05, 0x01, // Usage Page: Generic Desktop
    0x09, 0x02, // Usage: Mouse
    0xA1, 0x01, // Collection: Application
    0x85, MOUSE_ID, // Report ID 3
    0x09, 0x01, // Usage: Pointer
    0xA1, 0x00, // Collection: Physical
    0x05, 0x09, // Usage Page: Button
    0x19, 0x01, // Usage Minimum: Button 1
    0x29, 0x03, // Usage Maximum: Button 3
    0x15, 0x00, // Logical Minimum: 0
    0x25, 0x01, // Logical Maximum: 1
    0x95, 0x03, // Report Count: 3
    0x75, 0x01, // Report Size: 1
    0x81, 0x02, // Input: Data, Variable, Absolute
    0x95, 0x01, // Report Count: 1
    0x75, 0x05, // Report Size: 5 (padding)
    0x81, 0x03, // Input: Constant, Variable, Absolute
    0x05, 0x01, // Usage Page: Generic Desktop
    0x09, 0x30, // X
    0x09, 0x31, // Y
    0x09, 0x38, // Wheel
    0x15, 0x81, // Logical Minimum: -127
    0x25, 0x7F, // Logical Maximum: +127
    0x75, 0x08, // Report Size: 8
    0x95, 0x03, // Report Count: 3
    0x81, 0x06, // Input: Data, Variable, Relative
    0xC0, 0xC0 // End both collections
};

BleKeyboard::BleKeyboard(String deviceName, String deviceManufacturer, uint8_t batteryLevel)
    : hid(0), deviceName(String(deviceName).substring(0, 15)),
      deviceManufacturer(String(deviceManufacturer).substring(0, 15)), batteryLevel(batteryLevel) {}

void BleKeyboard::begin(const uint8_t *layout, uint16_t showAs) {
    appearance = showAs;
    _asciimap = layout;
    BLEDevice::init(deviceName.c_str());
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));
    if (mouseEnabled) pServer->advertiseOnDisconnect(true);
    hid = new BLEHIDDevice(pServer);
    inputKeyboard = hid->getInputReport(KEYBOARD_ID); // <-- input REPORTID from report map
    outputKeyboard = hid->getOutputReport(KEYBOARD_ID);
    inputMediaKeys = hid->getInputReport(MEDIA_KEYS_ID);
    if (mouseEnabled) inputMouse = hid->getInputReport(MOUSE_ID);

    // NimBLE characteristics do not own their callback object.
    inputKeyboard->setCallbacks(&characteristicCallbacks);
    outputKeyboard->setCallbacks(&characteristicCallbacks);
    inputMediaKeys->setCallbacks(&characteristicCallbacks);
    if (mouseEnabled) inputMouse->setCallbacks(&characteristicCallbacks);

    hid->setManufacturer("Espressif");
    hid->setPnp(0x02, vid, pid, version);
    hid->setHidInfo(0x00, 0x01);

    BLEDevice::setSecurityAuth(true, true, true);

    if (mouseEnabled) {
        static uint8_t comboDescriptor[sizeof(_hidReportDescriptor) + sizeof(_mouseHidReportDescriptor)];
        memcpy(comboDescriptor, _hidReportDescriptor, sizeof(_hidReportDescriptor));
        memcpy(comboDescriptor + sizeof(_hidReportDescriptor), _mouseHidReportDescriptor,
               sizeof(_mouseHidReportDescriptor));
        hid->setReportMap(comboDescriptor, sizeof(comboDescriptor));
    } else {
        hid->setReportMap((uint8_t *)_hidReportDescriptor, sizeof(_hidReportDescriptor));
    }
    hid->startServices();
    advertising = pServer->getAdvertising();
    advertising->setAppearance(appearance);
    if (_randUUID) {
        advertising->addServiceUUID(BLEUUID((uint16_t)(ESP.getEfuseMac() & 0xFFFF)));
    } else {

        advertising->addServiceUUID(hid->getHidService()->getUUID());
        NimBLEAdvertisementData advertisementData = NimBLEAdvertisementData();
        advertisementData.setFlags(BLE_HS_ADV_F_DISC_GEN);
        advertisementData.setName(deviceName.c_str());
        advertisementData.setAppearance(appearance);
        // The custom payload above bypasses addServiceUUID() packing, and the
        // scan response is disabled, so without this the HID UUID (0x1812) is
        // never advertised. iOS Settings only lists pairable BLE HID
        // accessories that advertise 0x1812; Android lists any BLE peripheral.
        advertisementData.setCompleteServices(hid->getHidService()->getUUID());
        advertising->setAdvertisementData(advertisementData);
    }

    advertising->enableScanResponse(false);
    advertising->start();
    hid->setBatteryLevel(batteryLevel);
}

void BleKeyboard::end(void) {
    const auto peerHandles = pServer->getPeerDevices();
    for (const uint16_t handle : peerHandles) pServer->disconnect(handle);
    delete hid;
    hid = nullptr;

    BLEDevice::deinit(true);
    this->connected = false;
}

bool BleKeyboard::isConnected(void) { return this->connected; }

void BleKeyboard::setBatteryLevel(uint8_t level) {
    this->batteryLevel = level;
    if (hid != 0) this->hid->setBatteryLevel(this->batteryLevel);
}

// must be called before begin in order to set the name
void BleKeyboard::setName(const String &deviceName) { this->deviceName = deviceName; }

/**
 * @brief Sets the waiting time (in milliseconds) between multiple keystrokes in NimBLE mode.
 *
 * @param ms Time in milliseconds
 */
void BleKeyboard::setDelay(uint32_t ms) { this->_delay_ms = ms; }

void BleKeyboard::set_vendor_id(uint16_t vid) { this->vid = vid; }

void BleKeyboard::set_product_id(uint16_t pid) { this->pid = pid; }

void BleKeyboard::set_version(uint16_t version) { this->version = version; }

// Mirrors bleNotifyRetry() from modules/ble/ble_common: NimBLECharacteristic::notify()
// returns false when the os_mbuf could not be allocated (MSYS pool temporarily
// full, made worse by reducing CONFIG_BT_NIMBLE_MSYS_*_BLOCK_COUNT). If untreated,
// the key is silently dropped. Retry while yielding 1 tick for the host to drain.
// Kept local because this library is self-contained (does not depend on src/).

static bool bleKbNotifyRetry(BLECharacteristic *chr, const uint8_t *value, size_t len, uint8_t retries = 8) {
    if (chr == nullptr) return false;
    if (chr->notify(value, len)) return true;
    for (uint8_t i = 0; i < retries; i++) {
        vTaskDelay(1);
        if (chr->notify(value, len)) return true;
    }
    return false;
}

bool BleKeyboard::scrollWheel(int8_t wheel) {
    if (!mouseEnabled || inputMouse == nullptr || wheel == 0 ||
        !isConnected() || !mouseSubscribed) return false;
    const uint8_t report[4] = {0, 0, 0, static_cast<uint8_t>(wheel)};
    // The caller rate-limits wheel reports; avoid the keyboard delay_ms busy-wait.
    if (!bleKbNotifyRetry(inputMouse, report, sizeof(report))) return false;
    return true;
}

bool BleKeyboard::parkPointer() {
    if (!mouseEnabled || inputMouse == nullptr || !isConnected() || !mouseSubscribed) return false;
    const uint8_t report[4] = {0, 127, 0, 0};
    return bleKbNotifyRetry(inputMouse, report, sizeof(report));
}

void BleKeyboard::sendReport(KeyReport *keys) {
    if (this->isConnected() && this->getSubscribedCount() > 0) {
        this->inputKeyboard->setValue((uint8_t *)keys, sizeof(KeyReport));
        bleKbNotifyRetry(this->inputKeyboard, (uint8_t *)keys, sizeof(KeyReport));
        this->delay_ms(_delay_ms);
    }
}

void BleKeyboard::sendReport(MediaKeyReport *keys) {
    if (this->isConnected() && this->getSubscribedCount() > 0) {
        this->inputMediaKeys->setValue((uint8_t *)keys, sizeof(MediaKeyReport));
        bleKbNotifyRetry(this->inputMediaKeys, (uint8_t *)keys, sizeof(MediaKeyReport));
        this->delay_ms(_delay_ms);
    }
}

uint8_t USBPutChar(uint8_t c);

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t BleKeyboard::press(uint8_t k) {
    uint8_t i;
    if (k >= 0xE0 && k < 0xE8) {
        // k is not to be changed
    } else if (k >= 0x88) { // it's a non-printing key (not a modifier)
        k = k - 0x88;
    } else if (k >= 0x80) { // it's a modifier key
        _keyReport.modifiers |= (1 << (k - 0x80));
        k = 0;
    } else { // it's a printing key
        k = _asciimap[k];
        if (!k) {
            setWriteError();
            return 0;
        }
        if ((k & 0xc0) == 0xc0) {         // ALT_GR
            _keyReport.modifiers |= 0x40; // AltGr = right Alt
            k &= 0x3F;
        } else if ((k & 0x80) == 0x80) {  // SHIFT
            _keyReport.modifiers |= 0x02; // the left shift modifier
            k &= 0x7F;
        }
        if (k == 0x32) // ISO_REPLACEMENT
            k = 0x64;  // ISO_KEY
    }

    // Add k to the key report only if it's not already present
    // and if there is an empty slot.
    if (_keyReport.keys[0] != k && _keyReport.keys[1] != k && _keyReport.keys[2] != k &&
        _keyReport.keys[3] != k && _keyReport.keys[4] != k && _keyReport.keys[5] != k) {

        for (i = 0; i < 6; i++) {
            if (_keyReport.keys[i] == 0x00) {
                _keyReport.keys[i] = k;
                break;
            }
        }
        if (i == 6) {
            setWriteError();
            return 0;
        }
    }
    sendReport(&_keyReport);
    return 1;
}

size_t BleKeyboard::press(const MediaKeyReport k) {
    uint16_t k_16 = k[1] | (k[0] << 8);
    uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);

    mediaKeyReport_16 |= k_16;
    _mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
    _mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

    sendReport(&_mediaKeyReport);
    return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t BleKeyboard::release(uint8_t k) {
    uint8_t i;
    if (k >= 136) { // it's a non-printing key (not a modifier)
        k = k - 136;
    } else if (k >= 128) { // it's a modifier key
        _keyReport.modifiers &= ~(1 << (k - 128));
        k = 0;
    } else { // it's a printing key
        k = pgm_read_byte(_asciimap + k);
        if (!k) { return 0; }
        if ((k & ALT_GR) == ALT_GR) {
            _keyReport.modifiers &= ~(0x40); // AltGr = right Alt
            k &= 0x3F;
        } else if ((k & SHIFT) == SHIFT) {
            _keyReport.modifiers &= ~(0x02); // the left shift modifier
            k &= 0x7F;
        }
        if (k == ISO_REPLACEMENT) { k = ISO_KEY; }
    }

    // Test the key report to see if k is present.  Clear it if it exists.
    // Check all positions in case the key is present more than once (which it shouldn't be)
    for (i = 0; i < 6; i++) {
        if (0 != k && _keyReport.keys[i] == k) { _keyReport.keys[i] = 0x00; }
    }

    sendReport(&_keyReport);
    return 1;
}

size_t BleKeyboard::release(const MediaKeyReport k) {
    uint16_t k_16 = k[1] | (k[0] << 8);
    uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);
    mediaKeyReport_16 &= ~k_16;
    _mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
    _mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

    sendReport(&_mediaKeyReport);
    return 1;
}

void BleKeyboard::releaseAll(void) {
    _keyReport.keys[0] = 0;
    _keyReport.keys[1] = 0;
    _keyReport.keys[2] = 0;
    _keyReport.keys[3] = 0;
    _keyReport.keys[4] = 0;
    _keyReport.keys[5] = 0;
    _keyReport.modifiers = 0;
    _mediaKeyReport[0] = 0;
    _mediaKeyReport[1] = 0;
    sendReport(&_keyReport);
    sendReport(&_mediaKeyReport);
}

size_t BleKeyboard::write(uint8_t c) {
    uint8_t p = press(c); // Keydown
    release(c);           // Keyup
    return p;             // just return the result of press() since release() almost always returns 1
}

size_t BleKeyboard::write(const MediaKeyReport c) {
    uint16_t p = press(c); // Keydown
    release(c);            // Keyup
    return p;              // just return the result of press() since release() almost always returns 1
}

size_t BleKeyboard::write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    while (size--) {
        if (*buffer != '\r') {
            if (write(*buffer)) {
                n++;
            } else {
                break;
            }
        }
        buffer++;
    }
    return n;
}
void BleKeyboard::ServerCallbacks::onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) {
    parent->connected = true;
    Serial.println("BRUCE KEYBOARD: lib connected");
}
void BleKeyboard::ServerCallbacks::onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) {
    parent->connected = false;
    parent->mouseSubscribed = false;
    Serial.println("BRUCE KEYBOARD: lib disconnected");
}
void BleKeyboard::ServerCallbacks::onAuthenticationComplete(NimBLEConnInfo &connInfo) {
    if (connInfo.isEncrypted()) {
        Serial.println("BRUCE KEYBOARD: Paired successfully.");
        parent->connected = true;
    } else {
        Serial.println("BRUCE KEYBOARD: Pairing failed");
        parent->connected = false;
    }
}

void BleKeyboard::CharacteristicCallbacks::onWrite(
    NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo
) {
    uint8_t *value = (uint8_t *)(pCharacteristic->getValue().c_str());
    (void)value;
    ESP_LOGI(LOG_TAG, "special keys: %d", *value);
}
void BleKeyboard::CharacteristicCallbacks::onSubscribe(
    NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue
) {
    if (pCharacteristic == parent->inputMouse) parent->mouseSubscribed = (subValue & 1) != 0;
    if (subValue == 0) {
        Serial.println("BRUCE KEYBOARD: Client unsubscribed from notifications/indications.");
        if (parent->m_subCount) parent->m_subCount--;
    } else {
        parent->m_subCount++;
        Serial.println("BRUCE KEYBOARD: Client subscribed to notifications.");
    }
}

void BleKeyboard::delay_ms(uint64_t ms) {
    uint64_t m = esp_timer_get_time();
    if (ms) {
        uint64_t e = (m + (ms * 1000));
        if (m > e) { // overflow
            while (esp_timer_get_time() > e) {}
        }
        while (esp_timer_get_time() < e) {}
    }
}
