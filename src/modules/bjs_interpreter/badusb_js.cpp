#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "badusb_js.h"
#include "modules/badusb_ble/ducky_typer.h"

#include "helpers_js.h"

// #include <USBHIDConsumerControl.h>  // used for badusbPressSpecial
// USBHIDConsumerControl cc;

// Module registration is performed in `mqjs_stdlib.c`.

JSValue native_badusbRunFile(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "badusbRunFile(filename:string) required");
    JSCStringBuf sb;
    const char *fn = JS_ToCString(ctx, argv[0], &sb);
    bool r = serialCli.parse(String("badusb tx_from_file ") + String(fn));
    return JS_NewBool(r);
}

// badusb functions

JSValue native_badusbSetup(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(USB_as_HID)
    if (hid_usb != nullptr) ducky_startKb(hid_usb, false);
    return JS_NewBool(true);
#else
    return JS_NewBool(false);
#endif
}

/*
duk_ret_t native_badusbQuit(duk_context *ctx) {
  // usage: badusbQuit();
  // returns: quit keyboard mode, reinit serial port
  #if defined(USB_as_HID)
    Kb.end();
    //cc.begin();
    USB.~ESPUSB(); // Explicit call to destructor
    Serial.begin(115200);  // need to reinit serial when finished
    duk_push_boolean(ctx, true);
  #else
    duk_push_boolean(ctx, false);
  #endif
  return 1;
}
* */

JSValue native_badusbPrint(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(USB_as_HID)
    if (argc > 0 && JS_IsString(ctx, argv[0]) && hid_usb != nullptr) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[0], &sb);
        if (s) hid_usb->print(s);
    }
#endif
    return JS_UNDEFINED;
}

JSValue native_badusbPrintln(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(USB_as_HID)
    if (argc > 0 && JS_IsString(ctx, argv[0]) && hid_usb != nullptr) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[0], &sb);
        if (s) hid_usb->println(s);
    }
#endif
    return JS_UNDEFINED;
}

JSValue native_badusbPress(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(USB_as_HID)
    if (argc > 0 && JS_IsNumber(ctx, argv[0]) && hid_usb != nullptr) {
        int v = 0;
        JS_ToInt32(ctx, &v, argv[0]);
        hid_usb->press(v);
        delay(1);
        hid_usb->release(v);
    }
#endif
    return JS_UNDEFINED;
}

JSValue native_badusbHold(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(USB_as_HID)
    if (argc > 0 && JS_IsNumber(ctx, argv[0]) && hid_usb != nullptr) {
        int v = 0;
        JS_ToInt32(ctx, &v, argv[0]);
        hid_usb->press(v);
    }
#endif
    return JS_UNDEFINED;
}

JSValue native_badusbRelease(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(USB_as_HID)
    if (argc > 0 && JS_IsNumber(ctx, argv[0]) && hid_usb != nullptr) {
        int v = 0;
        JS_ToInt32(ctx, &v, argv[0]);
        hid_usb->release(v);
    }
#endif
    return JS_UNDEFINED;
}

JSValue native_badusbReleaseAll(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(USB_as_HID)
    if (hid_usb != nullptr) hid_usb->releaseAll();
#endif
    return JS_UNDEFINED;
}

JSValue native_badusbPressRaw(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(USB_as_HID)
    if (argc > 0 && JS_IsNumber(ctx, argv[0]) && hid_usb != nullptr) {
        int v = 0;
        JS_ToInt32(ctx, &v, argv[0]);
        hid_usb->pressRaw(v);
        delay(1);
        hid_usb->releaseRaw(v);
    }
#endif
    return JS_UNDEFINED;
}

/*
duk_ret_t native_badusbPressSpecial(duk_context *ctx) {
  // usage: badusbPressSpecial(keycode_number);
  // keycodes list:
https://github.com/espressif/arduino-esp32/blob/master/libraries/USB/src/USBHIDConsumerControl.h
  #if defined(USB_as_HID)
    cc.press(duk_to_int(ctx, 0));
    delay(10);
    cc.release();
    //cc.end();
  #endif
  return 0;
}
*/

#endif
