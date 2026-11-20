#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "modules/rf/protocols/rf_protocol_engine.h"
#include "subghz_js.h"

JSValue native_subghzProtoInfo(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewString(ctx, rf_protocol_catalog_json().c_str());
}

static JSValue protocolCall(JSContext *ctx, int argc, JSValue *argv, bool encode) {
    if (argc < 1 || !JS_IsString(ctx, argv[0]))
        return JS_NewString(ctx, "{\"ok\":false,\"error\":\"Expected a SubGhz file string\"}");
    JSCStringBuf buffer;
    const char *input = JS_ToCString(ctx, argv[0], &buffer);
    if (!input) return JS_EXCEPTION;
    String result = encode ? rf_protocol_encode_json(input) : rf_protocol_decode_json(input);
    return JS_NewString(ctx, result.c_str());
}
JSValue native_subghzProtoDecode(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return protocolCall(ctx, argc, argv, false);
}
JSValue native_subghzProtoEncode(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return protocolCall(ctx, argc, argv, true);
}
JSValue native_subghzProtoTransmit(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) return JS_NewBool(false);
    JSCStringBuf buffer;
    const char *input = JS_ToCString(ctx, argv[0], &buffer);
    if (!input) return JS_EXCEPTION;
    return JS_NewBool(rf_protocol_transmit_raw(input));
}
#endif
