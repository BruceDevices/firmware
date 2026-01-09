#include "BleKeyboard.h"
#include "KeyboardLayout.h"

#include "HIDTypes.h"
#include "sdkconfig.h"
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>

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

BleKeyboard::BleKeyboard(String deviceName, String deviceManufacturer, uint8_t batteryLevel)
    : hid(0), deviceName(String(deviceName).substring(0, 15)),
      deviceManufacturer(String(deviceManufacturer).substring(0, 15)), batteryLevel(batteryLevel) {}

void BleKeyboard::begin(const uint8_t *layout, uint16_t showAs) {
    appearance = showAs;
    _asciimap = layout;
    BLEDevice::init(deviceName.c_str());
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));
    hid = new BLEHIDDevice(pServer);
    inputKeyboard = hid->getInputReport(KEYBOARD_ID); // <-- input REPORTID from report map
    outputKeyboard = hid->getOutputReport(KEYBOARD_ID);
    inputMediaKeys = hid->getInputReport(MEDIA_KEYS_ID);

    inputKeyboard->setCallbacks(new CharacteristicCallbacks(this));
    outputKeyboard->setCallbacks(new CharacteristicCallbacks(this));
    inputMediaKeys->setCallbacks(new CharacteristicCallbacks(this));

    hid->setManufacturer("Espressif");
    hid->setPnp(0x02, vid, pid, version);
    hid->setHidInfo(0x00, 0x01);

    BLEDevice::setSecurityAuth(true, true, true);

    hid->setReportMap((uint8_t *)_hidReportDescriptor, sizeof(_hidReportDescriptor));
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
        advertising->setAdvertisementData(advertisementData);
    }

    advertising->enableScanResponse(false);
    advertising->start();
    hid->setBatteryLevel(batteryLevel);
}

void BleKeyboard::end(void) {
    int i = 0;
    i = pServer->getConnectedCount();
    if (i > 0) {
        int j;
        for (j = 0; j < i; j++) pServer->disconnect(pServer->getPeerInfo(i).getConnHandle());
    }
    delete hid;
    BLEDevice::deinit();
    this->connected = false;
}

bool BleKeyboard::isConnected(void) { return this->connected; }

void BleKeyboard::setBatteryLevel(uint8_t level) {
    this->batteryLevel = level;
    if (hid != 0) this->hid->setBatteryLevel(this->batteryLevel);
}

// must be called before begin in order to set the name
void BleKeyboard::setName(String deviceName) { this->deviceName = deviceName; }

/**
 * @brief Sets the waiting time (in milliseconds) between multiple keystrokes in NimBLE mode.
 *
 * @param ms Time in milliseconds
 */
void BleKeyboard::setDelay(uint32_t ms) { this->_delay_ms = ms; }

void BleKeyboard::set_vendor_id(uint16_t vid) { this->vid = vid; }

void BleKeyboard::set_product_id(uint16_t pid) { this->pid = pid; }

void BleKeyboard::set_version(uint16_t version) { this->version = version; }

void BleKeyboard::sendReport(KeyReport *keys) {
#ifdef NIMBLE_V2_PLUS
    if (this->isConnected() && this->getSubscribedCount() > 0)
#else
    if (this->isConnected() && this->inputKeyboard->getSubscribedCount() > 0)
#endif
    {
        this->inputKeyboard->setValue((uint8_t *)keys, sizeof(KeyReport));
        this->inputKeyboard->notify();
#if defined(USE_NIMBLE)
        // vTaskDelay(delayTicks);
        this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
    }
}

void BleKeyboard::sendReport(MediaKeyReport *keys) {
#ifdef NIMBLE_V2_PLUS
    if (this->isConnected() && this->getSubscribedCount() > 0)
#else
    if (this->isConnected() && this->inputKeyboard->getSubscribedCount() > 0)
#endif
    {
        this->inputMediaKeys->setValue((uint8_t *)keys, sizeof(MediaKeyReport));
        this->inputMediaKeys->notify();
#if defined(USE_NIMBLE)
        // vTaskDelay(delayTicks);
        this->delay_ms(_delay_ms);
#endif // USE_NIMBLE
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
#ifdef NIMBLE_V2_PLUS
void BleKeyboard::ServerCallbacks::onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) {
    // BleKeyboard::connected = true;
    Serial.println("BRUCE KEYBOARD: lib connected");
}
void BleKeyboard::ServerCallbacks::onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) {
    // BleKeyboard::connected = true;
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
    if (subValue == 0) {
        Serial.println("BRUCE KEYBOARD: Client unsubscribed from notifications/indications.");
        if (parent->m_subCount) parent->m_subCount--;
    } else {
        parent->m_subCount++;
        Serial.println("BRUCE KEYBOARD: Client subscribed to notifications.");
    }
}

#else
void BleKeyboard::onConnect(BLEServer *pServer) {
    // this->connected = true;
    Serial.println("lib connected");
}
void BleKeyboard::onDisconnect(BLEServer *pServer) {
    this->connected = false;
    // NimBLEDevice::startAdvertising();
    Serial.println("lib disconnected");
}
void BleKeyboard::onWrite(BLECharacteristic *me) {
    uint8_t *value = (uint8_t *)(me->getValue().c_str());
    (void)value;
    ESP_LOGI(LOG_TAG, "special keys: %d", *value);
}
void BleKeyboard::onSubscribe(
    NimBLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue
) {
    if (subValue == 0) {
        Serial.println("Client unsubscribed from notifications/indications.");
    } else {
        Serial.println("Client subscribed to notifications.");
    }
}
void BleKeyboard::onAuthenticationComplete(ble_gap_conn_desc *desc) {
    if (desc->sec_state.encrypted) {
        Serial.println("Paired successfully.");
        this->connected = true;
    } else {
        Serial.println("Pairing failed");
        this->connected = false;
    }
}
#endif

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
