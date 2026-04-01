#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "ble_js.h"
#include "modules/ble/ble_common.h"

JSValue native_bleScan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: ble.scan(timeout_seconds)
    // returns: array of objects [{name, address, rssi}, ...]
    int timeout = 5; // default 5 seconds
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        JS_ToInt32(ctx, &timeout, argv[0]);
    }
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
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        name = JS_ToCString(ctx, argv[0], &name_buf);
    }

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

#endif
