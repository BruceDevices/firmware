#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "ir_js.h"

#include "modules/ir/ir_read.h"

#include "helpers_js.h"

// Module registration is performed in `mqjs_stdlib.c`.

JSValue native_irTransmitFile(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "irTransmitFile(filename:string) required");
    JSCStringBuf sb;
    const char *filename = JS_ToCString(ctx, argv[0], &sb);
    bool hideDefaultUI = false;
    if (argc > 1 && JS_IsBool(argv[1])) hideDefaultUI = JS_ToBool(ctx, argv[1]);
    bool r = serialCli.parse(String("ir tx_from_file ") + String(filename) + " " + String(hideDefaultUI));
    return JS_NewBool(r);
}

JSValue native_irTransmit(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "irTransmit(data:string,protocol?:string,bits?:int) required");
    JSCStringBuf sb;
    const char *data = JS_ToCString(ctx, argv[0], &sb);
    const char *protocol = "NEC";
    if (argc > 1 && JS_IsString(ctx, argv[1])) {
        JSCStringBuf sb2;
        protocol = JS_ToCString(ctx, argv[1], &sb2);
    }
    int bits = 32;
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &bits, argv[2]);
    bool r = serialCli.parse(
        String("IRSend {'Data':'") + String(data) + "','Protocol':'" + String(protocol) +
        "','Bits':" + String(bits) + "}"
    );
    return JS_NewBool(r);
}

JSValue native_irRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: irRead();
    // usage: irRead(timeout_in_seconds : number);
    // returns a string of the generated ir file, empty string on timeout or other
    // errors
    // This function is registered twice in stdlib: normal and raw. The "raw"
    // variant will be a separate C function (native_irReadRaw) that passes raw=true.
    IrRead irRead = IrRead(true, 0);
    int timeout = 10;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &timeout, argv[0]);
    String result = irRead.loop_headless(timeout);
    return JS_NewString(ctx, result.c_str());
}

JSValue native_irReadRaw(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    IrRead irRead = IrRead(true, 1);
    int timeout = 10;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &timeout, argv[0]);
    String result = irRead.loop_headless(timeout);
    return JS_NewString(ctx, result.c_str());
}

#endif
