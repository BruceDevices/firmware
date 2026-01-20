#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "gpio_js.h"

#include "helpers_js.h"

JSValue native_digitalWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    else if (argc > 0 && JS_IsString(ctx, argv[0])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[0], &sb);
        if (s && s[0] == 'G') pin = atoi(&s[1]);
    }

    bool val = false;
    if (argc > 1) val = JS_ToBool(ctx, argv[1]);
    digitalWrite(pin, val);
    return JS_UNDEFINED;
}

JSValue native_analogWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0, v = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &v, argv[1]);
    analogWrite(pin, v);
    return JS_UNDEFINED;
}

JSValue native_digitalRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    int val = digitalRead(pin);
    return JS_NewInt32(ctx, val);
}

JSValue native_analogRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    int val = analogRead(pin);
    return JS_NewInt32(ctx, val);
}

JSValue native_touchRead(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if SOC_TOUCH_SENSOR_SUPPORTED
    int pin = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    int val = touchRead(pin);
    return JS_NewInt32(ctx, val);
#else
    return JS_ThrowTypeError(ctx, "%s function not supported on this device", "gpio.touchRead()");
#endif
}

JSValue native_dacWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(SOC_DAC_SUPPORTED)
    int pin = 0, value = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &value, argv[1]);
    dacWrite(pin, value);
    return JS_UNDEFINED;
#else
    return JS_ThrowTypeError(ctx, "%s function not supported on this device", "gpio.dacWrite()");
#endif
}

JSValue native_ledcSetup(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int ch = 0, freq = 50, duty = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &ch, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &freq, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &duty, argv[2]);
    int val = ledcAttach(ch, freq, duty);
    return JS_NewInt32(ctx, val);
}

JSValue native_ledcAttachPin(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0, ch = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &ch, argv[1]);
    ledcAttach(pin, 50, ch);
    return JS_UNDEFINED;
}

JSValue native_ledcWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int ch = 0, value = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &ch, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &value, argv[1]);
    ledcWrite(ch, value);
    return JS_UNDEFINED;
}

JSValue native_pinMode(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = -1;
    int mode = INPUT;

    if (argc > 0) {
        if (JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
        else if (JS_IsString(ctx, argv[0])) {
            JSCStringBuf sb;
            const char *s = JS_ToCString(ctx, argv[0], &sb);
            if (s && s[0] == 'G') pin = atoi(&s[1]);
        }
    }

    if (pin < 0) return JS_ThrowTypeError(ctx, "gpio.pinMode(): invalid pin");

    if (argc > 1) {
        if (JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &mode, argv[1]);
        else if (JS_IsString(ctx, argv[1])) {
            JSCStringBuf msb;
            const char *ms = JS_ToCString(ctx, argv[1], &msb);
            JSCStringBuf psb;
            const char *ps = NULL;
            if (argc > 2 && JS_IsString(ctx, argv[2])) ps = JS_ToCString(ctx, argv[2], &psb);

            if (ms) {
                if (strcmp(ms, "input") == 0 || strcmp(ms, "analog") == 0) {
                    if (ps && strcmp(ps, "up") == 0) mode = INPUT_PULLUP;
                    else if (ps && strcmp(ps, "down") == 0) mode = INPUT_PULLDOWN;
                    else mode = INPUT;
                } else if (strncmp(ms, "output", 6) == 0) {
                    mode = OUTPUT;
                }
            }
        }
    }

    pinMode(pin, mode);
    return JS_UNDEFINED;
}

JSValue native_pins(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "grove_sda", JS_NewInt32(ctx, bruceConfigPins.i2c_bus.sda));
    JS_SetPropertyStr(ctx, obj, "grove_scl", JS_NewInt32(ctx, bruceConfigPins.i2c_bus.scl));
    JS_SetPropertyStr(ctx, obj, "serial_tx", JS_NewInt32(ctx, bruceConfigPins.uart_bus.tx));
    JS_SetPropertyStr(ctx, obj, "serial_rx", JS_NewInt32(ctx, bruceConfigPins.uart_bus.rx));
    JS_SetPropertyStr(ctx, obj, "spi_sck", JS_NewInt32(ctx, SPI_SCK_PIN));
    JS_SetPropertyStr(ctx, obj, "spi_mosi", JS_NewInt32(ctx, SPI_MOSI_PIN));
    JS_SetPropertyStr(ctx, obj, "spi_miso", JS_NewInt32(ctx, SPI_MISO_PIN));
    JS_SetPropertyStr(ctx, obj, "spi_ss", JS_NewInt32(ctx, SPI_SS_PIN));
    JS_SetPropertyStr(ctx, obj, "ir_tx", JS_NewInt32(ctx, TXLED));
    JS_SetPropertyStr(ctx, obj, "ir_rx", JS_NewInt32(ctx, RXLED));
    return obj;
}
#endif
