#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "ble_js.h"
#include "modules/ble/ble_common.h"

JSValue native_bleScan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: ble.scan(timeout_seconds)
    // returns: array of objects [{name, address, rssi}, ...]
    int timeout = 5; // default 5 seconds
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) { JS_ToInt32(ctx, &timeout, argv[0]); }
    if (timeout < 1) timeout = 1;
    if (timeout > 30) timeout = 30;

    BLEDevice::init("");
    NimBLEScan *pScan = BLEDevice::getScan();
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);

#if __has_include(<NimBLEExtAdvertising.h>)
    BLEScanResults results = pScan->getResults(timeout * 1000, false);
    int count = results.getCount();
#else
    BLEScanResults results = pScan->start(timeout, false);
    int count = results.getCount();
#endif

    JSValue arr = JS_NewArray(ctx, 0);
    for (int i = 0; i < count; i++) {
#if __has_include(<NimBLEExtAdvertising.h>)
        const NimBLEAdvertisedDevice *dev = results.getDevice(i);
#else
        NimBLEAdvertisedDevice dev_obj = results.getDevice(i);
        NimBLEAdvertisedDevice *dev = &dev_obj;
#endif
        JSValue obj = JS_NewObject(ctx);
        String name = dev->getName().c_str();
        if (name.isEmpty()) name = "<unknown>";
        JS_SetPropertyStr(ctx, obj, "name", JS_NewString(ctx, name.c_str()));
        JS_SetPropertyStr(ctx, obj, "address", JS_NewString(ctx, dev->getAddress().toString().c_str()));
        JS_SetPropertyStr(ctx, obj, "rssi", JS_NewInt32(ctx, dev->getRSSI()));
        JS_SetPropertyUint32(ctx, arr, i, obj);
    }

    pScan->clearResults();
    return arr;
}

JSValue native_bleAdvertise(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: ble.advertise(name_string)
    const char *name = "Bruce-App";
    JSCStringBuf name_buf;
    if (argc > 0 && JS_IsString(ctx, argv[0])) { name = JS_ToCString(ctx, argv[0], &name_buf); }

    BLEDevice::init(name);
    NimBLEServer *pServer = BLEDevice::createServer();
    pServer->getAdvertising()->start();

    return JS_NewBool(true);
}

JSValue native_bleStopAdvertise(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    NimBLEDevice::getAdvertising()->stop();
    BLEDevice::deinit();
    return JS_UNDEFINED;
}

// ---------------------------------------------------------------------------
// GATT client: connect to a peripheral and read/write its characteristics.
//
//   ble.connect(nameOrAddress, timeoutSec?) -> bool
//       Scans and connects to the first device whose name starts with
//       nameOrAddress (case-insensitive) or whose MAC equals it.
//   ble.isConnected() -> bool
//   ble.read(serviceUUID, charUUID) -> Uint8Array | null
//   ble.write(serviceUUID, charUUID, Uint8Array|string, withResponse?) -> bool
//   ble.disconnect()
// ---------------------------------------------------------------------------
#if __has_include(<NimBLEExtAdvertising.h>)
static NimBLEClient *jsBleClient = nullptr;

static bool jsBleIsConnected() { return jsBleClient != nullptr && jsBleClient->isConnected(); }

JSValue native_bleConnect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "ble.connect: arg0 must be a device name prefix or MAC address");
    }
    JSCStringBuf target_buf;
    const char *target_c = JS_ToCString(ctx, argv[0], &target_buf);
    String target = target_c ? target_c : "";
    int timeout = 5;
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &timeout, argv[1]);
    if (timeout < 1) timeout = 1;
    if (timeout > 30) timeout = 30;

    if (jsBleIsConnected()) jsBleClient->disconnect();

    BLEDevice::init("");
    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
    NimBLEScanResults results = pScan->getResults(timeout * 1000, false);

    const NimBLEAdvertisedDevice *found = nullptr;
    for (int i = 0; i < results.getCount(); i++) {
        const NimBLEAdvertisedDevice *dev = results.getDevice(i);
        String name = dev->getName().c_str();
        String addr = dev->getAddress().toString().c_str();
        if ((name.length() >= target.length() &&
             strncasecmp(name.c_str(), target.c_str(), target.length()) == 0) ||
            addr.equalsIgnoreCase(target)) {
            found = dev;
            break;
        }
    }

    bool ok = false;
    if (found != nullptr) {
        if (jsBleClient == nullptr) jsBleClient = NimBLEDevice::createClient();
        jsBleClient->setConnectTimeout(timeout * 1000);
        ok = jsBleClient->connect(found);
    }
    pScan->clearResults();
    return JS_NewBool(ok);
}

JSValue native_bleIsConnected(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewBool(jsBleIsConnected());
}

static NimBLERemoteCharacteristic *jsBleGetChr(JSContext *ctx, int argc, JSValue *argv) {
    if (!jsBleIsConnected() || argc < 2 || !JS_IsString(ctx, argv[0]) || !JS_IsString(ctx, argv[1])) {
        return nullptr;
    }
    JSCStringBuf svc_buf, chr_buf;
    const char *svc_c = JS_ToCString(ctx, argv[0], &svc_buf);
    std::string svc_uuid = svc_c ? svc_c : "";
    const char *chr_c = JS_ToCString(ctx, argv[1], &chr_buf);
    std::string chr_uuid = chr_c ? chr_c : "";

    NimBLERemoteService *svc = jsBleClient->getService(NimBLEUUID(svc_uuid));
    if (svc == nullptr) return nullptr;
    return svc->getCharacteristic(NimBLEUUID(chr_uuid));
}

JSValue native_bleRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    NimBLERemoteCharacteristic *chr = jsBleGetChr(ctx, argc, argv);
    if (chr == nullptr || !chr->canRead()) return JS_NULL;
    NimBLEAttValue val = chr->readValue();
    return JS_NewUint8ArrayCopy(ctx, val.data(), val.size());
}

JSValue native_bleWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    NimBLERemoteCharacteristic *chr = jsBleGetChr(ctx, argc, argv);
    if (chr == nullptr || argc < 3) return JS_NewBool(false);

    const uint8_t *buf = nullptr;
    size_t len = 0;
    JSCStringBuf str_buf;
    if (JS_IsString(ctx, argv[2])) {
        buf = (const uint8_t *)JS_ToCStringLen(ctx, &len, argv[2], &str_buf);
    } else if (JS_IsTypedArray(ctx, argv[2])) {
        buf = (const uint8_t *)JS_GetTypedArrayBuffer(ctx, &len, argv[2]);
    } else {
        return JS_ThrowTypeError(ctx, "ble.write: arg2 must be Uint8Array or string");
    }
    if (buf == nullptr) return JS_NewBool(false);

    bool response = chr->canWrite();
    if (argc > 3) response = JS_ToBool(ctx, argv[3]);
    return JS_NewBool(chr->writeValue(buf, len, response));
}

JSValue native_bleDisconnect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (jsBleIsConnected()) jsBleClient->disconnect();
    return JS_UNDEFINED;
}
#else
static JSValue jsBleGattUnsupported(JSContext *ctx) {
    return JS_ThrowInternalError(ctx, "ble GATT client requires NimBLE-Arduino 2.x");
}
JSValue native_bleConnect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return jsBleGattUnsupported(ctx);
}
JSValue native_bleIsConnected(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewBool(false);
}
JSValue native_bleRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return jsBleGattUnsupported(ctx);
}
JSValue native_bleWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return jsBleGattUnsupported(ctx);
}
JSValue native_bleDisconnect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_UNDEFINED;
}
#endif

#endif
