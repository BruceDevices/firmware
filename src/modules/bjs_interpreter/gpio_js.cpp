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

JSValue native_analogWriteResolution(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0, resolution = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &resolution, argv[1]);
    analogWriteResolution(pin, resolution);
    return JS_UNDEFINED;
}

JSValue native_analogWriteFrequency(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0, freq = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &freq, argv[1]);
    analogWriteFrequency(pin, freq);
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

JSValue native_ledcAttach(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0, freq = 0, resolution = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &freq, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &resolution, argv[2]);
    bool result = ledcAttach(pin, freq, resolution);
    return JS_NewBool(result);
}

JSValue native_ledcWrite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0, value = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &value, argv[1]);
    bool result = ledcWrite(pin, value);
    return JS_NewBool(result);
}

JSValue native_ledcWriteTone(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0, freq = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &freq, argv[1]);
    bool result = ledcWriteTone(pin, freq);
    return JS_NewBool(result);
}
JSValue native_ledcFade(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0, start_duty = 0, target_duty = 0, max_fade_time_ms = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &start_duty, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &target_duty, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &max_fade_time_ms, argv[3]);
    bool result = ledcFade(pin, start_duty, target_duty, max_fade_time_ms);
    return JS_NewBool(result);
}
JSValue native_ledcChangeFrequency(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0, freq = 0, resolution = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &freq, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &resolution, argv[2]);
    bool result = ledcChangeFrequency(pin, freq, resolution);
    return JS_NewBool(result);
}
JSValue native_ledcDetach(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int pin = 0, freq = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &pin, argv[0]);
    bool result = ledcDetach(pin);
    return JS_NewBool(result);
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
