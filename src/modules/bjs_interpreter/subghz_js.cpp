#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "subghz_js.h"

#include "modules/rf/rf_scan.h"

#include "helpers_js.h"

duk_ret_t putPropSubGHzFunctions(duk_context *ctx, duk_idx_t obj_idx, uint8_t magic) {
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "setFrequency", native_subghzSetFrequency, 1, magic);
    // TODO: getFrequency
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "read", native_subghzRead, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "readRaw", native_subghzReadRaw, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "transmitFile", native_subghzTransmitFile, 1, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "transmit", native_subghzTransmit, 4, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "setup", native_noop, 0, magic);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "setIdle", native_noop, 0, magic);
    return 0;
}

duk_ret_t registerSubGHz(duk_context *ctx) {
    bduk_register_c_lightfunc(ctx, "subghzRead", native_subghzRead, 0);
    bduk_register_c_lightfunc(ctx, "subghzReadRaw", native_subghzReadRaw, 0);
    bduk_register_c_lightfunc(ctx, "subghzSetFrequency", native_subghzSetFrequency, 1);
    bduk_register_c_lightfunc(ctx, "subghzTransmitFile", native_subghzTransmitFile, 1);
    bduk_register_c_lightfunc(ctx, "subghzTransmit", native_subghzTransmit, 4);
    // bduk_register_c_lightfunc(ctx, "subghzSetIdle", native_subghzSetIdle, 1);
    return 0;
}

duk_ret_t native_subghzTransmitFile(duk_context *ctx) {
    // usage: subghzTransmitFile(filename : string, hideDefaultUI : boolean);
    // returns: bool==true on success, false on any error

    // Get the filename (required)
    const char *filename = duk_to_string(ctx, 0);

    // Default for the hideDefaultUI parameter
    bool hideDefaultUI = false;

    // Check if second argument exists and is boolean
    if (duk_get_top(ctx) > 1 && duk_is_boolean(ctx, 1)) { hideDefaultUI = duk_to_boolean(ctx, 1); }

    bool r = serialCli.parse("subghz tx_from_file " + String(filename) + " " + String(hideDefaultUI));
    duk_push_boolean(ctx, r);
    return 1;
}

duk_ret_t native_subghzTransmit(duk_context *ctx) {
    // usage: subghzTransmit(data : string, frequency : int, te : int, count : int);
    // returns: bool==true on success, false on any error
    bool r = serialCli.parse(
        "subghz tx " + String(duk_to_string(ctx, 0)) + " " + String(duk_get_uint_default(ctx, 1, 433920000)) +
        " " + String(duk_get_uint_default(ctx, 2, 174)) + " " + String(duk_get_uint_default(ctx, 3, 10))
    );
    duk_push_boolean(ctx, r);
    return 1;
}

duk_ret_t native_subghzRead(duk_context *ctx) {
    // usage: subghzRead();
    // usage: subghzRead(timeout_in_seconds : number);
    // returns a string of the generated sub file, empty string on timeout or
    // other errors (decoding failed)
    String r = "";
    if (duk_is_number(ctx, 0))
        r = RCSwitch_Read(bruceConfigPins.rfFreq, duk_to_int(ctx, 0)); // custom timeout
    else r = RCSwitch_Read(bruceConfigPins.rfFreq, 10);
    duk_push_string(ctx, r.c_str());
    return 1;
}

duk_ret_t native_subghzReadRaw(duk_context *ctx) {
    String r = "";
    if (duk_is_number(ctx, 0))
        r = RCSwitch_Read(bruceConfigPins.rfFreq, duk_to_int(ctx, 0),
                          true); // custom timeout
    else r = RCSwitch_Read(bruceConfigPins.rfFreq, 10, true);
    duk_push_string(ctx, r.c_str());
    return 1;
}

duk_ret_t native_subghzSetFrequency(duk_context *ctx) {
    // usage: subghzSetFrequency(freq_as_float);
    if (duk_is_number(ctx, 0)) bruceConfigPins.rfFreq = duk_to_number(ctx, 0); // float global var
    return 0;
}

#endif
