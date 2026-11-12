#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "i2c_js.h"

#include "core/bus_HAL.h"
#include "helpers_js.h"

// Bus leased by i2c.begin() and used by every other native_i2c_* call. Pins equal to sys_i2c share
// the system bus (its clock/timeout are restored on release); any other pins open the user bus.
// Released when the script re-runs i2c.begin() or when the interpreter finishes the script.
static I2CLease jsI2CLease;
static TwoWire *jsI2CBus = nullptr;

void i2c_js_release() {
    jsI2CLease.release();
    jsI2CBus = nullptr;
}

// Returns from the native function with a JS exception (instead of dereferencing a null bus).
#define I2C_REQUIRE_READY(ctx)                                                                     \
    do {                                                                                           \
        if (jsI2CBus == nullptr)                                                                   \
            return JS_ThrowInternalError(ctx, "i2c not initialized: call i2c.begin(sda,scl,hz) first"); \
    } while (0)

JSValue native_i2c_begin(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 3 || !JS_IsNumber(ctx, argv[0]) || !JS_IsNumber(ctx, argv[1]) || !JS_IsNumber(ctx, argv[2])) {
        return JS_ThrowTypeError(ctx, "i2c.begin(sda:int, scl:int, hz:int) is required");
    }
    int sda = 0, scl = 0;
    uint32_t hz = 0;
    JS_ToInt32(ctx, &sda, argv[0]);
    JS_ToInt32(ctx, &scl, argv[1]);
    JS_ToUint32(ctx, &hz, argv[2]);
    if (sda < 0 || scl < 0) return JS_ThrowRangeError(ctx, "i2c.begin: pins must be >= 0");
    if (hz < 1000 || hz > 1000000) return JS_ThrowRangeError(ctx, "i2c.begin: hz must be 1k..1MHz");

    // Drop the previous lease first: holding it would make a re-begin on other pins look busy.
    i2c_js_release();
    jsI2CLease = i2cLeasePins((int8_t)sda, (int8_t)scl);
    if (!jsI2CLease) {
        return JS_ThrowInternalError(
            ctx, "i2c.begin: bus unavailable (pins in use, partially on sys_i2c, or a line held low)"
        );
    }
    jsI2CLease.setClock(hz);
    jsI2CLease.setTimeOut(50);
    jsI2CBus = jsI2CLease.wire();
    return JS_NewBool(true);
}

JSValue native_i2c_scan(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    I2C_REQUIRE_READY(ctx);
    JSValue arr = JS_NewArray(ctx, 127);
    uint32_t idx = 0;
    for (uint8_t a = 1; a < 127; a++) {
        jsI2CBus->beginTransmission(a);
        if (jsI2CBus->endTransmission(true) == 0) { JS_SetPropertyUint32(ctx, arr, idx++, JS_NewInt32(ctx, a)); }
    }
    return arr;
}

JSValue native_i2c_write(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    I2C_REQUIRE_READY(ctx);
    int addr = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &addr, argv[0]);
    std::vector<uint8_t> tmp;
    const uint8_t *buf = NULL;
    size_t len = 0;
    if (argc < 2) return JS_ThrowTypeError(ctx, "i2c.write: arg1 must be Uint8Array or string");

    if (JS_IsString(ctx, argv[1])) {
        JSCStringBuf sb;
        const char *s = JS_ToCStringLen(ctx, &len, argv[1], &sb);
        if (!s) return JS_ThrowTypeError(ctx, "i2c.write: invalid string argument");
        tmp.assign((const uint8_t *)s, (const uint8_t *)s + len);
        buf = tmp.data();
    } else if (JS_IsTypedArray(ctx, argv[1])) {
        const char *b = JS_GetTypedArrayBuffer(ctx, &len, argv[1]);
        if (!b) return JS_ThrowTypeError(ctx, "i2c.write: invalid typed array argument");
        buf = (const uint8_t *)b;
    } else {
        return JS_ThrowTypeError(ctx, "i2c.write: arg1 must be Uint8Array or string");
    }
    bool sendStop = true;
    if (argc > 2) sendStop = JS_ToBool(ctx, argv[2]);

    jsI2CBus->beginTransmission((uint8_t)addr);
    jsI2CBus->write(buf, (size_t)len);
    uint8_t err = jsI2CBus->endTransmission(sendStop);
    return JS_NewInt32(ctx, err);
}

JSValue native_i2c_read(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    I2C_REQUIRE_READY(ctx);
    int addr = 0;
    int len = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &addr, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &len, argv[1]);
    size_t got = jsI2CBus->requestFrom((uint8_t)addr, (uint8_t)len, (uint8_t)true);
    std::vector<uint8_t> buf(got);
    for (size_t i = 0; i < got && jsI2CBus->available(); i++) buf[i] = jsI2CBus->read();
    // TODO: Create JS_NewArrayBufferCopy in MicroQuickJS
    // JSValue arrbuf = JS_NewArrayBufferCopy(ctx, (const uint8_t *)buf.data(), got);
    // return arrbuf;
    return JS_ThrowInternalError(ctx, "i2c.read: not implemented");
}

JSValue native_i2c_write_read(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    I2C_REQUIRE_READY(ctx);
    int addr = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &addr, argv[0]);
    std::vector<uint8_t> tmp;
    const uint8_t *wbuf = NULL;
    size_t wlen = 0;
    if (argc < 3) return JS_ThrowTypeError(ctx, "i2c.writeRead: arg1 must be Uint8Array or string");

    if (JS_IsString(ctx, argv[1])) {
        JSCStringBuf sb;
        const char *s = JS_ToCStringLen(ctx, &wlen, argv[1], &sb);
        if (!s) return JS_ThrowTypeError(ctx, "i2c.writeRead: invalid string argument");
        tmp.assign((const uint8_t *)s, (const uint8_t *)s + wlen);
        wbuf = tmp.data();
    } else if (JS_IsTypedArray(ctx, argv[1])) {
        const char *b = JS_GetTypedArrayBuffer(ctx, &wlen, argv[1]);
        if (!b) return JS_ThrowTypeError(ctx, "i2c.writeRead: invalid typed array argument");
        wbuf = (const uint8_t *)b;
    } else {
        return JS_ThrowTypeError(ctx, "i2c.writeRead: arg1 must be Uint8Array or string");
    }
    int rlen = 0;
    if (JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &rlen, argv[2]);
    int wait = 0;
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &wait, argv[3]);

    jsI2CBus->beginTransmission((uint8_t)addr);
    jsI2CBus->write(wbuf, (size_t)wlen);
    uint8_t err = jsI2CBus->endTransmission(false);
    if (err != 0) { return JS_NewInt32(ctx, -err); }
    if (wait > 0) delay(wait);
    size_t got = jsI2CBus->requestFrom((uint8_t)addr, (uint8_t)rlen, (uint8_t)true);
    std::vector<uint8_t> buf(got);
    for (size_t i = 0; i < got && jsI2CBus->available(); i++) buf[i] = jsI2CBus->read();
    // TODO: Create JS_NewArrayBufferCopy in MicroQuickJS
    // JSValue arrbuf = JS_NewArrayBufferCopy(ctx, (const uint8_t *)buf.data(), got);
    // return arrbuf;
    return JS_ThrowInternalError(ctx, "i2c.writeRead: not implemented");
}

#endif
