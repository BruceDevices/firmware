#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "subghz_js.h"

#include "modules/rf/rf_scan.h"

#include "helpers_js.h"

JSValue native_subghzTransmitFile(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: subghzTransmitFile(filename : string, hideDefaultUI : boolean);
    // returns: bool==true on success, false on any error

    const char *filename = NULL;
    JSCStringBuf filename_buf;
    if (argc > 0 && JS_IsString(ctx, argv[0])) filename = JS_ToCString(ctx, argv[0], &filename_buf);

    bool hideDefaultUI = false;
    if (argc > 1 && JS_IsBool(argv[1])) hideDefaultUI = JS_ToBool(ctx, argv[1]);

    bool r = false;
    if (filename != NULL) {
        r = serialCli.parse("subghz tx_from_file " + String(filename) + " " + String(hideDefaultUI));
    }

    return JS_NewBool(r);
}

JSValue native_subghzTransmit(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: subghzTransmit(data : string, frequency : int, te : int, count : int);
    // returns: bool==true on success, false on any error
    const char *data = NULL;
    JSCStringBuf data_buf;
    if (argc > 0 && JS_IsString(ctx, argv[0])) data = JS_ToCString(ctx, argv[0], &data_buf);

    uint32_t freq = 433920000;
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) {
        double fv;
        JS_ToNumber(ctx, &fv, argv[1]);
        freq = (uint32_t)fv;
    }

    uint32_t te = 174;
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) {
        int tmp;
        JS_ToInt32(ctx, &tmp, argv[2]);
        te = tmp;
    }

    uint32_t count = 10;
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) {
        int tmp;
        JS_ToInt32(ctx, &tmp, argv[3]);
        count = tmp;
    }

    bool r = false;
    if (data != NULL) {
        r = serialCli.parse(
            "subghz tx " + String(data) + " " + String(freq) + " " + String(te) + " " + String(count)
        );
    }

    return JS_NewBool(r);
}

JSValue native_subghzRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: subghzRead();
    // usage: subghzRead(timeout_in_seconds : number);
    // returns a string of the generated sub file, empty string on timeout or other errors
    String r = "";
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        int t;
        JS_ToInt32(ctx, &t, argv[0]);
        r = RCSwitch_Read(bruceConfigPins.rfFreq, t, false, true); // headless mode for JS
    } else {
        r = RCSwitch_Read(bruceConfigPins.rfFreq, 10, false, true);
    }
    return JS_NewString(ctx, r.c_str());
}

JSValue native_subghzReadRaw(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    String r = "";
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        int t;
        JS_ToInt32(ctx, &t, argv[0]);
        r = RCSwitch_Read(bruceConfigPins.rfFreq, t, true, true); // raw + headless for JS
    } else {
        r = RCSwitch_Read(bruceConfigPins.rfFreq, 10, true, true);
    }
    return JS_NewString(ctx, r.c_str());
}

JSValue native_subghzSetFrequency(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: subghzSetFrequency(freq_as_float);
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        double v;
        JS_ToNumber(ctx, &v, argv[0]);
        bruceConfigPins.rfFreq = v; // float global var
    }
    return JS_UNDEFINED;
}

#endif
