#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "led_js.h"
#include <globals.h>

JSValue native_ledSetColor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: led.setColor(r, g, b)
    if (argc < 3) return JS_UNDEFINED;
    int r = 0, g = 0, b = 0;
    JS_ToInt32(ctx, &r, argv[0]);
    JS_ToInt32(ctx, &g, argv[1]);
    JS_ToInt32(ctx, &b, argv[2]);
    r = constrain(r, 0, 255);
    g = constrain(g, 0, 255);
    b = constrain(b, 0, 255);

#ifdef HAS_RGB_LED
    uint32_t color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    bruceConfig.setLedColor(color);
#endif
    return JS_UNDEFINED;
}

JSValue native_ledSetBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: led.setBrightness(0-255)
    if (argc < 1) return JS_UNDEFINED;
    int brightness = 0;
    JS_ToInt32(ctx, &brightness, argv[0]);
    brightness = constrain(brightness, 0, 255);

#ifdef HAS_RGB_LED
    bruceConfig.setLedBright(brightness);
#endif
    return JS_UNDEFINED;
}

JSValue native_ledOff(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#ifdef HAS_RGB_LED
    bruceConfig.setLedColor(0);
#endif
    return JS_UNDEFINED;
}

JSValue native_ledBlink(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: led.blink(delay_ms) - blink current color once
    int delayMs = 100;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) {
        JS_ToInt32(ctx, &delayMs, argv[0]);
    }
    delayMs = constrain(delayMs, 10, 5000);

#ifdef HAS_RGB_LED
    // Store current, turn off, wait, restore
    bruceConfig.setLedColor(0);
    vTaskDelay(pdMS_TO_TICKS(delayMs));
    // Restore the configured color
    bruceConfig.setLedColor(bruceConfig.ledColor);
#endif
    return JS_UNDEFINED;
}

#endif
