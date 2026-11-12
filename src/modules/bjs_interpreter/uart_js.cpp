#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "uart_js.h"

#include <HardwareSerial.h>
#include <vector>

namespace {
bool uartReady = false;

bool requireUart(JSContext *ctx) {
    if (uartReady) return true;
    JS_ThrowInternalError(ctx, "uart not initialized: call uart.begin(baud, rxPin, txPin) first");
    return false;
}
} // namespace

JSValue native_uart_begin(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 3 || !JS_IsNumber(ctx, argv[0]) || !JS_IsNumber(ctx, argv[1]) ||
        !JS_IsNumber(ctx, argv[2])) {
        return JS_ThrowTypeError(ctx, "uart.begin(baud:int, rxPin:int, txPin:int) is required");
    }

    uint32_t baud = 0;
    int rxPin = -1;
    int txPin = -1;
    JS_ToUint32(ctx, &baud, argv[0]);
    JS_ToInt32(ctx, &rxPin, argv[1]);
    JS_ToInt32(ctx, &txPin, argv[2]);
    if (baud < 300 || baud > 5000000) return JS_ThrowRangeError(ctx, "uart.begin: baud must be 300..5000000");
    if (rxPin < 0 || txPin < 0) return JS_ThrowRangeError(ctx, "uart.begin: pins must be >= 0");

    if (uartReady) Serial1.end();
    Serial1.setRxBufferSize(1024);
    Serial1.begin(baud, SERIAL_8N1, rxPin, txPin);
    uartReady = true;
    return JS_NewBool(true);
}

JSValue native_uart_write(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (!requireUart(ctx)) return JS_EXCEPTION;
    if (argc < 1) return JS_ThrowTypeError(ctx, "uart.write: byte array, Uint8Array, or string required");

    const uint8_t *data = nullptr;
    size_t length = 0;
    std::vector<uint8_t> bytes;
    JSCStringBuf stringBuffer;

    if (JS_IsString(ctx, argv[0])) {
        const char *stringData = JS_ToCStringLen(ctx, &length, argv[0], &stringBuffer);
        if (!stringData) return JS_ThrowTypeError(ctx, "uart.write: invalid string");
        data = reinterpret_cast<const uint8_t *>(stringData);
    } else if (JS_IsTypedArray(ctx, argv[0])) {
        const char *typedData = JS_GetTypedArrayBuffer(ctx, &length, argv[0]);
        if (!typedData) return JS_ThrowTypeError(ctx, "uart.write: invalid typed array");
        data = reinterpret_cast<const uint8_t *>(typedData);
    } else if (JS_IsObject(ctx, argv[0]) && JS_GetClassID(ctx, argv[0]) == JS_CLASS_ARRAY) {
        JSValue lengthValue = JS_GetPropertyStr(ctx, argv[0], "length");
        uint32_t arrayLength = 0;
        if (!JS_IsNumber(ctx, lengthValue) || JS_ToUint32(ctx, &arrayLength, lengthValue) < 0) {
            return JS_ThrowTypeError(ctx, "uart.write: invalid byte array");
        }
        if (arrayLength > 4096) return JS_ThrowRangeError(ctx, "uart.write: maximum length is 4096 bytes");
        bytes.reserve(arrayLength);
        for (uint32_t i = 0; i < arrayLength; ++i) {
            JSValue item = JS_GetPropertyUint32(ctx, argv[0], i);
            int value = 0;
            if (!JS_IsNumber(ctx, item) || JS_ToInt32(ctx, &value, item) < 0 || value < 0 || value > 255) {
                return JS_ThrowRangeError(ctx, "uart.write: array values must be bytes (0..255)");
            }
            bytes.push_back(static_cast<uint8_t>(value));
        }
        data = bytes.data();
        length = bytes.size();
    } else {
        return JS_ThrowTypeError(ctx, "uart.write: byte array, Uint8Array, or string required");
    }

    return JS_NewInt32(ctx, static_cast<int32_t>(Serial1.write(data, length)));
}

JSValue native_uart_read(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (!requireUart(ctx)) return JS_EXCEPTION;
    int maximum = 256;
    if (argc > 0) {
        if (!JS_IsNumber(ctx, argv[0]) || JS_ToInt32(ctx, &maximum, argv[0]) < 0) {
            return JS_ThrowTypeError(ctx, "uart.read: maxLength must be an integer");
        }
    }
    if (maximum < 0 || maximum > 4096) return JS_ThrowRangeError(ctx, "uart.read: maxLength must be 0..4096");

    JSValue result = JS_NewArray(ctx, 0);
    uint32_t index = 0;
    while (index < static_cast<uint32_t>(maximum) && Serial1.available()) {
        JS_SetPropertyUint32(ctx, result, index++, JS_NewInt32(ctx, Serial1.read()));
    }
    return result;
}

JSValue native_uart_available(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!requireUart(ctx)) return JS_EXCEPTION;
    return JS_NewInt32(ctx, Serial1.available());
}

JSValue native_uart_flush(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!requireUart(ctx)) return JS_EXCEPTION;
    Serial1.flush();
    return JS_UNDEFINED;
}

JSValue native_uart_end(JSContext *, JSValue *, int, JSValue *) {
    if (uartReady) Serial1.end();
    uartReady = false;
    return JS_UNDEFINED;
}

#endif
