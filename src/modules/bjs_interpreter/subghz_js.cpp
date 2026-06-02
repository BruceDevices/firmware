#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "subghz_js.h"

#include "modules/rf/rf_scan.h"
#include "modules/rf/rf_utils.h"

#include "helpers_js.h"

// Tracks whether txSetup() has been called (raw pulse TX session is active)
static bool _txSessionActive = false;
static int _txPin = -1;

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

// ============================================================================
// Raw pulse TX API — allows JS brute-force apps to send arbitrary pulse
// sequences without per-code init/deinit overhead.
//
//   subghz.txSetup(freq_mhz)  — init CC1101 for TX at given frequency
//   subghz.txPulses(array)    — send array of signed µs durations (+HIGH/−LOW)
//   subghz.txEnd()            — deinit the RF module
// ============================================================================

JSValue native_subghzTxSetup(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: subghz.txSetup(freq_mhz : number)
    // returns: true on success, false on error
    float freq = 433.92f;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        double fv;
        JS_ToNumber(ctx, &fv, argv[0]);
        freq = (float)fv;
    }

    if (!initRfModule("tx", freq)) { return JS_NewBool(false); }

    // Determine TX pin based on module configuration
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        _txPin = bruceConfigPins.CC1101_bus.io0;
    } else {
        _txPin = bruceConfigPins.rfTx;
    }
    pinMode(_txPin, OUTPUT);
    _txSessionActive = true;
    return JS_NewBool(true);
}

JSValue native_subghzTxPulses(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: subghz.txPulses(pulses : int[])
    // pulses: array of signed integers in microseconds
    //   positive value → set pin HIGH for that many µs
    //   negative value → set pin LOW  for |value| µs
    // returns: true on success, false on error
    if (!_txSessionActive || _txPin < 0) { return JS_NewBool(false); }
    if (argc < 1) return JS_NewBool(false);

    // Get array length
    JSValue lenVal = JS_GetPropertyStr(ctx, argv[0], "length");
    int len = 0;
    JS_ToInt32(ctx, &len, lenVal);

    if (len <= 0 || len > 2048) { return JS_NewBool(false); }

    // Read pulses from JS array and transmit directly (no heap allocation
    // needed — process one element at a time to save RAM)
    for (int i = 0; i < len; i++) {
        JSValue v = JS_GetPropertyUint32(ctx, argv[0], i);
        int duration = 0;
        JS_ToInt32(ctx, &duration, v);

        if (duration > 0) {
            digitalWrite(_txPin, HIGH);
            delayMicroseconds(duration);
        } else if (duration < 0) {
            digitalWrite(_txPin, LOW);
            delayMicroseconds(-duration);
        }
    }
    // Ensure pin is LOW after transmission
    digitalWrite(_txPin, LOW);
    return JS_NewBool(true);
}

JSValue native_subghzTxEnd(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: subghz.txEnd()
    // Deinitializes the RF module after a txSetup() session
    if (_txSessionActive && _txPin >= 0) { digitalWrite(_txPin, LOW); }
    deinitRfModule();
    _txSessionActive = false;
    _txPin = -1;
    return JS_UNDEFINED;
}

#endif
